/* l1.c: Design 3, layer 1 (spec 09-interning v0.1).  One source, compiled once per row:
 *   no setting:             L1, the baseline (spec §§4-15), symbols l1_*;
 *   -DL1_OPT:               L1-opt (spec §17), symbols l1o_*: L1 with the tricks O-1 to O-7 added, and nothing else;
 *   -DL1_Q32:               L1-Q32 (spec §18), symbols l1q_*: L1 with 16-bit index fields and lookup slots;
 *   -DL1_OPT -DL1_Q32:      L1-opt-Q32 (spec §18), symbols l1oq_*: L1-opt with 16-bit index fields;
 *   -DL1_LKGATE:            the lookup gate's build (spec §18.1 item 4; test only), symbols l1g_*: L1 whose lookup
 *                           is the slot-width-parameterized lookup of the Q32 rows, built at 32-bit slots.
 * With L1_OPT off, every line between "#ifdef L1_OPT" and its "#endif" is gone, and the helpers below reduce to the
 * 08 routines they wrap (spec §17 item 2).  With L1_Q32 and L1_LKGATE off, the source compiles to the L1 and L1-opt
 * measured at commit b3b88b6 (the "Q64" rows): every Q32 line is inside "#ifdef L1_Q32" or the PLK lookup.
 *
 * Three structures (spec §5): the whole numbers W (one 64-bit word each, distinct), the lookup (the 08 lookup in L1;
 * in L1-opt the same hash, probing and growth with fingerprinted slots, O-6; in L1-Q32 the same hash, probing and
 * growth with 16-bit slots, in its own translation unit src/plk.c as the 08 lookup is in common.c), and the
 * rationals (two 32-bit fields, or two 16-bit fields in the Q32 rows: tag bit and numerator index, denominator
 * index).  Every full-width operation goes through the 08 module (sk_internal.h), unchanged. */
#include "l1.h"

#include "sk_internal.h"

#if defined(L1_LKGATE) && (defined(L1_OPT) || defined(L1_Q32))
#error "the lookup gate's build is L1 at 32-bit slots: L1_LKGATE goes without L1_OPT and L1_Q32"
#endif

#if defined(L1_OPT) && defined(L1_Q32)
#define P(name) l1oq_##name
#define L1S l1oq_class
#define ROWNAME "l1oq"
#define IS_OPT 1
#elif defined(L1_OPT)
#define P(name) l1o_##name
#define L1S l1o_class
#define ROWNAME "l1o"
#define IS_OPT 1
#elif defined(L1_Q32)
#define P(name) l1q_##name
#define L1S l1q_class
#define ROWNAME "l1q"
#define IS_OPT 0
#elif defined(L1_LKGATE)
#define P(name) l1g_##name
#define L1S l1g_class
#define ROWNAME "l1g"
#define IS_OPT 0
#else
#define P(name) l1_##name
#define L1S l1_class
#define ROWNAME "l1"
#define IS_OPT 0
#endif

/* The parameterized plain lookup (PLK): L1-Q32's lookup, and the lookup gate's.  L1 and L1-opt (Q64) do not use it. */
#if !defined(L1_OPT) && (defined(L1_Q32) || defined(L1_LKGATE))
#define PLK 1
#endif

typedef struct L1S L1T;

#define NOIDX SK_NONE /* "no index" in a 32-bit variable: never stored in a field or a slot */
#ifdef L1_Q32
/* spec §18.1: a 16-bit numerator field (the tag in its top bit, the index in its low 15 bits) and a 16-bit denominator
 * field; capacity 32,768 whole numbers, at indices 0 to 32,767.  The no-index value is NOIDX (UINT32_MAX), which is
 * never written to a field; a slot's empty mark is 0xFFFF, above every index. */
typedef uint16_t l1_field;
#define TAG 0x8000u
#define IDX 0x7FFFu
#define L1_W_LIMIT 32768u /* the count of whole numbers may not exceed it (spec §18.1 item 2) */
#else
typedef uint32_t l1_field;
#define TAG 0x80000000u
#define IDX 0x7FFFFFFFu
/* spec §15: fewer than 2^31 - 1 whole numbers (the value 2^31 - 1 is kept free) */
#define L1_W_LIMIT 0x7FFFFFFEu
#endif

typedef struct rat {
    l1_field num; /* tag in the top bit (1 for -), numerator index in the low 31 bits (15 in the Q32 rows) */
    l1_field den; /* denominator index */
} rat;

#define L1_PHI UINT64_C(0x9E3779B97F4A7C15)

#ifdef L1_OPT
/* O-6: a slot holds the index and the low 32 bits of the hash product v * phi mod 2^64.  L1-opt-Q32 keeps the 32-bit
 * fingerprint and narrows only the index, so its slot is 6 bytes, packed (spec §18.1 item 5). */
#ifdef L1_Q32
typedef struct __attribute__((packed)) oslot {
    uint16_t idx;
    uint32_t fp;
} oslot;
#define SLOT_EMPTY 0xFFFFu
#else
typedef struct oslot {
    uint32_t idx, fp;
} oslot;
#define SLOT_EMPTY NOIDX
#endif
typedef struct olookup {
    oslot *slot;
    uint32_t S;
    unsigned log2S;
#ifdef SK_AUDIT
    uint32_t filled;
#endif
} olookup;
#endif

