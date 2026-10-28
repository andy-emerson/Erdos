/* ab_v5.c: V5 = V0' + shared denominator objects.  #included after ab_core.c.
 *
 * Each distinct denominator (edge set) exists once, as an object interned in a chained hash table keyed by its
 * value, with a reference count.  A value is (numerator, pointer to its object); NULL is bad.  "Same denominator?"
 * is a pointer comparison: add with equal pointers is one integer add (in i128) and the reduction over that
 * object's edges; every other add and every mul is V0'  merge path.  Every stored result is interned (a hash
 * lookup, or a new object), which takes a reference; overwriting a slot or dropping a temporary releases one, and
 * an object whose count reaches 0 is unlinked from the table and returned to a free list at once.  All of this is
 * inside the timed phases.  The per-rep reset (untimed, like pg_reset) drops every object at once.
 */
typedef struct dobj {
    uint64_t den;
    uint32_t rc, aud;
    struct dobj *next;
    uint8_t ne, dead;
    uint32_t e[AB_MAXE];
} dobj;
typedef struct ab_arr { size_t n; int64_t *num; dobj **d; } ab_arr;

static ab_arr *arrs[16];
static int narrs;
static dobj **tb;
static size_t tbcap, tblive, tbpeak;
static uint64_t st_made, st_freed;
static dobj *freel;

static size_t arr_n(const struct ab_arr *a) { return a->n; }
static ab_arr *ab_new(size_t n) {
    ab_arr *a = xcalloc(sizeof *a);
    a->n = n;
    a->num = xmalloc(n * 8 + 8);
    a->d = xcalloc(n * sizeof(dobj *) + 8);
    if (narrs == 16) die("too many arrays");
    arrs[narrs++] = a;
    return a;
}
static void ab_init(void) {
    tbcap = 1024;
    tb = xcalloc(tbcap * sizeof *tb);
}
static void layout_reset(void) {  /* arena-style: every object back to the free list, every slot cleared */
    for (size_t h = 0; h < tbcap; h++)
        for (dobj *o = tb[h], *nx; o; o = nx) { nx = o->next; o->dead = 1; o->rc = 0; o->next = freel; freel = o; }
    memset(tb, 0, tbcap * sizeof *tb);
    tblive = 0; tbpeak = 0; st_made = 0; st_freed = 0;
    for (int k = 0; k < narrs; k++) memset(arrs[k]->d, 0, arrs[k]->n * sizeof(dobj *));
}

static dobj *obj_alloc(void) {
    if (!freel) {
        size_t nb = 1024;
        dobj *blk = xmalloc(nb * sizeof(dobj));  /* blocks are kept for the process (reused through the free list) */
        for (size_t i = 0; i < nb; i++) { blk[i].next = freel; freel = &blk[i]; }
    }
    dobj *o = freel;
    freel = o->next;
    return o;
}
static void tb_grow(void) {
    size_t oc = tbcap;
    dobj **old = tb;
    tbcap *= 2;
    tb = xcalloc(tbcap * sizeof *tb);
    for (size_t h = 0; h < oc; h++)
        for (dobj *o = old[h], *nx; o; o = nx) {
            nx = o->next;
            size_t g = hsh(o->den) & (tbcap - 1);
            o->next = tb[g]; tb[g] = o;
        }
    xfree(old);
}
static dobj *intern(uint64_t den, int ne, const uint32_t *e) {  /* returns the object with a new reference */
    size_t h = hsh(den) & (tbcap - 1);
    for (dobj *o = tb[h]; o; o = o->next)
        if (o->den == den) { o->rc++; return o; }
    if (tblive + 1 > tbcap) { tb_grow(); h = hsh(den) & (tbcap - 1); }
    dobj *o = obj_alloc();
    o->den = den; o->rc = 1; o->aud = 0; o->ne = (uint8_t)ne; o->dead = 0;
    if (ne) memcpy(o->e, e, sizeof(uint32_t) * ne);
    o->next = tb[h]; tb[h] = o;
    tblive++; st_made++;
    if (tblive > tbpeak) tbpeak = tblive;
    return o;
}
static inline dobj *ref(dobj *o) { o->rc++; return o; }
static void obj_free(dobj *o) {
    size_t h = hsh(o->den) & (tbcap - 1);
    dobj **p = &tb[h];
    while (*p != o) {
        if (!*p) die("freeing an object not in the table");
        p = &(*p)->next;
    }
    *p = o->next;
    o->dead = 1;
    o->next = freel; freel = o;
    tblive--; st_freed++;
}
static inline void rel(dobj *o) {
    if (o && --o->rc == 0) obj_free(o);
}
static inline void put(ab_arr *R, size_t i, int64_t num, dobj *o) {  /* takes over o's reference */
    dobj *old = R->d[i];
    R->d[i] = o; R->num[i] = num;
    rel(old);
}
static inline dobj *robj(const raw *r) { return r->den ? intern(r->den, r->ne, r->e) : NULL; }
static inline view ov(int64_t num, const dobj *o) {
    return o ? (view){num, o->den, o->ne, o->e} : (view){0, 0, 0, NULL};
}

/* ---------------------------------------------------------------- entry */

