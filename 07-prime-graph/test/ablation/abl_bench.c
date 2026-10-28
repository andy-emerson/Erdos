/* The ablation driver: one exact job per invocation, for the strategy this binary was built for (-DVARIANT=...).
 *
 *   abl_<variant> <job> <indir> <outfile> <reps> [K]
 *
 * variants:   VAR_V0 (the prototype pg.c, unchanged, with grouping), VAR_V0P (the control: grouping removed), VAR_V1 ..
 *             VAR_V5 (the control plus one idea each); see ablation_bench.py
 * jobs:       sum (column sum of a), fma (a*b + c), add / mul (a op b, single values), chain (x <- x*b + c, K times,
 *             x starting at a), fma2 ((a*b + c)*e + f)
 * inputs:     <indir>/{a,b,c,e,f}.bin: int64 n, then n int64 numerators, then n uint64 denominators
 * stdout:     entry=<ns> decomp=<ns> op=<ns> red=<ns> total=<ns> ... : the minimum of each phase over the reps;
 *             entry includes decomp; total = the minimum over reps of that rep's entry + op + red.  The reps run in one
 *             warm process and each re-enters the values from the raw inputs (the node table is reset).
 * outfile:    one "num den" line per result ("0 0": an overflow was detected), written after the last rep (untimed)
 * exit 3:     unable (a column sum whose common denominator or numerator overflows)
 * env:        AB_AUDIT=1 runs the strategy's structural audit after the last rep and prints audit=ok|FAIL:<why>;
 *             AB_CORRUPT=value|unreduced corrupts the first output line (to show the harness's checks catch it);
 *             AB_CORRUPT=den|xi|rc corrupts the structure before the audit (to show the audit catches it).
 */
#define _POSIX_C_SOURCE 200809L
#define VAR_V0 0
#define VAR_V0P 1
#define VAR_V1 2
#define VAR_V2 3
#define VAR_V3 4
#define VAR_V4 5
#define VAR_V5 6
#ifndef VARIANT
#error "build with -DVARIANT=VAR_V0 .. VAR_V5"
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>

#include "../pg.h"

#if VARIANT != VAR_V0
#include "ab_core.c"
#if VARIANT == VAR_V3
#include "ab_v3.c"
#elif VARIANT == VAR_V5
#include "ab_v5.c"
#else
#include "ab_l1.c"
#endif
#endif

typedef struct { long n; int64_t *num; uint64_t *den; } in_arr;

static const char *JOB, *DIR, *OUT;
static int REPS, K, NIN;
static long N;
static in_arr IN[5];
static const char *NAMES[5] = {"a", "b", "c", "e", "f"};

