/* ab_l1.c: the per-value layout of V0' (the control), V1, V2 and V4.  #included after ab_core.c.
 *
 * An array holds, per element, its numerator, its cached denominator (0: bad), its edge count and an offset into
 * the array's edge pool; the edges themselves are packed u32s in that pool.  Results are written in index order,
 * so an output array must not alias an input (the driver uses a second array for chains).
 *
 *   V0'  every op merges the operands' edges, scales, and reduces its result (column sum: one graph add per element)
 *   V1   V0' + core_add / core_mul check the cached denominators first (equal: one integer op; one divides the
 *        other: one scaling) [in ab_core.c]
 *   V2   V0' + the column sum buckets numerators by denominator in one pass (i128, checked), reduces each bucket
 *        and combines them with V0'  reducing graph adds; the buckets are freed when the sum returns
 *   V4   V0' + no reduction at entry or inside the expression; an overflow reduces the operands and retries with
 *        V0'  reducing op; ab_final reduces every result once (timed)
 */
typedef struct ab_arr {
    size_t n;
    anum *num;
    aden *den;
    uint8_t *ne;
    uint32_t *eo;
    uint32_t *e;
    size_t elen, ecap;
} ab_arr;

static ab_arr *arrs[16];
static int narrs;
static uint64_t st_early, st_buckets;  /* V4: early reduce-and-retries; V2: largest bucket count of a sum */

static size_t arr_n(const struct ab_arr *a) { return a->n; }
static ab_arr *ab_new(size_t n) {
    ab_arr *a = xcalloc(sizeof *a);
    a->n = n;
    a->num = xmalloc(n * sizeof(anum) + 8);
    a->den = xmalloc(n * sizeof(aden) + 8);
    a->ne = xmalloc(n + 8);
    a->eo = xmalloc(n * 4 + 8);
    a->ecap = 2 * n + 16;
    a->e = xmalloc(a->ecap * 4);
    if (narrs == 16) die("too many arrays");
    arrs[narrs++] = a;
    return a;
}
#if VARIANT == VAR_V6
static void v6_reset(void);
static void v6_fill_end(void);
static void v6_fill_arr(const ab_arr *a);
static void v6_fill_split(size_t i);
static void v6_stats(FILE *f);
static const char *v6_audit(ab_arr **L, int nl, const char *corrupt);
static void layout_reset(void) { st_early = 0; st_buckets = 0; v6_reset(); }
#else
static void layout_reset(void) { st_early = 0; st_buckets = 0; }
#endif
static void ab_init(void) {}

static inline view l1v(const ab_arr *a, size_t i) { return (view){a->num[i], a->den[i], a->ne[i], a->e + a->eo[i]}; }
static inline void l1put(ab_arr *a, size_t i, const raw *r) {
    if (a->elen + (size_t)r->ne > a->ecap) {
        a->ecap = 2 * a->ecap + (size_t)r->ne;
        a->e = xrealloc(a->e, a->ecap * 4);
    }
    a->num[i] = r->num; a->den[i] = r->den; a->ne[i] = (uint8_t)r->ne; a->eo[i] = (uint32_t)a->elen;
    memcpy(a->e + a->elen, r->e, sizeof(uint32_t) * r->ne);
    a->elen += (size_t)r->ne;
}

static void fill_begin(void) {}
#if VARIANT == VAR_V6
static void fill_end(void) { v6_fill_end(); }  /* the edges of nodes split during the fill are rewritten (timed) */
#else
static void fill_end(void) {}
#endif
static void fill(ab_arr *a, const int64_t *num, const uint32_t *di, int single) {
    (void)single;
    a->elen = 0;
    raw r;
#if VARIANT == VAR_V6
    uint32_t ep = v6_epoch;
    v6_fill_arr(a);
#endif
    for (size_t i = 0; i < a->n; i++) {
        const dden *D = &dd[di[i]];
        if (D->bad) setbad(&r);
        else {
#if VARIANT == VAR_V4
            r.num = num[i]; r.den = D->den; r.ne = D->ne;  /* entered as given: not reduced */
            memcpy(r.e, D->e, sizeof(uint32_t) * D->ne);
#elif defined(AB_32)
            if (!fin((awide)num[i], D->den, D->e, D->ne, 1, &r)) { ovf(); setbad(&r); }  /* beyond int32 */
#else
            fin((i128)num[i], D->den, D->e, D->ne, 1, &r);  /* entry reduction, as the prototype does it */
#endif
        }
        l1put(a, i, &r);
#if VARIANT == VAR_V6
        if (v6_epoch != ep) { v6_fill_split(i); ep = v6_epoch; }  /* elements before i may name a dead node */
#endif
    }
}

