/* mh.c: the five hash rows of the matrix round (spec 11-matrix v0.1 §§3, 5-7), one source built five ways (mh.h):
 * Hash-V-fixed, Hash-M-fixed, Hash-V-grow, Hash-M-grow and Hash-V-grow-map.  The settings change only the cross
 * tests (V or M: cross()), the table's sizing (fixed or grow: create() and store_whole()) and the rewrite of a
 * growth (lookup or map: grow()); everything else is one text.
 *
 * Started from round 3's Q32-hash (../../10-interning/lib/src/q32h.c, unchanged there): its table, hash, probing,
 * 0 and 1 at slots 0 and 1, and its arithmetic bodies (the cases of v0.3 §17.3 in its order, every known-answer skip
 * [K-n], the reads of K-Q3), which equal tc3.c's call for call.  What differs from q32h.c, as the spec says:
 *   - each matrix has its own store (§0 item 3): its cells (a) and its table (b), sized from its dimensions (fixed:
 *     T_fix; grow: from 16, doubling before it passes half full); handle fields of w_h bits (8, 16 or 32), fixed at
 *     creation (§5.2); A and B 64-bit, C from 8 bits, widened by copy (§5.5);
 *   - the operands are cells of A and B, or local pairs (temporaries, in multiplication); the result goes to a cell of
 *     C (stored at once: numerator's whole number and field first, §5.7) or to a local pair (§6.2);
 *   - the four cross tests (b = d, a = c, a = d, c = b) between a whole number of A and one of B are made by V (read
 *     both values; §7.1) or by M (tau, one comparison of A's handles; §7.2); the tests within one matrix (zero,
 *     integer, +-1) compare a handle with 0 or 1, or the two handles of one operand, as q32h.c;
 *   - the store hints (K-Q1, K-Q2) do not apply (§6.1 item 3, §6.2 item 4): every result is stored by value;
 *   - an addition of two temporaries (multiplication's add(s, p)) compares values, as tc3.c, and makes no cross test.
 * Every full-width operation goes through the 08 module (sk_internal.h), unchanged.  No cache, index, side table or
 * lookup aid: the structures are (a), (b), and (t) in the M rows and (r) in the map row (§3). */
#include "mh.h"

#include "m1int.h"

#ifndef MHF
#error "compile mh.c with MH_M, MH_GROW and MH_MAP defined (mh.h)"
#endif

typedef MHF(class) X;

#define T0 16u                 /* grow: the start size (spec §5.6 item 1, M-4) */
#define EMPTY ((sk_int)-1)     /* the empty mark (v0.3 §8) */
#define H_ZERO 0u              /* 0 at slot 0, 1 at slot 1, in every table (§0 item 5, v0.3 §17.2) */
#define H_ONE 1u

typedef struct tpair {
    sk_int m, d;
} tpair;

struct MHF(class) {
    void *cells;      /* structure (a): 2 R K handle fields of wh bits, row-major, the numerator's field first */
    void *tab;        /* structure (b): T slots of w bits, each EMPTY or a whole number */
    uint32_t R, K;
    unsigned wh;      /* handle field width, from the dimensions, fixed at creation (§5.2) */
    unsigned w;       /* the table's width: 64 for A and B; C from 8 (§5.4) */
    uint32_t T;       /* the table's size, a power of two */
    unsigned t;       /* log2 T */
    uint32_t W;       /* whole numbers held, 0 and 1 included */
    uint32_t nf;      /* handle fields written so far, in the order of entry (§5.7) */
    int role;         /* M1_A, M1_B, M1_C */
#if MH_M
    void *tau;        /* structure (t), C's header only, during the operation: T_tau positions of A's wh bits */
    uint32_t tauT;
    unsigned tauwh;
#endif
    sk_opctx ctx;
    sk_overflow ovf;
#ifdef SK_COUNT
    sk_counters cnt;
    r2_xcounters xcnt;
    r2_xcount *xcur;
    m1_scount sc;     /* this matrix's store counters */
    m1_ocount oc;     /* C: the operation's own counters */
#endif
#ifdef SK_AUDIT
    int force_general;
    unsigned koff;
    char msg[200];
#endif
};

#ifdef SK_COUNT
static r2_xcount x_sink;
#define SC(x, f, n) ((x)->sc.f += (uint64_t)(n))
#define OC(x, f, n) ((x)->oc.f += (uint64_t)(n))
#else
#define SC(x, f, n) ((void)0)
#define OC(x, f, n) ((void)0)
#endif

/* ----------------------------------------------------------------------------------------- fields and slots */

