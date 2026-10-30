/* scale.c: the scalability stress driver (research/idea.md, "Scalability requirements").  Built twice:
 *
 *   -DVAR_S  scale_s: S, every requirement at once (g_s.c) on V3 arrays (s_v3.c)
 *   -DVAR_C  scale_c: C, the control: round two's V0' gcd-scan entry on V3, one-sided edges, no reference counts
 *
 *   scale_x <workload> key=value ...     one JSON object per sweep step on stdout; sampled checks to chk=<file>
 *   scale_x script                       commands on stdin (test_scale.py)
 *
 * Workloads (scale_bench.py drives them):
 *   growth    batches of B values until N values; after each batch, M random ops (half add, half mul, between random
 *             live values) timed with the batch's entry and the idle pass
 *   heavy     deg values 1/30030 (singles: each its own numerator), then 1/7 and 1/6 force two splits of that node
 *   chain     deg values over the product of the first 15 primes, then 1/2, 1/3, ..., 1/43: 14 splits in a row
 *   churn     L live values, `warm` turnovers of L unreported, then E events alternating delete-random and insert
 *             (inserts entered in batches of b; the budget is checked per batch)
 *   distinct  ND distinct denominators, `per` values each, entered in batches, then M random ops
 * Common keys: seed, chk (file for sampled checks), nchk (op results checked per step), nval (stored values checked
 * per step), audit_every, lazy (S: 1 = no Miller-Rabin or rho at entry, leftovers refined in the idle pass),
 * budget_s (stop after this many seconds; for C), policy (V3's rule for D: lcm or freq).
 *
 * Timers cover only the measured phases; checks and audits run outside them (their time is reported separately).
 */
#include "s_core.c"
#ifdef VAR_S
#include "g_s.c"
#define SET_PHASE(x) (s_phase = (x))
#define VARIANT_NAME "S"
#elif defined(VAR_C)
#include "g_c.c"
#define SET_PHASE(x) ((void)0)
#define VARIANT_NAME "C"
#else
#error "build with -DVAR_S or -DVAR_C"
#endif
#include "s_v3.c"

/* ---------------------------------------------------------------- arguments */

static int nargs;
static char **args;
static const char *arg_s(const char *k, const char *def) {
    size_t n = strlen(k);
    for (int i = 0; i < nargs; i++) if (!strncmp(args[i], k, n) && args[i][n] == '=') return args[i] + n + 1;
    return def;
}
static double arg_d(const char *k, double def) { const char *s = arg_s(k, NULL); return s ? atof(s) : def; }
static uint64_t arg_u(const char *k, uint64_t def) { return (uint64_t)arg_d(k, (double)def); }

static FILE *CHK;
static uint64_t T_START;
static double BUDGET;

/* ---------------------------------------------------------------- prime pools */

static uint64_t *pset;
static size_t psetcap, psetn;
static int pset_add(uint64_t x) {
    if (2 * (psetn + 1) > psetcap) {
        uint64_t *o = pset;
        size_t oc = psetcap;
        psetcap = psetcap ? 2 * psetcap : 4096;
        pset = calloc(psetcap, 8);
        for (size_t i = 0; i < oc; i++)
            if (o[i]) { size_t j = hsh(o[i]) & (psetcap - 1); while (pset[j]) j = (j + 1) & (psetcap - 1); pset[j] = o[i]; }
        free(o);
    }
    size_t j = hsh(x) & (psetcap - 1);
    while (pset[j]) { if (pset[j] == x) return 0; j = (j + 1) & (psetcap - 1); }
    pset[j] = x;
    psetn++;
    return 1;
}
static uint64_t new_prime(int lo, int hi) {  /* a random prime in [2^lo, 2^hi), not handed out before */
    for (;;) {
        uint64_t x = (1ull << lo) + rndn((1ull << hi) - (1ull << lo));
        x |= 1;
        while (!is_prime(x)) x += 2;
        if (x < (1ull << hi) && pset_add(x)) return x;
    }
}
static uint64_t *POOL;
static size_t npool, poolcap;
static void pool_push(uint64_t p) {
    if (npool == poolcap) { poolcap = poolcap ? 2 * poolcap : 4096; POOL = realloc(POOL, poolcap * 8); }
    POOL[npool++] = p;
}
static const uint64_t SMALLP[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};
static uint64_t smooth_den(void) {  /* a product of up to three primes below 100, with exponents 1-2, below 2^24 */
    uint64_t d = 1;
    int k = 1 + (int)rndn(3);
    for (int j = 0; j < k; j++) {
        uint64_t p = SMALLP[rndn(25)], e = 1 + rndn(2);
        for (uint64_t t = 0; t < e; t++) if (d * p < (1u << 24)) d *= p;
    }
    return d;
}
static int64_t rnum(void) { return (int64_t)rndn(1ull << 21) - (1ll << 20); }