#ifdef PLK
/* The 08 lookup (08 common.c) with its slot width as a parameter: 16-bit slots in L1-Q32, 32-bit in the gate's build.
 * It is its own translation unit, src/plk.c, called across it as L1-Q64 calls sk_lookup in common.c (plk.h; D-25). */
#include "plk.h"
#endif

struct L1S {
    sk_int *W; /* the whole numbers, each in [0, 2^63 - 1] */
    uint32_t nW, capW;
#ifdef L1_OPT
    olookup lk;
    uint32_t idx0, idx1; /* O-2: the indices of 0 and 1 once interned, else NOIDX */
    int reserved;        /* O-7: the rational list was reserved */
    uint32_t shadow_cap; /* O-7: the capacity L1's doubling would have had (for the counters) */
#elif defined(PLK)
    plookup lk;
#else
    sk_lookup lk;
#endif
    rat *R; /* the rationals */
    uint32_t N, capR;
    sk_opctx ctx;
    sk_overflow ovf;
#ifdef SK_COUNT
    sk_counters cnt;
    l1_xcounters xcnt;
    l1_xcount *xcur;
#endif
#ifdef SK_AUDIT
    uint32_t mark_N, mark_W; /* the change list: rationals and whole numbers since the last audit */
    int force_general;       /* §14.3: send every operation to its general case */
#ifdef L1_OPT
    unsigned tricks; /* §17.3 item 4: the tricks that are on */
#endif
    char msg[200];
#endif
};

/* ----------------------------------------------------------------------------------------- counting */

#ifdef SK_COUNT
static l1_xcount x_sink;
#define XCNT(c, field, n) ((c)->xcur->field += (uint64_t)(n))
#define SETCELL(c, phase, part)                                                                                   \
    do {                                                                                                          \
        CNT_SET((c)->cnt, (phase), (part));                                                                       \
        (c)->xcur = &(c)->xcnt.c[(phase)][(part)];                                                                \
    } while (0)
#else
#define XCNT(c, field, n) ((void)0)
#define SETCELL(c, phase, part) ((void)0)
#endif
#define XCASE(c, k) XCNT(c, cases[(k)], 1)

#ifdef L1_OPT
#ifdef SK_AUDIT
#define ON(c, t) (((c)->tricks & (t)) != 0)
#else
#define ON(c, t) 1
#endif
#endif

/* ----------------------------------------------------------------------------------------- known ones (O-4) */

/* A value, and whether |value| = 1 is known without a comparison of values (spec §17.1, O-4): a whole number at the
 * index of 1, a gcd that came out as 1, or a quotient or product that inherits it.  In L1 `one` is always false and
 * the helpers are exactly the 08 routines. */
typedef struct kv {
    sk_int v;
    bool one;
} kv;

static inline kv KV(sk_int v) { return (kv){v, false}; }

static inline kv whole(const L1T *c, uint32_t i) {
#ifdef L1_OPT
    return (kv){c->W[i], c->idx1 != NOIDX && i == c->idx1};
#else
    return (kv){c->W[i], false};
#endif
}

static inline kv sgn(kv a, bool neg) { return (kv){neg ? -a.v : a.v, a.one}; } /* a tag: not counted */

/* gcd(p, q); pi and qi are the whole-number indices of the arguments when they are an operand's whole numbers. */
static inline kv gcdk(L1T *c, sk_int p, uint32_t pi, sk_int q, uint32_t qi) {
#ifdef L1_OPT
    if (ON(c, L1O_T3) && c->idx1 != NOIDX && (pi == c->idx1 || qi == c->idx1)) { /* O-3 */
        XCNT(c, o3_gcd_skipped, 1);
#ifdef SK_COUNT
        sk_uint mp = sk_mag(p), mq = sk_mag(q);
        XCNT(c, o3_gcd_work, sk_bits_u_raw((uint64_t)(mp > mq ? mp : mq)));
#endif
        return (kv){1, true};
    }
    sk_int g = sk_gcd(p, q);
    return (kv){g, g == 1};
#else
    (void)c;
    (void)pi;
    (void)qi;
    return KV(sk_gcd(p, q));
#endif
}

/* the exact division x / h */
static inline kv dvx(L1T *c, kv x, kv h) {
    (void)c;
#ifdef L1_OPT
    if (ON(c, L1O_T4) && h.one) { /* O-4 */
        XCNT(c, o4_div_skipped, 1);
        return x;
    }
#endif
    return KV(sk_divexact(x.v, h.v));
}

/* the multiplication p * q; true on overflow */
static inline bool mulx(L1T *c, kv p, kv q, kv *r) {
    (void)c;
#ifdef L1_OPT
    if (ON(c, L1O_T4) && (p.one || q.one)) { /* O-4: |p| = 1 or |q| = 1; only a sign is applied */
        XCNT(c, o4_mul_skipped, 1);
        if (p.one)
            *r = (kv){p.v < 0 ? -q.v : q.v, q.one};
        else
            *r = (kv){q.v < 0 ? -p.v : p.v, p.one};
        return false;
    }
#endif
    r->one = false;
    return sk_mul(p.v, q.v, &r->v);
}

/* ----------------------------------------------------------------------------------------- the lookup */

/* Q32, test build only (spec §18.1 item 3): an index written to a 16-bit field or slot reads back equal to the index,
 * so nothing was truncated.  Capacity itself is checked in every build, once per new whole number, by intern. */