static dobj **ddobj;
static void fill_begin(void) { ddobj = xcalloc((size_t)ndd * sizeof *ddobj + 8); }
static void fill(ab_arr *a, const int64_t *num, const uint32_t *di, int single) {
    (void)single;
    raw r;
    for (size_t i = 0; i < a->n; i++) {
        uint32_t k = di[i];
        const dden *D = &dd[k];
        if (D->bad) { put(a, i, 0, NULL); continue; }
        dobj *o = ddobj[k];
        if (!o) o = ddobj[k] = intern(D->den, D->ne, D->e);  /* held until fill_end */
        fin((i128)num[i], D->den, D->e, D->ne, 1, &r);        /* entry reduction, as the prototype does it */
        put(a, i, r.num, r.den == D->den ? ref(o) : intern(r.den, r.ne, r.e));
    }
}
static void fill_end(void) {
    for (uint32_t k = 0; k < ndd; k++) rel(ddobj[k]);
    xfree(ddobj);
    ddobj = NULL;
}

/* ---------------------------------------------------------------- ops */

static inline dobj *v5add(int64_t na, dobj *a, int64_t nb, dobj *b, int64_t *rn) {
    raw r;
    if (!a || !b) { *rn = 0; return NULL; }
    if (a == b) {  /* same object: one integer add, then the reduction over its edges */
        if (!fin((i128)na + nb, a->den, a->e, a->ne, 1, &r)) { ovf(); *rn = 0; return NULL; }
        *rn = r.num;
        return r.den == a->den ? ref(a) : intern(r.den, r.ne, r.e);
    }
    if (!core_add(ov(na, a), ov(nb, b), 0, 1, &r)) ovf();
    *rn = r.num;
    return robj(&r);
}
static inline dobj *v5mul(int64_t na, dobj *a, int64_t nb, dobj *b, int64_t *rn) {
    raw r;
    if (!core_mul(ov(na, a), ov(nb, b), 1, &r)) ovf();
    *rn = r.num;
    return robj(&r);
}

static void ab_add(ab_arr *R, const ab_arr *A, const ab_arr *B) {
    for (size_t i = 0; i < R->n; i++) {
        int64_t n;
        dobj *o = v5add(A->num[i], A->d[i], B->num[i], B->d[i], &n);
        put(R, i, n, o);
    }
}
static void ab_mul(ab_arr *R, const ab_arr *A, const ab_arr *B) {
    for (size_t i = 0; i < R->n; i++) {
        int64_t n;
        dobj *o = v5mul(A->num[i], A->d[i], B->num[i], B->d[i], &n);
        put(R, i, n, o);
    }
}
static void ab_fma(ab_arr *R, const ab_arr *A, const ab_arr *B, const ab_arr *C) {
    for (size_t i = 0; i < R->n; i++) {
        int64_t tn, n;
        dobj *t = v5mul(A->num[i], A->d[i], B->num[i], B->d[i], &tn);  /* the product is a value: interned */
        dobj *o = t ? v5add(tn, t, C->num[i], C->d[i], &n) : (n = 0, (dobj *)NULL);
        rel(t);
        put(R, i, n, o);
    }
}
static void ab_sum(ab_arr *S, const ab_arr *A) {
    int64_t an = 0;
    dobj *ao = intern(1, 0, NULL);
    for (size_t i = 0; i < A->n && ao; i++) {
        int64_t nn;
        dobj *no = v5add(an, ao, A->num[i], A->d[i], &nn);
        rel(ao);
        an = nn; ao = no;
    }
    put(S, 0, an, ao);
}
static void ab_final(ab_arr *R) { (void)R; }
static inline void ab_get(const ab_arr *a, size_t i, int64_t *num, uint64_t *den) {
    *num = a->num[i];
    *den = a->d[i] ? a->d[i]->den : 0;
}

/* Audit of the reference counts: recount every reference held by every array slot and compare with each object's
 * count; every object in the table must be live (count > 0), in its hash bucket, and match its edges; no slot may
 * point to a freed object; the table's size must equal the live count. */
static const char *ab_audit(ab_arr **L, int nl, const char *corrupt) {
    (void)L; (void)nl;
    if (corrupt && !strcmp(corrupt, "rc"))
        for (size_t h = 0; h < tbcap; h++) if (tb[h]) { tb[h]->rc++; break; }
    size_t cnt = 0;
    for (size_t h = 0; h < tbcap; h++)
        for (dobj *o = tb[h]; o; o = o->next) {
            o->aud = 0; cnt++;
            if ((hsh(o->den) & (tbcap - 1)) != h) return "object_in_wrong_bucket";
            if (o->dead) return "dead_object_in_table";
            if (o->rc == 0) return "unreferenced_object_not_freed";
            const char *m = audit_edges(o->den, o->ne, o->e);
            if (m) return m;
        }
    if (cnt != tblive) return "table_size_mismatch";
    for (int k = 0; k < narrs; k++)
        for (size_t i = 0; i < arrs[k]->n; i++) {
            dobj *o = arrs[k]->d[i];
            if (!o) continue;
            if (o->dead) return "slot_points_to_freed_object";
            o->aud++;
        }
    for (size_t h = 0; h < tbcap; h++)
        for (dobj *o = tb[h]; o; o = o->next)
            if (o->aud != o->rc) return "refcount_mismatch";
    return NULL;
}
static void ab_stats(FILE *f) {
    fprintf(f, " objs_peak=%zu objs_live=%zu objs_made=%llu objs_freed=%llu", tbpeak, tblive,
            (unsigned long long)st_made, (unsigned long long)st_freed);
}