/* ---------------------------------------------------------------- measurement helpers */

typedef struct { uint64_t ns_add, ns_mul, ovf, n, first_add, first_mul; } opres;
static int REPS = 3;
/* M ops (half add, half mul) on the operand pairs fa, fb (random live values when NULL), results into ring slots
 * [off, off + M); repeated REPS times on the same pairs, the minimum kept (the first repetition reported too);
 * nchk results of the last repetition checked */
static opres op_sample_on(uint32_t M, uint32_t nchk, const uint32_t *fa, const uint32_t *fb, uint32_t off) {
    opres o = {UINT64_MAX, UINT64_MAX, 0, M, 0, 0};
    if (!nlv || !M) { o.ns_add = o.ns_mul = 0; return o; }
    if (ring_n < off + M) die("ring too small");
    uint32_t *pa = malloc((size_t)M * 4), *pb = malloc((size_t)M * 4);
    for (uint32_t j = 0; j < M; j++) {
        pa[j] = fa ? fa[j] : LV[rndn(nlv)];
        pb[j] = fb ? fb[j] : LV[rndn(nlv)];
    }
    raw r;
    for (int rep = 0; rep < REPS; rep++) {
        uint64_t ov0 = st_ovf_ops;
        SET_PHASE(1);
        uint64_t t0 = now_ns();
        for (uint32_t j = 0; j < M / 2; j++) { op_run(OP_ADD, pa[j], pb[j], &r); ring_store(off + j, &r); }
        uint64_t t1 = now_ns();
        for (uint32_t j = M / 2; j < M; j++) { op_run(OP_MUL, pa[j], pb[j], &r); ring_store(off + j, &r); }
        uint64_t t2 = now_ns();
        SET_PHASE(0);
        if (t1 - t0 < o.ns_add) o.ns_add = t1 - t0;
        if (t2 - t1 < o.ns_mul) o.ns_mul = t2 - t1;
        if (!rep) { o.first_add = t1 - t0; o.first_mul = t2 - t1; o.ovf = st_ovf_ops - ov0; }
    }
    if (CHK)
        for (uint32_t c = 0; c < nchk; c++) {
            uint32_t j = (uint32_t)rndn(M);
            const holder_t *X = &H[RING[off + j] - 1];
            fprintf(CHK, "o %c %lld %llu %lld %llu %lld %llu\n", j < M / 2 ? '+' : '*', (long long)ORN[pa[j]],
                    (unsigned long long)ORD[pa[j]], (long long)ORN[pb[j]], (unsigned long long)ORD[pb[j]],
                    (long long)X->num, (unsigned long long)X->den);
        }
    free(pa); free(pb);
    return o;
}
static opres op_sample(uint32_t M, uint32_t nchk) { ring_init(M); return op_sample_on(M, nchk, NULL, NULL, 0); }
static void val_sample(uint32_t nval) {
    if (!CHK || !nlv) return;
    for (uint32_t c = 0; c < nval; c++) {
        uint32_t h = LV[rndn(nlv)];
        int64_t n;
        uint64_t d;
        get_value(h, &n, &d);
        fprintf(CHK, "v %lld %llu %lld %llu\n", (long long)ORN[h], (unsigned long long)ORD[h], (long long)n,
                (unsigned long long)d);
    }
}
static const char *full_audit(uint64_t *ns) {
    uint64_t t0 = now_ns();
    const char *m = g_audit("all");
    if (!m) m = v3_audit();
    *ns = now_ns() - t0;
    return m;
}
static void emit_common(FILE *f) {
    fprintf(f, "\"variant\": \"%s\", \"values\": %u, \"holders\": %u, \"arrays\": %u, \"mem\": %zu, \"mem_peak\": %zu, "
               "\"blocks\": %lld, \"pool_slots\": %zu, \"pool_used\": %zu, \"exceptions_in\": %llu, \"dense_in\": %llu, "
               "\"compactions\": %llu, \"op_restarts\": %llu, \"fill_restarts\": %llu, \"elapsed_s\": %.3f, ",
            VARIANT_NAME, nlv, hlive, arlive, mem_cur, mem_peak, (long long)blk_cur, pool_n, pool_used,
            (unsigned long long)st_exc, (unsigned long long)st_dense, (unsigned long long)st_compact,
            (unsigned long long)st_op_restart, (unsigned long long)st_fill_restart, (now_ns() - T_START) / 1e9);
    g_stats(f);
}
static void emit_audit(FILE *f, const char *m, uint64_t ns, int ran) {
    if (!ran) fprintf(f, ", \"audit\": null");
    else fprintf(f, ", \"audit\": \"%s\", \"audit_ns\": %llu", m ? m : "ok", (unsigned long long)ns);
}
static int over_budget(void) { return BUDGET > 0 && (now_ns() - T_START) / 1e9 > BUDGET; }

