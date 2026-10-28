/* s_v3.c: V3's array layout (ab_v3.c) over the graph, with values reached through handles and deletable.
 * #included by scale.c after the graph (g_s.c or g_c.c); S and C share it.
 *
 * An entered batch is an array: one shared denominator D (its edges on a kind-1 holder, linked like any other) and
 * per element a numerator scaled to D; an element whose denominator does not divide D, or whose scaled numerator
 * overflows, is an exception: its own kind-0 holder, in lowest terms, with the dense slot 0.  D is ab_v3.c's rule:
 * the lcm of the batch's denominators if below 2^63 (policy "lcm"), else (or with policy "freq") the most frequent
 * denominator, then the lcm of the denominators dividing it.  The lcm is computed sparsely (scratch weights per node
 * id, cleared through a touched list), not with a pass over the node table as in ab_v3.c.  A batch entered "single"
 * is all exceptions (V3's single values), so every value is its own numerator on the graph.
 *
 * Handles: a value is (array, index); deleting it unlinks and frees its exception holder, frees the array (and its D
 * holder) when its last value goes, and compacts the array in place when fewer than a quarter of its slots are live.
 * The result ring holds op results as kind-2 holders: writing a slot links the new result and frees the old one.
 */

typedef struct { uint32_t n, live; uint8_t plain, alive; uint64_t D; uint32_t dh; int64_t *num; uint32_t *xh, *hid; } arr_t;
typedef struct { uint32_t arr, idx, lpos; } handle_t;
static arr_t *AR;
static uint32_t nar, arcap, arlive, *arfree, narfree, arfreecap;
static handle_t *HD;
static uint32_t nhd, hdcap, hdfree = NONE, hdlive;
static uint32_t *LV;          /* live handles, for uniform sampling */
static uint32_t nlv, lvcap;
static int64_t *ORN;          /* the values as entered, per handle: verification only (plain malloc, not counted) */
static uint64_t *ORD;
static size_t orcap;
static uint32_t *RING;        /* op results: holder + 1 */
static uint32_t ring_n;
static int v3_freq;
static uint64_t st_exc, st_dense, st_compact, st_fill_restart, st_op_restart, st_ovf_ops;

static uint32_t arr_new(uint32_t n) {
    uint32_t a;
    if (narfree) a = arfree[--narfree];
    else {
        if (nar == arcap) { arcap = arcap ? 2 * arcap : 64; AR = xrealloc(AR, (size_t)arcap * sizeof *AR); }
        a = nar++;
    }
    arr_t *A = &AR[a];
    memset(A, 0, sizeof *A);
    A->n = n; A->live = n; A->alive = 1; A->D = 1; A->dh = NONE;
    A->num = xmalloc((size_t)n * 8 + 8);
    A->hid = xmalloc((size_t)n * 4 + 4);
    arlive++;
    return a;
}
static void arr_free(uint32_t a) {
    arr_t *A = &AR[a];
    if (A->dh != NONE) { g_unlink(A->dh); h_free(A->dh); }
    xfree(A->num); xfree(A->xh); xfree(A->hid);
    memset(A, 0, sizeof *A);
    GROW(arfree, narfree, arfreecap, 1);
    arfree[narfree++] = a;
    arlive--;
}
static uint32_t handle_new(uint32_t a, uint32_t i) {
    uint32_t h;
    if (hdfree != NONE) { h = hdfree; hdfree = HD[h].arr; }
    else {
        if (nhd == hdcap) { hdcap = hdcap ? 2 * hdcap : 4096; HD = xrealloc(HD, (size_t)hdcap * sizeof *HD); }
        h = nhd++;
    }
    if (h >= orcap) {
        while (h >= orcap) orcap = orcap ? 2 * orcap : 4096;
        ORN = realloc(ORN, orcap * 8); ORD = realloc(ORD, orcap * 8);
    }
    GROW(LV, nlv, lvcap, 1);
    HD[h] = (handle_t){a, i, nlv};
    LV[nlv++] = h;
    hdlive++;
    return h;
}

