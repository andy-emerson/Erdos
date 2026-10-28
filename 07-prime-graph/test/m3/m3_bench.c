/* m3_bench.c: the contenders of the M3 peer comparison, one exact job per invocation (driven by m3_bench.py).
 *
 *   m3_bench <contender> <job> <indir> <outfile> <reps> [K | opsname]
 *   m3_bench --version
 *
 * contenders: pg       the M1 library (lib/build/libprimegraph.a), V4 form: array jobs through its expression API
 *                      (pg_array_* between pg_expr_begin and pg_expr_end), ops jobs through single-value calls
 *             pg-v3    the same, V3 form: each array call outside any expression (reduced on its own); the harness
 *                      calibrates between pg and pg-v3 for fma and chain jobs
 *             mpq      GMP mpq_t per element (mpq_set_si + mpq_canonicalize at entry)
 *             fmpq     FLINT fmpq per element (fmpq_set_si at entry); a*b + c as fmpq_mul + fmpq_add
 *             fmpq-am  the same, a*b + c as fmpq_set + fmpq_addmul (the harness calibrates between the two forms)
 *             fz       FLINT fmpz numerators over one shared denominator D (array jobs only); its reduction of the
 *                      results to lowest terms (fmpq_set_fmpz_frac) is part of its timed exit
 * jobs:       sum      column sum of a
 *             fma      a*b + c elementwise
 *             chain    x <- x*b + c, K times, x starting at a (pg: one expression, intermediates released inside)
 *             ops      single values: enter every value of v.bin, then run the operation list ops-<opsname>.bin
 *                      (result t = v[ia[t]] + v[ib[t]] if op[t] = 0, v[ia[t]] * v[ib[t]] if op[t] = 1)
 * inputs:     <indir>/{a,b,c,v}.bin: int64 n, n int64 numerators, n int64 denominators (positive, below 2^63)
 *             <indir>/ops-<name>.bin: int64 T, then T int32 ia, T int32 ib, T int32 op
 * phases:     entry  conversion from the raw (num, den) inputs (pg: pg_graph_create + entry, every rep a new graph)
 *             op     the job
 *             exit   every result read out in lowest terms as two int64 (pg_array_to_fracs / pg_value_get;
 *                    mpz_get_si / fmpz_get_si after a fits test; fz: fmpq_set_fmpz_frac, then fmpz_get_si)
 *             total  the minimum over reps of one rep's entry + op + exit; each phase column is its own minimum.
 *                    With reps > 1 the first (cold) rep is discarded: minima over reps 2..reps.
 *             Allocation: in ops jobs (single values) GMP and FLINT initialise every value inside the timed entry
 *             and every result inside the timed op, as M1 allocates its values and results there; they are cleared
 *             untimed after the rep.  In array jobs GMP and FLINT vectors are initialised once before the reps and
 *             reused.  pg's graph is destroyed after each rep.  Releases, clears and destruction are not timed.
 * outfile:    int64 count, count int64 numerators, count int64 denominators; denominator 0 marks an element the
 *             contender could not produce (pg: an operation or a read-out that returned an error; GMP/FLINT: a
 *             result beyond 64 bits)
 * stdout:     entry=<ns> op=<ns> exit=<ns> total=<ns> flint_threads=<k> ..., then contender statistics
 * exit code:  0 ok, 3 unable (fz on an ops job), other: error
 * M3_CORRUPT: (tests only) "num:i", "den:i", "unred:i", "sign:i", "ovf:i", "drop", "swap:i" corrupts the output
 *             after the timed reps, so test_m3.py can show that each check catches a wrong result.
 */
#include <flint/flint.h>
#include <flint/fmpq.h>
#include <flint/fmpq_vec.h>
#include <flint/fmpz.h>
#include <flint/fmpz_vec.h>
#include <gmp.h>
#include <inttypes.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "primegraph.h"

typedef struct { long n; int64_t *num, *den; } in_arr;

static const char *CON, *JOB, *DIR, *OUT;
static int REPS, K;
static in_arr IN[3], V;
static int NIN;
static long N;                      /* array length (array jobs) */
static long T, NV;                  /* operations and values (ops job) */
static int32_t *IA, *IB, *OPC;
static long NOUT;
static int64_t *ON, *OD;            /* the results, read out in lowest terms (the exit phase) */
static uint64_t be = UINT64_MAX, bo = UINT64_MAX, bx = UINT64_MAX, bt = UINT64_MAX;
static long toolong;                /* GMP/FLINT results that did not fit int64 (never expected) */