/* ---------------------------------------------------------------- workloads */

static void wl_growth(void) {
    uint64_t N = arg_u("N", 10000000), B = arg_u("B", 100000), M = arg_u("M", 100000), T = arg_u("T", 100000);
    uint32_t nchk = (uint32_t)arg_u("nchk", 2000), nval = (uint32_t)arg_u("nval", 1000);
    uint64_t every = arg_u("audit_every", 10), nb = N / B, per = T / (nb ? nb : 1);
    if (!per) per = 1;
    int64_t *num = malloc(B * 8);
    uint64_t *den = malloc(B * 8);
    uint32_t *fa = NULL, *fb = NULL;  /* the fixed sample: the same operand pairs, from the first batch, every step */
    ring_init(2 * (uint32_t)M);
    for (uint64_t b = 0; b < nb; b++) {
        size_t first = npool;
        for (uint64_t t = 0; t < per; t++) pool_push(new_prime(16, 30));
        for (uint64_t i = 0; i < B; i++) {
            double r = rndf();
            uint64_t d;
            if (r < 0.20) d = 1 + rndn(12);
            else if (r < 0.40) d = r < 0.27 ? 10 : r < 0.34 ? 100 : 1000;
            else if (r < 0.55) d = smooth_den();
            else {
                uint64_t q = (rndf() < 0.3) ? POOL[first + rndn(npool - first)] : POOL[rndn(npool)];
                if (r < 0.90) d = (1 + rndn(12)) * q;
                else {
                    uint64_t q2;
                    do q2 = POOL[rndn(npool)]; while (q2 == q);
                    d = q * q2;
                }
            }
            num[i] = rnum();
            den[i] = d;
        }
        uint64_t s0 = 0;
#ifdef VAR_S
        s0 = st_split[0];
#endif
        uint64_t t0 = now_ns();
        enter_batch(num, den, (uint32_t)B, 0);
        uint64_t t1 = now_ns();
        uint64_t idle = g_idle();
        if (!fa) {
            fa = malloc(M * 4); fb = malloc(M * 4);
            for (uint64_t j = 0; j < M; j++) { fa[j] = LV[rndn(nlv)]; fb[j] = LV[rndn(nlv)]; }
        }
        opres f = op_sample_on((uint32_t)M, nchk / 2, fa, fb, 0);
        opres o = op_sample_on((uint32_t)M, nchk - nchk / 2, NULL, NULL, (uint32_t)M);
        val_sample(nval);
        uint64_t ans = 0;
        int ran = (b % every == 0) || b == nb - 1;
        const char *m = ran ? full_audit(&ans) : NULL;
        printf("{\"wl\": \"growth\", \"step\": %llu, \"batch_values\": %llu, \"distinct_in_batch\": %u, "
               "\"entry_ns\": %llu, \"entry_ns_per_value\": %.2f, \"idle_ns\": %llu, \"op_add_ns\": %.2f, "
               "\"op_mul_ns\": %.2f, \"op_ns\": %.2f, \"op_ovf\": %llu, \"ops\": %llu, \"batch_entry_splits\": %llu, "
               "\"fixed_add_ns\": %.2f, \"fixed_mul_ns\": %.2f, \"fixed_ns\": %.2f, \"fixed_ovf\": %llu, "
               "\"fixed_first_ns\": %.2f, \"op_first_ns\": %.2f, \"reps\": %d, ",
               (unsigned long long)b, (unsigned long long)B, (unsigned)dhcnt, (unsigned long long)(t1 - t0),
               (double)(t1 - t0) / B, (unsigned long long)idle, (double)o.ns_add / (M / 2), (double)o.ns_mul / (M - M / 2),
               (double)(o.ns_add + o.ns_mul) / M, (unsigned long long)o.ovf, (unsigned long long)M,
               (unsigned long long)(
#ifdef VAR_S
                   st_split[0] -
#endif
                   s0),
               (double)f.ns_add / (M / 2), (double)f.ns_mul / (M - M / 2), (double)(f.ns_add + f.ns_mul) / M,
               (unsigned long long)f.ovf, (double)(f.first_add + f.first_mul) / M, (double)(o.first_add + o.first_mul) / M,
               REPS);
        emit_common(stdout);
        emit_audit(stdout, m, ans, ran);
        printf("}\n");
        fflush(stdout);
        if (over_budget()) { printf("{\"wl\": \"growth\", \"stopped\": \"budget\", \"step\": %llu}\n", (unsigned long long)b); break; }
    }
    free(num); free(den); free(fa); free(fb);
}