/* an exception: element i of array a, num over the distinct denominator k, reduced, as its own holder */
static void x_enter(uint32_t a, uint32_t i, int64_t num, uint32_t k) {
    uint32_t h;
    const dden *D = &dd[k];
    if (D->bad) h = h_new(0, 0, NULL, 0, 0);
    else if (D->den == 1) h = h_new(num, 1, NULL, 0, 0);
    else {
        raw r;
        for (;;) {
            view v = hview(dd[k].hd);
            int ok = fin((i128)num, dd[k].den, v.e, v.ne, &r);
            if (ok < 0) { st_fill_restart++; continue; }
            if (!ok) die("entry: reduction grew the numerator");
            break;
        }
        h = h_new(r.num, r.den, r.e, r.ne, 0);
        g_link(h);
    }
    arr_t *A = &AR[a];
    if (!A->xh) A->xh = xcalloc((size_t)A->n * 4 + 4);
    A->num[i] = 0;
    A->xh[i] = h + 1;
    st_exc++;
}

static void fill(uint32_t a, const int64_t *num, const uint32_t *di, int single) {
    uint32_t n = AR[a].n;
    if (single) {
        AR[a].plain = 1;
        for (uint32_t i = 0; i < n; i++) x_enter(a, i, num[i], di[i]);
        return;
    }
    uint32_t *ta = xmalloc(((size_t)ndd * MAXE + 1) * 4), nta = 0, *tb = xmalloc(((size_t)ndd * MAXE + 1) * 4), ntb = 0;
    int fits = !v3_freq;
    if (fits) {  /* C = the lcm, if below 2^63 */
        for (uint32_t k = 0; k < ndd; k++) {
            if (!dd[k].cnt || dd[k].bad || dd[k].hd == NONE) continue;
            view v = hview(dd[k].hd);
            for (int j = 0; j < v.ne; j++) {
                uint32_t id = EN(v.e[j]), w = EW(v.e[j]);
                if (!wsa[id]) ta[nta++] = id;
                if (w > wsa[id]) wsa[id] = (uint8_t)w;
            }
        }
        uint64_t L = 1;
        for (uint32_t t = 0; t < nta && fits; t++)
            if (!mulpow(&L, nd[ta[t]].v, wsa[ta[t]]) || (L >> 63)) fits = 0;
    }
    if (!fits) {  /* else C = the most frequent denominator */
        for (uint32_t t = 0; t < nta; t++) wsa[ta[t]] = 0;
        nta = 0;
        uint32_t best = NONE;
        for (uint32_t k = 0; k < ndd; k++)
            if (dd[k].cnt && !dd[k].bad && (best == NONE || dd[k].cnt > dd[best].cnt)) best = k;
        if (best != NONE && dd[best].hd != NONE) {
            view v = hview(dd[best].hd);
            for (int j = 0; j < v.ne; j++) { wsa[EN(v.e[j])] = (uint8_t)EW(v.e[j]); ta[nta++] = EN(v.e[j]); }
        }
    }
    for (uint32_t k = 0; k < ndd; k++) {  /* D = the lcm of the denominators that divide C */
        if (!dd[k].cnt || dd[k].bad || dd[k].hd == NONE) continue;
        view v = hview(dd[k].hd);
        int div = 1;
        for (int j = 0; j < v.ne && div; j++) div = EW(v.e[j]) <= wsa[EN(v.e[j])];
        if (!div) continue;
        for (int j = 0; j < v.ne; j++) {
            uint32_t id = EN(v.e[j]), w = EW(v.e[j]);
            if (!wsb[id]) tb[ntb++] = id;
            if (w > wsb[id]) wsb[id] = (uint8_t)w;
        }
    }
    qsort(tb, ntb, 4, cmp_u32);
    uint64_t D = 1;
    uint32_t de[MAXE];
    int dne = 0;
    for (uint32_t t = 0; t < ntb; t++) {
        if (!mulpow(&D, nd[tb[t]].v, wsb[tb[t]]) || (D >> 63)) die("D overflow");
        if (dne == MAXE) die("D: too many edges");
        de[dne++] = EP(tb[t], wsb[tb[t]]);
    }
    for (uint32_t t = 0; t < nta; t++) wsa[ta[t]] = 0;
    for (uint32_t t = 0; t < ntb; t++) wsb[tb[t]] = 0;
    xfree(ta); xfree(tb);
    AR[a].D = D;
    if (D > 1) { AR[a].dh = h_new(0, D, de, dne, 1); g_link(AR[a].dh); }
    int64_t *sc = xmalloc((size_t)ndd * 8 + 8);
    for (uint32_t k = 0; k < ndd; k++) sc[k] = (!dd[k].bad && D % dd[k].den == 0) ? (int64_t)(D / dd[k].den) : 0;
    for (uint32_t i = 0; i < n; i++) {
        int64_t s = sc[di[i]], x;
        if (s && !__builtin_mul_overflow(num[i], s, &x)) { AR[a].num[i] = x; st_dense++; continue; }
        x_enter(a, i, num[i], di[i]);
    }
    xfree(sc);
}