/* a table slot of width w (8, 16, 32 or 64 bits, two's complement), read sign-extended to 64 bits */
static inline sk_int slot_get(const void *tab, unsigned w, uint32_t s) {
    switch (w) {
    case 8: return ((const int8_t *)tab)[s];
    case 16: return ((const int16_t *)tab)[s];
    case 32: return ((const int32_t *)tab)[s];
    default: return ((const int64_t *)tab)[s];
    }
}
static inline void slot_set(void *tab, unsigned w, uint32_t s, sk_int v) {
    switch (w) {
    case 8: ((int8_t *)tab)[s] = (int8_t)v; break;
    case 16: ((int16_t *)tab)[s] = (int16_t)v; break;
    case 32: ((int32_t *)tab)[s] = (int32_t)v; break;
    default: ((int64_t *)tab)[s] = v; break;
    }
#ifdef SK_AUDIT
    if (slot_get(tab, w, s) != v) sk_die("mh: a whole number does not fit its table's width");
#endif
}
/* a handle field (or a tau or map entry) of wh bits (8, 16 or 32), read by zero extension */
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
    if (fld_get(f, wh, i) != v) sk_die("mh: a handle did not fit its field");
#endif
}

/* The hash function (§5.3): round 3's, with t = log2 T bits: the top t bits of v * phi mod 2^64, in uint64_t.  It
 * depends on the value and T only. */
static inline uint32_t home(sk_int v, unsigned t) { return (uint32_t)(((uint64_t)v * R3_PHI) >> (64 - t)); }

/* A new table of T slots at width w: every slot EMPTY, then 0 at slot 0 and 1 at slot 1. */
static void *table_new(uint32_t T, unsigned w) {
    void *tab = sk_xmalloc((size_t)T * (w / 8));
    for (uint32_t s = 0; s < T; s++) slot_set(tab, w, s, EMPTY);
    slot_set(tab, w, H_ZERO, 0);
    slot_set(tab, w, H_ONE, 1);
    return tab;
}

/* The probe (§5.3): from h(v), slot by slot, wrapping, until v (*found = 1) or an empty slot (*found = 0); *n is the
 * slots read, the home slot included.  Bounded by T: after T slots with neither, *found = -1 (the table is full and
 * lacks v).  For a store that is an error (store_whole); for tau's find it means that A lacks v. */
static uint32_t probe(const void *tab, uint32_t T, unsigned t, unsigned w, sk_int v, uint32_t *n, int *found) {
    uint32_t s = home(v, t);
    for (uint32_t k = 1; k <= T; k++) {
        sk_int e = slot_get(tab, w, s);
        if (e == v || e == EMPTY) {
            *n = k;
            *found = e == v;
            return s;
        }
        s = (s + 1) & (T - 1);
    }
    *n = T;
    *found = -1;
    return s;
}

static void full(uint32_t T) {
    fprintf(stderr, "%s: hash table full: all %u slots are used (spec §5.3)\n", MH_NAME, T);
    sk_die("hash table full");
}

/* ----------------------------------------------------------------------------------------- widening, growth */

/* Widen C's table (§5.5): a table of T slots at the new width, every slot copied in order by sign extension, made the
 * table; the old one freed.  No slot moves, so no handle changes. */
static void widen(X *x, unsigned w2) {
    void *nt = sk_xmalloc((size_t)x->T * (w2 / 8)); /* (w), the widened table */
    for (uint32_t s = 0; s < x->T; s++) slot_set(nt, w2, s, slot_get(x->tab, x->w, s));
    free(x->tab);
    x->tab = nt;
    x->w = w2;
    SC(x, widenings, 1);
    SC(x, copied, x->T);
}

#if MH_GROW
/* One growth (§5.6 item 3), inside the store call that triggers it: (n) of 2T slots; every whole number of slots 2 ..
 * T - 1 reinserted in slot order; one pass over the handle fields written so far, in the order of entry, rewriting
 * each handle k to its new slot k' (lookup: v = old[k], found in (n); map: k' = r[k], recorded during the
 * reinsertion), the tag kept; the hook; (n) made the table, the old one (and (r)) freed. */
