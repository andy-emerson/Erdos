/* diff.c: randomized differential test of every row against an exact reference (spec §13.1), with the gate's audit
 * schedule (spec §13.3 item 2) in the test build.
 *
 * For each seed and workload family, the same entries and operations are applied to the control, Design 1 and
 * Design 2; half the operations take an earlier result as first operand (as the rule's G3).  Expectations:
 *   - an operation within the operand rule H(x)H(y) <= (2^(w/2-1) - 1)^2 MUST succeed in every row;
 *   - an exact result that does not fit the width MUST be reported as overflow;
 *   - otherwise a row may report overflow (an intermediate did not fit, G-3), and stores nothing;
 *   - every stored value of every row is read out at the end and compared with the reference.
 * The ceiling is tested the same way on vectors.  A checksum of every status, handle and value read is printed,
 * so the timing, counting and test builds can be compared (spec §4.6 item 4: counters and audits change nothing).
 *
 * Usage: diff [--seeds N] [--seed0 S] [--quiet] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ref.h"
#include "skeletons.h"

static const sk_row *ROWS[3] = {&sk_row_ctl, &sk_row_d1, &sk_row_d2};
#define NROWS 3

static uint64_t checksum = UINT64_C(1469598103934665603);
static void mix(uint64_t v) {
    for (int k = 0; k < 8; k++) {
        checksum ^= (v >> (8 * k)) & 0xff;
        checksum *= UINT64_C(1099511628211);
    }
}

static int fail(const char *what, uint64_t seed, int fam, long call) {
    fprintf(stderr, "FAIL seed %llu family %d call %ld: %s\n", (unsigned long long)seed, fam, call, what);
    exit(1);
}

#if SK_WIDTH == 64
#define B_BITS 20
#define OPERAND_BOUND ((i128)2147483647 * 2147483647)
#else
#define B_BITS 9
#define OPERAND_BOUND ((i128)32767 * 32767)
#endif

/* ----------------------------------------------------------------------------------------- workload families */

enum { F_SHARED, F_LG, F_COMPOSITE, F_NONE, F_ALL, F_SMALL, F_WILD, NFAM };
static const char *FAMNAME[NFAM] = {"shared", "lg", "composite", "none", "all", "small", "wild"};

