/* ab_v3.c: V3 = V0' + an array-level shared part with sparse exceptions.  #included after ab_core.c.
 *
 * An array holds one array denominator D (with its edges) and, per element, a numerator scaled to D.  Elements that
 * cannot be written over D are exceptions: a sorted list of (index, value), where the value is the element in
 * lowest terms over its own denominator (whose edges go beyond D), and the dense slot holds 0.
 *
 * Choosing D at entry (timed): C = the lcm of the array's denominators if it is below 2^63 (policy "lcm", the
 * default), else the edges of its most frequent denominator; policy "freq" (env AB_V3_POLICY=freq) always takes
 * the most frequent.  D = the lcm of the array's denominators that divide C.  An element is an exception if its
 * denominator does not divide D, or if its numerator scaled to D overflows int64.
 *
 * Elementwise ops compute the result's D once per op (lcm for +, product for *, both for a*b + c), then run a
 * dense, checked int64 loop over every slot.  Exceptions of any operand, and slots whose dense arithmetic
 * overflowed, then take the graph path: each operand as its own lowest-terms value, V0'  reducing op, and the
 * result is written densely if its denominator divides the result's D (and the scaled numerator fits), else as an
 * exception.  If the result's D would reach 2^63, every element takes the graph path.  The column sum adds the
 * dense numerators in i128, adds the exceptions with V0' graph adds, combines and reduces once.
 *
 * Single values (W5: add, mul) are entered "plain": every element an exception, so V3 is V0' there.
 *
 * Outputs: ab_final reduces each dense element n/D to lowest terms (timed, the final-reduction phase).
 */
typedef struct { int64_t num; uint64_t den; uint8_t ne; uint32_t e[AB_MAXE]; } xval;  /* den 0: bad */
typedef struct ab_arr {
    size_t n;
    int plain;
    uint64_t D;
    int dne;
    uint32_t de[AB_W];
    int64_t *num;
    size_t nx, xcap;
    uint32_t *xi;
    xval *xv;
    int64_t *on;   /* the final reduction's output */
    uint64_t *od;
    int fin;
} ab_arr;

static ab_arr *arrs[16];
static int narrs;
static int v3_freq;  /* policy */
static uint64_t st_exc_in, st_dense_ovf, st_allslow;

static size_t arr_n(const struct ab_arr *a) { return a->n; }
static ab_arr *ab_new(size_t n) {
    ab_arr *a = xcalloc(sizeof *a);
    a->n = n;
    a->num = xmalloc(n * 8 + 8);
    a->xcap = 16;
    a->xi = xmalloc(a->xcap * 4);
    a->xv = xmalloc(a->xcap * sizeof(xval));
    a->D = 1;
    if (narrs == 16) die("too many arrays");
    arrs[narrs++] = a;
    return a;
}
static void layout_reset(void) { st_exc_in = 0; st_dense_ovf = 0; st_allslow = 0; }
static void ab_init(void) {
    const char *p = getenv("AB_V3_POLICY");
    v3_freq = p && !strcmp(p, "freq");
}

static inline void xpush(ab_arr *a, size_t i, const raw *r) {
    if (a->nx == a->xcap) {
        a->xcap *= 2;
        a->xi = xrealloc(a->xi, a->xcap * 4);
        a->xv = xrealloc(a->xv, a->xcap * sizeof(xval));
    }
    xval *x = &a->xv[a->nx];
    x->num = r->num; x->den = r->den; x->ne = (uint8_t)r->ne;
    memcpy(x->e, r->e, sizeof(uint32_t) * r->ne);
    a->xi[a->nx++] = (uint32_t)i;
}
static inline view xview(const xval *x) { return (view){x->num, x->den, x->ne, x->e}; }
static inline view dview(const ab_arr *a, size_t i, raw *t) {  /* a dense element as its own lowest-terms value */
    fin((i128)a->num[i], a->D, a->de, a->dne, 1, t);
    return vraw(t);
}
static inline view aview(const ab_arr *a) { return (view){0, a->D, a->dne, a->de}; }

/* ---------------------------------------------------------------- entry */