static void grow(X *x) {
    uint32_t T = x->T, T2 = 2 * T;
    unsigned t2 = x->t + 1;
    void *old = x->tab;
    void *nt = table_new(T2, x->w); /* (n), the grown table */
    m1_growth g = {x->role, x->nf, x->W, T, 0, 0, 0, 0, (uint64_t)T2 * (x->w / 8), 0};
#if MH_MAP
    void *r = sk_xmalloc((size_t)T * (x->wh / 8)); /* (r), the slot map: old slot -> new slot (empty slots unused) */
    fld_set(r, x->wh, H_ZERO, H_ZERO);
    fld_set(r, x->wh, H_ONE, H_ONE);
    g.bytes_map = (uint64_t)T * (x->wh / 8);
#endif
    for (uint32_t s = 2; s < T; s++) { /* reinsert */
        sk_int v = slot_get(old, x->w, s);
        if (v == EMPTY) continue;
        uint32_t n;
        int f;
        uint32_t s2 = probe(nt, T2, t2, x->w, v, &n, &f);
        if (f) sk_die("mh: a whole number stored twice, or the grown table full (growth)");
        slot_set(nt, x->w, s2, v);
        g.reinserted++;
        g.rprobes += n;
#if MH_MAP
        fld_set(r, x->wh, s, s2);
#endif
    }
#ifdef SK_AUDIT
    sk_int *before = sk_xmalloc(sizeof(sk_int) * (x->nf + 1)); /* audit copy: every written field's value */
    for (uint32_t i = 0; i < x->nf; i++) before[i] = slot_get(old, x->w, fld_get(x->cells, x->wh, i) >> 1);
#endif
    for (uint32_t i = 0; i < x->nf; i++) { /* rewrite the cells, one pass, in the order of entry */
        uint32_t f = fld_get(x->cells, x->wh, i), k = f >> 1, k2;
#if MH_MAP
        k2 = fld_get(r, x->wh, k);
#else
        sk_int v = slot_get(old, x->w, k);
        if (v == 0 || v == 1) {
            k2 = (uint32_t)v;
        } else {
            uint32_t n;
            int found;
            k2 = probe(nt, T2, t2, x->w, v, &n, &found);
            if (found != 1) sk_die("mh: a whole number not found in the grown table");
            g.wprobes += n;
        }
#endif
        fld_set(x->cells, x->wh, i, 2 * k2 + (f & 1));
        g.rewritten++;
    }
#ifdef SK_AUDIT
    for (uint32_t i = 0; i < x->nf; i++)
        if (slot_get(nt, x->w, fld_get(x->cells, x->wh, i) >> 1) != before[i])
            sk_die("mh: a cell's value changed in a growth");
    if (slot_get(nt, x->w, H_ZERO) != 0 || slot_get(nt, x->w, H_ONE) != 1) sk_die("mh: 0 or 1 moved in a growth");
    free(before);
#endif
    SC(x, growths, 1);
    SC(x, reinserted, g.reinserted);
    SC(x, rprobes, g.rprobes);
    SC(x, rewritten, g.rewritten);
    SC(x, wprobes, g.wprobes);
    if (m1_growth_hook) m1_growth_hook(&g); /* a read: the memory run's heap figure, the gate's record */
    free(old);
    x->tab = nt;
    x->T = T2;
    x->t = t2;
#if MH_MAP
    free(r);
#endif
}
#endif

/* ----------------------------------------------------------------------------------------- the store (§5.3-5.6) */

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

/* Store a whole number v (0 <= v <= 2^63 - 1) in x's table; returns its slot, its handle.  0 and 1 by the first test
 * (no probe); any other value probed from h(v): found, or new.  A new value: in the grow rows, a growth first if W + 1
 * > T/2 (then the value is probed again in the grown table); then, in C, a widening if v needs more than w bits; then
 * v is written into the empty slot. */
static uint32_t store_whole(X *x, sk_int v) {
    if (v == 0 || v == 1) {
        COUNT_STORE(x, 1, 0);
        return (uint32_t)v;
    }
    uint32_t n;
    int found;
    uint32_t s = probe(x->tab, x->T, x->t, x->w, v, &n, &found);
    if (found == 1) {
        COUNT_STORE(x, 1, n);
        return s;
    }
    if (found < 0) full(x->T);
#if MH_GROW
    if (x->W + 1 > x->T / 2) { /* the trigger (§5.6 item 2, M-5) */
        grow(x);
        uint32_t n2;
        s = probe(x->tab, x->T, x->t, x->w, v, &n2, &found);
        if (found) sk_die("mh: a new whole number found after a growth (or the grown table full)");
        n += n2;
    }
#endif
    unsigned need = (unsigned)sk_bits_u_raw((uint64_t)v) + 1; /* two's complement */
    if (need > x->w) widen(x, need <= 16 ? 16 : need <= 32 ? 32 : 64); /* C only: A and B are 64-bit */
    slot_set(x->tab, x->w, s, v);
    x->W++;
    COUNT_STORE(x, 0, n);
    return s;
}

