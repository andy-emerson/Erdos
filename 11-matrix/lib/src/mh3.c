/* mh3.c: the two factored rows of round m3 (spec 11-matrix v0.3 §16), one source built two ways (mh3.h): Pack-primes
 * (MH3_COPRIME 0) and Pack-coprime (MH3_COPRIME 1).
 *
 * Started from m2's mh2.c (unchanged, m2's record), Hash-V-pack's path (MH_GROW 0, MH_PACK 1), copied line for line:
 * the fields and slots, the hash, the build table (h) of T_fix slots with its probing and widening, the store, the
 * packing into (l) with the pack map (p) and the in-place pass, the entry, the operands and comparisons, and the
 * arithmetic bodies (v0.3 §17.3's cases in their order, every known-answer skip [K-n]).  So (h) and (l) are
 * Hash-V-pack's, slot for slot and entry for entry, and the store counters are Hash-V-pack's (v0.3 §16.4 item 1).
 * What is added, as v0.3 §16 says:
 *   - after the pack, each whole number of (l) is factored once, in list order, into the factorization list (f):
 *     offsets (W + 1, 32-bit), factors (E, at the list's width w) and exponents (E, 8-bit), grown by doubling from 2W
 *     entries, then shrunk with realloc (§16.4 items 1-2).  Pack-primes: schoolbook trial division over 2 and the odd
 *     numbers, stopping at d^2 > m (§16.4 item 3).  Pack-coprime: pass 1 builds the matrix's coprime base (q) by 09's
 *     absorb scan, gcds and divisions only; pass 2 writes each number's factors over the final base (§16.4 item 4);
 *   - the value arithmetic is TC's, step for step: only how each gcd value is obtained changes (§16.5).  A gcd of two
 *     stored or temporary whole numbers is a merge of their sorted factorizations; a new numerator is reduced only at
 *     the factors of equal valuation (Pack-primes: divisibility tests; Pack-coprime: one Euclid gcd per piece,
 *     gcd(t, q^nu)), never factored (§16.6);
 *   - multiplication's temporaries (the product p and the sum s) carry their denominator's list (t), formed by each case
 *     from its operands' lists (§16.5 last columns; §16.6 item 4);
 *   - Pack-coprime: at the start of each step the two operands are reconciled in place against denominator factors
 *     only (§16.7), with the denominator-factor lists (r); a split rewrites the base (q) and rebuilds (f) into (f')
 *     (§16.8); a leftover (gcd(t, q^nu) not a power of q) of a temporary's sum splits q in the step's matrices and in
 *     the live temporary lists (§16.8);
 *   - every factoring, merge, test, per-piece gcd, split and rebuild is counted (counting builds; §16.10), and in the
 *     test build every obtained gcd is checked against the 08 module's Euclid (sk_gcd_raw) (§16.9 item 3);
 *   - the memory build's accounted running total gains (f), (f'), (q) and (r) (§16.10).
 * Every full-width operation of the value arithmetic goes through the 08 module (sk_internal.h), unchanged.  The
 * factor work (trial division, pass 1 and 2, tests, the merges' values, the splits) uses the raw forms and is counted
 * apart, so the core's counts stay TC's.  No cache, index, side table or lookup aid: the structures are (a), (l), (f)
 * and (q), the temporaries (w), (h), (p), (f') and (r), and the scratch of §16.3: (t), the merge's gcd list and S. */
#include "mh3.h"

#include "m1int.h"

#ifndef MH3F
#error "compile mh3.c with MH3_COPRIME defined (mh3.h)"
#endif

typedef MH3F(class) X;

#define EMPTY ((sk_int)-1) /* the empty mark (v0.3 §8) */
#define FMAX 15u           /* a list's most entries: a 64-bit number has at most 15 pairwise coprime factors (§16.3) */
#define SMAX 32u           /* refine's working set S: at most 32 entries (§16.3) */

typedef struct tpair {
    sk_int m, d;
} tpair;

struct MH3F(class) {
    void *cells;      /* structure (a): 2 R K handle fields of wh bits, row-major, the numerator's field first */
    void *tab;        /* (h), the build table, during the build only (NULL once packed) */
    void *list;       /* structure (l), once packed: W entries of w bits */
    int packed;
    unsigned whb;     /* the build handle width (the cells' width before the packing) */
    uint32_t R, K;
    unsigned wh;      /* handle field width: from the dimensions at creation; shrunk by the packing */
    unsigned w;       /* the width of the table (and list, factors, pieces): 64 for an input; a result from 8 */
    uint32_t T;       /* (h)'s size, T_fix, kept once packed */
    unsigned t;       /* log2 T */
    uint32_t W;       /* whole numbers held */
    uint32_t nf;      /* handle fields written so far, in the order of entry (v0.1 §5.7) */
    int id;           /* M2_A .. M2_R (m2.h) */
    /* structure (f), the factorization list (v0.3 §16.3), once factored: entry j of number k at foff[k] + j */
    uint32_t *foff;   /* W + 1 offsets */
    void *ffac;       /* factors, at width w */
    uint8_t *fexp;    /* exponents */
    uint32_t E, Ecap; /* entries written, entries allocated */
    int factored;
#if MH3_COPRIME
    void *piece;      /* structure (q), the coprime base: nq pieces at width w, in the order they were made */
    uint32_t nq, qcap;
#endif
    sk_opctx ctx;
    sk_overflow ovf;
#ifdef SK_COUNT
    sk_counters cnt;
    r2_xcounters xcnt;
    r2_xcount *xcur;
    m2_scount sc;     /* this matrix's store counters */
    m2_ocount oc;     /* a result: its operation's own counters */
    m3_fcount fc;     /* this matrix's factoring */
    m3_acount ac;     /* a result: its step's factored arithmetic */
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
#define FC(x, f, n) ((x)->fc.f += (uint64_t)(n))
#define AC(x, f, n) ((x)->ac.f += (uint64_t)(n))
#else
#define SC(x, f, n) ((void)0)
#define OC(x, f, n) ((void)0)
#define FC(x, f, n) ((void)0)
#define AC(x, f, n) ((void)0)
#endif

/* the memory build's accounted running total (m2.h): + at a structure's allocation, - at its free */
#ifdef M2_MEM
#define ACCT(d) m2_acct((int64_t)(d))
#else
#define ACCT(d) ((void)0)
#endif

/* ----------------------------------------------------------------------------------------- fields and slots */

/* a table slot or list entry of width w (8, 16, 32 or 64 bits, two's complement), read sign-extended to 64 bits */
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
    if (slot_get(tab, w, s) != v) sk_die("mh3: a whole number does not fit its table's width");
#endif
}
/* a handle field (or a map entry) of wh bits (8, 16 or 32), read by zero extension */
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
    if (fld_get(f, wh, i) != v) sk_die("mh3: a handle did not fit its field");
#endif
}

/* the values a handle names: (h) during the build, the list once packed */
static inline const void *vals(const X *x) {
    if (x->packed) return x->list;
    return x->tab;
}

/* The hash function (v0.1 §5.3): round 3's, with t = log2 T bits.  It depends on the value and T only. */
static inline uint32_t home(sk_int v, unsigned t) { return (uint32_t)(((uint64_t)v * R3_PHI) >> (64 - t)); }

/* A new table of T slots at width w: every slot EMPTY (v0.2 §15.2: no slot is reserved). */
static void *table_new(uint32_t T, unsigned w) {
    void *tab = sk_xmalloc((size_t)T * (w / 8));
    for (uint32_t s = 0; s < T; s++) slot_set(tab, w, s, EMPTY);
    ACCT((uint64_t)T * (w / 8));
    return tab;
}

/* The probe (v0.1 §5.3): from h(v), slot by slot, wrapping, until v (*found = 1) or an empty slot (*found = 0); *n is
 * the slots read, the home slot included.  Bounded by T: after T slots with neither, *found = -1 (the table is full). */
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
    fprintf(stderr, "%s: hash table full: all %u slots are used (spec v0.1 §5.3)\n", MH3_NAME, T);
    sk_die("hash table full");
}

/* ----------------------------------------------------------------------------------------- widening */

/* Widen a result's table (v0.1 §5.5): a table of T slots at the new width, every slot copied in order by sign
 * extension, made the table; the old one freed.  No slot moves, so no handle changes. */
static void widen(X *x, unsigned w2) {
    void *nt = sk_xmalloc((size_t)x->T * (w2 / 8)); /* (w), the widened table */
    ACCT((uint64_t)x->T * (w2 / 8));
    for (uint32_t s = 0; s < x->T; s++) slot_set(nt, w2, s, slot_get(x->tab, x->w, s));
    free(x->tab);
    ACCT(-(int64_t)((uint64_t)x->T * (x->w / 8)));
    x->tab = nt;
    x->w = w2;
    SC(x, widenings, 1);
    SC(x, copied, x->T);
}

/* ----------------------------------------------------------------------------------------- the packing */

/* The packing (spec v0.2 §15.4 item 3), once the matrix's build is finished, as mh2.c's Hash-V-pack:
 *   1. (l), W entries of w bits, and (p), T_fix entries of the build width;
 *   2. (h)'s used slots copied in slot order: slot s's value to the next position k of (l), and p[s] = k;
 *   3-4. one forward pass over the cells, in place: each field read at the build width, its handle rewritten through
 *      (p) with the tag kept, written at the packed width, the smallest of 8, 16, 32 with 2^(wp - 1) >= W; if the width
 *      shrank, the cells shrunk with realloc;
 *   5. the hook; (h) and (p) freed. */
static void pack(X *x) {
    uint32_t T = x->T, W = x->W, nf = x->nf;
    unsigned w = x->w, wb = x->wh;
    unsigned wp = W <= 128u ? 8 : W <= 32768u ? 16 : 32;
    if (nf != 2 * x->R * x->K || x->packed) sk_die("mh3: a packing outside the end of a build");
    void *list = sk_xmalloc((size_t)W * (w / 8)); /* (l) */
    ACCT((uint64_t)W * (w / 8));
    void *p = sk_xmalloc((size_t)T * (wb / 8)); /* (p), the pack map: old slot -> new position */
    ACCT((uint64_t)T * (wb / 8));
    uint32_t k = 0;
    for (uint32_t s = 0; s < T; s++) { /* copy in slot order */
        sk_int v = slot_get(x->tab, w, s);
        if (v == EMPTY) continue;
        if (k >= W) sk_die("mh3: more used slots than W (packing)");
        slot_set(list, w, k, v);
        fld_set(p, wb, s, k);
        k++;
    }
    if (k != W) sk_die("mh3: fewer used slots than W (packing)");
#ifdef SK_AUDIT
    sk_int *before = sk_xmalloc(sizeof(sk_int) * (nf + 1)); /* audit copy: every field's value */
    for (uint32_t i = 0; i < nf; i++) before[i] = slot_get(x->tab, w, fld_get(x->cells, wb, i) >> 1);
#endif
    for (uint32_t i = 0; i < nf; i++) { /* rewrite through (p) and narrow, one forward pass, in place */
        uint32_t f = fld_get(x->cells, wb, i);
        fld_set(x->cells, wp, i, 2 * fld_get(p, wb, f >> 1) + (f & 1));
    }
    if (wp < wb) { /* shrink the cells */
        x->cells = sk_xrealloc(x->cells, (size_t)nf * (wp / 8));
        ACCT(-(int64_t)((uint64_t)nf * ((wb - wp) / 8)));
    }
    m2_pack rec = {x->id, x, W, T, w, wb, wp, nf, (uint64_t)W * (w / 8), (uint64_t)T * (wb / 8),
                   (uint64_t)T * (w / 8), (uint64_t)nf * (wb / 8), (uint64_t)nf * (wp / 8)};
    if (m2_pack_hook) m2_pack_hook(&rec); /* a read: the gate reads (h) here, through t_whole */
    free(x->tab); /* (h) */
    ACCT(-(int64_t)((uint64_t)T * (w / 8)));
    free(p);
    ACCT(-(int64_t)((uint64_t)T * (wb / 8)));
    x->tab = NULL;
    x->list = list;
    x->packed = 1;
    x->whb = wb;
    x->wh = wp;
#ifdef SK_AUDIT
    for (uint32_t i = 0; i < nf; i++) {
        uint32_t f = fld_get(x->cells, wp, i);
        if ((f >> 1) >= W || slot_get(list, w, f >> 1) != before[i]) sk_die("mh3: a cell's value changed in the packing");
    }
    free(before);
#endif
}

