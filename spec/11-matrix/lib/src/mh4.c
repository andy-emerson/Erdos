/* mh4.c: Hash-V-pack-4, the row of round m4 (test/DECISIONS-m4.md): Hash-V-pack with the changes the owner decided,
 * as one model with no build switches.  Started from m2's mh2.c (unchanged, m2's record: its earlier results stay
 * reproducible).  Prefix mhv4_ (mh4.h), so that a test can link it beside mh2.c's mhvp_.  Every cell's value, every
 * operation's case and arithmetic, and every counter of the arithmetic is mh2.c's, input for input; the arithmetic
 * bodies (add_body, the multiply body of mul_cells: v0.3 §17.3's cases in their order, every known-answer skip [K-n])
 * are mh2.c's, line for line.  What differs from mh2.c:
 *   1. the build, the compact index table (the owner's variant G): the build table (h) holds list positions, T slots
 *      of the width named by T (the handle width: the smallest of 8, 16, 32 with w - 1 >= log2 T, so that the empty
 *      mark, all ones, is never a position); (h) is filled with the empty mark by one memset; the list (l) is allocated
 *      at create for the worst case, 2RK entries of 64 bits, and a new value is appended to it; the probe compares
 *      l[h[s]] with the value.  The handle is the list position from the start, so (l) is in the order of arrival;
 *   2. the list's width: (l) is built at 64 bits and the widest value stored is tracked (its bits, the sign bit
 *      included); at the pack (l) is narrowed in place, one forward pass, to the smallest of 8, 16, 32, 64 that holds
 *      that value, and shrunk to W entries.  The same rule for every matrix, inputs and results;
 *   3. the pack: the hook (the gate reads (h) through t_whole); (h) freed; (l) narrowed and shrunk; the fields narrowed
 *      in place if the packed handle width is smaller (one forward pass), the cells shrunk.  No map, no scan of (h),
 *      no rewrite of handles;
 *   4. the bound (a bug fix): 2RK, not mh2.c's 2RK + 2, so T_fix = 2^ceil(log2 2RK) (create());
 *   5. operand reads: mmul decodes A's row i once per i into a local scratch row (ra, on the stack: n decoded cells),
 *      not once per column, released before the result's pack; every other read is by handle, as mh2.c's;
 *   6. a cell's two fields are written together, after both whole numbers are stored (store_cell);
 *   7. the gcd: the shared module's, from lib/vendor08/m4/sk_internal.h in the m4 builds (the include path), as TC's.
 * Structures: per matrix (a), the cells, and (l), the list; during a build (h), the index table; in mmul the scratch
 * row (ra), local to mmul_cells.  Nothing else (the audit build's copies aside). */
#include "mh4.h"

#include "m1int.h"

typedef MH4F(class) X;
#define MHF(f) MH4F(f)
#define MH_NAME MH4_NAME

#define EMPTY ((sk_int)-1) /* a value slot's empty mark, as t_whole reports an empty slot of (h) */

typedef struct tpair {
    sk_int m, d;
} tpair;

struct MH4F(class) {
    void *cells;      /* structure (a): 2 R K handle fields of wh bits, row-major, the numerator's field first */
    void *tab;        /* (h), during the build only (NULL once packed): T slots of whb bits, each the empty mark (all
                         ones) or a list position */
    void *list;       /* structure (l): during the build 2RK entries of 64 bits, W used; once packed W entries of w
                         bits */
    int packed;
    unsigned whb;     /* the build handle width, named by T: the width of the build fields and of (h) */
    uint32_t R, K;
    unsigned wh;      /* handle field width: whb during the build; the packed width once packed */
    unsigned w;       /* the list's width: 64 during the build; once packed, the smallest that holds wneed */
    unsigned wneed;   /* the bits of the widest whole number stored, the sign bit included (0: none yet) */
    uint32_t T;       /* (h)'s size, a power of two, T_fix, kept once packed */
    unsigned t;       /* log2 T */
    uint32_t W;       /* whole numbers held */
    uint32_t nf;      /* handle fields written so far, in the order of entry (v0.1 §5.7) */
    int id;           /* M2_A .. M2_R (m2.h) */
    sk_opctx ctx;
    sk_overflow ovf;
#ifdef SK_COUNT
    sk_counters cnt;
    r2_xcounters xcnt;
    r2_xcount *xcur;
    m2_scount sc;     /* this matrix's store counters */
    m2_ocount oc;     /* a result: its operation's own counters */
#endif
#ifdef SK_AUDIT
    int force_general;
    unsigned koff;
    char msg[200];
#endif
};

#ifdef SK_COUNT
static r2_xcount x_sink;
#define OC(x, f, n) ((x)->oc.f += (uint64_t)(n))
#else
#define OC(x, f, n) ((void)0)
#endif

/* the memory build's accounted running total (m2.h): + at a structure's allocation, - at its free */
#ifdef M2_MEM
#define ACCT(d) m2_acct((int64_t)(d))
#else
#define ACCT(d) ((void)0)
#endif

/* ----------------------------------------------------------------------------------------- fields and entries */

/* a list entry of width w (8, 16, 32 or 64 bits, two's complement), read sign-extended to 64 bits */
static inline sk_int slot_get(const void *l, unsigned w, uint32_t k) {
    switch (w) {
    case 8: return ((const int8_t *)l)[k];
    case 16: return ((const int16_t *)l)[k];
    case 32: return ((const int32_t *)l)[k];
    default: return ((const int64_t *)l)[k];
    }
}
static inline void slot_set(void *l, unsigned w, uint32_t k, sk_int v) {
    switch (w) {
    case 8: ((int8_t *)l)[k] = (int8_t)v; break;
    case 16: ((int16_t *)l)[k] = (int16_t)v; break;
    case 32: ((int32_t *)l)[k] = (int32_t)v; break;
    default: ((int64_t *)l)[k] = v; break;
    }
#ifdef SK_AUDIT
    if (slot_get(l, w, k) != v) sk_die("mh4: a whole number does not fit its list's width");
#endif
}
/* a handle field or an (h) slot of wh bits (8, 16 or 32), read by zero extension */
static inline uint32_t fld_get(const void *f, unsigned wh, uint32_t i) {
    switch (wh) {
    case 8: return ((const uint8_t *)f)[i];
    case 16: return ((const uint16_t *)f)[i];
    default: return ((const uint32_t *)f)[i];
    }
}
static inline void fld_set(void *f, unsigned wh, uint32_t i, uint32_t v) {
    switch (wh) {
    case 8: ((uint8_t *)f)[i] = (uint8_t)v; break;
    case 16: ((uint16_t *)f)[i] = (uint16_t)v; break;
    default: ((uint32_t *)f)[i] = v; break;
    }
#ifdef SK_AUDIT
    if (fld_get(f, wh, i) != v) sk_die("mh4: a handle did not fit its field");
#endif
}

/* (h)'s empty mark at width wh: all ones */
static inline uint32_t hempty(unsigned wh) { return wh == 32 ? UINT32_MAX : (1u << wh) - 1u; }