#if defined(L1_Q32) && defined(SK_AUDIT)
#define Q32_READBACK(stored, k)                                                                                   \
    do {                                                                                                          \
        if ((uint32_t)(stored) != (uint32_t)(k)) sk_die("Q32: an index did not fit its 16-bit field or slot");  \
    } while (0)
#else
#define Q32_READBACK(stored, k) ((void)0)
#endif

#ifdef L1_OPT
/* The 08 lookup's hash, probing order and growth rule (08 common.c), with O-6's slots. */
static inline uint64_t hprod(sk_int v) { return (uint64_t)v * L1_PHI; }

static void olk_init(olookup *t) {
    t->S = 16;
    t->log2S = 4;
    t->slot = sk_xmalloc(sizeof(oslot) * t->S);
    for (uint32_t s = 0; s < t->S; s++) t->slot[s] = (oslot){SLOT_EMPTY, 0};
#ifdef SK_AUDIT
    t->filled = 0;
#endif
}

static uint32_t olk_find(const L1T *c, sk_int v, uint32_t *empty) {
    const olookup *t = &c->lk;
    CNT(lookup_search, 1);
    uint64_t hp = hprod(v);
    uint32_t fp = (uint32_t)hp, mask = t->S - 1, s = (uint32_t)(hp >> (64 - t->log2S));
    for (;;) {
        CNT(lookup_probe, 1);
        oslot sl = t->slot[s];
        if (sl.idx == SLOT_EMPTY) {
            if (empty) *empty = s;
            return NOIDX;
        }
        if (ON(c, L1O_T6) && sl.fp != fp) {
            XCNT(c, o6_reads_avoided, 1);
        } else if (c->W[sl.idx] == v) {
            return sl.idx;
        }
        s = (s + 1) & mask;
    }
}

static void olk_place(olookup *t, const sk_int *W, uint32_t k) {
    CNT(lookup_search, 1);
    uint64_t hp = hprod(W[k]);
    uint32_t mask = t->S - 1, s = (uint32_t)(hp >> (64 - t->log2S));
    for (;;) {
        CNT(lookup_probe, 1);
        if (t->slot[s].idx == SLOT_EMPTY) break;
        s = (s + 1) & mask;
    }
    t->slot[s] = (oslot){k, (uint32_t)hp};
    Q32_READBACK(t->slot[s].idx, k);
}

static void olk_insert(olookup *t, const sk_int *W, uint32_t k, uint32_t empty) {
    uint64_t count = (uint64_t)k + 1;
    if (2 * count > t->S) { /* double S and place every index again, in index order (08 spec §5 item 2) */
        if (t->log2S >= 32) sk_die("lookup too large");
        t->S *= 2;
        t->log2S++;
        t->slot = sk_xrealloc(t->slot, sizeof(oslot) * t->S);
        for (uint32_t s = 0; s < t->S; s++) t->slot[s] = (oslot){SLOT_EMPTY, 0};
        for (uint32_t i = 0; i <= k; i++) olk_place(t, W, i);
#ifdef SK_AUDIT
        t->filled = k + 1;
#endif
        return;
    }
    t->slot[empty] = (oslot){k, (uint32_t)hprod(W[k])};
    Q32_READBACK(t->slot[empty].idx, k);
#ifdef SK_AUDIT
    t->filled++;
#endif
}
#endif

static inline uint32_t lk_find(const L1T *c, sk_int v, uint32_t *empty) {
#ifdef L1_OPT
    return olk_find(c, v, empty);
#elif defined(PLK)
    return PLKN(find)(&c->lk, c->W, v, empty);
#else
    return sk_lookup_find(&c->lk, c->W, v, empty);
#endif
}

/* intern (spec §6): look v up; append it if it is new. */
static uint32_t intern(L1T *c, sk_int v) {
    uint32_t empty = 0, k = lk_find(c, v, &empty);
    if (k != NOIDX) {
        XCNT(c, intern_found, 1);
        return k;
    }
    if (c->nW >= L1_W_LIMIT) { /* one check per new whole number, in every build */
#ifdef L1_Q32
        fprintf(stderr, "layer 1 (Q32): capacity of 32,768 whole numbers exceeded (spec §18.1 item 2)\n");
        sk_die("capacity exceeded");
#else
        fprintf(stderr, "layer 1: limit of spec §15 exceeded: whole numbers\n");
        sk_die("limit exceeded");
#endif
    }
    SK_RESERVE(c->W, c->nW, c->capW, 16);
    k = c->nW++;
    c->W[k] = v;
#ifdef L1_OPT
    olk_insert(&c->lk, c->W, k, empty);
    if (v == 0) c->idx0 = k; /* O-2 */
    else if (v == 1) c->idx1 = k;
#elif defined(PLK)
    PLKN(insert)(&c->lk, c->W, k, empty);
#else
    sk_lookup_insert(&c->lk, c->W, k, empty);
#endif
    XCNT(c, intern_appended, 1);
    return k;
}

/* ----------------------------------------------------------------------------------------- store (spec §6, §17.1) */

/* store_known: n / e in lowest terms, e >= 1; i and j are the indices of |n| and e when known (O-1), else NOIDX.
 * L1 calls it with both unknown, which is the store of spec §6. */