/* Enter a batch as one array (single: every element its own holder).  Returns the first handle; the batch's handles
 * are consecutive only when the handle free list was empty, so callers use LV / the array instead. */
static uint32_t enter_batch(const int64_t *num, const uint64_t *den, uint32_t n, int single) {
    dd_begin();
    uint32_t *di = xmalloc((size_t)n * 4 + 4), li = 0;
    uint64_t last = 0;
    for (uint32_t i = 0; i < n; i++) {
        if (den[i] != last) { li = dd_get(den[i]); last = den[i]; }
        di[i] = li;
        dd[li].cnt++;
    }
    g_resolve();
    uint32_t a = arr_new(n);
    fill(a, num, di, single);
    g_entry_done();
    xfree(di);
    dd_end();
    for (uint32_t i = 0; i < n; i++) {
        uint32_t h = handle_new(a, i);
        AR[a].hid[i] = h;
        ORN[h] = num[i]; ORD[h] = den[i];
    }
    return a;
}

/* a value's view: an exception's holder, or a dense element reduced over D.  -1: a split happened, restart. */
static int get_view(uint32_t hid, raw *t, view *v) {
    const handle_t *Hh = &HD[hid];
    const arr_t *A = &AR[Hh->arr];
    uint32_t i = Hh->idx;
    if (A->xh && A->xh[i]) { *v = hview(A->xh[i] - 1); return 1; }
    if (A->D == 1) { *v = (view){A->num[i], 1, 0, NULL}; return 1; }
    view d = hview(A->dh);
    int ok = fin((i128)A->num[i], A->D, d.e, d.ne, t);
    if (ok < 0) return -1;
    if (!ok) die("dense reduction grew the numerator");
    *v = vraw(t);
    return 1;
}
static void get_value(uint32_t hid, int64_t *num, uint64_t *den) {
    raw t;
    view v;
    while (get_view(hid, &t, &v) < 0) st_op_restart++;
    *num = v.num; *den = v.den;
}