static int64_t num_coprime(uint64_t d) {  /* a random numerator coprime to d, random sign */
    for (;;) {
        int64_t n = (int64_t)rndn(1u << 20) + 1;
        if (gcd64((uint64_t)n, d) == 1) return rndn(2) ? n : -n;
    }
}
/* deg single values over den (each its own numerator) */
static uint64_t build_singles(uint64_t deg, uint64_t den, uint64_t B) {
    int64_t *num = malloc(B * 8);
    uint64_t *dn = malloc(B * 8), t = 0;
    for (uint64_t done = 0; done < deg;) {
        uint64_t n = deg - done < B ? deg - done : B;
        for (uint64_t i = 0; i < n; i++) { num[i] = num_coprime(den); dn[i] = den; }
        uint64_t t0 = now_ns();
        enter_batch(num, dn, (uint32_t)n, 1);
        t += now_ns() - t0;
        done += n;
    }
    free(num); free(dn);
    return t;
}
typedef struct { uint64_t ns, splits, num, entry_ns; } splitres;
static splitres force_split(uint64_t den) {  /* enter 1/den alone; the splits it caused */
    splitres r = {0, 0, 0, 0};
    int64_t one = 1;
#ifdef VAR_S
    uint64_t a = st_split_ns, b = st_split[0], c = st_split_num;
#endif
    uint64_t t0 = now_ns();
    enter_batch(&one, &den, 1, 1);
    r.entry_ns = now_ns() - t0;
#ifdef VAR_S
    r.ns = st_split_ns - a; r.splits = st_split[0] - b; r.num = st_split_num - c;
#endif
    return r;
}
static void emit_split(const char *wl, uint64_t deg, int k, uint64_t den, splitres s, const char *m, uint64_t ans, int ran) {
    printf("{\"wl\": \"%s\", \"deg\": %llu, \"split\": %d, \"den\": %llu, \"entry_ns\": %llu, \"split_ns\": %llu, "
           "\"splits\": %llu, \"numerators\": %llu, \"ns_per_numerator\": %.3f, ",
           wl, (unsigned long long)deg, k, (unsigned long long)den, (unsigned long long)s.entry_ns,
           (unsigned long long)s.ns, (unsigned long long)s.splits, (unsigned long long)s.num,
           s.num ? (double)s.ns / s.num : 0.0);
    emit_common(stdout);
    emit_audit(stdout, m, ans, ran);
    printf("}\n");
    fflush(stdout);
}
static void wl_heavy(void) {
    uint64_t deg = arg_u("deg", 1000000), B = arg_u("B", 100000);
    uint32_t nchk = (uint32_t)arg_u("nchk", 2000), nval = (uint32_t)arg_u("nval", 2000);
    uint64_t tb = build_singles(deg, 30030, B), ans = 0;
    const char *m = full_audit(&ans);
    printf("{\"wl\": \"heavy\", \"deg\": %llu, \"split\": 0, \"build_ns\": %llu, \"build_ns_per_value\": %.2f, ",
           (unsigned long long)deg, (unsigned long long)tb, (double)tb / deg);
    emit_common(stdout);
    emit_audit(stdout, m, ans, 1);
    printf("}\n");
    fflush(stdout);
    uint64_t dens[2] = {7, 6};
    for (int k = 0; k < 2; k++) {
        splitres s = force_split(dens[k]);
        if (k == 1) { op_sample(20000, nchk); val_sample(nval); }
        const char *m2 = full_audit(&ans);  /* after every split, outside the timer */
        emit_split("heavy", deg, k + 1, dens[k], s, m2, ans, 1);
    }
}
static void wl_chain(void) {
    uint64_t deg = arg_u("deg", 100000), B = arg_u("B", 100000), P = 1;
    uint32_t nchk = (uint32_t)arg_u("nchk", 2000), nval = (uint32_t)arg_u("nval", 2000);
    for (int i = 0; i < 15; i++) P *= SMALLP[i];  /* 614889782588491410 < 2^63 */
    uint64_t tb = build_singles(deg, P, B), ans = 0;
    printf("{\"wl\": \"chain\", \"deg\": %llu, \"split\": 0, \"den\": %llu, \"build_ns\": %llu, ", (unsigned long long)deg,
           (unsigned long long)P, (unsigned long long)tb);
    emit_common(stdout);
    emit_audit(stdout, NULL, 0, 0);
    printf("}\n");
    for (int k = 0; k < 14; k++) {
        splitres s = force_split(SMALLP[k]);
        if (k == 13) { op_sample(20000, nchk); val_sample(nval); }
        const char *m = full_audit(&ans);  /* after every split, outside the timer */
        emit_split("chain", deg, k + 1, SMALLP[k], s, m, ans, 1);
    }
}