/* the bits a whole number v >= 0 needs in two's complement, and the list width that holds `need` bits */
static inline unsigned need_bits(sk_int v) { return (unsigned)sk_bits_u_raw((uint64_t)v) + 1; }
static inline unsigned width_for(unsigned need) { return need <= 8 ? 8 : need <= 16 ? 16 : need <= 32 ? 32 : 64; }

/* the whole number at list position k: at 64 bits during the build, at w once packed */
static inline sk_int value(const X *x, uint32_t k) { return x->packed ? slot_get(x->list, x->w, k) : ((const sk_int *)x->list)[k]; }

/* The hash function (v0.1 §5.3): round 3's, with t = log2 T bits.  It depends on the value and T only. */
static inline uint32_t home(sk_int v, unsigned t) { return (uint32_t)(((uint64_t)v * R3_PHI) >> (64 - t)); }

/* The probe (v0.1 §5.3), during the build: from h(v), slot by slot, wrapping, until a slot whose position holds v
 * (*found = 1) or an empty slot (*found = 0); *n is the slots read, the home slot included.  Bounded by T: after T
 * slots with neither, *found = -1 (the table is full). */
static uint32_t probe(const X *x, sk_int v, uint32_t *n, int *found) {
    const sk_int *l = x->list;
    uint32_t T = x->T, e = hempty(x->whb), s = home(v, x->t);
    for (uint32_t k = 1; k <= T; k++) {
        uint32_t q = fld_get(x->tab, x->whb, s);
        if (q == e || l[q] == v) {
            *n = k;
            *found = q != e;
            return s;
        }
        s = (s + 1) & (T - 1);
    }
    *n = T;
    *found = -1;
    return s;
}

static void full(uint32_t T) {
    fprintf(stderr, "%s: hash table full: all %u slots are used (spec v0.1 §5.3)\n", MH_NAME, T);
    sk_die("hash table full");
}

/* ----------------------------------------------------------------------------------------- the packing */

/* The packing, once the matrix's build is finished (item 3): the hook (a read: the gate reads (h) here, through
 * t_whole); (h) freed; (l) narrowed in place from 64 bits to w, one forward pass (entry k's new place never lies past
 * its old one), and shrunk from 2RK to W entries; the fields narrowed in place to the packed width wp, the smallest
 * of 8, 16, 32 that names W positions, one forward pass, and the cells shrunk, if wp is smaller than the build width.
 * No map, no scan of (h), no rewrite of a handle. */
static void pack(X *x) {
    uint32_t T = x->T, W = x->W, nf = x->nf, cap = 2 * x->R * x->K;
    unsigned wb = x->wh, wp = W <= 128u ? 8 : W <= 32768u ? 16 : 32, w = width_for(x->wneed);
    if (nf != cap || x->packed || W == 0) sk_die("mh4: a packing outside the end of a build");
#ifdef SK_AUDIT
    sk_int *before = sk_xmalloc(sizeof(sk_int) * (nf + 1)); /* audit copy: every field's value */
    for (uint32_t i = 0; i < nf; i++) before[i] = ((const sk_int *)x->list)[fld_get(x->cells, wb, i) >> 1];
#endif
    m2_pack rec = {x->id, x, W, T, w, wb, wp, wp < wb ? nf : 0, (uint64_t)W * (w / 8), 0, (uint64_t)T * (wb / 8),
                   (uint64_t)nf * (wb / 8), (uint64_t)nf * (wp / 8)};
    if (m2_pack_hook) m2_pack_hook(&rec);
    free(x->tab); /* (h) */
    ACCT(-(int64_t)((uint64_t)T * (wb / 8)));
    x->tab = NULL;
    if (w < 64) { /* narrow (l) */
        const sk_int *l64 = x->list;
        for (uint32_t k = 0; k < W; k++) slot_set(x->list, w, k, l64[k]);
    }
    x->list = sk_xrealloc(x->list, (size_t)W * (w / 8)); /* shrink (l) */
    ACCT(-(int64_t)((uint64_t)cap * 8 - (uint64_t)W * (w / 8)));
    if (wp < wb) { /* narrow the fields, and shrink the cells */
        for (uint32_t i = 0; i < nf; i++) fld_set(x->cells, wp, i, fld_get(x->cells, wb, i));
        x->cells = sk_xrealloc(x->cells, (size_t)nf * (wp / 8));
        ACCT(-(int64_t)((uint64_t)nf * ((wb - wp) / 8)));
    }
    x->packed = 1;
    x->wh = wp;
    x->w = w;
#ifdef SK_AUDIT
    for (uint32_t i = 0; i < nf; i++) {
        uint32_t f = fld_get(x->cells, wp, i);
        if ((f >> 1) >= W || slot_get(x->list, w, f >> 1) != before[i]) sk_die("mh4: a cell's value changed in the packing");
    }
    free(before);
#endif
}

/* ----------------------------------------------------------------------------------------- the store */

/* the store counters of one store: its probes, and the longest store */
#ifdef SK_COUNT
static inline void count_store(X *x, int found, uint32_t n) {
    if (found) x->sc.found++;
    else x->sc.added++;
    x->sc.probes += n;
    if (n > x->sc.longest) x->sc.longest = n;
}
#define COUNT_STORE(x, f, n) count_store((x), (f), (n))
#else
#define COUNT_STORE(x, f, n) ((void)0)
#endif

/* Store a whole number v (0 <= v <= 2^63 - 1); returns its handle, its list position.  Every value, 0 and 1 included,
 * is probed from h(v) (§15.2): found, its position; or new, appended to (l) at W, its position written into the empty
 * slot, and the widest value's bits kept. */
static uint32_t store_whole(X *x, sk_int v) {
    uint32_t n;
    int found;
    uint32_t s = probe(x, v, &n, &found);
    if (found == 1) {
        COUNT_STORE(x, 1, n);
        return fld_get(x->tab, x->whb, s);
    }
    if (found < 0) full(x->T);
    uint32_t k = x->W;
    if (k >= 2 * x->R * x->K) sk_die("mh4: more whole numbers than the build list holds");
    ((sk_int *)x->list)[k] = v;
    fld_set(x->tab, x->whb, s, k);
    x->W = k + 1;
    unsigned need = need_bits(v);
    if (need > x->wneed) x->wneed = need;
    COUNT_STORE(x, 0, n);
    return k;
}

/* Store a rational in the next cell (v0.1 §5.7): the numerator's whole number and the denominator's, then the cell's
 * two fields (handle and sign tag; handle and tag 0), written together at the cell's width (item 6). */
static void store_cell(X *x, uint32_t cell, bool neg, sk_int mag, sk_int e) {
    if (x->nf != 2 * cell) sk_die("mh4: a cell stored out of the order of entry");
    uint32_t f = 2 * store_whole(x, mag) + (neg ? 1u : 0u);
    uint32_t g = 2 * store_whole(x, e);
    uint32_t i = x->nf;
    switch (x->wh) {
    case 8: {
        uint8_t *q = (uint8_t *)x->cells + i;
        q[0] = (uint8_t)f;
        q[1] = (uint8_t)g;
        break;
    }
    case 16: {
        uint16_t *q = (uint16_t *)x->cells + i;
        q[0] = (uint16_t)f;
        q[1] = (uint16_t)g;
        break;
    }
    default: {
        uint32_t *q = (uint32_t *)x->cells + i;
        q[0] = f;
        q[1] = g;
        break;
    }
    }
#ifdef SK_AUDIT
    if (fld_get(x->cells, x->wh, i) != f || fld_get(x->cells, x->wh, i + 1) != g)
        sk_die("mh4: a handle did not fit its field");
#endif
    x->nf = i + 2;
}

