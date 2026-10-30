/* r2_diff.c: the differential test of round 2's rows (spec v0.2 §14.1, §14.3).  Q32 and TC run the same random
 * workloads call for call; every stored value is compared with an exact reference (128-bit integers, its own Euclid
 * gcd), and every call's counters of the two rows with each other: the core arithmetic (gcds, gcd work, exact
 * divisions, multiplications, additions), the case, the comparisons, every K counter and the zero results must be
 * equal (§14.1 item 3).  The values are drawn to meet every case: 0, +-1, x - x, shared whole numbers, small and
 * large magnitudes; divisions by zero and overflows are attempted, and both rows must report them alike, storing
 * nothing.  Audits run along the way; at the end, every case and every K-skip must have been met.
 *
 *   r2_diff [--seeds N]      (test build) */
#include "r2t.h"

static const sk_int SMALL[] = {0, 1, 1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 15, 30};
static const sk_int LARGE[] = {4294967297LL, 4294967291LL, 2147483648LL, 3037000499LL, 1099511627777LL,
                               4611686018427387903LL, 4611686018427387905LL, 1317624576693539401LL,
                               1000000000000000001LL, 999999937LL, 65537LL, 1048576LL};

static sk_int draw(int scale) {
    if (scale == 0 || rndn(4)) return SMALL[rndn(sizeof SMALL / sizeof SMALL[0])];
    if (scale == 1) return 1 + (sk_int)rndn(1u << 20);
    return LARGE[rndn(sizeof LARGE / sizeof LARGE[0])];
}

typedef struct state {
    void *c[2];
    sk_handle *H[2];
    ref_q *V;
    int n;
} state;

typedef struct snap {
    sk_counters b;
    r2_xcounters x;
} snap;

static uint64_t case_hits[R2_NCASE], k_hits[R2_NK + 1], ndivz, novf, ncalls;

/* every per-call counter of the two rows that must be equal: base arithmetic, cases, comparisons, K counters, zero
 * results (Q32's reads, lookups and interning are its own) */
static int compare_calls(const snap *a0, const snap *a1, const snap *b0, const snap *b1) {
    int bad = 0;
    for (int p = 0; p < SK_NPHASE; p++)
        for (int q = 0; q < SK_NPART; q++) {
            const sk_count *qa0 = &a0->b.c[p][q], *qa1 = &a1->b.c[p][q], *qb0 = &b0->b.c[p][q], *qb1 = &b1->b.c[p][q];
#define D(f) (qa1->f - qa0->f != qb1->f - qb0->f)
            if (D(gcd) || D(gcd_work) || D(div_exact) || D(div_rem) || D(mul) || D(add) || D(ev_overflows)) bad++;
            for (int h = 0; h < SK_HIST_BINS; h++)
                if (D(gcd_hist[h])) bad++;
#undef D
            const r2_xcount *xa0 = &a0->x.c[p][q], *xa1 = &a1->x.c[p][q], *xb0 = &b0->x.c[p][q], *xb1 = &b1->x.c[p][q];
#define D(f) (xa1->f - xa0->f != xb1->f - xb0->f)
            if (D(cmp) || D(zero_results)) bad++;
            for (int k = 0; k < R2_NCASE; k++)
                if (D(cases[k])) bad++;
            for (int k = 0; k < R2_NK; k++)
                if (D(k[k].ops) || D(k[k].gcd) || D(k[k].gcd_work) || D(k[k].div) || D(k[k].mul) || D(k[k].add)) bad++;
#undef D
        }
    return bad;
}