static uint64_t *PSEQ;  /* churn: a sequence of fresh primes; the window of usable ones slides along it */
static size_t npseq, pseqcap;
static uint64_t pseq(size_t i) {
    while (npseq <= i) {
        if (npseq == pseqcap) { pseqcap = pseqcap ? 2 * pseqcap : 4096; PSEQ = realloc(PSEQ, pseqcap * 8); }
        PSEQ[npseq++] = new_prime(16, 30);
    }
    return PSEQ[i];
}
static uint64_t ins_count;
static uint64_t churn_den(uint64_t rate, uint64_t W) {
    double r = rndf();
    if (r < 0.25) return 1 + rndn(12);
    if (r < 0.40) return r < 0.30 ? 10 : r < 0.35 ? 100 : 1000;
    if (r < 0.50) return smooth_den();
    size_t ws = (size_t)(ins_count / rate);
    uint64_t q = pseq(ws + rndn(W));
    if (r < 0.90) return (1 + rndn(12)) * q;
    uint64_t q2;
    do q2 = pseq(ws + rndn(W)); while (q2 == q);
    return q * q2;
}
static void wl_churn(void) {
    uint64_t L = arg_u("L", 1000000), E = arg_u("E", 10000000), b = arg_u("b", 1000), step = arg_u("step", 1000000);
    uint64_t rate = arg_u("rate", 20), W = arg_u("W", 20000), Bi = arg_u("Bi", 10000);
    uint32_t nchk = (uint32_t)arg_u("nchk", 1000), nval = (uint32_t)arg_u("nval", 1000);
    int64_t *num = malloc((Bi > b ? Bi : b) * 8);
    uint64_t *den = malloc((Bi > b ? Bi : b) * 8);
    /* warm-up: one batch in and out, so the tables that persist are allocated before the baseline */
    for (uint64_t i = 0; i < b; i++) { num[i] = rnum(); den[i] = churn_den(rate, W); }
    enter_batch(num, den, (uint32_t)b, 0);
    g_idle();
    op_sample(64, 0);
    delete_all();
    ring_free();
    g_idle();
    int64_t blk0 = blk_cur;
    size_t mem0 = mem_cur;
    for (uint64_t done = 0; done < L;) {  /* the initial population */
        uint64_t n = L - done < Bi ? L - done : Bi;
        for (uint64_t i = 0; i < n; i++) { num[i] = rnum(); den[i] = churn_den(rate, W); ins_count++; }
        enter_batch(num, den, (uint32_t)n, 0);
        g_idle();
        done += n;
        if (over_budget()) break;  /* the budget applies to the initial fill too */
    }
    uint64_t nbuf = 0, ev = 0, warm = arg_u("warm", 2) * 2 * L;  /* warm-up: `warm` turnovers of L, not reported */
    int k = 0, stopped = over_budget();
    for (uint64_t w = 0; w < warm && !stopped; w++) {
        if (w & 1) { if (nlv) delete_value(LV[rndn(nlv)]); }
        else {
            num[nbuf] = rnum(); den[nbuf] = churn_den(rate, W); nbuf++; ins_count++;
            if (nbuf == b) { enter_batch(num, den, (uint32_t)b, 0); nbuf = 0; g_idle(); stopped = over_budget(); }
        }
    }
    while (ev < E && !stopped) {
        uint64_t t0 = now_ns(), tidle = 0, n_ins = 0, n_del = 0, ev0 = ev;
        uint64_t stop = ev + step < E ? ev + step : E;
        for (; ev < stop; ev++) {
            if (ev & 1) {
                if (nlv) { delete_value(LV[rndn(nlv)]); n_del++; }
            } else {
                num[nbuf] = rnum(); den[nbuf] = churn_den(rate, W); nbuf++; ins_count++; n_ins++;
                if (nbuf == b) {
                    enter_batch(num, den, (uint32_t)b, 0); nbuf = 0; uint64_t ti = g_idle(); tidle += ti;
                    if (over_budget()) { stopped = 1; ev++; break; }  /* checked per batch */
                }
            }
        }
        uint64_t dt = now_ns() - t0;
        val_sample(nval);
        op_sample(1000, nchk);
        ring_free();
        uint64_t ans = 0;
        const char *m = full_audit(&ans);
        printf("{\"wl\": \"churn\", \"step\": %d, \"events\": %llu, \"inserts\": %llu, \"deletes\": %llu, "
               "\"ns_per_event\": %.2f, \"idle_ns\": %llu, \"window_start_prime\": %llu, \"blocks_over_baseline\": %lld, "
               "\"mem_over_baseline\": %lld, ",
               k++, (unsigned long long)ev, (unsigned long long)n_ins, (unsigned long long)n_del, (double)dt / (double)(ev > ev0 ? ev - ev0 : 1),
               (unsigned long long)tidle, (unsigned long long)(ins_count / rate), (long long)(blk_cur - blk0),
               (long long)mem_cur - (long long)mem0);
        emit_common(stdout);
        emit_audit(stdout, m, ans, 1);
        printf("}\n");
        fflush(stdout);
        if (stopped || over_budget()) { stopped = 1; break; }
    }
    if (stopped) printf("{\"wl\": \"churn\", \"stopped\": \"budget\", \"events\": %llu}\n", (unsigned long long)ev);
    /* the end: everything deleted; the leak check */
    if (nbuf) { enter_batch(num, den, (uint32_t)nbuf, 0); nbuf = 0; }
    delete_all();
    ring_free();
    g_idle();
    uint64_t ans = 0;
    const char *m = full_audit(&ans);
    printf("{\"wl\": \"churn\", \"final\": 1, \"blocks_over_baseline\": %lld, \"mem_over_baseline\": %lld, "
           "\"handles_live\": %u, ",
           (long long)(blk_cur - blk0), (long long)mem_cur - (long long)mem0, hdlive);
    emit_common(stdout);
    emit_audit(stdout, m, ans, 1);
    printf("}\n");
    free(num); free(den);
}