static void fill_begin(void) {}
static void fill_end(void) {}
static void fill(ab_arr *a, const int64_t *num, const uint32_t *di, int single) {
    a->nx = 0; a->fin = 0;
    raw r;
    if (single) {
        a->plain = 1; a->D = 1; a->dne = 0;
        for (size_t i = 0; i < a->n; i++) {
            const dden *D = &dd[di[i]];
            if (D->bad) setbad(&r);
            else fin((i128)num[i], D->den, D->e, D->ne, 1, &r);
            a->num[i] = 0;
            xpush(a, i, &r);
        }
        st_exc_in += a->nx;
        return;
    }
    a->plain = 0;
    uint32_t *cnt = xcalloc((size_t)ndd * 4 + 4);
    for (size_t i = 0; i < a->n; i++) cnt[di[i]]++;
    uint8_t *wc = xcalloc(nnodes + 1), *wd = xcalloc(nnodes + 1);
    int fits = !v3_freq;
    if (fits) {  /* C = the lcm, if below 2^63 */
        for (uint32_t k = 0; k < ndd; k++)
            if (cnt[k] && !dd[k].bad)
                for (int j = 0; j < dd[k].ne; j++)
                    if (EW(dd[k].e[j]) > wc[EN(dd[k].e[j])]) wc[EN(dd[k].e[j])] = (uint8_t)EW(dd[k].e[j]);
        uint64_t L = 1;
        for (uint32_t id = 0; id < nnodes && fits; id++)
            if (wc[id] && (!mulpow(&L, nodes[id].v, wc[id]) || (L >> 63))) fits = 0;
    }
    if (!fits) {  /* else C = the most frequent denominator */
        memset(wc, 0, nnodes + 1);
        uint32_t best = UINT32_MAX;
        for (uint32_t k = 0; k < ndd; k++)
            if (cnt[k] && !dd[k].bad && (best == UINT32_MAX || cnt[k] > cnt[best])) best = k;
        if (best != UINT32_MAX)
            for (int j = 0; j < dd[best].ne; j++) wc[EN(dd[best].e[j])] = (uint8_t)EW(dd[best].e[j]);
    }
    for (uint32_t k = 0; k < ndd; k++) {  /* D = the lcm of the denominators that divide C */
        if (!cnt[k] || dd[k].bad) continue;
        int div = 1;
        for (int j = 0; j < dd[k].ne && div; j++) div = EW(dd[k].e[j]) <= wc[EN(dd[k].e[j])];
        if (div)
            for (int j = 0; j < dd[k].ne; j++)
                if (EW(dd[k].e[j]) > wd[EN(dd[k].e[j])]) wd[EN(dd[k].e[j])] = (uint8_t)EW(dd[k].e[j]);
    }
    uint64_t D = 1;
    a->dne = 0;
    for (uint32_t id = 0; id < nnodes; id++)
        if (wd[id]) {
            if (!mulpow(&D, nodes[id].v, wd[id])) die("D overflow");
            a->de[a->dne++] = EP(id, wd[id]);
        }
    a->D = D;
    int64_t *sc = xmalloc((size_t)ndd * 8 + 8);  /* per distinct denominator: D / d, or 0 if d does not divide D */
    for (uint32_t k = 0; k < ndd; k++) {
        sc[k] = 0;
        if (!cnt[k] || dd[k].bad) continue;
        int div = 1;
        for (int j = 0; j < dd[k].ne && div; j++) div = EW(dd[k].e[j]) <= wd[EN(dd[k].e[j])];
        if (div) sc[k] = (int64_t)(D / dd[k].den);
    }
    for (size_t i = 0; i < a->n; i++) {
        int64_t s = sc[di[i]], x;
        if (s && !__builtin_mul_overflow(num[i], s, &x)) { a->num[i] = x; continue; }
        const dden *Dd = &dd[di[i]];
        if (Dd->bad) setbad(&r);
        else fin((i128)num[i], Dd->den, Dd->e, Dd->ne, 1, &r);
        a->num[i] = 0;
        xpush(a, i, &r);
    }
    st_exc_in += a->nx;
    xfree(cnt); xfree(wc); xfree(wd); xfree(sc);
}

/* ---------------------------------------------------------------- elementwise ops */

static uint32_t *ovl;
static size_t novl, ovlcap;
static inline void ovpush(size_t i) {
    if (novl == ovlcap) { ovlcap = ovlcap ? 2 * ovlcap : 1024; ovl = xrealloc(ovl, ovlcap * 4); }
    ovl[novl++] = (uint32_t)i;
}

static inline void v3store(ab_arr *R, size_t i, const raw *r) {
    if (r->den && !R->plain && R->D % r->den == 0) {
        int64_t x;
        if (!__builtin_mul_overflow(r->num, (int64_t)(R->D / r->den), &x)) { R->num[i] = x; return; }
    }
    R->num[i] = 0;
    xpush(R, i, r);
}