static uint64_t now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec;
}
static int REP;  /* the current rep; with REPS > 1, rep 0 (cold) is not recorded */
static void record(uint64_t t0, uint64_t t1, uint64_t t2, uint64_t t3) {
    if (REPS > 1 && REP == 0) return;
    if (t1 - t0 < be) be = t1 - t0;
    if (t2 - t1 < bo) bo = t2 - t1;
    if (t3 - t2 < bx) bx = t3 - t2;
    if (t3 - t0 < bt) bt = t3 - t0;
}
static int is(const char *j) { return strcmp(JOB, j) == 0; }
static void *xmalloc(size_t n) { void *p = malloc(n ? n : 1); if (!p) { fprintf(stderr, "out of memory\n"); exit(1); } return p; }

static void read_arr(const char *name, in_arr *a) {
    char p[4096];
    snprintf(p, sizeof p, "%s/%s.bin", DIR, name);
    FILE *f = fopen(p, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", p); exit(1); }
    int64_t n;
    if (fread(&n, 8, 1, f) != 1) exit(1);
    a->n = n;
    a->num = xmalloc(8 * (size_t)n);
    a->den = xmalloc(8 * (size_t)n);
    if (fread(a->num, 8, n, f) != (size_t)n || fread(a->den, 8, n, f) != (size_t)n) { fprintf(stderr, "short %s\n", p); exit(1); }
    fclose(f);
}

static void read_ops(const char *name) {
    char p[4096];
    snprintf(p, sizeof p, "%s/ops-%s.bin", DIR, name);
    FILE *f = fopen(p, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", p); exit(1); }
    int64_t t;
    if (fread(&t, 8, 1, f) != 1) exit(1);
    T = t;
    IA = xmalloc(4 * (size_t)T); IB = xmalloc(4 * (size_t)T); OPC = xmalloc(4 * (size_t)T);
    if (fread(IA, 4, T, f) != (size_t)T || fread(IB, 4, T, f) != (size_t)T || fread(OPC, 4, T, f) != (size_t)T) {
        fprintf(stderr, "short %s\n", p); exit(1);
    }
    fclose(f);
    for (long i = 0; i < T; i++)
        if (IA[i] < 0 || IA[i] >= NV || IB[i] < 0 || IB[i] >= NV || (OPC[i] != 0 && OPC[i] != 1)) {
            fprintf(stderr, "bad operation %ld\n", i); exit(1);
        }
}

/* ------------------------------------------------------------------ output (after the timed reps) */

static void corrupt(void) {
    const char *c = getenv("M3_CORRUPT");
    if (!c || !*c) return;
    long i = 0;
    const char *colon = strchr(c, ':');
    if (colon) i = atol(colon + 1);
    if (i < 0 || i >= NOUT) i = NOUT - 1;
    if (!strncmp(c, "num", 3)) ON[i] += 1;
    else if (!strncmp(c, "den", 3)) OD[i] += 1;
    else if (!strncmp(c, "unred", 5)) { ON[i] *= 2; OD[i] *= 2; }
    else if (!strncmp(c, "sign", 4)) { ON[i] = -ON[i]; OD[i] = -OD[i]; }
    else if (!strncmp(c, "ovf", 3)) { ON[i] = 0; OD[i] = 0; }
    else if (!strncmp(c, "drop", 4)) NOUT -= 1;
    else if (!strncmp(c, "swap", 4) && NOUT > 1) {
        long j = i ? i - 1 : 1;
        int64_t tn = ON[i], td = OD[i];
        ON[i] = ON[j]; OD[i] = OD[j]; ON[j] = tn; OD[j] = td;
    } else { fprintf(stderr, "unknown M3_CORRUPT %s\n", c); exit(1); }
}

static void write_out(void) {
    corrupt();
    FILE *f = fopen(OUT, "wb");
    if (!f) { perror(OUT); exit(1); }
    int64_t n = NOUT;
    fwrite(&n, 8, 1, f);
    fwrite(ON, 8, (size_t)NOUT, f);
    fwrite(OD, 8, (size_t)NOUT, f);
    fclose(f);
}

static void print_times(void) {
    printf("entry=%llu op=%llu exit=%llu total=%llu flint_threads=%d toolong=%ld", (unsigned long long)be,
           (unsigned long long)bo, (unsigned long long)bx, (unsigned long long)bt, flint_get_num_threads(), toolong);
}

/* ------------------------------------------------------------------ pg: the M1 library */

static void run_pg(int v4) {
    pg_stats st = {0};
    pg_array_info info = {0};
    long fails = 0;
    if (is("ops")) {
        pg_value **v = xmalloc(sizeof(pg_value *) * (size_t)NV), **r = xmalloc(sizeof(pg_value *) * (size_t)T);
        for (int rep = 0; rep < REPS; rep++) {
        REP = rep;
            fails = 0;
            uint64_t t0 = now();
            pg_graph *g = pg_graph_create();
            for (long i = 0; i < NV; i++)
                if (pg_value_new(g, V.num[i], V.den[i], &v[i]) != PG_OK) { fprintf(stderr, "entry failed at %ld\n", i); exit(1); }
            uint64_t t1 = now();
            for (long t = 0; t < T; t++) {
                pg_status s = OPC[t] ? pg_mul(g, v[IA[t]], v[IB[t]], &r[t]) : pg_add(g, v[IA[t]], v[IB[t]], &r[t]);
                if (s != PG_OK) r[t] = NULL;
            }
            uint64_t t2 = now();
            for (long t = 0; t < T; t++) {
                if (!r[t] || pg_value_get(g, r[t], &ON[t], &OD[t]) != PG_OK) { ON[t] = 0; OD[t] = 0; fails++; }
            }
            uint64_t t3 = now();
            record(t0, t1, t2, t3);
            pg_stats_get(g, &st);
            pg_graph_destroy(g);  /* frees every value (untimed) */
        }
    } else {
        for (int rep = 0; rep < REPS; rep++) {
        REP = rep;
            pg_array *X[3] = {0}, *R = NULL;
            pg_value *sv = NULL;
            pg_status s = PG_OK;
            fails = 0;
            uint64_t t0 = now();
            pg_graph *g = pg_graph_create();
            for (int j = 0; j < NIN; j++)
                if (pg_array_from_fracs(g, IN[j].num, IN[j].den, (size_t)N, &X[j]) != PG_OK) { fprintf(stderr, "entry failed\n"); exit(1); }
            uint64_t t1 = now();
            if (v4) pg_expr_begin(g);
            if (is("sum")) s = pg_array_sum(g, X[0], &sv);
            else if (is("fma")) s = pg_array_fma(g, X[0], X[1], X[2], &R);
            else {  /* chain (v4: one expression); each intermediate released as soon as the next exists */
                pg_array *x = X[0];
                for (int k = 0; k < K && s == PG_OK; k++) {
                    pg_array *y = NULL;
                    s = pg_array_fma(g, x, X[1], X[2], &y);
                    if (x != X[0]) pg_array_release(g, x);
                    x = y;
                }
                R = s == PG_OK ? x : NULL;
            }
            if (v4 && pg_expr_end(g) != PG_OK) { fprintf(stderr, "pg_expr_end failed\n"); exit(1); }
            uint64_t t2 = now();
            if (s != PG_OK) {
                for (long i = 0; i < NOUT; i++) ON[i] = OD[i] = 0;
                fails = NOUT;
            } else if (is("sum")) {
                if (pg_value_get(g, sv, &ON[0], &OD[0]) != PG_OK) { ON[0] = OD[0] = 0; fails = 1; }
            } else if (pg_array_to_fracs(g, R, ON, OD) != PG_OK) {
                for (long i = 0; i < NOUT; i++) ON[i] = OD[i] = 0;
                fails = NOUT;
            }
            uint64_t t3 = now();
            record(t0, t1, t2, t3);
            pg_stats_get(g, &st);
            if (R) pg_array_info_get(R, &info);
            pg_graph_destroy(g);  /* frees the arrays and values (untimed) */
        }
    }
    write_out();
    print_times();
    printf(" form=%s fails=%ld nodes=%zu splits_entry=%zu splits_reduce=%zu restarts=%zu v4_retries=%zu rho=%zu "
           "cache_hits=%zu exc_out=%zu D=%" PRId64 " mem_peak=%zu\n", v4 ? "v4" : "v3", fails, st.nodes_live, st.splits_entry,
           st.splits_reduce, st.restarts, st.v4_retries, st.rho_calls, st.den_cache_hits, info.exceptions, info.D,
           st.mem_peak);
}

/* ------------------------------------------------------------------ mpq: GMP per element */

static void mpq_out(long i, const mpq_t q) {
    if (mpz_fits_slong_p(mpq_numref(q)) && mpz_fits_slong_p(mpq_denref(q))) {
        ON[i] = mpz_get_si(mpq_numref(q));
        OD[i] = mpz_get_si(mpq_denref(q));
    } else { ON[i] = OD[i] = 0; toolong++; }
}

static void run_mpq(void) {
    if (is("ops")) {
        mpq_t *v = xmalloc(sizeof(mpq_t) * (size_t)NV), *r = xmalloc(sizeof(mpq_t) * (size_t)T);
        for (int rep = 0; rep < REPS; rep++) {
        REP = rep;
            toolong = 0;
            uint64_t t0 = now();  /* values and results allocated inside the timed phases, as M1's are */
            for (long i = 0; i < NV; i++) { mpq_init(v[i]); mpq_set_si(v[i], V.num[i], (unsigned long)V.den[i]); mpq_canonicalize(v[i]); }
            uint64_t t1 = now();
            for (long t = 0; t < T; t++) {
                mpq_init(r[t]);
                if (OPC[t]) mpq_mul(r[t], v[IA[t]], v[IB[t]]);
                else mpq_add(r[t], v[IA[t]], v[IB[t]]);
            }
            uint64_t t2 = now();
            for (long t = 0; t < T; t++) mpq_out(t, r[t]);
            uint64_t t3 = now();
            record(t0, t1, t2, t3);
            for (long i = 0; i < NV; i++) mpq_clear(v[i]);  /* untimed, as pg_graph_destroy */
            for (long t = 0; t < T; t++) mpq_clear(r[t]);
        }
    } else {
        mpq_t *x[3];
        for (int j = 0; j < NIN; j++) { x[j] = xmalloc(sizeof(mpq_t) * (size_t)N); for (long i = 0; i < N; i++) mpq_init(x[j][i]); }
        mpq_t *r = xmalloc(sizeof(mpq_t) * (size_t)N);
        for (long i = 0; i < N; i++) mpq_init(r[i]);
        mpq_t s;
        mpq_init(s);
        for (int rep = 0; rep < REPS; rep++) {
        REP = rep;
            toolong = 0;
            uint64_t t0 = now();
            for (int j = 0; j < NIN; j++)
                for (long i = 0; i < N; i++) { mpq_set_si(x[j][i], IN[j].num[i], (unsigned long)IN[j].den[i]); mpq_canonicalize(x[j][i]); }
            uint64_t t1 = now();
            if (is("sum")) { mpq_set_ui(s, 0, 1); for (long i = 0; i < N; i++) mpq_add(s, s, x[0][i]); }
            else if (is("fma")) for (long i = 0; i < N; i++) { mpq_mul(r[i], x[0][i], x[1][i]); mpq_add(r[i], r[i], x[2][i]); }
            else for (int k = 0; k < K; k++) for (long i = 0; i < N; i++) { mpq_mul(x[0][i], x[0][i], x[1][i]); mpq_add(x[0][i], x[0][i], x[2][i]); }
            uint64_t t2 = now();
            if (is("sum")) mpq_out(0, s);
            else { mpq_t *res = is("chain") ? x[0] : r; for (long i = 0; i < N; i++) mpq_out(i, res[i]); }
            uint64_t t3 = now();
            record(t0, t1, t2, t3);
        }
    }
    write_out();
    print_times();
    printf("\n");
}

/* ------------------------------------------------------------------ fmpq: FLINT per element */

static void fmpq_out(long i, const fmpq_t q) {
    if (fmpz_fits_si(fmpq_numref(q)) && fmpz_fits_si(fmpq_denref(q))) {
        ON[i] = fmpz_get_si(fmpq_numref(q));
        OD[i] = fmpz_get_si(fmpq_denref(q));
    } else { ON[i] = OD[i] = 0; toolong++; }
}

static void run_fmpq(int addmul) {
    if (is("ops")) {
        fmpq *v = xmalloc(sizeof(fmpq) * (size_t)NV), *r = xmalloc(sizeof(fmpq) * (size_t)T);
        for (int rep = 0; rep < REPS; rep++) {
        REP = rep;
            toolong = 0;
            uint64_t t0 = now();  /* values and results initialised inside the timed phases, as M1's are */
            for (long i = 0; i < NV; i++) { fmpq_init(v + i); fmpq_set_si(v + i, V.num[i], (ulong)V.den[i]); }
            uint64_t t1 = now();
            for (long t = 0; t < T; t++) {
                fmpq_init(r + t);
                if (OPC[t]) fmpq_mul(r + t, v + IA[t], v + IB[t]);
                else fmpq_add(r + t, v + IA[t], v + IB[t]);
            }
            uint64_t t2 = now();
            for (long t = 0; t < T; t++) fmpq_out(t, r + t);
            uint64_t t3 = now();
            record(t0, t1, t2, t3);
            for (long i = 0; i < NV; i++) fmpq_clear(v + i);  /* untimed, as pg_graph_destroy */
            for (long t = 0; t < T; t++) fmpq_clear(r + t);
        }
    } else {
        fmpq *x[3];
        for (int j = 0; j < NIN; j++) x[j] = _fmpq_vec_init(N);
        fmpq *r = _fmpq_vec_init(N);
        fmpq_t s;
        fmpq_init(s);
        for (int rep = 0; rep < REPS; rep++) {
        REP = rep;
            toolong = 0;
            uint64_t t0 = now();
            for (int j = 0; j < NIN; j++) for (long i = 0; i < N; i++) fmpq_set_si(x[j] + i, IN[j].num[i], (ulong)IN[j].den[i]);
            uint64_t t1 = now();
            if (is("sum")) { fmpq_zero(s); for (long i = 0; i < N; i++) fmpq_add(s, s, x[0] + i); }
            else if (is("fma") && addmul) for (long i = 0; i < N; i++) { fmpq_set(r + i, x[2] + i); fmpq_addmul(r + i, x[0] + i, x[1] + i); }
            else if (is("fma")) for (long i = 0; i < N; i++) { fmpq_mul(r + i, x[0] + i, x[1] + i); fmpq_add(r + i, r + i, x[2] + i); }
            else if (addmul)  /* chain: x*b + c as c + x*b, with one temporary per element */
                for (int k = 0; k < K; k++) for (long i = 0; i < N; i++) { fmpq_set(r + i, x[2] + i); fmpq_addmul(r + i, x[0] + i, x[1] + i); fmpq_swap(r + i, x[0] + i); }
            else for (int k = 0; k < K; k++) for (long i = 0; i < N; i++) { fmpq_mul(x[0] + i, x[0] + i, x[1] + i); fmpq_add(x[0] + i, x[0] + i, x[2] + i); }
            uint64_t t2 = now();
            if (is("sum")) fmpq_out(0, s);
            else { fmpq *res = is("chain") ? x[0] : r; for (long i = 0; i < N; i++) fmpq_out(i, res + i); }
            uint64_t t3 = now();
            record(t0, t1, t2, t3);
        }
    }
    write_out();
    print_times();
    printf("\n");
}

/* ------------------------------------------------------------------ fz: FLINT fmpz over one shared denominator */

static void run_fz(void) {
    if (is("ops")) { printf("unable no shared denominator in a single-value job\n"); exit(3); }
    fmpz *x[3];
    for (int j = 0; j < NIN; j++) x[j] = _fmpz_vec_init(N);
    fmpz *r = _fmpz_vec_init(N);
    fmpz *cof = _fmpz_vec_init(65536);
    long nout = is("sum") ? 1 : N;
    fmpq *red = _fmpq_vec_init(nout);
    static uint8_t seen[65536];
    fmpz_t D, DO, s, S, t;
    fmpz_init(D); fmpz_init(DO); fmpz_init(s); fmpz_init(S); fmpz_init(t);
    for (int rep = 0; rep < REPS; rep++) {
        REP = rep;
        toolong = 0;
        memset(seen, 0, sizeof seen);
        for (int d = 0; d < 65536; d++) fmpz_zero(cof + d);
        uint64_t t0 = now();
        fmpz_one(D);  /* the common denominator of every input, computed from the data */
        for (int j = 0; j < NIN; j++)
            for (long i = 0; i < N; i++) {
                uint64_t d = (uint64_t)IN[j].den[i];
                if (d < 65536) { if (seen[d]) continue; seen[d] = 1; }
                fmpz_set_ui(t, d);
                fmpz_lcm(D, D, t);
            }
        for (int j = 0; j < NIN; j++)
            for (long i = 0; i < N; i++) {
                uint64_t d = (uint64_t)IN[j].den[i];
                if (d < 65536) {
                    if (fmpz_is_zero(cof + d)) fmpz_divexact_ui(cof + d, D, d);
                    fmpz_mul_si(x[j] + i, cof + d, IN[j].num[i]);
                } else {
                    fmpz_divexact_ui(t, D, d);
                    fmpz_mul_si(x[j] + i, t, IN[j].num[i]);
                }
            }
        uint64_t t1 = now();
        if (is("sum")) { _fmpz_vec_sum(s, x[0], N); fmpz_set(DO, D); }
        else if (is("fma")) {  /* (a*b + c*D) / D^2 */
            for (long i = 0; i < N; i++) { fmpz_mul(r + i, x[0] + i, x[1] + i); fmpz_addmul(r + i, x[2] + i, D); }
            fmpz_mul(DO, D, D);
        } else {  /* chain: x over S; x*b + c*S over S*D */
            fmpz_set(S, D);
            for (int k = 0; k < K; k++) {
                for (long i = 0; i < N; i++) { fmpz_mul(x[0] + i, x[0] + i, x[1] + i); fmpz_addmul(x[0] + i, x[2] + i, S); }
                fmpz_mul(S, S, D);
            }
            fmpz_set(DO, S);
        }
        uint64_t t2 = now();
        if (is("sum")) { fmpq_set_fmpz_frac(red, s, DO); fmpq_out(0, red); }
        else {
            fmpz *res = is("chain") ? x[0] : r;
            for (long i = 0; i < N; i++) { fmpq_set_fmpz_frac(red + i, res + i, DO); fmpq_out(i, red + i); }
        }
        uint64_t t3 = now();
        record(t0, t1, t2, t3);
    }
    write_out();
    print_times();
    printf(" Dbits=%lu\n", (unsigned long)fmpz_bits(D));
}

int main(int argc, char **argv) {
    flint_set_num_threads(1);  /* single-threaded, recorded in every output line */
    if (argc == 2 && !strcmp(argv[1], "--version")) {
        printf("gmp=%s flint=%s flint_threads=%d\n", gmp_version, FLINT_VERSION, flint_get_num_threads());
        return 0;
    }
    if (argc < 6) { fprintf(stderr, "usage: m3_bench contender job indir outfile reps [K | opsname]\n"); return 2; }
    CON = argv[1]; JOB = argv[2]; DIR = argv[3]; OUT = argv[4]; REPS = atoi(argv[5]);
    if (REPS < 1) { fprintf(stderr, "reps must be at least 1\n"); return 2; }
    /* every contender: freed memory is kept by malloc, so later reps reuse it (as the lib's own bench does) */
    mallopt(M_MMAP_THRESHOLD, 1 << 30);
    mallopt(M_TRIM_THRESHOLD, 1 << 30);
    if (is("ops")) {
        if (argc < 7) { fprintf(stderr, "ops needs an operation list name\n"); return 2; }
        read_arr("v", &V);
        NV = V.n;
        read_ops(argv[6]);
        NOUT = T;
    } else {
        if (is("sum")) NIN = 1;
        else if (is("fma")) NIN = 3;
        else if (is("chain")) { NIN = 3; K = argc > 6 ? atoi(argv[6]) : 0; if (K < 1) { fprintf(stderr, "chain needs K >= 1\n"); return 2; } }
        else { fprintf(stderr, "unknown job %s\n", JOB); return 2; }
        const char *names[3] = {"a", "b", "c"};
        for (int j = 0; j < NIN; j++) read_arr(names[j], &IN[j]);
        N = IN[0].n;
        for (int j = 1; j < NIN; j++) if (IN[j].n != N) { fprintf(stderr, "inputs of different lengths\n"); return 2; }
        NOUT = is("sum") ? 1 : N;
    }
    ON = xmalloc(8 * (size_t)NOUT);
    OD = xmalloc(8 * (size_t)NOUT);
    if (!strcmp(CON, "pg")) run_pg(1);
    else if (!strcmp(CON, "pg-v3")) run_pg(0);
    else if (!strcmp(CON, "mpq")) run_mpq();
    else if (!strcmp(CON, "fmpq")) run_fmpq(0);
    else if (!strcmp(CON, "fmpq-am")) run_fmpq(1);
    else if (!strcmp(CON, "fz")) run_fz();
    else { fprintf(stderr, "unknown contender %s\n", CON); return 2; }
    return 0;
}