/* ---------------------------------------------------------------- the element ops */

#if VARIANT == VAR_V4
static inline int op_add(view a, view b, raw *r) {
    if (core_add(a, b, 0, 0, r)) return 1;  /* unreduced */
    st_early++;                              /* overflow: reduce the operands, retry with the reducing op */
    raw ra, rb;
    if (!fin((i128)a.num, a.den, a.e, a.ne, 1, &ra) || !fin((i128)b.num, b.den, b.e, b.ne, 1, &rb)) die("reduce grew");
    return core_add(vraw(&ra), vraw(&rb), 0, 1, r);
}
static inline int op_mul(view a, view b, raw *r) {
    if (core_mul(a, b, 0, r)) return 1;
    st_early++;
    raw ra, rb;
    if (!fin((i128)a.num, a.den, a.e, a.ne, 1, &ra) || !fin((i128)b.num, b.den, b.e, b.ne, 1, &rb)) die("reduce grew");
    return core_mul(vraw(&ra), vraw(&rb), 1, r);
}
#else
static inline int op_add(view a, view b, raw *r) { return core_add(a, b, 0, 1, r); }
static inline int op_mul(view a, view b, raw *r) { return core_mul(a, b, 1, r); }
#endif

static void ab_add(ab_arr *R, const ab_arr *A, const ab_arr *B) {
    R->elen = 0;
    raw r;
    for (size_t i = 0; i < R->n; i++) {
        if (!op_add(l1v(A, i), l1v(B, i), &r)) ovf();
        l1put(R, i, &r);
    }
}
static void ab_mul(ab_arr *R, const ab_arr *A, const ab_arr *B) {
    R->elen = 0;
    raw r;
    for (size_t i = 0; i < R->n; i++) {
        if (!op_mul(l1v(A, i), l1v(B, i), &r)) ovf();
        l1put(R, i, &r);
    }
}
static void ab_fma(ab_arr *R, const ab_arr *A, const ab_arr *B, const ab_arr *C) {
    R->elen = 0;
    raw t, r;
    for (size_t i = 0; i < R->n; i++) {
        if (!op_mul(l1v(A, i), l1v(B, i), &t)) { ovf(); setbad(&r); }
        else if (!op_add(vraw(&t), l1v(C, i), &r)) ovf();
        l1put(R, i, &r);
    }
}