enum { OP_ADD, OP_MUL, OP_FMA };
/* The graph path for every exception of an operand and every slot in ovl (all: every slot). */
static void slow(ab_arr *R, int op, const ab_arr *A, const ab_arr *B, const ab_arr *C, int all) {
    size_t a = 0, b = 0, c = 0, o = 0, i = 0, n = R->n;
    for (;;) {
        if (all) {
            if (i >= n) break;
        } else {
            size_t m = SIZE_MAX;
            if (a < A->nx) m = A->xi[a];
            if (b < B->nx && B->xi[b] < m) m = B->xi[b];
            if (C && c < C->nx && C->xi[c] < m) m = C->xi[c];
            if (o < novl && ovl[o] < m) m = ovl[o];
            if (m == SIZE_MAX) break;
            i = m;
        }
        raw ta, tb, tc, t, r;
        view va = (a < A->nx && A->xi[a] == i) ? xview(&A->xv[a++]) : dview(A, i, &ta);
        view vb = (b < B->nx && B->xi[b] == i) ? xview(&B->xv[b++]) : dview(B, i, &tb);
        view vc = {0, 1, 0, NULL};
        if (C) vc = (c < C->nx && C->xi[c] == i) ? xview(&C->xv[c++]) : dview(C, i, &tc);
        if (o < novl && ovl[o] == i) o++;
        int ok;
        if (op == OP_ADD) ok = core_add(va, vb, 0, 1, &r);
        else if (op == OP_MUL) ok = core_mul(va, vb, 1, &r);
        else {
            ok = core_mul(va, vb, 1, &t);
            if (!ok) setbad(&r);
            else ok = core_add(vraw(&t), vc, 0, 1, &r);
        }
        if (!ok) ovf();
        v3store(R, i, &r);
        if (all) i++;
    }
}

static void begin_op(ab_arr *R) { R->nx = 0; R->fin = 0; novl = 0; }
static void set_D(ab_arr *R, const raw *P) {
    R->plain = 0; R->D = P->den; R->dne = P->ne;
    memcpy(R->de, P->e, sizeof(uint32_t) * P->ne);
}
static void no_plan(ab_arr *R, int plain) { R->plain = plain; R->D = 1; R->dne = 0; st_allslow++; }

static void ab_add(ab_arr *R, const ab_arr *A, const ab_arr *B) {
    begin_op(R);
    raw P;
    uint64_t ca, cb;
    view a = aview(A), b = aview(B);
    if (A->plain || B->plain || !lcm_plan(&a, &b, &ca, &cb, &P)) {
        no_plan(R, A->plain || B->plain);
        slow(R, OP_ADD, A, B, NULL, 1);
        return;
    }
    set_D(R, &P);
    int64_t xa = (int64_t)ca, xb = (int64_t)cb;
    for (size_t i = 0; i < R->n; i++) {
        int64_t x, y, z;
        int o = (int)__builtin_mul_overflow(A->num[i], xa, &x) | (int)__builtin_mul_overflow(B->num[i], xb, &y);
        o |= (int)__builtin_add_overflow(x, y, &z);
        R->num[i] = z;
        if (o) ovpush(i);
    }
    st_dense_ovf += novl;
    slow(R, OP_ADD, A, B, NULL, 0);
}

static int prod_plan(const view *a, const view *b, raw *r) {  /* the product of two denominators, with its edges */
    uint32_t n[AB_W];
    uint8_t wa[AB_W], wb[AB_W];
    uint64_t d;
    if (__builtin_mul_overflow(a->den, b->den, &d) || (d >> 63)) return 0;
    int k = merge(a, b, n, wa, wb);
    for (int i = 0; i < k; i++) r->e[i] = EP(n[i], wa[i] + wb[i]);
    r->den = d; r->ne = k;
    return 1;
}

static void ab_mul(ab_arr *R, const ab_arr *A, const ab_arr *B) {
    begin_op(R);
    raw P;
    view a = aview(A), b = aview(B);
    if (A->plain || B->plain || !prod_plan(&a, &b, &P)) {
        no_plan(R, A->plain || B->plain);
        slow(R, OP_MUL, A, B, NULL, 1);
        return;
    }
    set_D(R, &P);
    for (size_t i = 0; i < R->n; i++) {
        int64_t z;
        int o = __builtin_mul_overflow(A->num[i], B->num[i], &z);
        R->num[i] = z;
        if (o) ovpush(i);
    }
    st_dense_ovf += novl;
    slow(R, OP_MUL, A, B, NULL, 0);
}