static void wl_distinct(void) {
    uint64_t ND = arg_u("ND", 100000), per = arg_u("per", 10), B = arg_u("B", 100000), M = arg_u("M", 100000);
    uint32_t nchk = (uint32_t)arg_u("nchk", 2000), nval = (uint32_t)arg_u("nval", 1000);
    uint64_t np = ND / 4 < 16 ? 16 : ND / 4;
    for (uint64_t i = 0; i < np; i++) pool_push(new_prime(16, 30));
    uint64_t *dl = malloc(ND * 8), nd2 = 0, cap = 1;
    while (cap < 4 * ND) cap *= 2;
    uint64_t *set = calloc(cap, 8);
    while (nd2 < ND) {
        uint64_t d;
        if (rndf() < 0.7) d = (1 + rndn(12)) * POOL[rndn(npool)];
        else { uint64_t a = POOL[rndn(npool)], c = POOL[rndn(npool)]; if (a == c) continue; d = a * c; }
        size_t j = hsh(d) & (cap - 1);
        int dup = 0;
        while (set[j]) { if (set[j] == d) { dup = 1; break; } j = (j + 1) & (cap - 1); }
        if (dup) continue;
        set[j] = d;
        dl[nd2++] = d;
    }
    free(set);
    uint64_t NV = ND * per;
    uint64_t *vd = malloc(NV * 8);
    for (uint64_t i = 0; i < NV; i++) vd[i] = dl[i % ND];
    for (uint64_t i = NV - 1; i > 0; i--) { uint64_t j = rndn(i + 1), t = vd[i]; vd[i] = vd[j]; vd[j] = t; }
    int64_t *num = malloc(B * 8);
    uint64_t tent = 0, tidle = 0, nb = 0, ndist = 0;  /* ndist: distinct denominators summed over the batches */
    for (uint64_t done = 0; done < NV; nb++) {
        uint64_t n = NV - done < B ? NV - done : B;
        for (uint64_t i = 0; i < n; i++) num[i] = rnum();
        uint64_t t0 = now_ns();
        enter_batch(num, vd + done, (uint32_t)n, 0);
        tent += now_ns() - t0;
        ndist += dhcnt;
        tidle += g_idle();
        done += n;
        if (over_budget()) { printf("{\"wl\": \"distinct\", \"ND\": %llu, \"stopped\": \"budget\"}\n", (unsigned long long)ND); return; }
    }
    opres o = op_sample((uint32_t)M, nchk);
    val_sample(nval);
    uint64_t ans = 0;
    const char *m = full_audit(&ans);
    printf("{\"wl\": \"distinct\", \"ND\": %llu, \"per\": %llu, \"batches\": %llu, \"entry_ns\": %llu, "
           "\"entry_ns_per_value\": %.2f, \"entry_ns_per_distinct\": %.2f, \"distinct_entered\": %llu, \"idle_ns\": %llu, \"op_add_ns\": %.2f, "
           "\"op_mul_ns\": %.2f, \"op_ns\": %.2f, \"op_ovf\": %llu, ",
           (unsigned long long)ND, (unsigned long long)per, (unsigned long long)nb, (unsigned long long)tent,
           (double)tent / NV, (double)tent / (double)(ndist ? ndist : 1), (unsigned long long)ndist, (unsigned long long)tidle, (double)o.ns_add / (M / 2),
           (double)o.ns_mul / (M - M / 2), (double)(o.ns_add + o.ns_mul) / M, (unsigned long long)o.ovf);
    emit_common(stdout);
    emit_audit(stdout, m, ans, 1);
    printf("}\n");
    free(dl); free(vd); free(num);
}