static const int POOL[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
static const int KPR[] = {41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127};
static const int COMPS[] = {2, 3, 4, 6, 8, 9, 10, 12, 15, 21, 35, 36, 77, 143, 216, 221, 1763, 4087};

static bool is_prime(int64_t n) {
    if (n < 2) return false;
    for (int64_t q = 2; q * q <= n; q++)
        if (n % q == 0) return false;
    return true;
}

static int64_t bits_of(int64_t v) {
    int64_t b = 0;
    while (v) {
        b++;
        v >>= 1;
    }
    return b;
}

static int64_t rand_bits(rng *g, int64_t a) { /* uniform in [2^(a-1), 2^a) */
    if (a < 1) return 1;
    return rng_range(g, (int64_t)1 << (a - 1), ((int64_t)1 << a) - 1);
}

static int64_t gen_den(rng *g, int fam, int64_t fixed) {
    int B = B_BITS;
    switch (fam) {
    case F_SHARED: {
        int cap = (int)(rng_below(g, (uint64_t)(B - 4)) + 1);
        int64_t s = 1;
        for (;;) {
            int64_t p = POOL[rng_below(g, 12)];
            if (bits_of(s * p) > cap) break;
            s *= p;
        }
        int64_t a = B - bits_of(s);
        int64_t u;
        do {
            u = rand_bits(g, a < 2 ? 2 : a);
        } while (u % 2 == 0 || u % 3 == 0 || u % 5 == 0 || u % 7 == 0);
        return s * u;
    }
    case F_LG: {
        int64_t s = KPR[rng_below(g, 19)];
        if (rng_below(g, 2) && B >= 20) s *= KPR[rng_below(g, 6)];
        if (B < 20) s = POOL[rng_below(g, 4)] * (int64_t)(rng_below(g, 2) ? POOL[rng_below(g, 4)] : 1);
        int64_t a = B - bits_of(s);
        int64_t u;
        do {
            u = rand_bits(g, a < 2 ? 2 : a);
        } while (!is_prime(u) || (a >= 9 && u <= 127));
        return s * u;
    }
    case F_COMPOSITE: {
        int64_t d = 1;
        int n = (int)rng_below(g, 3) + 1;
        int64_t lim = (int64_t)1 << B;
        for (int k = 0; k < n; k++) {
            int64_t c = COMPS[rng_below(g, sizeof COMPS / sizeof COMPS[0])];
            if (d * c < lim) d *= c;
        }
        return d;
    }
    case F_NONE: {
        int64_t u;
        do {
            u = rand_bits(g, B);
        } while (!is_prime(u));
        return u;
    }
    case F_ALL: return fixed;
    case F_SMALL: return rng_range(g, 1, 64);
    case F_WILD: {
        int64_t top = (int64_t)SK_INT_MAX;
        switch (rng_below(g, 4)) {
        case 0: return rng_range(g, 1, 1000);
        case 1: return rng_range(g, 1, top);
        case 2: return ((int64_t)COMPS[rng_below(g, sizeof COMPS / sizeof COMPS[0])] << rng_below(g, 20)) % top + 1;
        default: return rng_range(g, top / 4, top);
        }
    }
    }
    return 1;
}

static int64_t gen_num(rng *g, int fam) {
    if (fam == F_SMALL) return rng_range(g, -20, 20);
    if (fam == F_WILD) {
        int64_t top = (int64_t)SK_INT_MAX;
        switch (rng_below(g, 4)) {
        case 0: return rng_range(g, -1000, 1000);
        case 1: return rng_range(g, -top, top);
        case 2: return 0;
        default: return rng_range(g, -top / 2, top / 2);
        }
    }
    int64_t lim = ((int64_t)1 << B_BITS) - 1;
    return rng_range(g, -lim, lim);
}

/* ----------------------------------------------------------------------------------------- the rows */

typedef struct value {
    ref_q q;
    sk_handle h[NROWS]; /* UINT32_MAX where the row did not store it */
    bool usable;        /* stored by every row: may be an operand */
    bool result;        /* the result of an operation */
} value;

static value *vals;
static long nvals, capvals;

static long add_value(void) {
    if (nvals == capvals) {
        capvals = capvals ? 2 * capvals : 1024;
        vals = realloc(vals, sizeof(value) * (size_t)capvals);
        if (!vals) exit(2);
    }
    return nvals++;
}

typedef struct stats {
    long calls, ok, ovf, divzero, inval;
    long audits_full, audits_inc;
    long ceil_calls, ceil_ok, ceil_ovf;
    uint64_t d1_inserted, d1_shifted, d1_stride, d1_hits, d2_splits, d2_refined, d2_u1, fast;
} stats;
static stats ST;

static void audit_after(void *const *c, long call, bool every_call_inc, uint64_t seed, int fam, bool last) {
    if (!sk_build_audit()) return;
    for (int t = 0; t < NROWS; t++) {
        const char *why = NULL;
        sk_status s;
        if (every_call_inc || call % 7 == 0) {
            s = ROWS[t]->audit_changed(c[t], &why);
            ST.audits_inc++;
            if (s != SK_OK) {
                fprintf(stderr, "%s: %s\n", ROWS[t]->name, why ? why : "?");
                fail("incremental audit", seed, fam, call);
            }
        }
        if (last || call <= 200 || call % 64 == 0) {
            s = last ? ROWS[t]->audit(c[t], &why) : ROWS[t]->audit_scheduled(c[t], &why);
            ST.audits_full++;
            if (s != SK_OK) {
                fprintf(stderr, "%s: %s\n", ROWS[t]->name, why ? why : "?");
                fail("full audit", seed, fam, call);
            }
        }
    }
}

static int cmp_i128(const void *a, const void *b) {
    i128 x = *(const i128 *)a, y = *(const i128 *)b;
    return x < y ? -1 : x > y;
}

static void run_rows(uint64_t seed, int fam) {
    rng g = {seed * 0x9E3779B97F4A7C15ull + (uint64_t)fam * 0x632BE59BD9B4E019ull + 1};
    for (int k = 0; k < 4; k++) rng_next(&g);
    void *c[NROWS];
    for (int t = 0; t < NROWS; t++) c[t] = ROWS[t]->create();
    nvals = 0;
    long call = 0;
    bool every = (seed % 2) == 0;
    int nden = fam == F_ALL ? 1 : (int)(rng_below(&g, 60) + 4);
    int64_t dens[80];
    int64_t fixed = gen_den(&g, F_SHARED, 0);
    for (int k = 0; k < nden; k++) dens[k] = gen_den(&g, fam, fixed);
    long n_in = 120 + (long)rng_below(&g, 200), n_ops = 600 + (long)rng_below(&g, 900);
    long *entered = malloc(sizeof(long) * (size_t)(n_in + 1)), *results = malloc(sizeof(long) * (size_t)(n_ops + 1));
    long ne = 0, nr = 0;
    long overflows[NROWS] = {0};

    /* entry */
    for (long k = 0; k < n_in; k++) {
        int64_t d = dens[rng_below(&g, (uint64_t)nden)], m = gen_num(&g, fam);
        if (rng_below(&g, 8) == 0) d = -d;              /* a negative denominator */
        if (rng_below(&g, 6) == 0 && fam != F_WILD) {   /* not in lowest terms */
            int64_t f = rng_range(&g, 2, 9);
            if ((i128)m * f <= (i128)SK_INT_MAX && (i128)m * f >= -(i128)SK_INT_MAX &&
                (i128)d * f <= (i128)SK_INT_MAX && (i128)d * f >= -(i128)SK_INT_MAX) {
                m *= f;
                d *= f;
            }
        }
        bool dz = rng_below(&g, 100) == 0, mn = fam == F_WILD && rng_below(&g, 100) == 0;
        if (dz) d = 0;
        if (mn) m = -(int64_t)SK_INT_MAX - 1;
        long id = add_value();
        vals[id].usable = true;
        vals[id].result = false;
        call++;
        for (int t = 0; t < NROWS; t++) {
            sk_handle h = UINT32_MAX;
            sk_status s = ROWS[t]->enter(c[t], (sk_int)m, (sk_int)d, &h);
            mix((uint64_t)s);
            mix(h);
            sk_status want = mn ? SK_EINVAL : dz ? SK_EDIVZERO : SK_OK;
            if (s != want) fail("entry status", seed, fam, call);
            vals[id].h[t] = s == SK_OK ? h : UINT32_MAX;
        }
        ST.calls++;
        if (dz || mn) {
            vals[id].usable = false;
            if (dz) ST.divzero++;
            else ST.inval++;
        } else {
            vals[id].q = ref_make(m, d);
            entered[ne++] = id;
            ST.ok++;
        }
        audit_after(c, call, every, seed, fam, false);
    }
    if (ne == 0) goto done;

    /* operations */
    for (long k = 0; k < n_ops; k++) {
        long xi = (nr > 0 && rng_below(&g, 2)) ? results[rng_below(&g, (uint64_t)nr)] : entered[rng_below(&g, (uint64_t)ne)];
        long yi = entered[rng_below(&g, (uint64_t)ne)];
        if (rng_below(&g, 8) == 0) { long t = xi; xi = yi; yi = t; }
        int op = (int)rng_below(&g, 4);
        ref_q x = vals[xi].q, y = vals[yi].q, r = {0, 1};
        bool dz = op == 3 && y.n == 0;
        if (!dz) r = op == 0 ? ref_add(x, y) : op == 1 ? ref_sub(x, y) : op == 2 ? ref_mul(x, y) : ref_div(x, y);
        bool must_ok = !dz && ref_height(x) * ref_height(y) <= OPERAND_BOUND;
        bool fits = dz || ref_fits(r);
        long id = add_value();
        vals[id].q = r;
        vals[id].usable = !dz;
        vals[id].result = true;
        call++;
        ST.calls++;
        int nok = 0;
        for (int t = 0; t < NROWS; t++) {
            const sk_row *R = ROWS[t];
            sk_status (*f)(void *, sk_handle, sk_handle, sk_handle *) =
                op == 0 ? R->add : op == 1 ? R->sub : op == 2 ? R->mul : R->div;
            uint32_t before = R->size(c[t]);
            sk_handle h = UINT32_MAX;
            sk_status s = f(c[t], vals[xi].h[t], vals[yi].h[t], &h);
            mix((uint64_t)s);
            mix(h);
            if (dz) {
                if (s != SK_EDIVZERO) fail("expected division by zero", seed, fam, call);
            } else if (s == SK_OK) {
                if (!fits) fail("a result that does not fit was stored", seed, fam, call);
                sk_int m, d;
                if (R->read(c[t], h, &m, &d) != SK_OK || !ref_eq(r, m, d)) {
                    fprintf(stderr, "%s op %d: got %lld/%lld, want ", R->name, op, (long long)m, (long long)d);
                    ref_print(stderr, r.n);
                    fputc('/', stderr);
                    ref_print(stderr, r.d);
                    fputc('\n', stderr);
                    fail("wrong value", seed, fam, call);
                }
                nok++;
            } else if (s == SK_EOVERFLOW) {
                if (must_ok) fail("overflow within the operand rule", seed, fam, call);
                overflows[t]++;
                sk_overflow o;
                if (R->overflow(c[t], &o) != SK_OK || o.op != (sk_phase)(SK_PH_ADD + op) || o.x != vals[xi].h[t] ||
                    o.y != vals[yi].h[t] || !o.step)
                    fail("overflow record", seed, fam, call);
                if (R->size(c[t]) != before) fail("overflow stored a value", seed, fam, call);
            } else {
                fail("unexpected status", seed, fam, call);
            }
            vals[id].h[t] = s == SK_OK ? h : UINT32_MAX;
        }
        if (dz) ST.divzero++;
        else if (nok == NROWS) ST.ok++;
        else ST.ovf++;
        if (nok != NROWS) vals[id].usable = false;
        if (vals[id].usable) {
            /* the size bound: results of results are stored but not used again (as G3) */
            if (!vals[xi].result) results[nr++] = id;
        }
        audit_after(c, call, every, seed, fam, k == n_ops - 1);
    }

    /* read-out: every stored value of every row (spec §13.1) */
    for (long v = 0; v < nvals; v++)
        for (int t = 0; t < NROWS; t++) {
            if (vals[v].h[t] == UINT32_MAX) continue;
            sk_int m, d;
            if (ROWS[t]->read(c[t], vals[v].h[t], &m, &d) != SK_OK) fail("read-out", seed, fam, call);
            if (!ref_eq(vals[v].q, m, d)) fail("read-out value", seed, fam, call);
            mix((uint64_t)m);
            mix((uint64_t)d);
        }
    /* counters: overflows, and Design 1 and 2's new denominators equal the distinct stored denominators */
    if (sk_build_counting()) {
        for (int t = 0; t < NROWS; t++) {
            sk_counters k;
            ROWS[t]->counters(c[t], &k);
            uint64_t ov = 0, nd = 0;
            for (int p = 0; p < SK_NPHASE; p++)
                for (int q = 0; q < SK_NPART; q++) {
                    ov += k.c[p][q].ev_overflows;
                    nd += k.c[p][q].ev_new_dens;
                }
            if (ov != (uint64_t)overflows[t]) fail("overflow counter", seed, fam, call);
            for (int p = 0; p < SK_NPHASE; p++)
                for (int q = 0; q < SK_NPART; q++) {
                    const sk_count *x = &k.c[p][q];
                    ST.d1_inserted += x->ref_div_inserted;
                    ST.d1_shifted += x->ref_words_shifted;
                    ST.d1_stride += x->ref_stride_growths;
                    ST.d1_hits += x->ev_pair_hits;
                    ST.d2_splits += x->ref_splits;
                    ST.d2_refined += x->ev_rows_refined;
                    ST.d2_u1 += x->ev_rows_u1;
                    ST.fast += x->ev_fast_paths;
                }
            if (t == 0) continue;
            i128 *ds = malloc(sizeof(i128) * (size_t)(nvals + 1));
            long n = 0;
            for (long v = 0; v < nvals; v++)
                if (vals[v].h[t] != UINT32_MAX) ds[n++] = vals[v].q.d;
            qsort(ds, (size_t)n, sizeof(i128), cmp_i128);
            long distinct = 0;
            for (long v = 0; v < n; v++)
                if (v == 0 || ds[v] != ds[v - 1]) distinct++;
            free(ds);
            if (nd != (uint64_t)distinct) fail("new-denominator counter", seed, fam, call);
        }
    }
    /* memory report: totals are the sum of the items; bytes per rational is used / N */
    for (int t = 0; t < NROWS; t++) {
        sk_memory m;
        ROWS[t]->memory(c[t], &m);
        uint64_t u = 0, a = 0;
        for (unsigned i = 0; i < m.n; i++) {
            u += m.item[i].used;
            a += m.item[i].allocated;
            if (m.item[i].allocated < m.item[i].used) fail("allocated below used", seed, fam, call);
        }
        if (u != m.used || a != m.allocated || m.values != ROWS[t]->size(c[t])) fail("memory totals", seed, fam, call);
    }
done:
    for (int t = 0; t < NROWS; t++) ROWS[t]->destroy(c[t]);
    free(entered);
    free(results);
}

/* ----------------------------------------------------------------------------------------- the ceiling */

typedef struct cvec {
    sk_handle h;
    uint32_t len;
    ref_q *e;
} cvec;

static void run_ceil(uint64_t seed) {
    rng g = {seed * 0xD1B54A32D192ED03ull + 7};
    for (int k = 0; k < 4; k++) rng_next(&g);
    sk_ceil *c = sk_ceil_create();
    const char *why;
    int cap = 400, n = 0;
    cvec *v = calloc((size_t)cap, sizeof(cvec));
    uint32_t L = 1 + (uint32_t)rng_below(&g, 12);
    int64_t dset[4];
    for (int k = 0; k < 4; k++) dset[k] = gen_den(&g, k == 0 ? F_SHARED : F_SMALL, 0);
    long call = 0;
    sk_int *m = malloc(sizeof(sk_int) * L);
    uint32_t *pos = malloc(sizeof(uint32_t) * L);
    unsigned char *neg = malloc(L);
    for (int step = 0; step < 300 && n < cap - 1; step++) {
        int kind = step < 6 ? 0 : (int)rng_below(&g, 7);
        sk_handle h;
        sk_status s;
        call++;
        ST.ceil_calls++;
        if (kind == 0) { /* enter a vector */
            int64_t d = dset[rng_below(&g, 4)] * (rng_below(&g, 5) == 0 ? -1 : 1);
            for (uint32_t k = 0; k < L; k++) m[k] = (sk_int)(rng_below(&g, 6) == 0 ? 0 : rng_range(&g, -300, 300));
            s = sk_ceil_enter_vector(c, m, L, (sk_int)d, &h);
            mix((uint64_t)s);
            if (s != SK_OK) fail("ceil entry", seed, -1, call);
            v[n].h = h;
            v[n].len = L;
            v[n].e = malloc(sizeof(ref_q) * L);
            for (uint32_t k = 0; k < L; k++) v[n].e[k] = ref_make(m[k], d);
            n++;
        } else if (kind == 1) { /* a working copy */
            int src = (int)rng_below(&g, (uint64_t)n);
            for (uint32_t k = 0; k < L; k++) {
                pos[k] = (uint32_t)rng_below(&g, L);
                neg[k] = (unsigned char)rng_below(&g, 2);
            }
            s = sk_ceil_gather_signed(c, v[src].h, pos, neg, L, &h);
            if (s != SK_OK || !(h & SK_CEIL_WORKING)) fail("ceil gather", seed, -1, call);
            v[n].h = h;
            v[n].len = L;
            v[n].e = malloc(sizeof(ref_q) * L);
            for (uint32_t k = 0; k < L; k++) {
                ref_q q = v[src].e[pos[k]];
                if (neg[k]) q.n = -q.n;
                v[n].e[k] = q;
            }
            n++;
        } else { /* an operation */
            int op = (kind - 2) % 4;
            int xi = (int)rng_below(&g, (uint64_t)n), yi = (int)rng_below(&g, (uint64_t)n);
            bool dz = false, fits = true;
            ref_q *r = malloc(sizeof(ref_q) * L);
            for (uint32_t k = 0; k < L; k++) {
                ref_q x = v[xi].e[k], y = v[yi].e[k];
                if (op == 3 && y.n == 0) {
                    dz = true;
                    continue;
                }
                r[k] = op == 0 ? ref_add(x, y) : op == 1 ? ref_sub(x, y) : op == 2 ? ref_mul(x, y) : ref_div(x, y);
                if (!ref_fits(r[k])) fits = false;
            }
            uint32_t before = sk_ceil_size(c);
            s = op == 0 ? sk_ceil_add(c, v[xi].h, v[yi].h, &h) : op == 1 ? sk_ceil_sub(c, v[xi].h, v[yi].h, &h)
              : op == 2 ? sk_ceil_mul(c, v[xi].h, v[yi].h, &h) : sk_ceil_div(c, v[xi].h, v[yi].h, &h);
            mix((uint64_t)s);
            if (dz) {
                if (s != SK_EDIVZERO) fail("ceil: expected division by zero", seed, -1, call);
                free(r);
            } else if (s == SK_OK) {
                if (!fits) fail("ceil: a vector that does not fit was stored", seed, -1, call);
                v[n].h = h;
                v[n].len = L;
                v[n].e = r;
                n++;
                ST.ceil_ok++;
            } else if (s == SK_EOVERFLOW) {
                if (sk_ceil_size(c) != before) fail("ceil: overflow stored a vector", seed, -1, call);
                ST.ceil_ovf++;
                free(r);
            } else {
                fail("ceil: unexpected status", seed, -1, call);
            }
        }
        if (sk_build_audit()) {
            if (sk_ceil_audit_changed(c, &why) != SK_OK) fail(why, seed, -1, call);
            if ((call <= 200 || call % 64 == 0) && sk_ceil_audit_scheduled(c, &why) != SK_OK) fail(why, seed, -1, call);
        }
    }
    for (int t = 0; t < n; t++)
        for (uint32_t k = 0; k < v[t].len; k++) {
            sk_int a, b;
            if (sk_ceil_read(c, v[t].h, k, &a, &b) != SK_OK || !ref_eq(v[t].e[k], a, b)) fail("ceil read-out", seed, -1, t);
            mix((uint64_t)a);
            mix((uint64_t)b);
        }
    if (sk_build_audit() && sk_ceil_audit(c, &why) != SK_OK) fail(why, seed, -1, call);
    for (int t = 0; t < n; t++) free(v[t].e);
    free(v);
    free(m);
    free(pos);
    free(neg);
    sk_ceil_destroy(c);
}

int main(int argc, char **argv) {
    long seeds = 40;
    uint64_t seed0 = 1;
    for (int k = 1; k < argc; k++) {
        if (!strcmp(argv[k], "--seeds") && k + 1 < argc) seeds = atol(argv[++k]);
        else if (!strcmp(argv[k], "--seed0") && k + 1 < argc) seed0 = strtoull(argv[++k], NULL, 10);
    }
    for (long s = 0; s < seeds; s++) {
        for (int f = 0; f < NFAM; f++) run_rows(seed0 + (uint64_t)s, f);
        run_ceil(seed0 + (uint64_t)s);
    }
    printf("width %d, build %s: %ld seeds x %d families + ceiling\n", SK_WIDTH,
           sk_build_audit() ? "test" : sk_build_counting() ? "counting" : "timing", seeds, NFAM);
    printf("row calls %ld: all rows ok %ld, some row overflowed %ld, division by zero %ld, invalid %ld; audits: %ld full, %ld incremental\n",
           ST.calls, ST.ok, ST.ovf, ST.divzero, ST.inval, ST.audits_full, ST.audits_inc);
    printf("ceiling calls %ld: operations ok %ld, overflowed %ld\n", ST.ceil_calls, ST.ceil_ok, ST.ceil_ovf);
    if (sk_build_counting())
        printf("coverage: d1 divisors inserted %llu, words shifted %llu, stride growths %llu, pair hits %llu; "
               "d2 splits %llu, rows refined %llu, rows with u = 1 %llu; fast paths %llu\n",
               (unsigned long long)ST.d1_inserted, (unsigned long long)ST.d1_shifted,
               (unsigned long long)ST.d1_stride, (unsigned long long)ST.d1_hits, (unsigned long long)ST.d2_splits,
               (unsigned long long)ST.d2_refined, (unsigned long long)ST.d2_u1, (unsigned long long)ST.fast);
    printf("checksum %016llx\n", (unsigned long long)checksum);
    free(vals);
    (void)FAMNAME;
    return 0;
}