/* Where a result goes: a cell of a matrix (stored at once), or a local pair. */
typedef struct dest {
    X *m;
    uint32_t cell;
    tpair *tmp;
} dest;

/* Store a result n / e (lowest terms, e >= 1), counted in c (the matrix the operation is charged to).  No zero test
 * and no e = 1 (K-14); no hints. */
static sk_status store(X *c, sk_int n, sk_int e, int kase, dest out) {
    (void)kase, (void)c;
    SETCELL(c, c->ctx.op, SK_STORE);
    KC(c, 14, ops, 1);
#ifdef SK_AUDIT
    if (!KON(c, 14) && n == 0) e = 1; /* K-14 off: the v0.1 store's normalization */
    if (n == 0 && e != 1) sk_die("K-14: a zero result without the denominator 1");
#endif
    XC(c, zero_results, n == 0);
    if (out.tmp) {
        out.tmp->m = n;
        out.tmp->d = e;
    } else {
        store_cell(out.m, out.cell, n < 0, n < 0 ? -n : n, e);
    }
    return SK_OK;
}

/* ----------------------------------------------------------------------------------------- create, entry */

/* A matrix of R x K cells: the bound 2RK, the most whole numbers its fields can name (item 4, the bug fix: mh2.c's
 * "+2" was headroom for the pre-stored 0 and 1, which no table holds since v0.2 §15.2); T_fix the smallest power of
 * two at least the bound (2RK on the sweep, so (h) may fill to 100%); the build handle width whb the smallest of 8,
 * 16, 32 with whb - 1 >= log2 T_fix (so a position, < T_fix, is never the empty mark); (h) at T_fix slots of whb
 * bits, all empty (one memset); (l) at 2RK entries of 64 bits.
 * The probe always ends: a build stores at most 2RK distinct whole numbers (two per cell) in T_fix >= 2RK slots, so a
 * new value is probed only while W < T_fix, when an empty slot exists and the wrapping probe reaches it within T_fix
 * slots; a value already held is found on its probe sequence.  full() stays as the guard of that bound. */
X *MHF(create)(uint32_t R, uint32_t K, int id) {
    if (R == 0 || K == 0 || (uint64_t)R * K > (1u << 28) || id < M2_A || id >= M2_NMAT) sk_die("mh4: bad dimensions");
    uint64_t kb = 2 * (uint64_t)R * K;
    unsigned tf = 0;
    while ((1ull << tf) < kb) tf++;
    X *x = sk_xcalloc(1, sizeof *x); /* the header */
    x->wh = tf <= 7 ? 8 : tf <= 15 ? 16 : tf <= 31 ? 32 : 0;
    if (!x->wh) sk_die("mh4: dimensions beyond 32-bit handles");
    x->whb = x->wh;
    x->R = R;
    x->K = K;
    x->id = id;
    x->t = tf;
    x->T = 1u << x->t;
    x->w = 64;
    x->cells = sk_xmalloc((size_t)2 * R * K * (x->wh / 8)); /* (a) */
    ACCT((uint64_t)2 * R * K * (x->wh / 8));
    x->tab = sk_xmalloc((size_t)x->T * (x->whb / 8)); /* (h): every slot the empty mark, all ones */
    memset(x->tab, 0xFF, (size_t)x->T * (x->whb / 8));
    ACCT((uint64_t)x->T * (x->whb / 8));
    x->list = sk_xmalloc((size_t)kb * sizeof(sk_int)); /* (l), at the worst case 2RK */
    ACCT(kb * sizeof(sk_int));
    x->W = 0;
#ifdef SK_COUNT
    x->xcur = &x_sink;
#endif
    return x;
}

/* the bytes of the cells (a), and of (h) and (l) during the build or of (l) once packed */
static uint64_t bytes_cells(const X *x) { return (uint64_t)2 * x->R * x->K * (x->wh / 8); }
static uint64_t bytes_vals(const X *x) {
    if (x->packed) return (uint64_t)x->W * (x->w / 8);
    return (uint64_t)x->T * (x->whb / 8) + (uint64_t)2 * x->R * x->K * sizeof(sk_int);
}

/* Freeing a matrix frees all its structures (v0.2 §15.5 item 6). */
void MHF(destroy)(X *x) {
    if (!x) return;
    ACCT(-(int64_t)(bytes_cells(x) + bytes_vals(x)));
    free(x->cells);
    free(x->tab);
    free(x->list);
    free(x);
}

#define OVF(step) return sk_overflow_at(&c->ovf, &c->ctx, (step))

/* gcd, as q32h.c's (without the profile's brackets: no profiling build) */
static inline sk_int gcdp(X *c, sk_int p, sk_int q) {
    (void)c;
    return sk_gcd(p, q);
}

/* Enter m / d as the next cell of an input, in the order of entry: reduced gcd-first as tc3.c's entry, then stored.
 * The input's build ends with its last cell, so it is packed inside that call (v0.2 §15.4 item 5). */
static sk_status enter_cell(X *c, sk_int m, sk_int d) {
    dest out = {c, c->nf / 2, NULL};
    c->ctx.op = SK_PH_ENTRY;
    c->ctx.x = c->ctx.y = SK_NONE;
    SETCELL(c, SK_PH_ENTRY, SK_CORE);
    if (d < 0) { /* move the sign of d to m; negation is not counted */
        m = -m;
        d = -d;
    }
    sk_int g = gcdp(c, m, d);
    if (g == 1 && KON(c, 1)) { /* [K-1] */
        KC(c, 1, ops, 1);
        KC(c, 1, div, 2);
        return store(c, m, d, R2_ENTRY, out);
    }
    sk_int n = sk_divexact(m, g);
    sk_int e = sk_divexact(d, g);
    return store(c, n, e, R2_ENTRY, out);
}

sk_status MHF(enter)(X *c, sk_int m, sk_int d) {
    if (!sk_valid_input(m) || !sk_valid_input(d)) return SK_EINVAL;
    if (d == 0) return SK_EDIVZERO;
    if (c->nf >= 2 * c->R * c->K || !M2_INPUT(c->id)) return SK_EINVAL;
    sk_status st = enter_cell(c, m, d);
    if (st == SK_OK && c->nf == 2 * c->R * c->K) pack(c);
    return st;
}

/* ----------------------------------------------------------------------------------------- operands (v0.1 §§6, 7) */

/* One operand cell, decoded: its numerator's and denominator's values, its tag, and whether both fields name one
 * handle (then V reads that value once: K-Q3).  Within one matrix a handle names one value and no value is held
 * twice, so the values are those mh2.c's cross reads put at hand. */
typedef struct opv {
    sk_int a, b;
    bool neg, same;
} opv;