static void ab_fma(ab_arr *R, const ab_arr *A, const ab_arr *B, const ab_arr *C) {
    begin_op(R);
    raw T, P;
    uint64_t ct, cc;
    view a = aview(A), b = aview(B), c = aview(C);
    int ok = !A->plain && !B->plain && !C->plain && prod_plan(&a, &b, &T);
    if (ok) {
        view t = vraw(&T);
        ok = lcm_plan(&t, &c, &ct, &cc, &P);
    }
    if (!ok) {
        no_plan(R, A->plain || B->plain || C->plain);
        slow(R, OP_FMA, A, B, C, 1);
        return;
    }
    set_D(R, &P);
    int64_t xt = (int64_t)ct, xc = (int64_t)cc;
    for (size_t i = 0; i < R->n; i++) {
        int64_t t, x, y, z;
        int o = __builtin_mul_overflow(A->num[i], B->num[i], &t);
        o |= (int)__builtin_mul_overflow(t, xt, &x) | (int)__builtin_mul_overflow(C->num[i], xc, &y);
        o |= (int)__builtin_add_overflow(x, y, &z);
        R->num[i] = z;
        if (o) ovpush(i);
    }
    st_dense_ovf += novl;
    slow(R, OP_FMA, A, B, C, 0);
}

static void ab_sum(ab_arr *S, const ab_arr *A) {
    begin_op(S);
    S->plain = 1; S->D = 1; S->dne = 0;
    raw128 acc = {0, 1, 0, {0}};
    int bad = 0;
    if (!A->plain) {  /* the dense part: one integer sum over D (exception slots hold 0) */
        i128 s = 0;
        for (size_t i = 0; i < A->n; i++) s += A->num[i];
        acc.num = s; acc.den = A->D; acc.ne = A->dne;
        memcpy(acc.e, A->de, sizeof(uint32_t) * A->dne);
    }
    raw buf[2];
    int cur = 0;
    buf[0].num = 0; buf[0].den = 1; buf[0].ne = 0;
    for (size_t k = 0; k < A->nx && !bad; k++) {  /* the exceptions: V0' graph adds */
        if (!core_add(vraw(&buf[cur]), xview(&A->xv[k]), 0, 1, &buf[cur ^ 1])) ovf();
        cur ^= 1;
        if (!buf[cur].den) bad = 1;
    }
    raw r;
    if (!bad && buf[cur].num && !add128(&acc, buf[cur].num, vraw(&buf[cur]))) { ovf(); bad = 1; }
    if (bad) setbad(&r);
    else if (!fin(acc.num, acc.den, acc.e, acc.ne, 1, &r)) { ovf(); setbad(&r); }
    S->num[0] = 0;
    xpush(S, 0, &r);
}

static void ab_final(ab_arr *R) {  /* lowest terms for every element: dense ones reduced against D's nodes */
    if (!R->on) { R->on = xmalloc(R->n * 8 + 8); R->od = xmalloc(R->n * 8 + 8); }
    size_t k = 0;
    raw t;
    for (size_t i = 0; i < R->n; i++) {
        if (k < R->nx && R->xi[k] == i) { R->on[i] = R->xv[k].num; R->od[i] = R->xv[k].den; k++; continue; }
        fin((i128)R->num[i], R->D, R->de, R->dne, 1, &t);
        R->on[i] = t.num; R->od[i] = t.den;
    }
    R->fin = 1;
}
static inline void ab_get(const ab_arr *a, size_t i, int64_t *num, uint64_t *den) {
    if (!a->fin) die("ab_get before ab_final");
    *num = a->on[i]; *den = a->od[i];
}

/* Audit: D matches its edges; exception indices strictly ascending and in range; their dense slots hold 0; each
 * exception value's denominator matches its edges and it is in lowest terms. */
static const char *ab_audit(ab_arr **L, int nl, const char *corrupt) {
    if (corrupt && !strcmp(corrupt, "xi")) {
        for (int k = 0; k < nl; k++)
            if (L[k]->nx >= 2) { uint32_t t = L[k]->xi[0]; L[k]->xi[0] = L[k]->xi[1]; L[k]->xi[1] = t; break; }
            else if (L[k]->nx == 1) { L[k]->xi[0] = (uint32_t)L[k]->n; break; }
    }
    for (int k = 0; k < nl; k++) {
        const ab_arr *a = L[k];
        const char *m = audit_edges(a->D, a->dne, a->de);
        if (m) return m;
        for (size_t j = 0; j < a->nx; j++) {
            if (a->xi[j] >= a->n) return "exception_index_out_of_range";
            if (j && a->xi[j] <= a->xi[j - 1]) return "exception_indices_not_ascending";
            if (a->num[a->xi[j]] != 0) return "exception_slot_not_zero";
            const xval *x = &a->xv[j];
            if (!x->den) continue;
            if ((m = audit_edges(x->den, x->ne, x->e))) return m;
            if (gcd64(mag(x->num), x->den) != 1 && x->num) return "exception_not_reduced";
        }
    }
    return NULL;
}
static void ab_stats(FILE *f) {
    fprintf(f, " exc_in=%llu dense_ovf=%llu allslow=%llu policy=%s", (unsigned long long)st_exc_in,
            (unsigned long long)st_dense_ovf, (unsigned long long)st_allslow, v3_freq ? "freq" : "lcm");
}