static sk_status store_known(L1T *c, sk_int n, uint32_t i, sk_int e, uint32_t j, int o1, sk_handle *out) {
    SETCELL(c, c->ctx.op, SK_STORE);
    if (n == 0) {
        e = 1;
        XCNT(c, zero_results, 1);
    }
    uint32_t tag = n < 0 ? TAG : 0;
    sk_int mag = n < 0 ? -n : n;
    (void)o1;
#ifdef L1_OPT
#ifdef SK_AUDIT
    if ((i != NOIDX && (i >= c->nW || c->W[i] != mag)) || (j != NOIDX && (j >= c->nW || c->W[j] != e)))
        sk_die("O-1: a known index does not hold the number it stands for");
#endif
    if (i != NOIDX) {
        XCNT(c, o1_skipped[o1], 1);
    } else if (ON(c, L1O_T2) && mag == 0 && c->idx0 != NOIDX) {
        i = c->idx0;
        XCNT(c, o2_skipped0, 1);
    } else if (ON(c, L1O_T2) && mag == 1 && c->idx1 != NOIDX) {
        i = c->idx1;
        XCNT(c, o2_skipped1, 1);
    } else {
        i = intern(c, mag); /* the numerator first */
    }
    if (j != NOIDX) {
        XCNT(c, o1_skipped[o1], 1);
    } else if (ON(c, L1O_T2) && e == 1 && c->idx1 != NOIDX) {
        j = c->idx1;
        XCNT(c, o2_skipped1, 1);
    } else {
        j = intern(c, e); /* then the denominator */
    }
#else
    i = intern(c, mag); /* the numerator first */
    j = intern(c, e);   /* then the denominator */
#endif
    sk_check_limit(c->N, "stored rationals");
#ifdef L1_OPT
    if (c->reserved && c->N >= c->shadow_cap) { /* O-7: a growth L1 would have made here */
        XCNT(c, o7_growths_avoided, 1);
        XCNT(c, o7_bytes_avoided, (uint64_t)c->N * sizeof(rat));
        c->shadow_cap = c->shadow_cap ? 2 * c->shadow_cap : 16;
    }
#endif
    SK_RESERVE(c->R, c->N, c->capR, 16);
    c->R[c->N].num = tag | i;
    c->R[c->N].den = j;
    Q32_READBACK(c->R[c->N].num & IDX, i);
    Q32_READBACK(c->R[c->N].den, j);
    *out = c->N++;
    return SK_OK;
}

static inline sk_status store(L1T *c, sk_int n, sk_int e, sk_handle *out) {
    return store_known(c, n, NOIDX, e, NOIDX, 0, out);
}

/* ----------------------------------------------------------------------------------------- create, entry */


L1T *P(create)(void) {
    L1T *c = sk_xcalloc(1, sizeof *c);
#ifdef L1_OPT
    olk_init(&c->lk);
    c->idx0 = c->idx1 = NOIDX;
#ifdef SK_AUDIT
    c->tricks = L1O_ALL;
#endif
#elif defined(PLK)
    PLKN(init)(&c->lk);
#else
    sk_lookup_init(&c->lk);
#endif
#ifdef SK_COUNT
    c->xcur = &x_sink;
#endif
    return c;
}

void P(destroy)(L1T *c) {
    if (!c) return;
    free(c->W);
    free(c->lk.slot);
    free(c->R);
    free(c);
}

#ifdef L1_OPT
void P(reserve)(L1T *c, uint32_t n) {
    if (!ON(c, L1O_T7) || c->N != 0 || n <= c->capR) return;
    c->R = sk_xrealloc(c->R, (size_t)n * sizeof(rat));
    c->capR = n;
    c->reserved = 1;
}
#endif

#define OVF(step) return sk_overflow_at(&c->ovf, &c->ctx, (step))

sk_status P(enter)(L1T *c, sk_int m, sk_int d, sk_handle *out) {
    if (!sk_valid_input(m) || !sk_valid_input(d)) return SK_EINVAL;
    if (d == 0) return SK_EDIVZERO;
    c->ctx.op = SK_PH_ENTRY;
    c->ctx.x = c->ctx.y = SK_NONE;
    SETCELL(c, SK_PH_ENTRY, SK_CORE);
    if (d < 0) { /* move the sign of d to m (spec §4.2 item 4); negation is not counted */
        m = -m;
        d = -d;
    }
    kv g = gcdk(c, m, NOIDX, d, NOIDX);
    kv n = dvx(c, KV(m), g);
    kv e = dvx(c, KV(d), g);
    return store(c, n.v, e.v, out);
}

/* ----------------------------------------------------------------------------------------- operations (spec §8) */

typedef struct opnd {
    uint32_t i, j; /* numerator and denominator indices */
    bool neg;      /* the tag */
} opnd;

static inline opnd operand(const L1T *c, sk_handle h) {
    rat r = c->R[h];
    return (opnd){r.num & IDX, r.den, (r.num & TAG) != 0};
}

static sk_status begin(L1T *c, sk_phase op, sk_handle x, sk_handle y) {
    if (x >= c->N || y >= c->N) return SK_EINVAL;
    c->ctx.op = op;
    c->ctx.x = x;
    c->ctx.y = y;
    SETCELL(c, op, SK_CORE);
    return SK_OK;
}