/* cell `cell` of the finished (packed) matrix m: its two fields and the values they name, by handle */
static inline opv cell_at(const X *m, uint32_t cell) {
    uint32_t f = fld_get(m->cells, m->wh, 2 * cell), g = fld_get(m->cells, m->wh, 2 * cell + 1);
    return (opv){slot_get(m->list, m->w, f >> 1), slot_get(m->list, m->w, g >> 1), (f & 1) != 0, (f >> 1) == (g >> 1)};
}

/* An operation's operands, as mh2.c's: the values a, b (x's numerator and denominator) and c, d (y's), every one at
 * hand (`have`: bit k, v[k]) before a case is chosen. */
typedef struct ops2 {
    sk_int v[4];
    unsigned have;
} ops2;

/* Put the values in `need` at hand; each value read counts as a K-Q3 read.  In V every value of an operation is at
 * hand before a case is chosen, so this reads nothing (DECISIONS M1-D8): kept as mh2.c's, counting 0. */
static inline void rdset(X *c, ops2 *o, unsigned need) {
    (void)c;
    for (int k = 0; k < 4; k++)
        if ((need >> k) & 1) {
            int read = !((o->have >> k) & 1);
            o->have |= 1u << k;
            XC(c, reads, read);
            (void)read;
        }
}

/* a value not read by the operation, for a counter only (K-2's gcd work): at hand */
static inline sk_int peek(const ops2 *o, int k) { return o->v[k]; }

static inline void begin(X *c, sk_phase op) {
    c->ctx.op = op;
    SETCELL(c, op, SK_CORE);
}

/* The comparisons, all made before a case is chosen: the four cross tests b = d, a = c, a = d, c = b; in
 * multiplication the +-1 tests (a = b, c = d); the two integer tests (b = 1, d = 1) and the two zero tests (a = 0,
 * c = 0).  As tc3.c's compare(). */
typedef struct eq {
    bool den, num, ad, cb, ox, oy, ix, iy, zx, zy;
} eq;

/* Two cells, x and y: V's cross reads (each distinct handle of one matrix once: 1 + !same per cell), and every test
 * compares the values (v0.2 §15.2: no handle is tested against 0 or 1). */
static inline eq compare_cells(X *c, const ops2 *o, opv x, opv y, int six) {
    (void)c, (void)x, (void)y;
    eq q;
    OC(c, cross_reads, 4 - (int)x.same - (int)y.same);
    q.den = o->v[1] == o->v[3];
    q.num = o->v[0] == o->v[2];
    q.ad = o->v[0] == o->v[3];
    q.cb = o->v[2] == o->v[1];
    OC(c, cross, 4);
    OC(c, cross_match, (int)q.den + (int)q.num + (int)q.ad + (int)q.cb);
    q.ox = six && o->v[0] == o->v[1]; /* x = +-1: a = b (a cell is in lowest terms, so a = b = 1) */
    q.oy = six && o->v[2] == o->v[3];
    q.ix = o->v[1] == 1; /* x is an integer: b = 1 */
    q.iy = o->v[3] == 1;
    q.zx = o->v[0] == 0; /* x is zero: a = 0 */
    q.zy = o->v[2] == 0;
    XC(c, cmp, six ? 10 : 8);
    if (FORCE_GENERAL(c)) q.den = q.num = q.ad = q.cb = q.ox = q.oy = q.ix = q.iy = q.zx = q.zy = false;
    return q;
}

/* Two local pairs (multiplication's add(s, p)): every test compares values, as tc3.c's compare(); no cross test. */
static inline eq compare_values(X *c, const ops2 *o) {
    (void)c;
    eq q;
    q.den = o->v[1] == o->v[3];
    q.num = o->v[0] == o->v[2];
    q.ad = o->v[0] == o->v[3];
    q.cb = o->v[2] == o->v[1];
    q.ox = q.oy = false;
    q.ix = o->v[1] == 1;
    q.iy = o->v[3] == 1;
    q.zx = o->v[0] == 0;
    q.zy = o->v[2] == 0;
    XC(c, cmp, 8);
    if (FORCE_GENERAL(c)) q.den = q.num = q.ad = q.cb = q.ox = q.oy = q.ix = q.iy = q.zx = q.zy = false;
    return q;
}

/* ----------------------------------------------------------------------------------------- addition (v0.3 §17.3) */

/* x + y with the tags xneg, yneg, the comparisons q made, the operands o, the result to out.  mh2.c's add_body, line
 * for line. */