enum { OP_ADD, OP_MUL };
/* r := a op b, from the handles (restarted if a split invalidates the views).  Returns 0 on overflow (r bad). */
static int op_run(int op, uint32_t ha, uint32_t hb, raw *r) {
    for (;;) {
        raw ta, tb;
        view va, vb;
        if (get_view(ha, &ta, &va) < 0 || get_view(hb, &tb, &vb) < 0) { st_op_restart++; continue; }
        int ok = op == OP_ADD ? core_add(va, vb, 0, r) : core_mul(va, vb, r);
        if (ok < 0) { st_op_restart++; continue; }
        if (corrupt_result && r->den) { corrupt_result = 0; r->num += 1; }
        if (!ok) st_ovf_ops++;
        return ok;
    }
}
static void ring_init(uint32_t n) {
    if (ring_n != n) { xfree(RING); RING = xcalloc((size_t)n * 4 + 4); ring_n = n; }
}
static void ring_store(uint32_t slot, const raw *r) {
    uint32_t h = h_new(r->num, r->den, r->e, r->ne, 2);
    g_link(h);
    uint32_t old = RING[slot];
    RING[slot] = h + 1;
    if (old) { g_unlink(old - 1); h_free(old - 1); }
}
static void ring_clear(void) {
    for (uint32_t s = 0; s < ring_n; s++)
        if (RING[s]) { g_unlink(RING[s] - 1); h_free(RING[s] - 1); RING[s] = 0; }
}
static void ring_free(void) { ring_clear(); xfree(RING); RING = NULL; ring_n = 0; }

static void arr_compact(uint32_t a) {
    arr_t *A = &AR[a];
    uint32_t j = 0;
    for (uint32_t i = 0; i < A->n; i++) {
        if (A->hid[i] == NONE) continue;
        A->num[j] = A->num[i];
        A->hid[j] = A->hid[i];
        if (A->xh) A->xh[j] = A->xh[i];
        HD[A->hid[j]].idx = j;
        j++;
    }
    A->n = j;
    A->num = xrealloc(A->num, (size_t)j * 8 + 8);
    A->hid = xrealloc(A->hid, (size_t)j * 4 + 4);
    if (A->xh) A->xh = xrealloc(A->xh, (size_t)j * 4 + 4);
    st_compact++;
}
static void delete_value(uint32_t hid) {
    handle_t *Hh = &HD[hid];
    uint32_t a = Hh->arr, i = Hh->idx;
    arr_t *A = &AR[a];
    if (A->xh && A->xh[i]) { uint32_t h = A->xh[i] - 1; g_unlink(h); h_free(h); A->xh[i] = 0; }
    A->num[i] = 0;
    A->hid[i] = NONE;
    A->live--;
    uint32_t p = Hh->lpos, last = LV[--nlv];  /* the live list: swap-remove */
    LV[p] = last;
    HD[last].lpos = p;
    HD[hid].arr = hdfree; HD[hid].idx = NONE; hdfree = hid;
    hdlive--;
    if (AR[a].live == 0) arr_free(a);
    else if (AR[a].n >= 64 && AR[a].live * 4 < AR[a].n) arr_compact(a);
}
static void delete_all(void) { while (nlv) delete_value(LV[nlv - 1]); }

/* the layout audit: D on its holder, slots, handles, the live list */
static const char *v3_audit(void) {
    uint64_t live = 0;
    for (uint32_t a = 0; a < nar; a++) {
        const arr_t *A = &AR[a];
        if (!A->alive) continue;
        if (A->dh != NONE) {
            if (!H[A->dh].alive || H[A->dh].kind != 1) return "array_D_holder";
            if (H[A->dh].den != A->D) return "array_D_mismatch";
        } else if (A->D != 1) return "array_D_without_holder";
        uint32_t l = 0;
        for (uint32_t i = 0; i < A->n; i++) {
            uint32_t h = A->hid[i];
            if (A->xh && A->xh[i]) {
                if (!H[A->xh[i] - 1].alive || H[A->xh[i] - 1].kind != 0) return "exception_holder";
                if (A->num[i]) return "exception_slot_not_zero";
            }
            if (h == NONE) continue;
            l++;
            if (h >= nhd || HD[h].arr != a || HD[h].idx != i) return "handle_mismatch";
        }
        if (l != A->live || !l) return "array_live_count";
        live += l;
    }
    if (live != nlv || live != hdlive) return "live_value_count";
    for (uint32_t p = 0; p < nlv; p++) if (HD[LV[p]].lpos != p) return "live_list";
    return NULL;
}