/* The four index comparisons of spec §7, all made, in this order, before any case is chosen. */
typedef struct eq {
    bool den, num, ad, cb;
} eq;

static inline eq compare(L1T *c, opnd X, opnd Y) {
    (void)c;
    eq q;
    q.den = X.j == Y.j; /* same denominator, b = d */
    q.num = X.i == Y.i; /* same numerator, a = c */
    q.ad = X.i == Y.j;  /* cross, a = d */
    q.cb = Y.i == X.j;  /* cross, c = b */
    XCNT(c, idx_cmp, 4);
#ifdef SK_AUDIT
    if (c->force_general) q.den = q.num = q.ad = q.cb = false;
#endif
    return q;
}

/* add, and subtract as addition of the second operand with its tag flipped (spec §4.2 item 3, §8.1) */
static sk_status addsub(L1T *c, sk_phase op, sk_handle x, sk_handle y, sk_handle *out) {
    sk_status st = begin(c, op, x, y);
    if (st) return st;
    opnd X = operand(c, x), Y = operand(c, y);
    if (op == SK_PH_SUB) Y.neg = !Y.neg;
    eq q = compare(c, X, Y);
    kv a = whole(c, X.i), b = whole(c, X.j), cc = whole(c, Y.i), d = whole(c, Y.j);
    kv mx = sgn(a, X.neg), my = sgn(cc, Y.neg);
    kv p, r, t, e;
    if (q.ad || q.cb) { /* case cross: gcd(b, d) = 1 */
        XCASE(c, L1_ADD_CROSS);
        if (mulx(c, mx, d, &p)) OVF("add cross: m_x*d");
        if (mulx(c, my, b, &r)) OVF("add cross: m_y*b");
        if (sk_add(p.v, r.v, &t.v)) OVF("add cross: m_x*d + m_y*b");
        if (mulx(c, b, d, &e)) OVF("add cross: b*d");
        return store(c, t.v, e.v, out);
    }
    if (q.den) { /* case den: gcd(b, d) = d */
        XCASE(c, L1_ADD_DEN);
#ifdef L1_OPT
        if (ON(c, L1O_T5) && q.num && X.neg != Y.neg) { /* O-5: x + (-x) is 0/1 */
            XCNT(c, o5_ops, 1);
            XCNT(c, o5_add_skipped, 1);
            XCNT(c, o5_gcd_skipped, 1);
            XCNT(c, o5_gcd_work, sk_bits_u_raw((uint64_t)d.v)); /* gcd(0, d): the larger operand is d */
            XCNT(c, o5_div_skipped, 2);
            return store(c, 0, 1, out);
        }
#endif
        if (sk_add(mx.v, my.v, &t.v)) OVF("add den: t = m_x + m_y");
        kv h = gcdk(c, t.v, NOIDX, d.v, Y.j);
        kv n = dvx(c, KV(t.v), h);
        e = dvx(c, d, h);
#ifdef L1_OPT
        if (ON(c, L1O_T1) && h.v == 1) return store_known(c, n.v, NOIDX, e.v, X.j, L1_O1_ADD_DEN, out);
#endif
        return store(c, n.v, e.v, out);
    }
    if (q.num) { /* case num: gcd(a, b * d) = 1 */
        XCASE(c, L1_ADD_NUM);
        kv g = gcdk(c, b.v, X.j, d.v, Y.j);
        kv bp = dvx(c, b, g), dp = dvx(c, d, g);
        if (sk_add(X.neg ? -dp.v : dp.v, Y.neg ? -bp.v : bp.v, &t.v)) OVF("add num: t = s_x*d' + s_y*b'");
        kv h;
        if (g.v == 1) {
            XCNT(c, gen_g1, 1);
            h = (kv){1, IS_OPT}; /* the second gcd is skipped (spec L-9) */
        } else {
            h = gcdk(c, t.v, NOIDX, g.v, NOIDX);
        }
        kv n, tq = dvx(c, KV(t.v), h);
        if (mulx(c, a, tq, &n)) OVF("add num: a*(t/h)");
        kv dq = dvx(c, d, h);
        if (mulx(c, bp, dq, &e)) OVF("add num: b'*(d/h)");
        return store(c, n.v, e.v, out);
    }
    XCASE(c, L1_ADD_GEN); /* case general: the control's procedure without its equal-denominator test */
    kv g = gcdk(c, b.v, X.j, d.v, Y.j);
    if (g.v == 1) {
        XCNT(c, gen_g1, 1);
        if (mulx(c, mx, d, &p)) OVF("add general: m_x*d");
        if (mulx(c, my, b, &r)) OVF("add general: m_y*b");
        if (sk_add(p.v, r.v, &t.v)) OVF("add general: m_x*d + m_y*b");
        if (mulx(c, b, d, &e)) OVF("add general: b*d");
        return store(c, t.v, e.v, out);
    }
    kv bp = dvx(c, b, g), dp = dvx(c, d, g);
    if (mulx(c, mx, dp, &p)) OVF("add general: m_x*d'");
    if (mulx(c, my, bp, &r)) OVF("add general: m_y*b'");
    if (sk_add(p.v, r.v, &t.v)) OVF("add general: t = m_x*d' + m_y*b'");
    kv h = gcdk(c, t.v, NOIDX, g.v, NOIDX);
    kv n = dvx(c, KV(t.v), h);
    kv dq = dvx(c, d, h);
    if (mulx(c, bp, dq, &e)) OVF("add general: b'*(d/h)");
    return store(c, n.v, e.v, out);
}