static int run_seed(uint64_t seed) {
    rng_s = seed * 0x9E3779B97F4A7C15ull + 1;
    int scale = (int)(seed % 3), nin = 20 + (int)rndn(80), nops = 400 + (int)rndn(800);
    int bad = 0, cap = nin + nops;
    state S;
    S.V = malloc(sizeof(ref_q) * (size_t)cap);
    for (int r = 0; r < 2; r++) {
        S.c[r] = ROWS[r]->create();
        S.H[r] = malloc(sizeof(sk_handle) * (size_t)cap);
    }
    static snap s0[2], s1[2];
    S.n = 0;
    for (int i = 0; i < nin; i++) {
        sk_int m = draw(scale), d = draw(scale);
        if (d == 0) d = 1;
        if (rndn(2)) m = -m;
        if (rndn(5) == 0) d = -d;
        for (int r = 0; r < 2; r++) {
            ROWS[r]->counters(S.c[r], &s0[r].b, &s0[r].x);
            CHECK_EQ(ROWS[r]->enter(S.c[r], m, d, &S.H[r][S.n]), SK_OK);
            ROWS[r]->counters(S.c[r], &s1[r].b, &s1[r].x);
        }
        bad += compare_calls(&s0[0], &s1[0], &s0[1], &s1[1]);
        S.V[S.n++] = ref_make(m, d);
    }
    for (int t = 0; t < nops; t++) {
        int op = 1 + (int)rndn(4);
        int x = (int)rndn((uint64_t)S.n), y = (int)rndn((uint64_t)S.n);
        if (rndn(4) == 0) y = x; /* x op x: x - x, x / x, x * x */
        sk_status st[2];
        sk_handle h[2] = {UINT32_MAX, UINT32_MAX};
        uint32_t n0[2];
        for (int r = 0; r < 2; r++) {
            n0[r] = ROWS[r]->size(S.c[r]);
            ROWS[r]->counters(S.c[r], &s0[r].b, &s0[r].x);
            st[r] = ROWS[r]->op[op](S.c[r], S.H[r][x], S.H[r][y], &h[r]);
            ROWS[r]->counters(S.c[r], &s1[r].b, &s1[r].x);
        }
        ncalls++;
        CHECK_EQ(st[0], st[1]);
        int c = compare_calls(&s0[0], &s1[0], &s0[1], &s1[1]);
        if (c) fprintf(stderr, "  seed %llu call %d (%s): counters differ between Q32 and TC\n",
                       (unsigned long long)seed, t, sk_phase_name((sk_phase)op));
        bad += c;
        if (op == SK_PH_DIV && S.V[y].n == 0) { /* division by zero: reported, nothing stored */
            ndivz++;
            for (int r = 0; r < 2; r++) {
                CHECK_EQ(st[r], SK_EDIVZERO);
                CHECK_EQ(ROWS[r]->size(S.c[r]), n0[r]);
            }
            continue;
        }
        ref_q want = ref_op(op, S.V[x], S.V[y]);
        if (st[0] == SK_EOVERFLOW) { /* an overflow: at the same step in both rows; nothing stored */
            novf++;
            sk_overflow o0, o1;
            ROWS[0]->overflow(S.c[0], &o0);
            ROWS[1]->overflow(S.c[1], &o1);
            CHECK(o0.step && o1.step && !strcmp(o0.step, o1.step));
            for (int r = 0; r < 2; r++) CHECK_EQ(ROWS[r]->size(S.c[r]), n0[r]);
            continue;
        }
        CHECK_EQ(st[0], SK_OK);
        CHECK(ref_fits(want)); /* a result that does not fit 64 bits must overflow */
        for (int r = 0; r < 2; r++) {
            sk_int m = 0, d = 0;
            ROWS[r]->read(S.c[r], h[r], &m, &d);
            if (m != want.n || d != want.d) {
                fprintf(stderr, "  seed %llu: %s %s: %lld/%lld, reference %lld/%lld\n", (unsigned long long)seed,
                        ROWS[r]->name, sk_phase_name((sk_phase)op), (long long)m, (long long)d, (long long)want.n,
                        (long long)want.d);
                bad++;
            }
            S.H[r][S.n] = h[r];
        }
        int sy = (S.V[y].n < 0) != (op == SK_PH_SUB);
        int kase = predict_case(op, (uint64_t)ref_abs(S.V[x].n), (uint64_t)S.V[x].d, (uint64_t)ref_abs(S.V[y].n),
                                (uint64_t)S.V[y].d, S.V[x].n < 0, sy);
        CHECK_EQ(XCASES(s1[0].x, op, kase) - XCASES(s0[0].x, op, kase), 1);
        case_hits[kase]++;
        for (int k = 1; k <= R2_NK; k++) k_hits[k] += XK(s1[0].x, -1, k, ops) - XK(s0[0].x, -1, k, ops);
        S.V[S.n++] = want;
        if (t % 97 == 0)
            for (int r = 0; r < 2; r++) {
                const char *why = NULL;
                if (ROWS[r]->audit_changed(S.c[r], &why) != SK_OK) {
                    fprintf(stderr, "  seed %llu: %s audit: %s\n", (unsigned long long)seed, ROWS[r]->name, why);
                    bad++;
                }
            }
    }
    for (int r = 0; r < 2; r++) {
        const char *why = NULL;
        if (ROWS[r]->audit(S.c[r], &why) != SK_OK) {
            fprintf(stderr, "  seed %llu: %s full audit: %s\n", (unsigned long long)seed, ROWS[r]->name, why);
            bad++;
        }
        for (int i = 0; i < S.n; i++) { /* the read-out, every stored value */
            sk_int m, d;
            ROWS[r]->read(S.c[r], S.H[r][i], &m, &d);
            if (m != S.V[i].n || d != S.V[i].d) bad++;
        }
        ROWS[r]->destroy(S.c[r]);
        free(S.H[r]);
    }
    free(S.V);
    CHECK_EQ(bad, 0);
    return bad;
}

int main(int argc, char **argv) {
    int seeds = 200;
    for (int i = 1; i + 1 < argc; i++)
        if (!strcmp(argv[i], "--seeds")) seeds = atoi(argv[i + 1]);
    for (int s = 1; s <= seeds; s++) run_seed((uint64_t)s);
    int missing = 0;
    for (int k = 0; k < R2_NCASE; k++)
        if (!case_hits[k]) {
            fprintf(stderr, "  case %s never met\n", r2_case_name(k));
            missing++;
        }
    for (int k = 1; k <= R2_NK; k++)
        if (!k_hits[k] && k != 1) { /* K-1 is entry's, not counted here */
            fprintf(stderr, "  K-%d never met\n", k);
            missing++;
        }
    CHECK_EQ(missing, 0);
    CHECK(ndivz > 0);
    CHECK(novf > 0);
    printf("r2_diff: %d seeds, %llu operations (%llu divisions by zero, %llu overflows), every case and K-skip met; "
           "%d checks, %d failures\n", seeds, (unsigned long long)ncalls, (unsigned long long)ndivz,
           (unsigned long long)novf, checks, failures);
    return failures != 0;
}