/* ----------------------------------------------------------------------------------------- (f): the factorization list */

static inline uint64_t fbytes(uint32_t W, uint32_t cap, unsigned w) { return 4ull * (W + 1) + (uint64_t)cap * (w / 8 + 1); }

/* (f) being written (by the factoring, or by a rebuild into (f')): its three arrays, grown together by doubling */
typedef struct fbuf {
    uint32_t *off;
    void *fac;
    uint8_t *exp;
    uint32_t E, cap, W;
    unsigned w;
} fbuf;

static void fbuf_new(fbuf *b, uint32_t W, unsigned w, uint32_t cap) {
    if (cap < 1) cap = 1;
    b->W = W;
    b->w = w;
    b->E = 0;
    b->cap = cap;
    b->off = sk_xmalloc(sizeof(uint32_t) * ((size_t)W + 1));
    b->fac = sk_xmalloc((size_t)cap * (w / 8));
    b->exp = sk_xmalloc(cap);
    ACCT(fbytes(W, cap, w));
}

/* append (q, e) to the number being written, moving it down past the larger entries already appended for it (the
 * insertion sort in place, §16.4 item 4) */
static void fbuf_put(fbuf *b, uint32_t k, sk_int q, unsigned e) {
    if (e < 1 || e > 255) sk_die("mh3: an exponent out of range");
    if (b->E == b->cap) { /* double */
        uint32_t c2 = b->cap > UINT32_MAX / 2 ? UINT32_MAX : 2 * b->cap;
        b->fac = sk_xrealloc(b->fac, (size_t)c2 * (b->w / 8));
        b->exp = sk_xrealloc(b->exp, c2);
        ACCT((int64_t)((uint64_t)(c2 - b->cap) * (b->w / 8 + 1)));
        b->cap = c2;
    }
    uint32_t j = b->E++;
    while (j > b->off[k] && slot_get(b->fac, b->w, j - 1) > q) {
        slot_set(b->fac, b->w, j, slot_get(b->fac, b->w, j - 1));
        b->exp[j] = b->exp[j - 1];
        j--;
    }
    slot_set(b->fac, b->w, j, q);
    b->exp[j] = (uint8_t)e;
    if (b->E - b->off[k] > FMAX) sk_die("mh3: more than 15 factors of one whole number");
}

/* shrink to E entries and make it the matrix's (f) */
static void fbuf_finish(X *x, fbuf *b) {
    b->off[b->W] = b->E;
    if (b->E == 0) { /* no entry (every number 0 or 1): the two arrays of no entries are not kept */
        free(b->fac);
        free(b->exp);
        b->fac = NULL;
        b->exp = NULL;
    } else {
        b->fac = sk_xrealloc(b->fac, (size_t)b->E * (b->w / 8));
        b->exp = sk_xrealloc(b->exp, b->E);
    }
    ACCT(-(int64_t)((uint64_t)(b->cap - b->E) * (b->w / 8 + 1)));
    x->foff = b->off;
    x->ffac = b->fac;
    x->fexp = b->exp;
    x->E = b->E;
    x->Ecap = b->E;
    x->factored = 1;
}

static void f_free(X *x) {
    if (!x->factored) return;
    ACCT(-(int64_t)fbytes(x->W, x->Ecap, x->w));
    free(x->foff);
    free(x->ffac);
    free(x->fexp);
    x->foff = NULL;
    x->ffac = NULL;
    x->fexp = NULL;
    x->factored = 0;
}

/* a raw multiplication of factor work, counted apart from the core's (v0.3 §16.5, "Forming a gcd's value") */
static inline sk_int fmul(X *c, sk_int a, sk_int b) {
    (void)c;
    sk_int r;
    AC(c, fmul, 1);
    if (sk_mul_raw(a, b, &r)) sk_die("mh3: a factor product past 64 bits");
    return r;
}

/* ----------------------------------------------------------------------------------------- the coprime base (q) */

#if MH3_COPRIME
static inline sk_int piece_get(const X *x, uint32_t i) { return slot_get(x->piece, x->w, i); }

/* append a piece to (q), growing it by doubling (from 16) */
static void piece_push(X *x, sk_int q) {
    if (x->nq == x->qcap) {
        uint32_t c2 = x->qcap ? 2 * x->qcap : 16;
        x->piece = sk_xrealloc(x->piece, (size_t)c2 * (x->w / 8));
        ACCT((uint64_t)(c2 - x->qcap) * (x->w / 8));
        x->qcap = c2;
    }
    slot_set(x->piece, x->w, x->nq++, q);
}

/* shrink (q) to its pieces */
static void piece_shrink(X *x) {
    if (x->qcap == x->nq) return;
    if (x->nq == 0) {
        free(x->piece);
        x->piece = NULL;
    } else {
        x->piece = sk_xrealloc(x->piece, (size_t)x->nq * (x->w / 8));
    }
    ACCT(-(int64_t)((uint64_t)(x->qcap - x->nq) * (x->w / 8)));
    x->qcap = x->nq;
}

/* drop the 1s of S and every later duplicate (a step of the notation note's §4: "Drop any 1s and merge duplicates") */
static void s_compact(sk_int *S, uint32_t *n) {
    uint32_t k = 0;
    for (uint32_t i = 0; i < *n; i++) {
        if (S[i] == 1) continue;
        int dup = 0;
        for (uint32_t j = 0; j < k && !dup; j++) dup = S[j] == S[i];
        if (!dup) S[k++] = S[i];
    }
    *n = k;
}

/* S := the coprime base of its numbers, by gcds alone (notation note §4): repeat until nothing changes: find the first
 * two numbers (i < j, in order) that share a factor; replace them by their gcd (at i) and their two residuals (appended,
 * j removed); drop 1s and merge duplicates.  S is the scratch of v0.3 §16.3, at most SMAX entries. */
static void cbase(sk_int *S, uint32_t *n, uint64_t *ngcd, uint64_t *ndiv) {
    s_compact(S, n);
    for (;;) {
        int changed = 0;
        for (uint32_t i = 0; i < *n && !changed; i++)
            for (uint32_t j = i + 1; j < *n && !changed; j++) {
                (*ngcd)++;
                sk_int g = sk_gcd_raw(S[i], S[j]);
                if (g == 1) continue;
                sk_int a = S[i] / g, b = S[j] / g;
                *ndiv += 2;
                S[i] = g;
                for (uint32_t k = j + 1; k < *n; k++) S[k - 1] = S[k];
                (*n)--;
                if (*n + 2 > SMAX) sk_die("mh3: refine's working set S past 32 entries");
                S[(*n)++] = a;
                S[(*n)++] = b;
                s_compact(S, n);
                changed = 1;
            }
        if (!changed) return;
    }
}

/* nu_s(*v) by divisibility tests, each one division with remainder (counted in *ndiv), *v divided by s as often as
 * it goes */
static unsigned val_div(sk_int *v, sk_int s, uint64_t *ndiv) {
    unsigned e = 0;
    for (;;) {
        (*ndiv)++;
        if (*v % s) return e;
        *v /= s;
        e++;
    }
}
#endif

/* ----------------------------------------------------------------------------------------- the factoring (§16.4) */

/* Factor every whole number of (l), once, in list order, into (f) (v0.3 §16.4 items 2-4), after the packing: (h) and
 * (p) are already freed.  0 and 1 get empty lists. */