sk_status P(add)(L1T *c, sk_handle x, sk_handle y, sk_handle *out) { return addsub(c, SK_PH_ADD, x, y, out); }
sk_status P(sub)(L1T *c, sk_handle x, sk_handle y, sk_handle *out) { return addsub(c, SK_PH_SUB, x, y, out); }

sk_status P(mul)(L1T *c, sk_handle x, sk_handle y, sk_handle *out) {
    sk_status st = begin(c, SK_PH_MUL, x, y);
    if (st) return st;
    opnd X = operand(c, x), Y = operand(c, y);
    eq q = compare(c, X, Y);
    kv a = whole(c, X.i), b = whole(c, X.j), cc = whole(c, Y.i), d = whole(c, Y.j);
    bool neg = X.neg != Y.neg; /* s = s_x * s_y */
    kv mx = sgn(a, X.neg), my = sgn(cc, Y.neg);
    kv n, e;
    if (q.ad && q.cb) { /* case both: a = d and c = b */
        XCASE(c, L1_MUL_BOTH);
        return store(c, neg ? -1 : 1, 1, out);
    }
    if (q.den) { /* case den: no gcd */
        XCASE(c, L1_MUL_DEN);
        if (mulx(c, mx, my, &n)) OVF("multiply den: m_x*m_y");
        if (mulx(c, d, d, &e)) OVF("multiply den: d*d");
        return store(c, n.v, e.v, out);
    }
    if (q.num) { /* case num: no gcd */
        XCASE(c, L1_MUL_NUM);
        if (mulx(c, a, a, &n)) OVF("multiply num: a*a");
        if (mulx(c, b, d, &e)) OVF("multiply num: b*d");
        return store(c, neg ? -n.v : n.v, e.v, out);
    }
    if (q.ad) { /* case cross_ad: a cancels */
        XCASE(c, L1_MUL_CROSS_AD);
        kv h = gcdk(c, cc.v, Y.i, b.v, X.j);
        n = dvx(c, cc, h);
        e = dvx(c, b, h);
#ifdef L1_OPT
        if (ON(c, L1O_T1) && h.v == 1)
            return store_known(c, neg ? -n.v : n.v, Y.i, e.v, X.j, L1_O1_MUL_CROSS_AD, out);
#endif
        return store(c, neg ? -n.v : n.v, e.v, out);
    }
    if (q.cb) { /* case cross_cb: b cancels */
        XCASE(c, L1_MUL_CROSS_CB);
        kv h = gcdk(c, a.v, X.i, d.v, Y.j);
        n = dvx(c, a, h);
        e = dvx(c, d, h);
#ifdef L1_OPT
        if (ON(c, L1O_T1) && h.v == 1)
            return store_known(c, neg ? -n.v : n.v, X.i, e.v, Y.j, L1_O1_MUL_CROSS_CB, out);
#endif
        return store(c, neg ? -n.v : n.v, e.v, out);
    }
    XCASE(c, L1_MUL_GEN); /* case general: the control's procedure */
    kv g1 = gcdk(c, mx.v, X.i, d.v, Y.j);
    kv g2 = gcdk(c, my.v, Y.i, b.v, X.j);
    kv q1 = dvx(c, mx, g1), q2 = dvx(c, my, g2);
    if (mulx(c, q1, q2, &n)) OVF("multiply general: (m_x/g_1)*(m_y/g_2)");
    kv r1 = dvx(c, b, g2), r2 = dvx(c, d, g1);
    if (mulx(c, r1, r2, &e)) OVF("multiply general: (b/g_2)*(d/g_1)");
    return store(c, n.v, e.v, out);
}