static sk_status add_body(X *c, eq q, bool xneg, bool yneg, ops2 *o, dest out) {
    sk_int *v = o->v;
    sk_int p, r, t, e, n, g, h;
    if (q.den && q.num && xneg != yneg && KON(c, 2)) { /* case zero: x + (-x) = 0/1  [K-2] */
        XCASE(c, R2_ADD_ZERO);
        KC(c, 2, ops, 1);
        if (q.zx) { /* 0 - 0: without K-2, case zero_x: no arithmetic (v0.3 §17.3) */
        } else if (q.ix) { /* x an integer (b = d = 1): without K-2, case int_both: t = 0, one addition (v0.3 §17.3) */
            KC(c, 2, add, 1);
        } else { /* without K-2, case den: t = 0, gcd(0, d), two divisions (counting builds read d for the work) */
            KC(c, 2, add, 1);
            KC(c, 2, gcd, 1);
            KC(c, 2, gcd_work, r2_work(0, peek(o, 3)));
            KC(c, 2, div, 2);
        }
        return store(c, 0, 1, R2_ADD_ZERO, out);
    }
    if (q.zx) { /* case zero_x: x = 0, x + y = s_y*y: no arithmetic (v0.3 §17.3) */
        XCASE(c, R2_ADD_ZERO_X);
        rdset(c, o, 0xC); /* c, d */
        return store(c, SGN(c, yneg, v[2]), v[3], R2_ADD_ZERO_X, out);
    }
    if (q.zy) { /* case zero_y: y = 0, x + y = x: no arithmetic */
        XCASE(c, R2_ADD_ZERO_Y);
        rdset(c, o, 0x3); /* a, b */
        return store(c, SGN(c, xneg, v[0]), v[1], R2_ADD_ZERO_Y, out);
    }
    if (q.ix && q.iy) { /* case int_both: b = d = 1, (s_x*a + s_y*c)/1: no gcd, no multiplication (v0.3 §17.3) */
        XCASE(c, R2_ADD_INT_BOTH);
        rdset(c, o, 0x5); /* a, c */
        if (sk_add(SGN(c, xneg, v[0]), SGN(c, yneg, v[2]), &t)) OVF("add int_both: t = s_x*a + s_y*c");
        return store(c, t, 1, R2_ADD_INT_BOTH, out);
    }
    if (q.ix) { /* case int_x: b = 1, (s_x*a*d + s_y*c)/d, in lowest terms as gcd(c, d) = 1: no gcd */
        XCASE(c, R2_ADD_INT_X);
        rdset(c, o, 0xD); /* a, c, d */
        if (sk_mul(SGN(c, xneg, v[0]), v[3], &p)) OVF("add int_x: s_x*a*d");
        if (sk_add(p, SGN(c, yneg, v[2]), &t)) OVF("add int_x: s_x*a*d + s_y*c");
        return store(c, t, v[3], R2_ADD_INT_X, out);
    }
    if (q.iy) { /* case int_y: d = 1, (s_x*a + s_y*c*b)/b, in lowest terms as gcd(a, b) = 1: no gcd */
        XCASE(c, R2_ADD_INT_Y);
        rdset(c, o, 0x7); /* a, b, c */
        if (sk_mul(SGN(c, yneg, v[2]), v[1], &r)) OVF("add int_y: s_y*c*b");
        if (sk_add(SGN(c, xneg, v[0]), r, &t)) OVF("add int_y: s_x*a + s_y*c*b");
        return store(c, t, v[1], R2_ADD_INT_Y, out);
    }
    if (q.ad || q.cb) { /* case cross: gcd(b, d) = 1 */
        XCASE(c, R2_ADD_CROSS);
        rdset(c, o, 0xF);
        if (sk_mul(SGN(c, xneg, v[0]), v[3], &p)) OVF("add cross: s_x*a*d");
        if (sk_mul(SGN(c, yneg, v[2]), v[1], &r)) OVF("add cross: s_y*c*b");
        if (sk_add(p, r, &t)) OVF("add cross: s_x*a*d + s_y*c*b");
        if (sk_mul(v[1], v[3], &e)) OVF("add cross: b*d");
        return store(c, t, e, R2_ADD_CROSS, out);
    }
    if (q.den) { /* case den: gcd(b, d) = d */
        XCASE(c, R2_ADD_DEN);
        rdset(c, o, 0xD); /* a, c, d */
        if (sk_add(SGN(c, xneg, v[0]), SGN(c, yneg, v[2]), &t)) OVF("add den: t = s_x*a + s_y*c");
        h = gcdp(c, t, v[3]);
        if (h == 1 && KON(c, 3)) { /* [K-3] */
            KC(c, 3, ops, 1);
            KC(c, 3, div, 2);
            return store(c, t, v[3], R2_ADD_DEN, out);
        }
        n = sk_divexact(t, h);
        e = sk_divexact(v[3], h);
        return store(c, n, e, R2_ADD_DEN, out);
    }
    if (q.num) { /* case num: gcd(a, b*d) = 1 */
        XCASE(c, R2_ADD_NUM);
        rdset(c, o, 0xB); /* a, b, d */
        g = gcdp(c, v[1], v[3]);
        if (g == 1 && KON(c, 4)) { /* [K-4] */
            if (sk_add(SGN(c, xneg, v[3]), SGN(c, yneg, v[1]), &t)) OVF("add num: t = s_x*d + s_y*b");
            KC(c, 4, ops, 1);
            KC(c, 4, div, 4);
            KC(c, 4, gcd, 1);
            KC(c, 4, gcd_work, r2_work(t, 1));
            if (sk_mul(v[0], t, &n)) OVF("add num: a*t");
            if (sk_mul(v[1], v[3], &e)) OVF("add num: b*d");
            return store(c, n, e, R2_ADD_NUM, out);
        }
        sk_int bp = sk_divexact(v[1], g), dp = sk_divexact(v[3], g);
        if (sk_add(SGN(c, xneg, dp), SGN(c, yneg, bp), &t)) OVF("add num: t = s_x*d' + s_y*b'");
        h = gcdp(c, t, g);
        if (h == 1 && KON(c, 5)) { /* [K-5] */
            KC(c, 5, ops, 1);
            KC(c, 5, div, 2);
            if (sk_mul(v[0], t, &n)) OVF("add num: a*t");
            if (sk_mul(bp, v[3], &e)) OVF("add num: b'*d");
            return store(c, n, e, R2_ADD_NUM, out);
        }
        sk_int tq = sk_divexact(t, h), dq = sk_divexact(v[3], h);
        if (sk_mul(v[0], tq, &n)) OVF("add num: a*(t/h)");
        if (sk_mul(bp, dq, &e)) OVF("add num: b'*(d/h)");
        return store(c, n, e, R2_ADD_NUM, out);
    }
    XCASE(c, R2_ADD_GEN); /* case general */
    rdset(c, o, 0xF);
    g = gcdp(c, v[1], v[3]);
    if (g == 1 && KON(c, 6)) { /* [K-6] the direct form is in lowest terms */
        if (sk_mul(SGN(c, xneg, v[0]), v[3], &p)) OVF("add general: s_x*a*d");
        if (sk_mul(SGN(c, yneg, v[2]), v[1], &r)) OVF("add general: s_y*c*b");
        if (sk_add(p, r, &t)) OVF("add general: s_x*a*d + s_y*c*b");
        if (sk_mul(v[1], v[3], &e)) OVF("add general: b*d");
        KC(c, 6, ops, 1);
        KC(c, 6, div, 4);
        KC(c, 6, gcd, 1);
        KC(c, 6, gcd_work, r2_work(t, 1));
        return store(c, t, e, R2_ADD_GEN, out);
    }
    sk_int bp = sk_divexact(v[1], g), dp = sk_divexact(v[3], g);
    if (sk_mul(SGN(c, xneg, v[0]), dp, &p)) OVF("add general: s_x*a*d'");
    if (sk_mul(SGN(c, yneg, v[2]), bp, &r)) OVF("add general: s_y*c*b'");
    if (sk_add(p, r, &t)) OVF("add general: t = s_x*a*d' + s_y*c*b'");
    h = gcdp(c, t, g);
    if (h == 1 && KON(c, 7)) { /* [K-7] */
        KC(c, 7, ops, 1);
        KC(c, 7, div, 2);
        if (sk_mul(bp, v[3], &e)) OVF("add general: b'*d");
        return store(c, t, e, R2_ADD_GEN, out);
    }
    n = sk_divexact(t, h);
    sk_int dq = sk_divexact(v[3], h);
    if (sk_mul(bp, dq, &e)) OVF("add general: b'*(d/h)");
    return store(c, n, e, R2_ADD_GEN, out);
}

/* addition's operands: a decoded cell x of A and y of B (§6.1), the result to a cell of C */
static sk_status add_cells(X *c, opv x, opv y, dest out) {
    begin(c, SK_PH_ADD);
    ops2 o = {{x.a, x.b, y.a, y.b}, 0xF};
    eq q = compare_cells(c, &o, x, y, 0);
    return add_body(c, q, x.neg, y.neg, &o, out);
}

/* multiplication's add(s, p): two local pairs, every value at hand (§6.2 item 3) */
static sk_status add_values(X *c, tpair s, tpair p, dest out) {
    begin(c, SK_PH_ADD);
    ops2 o = {{s.m < 0 ? -s.m : s.m, s.d, p.m < 0 ? -p.m : p.m, p.d}, 0xF};
    eq q = compare_values(c, &o);
    return add_body(c, q, s.m < 0, p.m < 0, &o, out);
}

/* ----------------------------------------------------------------------------------------- multiplication */