/* Store a rational in the next cell (§5.7): the numerator's whole number, its field (handle, sign tag) written to the
 * cell; then the denominator's, its field (handle, tag 0). */
static void store_cell(X *x, uint32_t cell, bool neg, sk_int mag, sk_int e) {
    if (x->nf != 2 * cell) sk_die("mh: a cell stored out of the order of entry");
    uint32_t i = store_whole(x, mag);
    fld_set(x->cells, x->wh, x->nf, 2 * i + (neg ? 1u : 0u));
    x->nf++;
    uint32_t j = store_whole(x, e);
    fld_set(x->cells, x->wh, x->nf, 2 * j);
    x->nf++;
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

/* A matrix of R x K cells (§§2, 5): the bound K_b = 2RK + 2; T_fix the smallest power of two at least K_b; w_h the
 * smallest of 8, 16, 32 with w_h - 1 >= log2 T_fix (fixed) or log2 T_fix + 1 (grow); the table at T_fix (fixed) or 16
 * (grow) slots, at 64 bits (A, B) or 8 (C), emptied, with 0 and 1. */
X *MHF(create)(uint32_t R, uint32_t K, int role) {
    if (R == 0 || K == 0 || (uint64_t)R * K > (1u << 28) || role < M1_A || role > M1_C) sk_die("mh: bad dimensions");
    uint64_t kb = 2 * (uint64_t)R * K + 2;
    unsigned tf = 0;
    while ((1ull << tf) < kb) tf++;
    unsigned need = tf + (MH_GROW ? 1u : 0u); /* log2 of the largest table the handles must name */
    X *x = sk_xcalloc(1, sizeof *x); /* the header */
    x->wh = need <= 7 ? 8 : need <= 15 ? 16 : need <= 31 ? 32 : 0;
    if (!x->wh) sk_die("mh: dimensions beyond 32-bit handles");
    x->R = R;
    x->K = K;
    x->role = role;
    x->t = MH_GROW ? 4 : tf; /* grow: T0 = 16 */
    x->T = 1u << x->t;
    x->w = role == M1_C ? 8 : 64;
    x->cells = sk_xmalloc((size_t)2 * R * K * (x->wh / 8)); /* (a) */
    x->tab = table_new(x->T, x->w);                          /* (b) */
    x->W = 2;
#ifdef SK_COUNT
    x->xcur = &x_sink;
#endif
    return x;
}

void MHF(destroy)(X *x) {
    if (!x) return;
#if MH_M
    free(x->tau);
#endif
    free(x->cells);
    free(x->tab);
    free(x);
}

#define OVF(step) return sk_overflow_at(&c->ovf, &c->ctx, (step))

/* gcd, as q32h.c's (without the profile's brackets: no profiling build) */
static inline sk_int gcdp(X *c, sk_int p, sk_int q) {
    (void)c;
    return sk_gcd(p, q);
}

/* Enter m / d as the next cell of A or B, in the order of entry: reduced gcd-first as tc3.c's entry, then stored. */
sk_status MHF(enter)(X *c, sk_int m, sk_int d) {
    if (!sk_valid_input(m) || !sk_valid_input(d)) return SK_EINVAL;
    if (d == 0) return SK_EDIVZERO;
    if (c->nf >= 2 * c->R * c->K || c->role == M1_C) return SK_EINVAL;
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

/* ----------------------------------------------------------------------------------------- tau (§7.2) */

/* M rows: tau, built once at the start of the operation, after A and B are complete: one position per slot of B's
 * table, holding the slot of A that holds the same whole number, or none (2^wh_A - 1, never a handle): 0 and 1 map to
 * 0 and 1; B's empty slots, and a whole number of B that A lacks, hold none.  Each other whole number of B is a find
 * in A's table: its probe, without the write.  Counted in C (the operation's matrix).  V rows: nothing. */
sk_status MHF(tau_build)(X *a, X *b, X *c) {
#if MH_M
    if (c->tau || a->nf != 2 * a->R * a->K || b->nf != 2 * b->R * b->K) return SK_EINVAL;
    uint32_t none = (uint32_t)((1ull << a->wh) - 1);
    c->tauT = b->T;
    c->tauwh = a->wh;
    c->tau = sk_xmalloc((size_t)b->T * (a->wh / 8)); /* (t) */
    for (uint32_t s = 0; s < b->T; s++) {
        sk_int v = slot_get(b->tab, b->w, s);
        uint32_t k = none;
        if (v == 0 || v == 1) {
            k = (uint32_t)v;
        } else if (v != EMPTY) {
            uint32_t n;
            int found;
            uint32_t sa = probe(a->tab, a->T, a->t, a->w, v, &n, &found);
            OC(c, tau_probes, n);
            if (found == 1) {
                k = sa;
                OC(c, tau_found, 1);
            } else {
                OC(c, tau_none, 1);
            }
        }
        fld_set(c->tau, a->wh, s, k);
    }
    return SK_OK;
#else
    (void)a, (void)b, (void)c;
    return SK_OK;
#endif
}

void MHF(tau_free)(X *c) {
#if MH_M
    free(c->tau);
    c->tau = NULL;
#else
    (void)c;
#endif
}

uint64_t MHF(tau_bytes)(const X *c) {
#if MH_M
    return c->tau ? (uint64_t)c->tauT * (c->tauwh / 8) : 0;
#else
    (void)c;
    return 0;
#endif
}

/* ----------------------------------------------------------------------------------------- operands (§§6, 7) */

/* An operation's operands: the handles of a, b (x's numerator and denominator, in x's matrix) and c, d (y's, in y's
 * matrix), or none for local pairs; the values read so far (v, `have`). */
typedef struct ops2 {
    const X *mx, *my;   /* the matrices of x and y (NULL: a local pair) */
    uint32_t idx[4];    /* a, b in mx; c, d in my */
    sk_int v[4];
    unsigned have;      /* bit k: v[k] is at hand */
} ops2;

typedef struct opnd {
    uint32_t i, j; /* numerator and denominator handles */
    bool neg;      /* the tag */
} opnd;

static inline opnd operand(const X *m, uint32_t cell) {
    uint32_t f = fld_get(m->cells, m->wh, 2 * cell), g = fld_get(m->cells, m->wh, 2 * cell + 1);
    return (opnd){f >> 1, g >> 1, (f & 1) != 0};
}

/* Put the value at position k (0 a, 1 b, 2 c, 3 d) at hand: from its sibling in the same matrix (a with b, c with d)
 * when that is at hand with the same handle, or read from its matrix's table by slot.  Returns 1 if a slot was read:
 * each distinct handle of one matrix is read once (K-Q3). */
static inline int rd1(ops2 *o, int k) {
    if ((o->have >> k) & 1) return 0;
    int sib = k ^ 1, slot = 0;
    if (((o->have >> sib) & 1) && o->idx[sib] == o->idx[k]) {
        o->v[k] = o->v[sib];
    } else {
        const X *m = k < 2 ? o->mx : o->my;
        o->v[k] = slot_get(m->tab, m->w, o->idx[k]);
        slot = 1;
    }
    o->have |= 1u << k;
    return slot;
}

/* Put the values in `need` (bit k: position k) at hand; each slot read counts as a K-Q3 read. */
static inline void rdset(X *c, ops2 *o, unsigned need) {
    (void)c;
    for (int k = 0; k < 4; k++)
        if ((need >> k) & 1) {
            int read = rd1(o, k);
            XC(c, reads, read);
            (void)read;
        }
}

/* a value not read by the operation, for a counter only (K-2's gcd work): uncounted */
static inline sk_int peek(const ops2 *o, int k) {
    if ((o->have >> k) & 1) return o->v[k];
    const X *m = k < 2 ? o->mx : o->my;
    return slot_get(m->tab, m->w, o->idx[k]);
}

static inline void begin(X *c, sk_phase op) {
    c->ctx.op = op;
    SETCELL(c, op, SK_CORE);
}

/* The comparisons, all made before a case is chosen: the four cross tests b = d, a = c, a = d, c = b; in
 * multiplication the +-1 tests (a = b, c = d); the two integer tests (b = 1, d = 1) and the two zero tests (a = 0,
 * c = 0).  As q32h.c's and tc3.c's compare(). */
typedef struct eq {
    bool den, num, ad, cb, ox, oy, ix, iy, zx, zy;
} eq;

/* Two cells, x of A and y of B: within one matrix by handle; across A and B by V or M (§7). */
static inline eq compare_cells(X *c, ops2 *o, opnd x, opnd y, int six) {
    (void)c;
    eq q;
#if MH_M
    /* M: tau[k_B] against k_A; tau's none is never a handle of A */
    uint32_t tc_ = fld_get(c->tau, c->tauwh, y.i), td = fld_get(c->tau, c->tauwh, y.j);
    q.den = td == x.j;  /* b = d */
    q.num = tc_ == x.i; /* a = c */
    q.ad = td == x.i;   /* a = d */
    q.cb = tc_ == x.j;  /* c = b */
#else
    /* V: read a, b, c, d (each distinct handle of one matrix once: the cross reads), compare the values */
    for (int k = 0; k < 4; k++) {
        int read = rd1(o, k);
        OC(c, cross_reads, read);
        (void)read;
    }
    q.den = o->v[1] == o->v[3];
    q.num = o->v[0] == o->v[2];
    q.ad = o->v[0] == o->v[3];
    q.cb = o->v[2] == o->v[1];
#endif
    OC(c, cross, 4);
    OC(c, cross_match, (int)q.den + (int)q.num + (int)q.ad + (int)q.cb);
    q.ox = six && x.i == x.j;
    q.oy = six && y.i == y.j;
    q.ix = x.j == H_ONE;  /* x is an integer */
    q.iy = y.j == H_ONE;  /* y is an integer */
    q.zx = x.i == H_ZERO; /* x is zero */
    q.zy = y.i == H_ZERO; /* y is zero */
    XC(c, cmp, six ? 10 : 8);
    if (FORCE_GENERAL(c)) q.den = q.num = q.ad = q.cb = q.ox = q.oy = q.ix = q.iy = q.zx = q.zy = false;
    (void)o;
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

/* x + y with the tags xneg, yneg, the comparisons q made, the operands o (read as each case needs them), the result to
 * out.  q32h.c's addsub body, without the hints. */
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

/* addition's operands: cell xc of A and cell yc of B (§6.1), the result to cell `cell` of C */
static sk_status add_cells(X *c, const X *a, uint32_t xc, const X *b, uint32_t yc, dest out) {
    begin(c, SK_PH_ADD);
    opnd x = operand(a, xc), y = operand(b, yc);
    ops2 o = {a, b, {x.i, x.j, y.i, y.j}, {0, 0, 0, 0}, 0};
    eq q = compare_cells(c, &o, x, y, 0);
    return add_body(c, q, x.neg, y.neg, &o, out);
}

/* multiplication's add(s, p): two local pairs, every value at hand (§6.2 item 3) */
static sk_status add_values(X *c, tpair s, tpair p, dest out) {
    begin(c, SK_PH_ADD);
    ops2 o = {NULL, NULL, {0, 0, 0, 0}, {s.m < 0 ? -s.m : s.m, s.d, p.m < 0 ? -p.m : p.m, p.d}, 0xF};
    eq q = compare_values(c, &o);
    return add_body(c, q, s.m < 0, p.m < 0, &o, out);
}

/* ----------------------------------------------------------------------------------------- multiplication */

/* x * y, cell xc of A and cell yc of B (§6.2), the result to a local pair.  q32h.c's multiply body, without the hints:
 * cases one_x and one_y read the other operand, as q32h.c's one() with the hints off. */
static sk_status mul_cells(X *c, const X *a, uint32_t xc, const X *b, uint32_t yc, dest out) {
    begin(c, SK_PH_MUL);
    opnd x = operand(a, xc), y = operand(b, yc);
    ops2 o = {a, b, {x.i, x.j, y.i, y.j}, {0, 0, 0, 0}, 0};
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

static sk_status ready(const X *a, const X *b, const X *c) {
    if (a->role != M1_A || b->role != M1_B || c->role != M1_C) return SK_EINVAL;
    if (a->R != c->R || a->K != c->K || b->R != c->R || b->K != c->K) return SK_EINVAL;
    if (a->nf != 2 * a->R * a->K || b->nf != 2 * b->R * b->K || c->nf != 0) return SK_EINVAL;
#if MH_M
    if (!c->tau || c->tauT != b->T || c->tauwh != a->wh) return SK_EINVAL; /* tau, built after A and B */
#endif
    return SK_OK;
}

/* C = A + B, elementwise, in row order (§6.1): one add per cell, its result stored in C at once. */
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
        st = add_cells(c, a, i, b, i, out);
        if (st) return st;
        M1_OP_END(c, SK_PH_ADD, s0);
    }
    return SK_OK;
}

/* C = A B (§6.2): for each cell of C in row order, s = 0/1; for k: p = A[i][k] B[k][j], s = s + p, on local pairs;
 * then s stored in C (0 and 1 by the store's first test, numerator's field first).  Square matrices (§2). */
sk_status MHF(mmul)(X *a, X *b, X *c) {
    sk_status st = ready(a, b, c);
    if (st) return st;
    uint32_t n = c->R;
    if (c->K != n) return SK_EINVAL;
    M1_SNAP(s0);
    for (uint32_t i = 0; i < n; i++)
        for (uint32_t j = 0; j < n; j++) {
            tpair s = {0, 1}, p;
            for (uint32_t k = 0; k < n; k++) {
                c->ctx.x = i * n + k;
                c->ctx.y = k * n + j;
                dest tp = {NULL, 0, &p}, ts = {NULL, 0, &s};
                M1_OP_BEGIN(c, SK_PH_MUL, s0);
                st = mul_cells(c, a, i * n + k, b, k * n + j, tp);
                if (st) return st;
                M1_OP_END(c, SK_PH_MUL, s0);
                c->ctx.x = c->ctx.y = SK_NONE; /* two local pairs */
                M1_OP_BEGIN(c, SK_PH_ADD, s0);
                st = add_values(c, s, p, ts);
                if (st) return st;
                M1_OP_END(c, SK_PH_ADD, s0);
            }
            store_cell(c, i * n + j, s.m < 0, s.m < 0 ? -s.m : s.m, s.d); /* store s as C[i][j] */
        }
    return SK_OK;
}

/* ----------------------------------------------------------------------------------------- read, reports */

sk_status MHF(read)(const X *x, uint32_t cell, sk_int *m, sk_int *d) {
    if (2 * cell + 1 >= x->nf) return SK_EINVAL;
    uint32_t f = fld_get(x->cells, x->wh, 2 * cell), g = fld_get(x->cells, x->wh, 2 * cell + 1);
    sk_int v = slot_get(x->tab, x->w, f >> 1);
    *m = (f & 1) ? -v : v;
    *d = slot_get(x->tab, x->w, g >> 1);
    return SK_OK;
}

void MHF(figures)(const X *x, m1_fig *f) {
    f->R = x->R;
    f->K = x->K;
    f->T = x->T;
    f->W = x->W;
    f->nf = x->nf;
    f->w = x->w;
    f->wh = x->wh;
    f->bytes_cells = (uint64_t)2 * x->R * x->K * (x->wh / 8);
    f->bytes_table = (uint64_t)x->T * (x->w / 8);
}

/* the cells and the table, every slot counted, used or not (spec §9); tau has its own figure (tau_bytes) */
void MHF(memory)(const X *x, sk_memory *r) {
    m1_fig f;
    MHF(figures)(x, &f);
    memset(r, 0, sizeof *r);
    sk_memory_add(r, "cells", f.bytes_cells, f.bytes_cells);
    sk_memory_add(r, "table", f.bytes_table, f.bytes_table);
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

sk_status MHF(storage)(const X *x, m1_scount *s, m1_ocount *o) {
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

/* ----------------------------------------------------------------------------------------- audits (§10 item 7) */

#ifdef SK_AUDIT
/* slot s's value v is found at its own slot by the store's probe from h(v) (so no value is stored twice); slots 0
 * and 1 hold 0 and 1, and no other slot does */
static const char *check_slot(const X *x, uint32_t s) {
    sk_int v = slot_get(x->tab, x->w, s);
    if (s <= H_ONE) return v == (sk_int)s ? NULL : "slot: slots 0 and 1 do not hold 0 and 1";
    if (v == EMPTY) return "slot: referenced by a cell, but empty";
    if (v < 0) return "slot: a negative value that is not the empty mark";
    if (v == 0 || v == 1) return "slot: 0 or 1 stored again, outside slots 0 and 1";
    if ((unsigned)sk_bits_u_raw((uint64_t)v) + 1 > x->w) return "slot: a value that does not fit the width";
    uint32_t n;
    int found;
    uint32_t at = probe(x->tab, x->T, x->t, x->w, v, &n, &found);
    if (found != 1) return "slot: its value is not found by its probe sequence";
    return at == s ? NULL : "slot: its value is found at another slot first (a duplicate)";
}

static sk_status audit_run(X *x, const char **why) {
    const char *fail = NULL;
    uint32_t at = 0;
    if (x->nf % 2) fail = "cells: a cell's denominator field not written";
    if (!fail && (slot_get(x->tab, x->w, H_ZERO) != 0 || slot_get(x->tab, x->w, H_ONE) != 1))
        fail = "table: slots 0 and 1 do not hold 0 and 1";
    if (!fail && (x->T & (x->T - 1))) fail = "table: its size is not a power of two";
    if (!fail && x->T > (1u << (x->wh - 1))) fail = "table: more slots than the handle field can name";
    if (!fail && MH_GROW && 2 * (uint64_t)x->W > x->T) fail = "table: a grow table more than half full";
    unsigned char *used = sk_xcalloc(x->T, 1); /* audit copy: the slots the cells name */
    for (at = 0; at + 1 < x->nf && !fail; at += 2) {
        uint32_t f = fld_get(x->cells, x->wh, at), g = fld_get(x->cells, x->wh, at + 1);
        uint32_t i = f >> 1, j = g >> 1;
        if (g & 1) fail = "cell: the denominator's tag is not 0";
        else if (i >= x->T || j >= x->T || slot_get(x->tab, x->w, i) == EMPTY || slot_get(x->tab, x->w, j) == EMPTY)
            fail = "cell: a field names a slot that is not used";
        else {
            sk_int a = slot_get(x->tab, x->w, i), b = slot_get(x->tab, x->w, j);
            used[i] = used[j] = 1;
            if (b < 1) fail = "cell: denominator below 1";
            else if (sk_gcd_raw(a, b) != 1) fail = "cell: not in lowest terms";
            else if (a == 0 && (b != 1 || (f & 1))) fail = "cell: zero not stored as +0/1";
            else if (!(fail = check_slot(x, i))) fail = check_slot(x, j);
        }
    }
    uint32_t held = 0;
    for (uint32_t s = 0; s < x->T && !fail; s++) {
        sk_int v = slot_get(x->tab, x->w, s);
        if (v == EMPTY) continue;
        held++;
        if (!used[s] && s > H_ONE) fail = "whole number: used by no cell";
        else fail = check_slot(x, s);
    }
    free(used);
    if (!fail && held != x->W) fail = "table: the used slots do not number W";
    if (fail) {
        snprintf(x->msg, sizeof x->msg, "%s (role %d): %s (field %u)", MH_NAME, x->role, fail, at);
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

/* M rows: tau[k] holds A's slot of B's whole number k, or none exactly when A lacks it or k is empty */
sk_status MHF(audit_tau)(X *a, X *b, X *c, const char **why) {
#if defined(SK_AUDIT) && MH_M
    const char *fail = NULL;
    uint32_t none = (uint32_t)((1ull << a->wh) - 1), k = 0;
    if (!c->tau || c->tauT != b->T || c->tauwh != a->wh) fail = "tau: missing, or not sized from B's table and A's handles";
    for (k = 0; k < b->T && !fail; k++) {
        sk_int v = slot_get(b->tab, b->w, k);
        uint32_t e = fld_get(c->tau, c->tauwh, k), want = none;
        for (uint32_t s = 0; s < a->T && v != EMPTY; s++)
            if (slot_get(a->tab, a->w, s) == v) want = s; /* by scanning A's table: independent of the probe */
        if (e != want) fail = "tau: an entry is not A's slot of B's whole number (or none)";
    }
    if (fail) {
        snprintf(c->msg, sizeof c->msg, "%s: %s (position %u)", MH_NAME, fail, k ? k - 1 : 0);
        if (why) *why = c->msg;
        return SK_EAUDIT;
    }
    if (why) *why = NULL;
    return SK_OK;
#else
    (void)a, (void)b, (void)c;
    if (why) *why = "audits not built";
    return SK_ENOTBUILT;
#endif
}

/* ----------------------------------------------------------------------------------------- test build only */

sk_status MHF(t_whole)(const X *x, uint32_t slot, sk_int *v) {
#ifdef SK_AUDIT
    if (slot >= x->T) return SK_EINVAL;
    *v = slot_get(x->tab, x->w, slot);
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

sk_status MHF(t_tau)(const X *c, uint32_t k, uint32_t *v) {
#if defined(SK_AUDIT) && MH_M
    if (!c->tau || k >= c->tauT) return SK_EINVAL;
    *v = fld_get(c->tau, c->tauwh, k);
    return SK_OK;
#else
    (void)c, (void)k, (void)v;
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

sk_status MHF(t_corrupt_slot)(X *x, uint32_t slot, sk_int v) {
#ifdef SK_AUDIT
    if (slot >= x->T) return SK_EINVAL;
    switch (x->w) {
    case 8: ((int8_t *)x->tab)[slot] = (int8_t)v; break;
    case 16: ((int16_t *)x->tab)[slot] = (int16_t)v; break;
    case 32: ((int32_t *)x->tab)[slot] = (int32_t)v; break;
    default: ((int64_t *)x->tab)[slot] = v; break;
    }
    return SK_OK;
#else
    (void)x, (void)slot, (void)v;
    return SK_ENOTBUILT;
#endif
}