/* ---------------------------------------------------------------- the script mode (tests) */

static void print_edges(const char *tag, uint32_t h) {
    printf("%s", tag);
    const holder_t *X = &H[h];
    for (int k = 0; k < X->ne; k++) {
        uint32_t e = EPL[X->eo + k];
        printf(" %llu^%u", (unsigned long long)nd[EN(e)].v, EW(e));
    }
    printf("\n");
}
static void script(void) {
    char line[1 << 16];
    uint32_t rslot = 0;
    int64_t blk0 = blk_cur;
    ring_init(64);
    while (fgets(line, sizeof line, stdin)) {
        char *save, *cmd = strtok_r(line, " \t\r\n", &save);
        if (!cmd || cmd[0] == '#') continue;
        if (!strcmp(cmd, "mode")) {
            const char *m = strtok_r(NULL, " \t\r\n", &save);
#ifdef VAR_S
            s_lazy = !strcmp(m, "lazy");
#else
            (void)m;
#endif
        } else if (!strcmp(cmd, "policy")) {
            v3_freq = !strcmp(strtok_r(NULL, " \t\r\n", &save), "freq");
        } else if (!strcmp(cmd, "enter")) {
            int single = !strcmp(strtok_r(NULL, " \t\r\n", &save), "single");
            int64_t num[4096];
            uint64_t den[4096];
            uint32_t n = 0;
            char *t;
            while ((t = strtok_r(NULL, " \t\r\n", &save))) {
                char *sl = strchr(t, '/');
                num[n] = strtoll(t, NULL, 10);
                den[n] = sl ? strtoull(sl + 1, NULL, 10) : 1;
                n++;
            }
            uint32_t a = enter_batch(num, den, n, single);
            printf("H");
            for (uint32_t i = 0; i < n; i++) printf(" %u", AR[a].hid[i]);
            printf("\n");
        } else if (!strcmp(cmd, "del")) {
            delete_value((uint32_t)atoi(strtok_r(NULL, " \t\r\n", &save)));
        } else if (!strcmp(cmd, "add") || !strcmp(cmd, "mul")) {
            uint32_t a = (uint32_t)atoi(strtok_r(NULL, " \t\r\n", &save)), b = (uint32_t)atoi(strtok_r(NULL, " \t\r\n", &save));
            raw r;
            SET_PHASE(1);
            op_run(cmd[0] == 'a' ? OP_ADD : OP_MUL, a, b, &r);
            SET_PHASE(0);
            ring_store(rslot, &r);
            rslot = (rslot + 1) % 64;
            printf("R %lld %llu\n", (long long)r.num, (unsigned long long)r.den);
        } else if (!strcmp(cmd, "get")) {
            uint32_t h = (uint32_t)atoi(strtok_r(NULL, " \t\r\n", &save));
            int64_t n;
            uint64_t d;
            get_value(h, &n, &d);
            printf("V %lld %llu\n", (long long)n, (unsigned long long)d);
        } else if (!strcmp(cmd, "edges")) {
            uint32_t h = (uint32_t)atoi(strtok_r(NULL, " \t\r\n", &save));
            const arr_t *A = &AR[HD[h].arr];
            uint32_t i = HD[h].idx;
            if (A->xh && A->xh[i]) print_edges("E", A->xh[i] - 1);
            else if (A->dh != NONE) print_edges("D", A->dh);
            else printf("E\n");
        } else if (!strcmp(cmd, "nodes")) {
            g_dump_nodes(stdout);
        } else if (!strcmp(cmd, "degree")) {
            uint64_t v = strtoull(strtok_r(NULL, " \t\r\n", &save), NULL, 10);
            printf("G %u %u\n", g_degree(v), g_refcount(v));
        } else if (!strcmp(cmd, "idle")) {
            g_idle();
            printf("I\n");
        } else if (!strcmp(cmd, "audit")) {
            char *w = strtok_r(NULL, " \t\r\n", &save);
            const char *m = g_audit(w ? w : "all");
            if (!m && (!w || !strcmp(w, "all"))) m = v3_audit();
            if (m) printf("A FAIL %s\n", m);
            else printf("A ok\n");
        } else if (!strcmp(cmd, "corrupt")) {
            printf("C %d\n", g_corrupt(strtok_r(NULL, " \t\r\n", &save)));
        } else if (!strcmp(cmd, "stats")) {
            printf("{");
            emit_common(stdout);
            printf("}\n");
        } else if (!strcmp(cmd, "baseline")) {
            blk0 = blk_cur;
        } else if (!strcmp(cmd, "delall")) {
            delete_all();
            ring_clear();
            g_idle();
            printf("L blocks=%lld nodes=%u holders=%u pool_used=%zu handles=%u arrays=%u\n", (long long)(blk_cur - blk0),
                   nlive, hlive, pool_used, hdlive, arlive);
        } else if (!strcmp(cmd, "cpcheck")) {
            uint64_t v[4096];
            size_t n = 0;
            char *t;
            while ((t = strtok_r(NULL, " \t\r\n", &save))) v[n++] = strtoull(t, NULL, 10);
            printf("P %d %d\n", coprime_check(v, n, 0) == NULL, coprime_check(v, n, 1) == NULL);
        } else if (!strcmp(cmd, "quit")) {
            break;
        } else {
            printf("? %s\n", cmd);
        }
        fflush(stdout);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s growth|heavy|chain|churn|distinct|script key=value ...\n", argv[0]); return 2; }
    nargs = argc - 2;
    args = argv + 2;
    rng_s = arg_u("seed", 1);
    const char *pol = arg_s("policy", "lcm");
    v3_freq = !strcmp(pol, "freq");
#ifdef VAR_S
    s_lazy = (int)arg_u("lazy", 0);
#endif
    BUDGET = arg_d("budget_s", 0);
    REPS = (int)arg_u("reps", 3);
    const char *chk = arg_s("chk", NULL);
    if (chk) CHK = fopen(chk, "w");
    g_init();
    T_START = now_ns();
    const char *w = argv[1];
    if (!strcmp(w, "growth")) wl_growth();
    else if (!strcmp(w, "heavy")) wl_heavy();
    else if (!strcmp(w, "chain")) wl_chain();
    else if (!strcmp(w, "churn")) wl_churn();
    else if (!strcmp(w, "distinct")) wl_distinct();
    else if (!strcmp(w, "script")) script();
    else { fprintf(stderr, "unknown workload %s\n", w); return 2; }
    if (CHK) fclose(CHK);
    return 0;
}