/* x * y, decoded cells x of A and y of B (§6.2), the result to a local pair.  mh2.c's multiply body, line for line. */
static sk_status mul_cells(X *c, opv x, opv y, dest out) {
    begin(c, SK_PH_MUL);
    ops2 o = {{x.a, x.b, y.a, y.b}, 0xF};
    eq q = compare_cells(c, &o, x, y, 1);
    sk_int *v = o.v;
    bool s = x.neg != y.neg; /* s = s_x * s_y */
    sk_int n, e, h;
    if (q.zx || q.zy) { /* case zero: a zero factor, x*y = 0/1: no arithmetic (v0.3 §17.3) */
        XCASE(c, R2_MUL_ZERO);
        return store(c, 0, 1, R2_MUL_ZERO, out);
    }
    if (q.ox) { /* case one_x: x = +-1, x*y = s_x*y  [K-15] */
        XCASE(c, R2_MUL_ONE_X);
        KC(c, 15, ops, 1);
        rdset(c, &o, 0xC); /* c, d */
        return store(c, SGN(c, s, v[2]), v[3], R2_MUL_ONE_X, out);
    }
    if (q.oy) { /* case one_y: y = +-1, x*y = s_y*x  [K-15] */
        XCASE(c, R2_MUL_ONE_Y);
        KC(c, 15, ops, 1);
        rdset(c, &o, 0x3); /* a, b */
        return store(c, SGN(c, s, v[0]), v[1], R2_MUL_ONE_Y, out);
    }
    if (q.ix && q.iy) { /* case int_both: b = d = 1, (a*c)/1: no gcd (v0.3 §17.3) */
        XCASE(c, R2_MUL_INT_BOTH);
        rdset(c, &o, 0x5); /* a, c */
        if (sk_mul(v[0], v[2], &n)) OVF("multiply int_both: a*c");
        return store(c, SGN(c, s, n), 1, R2_MUL_INT_BOTH, out);
    }
    if (q.ad && q.cb) { /* case both: a = d, c = b  [K-12] */
        XCASE(c, R2_MUL_BOTH);
        KC(c, 12, ops, 1);
        return store(c, SGN(c, s, 1), 1, R2_MUL_BOTH, out);
    }
    if (q.den) { /* case den: no gcd */
        XCASE(c, R2_MUL_DEN);
        rdset(c, &o, 0xD); /* a, c, d */
        if (sk_mul(v[0], v[2], &n)) OVF("multiply den: a*c");
        if (sk_mul(v[3], v[3], &e)) OVF("multiply den: d*d");
        return store(c, SGN(c, s, n), e, R2_MUL_DEN, out);
    }
    if (q.num) { /* case num: no gcd */
        XCASE(c, R2_MUL_NUM);
        rdset(c, &o, 0xB); /* a, b, d */
        if (sk_mul(v[0], v[0], &n)) OVF("multiply num: a*a");
        if (sk_mul(v[1], v[3], &e)) OVF("multiply num: b*d");
        return store(c, SGN(c, s, n), e, R2_MUL_NUM, out);
    }
    if (q.ad) { /* case cross_ad: a cancels */
        XCASE(c, R2_MUL_CROSS_AD);
        rdset(c, &o, 0x6); /* b, c */
        h = gcdp(c, v[2], v[1]);
        if (h == 1 && KON(c, 8)) { /* [K-8] */
            KC(c, 8, ops, 1);
            KC(c, 8, div, 2);
            return store(c, SGN(c, s, v[2]), v[1], R2_MUL_CROSS_AD, out);
        }
        n = sk_divexact(v[2], h);
        e = sk_divexact(v[1], h);
        return store(c, SGN(c, s, n), e, R2_MUL_CROSS_AD, out);
    }
    if (q.cb) { /* case cross_cb: b cancels */
        XCASE(c, R2_MUL_CROSS_CB);
        rdset(c, &o, 0x9); /* a, d */
        h = gcdp(c, v[0], v[3]);
        if (h == 1 && KON(c, 8)) { /* [K-8] */
            KC(c, 8, ops, 1);
            KC(c, 8, div, 2);
            return store(c, SGN(c, s, v[0]), v[3], R2_MUL_CROSS_CB, out);
        }
        n = sk_divexact(v[0], h);
        e = sk_divexact(v[3], h);
        return store(c, SGN(c, s, n), e, R2_MUL_CROSS_CB, out);
    }
    if (q.ix) { /* case int_x: b = 1, (a*c)/d: one cross gcd, gcd(a, d) (v0.3 §17.3) */
        XCASE(c, R2_MUL_INT_X);
        rdset(c, &o, 0xD); /* a, c, d */
        h = gcdp(c, v[0], v[3]);
        if (h == 1 && KON(c, 8)) { /* [K-8] */
            KC(c, 8, ops, 1);
            KC(c, 8, div, 2);
            if (sk_mul(v[0], v[2], &n)) OVF("multiply int_x: a*c");
            return store(c, SGN(c, s, n), v[3], R2_MUL_INT_X, out);
        }
        sk_int ap = sk_divexact(v[0], h);
        e = sk_divexact(v[3], h);
        if (sk_mul(ap, v[2], &n)) OVF("multiply int_x: a'*c");
        return store(c, SGN(c, s, n), e, R2_MUL_INT_X, out);
    }
    if (q.iy) { /* case int_y: d = 1, (a*c)/b: one cross gcd, gcd(c, b) (v0.3 §17.3) */
        XCASE(c, R2_MUL_INT_Y);
        rdset(c, &o, 0x7); /* a, b, c */
        h = gcdp(c, v[2], v[1]);
        if (h == 1 && KON(c, 8)) { /* [K-8] */
            KC(c, 8, ops, 1);
            KC(c, 8, div, 2);
            if (sk_mul(v[0], v[2], &n)) OVF("multiply int_y: a*c");
            return store(c, SGN(c, s, n), v[1], R2_MUL_INT_Y, out);
        }
        sk_int cp = sk_divexact(v[2], h);
        e = sk_divexact(v[1], h);
        if (sk_mul(v[0], cp, &n)) OVF("multiply int_y: a*c'");
        return store(c, SGN(c, s, n), e, R2_MUL_INT_Y, out);
    }
    XCASE(c, R2_MUL_GEN); /* case general, on magnitudes with the sign s */
    rdset(c, &o, 0xF);
    sk_int g1 = gcdp(c, v[0], v[3]);
    sk_int g2 = gcdp(c, v[2], v[1]);
    sk_int ap, dp, cp, bp;
    if (g1 == 1 && KON(c, 9)) { /* [K-9] */
        ap = v[0];
        dp = v[3];
        KC(c, 9, div, 2);
    } else {
        ap = sk_divexact(v[0], g1);
        dp = sk_divexact(v[3], g1);
    }
    if (g2 == 1 && KON(c, 9)) { /* [K-9] */
        cp = v[2];
        bp = v[1];
        KC(c, 9, div, 2);
    } else {
        cp = sk_divexact(v[2], g2);
        bp = sk_divexact(v[1], g2);
    }
    KC(c, 9, ops, (g1 == 1 || g2 == 1) && KON(c, 9));
    if (sk_mul(ap, cp, &n)) OVF("multiply general: a'*c'");
    if (sk_mul(bp, dp, &e)) OVF("multiply general: b'*d'");
    return store(c, SGN(c, s, n), e, R2_MUL_GEN, out);
}


/* ----------------------------------------------------------------------------------------- the matrix operations */

/* a matrix whose build is finished: every field written, and packed */
static int finished(const X *x) { return x->packed && x->nf == 2 * x->R * x->K; }