sk_status P(div)(L1T *c, sk_handle x, sk_handle y, sk_handle *out) {
    sk_status st = begin(c, SK_PH_DIV, x, y);
    if (st) return st;
    opnd X = operand(c, x), Y = operand(c, y);
    if (c->W[Y.i] == 0) return SK_EDIVZERO; /* by reading c, before the comparisons (spec L-12) */
    eq q = compare(c, X, Y);
    kv a = whole(c, X.i), b = whole(c, X.j), cc = whole(c, Y.i), d = whole(c, Y.j);
    bool neg = X.neg != Y.neg; /* s = s_x * s_y */
    kv n, e;
    if (q.num && q.den) { /* case both: a = c != 0 and b = d */
        XCASE(c, L1_DIV_BOTH);
        return store(c, neg ? -1 : 1, 1, out);
    }
    if (q.ad) { /* case cross_ad: a^2 / (b * c), no gcd */
        XCASE(c, L1_DIV_CROSS_AD);
        if (mulx(c, a, a, &n)) OVF("divide cross_ad: a*a");
        if (mulx(c, b, cc, &e)) OVF("divide cross_ad: b*c");
        return store(c, neg ? -n.v : n.v, e.v, out);
    }
    if (q.cb) { /* case cross_cb: a * d / b^2, no gcd */
        XCASE(c, L1_DIV_CROSS_CB);
        if (mulx(c, a, d, &n)) OVF("divide cross_cb: a*d");
        if (mulx(c, b, b, &e)) OVF("divide cross_cb: b*b");
        return store(c, neg ? -n.v : n.v, e.v, out);
    }
    if (q.num) { /* case num: a cancels, gcd(d, b) left */
        XCASE(c, L1_DIV_NUM);
        kv h = gcdk(c, d.v, Y.j, b.v, X.j);
        n = dvx(c, d, h);
        e = dvx(c, b, h);
#ifdef L1_OPT
        if (ON(c, L1O_T1) && h.v == 1) return store_known(c, neg ? -n.v : n.v, Y.j, e.v, X.j, L1_O1_DIV_NUM, out);
#endif
        return store(c, neg ? -n.v : n.v, e.v, out);
    }
    if (q.den) { /* case den: b cancels, gcd(a, c) left */
        XCASE(c, L1_DIV_DEN);
        kv h = gcdk(c, a.v, X.i, cc.v, Y.i);
        n = dvx(c, a, h);
        e = dvx(c, cc, h);
#ifdef L1_OPT
        if (ON(c, L1O_T1) && h.v == 1) return store_known(c, neg ? -n.v : n.v, X.i, e.v, Y.i, L1_O1_DIV_DEN, out);
#endif
        return store(c, neg ? -n.v : n.v, e.v, out);
    }
    XCASE(c, L1_DIV_GEN); /* case general: the control's divide on magnitudes, the sign in s */
    kv g1 = gcdk(c, a.v, X.i, cc.v, Y.i);
    kv g2 = gcdk(c, b.v, X.j, d.v, Y.j);
    kv q1 = dvx(c, a, g1), q2 = dvx(c, d, g2);
    if (mulx(c, q1, q2, &n)) OVF("divide general: (a/g_1)*(d/g_2)");
    kv r1 = dvx(c, b, g2), r2 = dvx(c, cc, g1);
    if (mulx(c, r1, r2, &e)) OVF("divide general: (b/g_2)*(c/g_1)");
    return store(c, neg ? -n.v : n.v, e.v, out);
}

/* ----------------------------------------------------------------------------------------- read, reports */

sk_status P(read)(const L1T *c, sk_handle x, sk_int *m, sk_int *d) {
    if (x >= c->N) return SK_EINVAL;
    rat r = c->R[x];
    sk_int v = c->W[r.num & IDX];
    *m = (r.num & TAG) ? -v : v;
    *d = c->W[r.den];
    return SK_OK;
}

uint32_t P(size)(const L1T *c) { return c->N; }
uint32_t P(wholes)(const L1T *c) { return c->nW; }

void P(memory)(const L1T *c, sk_memory *r) {
    memset(r, 0, sizeof *r);
    sk_memory_add(r, "whole numbers", (uint64_t)c->nW * sizeof(sk_int), (uint64_t)c->capW * sizeof(sk_int));
    uint64_t lb = (uint64_t)c->lk.S * sizeof c->lk.slot[0]; /* every slot, empty or not (spec L-17) */
    sk_memory_add(r, "lookup", lb, lb);
    sk_memory_add(r, "rationals", (uint64_t)c->N * sizeof(rat), (uint64_t)c->capR * sizeof(rat));
    sk_memory_finish(r, c->N);
}

sk_status P(counters)(const L1T *c, sk_counters *r, l1_xcounters *x) {
#ifdef SK_COUNT
    if (r) *r = c->cnt;
    if (x) *x = c->xcnt;
    return SK_OK;
#else
    (void)c;
    if (r) memset(r, 0, sizeof *r);
    if (x) memset(x, 0, sizeof *x);
    return SK_ENOTBUILT;
#endif
}

sk_status P(overflow)(const L1T *c, sk_overflow *r) {
    *r = c->ovf;
    return c->ovf.count ? SK_OK : SK_EINVAL;
}

/* ----------------------------------------------------------------------------------------- audits (spec §14.2) */

#ifdef SK_AUDIT
static const char *check_rats(const L1T *c, uint32_t from) {
    for (uint32_t h = from; h < c->N; h++) {
        uint32_t i = c->R[h].num & IDX, j = c->R[h].den;
        if (i >= c->nW || j >= c->nW) return "rational: a field holds an index not below W";
        sk_int a = c->W[i], b = c->W[j];
        if (b < 1) return "rational: denominator below 1";
        if (sk_gcd(a, b) != 1) return "rational: not in lowest terms";
        if (a == 0 && (b != 1 || (c->R[h].num & TAG))) return "rational: zero not stored as +0/1";
    }
    return NULL;
}

/* whole numbers k in [from, nW): in range, found by the lookup at their index, and used by a rational among those
 * from rfrom on (a whole number is appended only by a store, whose rational follows at once). */
static const char *check_wholes(const L1T *c, uint32_t from, uint32_t rfrom) {
    uint32_t n = c->nW - from;
    if (!n) return NULL;
    unsigned char *used = sk_xcalloc(n, 1);
    for (uint32_t h = rfrom; h < c->N; h++) {
        uint32_t i = c->R[h].num & IDX, j = c->R[h].den;
        if (i >= from && i < c->nW) used[i - from] = 1;
        if (j >= from && j < c->nW) used[j - from] = 1;
    }
    const char *fail = NULL;
    for (uint32_t k = from; k < c->nW && !fail; k++) {
        if (c->W[k] < 0) fail = "whole number: negative";
        else if (lk_find(c, c->W[k], NULL) != k) fail = "whole number: not found at its index by the lookup, or a duplicate";
        else if (!used[k - from]) fail = "whole number: used by no rational";
    }
    free(used);
    return fail;
}

