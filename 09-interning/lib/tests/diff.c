/* diff.c: randomized differential test (test build).  Random workloads of several shapes; every stored value of L1,
 * L1-opt (every trick on, and a random set of tricks) and the control against an exact 128-bit reference; the case
 * of every operation against the order of §7 applied to magnitudes; L1-opt's whole-number list and stored values
 * equal to L1's; and, per operation, L1's gcds, gcd work, divisions, multiplications and additions equal to the
 * control's on cases den and general (spec §14.1 item 3).  Audits on the gate schedule.
 *
 *   diff [--seeds N]   */
#include "t_common.h"

enum { NMAX = 3000 };

typedef struct wl {
    int n;
    int op[NMAX];
    sk_int m[NMAX], d[NMAX];
    int x[NMAX], y[NMAX];
    ref_q v[NMAX];
} wl;

static i128 H(ref_q q) { return ref_abs(q.n) > q.d ? ref_abs(q.n) : q.d; }

/* shape 0: tiny whole numbers (every case, many overlaps); 1: a pool of ~40 composite whole numbers; 2: wide values;
 * 3: money (cents over 100) */
static void make(wl *w, int shape, int nin, int nops) {
    w->n = 0;
    sk_int pool[40];
    for (int k = 0; k < 40; k++) pool[k] = 1 + (sk_int)rndn(60) * (sk_int)(1 + rndn(30));
    for (int i = 0; i < nin; i++) {
        sk_int a, b;
        switch (shape) {
        case 0: a = (sk_int)rndn(8); b = 1 + (sk_int)rndn(8); break;
        case 1: a = pool[rndn(40)]; b = pool[rndn(40)]; if (rndn(8) == 0) a = 0; break;
        case 2: a = (sk_int)rndn(1u << 20); b = 1 + (sk_int)rndn(1u << 20); break;
        default: a = (sk_int)rndn(2000000) - 1000000; b = 100; break;
        }
        if (rndn(2)) a = -a;
        if (rndn(4) == 0) b = -b;
        w->op[w->n] = 0;
        w->m[w->n] = a;
        w->d[w->n] = b;
        w->v[w->n++] = ref_make(a, b);
    }
    int mix = (int)rndn(4); /* 0 all, 1 add/sub, 2 mul/div, 3 chains */
    for (int t = 0; t < nops && w->n < NMAX; t++) {
        for (int tries = 0; tries < 400; tries++) {
            int op = 1 + (int)rndn(4);
            if (mix == 1) op = 1 + (int)rndn(2);
            if (mix == 2) op = 3 + (int)rndn(2);
            int x = (int)rndn((uint64_t)w->n), y = (int)rndn((uint64_t)w->n);
            if (mix == 3 && w->n > nin) x = w->n - 1;
            if (rndn(6) == 0) y = x;
            if (op == SK_PH_DIV && w->v[y].n == 0) continue;
            if (H(w->v[x]) * H(w->v[y]) > (i128)2147483647 * 2147483647) continue;
            w->op[w->n] = op;
            w->x[w->n] = x;
            w->y[w->n] = y;
            w->v[w->n] = ref_op(op, w->v[x], w->v[y]);
            w->n++;
            break;
        }
    }
}

static uint64_t checksum = 1469598103934665603ull;
static void mix64(uint64_t v) { checksum = (checksum ^ v) * 1099511628211ull; }

static void gate_audit(const row *R, void *c, int k, int n) {
    const char *why = NULL;
    sk_status s = lw_audit(R, c, 0, &why);
    if (s != SK_OK) fprintf(stderr, "  %s audit_changed: %s\n", R->name, why ? why : "?");
    CHECK(s == SK_OK);
    if (k + 1 <= 200 || (k + 1) % 64 == 0 || k + 1 == n) {
        s = lw_audit(R, c, 1, &why);
        if (s != SK_OK) fprintf(stderr, "  %s audit: %s\n", R->name, why ? why : "?");
        CHECK(s == SK_OK);
    }
}

typedef struct snap {
    uint64_t gcd, work, div, mul, add;
} snap;
static snap core_of(const sk_counters *b, int ph) {
    const sk_count *k = &b->c[ph][SK_CORE];
    return (snap){k->gcd, k->gcd_work, k->div_exact, k->mul, k->add};
}