/* the operands a and b finished, of c's dimensions; c an empty result (v0.2 §15.5 item 1: any stored matrices) */
static sk_status ready(const X *a, const X *b, const X *c) {
    if (M2_INPUT(c->id) || c->nf != 0 || a == c || b == c) return SK_EINVAL;
    if (a->R != c->R || a->K != c->K || b->R != c->R || b->K != c->K) return SK_EINVAL;
    if (!finished(a) || !finished(b)) return SK_EINVAL;
    return SK_OK;
}

/* c = a + b, elementwise, in row order (v0.1 §6.1): one add per cell, its result stored in c at once; c packed at the
 * end of the call (v0.2 §15.4 item 5). */
sk_status MHF(madd)(X *a, X *b, X *c) {
    sk_status st = ready(a, b, c);
    if (st) return st;
    uint32_t n = c->R * c->K;
    M1_SNAP(s0);
    for (uint32_t i = 0; i < n; i++) {
        c->ctx.x = i; /* the operands' cells, for the overflow report */
        c->ctx.y = i;
        dest out = {c, i, NULL};
        M1_OP_BEGIN(c, SK_PH_ADD, s0);
        st = add_cells(c, cell_at(a, i), cell_at(b, i), out);
        if (st) return st;
        M1_OP_END(c, SK_PH_ADD, s0);
    }
    pack(c);
    return SK_OK;
}

/* c's every cell, before the packing (item 5): for each row i of c, A's row decoded once into the scratch row ra;
 * then for each j, s = 0/1, and for k: p = a[i][k] b[k][j], s = s + p, on local pairs; s stored as c[i][j].  ra: n
 * decoded cells on the stack, sizeof(opv) = 24 bytes each (n <= 2^14 by create's bound: at most 384 KiB); it lives
 * in this call's frame only and is released when this function returns, before mmul packs c (not inlined, so that
 * the frame is popped before pack() runs). */
static __attribute__((noinline)) sk_status mmul_cells(X *c, const X *a, const X *b, uint32_t n) {
    opv ra[n];
    M1_SNAP(s0);
    for (uint32_t i = 0; i < n; i++) {
        for (uint32_t k = 0; k < n; k++) ra[k] = cell_at(a, i * n + k);
        for (uint32_t j = 0; j < n; j++) {
            tpair s = {0, 1}, p;
            for (uint32_t k = 0; k < n; k++) {
                c->ctx.x = i * n + k;
                c->ctx.y = k * n + j;
                dest tp = {NULL, 0, &p}, ts = {NULL, 0, &s};
                M1_OP_BEGIN(c, SK_PH_MUL, s0);
                sk_status st = mul_cells(c, ra[k], cell_at(b, k * n + j), tp);
                if (st) return st;
                M1_OP_END(c, SK_PH_MUL, s0);
                c->ctx.x = c->ctx.y = SK_NONE; /* two local pairs */
                M1_OP_BEGIN(c, SK_PH_ADD, s0);
                st = add_values(c, s, p, ts);
                if (st) return st;
                M1_OP_END(c, SK_PH_ADD, s0);
            }
            store_cell(c, i * n + j, s.m < 0, s.m < 0 ? -s.m : s.m, s.d); /* store s as c[i][j] */
        }
    }
    return SK_OK;
}

/* c = a b (v0.1 §6.2), in row order of c.  Square matrices.  c packed at the end, after the scratch row is gone. */
sk_status MHF(mmul)(X *a, X *b, X *c) {
    sk_status st = ready(a, b, c);
    if (st) return st;
    uint32_t n = c->R;
    if (c->K != n) return SK_EINVAL;
    st = mmul_cells(c, a, b, n);
    if (st) return st;
    pack(c);
    return SK_OK;
}

/* ----------------------------------------------------------------------------------------- read, reports */

sk_status MHF(read)(const X *x, uint32_t cell, sk_int *m, sk_int *d) {
    if (2 * cell + 1 >= x->nf) return SK_EINVAL;
    uint32_t f = fld_get(x->cells, x->wh, 2 * cell), g = fld_get(x->cells, x->wh, 2 * cell + 1);
    sk_int v = value(x, f >> 1);
    *m = (f & 1) ? -v : v;
    *d = value(x, g >> 1);
    return SK_OK;
}

void MHF(figures)(const X *x, m2_fig *f) {
    f->R = x->R;
    f->K = x->K;
    f->T = x->T;
    f->W = x->W;
    f->nf = x->nf;
    f->w = x->w;
    f->wh = x->wh;
    f->wh_build = x->whb;
    f->packed = x->packed;
    f->bytes_cells = bytes_cells(x);
    f->bytes_vals = bytes_vals(x);
}

/* the cells, and the build's (h) and (l) or the packed list */
void MHF(memory)(const X *x, sk_memory *r) {
    m2_fig f;
    MHF(figures)(x, &f);
    memset(r, 0, sizeof *r);
    sk_memory_add(r, "cells", f.bytes_cells, f.bytes_cells);
    sk_memory_add(r, f.packed ? "list" : "table", f.bytes_vals, f.bytes_vals);
    sk_memory_finish(r, (uint64_t)x->R * x->K);
}

sk_status MHF(counters)(const X *x, sk_counters *r, r2_xcounters *e) {
#ifdef SK_COUNT
    if (r) *r = x->cnt;
    if (e) *e = x->xcnt;
    return SK_OK;
#else
    (void)x;
    if (r) memset(r, 0, sizeof *r);
    if (e) memset(e, 0, sizeof *e);
    return SK_ENOTBUILT;
#endif
}

sk_status MHF(storage)(const X *x, m2_scount *s, m2_ocount *o) {
#ifdef SK_COUNT
    if (s) *s = x->sc;
    if (o) *o = x->oc;
    return SK_OK;
#else
    (void)x;
    if (s) memset(s, 0, sizeof *s);
    if (o) memset(o, 0, sizeof *o);
    return SK_ENOTBUILT;
#endif
}

sk_status MHF(overflow)(const X *x, sk_overflow *r) {
    *r = x->ovf;
    return x->ovf.count ? SK_OK : SK_EINVAL;
}

/* ----------------------------------------------------------------------------------------- audits (v0.2 §15.7) */

#ifdef SK_AUDIT
/* during the build: list position i holds a whole number, found by the store's probe at a slot whose position is i
 * (*at: that slot), so no value is held twice */
static const char *check_pos(const X *x, uint32_t i, uint32_t *at) {
    sk_int v = ((const sk_int *)x->list)[i];
    if (v < 0) return "list: a negative value";
    if (need_bits(v) > x->wneed) return "list: a value wider than the widest recorded";
    uint32_t n;
    int found;
    *at = probe(x, v, &n, &found);
    if (found != 1) return "slot: its value is not found by its probe sequence";
    return fld_get(x->tab, x->whb, *at) == i ? NULL : "slot: its value is found at another position first (a duplicate)";
}

static int cmp_int(const void *a, const void *b) {
    sk_int x = *(const sk_int *)a, y = *(const sk_int *)b;
    return (x > y) - (x < y);
}