static uint64_t now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (uint64_t)t.tv_sec * 1000000000ull + t.tv_nsec; }
static int is(const char *j) { return strcmp(JOB, j) == 0; }
static void read_arr(const char *name, in_arr *a) {
    char p[4096];
    snprintf(p, sizeof p, "%s/%s.bin", DIR, name);
    FILE *f = fopen(p, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", p); exit(1); }
    int64_t n;
    if (fread(&n, 8, 1, f) != 1) exit(1);
    a->n = n;
    a->num = malloc(8 * n + 8); a->den = malloc(8 * n + 8);
    if (fread(a->num, 8, n, f) != (size_t)n || fread(a->den, 8, n, f) != (size_t)n) { fprintf(stderr, "short %s\n", p); exit(1); }
    fclose(f);
}

static uint64_t b_entry = UINT64_MAX, b_dec = UINT64_MAX, b_op = UINT64_MAX, b_red = UINT64_MAX, b_tot = UINT64_MAX;
static void record(uint64_t entry, uint64_t dec, uint64_t op, uint64_t red) {  /* entry includes dec */
    if (entry < b_entry) b_entry = entry;
    if (dec < b_dec) b_dec = dec;
    if (op < b_op) b_op = op;
    if (red < b_red) b_red = red;
    if (entry + op + red < b_tot) b_tot = entry + op + red;
}
static void print_times(void) {
    printf("entry=%llu decomp=%llu op=%llu red=%llu total=%llu", (unsigned long long)b_entry, (unsigned long long)b_dec,
           (unsigned long long)b_op, (unsigned long long)b_red, (unsigned long long)b_tot);
}
/* Peak resident set of this process image: VmHWM (ru_maxrss would carry the forking parent's peak across exec). */
static long maxrss_kb(void) {
    FILE *f = fopen("/proc/self/status", "r");
    char line[256];
    long kb = -1;
    while (f && fgets(line, sizeof line, f))
        if (!strncmp(line, "VmHWM:", 6)) { kb = atol(line + 6); break; }
    if (f) fclose(f);
    if (kb < 0) { struct rusage u; getrusage(RUSAGE_SELF, &u); kb = u.ru_maxrss; }
    return kb;
}

static FILE *OUTF;
static int corrupted;
static void emit(int64_t num, uint64_t den) {  /* one output line; AB_CORRUPT=value|unreduced alters the first */
    const char *c = getenv("AB_CORRUPT");
    if (c && !corrupted && den) {
        if (!strcmp(c, "value")) { num += 1; corrupted = 1; }
        else if (!strcmp(c, "unreduced") && den < (1ull << 62) && num > INT64_MIN / 2 && num < INT64_MAX / 2) {
            num *= 2; den *= 2; corrupted = 1;
        }
    }
    if (!den) fprintf(OUTF, "0 0\n");
    else fprintf(OUTF, "%lld %llu\n", (long long)num, (unsigned long long)den);
}
static void out_open(void) { OUTF = fopen(OUT, "w"); if (!OUTF) { perror(OUT); exit(1); } }

#if VARIANT == VAR_V0
/* ------------------------------------------------------------------ V0: the prototype, as bench.c runs 'pg' */

static void run_v0(void) {
    pg_init();
    pg_keep_uniform = 0;
    uint64_t ovf = 0, splits = 0;
    size_t rep_bytes = 0;
    if (is("add") || is("mul")) {  /* single values: as bench.c */
        pg_val *va = malloc(sizeof(pg_val) * N), *vb = malloc(sizeof(pg_val) * N), *vr = malloc(sizeof(pg_val) * N);
        int add = is("add");
        for (int rep = 0; rep < REPS; rep++) {
            pg_reset(); pg_overflows = 0;
            pg_register_pool(va, N); pg_register_pool(vb, N);
            uint64_t t0 = now();
            for (long i = 0; i < N; i++) { va[i] = pg_from(IN[0].num[i], IN[0].den[i]); vb[i] = pg_from(IN[1].num[i], IN[1].den[i]); }
            uint64_t t1 = now();
            splits = pg_decompose_all();
            uint64_t t2 = now();
            for (long i = 0; i < N; i++) { va[i] = pg_reduce(va[i]); vb[i] = pg_reduce(vb[i]); }
            uint64_t t3 = now();
            if (add) for (long i = 0; i < N; i++) vr[i] = pg_add(va[i], vb[i]);
            else for (long i = 0; i < N; i++) vr[i] = pg_mul(va[i], vb[i]);
            uint64_t t4 = now();
            record(t3 - t0, t2 - t1, t4 - t3, 0);
            ovf = pg_overflows;
        }
        rep_bytes = pg_table_bytes() + 3 * sizeof(pg_val) * (size_t)N;
        out_open();
        for (long i = 0; i < N; i++) emit(vr[i].gid == PG_BAD ? 0 : vr[i].num, vr[i].gid == PG_BAD ? 0 : vr[i].den);
        fclose(OUTF);
    } else {
        pg_arr X[5], R, T;
        int na = 0;
        for (int j = 0; j < NIN; j++) { pg_arr_alloc(&X[j], N); na++; }
        if (!is("sum")) { pg_arr_alloc(&R, N); na++; }
        if (is("fma2")) { pg_arr_alloc(&T, N); na++; }
        pg_val s = {0};
        for (int rep = 0; rep < REPS; rep++) {
            pg_reset(); pg_overflows = 0;
            uint64_t t0 = now();
            for (int j = 0; j < NIN; j++) pg_arr_enter(&X[j], IN[j].num, IN[j].den);
            uint64_t t1 = now();
            splits = pg_decompose_all();
            uint64_t t2 = now();
            for (int j = 0; j < NIN; j++) pg_arr_reduce(&X[j]);
            uint64_t t3 = now();
            if (is("sum")) s = pg_arr_sum(&X[0]);
            else if (is("fma")) pg_arr_fma(&R, &X[0], &X[1], &X[2]);
            else if (is("chain")) for (int k = 0; k < K; k++) pg_arr_fma(&X[0], &X[0], &X[1], &X[2]);  /* in place */
            else { pg_arr_fma(&T, &X[0], &X[1], &X[2]); pg_arr_fma(&R, &T, &X[3], &X[4]); }
            uint64_t t4 = now();
            record(t3 - t0, t2 - t1, t4 - t3, 0);
            ovf = pg_overflows;
        }
        rep_bytes = pg_table_bytes() + (size_t)na * 12 * (size_t)N;
        if (is("sum") && s.gid == PG_BAD) {
            printf("unable common denominator overflows 64 bits (%llu overflows detected)\n", (unsigned long long)ovf);
            exit(3);
        }
        out_open();
        if (is("sum")) emit(s.num, s.den);
        else {
            pg_arr *Y = is("chain") ? &X[0] : &R;
            for (long i = 0; i < N; i++) {
                pg_val v = pg_arr_get(Y, i);
                emit(v.gid == PG_BAD ? 0 : v.num, v.gid == PG_BAD ? 0 : v.den);
            }
        }
        fclose(OUTF);
    }
    uint32_t alive = 0;
    for (uint32_t i = 0; i < pg_nnodes; i++) alive += pg_nodes[i].alive;
    print_times();
    printf(" ovf=%llu splits=%llu nodes=%u groups=%u rep_bytes=%zu maxrss_kb=%ld", (unsigned long long)ovf,
           (unsigned long long)splits, alive, pg_ngroups, rep_bytes, maxrss_kb());
    if (getenv("AB_AUDIT")) printf(" audit=ok");  /* V0 has no audit of its own: test_pg.py covers it */
    printf("\n");
}

#else
/* ------------------------------------------------------------------ V0' and V1..V5 */

static void run_ab(void) {
    pg_init();  /* the prime table used by pg_factor */
    ab_init();
    int single = is("add") || is("mul");
    ab_arr *X[5], *R = ab_new(is("sum") ? 1 : (size_t)N), *T = NULL;
    for (int j = 0; j < NIN; j++) X[j] = ab_new(N);
    if (is("chain") || is("fma2")) T = ab_new(N);
    ab_arr *out = R;
    uint64_t splits = 0, D_entry = 0;
    for (int rep = 0; rep < REPS; rep++) {
        ab_reset();
        ab_overflows = 0;
        uint64_t t0 = now();
        ab_enter_begin();
        for (int j = 0; j < NIN; j++) ab_enter(X[j], IN[j].num, IN[j].den, single);
        uint64_t t1 = now();
        splits = ab_decompose();
        uint64_t t2 = now();
        ab_enter_end();
        uint64_t t3 = now();
#if VARIANT == VAR_V3
        D_entry = X[0]->D;  /* the first input's array denominator, as built at entry (untimed read) */
#endif
        if (is("sum")) { ab_sum(R, X[0]); out = R; }
        else if (is("add")) { ab_add(R, X[0], X[1]); out = R; }
        else if (is("mul")) { ab_mul(R, X[0], X[1]); out = R; }
        else if (is("fma")) { ab_fma(R, X[0], X[1], X[2]); out = R; }
        else if (is("chain")) {  /* x <- x*b + c into a second array, then swap: results never alias inputs */
            for (int k = 0; k < K; k++) { ab_fma(T, X[0], X[1], X[2]); ab_arr *s = X[0]; X[0] = T; T = s; }
            out = X[0];
        } else { ab_fma(T, X[0], X[1], X[2]); ab_fma(R, T, X[3], X[4]); out = R; }
        uint64_t t4 = now();
        ab_final(out);
        uint64_t t5 = now();
        record(t3 - t0, t2 - t1, t4 - t3, t5 - t4);
    }
    int64_t num;
    uint64_t den;
    if (is("sum")) {
        ab_get(out, 0, &num, &den);
        if (!den) {
            printf("unable common denominator or numerator overflows 64 bits (%llu overflows detected)\n",
                   (unsigned long long)ab_overflows);
            exit(3);
        }
    }
    const char *audit = NULL;
    int audited = getenv("AB_AUDIT") != NULL;
    if (audited) {  /* the arrays written in the last rep: the inputs, the result and any intermediate */
        ab_arr *L[8];
        int nl = 0;
        L[nl++] = out;
        for (int j = 0; j < NIN; j++) L[nl++] = X[j];
        if (T && T != out) L[nl++] = T;
        if (R != out) L[nl++] = R;
        if (is("chain")) nl--;  /* after a chain, R was never written */
        audit = ab_audit(L, nl, getenv("AB_CORRUPT"));
    }
    out_open();
    for (size_t i = 0; i < out->n; i++) { ab_get(out, i, &num, &den); emit(num, den); }
    fclose(OUTF);
    print_times();
    (void)D_entry;
    printf(" ovf=%llu splits=%llu nodes=%u rep_bytes=%zu maxrss_kb=%ld", (unsigned long long)ab_overflows,
           (unsigned long long)splits, ab_live_nodes(), mem_peak, maxrss_kb());
#if VARIANT == VAR_V3
    printf(" exc_out=%zu D=%llu", is("sum") ? (size_t)0 : out->nx, (unsigned long long)D_entry);
#endif
    ab_stats(stdout);
    if (audited) printf(" audit=%s%s", audit ? "FAIL:" : "ok", audit ? audit : "");
    printf("\n");
}
#endif

int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: abl_<variant> job indir outfile reps [K]\n"); return 2; }
    JOB = argv[1]; DIR = argv[2]; OUT = argv[3]; REPS = atoi(argv[4]); K = argc > 5 ? atoi(argv[5]) : 0;
    if (is("sum")) NIN = 1;
    else if (is("add") || is("mul")) NIN = 2;
    else if (is("fma") || is("chain")) NIN = 3;
    else if (is("fma2")) NIN = 5;
    else { fprintf(stderr, "unknown job %s\n", JOB); return 2; }
    for (int j = 0; j < NIN; j++) read_arr(NAMES[j], &IN[j]);
    N = IN[0].n;
    for (int j = 1; j < NIN; j++) if (IN[j].n != N) { fprintf(stderr, "length mismatch\n"); return 2; }
#if VARIANT == VAR_V0
    run_v0();
#else
    run_ab();
#endif
    return 0;
}