static void one(int seed) {
    static wl w;
    static sk_handle h[3][NMAX];
    rng_s = 0x9E3779B97F4A7C15ull * (uint64_t)(seed + 1) + 12345;
    int shape = seed % 4;
    make(&w, shape, 50 + (int)rndn(400), 200 + (int)rndn(2000));
    unsigned mask = seed % 3 == 0 ? L1O_ALL : (unsigned)rndn(L1O_ALL + 1);
    l1_class *l = l1_create();
    l1o_class *o = l1o_create();
    sk_ctl *k = sk_ctl_create();
    l1o_t_tricks(o, mask);
    l1o_reserve(o, (uint32_t)w.n);
    snap den_l = {0}, den_c = {0}, gen_l = {0}, gen_c = {0};
    for (int i = 0; i < w.n; i++) {
        sk_counters bl0, bl1, bc0, bc1;
        l1_xcounters xl;
        int op = w.op[i];
        if (op) {
            l1_counters(l, &bl0, NULL);
            sk_ctl_counters(k, &bc0);
        }
        sk_status s0, s1, s2;
        if (op == 0) {
            s0 = l1_enter(l, w.m[i], w.d[i], &h[0][i]);
            s1 = l1o_enter(o, w.m[i], w.d[i], &h[1][i]);
            s2 = sk_ctl_enter(k, w.m[i], w.d[i], &h[2][i]);
        } else {
            s0 = ROW_L1.op[op](l, h[0][w.x[i]], h[0][w.y[i]], &h[0][i]);
            s1 = ROW_L1O.op[op](o, h[1][w.x[i]], h[1][w.y[i]], &h[1][i]);
            s2 = ROW_CTL.op[op](k, h[2][w.x[i]], h[2][w.y[i]], &h[2][i]);
        }
        CHECK(s0 == SK_OK && s1 == SK_OK && s2 == SK_OK);
        if (s0 || s1 || s2) {
            fprintf(stderr, "  seed %d call %d op %d: %s %s %s\n", seed, i, op, sk_status_name(s0), sk_status_name(s1),
                    sk_status_name(s2));
            break;
        }
        sk_int m, d;
        ref_q want = w.v[i];
        const row *rows[3] = {&ROW_L1, &ROW_L1O, &ROW_CTL};
        void *cls[3] = {l, o, k};
        for (int r = 0; r < 3; r++) {
            rows[r]->read(cls[r], h[r][i], &m, &d);
            if ((i128)m != want.n || (i128)d != want.d) {
                fprintf(stderr, "  seed %d call %d %s: %lld/%lld, want %lld/%lld\n", seed, i, rows[r]->name, (long long)m,
                        (long long)d, (long long)want.n, (long long)want.d);
                CHECK(0);
            }
        }
        mix64((uint64_t)want.n);
        mix64((uint64_t)want.d);
        if (op) {
            ref_q X = w.v[w.x[i]], Y = w.v[w.y[i]];
            int kase = predict_case(op, (uint64_t)ref_abs(X.n), (uint64_t)X.d, (uint64_t)ref_abs(Y.n), (uint64_t)Y.d);
            l1_counters(l, &bl1, &xl);
            sk_ctl_counters(k, &bc1);
            /* the case: the counter of the predicted case moved by one (cases are counted once per operation) */
            static l1_xcounters prev;
            static int prev_seed = -1;
            if (prev_seed != seed) {
                memset(&prev, 0, sizeof prev);
                prev_seed = seed;
            }
            CHECK_EQ(xsum_case(&xl, op, kase) - xsum_case(&prev, op, kase), 1);
            prev = xl;
            snap a0 = core_of(&bl0, op), a1 = core_of(&bl1, op), c0 = core_of(&bc0, op), c1 = core_of(&bc1, op);
            snap dl = {a1.gcd - a0.gcd, a1.work - a0.work, a1.div - a0.div, a1.mul - a0.mul, a1.add - a0.add};
            snap dc = {c1.gcd - c0.gcd, c1.work - c0.work, c1.div - c0.div, c1.mul - c0.mul, c1.add - c0.add};
            bool isden = kase == L1_ADD_DEN || kase == L1_MUL_DEN || kase == L1_DIV_DEN;
            bool isgen = kase == L1_ADD_GEN || kase == L1_MUL_GEN || kase == L1_DIV_GEN;
            snap *sl = isden ? &den_l : isgen ? &gen_l : NULL, *sc = isden ? &den_c : isgen ? &gen_c : NULL;
            if (sl) {
                sl->gcd += dl.gcd, sl->work += dl.work, sl->div += dl.div, sl->mul += dl.mul, sl->add += dl.add;
                sc->gcd += dc.gcd, sc->work += dc.work, sc->div += dc.div, sc->mul += dc.mul, sc->add += dc.add;
                if (memcmp(&dl, &dc, sizeof dl)) {
                    fprintf(stderr, "  seed %d call %d case %s: L1 differs from ctl\n", seed, i, l1_case_name(kase));
                    CHECK(0);
                }
            }
        }
        gate_audit(&ROW_L1, l, i, w.n);
        gate_audit(&ROW_L1O, o, i, w.n);
        gate_audit(&ROW_CTL, k, i, w.n);
    }
    /* L1-opt: the same whole-number list as L1 */
    CHECK_EQ(l1_wholes(l), l1o_wholes(o));
    for (uint32_t q = 0; q < l1_wholes(l); q++) {
        sk_int a, b;
        l1_t_whole(l, q, &a);
        l1o_t_whole(o, q, &b);
        if (a != b) {
            CHECK(0);
            break;
        }
    }
    /* W against the reference: the distinct magnitudes */
    {
        static sk_int all[2 * NMAX];
        int na = 0;
        for (int i = 0; i < w.n; i++) {
            all[na++] = (sk_int)ref_abs(w.v[i].n);
            all[na++] = (sk_int)w.v[i].d;
        }
        int distinct = 0;
        for (int i = 0; i < na; i++) {
            int seen = 0;
            for (int j = 0; j < i && !seen; j++) seen = all[j] == all[i];
            distinct += !seen;
        }
        CHECK_EQ(l1_wholes(l), distinct);
    }
    CHECK(memcmp(&den_l, &den_c, sizeof den_l) == 0);
    CHECK(memcmp(&gen_l, &gen_c, sizeof gen_l) == 0);
    mix64(l1_wholes(l));
    l1_destroy(l);
    l1o_destroy(o);
    sk_ctl_destroy(k);
}

int main(int argc, char **argv) {
    int seeds = 40;
    for (int i = 1; i + 1 < argc; i++)
        if (!strcmp(argv[i], "--seeds")) seeds = atoi(argv[i + 1]);
    for (int s = 0; s < seeds; s++) one(s);
    printf("diff: %d seeds, %d checks, %d failures\n", seeds, checks, failures);
    printf("checksum %016llx\n", (unsigned long long)checksum);
    return failures ? 1 : 0;
}