#if VARIANT == VAR_V2
typedef struct { uint64_t den; i128 acc; int ne; const uint32_t *e; } bucket;
static void ab_sum(ab_arr *S, const ab_arr *A) {
    size_t cap = 64, cnt = 0, nb = 0, bcap = 32;
    uint64_t *hk = xcalloc(cap * 8);
    uint32_t *hv = xmalloc(cap * 4);
    bucket *B = xmalloc(bcap * sizeof *B);
    raw r;
    int bad = 0;
    uint64_t last = 0;
    bucket *lb = NULL;
    for (size_t i = 0; i < A->n && !bad; i++) {  /* one pass: each numerator into its denominator's bucket */
        uint64_t d = A->den[i];
        if (!d) { bad = 1; break; }
        if (d != last) {
            size_t h = hsh(d) & (cap - 1);
            while (hk[h] && hk[h] != d) h = (h + 1) & (cap - 1);
            if (!hk[h]) {
                if (2 * (cnt + 1) > cap) {  /* grow and re-probe */
                    size_t oc = cap;
                    uint64_t *ok = hk;
                    uint32_t *ov = hv;
                    cap *= 2;
                    hk = xcalloc(cap * 8); hv = xmalloc(cap * 4);
                    for (size_t j = 0; j < oc; j++)
                        if (ok[j]) {
                            size_t g = hsh(ok[j]) & (cap - 1);
                            while (hk[g]) g = (g + 1) & (cap - 1);
                            hk[g] = ok[j]; hv[g] = ov[j];
                        }
                    xfree(ok); xfree(ov);
                    h = hsh(d) & (cap - 1);
                    while (hk[h]) h = (h + 1) & (cap - 1);
                }
                if (nb == bcap) { bcap *= 2; B = xrealloc(B, bcap * sizeof *B); }
                B[nb] = (bucket){d, 0, A->ne[i], A->e + A->eo[i]};
                hk[h] = d; hv[h] = (uint32_t)nb++; cnt++;
            }
            lb = &B[hv[h]];
            last = d;
        }
        if (__builtin_add_overflow(lb->acc, (i128)A->num[i], &lb->acc)) { ovf(); bad = 1; }
    }
    if (nb > st_buckets) st_buckets = nb;
    /* combine: each bucket reduced to lowest terms, then V0'  reducing graph adds (so V2 differs from V0' only in
     * the bucketing, with no deferred reduction) */
    raw buf[2], bv;
    int cur = 0;
    buf[0].num = 0; buf[0].den = 1; buf[0].ne = 0;
    for (size_t k = 0; k < nb && !bad; k++) {
        if (!B[k].acc) continue;
        if (!fin(B[k].acc, B[k].den, B[k].e, B[k].ne, 1, &bv)) { ovf(); bad = 1; break; }
        if (!core_add(vraw(&buf[cur]), vraw(&bv), 0, 1, &buf[cur ^ 1])) { ovf(); bad = 1; break; }
        cur ^= 1;
    }
    if (bad) setbad(&r);
    else r = buf[cur];
    xfree(hk); xfree(hv); xfree(B);
    S->elen = 0;
    l1put(S, 0, &r);
}
#else
static void ab_sum(ab_arr *S, const ab_arr *A) {  /* one graph add per element (V4: unreduced, retried on overflow) */
    raw buf[2];
    int cur = 0;
    buf[0].num = 0; buf[0].den = 1; buf[0].ne = 0;
    for (size_t i = 0; i < A->n; i++) {
        if (!op_add(vraw(&buf[cur]), l1v(A, i), &buf[cur ^ 1])) ovf();
        cur ^= 1;
        if (!buf[cur].den) break;
    }
    S->elen = 0;
    l1put(S, 0, &buf[cur]);
}
#endif

static void ab_final(ab_arr *R) {
#if VARIANT == VAR_V4
    raw r;
    for (size_t i = 0; i < R->n; i++) {  /* the one reduction, in place (edges only shrink) */
        if (!R->den[i]) continue;
        uint32_t *e = R->e + R->eo[i];
        if (!fin((i128)R->num[i], R->den[i], e, R->ne[i], 1, &r)) die("reduce grew");
        R->num[i] = r.num; R->den[i] = r.den; R->ne[i] = (uint8_t)r.ne;
        memcpy(e, r.e, sizeof(uint32_t) * r.ne);
    }
#else
    (void)R;
#endif
}
static inline void ab_get(const ab_arr *a, size_t i, int64_t *num, uint64_t *den) { *num = a->num[i]; *den = a->den[i]; }

/* Structural audit of the arrays written in the last rep: every good element's edges are ascending, on live nodes,
 * with positive weights, and multiply to its cached denominator. */
static const char *ab_audit(ab_arr **L, int nl, const char *corrupt) {
#if VARIANT == VAR_V6
    const char *m6 = v6_audit(L, nl, corrupt);  /* re-expands edges after splits (untimed), checks the graph */
    if (m6) return m6;
#endif
    if (corrupt && !strcmp(corrupt, "den") && nl && L[0]->n && L[0]->den[0]) L[0]->den[0] += 1;
    for (int k = 0; k < nl; k++)
        for (size_t i = 0; i < L[k]->n; i++) {
            if (!L[k]->den[i]) continue;
            const char *m = audit_edges(L[k]->den[i], L[k]->ne[i], L[k]->e + L[k]->eo[i]);
            if (m) return m;
        }
    return NULL;
}
static void ab_stats(FILE *f) {
#if VARIANT == VAR_V4
    fprintf(f, " early=%llu", (unsigned long long)st_early);
#elif VARIANT == VAR_V2
    fprintf(f, " buckets=%llu", (unsigned long long)st_buckets);
#elif VARIANT == VAR_V6
    v6_stats(f);
#else
    (void)f;
#endif
}