static void factor(X *x) {
    if (!x->packed || x->tab || x->factored) sk_die("mh3: a factoring outside the end of a packed build");
    uint32_t W = x->W;
    fbuf b;
#if !MH3_COPRIME
    fbuf_new(&b, W, x->w, 2 * W);
    for (uint32_t k = 0; k < W; k++) { /* schoolbook trial division (§16.4 item 3) */
        b.off[k] = b.E;
        sk_int v = slot_get(x->list, x->w, k);
        if (v < 2) continue;
        uint64_t m = (uint64_t)v, d = 2;
        while (d * d <= m) { /* d < 2^32 since m < 2^63, so d d fits 64 bits unsigned */
            unsigned e = 0;
            FC(x, td_cand, 1);
            for (;;) { /* each "mod" one division with remainder; each "m <- m / d" one exact division */
                FC(x, td_divs, 1);
                if (m % d != 0) break;
                m = m / d;
                FC(x, td_exact, 1);
                e++;
            }
            if (e > 0) fbuf_put(&b, k, (sk_int)d, e);
            d = d == 2 ? 3 : d + 2;
        }
        if (m > 1) fbuf_put(&b, k, (sk_int)m, 1); /* the cofactor left is prime */
    }
#else
    /* pass 1, the base: for each v >= 2 in list order, 09's absorb scan through (q), never going back */
    for (uint32_t k = 0; k < W; k++) {
        sk_int v = slot_get(x->list, x->w, k);
        if (v < 2) continue;
        sk_int rest = v;
        uint32_t p = 0;
        while (rest != 1 && p < x->nq) {
            sk_int q = piece_get(x, p);
            FC(x, rgcd, 1);
            sk_int h = sk_gcd_raw(rest, q);
            if (h == 1) {
                p++;
            } else if (h == q) { /* divide rest by q as many times as it goes, and stay at q */
                for (;;) {
                    FC(x, rdiv, 1);
                    if (rest % q) break;
                    rest /= q;
                }
            } else { /* split q into the coprime base of {h, q/h}; the first piece takes q's index; stay */
                sk_int S[SMAX];
                uint32_t n = 2;
                uint64_t ng = 0, nd = 1; /* q/h: one division */
                S[0] = h;
                S[1] = q / h;
                cbase(S, &n, &ng, &nd);
                FC(x, sgcd, ng);
                FC(x, sdiv, nd);
                FC(x, splits, 1);
                slot_set(x->piece, x->w, p, S[0]);
                for (uint32_t i = 1; i < n; i++) piece_push(x, S[i]);
            }
        }
        if (rest != 1) piece_push(x, rest);
    }
    piece_shrink(x);
    /* pass 2, the factorizations over the final base (§16.4 item 4) */
    fbuf_new(&b, W, x->w, 2 * W);
    for (uint32_t k = 0; k < W; k++) {
        b.off[k] = b.E;
        sk_int m = slot_get(x->list, x->w, k);
        for (uint32_t i = 0; i < x->nq && m > 1; i++) {
            sk_int q = piece_get(x, i);
            unsigned e = 0;
            for (;;) {
                FC(x, p2tests, 1);
                if (m % q) break;
                m /= q;
                e++;
            }
            if (e > 0) fbuf_put(&b, k, q, e);
        }
        if (m > 1) sk_die("mh3: a whole number not a product of the base's pieces");
    }
#endif
    fbuf_finish(x, &b);
    if (m3_factor_hook) {
        m3_factor r;
        memset(&r, 0, sizeof r);
        r.id = x->id;
        r.x = x;
        MH3F(ffigures)(x, &r.fig);
#ifdef SK_COUNT
        r.fc = x->fc;
#endif
        m3_factor_hook(&r);
    }
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

/* Store a whole number v (0 <= v <= 2^63 - 1) in x's build table (h); returns its slot, its handle.  Every value, 0
 * and 1 included, is probed from h(v): found, or new.  A new value: in a result, a widening if v needs more than w
 * bits; then v is written into the empty slot. */
static uint32_t store_whole(X *x, sk_int v) {
    uint32_t n;
    int found;
    uint32_t s = probe(x->tab, x->T, x->t, x->w, v, &n, &found);
    if (found == 1) {
        COUNT_STORE(x, 1, n);
        return s;
    }
    if (found < 0) full(x->T);
    unsigned need = (unsigned)sk_bits_u_raw((uint64_t)v) + 1; /* two's complement */
    if (need > x->w) widen(x, need <= 16 ? 16 : need <= 32 ? 32 : 64); /* results only: inputs are 64-bit */
    slot_set(x->tab, x->w, s, v);
    x->W++;
    COUNT_STORE(x, 0, n);
    return s;
}

/* Store a rational in the next cell (v0.1 §5.7): the numerator's whole number, its field (handle, sign tag) written to
 * the cell; then the denominator's, its field (handle, tag 0). */
static void store_cell(X *x, uint32_t cell, bool neg, sk_int mag, sk_int e) {
    if (x->nf != 2 * cell) sk_die("mh3: a cell stored out of the order of entry");
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

/* A matrix of R x K cells, as Hash-V-pack's: the bound 2RK + 2; T_fix the smallest power of two at least the bound; w_h
 * the smallest of 8, 16, 32 with w_h - 1 >= log2 T_fix; (h) at T_fix slots, at 64 bits (an input) or 8 (a result), all
 * empty. */
X *MH3F(create)(uint32_t R, uint32_t K, int id) {
    if (R == 0 || K == 0 || (uint64_t)R * K > (1u << 28) || id < M2_A || id >= M2_NMAT) sk_die("mh3: bad dimensions");
    uint64_t kb = 2 * (uint64_t)R * K + 2;
    unsigned tf = 0;
    while ((1ull << tf) < kb) tf++;
    unsigned need = tf; /* log2 of the largest table the handles must name */
    X *x = sk_xcalloc(1, sizeof *x); /* the header */
    x->wh = need <= 7 ? 8 : need <= 15 ? 16 : need <= 31 ? 32 : 0;
    if (!x->wh) sk_die("mh3: dimensions beyond 32-bit handles");
    x->R = R;
    x->K = K;
    x->id = id;
    x->t = tf;
    x->T = 1u << x->t;
    x->w = M2_INPUT(id) ? 64 : 8;
    x->cells = sk_xmalloc((size_t)2 * R * K * (x->wh / 8)); /* (a) */
    ACCT((uint64_t)2 * R * K * (x->wh / 8));
    x->tab = table_new(x->T, x->w); /* (h) */
    x->W = 0;
#ifdef SK_COUNT
    x->xcur = &x_sink;
#endif
    return x;
}

/* the bytes of the cells (a) and of (h) or the list (l) */
static uint64_t bytes_cells(const X *x) { return (uint64_t)2 * x->R * x->K * (x->wh / 8); }
static uint64_t bytes_vals(const X *x) {
    if (x->packed) return (uint64_t)x->W * (x->w / 8);
    return (uint64_t)x->T * (x->w / 8);
}
static uint64_t bytes_f(const X *x) { return x->factored ? fbytes(x->W, x->Ecap, x->w) : 0; }
static uint64_t bytes_q(const X *x) {
#if MH3_COPRIME
    return (uint64_t)x->qcap * (x->w / 8);
#else
    (void)x;
    return 0;
#endif
}

/* Freeing a matrix frees all its structures (v0.2 §15.5 item 6). */
void MH3F(destroy)(X *x) {
    if (!x) return;
    ACCT(-(int64_t)(bytes_cells(x) + bytes_vals(x) + bytes_q(x)));
    f_free(x);
    free(x->cells);
    free(x->tab);
    free(x->list);
#if MH3_COPRIME
    free(x->piece);
#endif
    free(x);
}

#define OVF(step) return sk_overflow_at(&c->ovf, &c->ctx, (step))

/* gcd, as q32h.c's (without the profile's brackets: no profiling build): the entry's (Hash-V-pack's), and Pack-coprime's
 * per-piece gcds */
static inline sk_int gcdp(X *c, sk_int p, sk_int q) {
    (void)c;
    return sk_gcd(p, q);
}

/* Enter m / d as the next cell of an input, in the order of entry: reduced gcd-first as tc3.c's entry, then stored.  The
 * input's build ends with its last cell, so it is packed and factored inside that call (v0.3 §16.4 item 5). */
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

sk_status MH3F(enter)(X *c, sk_int m, sk_int d) {
    if (!sk_valid_input(m) || !sk_valid_input(d)) return SK_EINVAL;
    if (d == 0) return SK_EDIVZERO;
    if (c->nf >= 2 * c->R * c->K || !M2_INPUT(c->id)) return SK_EINVAL;
    sk_status st = enter_cell(c, m, d);
    if (st == SK_OK && c->nf == 2 * c->R * c->K) {
        pack(c);
        factor(c);
    }
    return st;
}

/* ----------------------------------------------------------------------------------------- operands (v0.1 §§6, 7) */

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
 * when that is at hand with the same handle, or read from its matrix's list by handle.  Returns 1 if a value was read:
 * each distinct handle of one matrix is read once (K-Q3). */
static inline int rd1(ops2 *o, int k) {
    if ((o->have >> k) & 1) return 0;
    int sib = k ^ 1, slot = 0;
    if (((o->have >> sib) & 1) && o->idx[sib] == o->idx[k]) {
        o->v[k] = o->v[sib];
    } else {
        const X *m = k < 2 ? o->mx : o->my;
        o->v[k] = slot_get(vals(m), m->w, o->idx[k]);
        slot = 1;
    }
    o->have |= 1u << k;
    return slot;
}

/* Put the values in `need` (bit k: position k) at hand; each value read counts as a K-Q3 read.  In V every value of
 * an operation on two cells is at hand before a case is chosen, so this reads nothing there (DECISIONS M1-D8). */
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
    return slot_get(vals(m), m->w, o->idx[k]);
}

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

/* Two cells, x of the left operand and y of the right: V reads a, b, c, d (each distinct handle of one matrix once:
 * the cross reads) and every test compares those values (v0.2 §15.2: no handle is tested against 0 or 1). */
static inline eq compare_cells(X *c, ops2 *o, int six) {
    (void)c;
    eq q;
    for (int k = 0; k < 4; k++) {
        int read = rd1(o, k);
        OC(c, cross_reads, read);
        (void)read;
    }
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

/* ----------------------------------------------------------------------------------------- factorizations, merges */

/* (t), a temporary's denominator list (v0.3 §16.3): at most 15 entries, sorted by factor */
typedef struct flist {
    uint32_t n;
    sk_int q[FMAX + 1];
    uint8_t e[FMAX + 1];
} flist;

/* a factorization being read: of a stored whole number, in its matrix's (f) at width w; or a temporary's list (t) */
typedef struct lview {
    const void *q;
    unsigned w;
    const uint8_t *e;
    uint32_t n;
} lview;

static inline sk_int lq(lview v, uint32_t i) { return slot_get(v.q, v.w, i); }
static inline lview tview(const flist *l) { return (lview){l->q, 64, l->e, l->n}; }

/* F(v) of the whole number at handle k of a stored matrix m, read from (f).  The list of 0 is empty and is never read
 * (§16.6 item 1): a read of it is counted (in the step's result c) and, in the test build, is a bug. */
static inline lview fview(X *c, const X *m, uint32_t k) {
    (void)c;
    if (slot_get(m->list, m->w, k) == 0) {
        AC(c, zero_reads, 1);
#ifdef SK_AUDIT
        sk_die("mh3: a read of the factorization of 0 (spec v0.3 §16.6 item 1)");
#endif
    }
    uint32_t o = m->foff[k];
    return (lview){(const char *)m->ffac + (size_t)o * (m->w / 8), m->w, m->fexp + o, m->foff[k + 1] - o};
}

/* one entry of the merge's gcd list (v0.3 §16.3): a common factor q, its smaller exponent e, whether the two exponents
 * are equal (an equal valuation, §16.2), and, in the additions, h's part at q: q^k (Pack-primes, and Pack-coprime once
 * known to be a power of q) or hq = gcd(t, q^e) (Pack-coprime) */
typedef struct gent {
    sk_int q, hq;
    uint8_t e, k, eq, known;
} gent;
typedef struct glist {
    uint32_t n;
    gent g[FMAX + 1];
} glist;

static inline void g_put(glist *G, sk_int q, unsigned e, int eq) {
    if (G->n >= FMAX) sk_die("mh3: a gcd list past 15 entries");
    G->g[G->n++] = (gent){q, 1, (uint8_t)e, (uint8_t)e, (uint8_t)eq, 0};
}

/* The merge (§16.2): one walk of two sorted factorizations together, comparing factors by value; writes the gcd list
 * (common factors, smaller exponent, and whether the exponents are equal).  Counted: the merge and each comparison. */
static void merge(X *c, lview x, lview y, glist *G) {
    (void)c;
    uint32_t i = 0, j = 0;
    G->n = 0;
    AC(c, merges, 1);
    while (i < x.n && j < y.n) {
        sk_int p = lq(x, i), q = lq(y, j);
        AC(c, mentries, 1);
        if (p == q) {
            g_put(G, p, x.e[i] < y.e[j] ? x.e[i] : y.e[j], x.e[i] == y.e[j]);
            i++;
            j++;
            continue;
        }
#ifdef SK_AUDIT
        if (sk_gcd_raw(p, q) != 1) sk_die("mh3: a merge compared two factors neither equal nor coprime");
#endif
        if (p < q) i++;
        else j++;
    }
}

/* a gcd list's value, prod q^e, multiplied out with overflow checks (factor work) */
static sk_int gval(X *c, const glist *G) {
    sk_int v = 1;
    for (uint32_t i = 0; i < G->n; i++)
        for (unsigned k = 0; k < G->g[i].e; k++) v = fmul(c, v, G->g[i].q);
    return v;
}

static inline int g_has_eq(const glist *G) {
    for (uint32_t i = 0; i < G->n; i++)
        if (G->g[i].eq) return 1;
    return 0;
}

/* in the test build, an obtained gcd against the 08 module's Euclid on the same operands (§16.9 item 3) */
#ifdef SK_AUDIT
#define GCHECK(c, got, p, q)                                                                                      \
    do {                                                                                                          \
        AC(c, gchecked, 1);                                                                                       \
        if ((got) != sk_gcd_raw((p), (q))) sk_die("mh3: an obtained gcd differs from Euclid's (spec v0.3 §16.9)"); \
    } while (0)
#else
#define GCHECK(c, got, p, q) ((void)0)
#endif

#if !MH3_COPRIME
/* Pack-primes: the part of h at a prime q: divisibility tests of u (t's magnitude, as it is divided) by q, at most nu
 * divisions, then one failed test (§16.6 item 1); h multiplied by q at each division (factor work). */
static unsigned ptests(X *c, sk_int *u, sk_int q, unsigned nu, sk_int *h) {
    unsigned k = 0;
    while (k < nu) {
        AC(c, tests, 1);
        if (*u % q) break;
        *u /= q;
        *h = fmul(c, *h, q);
        k++;
    }
    return k;
}
#else
/* Pack-coprime: q^nu (factor work), then one Euclid gcd gcd(t, q^nu) (the core's gcd, counted as a per-piece gcd),
 * and h multiplied by it (factor work). */
static sk_int pgcd(X *c, sk_int t, sk_int q, unsigned nu, sk_int *h) {
    sk_int qn = 1;
    for (unsigned i = 0; i < nu; i++) qn = fmul(c, qn, q);
    AC(c, pgcd, 1);
    sk_int hq = gcdp(c, t, qn);
    *h = fmul(c, *h, hq);
    return hq;
}
#endif

/* h = gcd(t, g) from the gcd list G, tested only at its factors of equal valuation (v0.3 §16.5 cases 9c, 10c; §16.6) */
static sk_int eq_gcd(X *c, sk_int t, glist *G) {
    sk_int h = 1;
#if !MH3_COPRIME
    sk_int u = t < 0 ? -t : t;
#endif
    for (uint32_t i = 0; i < G->n; i++) {
        gent *g = &G->g[i];
        if (!g->eq) continue;
#if !MH3_COPRIME
        g->k = (uint8_t)ptests(c, &u, g->q, g->e, &h);
        g->known = 1;
#else
        g->hq = pgcd(c, t, g->q, g->e, &h);
#endif
    }
    return h;
}

/* k = gcd(t, b) in case den, d = b: tests of t against every factor of b, all at equal valuation (§16.5 case 8); G is
 * written as k's list: every factor of b, its exponent, and k's part at it */
static sk_int den_gcd(X *c, sk_int t, lview fb, glist *G) {
    sk_int k = 1;
#if !MH3_COPRIME
    sk_int u = t < 0 ? -t : t;
#endif
    G->n = 0;
    for (uint32_t i = 0; i < fb.n; i++) {
        g_put(G, lq(fb, i), fb.e[i], 1);
        gent *g = &G->g[G->n - 1];
#if !MH3_COPRIME
        g->k = (uint8_t)ptests(c, &u, g->q, g->e, &k);
        g->known = 1;
#else
        g->hq = pgcd(c, t, g->q, g->e, &k);
#endif
    }
    return k;
}

/* ----------------------------------------------------------------------------------------- forming a temporary's list */

static inline void l_clear(flist *o) {
    if (o) o->n = 0;
}

/* append (q, e) at the end of o (the caller keeps it sorted) */
static inline void l_app(flist *o, sk_int q, unsigned e) {
    if (o->n >= FMAX) sk_die("mh3: a temporary's list past 15 entries");
    if (e > 255) sk_die("mh3: an exponent past 255");
    o->q[o->n] = q;
    o->e[o->n] = (uint8_t)e;
    o->n++;
}

static void l_copy(X *c, flist *o, lview a, unsigned mult) {
    (void)c;
    o->n = 0;
    for (uint32_t i = 0; i < a.n; i++) l_app(o, lq(a, i), (unsigned)a.e[i] * mult);
}

/* o := the union of a and b, exponents summed (a product's list: F(b) + F(d)) */
static void l_sum(X *c, flist *o, lview a, lview b) {
    (void)c;
    uint32_t i = 0, j = 0;
    o->n = 0;
    while (i < a.n || j < b.n) {
        if (i < a.n && j < b.n) {
            sk_int p = lq(a, i), q = lq(b, j);
            AC(c, lwalk, 1);
            if (p == q) {
                l_app(o, p, (unsigned)a.e[i++] + b.e[j++]);
            } else if (p < q) {
                l_app(o, p, a.e[i++]);
            } else {
                l_app(o, q, b.e[j++]);
            }
        } else if (i < a.n) {
            l_app(o, lq(a, i), a.e[i]);
            i++;
        } else {
            l_app(o, lq(b, j), b.e[j]);
            j++;
        }
    }
}

/* o := a less G's exponents k at G's factors (F(b) - F(k)); G's factors are a's (sorted) */
static void l_sub(X *c, flist *o, lview a, const glist *G) {
    (void)c;
    uint32_t j = 0;
    o->n = 0;
    for (uint32_t i = 0; i < a.n; i++) {
        sk_int p = lq(a, i);
        unsigned e = a.e[i];
        while (j < G->n) {
            AC(c, lwalk, 1);
            if (G->g[j].q < p) {
                j++;
                continue;
            }
            if (G->g[j].q == p) e -= G->g[j++].k;
            break;
        }
        if (e) l_app(o, p, e);
    }
}

/* o += a less G's exponents (the second half of a general product's list: + (F(d) - F(g))), each entry inserted in
 * place, sorted, its exponent added to an equal factor's */
static void l_addsub(X *c, flist *o, lview a, const glist *G) {
    (void)c;
    uint32_t j = 0;
    for (uint32_t i = 0; i < a.n; i++) {
        sk_int p = lq(a, i);
        unsigned e = a.e[i];
        while (j < G->n) {
            AC(c, lwalk, 1);
            if (G->g[j].q < p) {
                j++;
                continue;
            }
            if (G->g[j].q == p) e -= G->g[j++].k;
            break;
        }
        if (!e) continue;
        uint32_t at = 0;
        while (at < o->n) {
            AC(c, lwalk, 1);
            if (o->q[at] >= p) break;
            at++;
        }
        if (at < o->n && o->q[at] == p) {
            if ((unsigned)o->e[at] + e > 255) sk_die("mh3: an exponent past 255");
            o->e[at] = (uint8_t)(o->e[at] + e);
            continue;
        }
        if (o->n >= FMAX) sk_die("mh3: a temporary's list past 15 entries");
        for (uint32_t k = o->n; k > at; k--) {
            o->q[k] = o->q[k - 1];
            o->e[k] = o->e[k - 1];
        }
        o->q[at] = p;
        o->e[at] = (uint8_t)e;
        o->n++;
    }
}

/* o := the lcm's list less h's at the equal factors: the union of a and b, the larger exponent where unequal, and
 * nu - k where equal (G: the gcd list, with h's exponents k at its equal factors; §16.5 cases 9, 10) */
static void l_lcm(X *c, flist *o, lview a, lview b, const glist *G) {
    (void)c;
    uint32_t i = 0, j = 0, g = 0;
    o->n = 0;
    while (i < a.n || j < b.n) {
        if (i < a.n && j < b.n) {
            sk_int p = lq(a, i), q = lq(b, j);
            AC(c, lwalk, 1);
            if (p == q) {
                unsigned ea = a.e[i++], eb = b.e[j++], e;
                while (g < G->n && G->g[g].q < p) g++;
                if (g >= G->n || G->g[g].q != p) sk_die("mh3: a common factor not in the gcd list");
                e = ea == eb ? ea - G->g[g].k : ea > eb ? ea : eb;
                if (e) l_app(o, p, e);
            } else if (p < q) {
                l_app(o, p, a.e[i++]);
            } else {
                l_app(o, q, b.e[j++]);
            }
        } else if (i < a.n) {
            l_app(o, lq(a, i), a.e[i]);
            i++;
        } else {
            l_app(o, lq(b, j), b.e[j]);
            j++;
        }
    }
}

/* ----------------------------------------------------------------------------------------- splits (Pack-coprime) */

#if MH3_COPRIME
/* Rebuild x's (f) in one pass into (f'), each entry (q, nu) replaced by (P[i], nu e[i]) for q = prod P[i]^e[i], each
 * list re-sorted; then the old (f) freed (v0.3 §16.8).  Counted in the step's result c. */
static void rebuild(X *c, X *x, sk_int q, const sk_int *P, const uint8_t *e, uint32_t np) {
    (void)c;
    fbuf b;
    fbuf_new(&b, x->W, x->w, x->E + np); /* (f'), grown by doubling as it is written */
    for (uint32_t k = 0; k < x->W; k++) {
        b.off[k] = b.E;
        for (uint32_t j = x->foff[k]; j < x->foff[k + 1]; j++) {
            sk_int f = slot_get(x->ffac, x->w, j);
            unsigned nu = x->fexp[j];
            if (f != q) {
                fbuf_put(&b, k, f, nu);
                continue;
            }
            for (uint32_t i = 0; i < np; i++) fbuf_put(&b, k, P[i], nu * e[i]);
        }
    }
    f_free(x);
    fbuf_finish(x, &b);
    AC(c, rebuilds, 1);
    AC(c, rentries, x->E);
}

/* In matrix x, if its base holds the piece q: q's entry becomes P[0], the others are appended to (q), and (f) is
 * rebuilt.  Nothing is done when q's pieces are q itself. */
static void split_in(X *c, X *x, sk_int q, const sk_int *P, const uint8_t *e, uint32_t np) {
    if (np == 1 && P[0] == q && e[0] == 1) return;
    for (uint32_t i = 0; i < x->nq; i++)
        if (piece_get(x, i) == q) {
            slot_set(x->piece, x->w, i, P[0]);
            for (uint32_t k = 1; k < np; k++) piece_push(x, P[k]);
            rebuild(c, x, q, P, e, np);
            return;
        }
}

/* the pieces of v among S (those with nu_s(v) >= 1, in S's order) and their exponents; checks that v is their product */
static uint32_t pieces_of(X *c, sk_int v, const sk_int *S, uint32_t n, sk_int *P, uint8_t *e) {
    (void)c;
    uint32_t np = 0;
    uint64_t nd = 0;
    sk_int u = v;
    for (uint32_t i = 0; i < n; i++) {
        unsigned k = val_div(&u, S[i], &nd);
        if (!k) continue;
        P[np] = S[i];
        e[np++] = (uint8_t)k;
    }
    AC(c, sp_div, nd);
    if (u != 1 || np == 0) sk_die("mh3: a number not a product of its split's pieces");
    return np;
}

/* replace (q, nu) in a temporary's list by (P[i], nu e[i]), re-sorted (a live temporary at a leftover split) */
static void l_split(flist *l, sk_int q, const sk_int *P, const uint8_t *e, uint32_t np) {
    flist o = *l;
    uint32_t at = 0;
    for (; at < o.n && o.q[at] != q; at++) {
    }
    if (at == o.n) return;
    unsigned nu = o.e[at];
    l->n = 0;
    for (uint32_t i = 0; i < o.n; i++) /* every entry but q's, in order */
        if (i != at) l_app(l, o.q[i], o.e[i]);
    for (uint32_t i = 0; i < np; i++) { /* q's pieces, each inserted in place */
        uint32_t k = l->n;
        l_app(l, P[i], nu * e[i]);
        while (k > 0 && l->q[k - 1] > l->q[k]) {
            sk_int tq = l->q[k];
            uint8_t te = l->e[k];
            l->q[k] = l->q[k - 1];
            l->e[k] = l->e[k - 1];
            l->q[k - 1] = tq;
            l->e[k - 1] = te;
            k--;
        }
    }
}

/* the step's matrices and live temporaries, where a split must also go */
typedef struct stepctx {
    X *L, *R;          /* the operands */
    flist *s, *p;      /* the live temporaries' lists (multiplication's sums) */
} stepctx;

/* Pack-coprime, a temporary's sum: each equal factor q of G whose hq = gcd(t, q^nu) is checked to be a power q^k of q,
 * by divisibility tests; if it is not (a leftover, §16.8), q is split, as at reconciliation, into the pieces of the
 * coprime base of {q, hq}: in the step's matrices that hold it, in the live temporary lists s and p, and in G (each new
 * piece with its exponent and h's part at it). */
static void leftovers(X *c, glist *G, stepctx *sc) {
    for (uint32_t i = 0; i < G->n; i++) {
        gent *g = &G->g[i];
        if (!g->eq || g->known) continue;
        sk_int u = g->hq;
        unsigned k = 0;
        for (;;) {
            AC(c, tests, 1);
            if (u % g->q) break;
            u /= g->q;
            k++;
        }
        if (u == 1) {
            g->k = (uint8_t)k;
            g->known = 1;
            continue;
        }
        /* a leftover: split q */
        sk_int q = g->q, hq = g->hq, S[SMAX], P[SMAX];
        uint8_t e[SMAX];
        unsigned nu = g->e;
        uint32_t n = 2;
        uint64_t ng = 0, nd = 0;
        S[0] = q;
        S[1] = hq;
        cbase(S, &n, &ng, &nd);
        AC(c, sp_gcd, ng);
        AC(c, sp_div, nd);
        AC(c, lsplits, 1);
        uint32_t np = pieces_of(c, q, S, n, P, e);
        split_in(c, sc->L, q, P, e, np);
        if (sc->R != sc->L) split_in(c, sc->R, q, P, e, np);
        l_split(sc->s, q, P, e, np);
        l_split(sc->p, q, P, e, np);
        /* G: the entry of q becomes one entry per piece, with h's part at it (nu_{P[j]}(hq), by tests); re-sorted */
        glist o = *G;
        G->n = 0;
        for (uint32_t j = 0; j < o.n; j++)
            if (j != i) G->g[G->n++] = o.g[j];
        sk_int left = hq;
        uint64_t nd2 = 0;
        for (uint32_t j = 0; j < np; j++) {
            unsigned f = val_div(&left, P[j], &nd2);
            g_put(G, P[j], nu * e[j], 1);
            G->g[G->n - 1].k = (uint8_t)f;
            G->g[G->n - 1].known = 1;
            uint32_t z = G->n - 1;
            while (z > 0 && G->g[z - 1].q > G->g[z].q) {
                gent t = G->g[z];
                G->g[z] = G->g[z - 1];
                G->g[z - 1] = t;
                z--;
            }
        }
        AC(c, sp_div, nd2);
        if (left != 1) sk_die("mh3: a leftover's gcd not a product of its piece's pieces");
        i = (uint32_t)-1; /* G re-sorted: start again (the entries settled are skipped) */
    }
}
#endif

/* ----------------------------------------------------------------------------------------- addition (v0.3 §17.3) */

/* The factored context of one addition: x's and y's denominator lists (F(b), F(d): from (f) for two cells, or the
 * temporaries' lists (t) for add(s, p)), and the result's list (t) when the result is a temporary (NULL: stored). */
typedef struct fctx {
    lview fb, fd;
    flist *lb, *ld; /* the temporaries' lists, when x and y are temporaries (their views are re-read after a split) */
    flist *out;
#if MH3_COPRIME
    stepctx *sc;
#endif
} fctx;

static inline void f_reread(fctx *f) {
    if (f->lb) f->fb = tview(f->lb);
    if (f->ld) f->fd = tview(f->ld);
}

/* the result's list of an addition whose reduction used the gcd list G (den: k's; num, general: g's, h at the equal
 * factors): Pack-coprime first settles every hq (a leftover splits); then F(b) - F(k), or the lcm less h */
static void list_reduced(X *c, fctx *f, glist *G, int den) {
    if (!f->out) return;
#if MH3_COPRIME
    leftovers(c, G, f->sc);
    f_reread(f);
#endif
    if (den) l_sub(c, f->out, f->fb, G);
    else l_lcm(c, f->out, f->fb, f->fd, G);
}

/* x + y with the tags xneg, yneg, the comparisons q made, the operands o (read as each case needs them), the result to
 * out.  mh2.c's add_body (q32h.c's addsub body, without the hints), with each gcd obtained from the factorizations
 * (v0.3 §16.5) and, when the result is a temporary, its list formed. */
static sk_status add_body(X *c, eq q, bool xneg, bool yneg, ops2 *o, dest out, fctx *f) {
    sk_int *v = o->v;
    sk_int p, r, t, e, n, g, h;
    glist G;
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
        l_clear(f->out);
        return store(c, 0, 1, R2_ADD_ZERO, out);
    }
    if (q.zx) { /* case zero_x: x = 0, x + y = s_y*y: no arithmetic (v0.3 §17.3) */
        XCASE(c, R2_ADD_ZERO_X);
        rdset(c, o, 0xC); /* c, d */
        if (f->out) l_copy(c, f->out, f->fd, 1);
        return store(c, SGN(c, yneg, v[2]), v[3], R2_ADD_ZERO_X, out);
    }
    if (q.zy) { /* case zero_y: y = 0, x + y = x: no arithmetic */
        XCASE(c, R2_ADD_ZERO_Y);
        rdset(c, o, 0x3); /* a, b */
        if (f->out) l_copy(c, f->out, f->fb, 1);
        return store(c, SGN(c, xneg, v[0]), v[1], R2_ADD_ZERO_Y, out);
    }
    if (q.ix && q.iy) { /* case int_both: b = d = 1, (s_x*a + s_y*c)/1: no gcd, no multiplication (v0.3 §17.3) */
        XCASE(c, R2_ADD_INT_BOTH);
        rdset(c, o, 0x5); /* a, c */
        if (sk_add(SGN(c, xneg, v[0]), SGN(c, yneg, v[2]), &t)) OVF("add int_both: t = s_x*a + s_y*c");
        l_clear(f->out);
        return store(c, t, 1, R2_ADD_INT_BOTH, out);
    }
    if (q.ix) { /* case int_x: b = 1, (s_x*a*d + s_y*c)/d, in lowest terms as gcd(c, d) = 1: no gcd */
        XCASE(c, R2_ADD_INT_X);
        rdset(c, o, 0xD); /* a, c, d */
        if (sk_mul(SGN(c, xneg, v[0]), v[3], &p)) OVF("add int_x: s_x*a*d");
        if (sk_add(p, SGN(c, yneg, v[2]), &t)) OVF("add int_x: s_x*a*d + s_y*c");
        if (f->out) l_copy(c, f->out, f->fd, 1);
        return store(c, t, v[3], R2_ADD_INT_X, out);
    }
    if (q.iy) { /* case int_y: d = 1, (s_x*a + s_y*c*b)/b, in lowest terms as gcd(a, b) = 1: no gcd */
        XCASE(c, R2_ADD_INT_Y);
        rdset(c, o, 0x7); /* a, b, c */
        if (sk_mul(SGN(c, yneg, v[2]), v[1], &r)) OVF("add int_y: s_y*c*b");
        if (sk_add(SGN(c, xneg, v[0]), r, &t)) OVF("add int_y: s_x*a + s_y*c*b");
        if (f->out) l_copy(c, f->out, f->fb, 1);
        return store(c, t, v[1], R2_ADD_INT_Y, out);
    }
    if (q.ad || q.cb) { /* case cross: gcd(b, d) = 1 */
        XCASE(c, R2_ADD_CROSS);
        rdset(c, o, 0xF);
        if (sk_mul(SGN(c, xneg, v[0]), v[3], &p)) OVF("add cross: s_x*a*d");
        if (sk_mul(SGN(c, yneg, v[2]), v[1], &r)) OVF("add cross: s_y*c*b");
        if (sk_add(p, r, &t)) OVF("add cross: s_x*a*d + s_y*c*b");
        if (sk_mul(v[1], v[3], &e)) OVF("add cross: b*d");
        if (f->out) l_sum(c, f->out, f->fb, f->fd);
        return store(c, t, e, R2_ADD_CROSS, out);
    }
    if (q.den) { /* case den: gcd(b, d) = d */
        XCASE(c, R2_ADD_DEN);
        rdset(c, o, 0xD); /* a, c, d */
        if (sk_add(SGN(c, xneg, v[0]), SGN(c, yneg, v[2]), &t)) OVF("add den: t = s_x*a + s_y*c");
        h = den_gcd(c, t, f->fb, &G); /* k by tests of t against every factor of b, all at equal valuation */
        GCHECK(c, h, t, v[3]);
        if (h == 1 && KON(c, 3)) { /* [K-3] */
            KC(c, 3, ops, 1);
            KC(c, 3, div, 2);
            list_reduced(c, f, &G, 1);
            return store(c, t, v[3], R2_ADD_DEN, out);
        }
        n = sk_divexact(t, h);
        e = sk_divexact(v[3], h);
        list_reduced(c, f, &G, 1);
        return store(c, n, e, R2_ADD_DEN, out);
    }
    if (q.num) { /* case num: gcd(a, b*d) = 1 */
        XCASE(c, R2_ADD_NUM);
        rdset(c, o, 0xB); /* a, b, d */
        merge(c, f->fb, f->fd, &G); /* g by merge */
        g = gval(c, &G);
        GCHECK(c, g, v[1], v[3]);
        AC(c, sub[G.n == 0 ? M3_NUM_COP : g_has_eq(&G) ? M3_NUM_EQ : M3_NUM_UNEQ], 1);
        if (g == 1 && KON(c, 4)) { /* [K-4] */
            if (sk_add(SGN(c, xneg, v[3]), SGN(c, yneg, v[1]), &t)) OVF("add num: t = s_x*d + s_y*b");
            KC(c, 4, ops, 1);
            KC(c, 4, div, 4);
            KC(c, 4, gcd, 1);
            KC(c, 4, gcd_work, r2_work(t, 1));
            if (sk_mul(v[0], t, &n)) OVF("add num: a*t");
            if (sk_mul(v[1], v[3], &e)) OVF("add num: b*d");
            list_reduced(c, f, &G, 0);
            return store(c, n, e, R2_ADD_NUM, out);
        }
        sk_int bp = sk_divexact(v[1], g), dp = sk_divexact(v[3], g);
        if (sk_add(SGN(c, xneg, dp), SGN(c, yneg, bp), &t)) OVF("add num: t = s_x*d' + s_y*b'");
        h = eq_gcd(c, t, &G); /* 9b: h = 1 known, no test; 9c: tests only at the equal factors */
        GCHECK(c, h, t, g);
        if (h == 1 && KON(c, 5)) { /* [K-5] */
            KC(c, 5, ops, 1);
            KC(c, 5, div, 2);
            if (sk_mul(v[0], t, &n)) OVF("add num: a*t");
            if (sk_mul(bp, v[3], &e)) OVF("add num: b'*d");
            list_reduced(c, f, &G, 0);
            return store(c, n, e, R2_ADD_NUM, out);
        }
        sk_int tq = sk_divexact(t, h), dq = sk_divexact(v[3], h);
        if (sk_mul(v[0], tq, &n)) OVF("add num: a*(t/h)");
        if (sk_mul(bp, dq, &e)) OVF("add num: b'*(d/h)");
        list_reduced(c, f, &G, 0);
        return store(c, n, e, R2_ADD_NUM, out);
    }
    XCASE(c, R2_ADD_GEN); /* case general */
    rdset(c, o, 0xF);
    merge(c, f->fb, f->fd, &G); /* g by merge */
    g = gval(c, &G);
    GCHECK(c, g, v[1], v[3]);
    AC(c, sub[G.n == 0 ? M3_GEN_COP : g_has_eq(&G) ? M3_GEN_EQ : M3_GEN_UNEQ], 1);
    if (g == 1 && KON(c, 6)) { /* [K-6] the direct form is in lowest terms */
        if (sk_mul(SGN(c, xneg, v[0]), v[3], &p)) OVF("add general: s_x*a*d");
        if (sk_mul(SGN(c, yneg, v[2]), v[1], &r)) OVF("add general: s_y*c*b");
        if (sk_add(p, r, &t)) OVF("add general: s_x*a*d + s_y*c*b");
        if (sk_mul(v[1], v[3], &e)) OVF("add general: b*d");
        KC(c, 6, ops, 1);
        KC(c, 6, div, 4);
        KC(c, 6, gcd, 1);
        KC(c, 6, gcd_work, r2_work(t, 1));
        list_reduced(c, f, &G, 0);
        return store(c, t, e, R2_ADD_GEN, out);
    }
    sk_int bp = sk_divexact(v[1], g), dp = sk_divexact(v[3], g);
    if (sk_mul(SGN(c, xneg, v[0]), dp, &p)) OVF("add general: s_x*a*d'");
    if (sk_mul(SGN(c, yneg, v[2]), bp, &r)) OVF("add general: s_y*c*b'");
    if (sk_add(p, r, &t)) OVF("add general: t = s_x*a*d' + s_y*c*b'");
    h = eq_gcd(c, t, &G); /* 10b: h = 1 known, no test; 10c: tests only at the equal factors */
    GCHECK(c, h, t, g);
    if (h == 1 && KON(c, 7)) { /* [K-7] */
        KC(c, 7, ops, 1);
        KC(c, 7, div, 2);
        if (sk_mul(bp, v[3], &e)) OVF("add general: b'*d");
        list_reduced(c, f, &G, 0);
        return store(c, t, e, R2_ADD_GEN, out);
    }
    n = sk_divexact(t, h);
    sk_int dq = sk_divexact(v[3], h);
    if (sk_mul(bp, dq, &e)) OVF("add general: b'*(d/h)");
    list_reduced(c, f, &G, 0);
    return store(c, n, e, R2_ADD_GEN, out);
}

/* addition's operands: cell xc of A and cell yc of B (§6.1), the result to cell `cell` of C (stored: no list) */
static sk_status add_cells(X *c, X *a, uint32_t xc, X *b, uint32_t yc, dest out) {
    begin(c, SK_PH_ADD);
    opnd x = operand(a, xc), y = operand(b, yc);
    ops2 o = {a, b, {x.i, x.j, y.i, y.j}, {0, 0, 0, 0}, 0};
    eq q = compare_cells(c, &o, 0);
    fctx f;
    memset(&f, 0, sizeof f);
    if (!q.zx && !q.zy && !(q.ix && q.iy) && !q.ix && !q.iy && !q.ad && !q.cb) { /* den, num, general: F(b), F(d) */
        f.fb = fview(c, a, x.j);
        f.fd = fview(c, b, y.j);
    }
    return add_body(c, q, x.neg, y.neg, &o, out, &f);
}

/* multiplication's add(s, p): two local pairs, every value at hand (§6.2 item 3), each with its denominator's list;
 * the result (to s) with its list (§16.6 item 4) */
static sk_status add_values(X *c, tpair s, tpair p, flist *ls, flist *lp, dest out, void *scp) {
    begin(c, SK_PH_ADD);
    ops2 o = {NULL, NULL, {0, 0, 0, 0}, {s.m < 0 ? -s.m : s.m, s.d, p.m < 0 ? -p.m : p.m, p.d}, 0xF};
    eq q = compare_values(c, &o);
    flist res;
    fctx f = {tview(ls), tview(lp), ls, lp, &res
#if MH3_COPRIME
              ,
              (stepctx *)scp
#endif
    };
    (void)scp;
    sk_status st = add_body(c, q, s.m < 0, p.m < 0, &o, out, &f);
    if (st == SK_OK) *ls = res; /* the sum's list goes with the sum, to s */
    return st;
}

/* ----------------------------------------------------------------------------------------- multiplication */

/* x * y, cell xc of A and cell yc of B (§6.2), the result to a local pair with its denominator's list lp (§16.5's
 * multiplication table).  mh2.c's mul_cells (q32h.c's multiply body, without the hints), with each gcd obtained by a
 * merge of the operands' factorizations: no multiplication makes a divisibility test. */
static sk_status mul_cells(X *c, X *a, uint32_t xc, X *b, uint32_t yc, dest out, flist *lp) {
    begin(c, SK_PH_MUL);
    opnd x = operand(a, xc), y = operand(b, yc);
    ops2 o = {a, b, {x.i, x.j, y.i, y.j}, {0, 0, 0, 0}, 0};
    eq q = compare_cells(c, &o, 1);
    sk_int *v = o.v;
    bool s = x.neg != y.neg; /* s = s_x * s_y */
    sk_int n, e, h;
    glist G, G2;
    if (q.zx || q.zy) { /* case zero: a zero factor, x*y = 0/1: no arithmetic (v0.3 §17.3) */
        XCASE(c, R2_MUL_ZERO);
        lp->n = 0;
        return store(c, 0, 1, R2_MUL_ZERO, out);
    }
    if (q.ox) { /* case one_x: x = +-1, x*y = s_x*y  [K-15] */
        XCASE(c, R2_MUL_ONE_X);
        KC(c, 15, ops, 1);
        rdset(c, &o, 0xC); /* c, d */
        l_copy(c, lp, fview(c, b, y.j), 1);
        return store(c, SGN(c, s, v[2]), v[3], R2_MUL_ONE_X, out);
    }
    if (q.oy) { /* case one_y: y = +-1, x*y = s_y*x  [K-15] */
        XCASE(c, R2_MUL_ONE_Y);
        KC(c, 15, ops, 1);
        rdset(c, &o, 0x3); /* a, b */
        l_copy(c, lp, fview(c, a, x.j), 1);
        return store(c, SGN(c, s, v[0]), v[1], R2_MUL_ONE_Y, out);
    }
    if (q.ix && q.iy) { /* case int_both: b = d = 1, (a*c)/1: no gcd (v0.3 §17.3) */
        XCASE(c, R2_MUL_INT_BOTH);
        rdset(c, &o, 0x5); /* a, c */
        if (sk_mul(v[0], v[2], &n)) OVF("multiply int_both: a*c");
        lp->n = 0;
        return store(c, SGN(c, s, n), 1, R2_MUL_INT_BOTH, out);
    }
    if (q.ad && q.cb) { /* case both: a = d, c = b  [K-12] */
        XCASE(c, R2_MUL_BOTH);
        KC(c, 12, ops, 1);
        lp->n = 0;
        return store(c, SGN(c, s, 1), 1, R2_MUL_BOTH, out);
    }
    if (q.den) { /* case den: no gcd */
        XCASE(c, R2_MUL_DEN);
        rdset(c, &o, 0xD); /* a, c, d */
        if (sk_mul(v[0], v[2], &n)) OVF("multiply den: a*c");
        if (sk_mul(v[3], v[3], &e)) OVF("multiply den: d*d");
        l_copy(c, lp, fview(c, a, x.j), 2); /* 2 F(b) */
        return store(c, SGN(c, s, n), e, R2_MUL_DEN, out);
    }
    if (q.num) { /* case num: no gcd */
        XCASE(c, R2_MUL_NUM);
        rdset(c, &o, 0xB); /* a, b, d */
        if (sk_mul(v[0], v[0], &n)) OVF("multiply num: a*a");
        if (sk_mul(v[1], v[3], &e)) OVF("multiply num: b*d");
        l_sum(c, lp, fview(c, a, x.j), fview(c, b, y.j)); /* F(b) + F(d) */
        return store(c, SGN(c, s, n), e, R2_MUL_NUM, out);
    }
    if (q.ad) { /* case cross_ad: a cancels */
        XCASE(c, R2_MUL_CROSS_AD);
        rdset(c, &o, 0x6); /* b, c */
        lview fb = fview(c, a, x.j);
        merge(c, fview(c, b, y.i), fb, &G); /* k = gcd(c, b): merge F(c), F(b) */
        h = gval(c, &G);
        GCHECK(c, h, v[2], v[1]);
        l_sub(c, lp, fb, &G); /* F(b) - F(k) */
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
        lview fd = fview(c, b, y.j);
        merge(c, fview(c, a, x.i), fd, &G); /* k = gcd(a, d): merge F(a), F(d) */
        h = gval(c, &G);
        GCHECK(c, h, v[0], v[3]);
        l_sub(c, lp, fd, &G); /* F(d) - F(k) */
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
        lview fd = fview(c, b, y.j);
        merge(c, fview(c, a, x.i), fd, &G); /* h = gcd(a, d): merge F(a), F(d) */
        h = gval(c, &G);
        GCHECK(c, h, v[0], v[3]);
        l_sub(c, lp, fd, &G); /* F(d) - F(h) */
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
        lview fb = fview(c, a, x.j);
        merge(c, fview(c, b, y.i), fb, &G); /* h = gcd(c, b): merge F(c), F(b) */
        h = gval(c, &G);
        GCHECK(c, h, v[2], v[1]);
        l_sub(c, lp, fb, &G); /* F(b) - F(h) */
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
    lview fb = fview(c, a, x.j), fd = fview(c, b, y.j);
    merge(c, fview(c, a, x.i), fd, &G);  /* g = gcd(a, d): merge F(a), F(d) */
    merge(c, fview(c, b, y.i), fb, &G2); /* h = gcd(c, b): merge F(c), F(b) */
    sk_int g1 = gval(c, &G);
    sk_int g2 = gval(c, &G2);
    GCHECK(c, g1, v[0], v[3]);
    GCHECK(c, g2, v[2], v[1]);
    AC(c, sub[G.n == 0 && G2.n == 0 ? M3_MGEN_COP : G.n == 0 || G2.n == 0 ? M3_MGEN_ONE : M3_MGEN_TWO], 1);
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
    l_sub(c, lp, fb, &G2);    /* (F(b) - F(h)) */
    l_addsub(c, lp, fd, &G);  /*  + (F(d) - F(g)) */
    return store(c, SGN(c, s, n), e, R2_MUL_GEN, out);
}

/* ----------------------------------------------------------------------------------------- reconciliation (§16.7) */

#if MH3_COPRIME
/* (r), one of the denominator-factor lists D_L, D_R: 64-bit, grown by doubling, made at the start of the step and freed
 * at its end */
typedef struct dlist {
    sk_int *v;
    uint32_t n, cap;
} dlist;

static void d_push(dlist *D, sk_int q) {
    if (D->n == D->cap) {
        uint32_t c2 = D->cap ? 2 * D->cap : 16;
        D->v = sk_xrealloc(D->v, sizeof(sk_int) * c2);
        ACCT(8ull * (c2 - D->cap));
        D->cap = c2;
    }
    D->v[D->n++] = q;
}

static void d_free(dlist *D) {
    ACCT(-(int64_t)(8ull * D->cap));
    free(D->v);
    D->v = NULL;
    D->n = D->cap = 0;
}

/* D := the distinct factors in the lists of x's denominators: a scan of x's cells, reading each denominator's entries
 * in (f); duplicates found by a scan of the list so far */
static void d_make(const X *x, dlist *D) {
    D->n = 0;
    uint32_t nc = x->R * x->K;
    for (uint32_t cell = 0; cell < nc; cell++) {
        uint32_t k = fld_get(x->cells, x->wh, 2 * cell + 1) >> 1;
        for (uint32_t j = x->foff[k]; j < x->foff[k + 1]; j++) {
            sk_int q = slot_get(x->ffac, x->w, j);
            uint32_t i = 0;
            while (i < D->n && D->v[i] != q) i++;
            if (i == D->n) d_push(D, q);
        }
    }
}

/* refine(X, D): every piece q of X becomes equal or coprime to every r in D (§16.7).  A split of q against r: S, the
 * coprime base of {q, r} by gcds alone; q split into its pieces from S in every matrix of the step that holds it; r
 * likewise; D[i] becomes r's first piece and its other pieces are appended to D; the scan of X restarts from its first
 * piece.  Returns the splits made. */
static uint64_t refine(X *c, X *x, dlist *D, stepctx *sc) {
    uint64_t made = 0;
    for (uint32_t i = 0; i < D->n; i++) {
        uint32_t j = 0;
        while (j < x->nq) {
            sk_int q = piece_get(x, j), r = D->v[i];
            AC(c, rc_gcd, 1);
            sk_int h = sk_gcd_raw(q, r);
            if (h == 1 || q == r) {
                j++;
                continue;
            }
            sk_int S[SMAX], Pq[SMAX], Pr[SMAX];
            uint8_t eq_[SMAX], er[SMAX];
            uint32_t n = 2;
            uint64_t ng = 0, nd = 0;
            S[0] = q;
            S[1] = r;
            cbase(S, &n, &ng, &nd);
            AC(c, sp_gcd, ng);
            AC(c, sp_div, nd);
            AC(c, rc_splits, 1);
            made++;
            uint32_t nq_ = pieces_of(c, q, S, n, Pq, eq_), nr = pieces_of(c, r, S, n, Pr, er);
            split_in(c, sc->L, q, Pq, eq_, nq_);
            if (sc->R != sc->L) split_in(c, sc->R, q, Pq, eq_, nq_);
            split_in(c, sc->L, r, Pr, er, nr);
            if (sc->R != sc->L) split_in(c, sc->R, r, Pr, er, nr);
            D->v[i] = Pr[0];
            for (uint32_t k = 1; k < nr; k++) d_push(D, Pr[k]);
            j = 0; /* restart the scan of X from its first piece, against the new D[i] */
        }
    }
    return made;
}

/* reconcile(L, R), the first thing in madd and mmul: repeat { make D_L and D_R; refine(L, D_R) and refine(R, D_L) }
 * until a whole pass splits nothing.  (r) stays alive until the end of the step (the caller frees it). */
static void reconcile(X *c, stepctx *sc, dlist *DL, dlist *DR) {
    for (;;) {
        AC(c, rc_passes, 1);
        d_make(sc->L, DL);
        d_make(sc->R, DR);
        uint64_t made = refine(c, sc->L, DR, sc);
        made += refine(c, sc->R, DL, sc);
        if (!made) break;
    }
#ifdef SK_COUNT
    c->ac.rc_DL = DL->n;
    c->ac.rc_DR = DR->n;
#endif
#ifdef SK_AUDIT
    /* every pair a merge of this step can compare is equal or coprime (§16.9 item 5) */
    for (int side = 0; side < 2; side++) {
        const X *x = side ? sc->R : sc->L;
        const dlist *D = side ? DL : DR;
        for (uint32_t j = 0; j < x->nq; j++)
            for (uint32_t i = 0; i < D->n; i++) {
                sk_int q = piece_get(x, j);
                if (q != D->v[i] && sk_gcd_raw(q, D->v[i]) != 1)
                    sk_die("mh3: after reconciliation, a pair neither equal nor coprime");
            }
    }
#endif
}
#endif

/* ----------------------------------------------------------------------------------------- the matrix operations */

#ifdef SK_AUDIT
/* a temporary's list: strictly increasing, and multiplies back to its denominator (§16.9 item 5) */
static void tcheck(const flist *l, sk_int d) {
    sk_int v = 1;
    for (uint32_t z = 0; z < l->n; z++) {
        if (z && l->q[z - 1] >= l->q[z]) sk_die("mh3: a temporary's list out of order");
        if (l->e[z] < 1 || l->q[z] < 2) sk_die("mh3: a temporary's list with a factor below 2 or an exponent below 1");
        for (unsigned u = 0; u < l->e[z]; u++)
            if (sk_mul_raw(v, l->q[z], &v)) sk_die("mh3: a temporary's list past 64 bits");
    }
    if (v != d) sk_die("mh3: a temporary's list does not multiply back to its denominator");
}
#define TCHECK(l, d) tcheck((l), (d))
#else
#define TCHECK(l, d) ((void)0)
#endif

/* a matrix whose build is finished: every field written, packed and factored */
static int finished(const X *x) { return x->packed && x->factored && x->nf == 2 * x->R * x->K; }

/* the operands a and b finished, of c's dimensions; c an empty result (v0.2 §15.5 item 1: any stored matrices) */
static sk_status ready(const X *a, const X *b, const X *c) {
    if (M2_INPUT(c->id) || c->nf != 0 || a == c || b == c) return SK_EINVAL;
    if (a->R != c->R || a->K != c->K || b->R != c->R || b->K != c->K) return SK_EINVAL;
    if (!finished(a) || !finished(b)) return SK_EINVAL;
    return SK_OK;
}

/* the end of a step: (r) freed and the operands' (q) shrunk (Pack-coprime); then c packed and factored */
static sk_status step_end(X *a, X *b, X *c, sk_status st, void *dl, void *dr) {
#if MH3_COPRIME
    d_free((dlist *)dl);
    d_free((dlist *)dr);
    piece_shrink(a);
    piece_shrink(b);
#else
    (void)a, (void)b, (void)dl, (void)dr;
#endif
    if (st) return st;
    pack(c);
    factor(c);
    return SK_OK;
}

/* c = a + b, elementwise, in row order (v0.1 §6.1): one add per cell, its result stored in c at once.  Pack-coprime:
 * a and b reconciled first.  c packed and factored at the end of the call (v0.3 §16.4 item 5). */
sk_status MH3F(madd)(X *a, X *b, X *c) {
    sk_status st = ready(a, b, c);
    if (st) return st;
#if MH3_COPRIME
    stepctx sc = {a, b, NULL, NULL};
    dlist DL = {NULL, 0, 0}, DR = {NULL, 0, 0};
    reconcile(c, &sc, &DL, &DR);
    void *dl = &DL, *dr = &DR;
#else
    void *dl = NULL, *dr = NULL;
#endif
    uint32_t n = c->R * c->K;
    M1_SNAP(s0);
    for (uint32_t i = 0; i < n && !st; i++) {
        c->ctx.x = i; /* the operands' cells, for the overflow report */
        c->ctx.y = i;
        dest out = {c, i, NULL};
        M1_OP_BEGIN(c, SK_PH_ADD, s0);
        st = add_cells(c, a, i, b, i, out);
        if (st) break;
        M1_OP_END(c, SK_PH_ADD, s0);
    }
    return step_end(a, b, c, st, dl, dr);
}

/* c = a b (v0.1 §6.2): for each cell of c in row order, s = 0/1 with an empty list; for k: p = a[i][k] b[k][j] with
 * its list, s = s + p with its list, on local pairs; then s stored in c (numerator's field first).  Square matrices.
 * Pack-coprime: a and b reconciled first.  c packed and factored at the end. */
sk_status MH3F(mmul)(X *a, X *b, X *c) {
    sk_status st = ready(a, b, c);
    if (st) return st;
    uint32_t n = c->R;
    if (c->K != n) return SK_EINVAL;
#if MH3_COPRIME
    stepctx sc = {a, b, NULL, NULL};
    dlist DL = {NULL, 0, 0}, DR = {NULL, 0, 0};
    reconcile(c, &sc, &DL, &DR);
    void *dl = &DL, *dr = &DR, *scp = &sc;
#else
    void *dl = NULL, *dr = NULL, *scp = NULL;
#endif
    M1_SNAP(s0);
    for (uint32_t i = 0; i < n && !st; i++)
        for (uint32_t j = 0; j < n && !st; j++) {
            tpair s = {0, 1}, p;
            flist ls, lp; /* (t): the temporaries' denominator lists */
            ls.n = 0;
#if MH3_COPRIME
            sc.s = &ls;
            sc.p = &lp;
#endif
            for (uint32_t k = 0; k < n; k++) {
                c->ctx.x = i * n + k;
                c->ctx.y = k * n + j;
                dest tp = {NULL, 0, &p}, ts = {NULL, 0, &s};
                M1_OP_BEGIN(c, SK_PH_MUL, s0);
                st = mul_cells(c, a, i * n + k, b, k * n + j, tp, &lp);
                if (st) break;
                TCHECK(&lp, p.d);
                M1_OP_END(c, SK_PH_MUL, s0);
                c->ctx.x = c->ctx.y = SK_NONE; /* two local pairs */
                M1_OP_BEGIN(c, SK_PH_ADD, s0);
                st = add_values(c, s, p, &ls, &lp, ts, scp);
                if (st) break;
                TCHECK(&ls, s.d);
                M1_OP_END(c, SK_PH_ADD, s0);
            }
            if (st) break;
            store_cell(c, i * n + j, s.m < 0, s.m < 0 ? -s.m : s.m, s.d); /* store s as c[i][j] */
        }
#if MH3_COPRIME
    sc.s = sc.p = NULL;
#endif
    (void)scp;
    return step_end(a, b, c, st, dl, dr);
}

/* ----------------------------------------------------------------------------------------- read, reports */

sk_status MH3F(read)(const X *x, uint32_t cell, sk_int *m, sk_int *d) {
    if (2 * cell + 1 >= x->nf) return SK_EINVAL;
    uint32_t f = fld_get(x->cells, x->wh, 2 * cell), g = fld_get(x->cells, x->wh, 2 * cell + 1);
    sk_int v = slot_get(vals(x), x->w, f >> 1);
    *m = (f & 1) ? -v : v;
    *d = slot_get(vals(x), x->w, g >> 1);
    return SK_OK;
}

void MH3F(figures)(const X *x, m2_fig *f) {
    f->R = x->R;
    f->K = x->K;
    f->T = x->T;
    f->W = x->W;
    f->nf = x->nf;
    f->w = x->w;
    f->wh = x->wh;
    f->wh_build = x->packed ? x->whb : x->wh;
    f->packed = x->packed;
    f->bytes_cells = bytes_cells(x);
    f->bytes_vals = bytes_vals(x);
}

void MH3F(ffigures)(const X *x, m3_ffig *f) {
    memset(f, 0, sizeof *f);
    f->W = x->W;
    f->E = x->factored ? x->E : 0;
    f->w = x->w;
    f->factored = x->factored;
#if MH3_COPRIME
    f->nq = x->nq;
#endif
    f->bytes_f = bytes_f(x);
    f->bytes_q = bytes_q(x);
}

/* the cells, (h) (every slot counted) or the list, (f), and (q) */
void MH3F(memory)(const X *x, sk_memory *r) {
    m2_fig f;
    MH3F(figures)(x, &f);
    memset(r, 0, sizeof *r);
    sk_memory_add(r, "cells", f.bytes_cells, f.bytes_cells);
    sk_memory_add(r, f.packed ? "list" : "table", f.bytes_vals, f.bytes_vals);
    if (x->factored) { /* (f): its three arrays */
        uint64_t o = 4ull * (x->W + 1), q = (uint64_t)x->Ecap * (x->w / 8), e = x->Ecap;
        sk_memory_add(r, "f-offsets", o, o);
        sk_memory_add(r, "f-factors", q, q);
        sk_memory_add(r, "f-exponents", e, e);
    }
#if MH3_COPRIME
    sk_memory_add(r, "base", bytes_q(x), bytes_q(x));
#endif
    sk_memory_finish(r, (uint64_t)x->R * x->K);
}

sk_status MH3F(counters)(const X *x, sk_counters *r, r2_xcounters *e) {
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

sk_status MH3F(storage)(const X *x, m2_scount *s, m2_ocount *o) {
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

sk_status MH3F(fcounters)(const X *x, m3_fcount *f, m3_acount *a) {
#ifdef SK_COUNT
    if (f) *f = x->fc;
    if (a) *a = x->ac;
    return SK_OK;
#else
    (void)x;
    if (f) memset(f, 0, sizeof *f);
    if (a) memset(a, 0, sizeof *a);
    return SK_ENOTBUILT;
#endif
}

sk_status MH3F(overflow)(const X *x, sk_overflow *r) {
    *r = x->ovf;
    return x->ovf.count ? SK_OK : SK_EINVAL;
}

/* ----------------------------------------------------------------------------------------- audits (v0.3 §16.9) */

#ifdef SK_AUDIT
/* a table's slot s: its value v is a whole number that fits the width and is found at its own slot by the store's
 * probe from h(v) (so no value is stored twice).  No slot is reserved (v0.2 §15.2).  As mh2.c's. */
static const char *check_slot(const X *x, uint32_t s) {
    sk_int v = slot_get(x->tab, x->w, s);
    if (v == EMPTY) return "slot: referenced by a cell, but empty";
    if (v < 0) return "slot: a negative value that is not the empty mark";
    if ((unsigned)sk_bits_u_raw((uint64_t)v) + 1 > x->w) return "slot: a value that does not fit the width";
    uint32_t n;
    int found;
    uint32_t at = probe(x->tab, x->T, x->t, x->w, v, &n, &found);
    if (found != 1) return "slot: its value is not found by its probe sequence";
    return at == s ? NULL : "slot: its value is found at another slot first (a duplicate)";
}

static int cmp_int(const void *a, const void *b) {
    sk_int x = *(const sk_int *)a, y = *(const sk_int *)b;
    return (x > y) - (x < y);
}

/* (f): every factorization multiplies back to its number (with overflow checks); entries strictly increasing by
 * factor, each at least 2, exponents at least 1; 0 and 1 empty; Pack-coprime: every factor a piece of (q), and (q)
 * pairwise coprime (by all pairs, up to 1,024 pieces: larger bases are compared with the model's by the gates) */
static const char *audit_f(const X *x) {
    if (!x->factored) return x->packed ? "factorizations: a packed matrix not factored" : NULL;
    if (x->foff[0] != 0 || x->foff[x->W] != x->E) return "factorizations: offsets do not span the entries";
    for (uint32_t k = 0; k < x->W; k++) {
        sk_int v = slot_get(x->list, x->w, k), prod = 1;
        uint32_t a = x->foff[k], b = x->foff[k + 1];
        if (b < a) return "factorizations: offsets decrease";
        if (v < 2 && b != a) return "factorizations: 0 or 1 with a nonempty list";
        if (b - a > FMAX) return "factorizations: more than 15 entries";
        for (uint32_t j = a; j < b; j++) {
            sk_int q = slot_get(x->ffac, x->w, j);
            if (q < 2) return "factorizations: a factor below 2";
            if (x->fexp[j] < 1) return "factorizations: an exponent below 1";
            if (j > a && slot_get(x->ffac, x->w, j - 1) >= q) return "factorizations: entries not strictly increasing";
            for (unsigned z = 0; z < x->fexp[j]; z++)
                if (sk_mul_raw(prod, q, &prod)) return "factorizations: a product past 64 bits";
#if MH3_COPRIME
            uint32_t i = 0;
            while (i < x->nq && piece_get(x, i) != q) i++;
            if (i == x->nq) return "factorizations: a factor that is not a piece of the base";
#endif
        }
        if (v >= 2 && prod != v) return "factorizations: a list does not multiply back to its number";
    }
#if MH3_COPRIME
    for (uint32_t i = 0; i < x->nq; i++)
        if (piece_get(x, i) < 2) return "base: a piece below 2";
    if (x->nq <= 1024)
        for (uint32_t i = 0; i < x->nq; i++)
            for (uint32_t j = i + 1; j < x->nq; j++)
                if (sk_gcd_raw(piece_get(x, i), piece_get(x, j)) != 1) return "base: two pieces not coprime";
#endif
    return NULL;
}

static sk_status audit_run(X *x, const char **why) {
    const char *fail = NULL;
    uint32_t at = 0;
    int packed = x->packed;
    if (packed && x->tab) fail = "pack: the build table alive after the packing";
    if (!packed && x->nf == 2 * x->R * x->K) fail = "pack: a finished matrix not packed (its build table alive)";
    uint32_t nv = packed ? x->W : x->T; /* the positions a handle may name */
    if (!fail && x->nf % 2) fail = "cells: a cell's denominator field not written";
    if (!fail && (x->T & (x->T - 1))) fail = "table: its size is not a power of two";
    if (!fail && !packed && x->T > (1u << (x->wh - 1))) fail = "table: more slots than the handle field can name";
    if (!fail && packed && x->W > (1u << (x->wh - 1))) fail = "list: more entries than the handle field can name";
    unsigned char *used = sk_xcalloc(nv + 1, 1); /* audit copy: the slots (positions) the cells name */
    for (at = 0; at + 1 < x->nf && !fail; at += 2) {
        uint32_t f = fld_get(x->cells, x->wh, at), g = fld_get(x->cells, x->wh, at + 1);
        uint32_t i = f >> 1, j = g >> 1;
        if (g & 1) fail = "cell: the denominator's tag is not 0";
        else if (i >= nv || j >= nv) fail = packed ? "cell: a handle of a packed matrix >= W" : "cell: a handle past T";
        else if (slot_get(vals(x), x->w, i) == EMPTY || slot_get(vals(x), x->w, j) == EMPTY)
            fail = "cell: a field names a slot that is not used";
        else {
            sk_int a = slot_get(vals(x), x->w, i), b = slot_get(vals(x), x->w, j);
            used[i] = used[j] = 1;
            if (b < 1) fail = "cell: denominator below 1";
            else if (sk_gcd_raw(a, b) != 1) fail = "cell: not in lowest terms";
            else if (a == 0 && (b != 1 || (f & 1))) fail = "cell: zero not stored as +0/1";
            else if (!packed && !(fail = check_slot(x, i))) fail = check_slot(x, j);
        }
    }
    uint32_t held = 0;
    if (!packed) {
        for (uint32_t s = 0; s < x->T && !fail; s++) {
            sk_int v = slot_get(x->tab, x->w, s);
            if (v == EMPTY) continue;
            held++;
            if (!used[s]) fail = "whole number: used by no cell";
            else fail = check_slot(x, s);
        }
    } else if (!fail) { /* the list: exactly W entries, each used by a cell, distinct, each fitting the width */
        sk_int *v = sk_xmalloc(sizeof(sk_int) * (x->W + 1)); /* audit copy */
        for (uint32_t k = 0; k < x->W; k++) {
            v[k] = slot_get(vals(x), x->w, k);
            if (!used[k] && !fail) fail = "list: an entry used by no cell";
            if ((v[k] < 0 || (unsigned)sk_bits_u_raw((uint64_t)v[k]) + 1 > x->w) && !fail)
                fail = "list: an entry that is not a whole number of its width";
        }
        qsort(v, x->W, sizeof(sk_int), cmp_int);
        for (uint32_t k = 1; k < x->W && !fail; k++)
            if (v[k] == v[k - 1]) fail = "list: a whole number held twice";
        free(v);
        held = x->W;
    }
    free(used);
    if (!fail && held != x->W) fail = "table: the used slots do not number W";
    if (!fail) fail = audit_f(x);
    if (fail) {
        snprintf(x->msg, sizeof x->msg, "%s (matrix %d): %s (field %u)", MH3_NAME, x->id, fail, at);
        if (why) *why = x->msg;
        return SK_EAUDIT;
    }
    if (why) *why = NULL;
    return SK_OK;
}
#endif

sk_status MH3F(audit)(X *x, const char **why) {
#ifdef SK_AUDIT
    return audit_run(x, why);
#else
    (void)x;
    if (why) *why = "audits not built";
    return SK_ENOTBUILT;
#endif
}

/* ----------------------------------------------------------------------------------------- test build only */

/* slot s of (h) (until the packing frees it), or position s of the list once packed */
sk_status MH3F(t_whole)(const X *x, uint32_t slot, sk_int *v) {
#ifdef SK_AUDIT
    if (slot >= (x->packed ? x->W : x->T)) return SK_EINVAL;
    *v = slot_get(x->packed ? vals(x) : x->tab, x->w, slot);
    return SK_OK;
#else
    (void)x, (void)slot, (void)v;
    return SK_ENOTBUILT;
#endif
}

sk_status MH3F(t_field)(const X *x, uint32_t f, uint32_t *v) {
#ifdef SK_AUDIT
    if (f >= x->nf) return SK_EINVAL;
    *v = fld_get(x->cells, x->wh, f);
    return SK_OK;
#else
    (void)x, (void)f, (void)v;
    return SK_ENOTBUILT;
#endif
}

sk_status MH3F(t_nfac)(const X *x, uint32_t k, uint32_t *n) {
#ifdef SK_AUDIT
    if (!x->factored || k >= x->W) return SK_EINVAL;
    *n = x->foff[k + 1] - x->foff[k];
    return SK_OK;
#else
    (void)x, (void)k, (void)n;
    return SK_ENOTBUILT;
#endif
}

sk_status MH3F(t_fac)(const X *x, uint32_t k, uint32_t j, sk_int *q, unsigned *e) {
#ifdef SK_AUDIT
    if (!x->factored || k >= x->W || x->foff[k] + j >= x->foff[k + 1]) return SK_EINVAL;
    *q = slot_get(x->ffac, x->w, x->foff[k] + j);
    *e = x->fexp[x->foff[k] + j];
    return SK_OK;
#else
    (void)x, (void)k, (void)j, (void)q, (void)e;
    return SK_ENOTBUILT;
#endif
}

sk_status MH3F(t_piece)(const X *x, uint32_t i, sk_int *q) {
#if defined(SK_AUDIT) && MH3_COPRIME
    if (i >= x->nq) return SK_EINVAL;
    *q = piece_get(x, i);
    return SK_OK;
#else
    (void)x, (void)i, (void)q;
    return SK_ENOTBUILT;
#endif
}

/* (q) pairwise coprime, by all pairs (the unit tests; v0.3 §16.9 item 5) */
sk_status MH3F(t_coprime)(const X *x, const char **why) {
#if defined(SK_AUDIT) && MH3_COPRIME
    for (uint32_t i = 0; i < x->nq; i++)
        for (uint32_t j = i + 1; j < x->nq; j++)
            if (sk_gcd_raw(piece_get(x, i), piece_get(x, j)) != 1) {
                if (why) *why = "two pieces not coprime";
                return SK_EAUDIT;
            }
    if (why) *why = NULL;
    return SK_OK;
#else
    (void)x;
    if (why) *why = "not built";
    return SK_ENOTBUILT;
#endif
}

sk_status MH3F(t_force_general)(X *x, int on) {
#ifdef SK_AUDIT
    x->force_general = on;
    return SK_OK;
#else
    (void)x, (void)on;
    return SK_ENOTBUILT;
#endif
}

sk_status MH3F(t_known_off)(X *x, unsigned mask) {
#ifdef SK_AUDIT
    x->koff = mask & R2_KOFF_ALL;
    return SK_OK;
#else
    (void)x, (void)mask;
    return SK_ENOTBUILT;
#endif
}

sk_status MH3F(t_corrupt_field)(X *x, uint32_t f, uint32_t v) {
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

sk_status MH3F(t_corrupt_slot)(X *x, uint32_t slot, sk_int v) {
#ifdef SK_AUDIT
    if (slot >= (x->packed ? x->W : x->T)) return SK_EINVAL;
    void *t = x->packed ? (void *)vals(x) : x->tab;
    switch (x->w) {
    case 8: ((int8_t *)t)[slot] = (int8_t)v; break;
    case 16: ((int16_t *)t)[slot] = (int16_t)v; break;
    case 32: ((int32_t *)t)[slot] = (int32_t)v; break;
    default: ((int64_t *)t)[slot] = v; break;
    }
    return SK_OK;
#else
    (void)x, (void)slot, (void)v;
    return SK_ENOTBUILT;
#endif
}

sk_status MH3F(t_corrupt_fac)(X *x, uint32_t k, uint32_t j, sk_int q, unsigned e) {
#ifdef SK_AUDIT
    if (!x->factored || k >= x->W || x->foff[k] + j >= x->foff[k + 1]) return SK_EINVAL;
    switch (x->w) {
    case 8: ((int8_t *)x->ffac)[x->foff[k] + j] = (int8_t)q; break;
    case 16: ((int16_t *)x->ffac)[x->foff[k] + j] = (int16_t)q; break;
    case 32: ((int32_t *)x->ffac)[x->foff[k] + j] = (int32_t)q; break;
    default: ((int64_t *)x->ffac)[x->foff[k] + j] = q; break;
    }
    x->fexp[x->foff[k] + j] = (uint8_t)e;
    return SK_OK;
#else
    (void)x, (void)k, (void)j, (void)q, (void)e;
    return SK_ENOTBUILT;
#endif
}