static const char *check_lookup_full(const L1T *c) {
#ifdef L1_OPT
    uint64_t held = 0;
    for (uint32_t s = 0; s < c->lk.S; s++) {
        oslot sl = c->lk.slot[s];
        if (sl.idx == SLOT_EMPTY) continue;
        if (sl.idx >= c->nW) return "lookup: a slot holds an index not below W";
        if (sl.fp != (uint32_t)hprod(c->W[sl.idx])) return "lookup: a fingerprint does not match its value";
        held++;
    }
    if (held != c->nW) return "lookup: does not hold exactly W indices";
    if (2 * (uint64_t)c->nW > c->lk.S) return "lookup: more than half full";
    return NULL;
#elif defined(PLK)
    return PLKN(audit_full)(&c->lk, c->W, c->nW);
#else
    return sk_lookup_audit_full(&c->lk, c->W, c->nW);
#endif
}

static const char *check_lookup_count(const L1T *c) {
#ifdef L1_OPT
    if (c->lk.filled != c->nW) return "lookup: does not hold exactly W indices";
    if (2 * (uint64_t)c->nW > c->lk.S) return "lookup: more than half full";
    return NULL;
#elif defined(PLK)
    return PLKN(audit_count)(&c->lk, c->nW);
#else
    return sk_lookup_audit_count(&c->lk, c->nW);
#endif
}

static sk_status audit_run(L1T *c, int full, const char **why) {
    sk_count *saved = sk_cur;
    l1_xcount *xsaved = c->xcur;
    sk_cur = &sk_sink; /* the audits change no count of the class */
    c->xcur = &x_sink;
    uint32_t rf = full ? 0 : c->mark_N, wf = full ? 0 : c->mark_W;
    const char *fail = check_rats(c, rf);
    if (!fail) fail = check_wholes(c, wf, rf);
    if (!fail) fail = full ? check_lookup_full(c) : check_lookup_count(c);
    sk_cur = saved;
    c->xcur = xsaved;
    if (fail) {
        snprintf(c->msg, sizeof c->msg, "%s: %s (%s audit, from rational %u, whole number %u)", ROWNAME, fail,
                 full ? "full" : "incremental", rf, wf);
        if (why) *why = c->msg;
        return SK_EAUDIT;
    }
    c->mark_N = c->N;
    c->mark_W = c->nW;
    if (why) *why = NULL;
    return SK_OK;
}
#endif

sk_status P(audit)(L1T *c, const char **why) {
#ifdef SK_AUDIT
    return audit_run(c, 1, why);
#else
    (void)c;
    if (why) *why = "audits not built";
    return SK_ENOTBUILT;
#endif
}

sk_status P(audit_changed)(L1T *c, const char **why) {
#ifdef SK_AUDIT
    return audit_run(c, 0, why);
#else
    (void)c;
    if (why) *why = "audits not built";
    return SK_ENOTBUILT;
#endif
}

/* ----------------------------------------------------------------------------------------- test build only */

sk_status P(t_whole)(const L1T *c, uint32_t k, sk_int *v) {
#ifdef SK_AUDIT
    if (k >= c->nW) return SK_EINVAL;
    *v = c->W[k];
    return SK_OK;
#else
    (void)c, (void)k, (void)v;
    return SK_ENOTBUILT;
#endif
}

sk_status P(t_rat)(const L1T *c, sk_handle x, int *neg, uint32_t *i, uint32_t *j) {
#ifdef SK_AUDIT
    if (x >= c->N) return SK_EINVAL;
    *neg = (c->R[x].num & TAG) != 0;
    *i = c->R[x].num & IDX;
    *j = c->R[x].den;
    return SK_OK;
#else
    (void)c, (void)x, (void)neg, (void)i, (void)j;
    return SK_ENOTBUILT;
#endif
}

sk_status P(t_force_general)(L1T *c, int on) {
#ifdef SK_AUDIT
    c->force_general = on;
    return SK_OK;
#else
    (void)c, (void)on;
    return SK_ENOTBUILT;
#endif
}

sk_status P(t_corrupt_rat)(L1T *c, sk_handle x, int neg, uint32_t i, uint32_t j) {
#ifdef SK_AUDIT
    if (x >= c->N) return SK_EINVAL;
    c->R[x].num = (neg ? TAG : 0) | (i & IDX);
    c->R[x].den = j;
    return SK_OK;
#else
    (void)c, (void)x, (void)neg, (void)i, (void)j;
    return SK_ENOTBUILT;
#endif
}

sk_status P(t_corrupt_whole)(L1T *c, uint32_t k, sk_int v) {
#ifdef SK_AUDIT
    if (k >= c->nW) return SK_EINVAL;
    c->W[k] = v;
    return SK_OK;
#else
    (void)c, (void)k, (void)v;
    return SK_ENOTBUILT;
#endif
}

#ifdef L1_OPT
sk_status P(t_tricks)(L1T *c, unsigned mask) {
#ifdef SK_AUDIT
    c->tricks = mask & L1O_ALL;
    return SK_OK;
#else
    (void)c, (void)mask;
    return SK_ENOTBUILT;
#endif
}
#endif