static sk_status audit_run(X *x, const char **why) {
    const char *fail = NULL;
    uint32_t at = 0, slot = 0;
    int packed = x->packed;
    if (packed && x->tab) fail = "pack: the build table alive after the packing";
    if (!packed && x->nf == 2 * x->R * x->K) fail = "pack: a finished matrix not packed (its build table alive)";
    if (!fail && !packed && x->w != 64) fail = "list: not 64 bits wide during the build";
    if (!fail && x->nf % 2) fail = "cells: a cell's denominator field not written";
    if (!fail && (x->T & (x->T - 1))) fail = "table: its size is not a power of two";
    if (!fail && x->T > (1u << (x->whb - 1))) fail = "table: more slots than the build handle width can name";
    if (!fail && packed && x->W > (1u << (x->wh - 1))) fail = "list: more entries than the handle field can name";
    unsigned char *used = sk_xcalloc(x->W + 1, 1); /* audit copy: the positions the cells name */
    for (at = 0; at + 1 < x->nf && !fail; at += 2) {
        uint32_t f = fld_get(x->cells, x->wh, at), g = fld_get(x->cells, x->wh, at + 1);
        uint32_t i = f >> 1, j = g >> 1;
        if (g & 1) fail = "cell: the denominator's tag is not 0";
        else if (i >= x->W || j >= x->W) fail = "cell: a handle >= W";
        else {
            sk_int a = value(x, i), b = value(x, j);
            used[i] = used[j] = 1;
            if (b < 1) fail = "cell: denominator below 1";
            else if (sk_gcd_raw(a, b) != 1) fail = "cell: not in lowest terms";
            else if (a == 0 && (b != 1 || (f & 1))) fail = "cell: zero not stored as +0/1";
            else if (!packed && !(fail = check_pos(x, i, &slot))) fail = check_pos(x, j, &slot);
        }
    }
    uint32_t held = 0;
    if (!packed) { /* (h): every used slot names a position < W used by a cell, found at that slot */
        for (uint32_t s = 0; s < x->T && !fail; s++) {
            uint32_t q = fld_get(x->tab, x->whb, s);
            if (q == hempty(x->whb)) continue;
            held++;
            if (q >= x->W) fail = "slot: a position past W";
            else if (!used[q]) fail = "whole number: used by no cell";
            else if (!(fail = check_pos(x, q, &slot)) && slot != s) fail = "slot: its position is found at another slot";
        }
    } else if (!fail) { /* the list: W entries, each used by a cell, distinct, each a whole number; w the narrowest */
        sk_int *v = sk_xmalloc(sizeof(sk_int) * (x->W + 1)); /* audit copy */
        unsigned widest = 0;
        for (uint32_t k = 0; k < x->W; k++) {
            v[k] = value(x, k);
            if (!used[k] && !fail) fail = "list: an entry used by no cell";
            if (v[k] < 0 && !fail) fail = "list: an entry that is not a whole number";
            if (v[k] >= 0 && need_bits(v[k]) > widest) widest = need_bits(v[k]);
        }
        if (!fail && (widest != x->wneed || x->w != width_for(widest))) fail = "list: not at the narrowest width";
        qsort(v, x->W, sizeof(sk_int), cmp_int);
        for (uint32_t k = 1; k < x->W && !fail; k++)
            if (v[k] == v[k - 1]) fail = "list: a whole number held twice";
        free(v);
        held = x->W;
    }
    free(used);
    if (!fail && held != x->W) fail = "table: the used slots do not number W";
    if (fail) {
        snprintf(x->msg, sizeof x->msg, "%s (matrix %d): %s (field %u)", MH_NAME, x->id, fail, at);
        if (why) *why = x->msg;
        return SK_EAUDIT;
    }
    if (why) *why = NULL;
    return SK_OK;
}
#endif

sk_status MHF(audit)(X *x, const char **why) {
#ifdef SK_AUDIT
    return audit_run(x, why);
#else
    (void)x;
    if (why) *why = "audits not built";
    return SK_ENOTBUILT;
#endif
}

/* ----------------------------------------------------------------------------------------- test build only */

/* slot s of (h) as the value it names (EMPTY for an empty slot), until the packing frees (h); position s of the list
 * once packed */
sk_status MHF(t_whole)(const X *x, uint32_t slot, sk_int *v) {
#ifdef SK_AUDIT
    if (slot >= (x->packed ? x->W : x->T)) return SK_EINVAL;
    if (x->packed) {
        *v = value(x, slot);
        return SK_OK;
    }
    uint32_t q = fld_get(x->tab, x->whb, slot);
    *v = q == hempty(x->whb) ? EMPTY : ((const sk_int *)x->list)[q];
    return SK_OK;
#else
    (void)x, (void)slot, (void)v;
    return SK_ENOTBUILT;
#endif
}

sk_status MHF(t_field)(const X *x, uint32_t f, uint32_t *v) {
#ifdef SK_AUDIT
    if (f >= x->nf) return SK_EINVAL;
    *v = fld_get(x->cells, x->wh, f);
    return SK_OK;
#else
    (void)x, (void)f, (void)v;
    return SK_ENOTBUILT;
#endif
}

sk_status MHF(t_force_general)(X *x, int on) {
#ifdef SK_AUDIT
    x->force_general = on;
    return SK_OK;
#else
    (void)x, (void)on;
    return SK_ENOTBUILT;
#endif
}

sk_status MHF(t_known_off)(X *x, unsigned mask) {
#ifdef SK_AUDIT
    x->koff = mask & R2_KOFF_ALL;
    return SK_OK;
#else
    (void)x, (void)mask;
    return SK_ENOTBUILT;
#endif
}

sk_status MHF(t_corrupt_field)(X *x, uint32_t f, uint32_t v) {
#ifdef SK_AUDIT
    if (f >= x->nf) return SK_EINVAL;
    switch (x->wh) {
    case 8: ((uint8_t *)x->cells)[f] = (uint8_t)v; break;
    case 16: ((uint16_t *)x->cells)[f] = (uint16_t)v; break;
    default: ((uint32_t *)x->cells)[f] = v; break;
    }
    return SK_OK;
#else
    (void)x, (void)f, (void)v;
    return SK_ENOTBUILT;
#endif
}

/* the value slot s of (h) names (an empty slot names none: SK_EINVAL), or position s of the packed list, written
 * without a check of its width */
sk_status MHF(t_corrupt_slot)(X *x, uint32_t slot, sk_int v) {
#ifdef SK_AUDIT
    if (slot >= (x->packed ? x->W : x->T)) return SK_EINVAL;
    if (!x->packed) {
        uint32_t q = fld_get(x->tab, x->whb, slot);
        if (q == hempty(x->whb)) return SK_EINVAL;
        ((sk_int *)x->list)[q] = v;
        return SK_OK;
    }
    switch (x->w) {
    case 8: ((int8_t *)x->list)[slot] = (int8_t)v; break;
    case 16: ((int16_t *)x->list)[slot] = (int16_t)v; break;
    case 32: ((int32_t *)x->list)[slot] = (int32_t)v; break;
    default: ((int64_t *)x->list)[slot] = v; break;
    }
    return SK_OK;
#else
    (void)x, (void)slot, (void)v;
    return SK_ENOTBUILT;
#endif
}
