/* The contenders for pg_bench.py, one exact job per invocation.
 *
 *   pg_bench <contender> <job> <indir> <outfile> <reps> [K]
 *
 * contenders: pg (the prototype; entry reduces), pgu (pg, documented unreduced: no entry reduction, uniform arrays
 *             kept uniform), vec (checked int64 numerators over one shared denominator, unreduced), mpq (GMP mpq_t
 *             per element), fmpq (FLINT fmpq per element, fma as fmpq_mul + fmpq_add), fmpq-am (fma as fmpq_set +
 *             fmpq_addmul), fz (FLINT fmpz numerators over one shared denominator; its reduction to lowest terms is
 *             timed separately as 'red')
 * jobs:       sum (column sum of a), fma (a*b + c elementwise), add / mul (a op b, single values), chain (x <- x*b + c,
 *             K times, x starting at a)
 * inputs:     <indir>/{a,b,c}.bin: int64 n, then n int64 numerators, then n uint64 denominators
 * stdout:     conv=<ns> decomp=<ns> op=<ns> red=<ns> total=<ns> ..., the minimum of each phase over the reps, and
 *             total = the minimum over reps of that rep's conv + decomp + op (red excluded).  The reps run in one
 *             warm process; each re-enters the values from the raw inputs (pg also resets its graph).
 * outfile:    one "num den" line per result, written after the last rep (untimed)
 * exit 3:     unable (vec: the shared denominator or a numerator does not fit int64)
 */
#define _POSIX_C_SOURCE 200809L
#include <flint/flint.h>
#include <flint/fmpq.h>
#include <flint/fmpq_vec.h>
#include <flint/fmpz.h>
#include <flint/fmpz_vec.h>
#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pg.h"

typedef struct { long n; int64_t *num; uint64_t *den; } in_arr;

static const char *CON, *JOB, *DIR, *OUT;
static int REPS, K;
static in_arr IA, IB, IC;
static int NIN;  /* how many input arrays the job uses */
static long N;

static uint64_t now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (uint64_t)t.tv_sec * 1000000000ull + t.tv_nsec; }

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

static uint64_t best_conv = UINT64_MAX, best_dec = UINT64_MAX, best_op = UINT64_MAX, best_tot = UINT64_MAX, best_red = 0;
static void record4(uint64_t conv, uint64_t dec, uint64_t op) {
    if (conv < best_conv) best_conv = conv;
    if (dec < best_dec) best_dec = dec;
    if (op < best_op) best_op = op;
    if (conv + dec + op < best_tot) best_tot = conv + dec + op;
}
static void record(uint64_t t0, uint64_t t1, uint64_t t2, uint64_t t3) { record4(t1 - t0, t2 - t1, t3 - t2); }
static void unable(const char *why) { printf("unable %s\n", why); exit(3); }
static int is(const char *j) { return strcmp(JOB, j) == 0; }
static FILE *out_open(void) { FILE *f = fopen(OUT, "w"); if (!f) { perror(OUT); exit(1); } return f; }

/* ------------------------------------------------------------------ pg */

static void pg_print(FILE *f, pg_val v) {
    if (v.gid == PG_BAD) fprintf(f, "0 0\n");
    else fprintf(f, "%lld %llu\n", (long long)v.num, (unsigned long long)v.den);
}

static void run_pg(int keep_uniform) {
    pg_init();
    pg_keep_uniform = keep_uniform;
    int reduce_entry = !keep_uniform;  /* pg reduces at entry (as mpq_canonicalize / fmpq_set_si do); pgu does not */
    uint64_t ovf = 0, splits = 0;
    if (is("add") || is("mul")) {  /* single values */
        pg_val *va = malloc(sizeof(pg_val) * N), *vb = malloc(sizeof(pg_val) * N), *vr = malloc(sizeof(pg_val) * N);
        int add = is("add");
        for (int rep = 0; rep < REPS; rep++) {
            pg_reset(); pg_overflows = 0;
            pg_register_pool(va, N); pg_register_pool(vb, N);
            uint64_t t0 = now();
            for (long i = 0; i < N; i++) { va[i] = pg_from(IA.num[i], IA.den[i]); vb[i] = pg_from(IB.num[i], IB.den[i]); }
            uint64_t t1 = now();
            splits = pg_decompose_all();
            uint64_t t2 = now();
            if (reduce_entry) for (long i = 0; i < N; i++) { va[i] = pg_reduce(va[i]); vb[i] = pg_reduce(vb[i]); }
            uint64_t t3 = now();
            if (add) for (long i = 0; i < N; i++) vr[i] = pg_add(va[i], vb[i]);
            else for (long i = 0; i < N; i++) vr[i] = pg_mul(va[i], vb[i]);
            uint64_t t4 = now();
            record4((t1 - t0) + (t3 - t2), t2 - t1, t4 - t3);
            ovf = pg_overflows;
        }
        FILE *f = out_open();
        for (long i = 0; i < N; i++) pg_print(f, vr[i]);
        fclose(f);
    } else {
        pg_arr A, B, C, R;
        pg_arr_alloc(&A, N);
        if (NIN > 1) { pg_arr_alloc(&B, N); pg_arr_alloc(&C, N); pg_arr_alloc(&R, N); }
        pg_val s = {0};
        for (int rep = 0; rep < REPS; rep++) {
            pg_reset(); pg_overflows = 0;
            uint64_t t0 = now();
            pg_arr_enter(&A, IA.num, IA.den);
            if (NIN > 1) { pg_arr_enter(&B, IB.num, IB.den); pg_arr_enter(&C, IC.num, IC.den); }
            uint64_t t1 = now();
            splits = pg_decompose_all();
            uint64_t t2 = now();
            if (reduce_entry) { pg_arr_reduce(&A); if (NIN > 1) { pg_arr_reduce(&B); pg_arr_reduce(&C); } }
            uint64_t t3 = now();
            if (is("sum")) s = pg_arr_sum(&A);
            else if (is("fma")) pg_arr_fma(&R, &A, &B, &C);
            else for (int k = 0; k < K; k++) pg_arr_fma(&A, &A, &B, &C);  /* chain, in place */
            uint64_t t4 = now();
            record4((t1 - t0) + (t3 - t2), t2 - t1, t4 - t3);
            ovf = pg_overflows;
        }
        if (is("sum") && s.gid == PG_BAD) {
            printf("unable common denominator overflows 64 bits (%llu overflows detected)\n", (unsigned long long)ovf);
            exit(3);
        }
        FILE *f = out_open();
        if (is("sum")) pg_print(f, s);
        else { pg_arr *X = is("fma") ? &R : &A; for (long i = 0; i < N; i++) pg_print(f, pg_arr_get(X, i)); }
        fclose(f);
    }
    uint32_t alive = 0;
    for (uint32_t i = 0; i < pg_nnodes; i++) alive += pg_nodes[i].alive;
    printf("conv=%llu decomp=%llu op=%llu red=0 total=%llu ovf=%llu groups=%u nodes=%u splits=%llu table_bytes=%zu\n",
           (unsigned long long)best_conv, (unsigned long long)best_dec, (unsigned long long)best_op,
           (unsigned long long)best_tot, (unsigned long long)ovf, pg_ngroups, alive, (unsigned long long)splits,
           pg_table_bytes());
}

/* ------------------------------------------------------------------ shared-denominator helpers */

static uint64_t gcdu(uint64_t a, uint64_t b) { while (b) { uint64_t t = a % b; a = b; b = t; } return a; }
static in_arr *ins[3];

/* ------------------------------------------------------------------ vec: checked int64 over one denominator */

static void run_vec(void) {
    int64_t *x[3]; for (int j = 0; j < NIN; j++) x[j] = malloc(8 * N);
    int64_t *r = malloc(8 * N);
    static uint64_t cof[65536];
    static uint8_t seen[65536];
    uint64_t D = 1, DO = 1;  /* D: the shared denominator; DO: the output denominator */
    int64_t s = 0;
    for (int rep = 0; rep < REPS; rep++) {
        memset(seen, 0, sizeof seen); memset(cof, 0, sizeof cof);
        uint64_t t0 = now();
        D = 1;  /* the common denominator of all values, computed up front */
        for (int j = 0; j < NIN; j++)
            for (long i = 0; i < N; i++) {
                uint64_t d = ins[j]->den[i];
                if (d < 65536) { if (seen[d]) continue; seen[d] = 1; }
                uint64_t g = gcdu(D, d);
                if (__builtin_mul_overflow(D / g, d, &D) || (D >> 63)) unable("shared denominator overflows 64 bits");
            }
        for (int j = 0; j < NIN; j++)
            for (long i = 0; i < N; i++) {
                uint64_t d = ins[j]->den[i], c;
                if (d < 65536) { if (!cof[d]) cof[d] = D / d; c = cof[d]; } else c = D / d;
                if (__builtin_mul_overflow(ins[j]->num[i], (int64_t)c, &x[j][i])) unable("numerator overflows int64");
            }
        uint64_t t1 = now();
        int o = 0;
        if (is("sum")) {
            __int128 t = 0;
            for (long i = 0; i < N; i++) t += x[0][i];
            if (t > INT64_MAX || t < INT64_MIN) unable("sum overflows int64");
            s = (int64_t)t; DO = D;
        } else if (is("fma")) {
            if (__builtin_mul_overflow(D, D, &DO) || (DO >> 63)) unable("D^2 overflows 64 bits");
            for (long i = 0; i < N; i++) {
                int64_t p, q;
                o |= (int)__builtin_mul_overflow(x[0][i], x[1][i], &p) | (int)__builtin_mul_overflow(x[2][i], (int64_t)D, &q);
                o |= (int)__builtin_add_overflow(p, q, &r[i]);
            }
        } else if (is("add")) {
            for (long i = 0; i < N; i++) o |= (int)__builtin_add_overflow(x[0][i], x[1][i], &r[i]);
            DO = D;
        } else if (is("mul")) {
            if (__builtin_mul_overflow(D, D, &DO) || (DO >> 63)) unable("D^2 overflows 64 bits");
            for (long i = 0; i < N; i++) o |= (int)__builtin_mul_overflow(x[0][i], x[1][i], &r[i]);
        } else {  /* chain: x over S; x*b + c*S over S*D */
            uint64_t S = D;
            for (int k = 0; k < K; k++) {
                for (long i = 0; i < N; i++) {
                    int64_t p, q;
                    o |= (int)__builtin_mul_overflow(x[0][i], x[1][i], &p) | (int)__builtin_mul_overflow(x[2][i], (int64_t)S, &q);
                    o |= (int)__builtin_add_overflow(p, q, &x[0][i]);
                }
                if (__builtin_mul_overflow(S, D, &S) || (S >> 63)) unable("scale overflows 64 bits");
            }
            DO = S;
        }
        uint64_t t2 = now();
        if (o) unable("numerator overflows int64");
        record(t0, t1, t1, t2);
    }
    FILE *f = out_open();
    if (is("sum")) fprintf(f, "%lld %llu\n", (long long)s, (unsigned long long)DO);
    else {
        int64_t *res = is("chain") ? x[0] : r;
        for (long i = 0; i < N; i++) fprintf(f, "%lld %llu\n", (long long)res[i], (unsigned long long)DO);
    }
    fclose(f);
    printf("conv=%llu decomp=0 op=%llu red=0 total=%llu ovf=0 D=%llu\n", (unsigned long long)best_conv,
           (unsigned long long)best_op, (unsigned long long)best_tot, (unsigned long long)D);
}

/* ------------------------------------------------------------------ fz: FLINT fmpz over one denominator */

static void q_print(FILE *f, const fmpq_t q) {
    fmpz_fprint(f, fmpq_numref(q)); fputc(' ', f); fmpz_fprint(f, fmpq_denref(q)); fputc('\n', f);
}

static void run_fz(void) {
    fmpz *x[3]; for (int j = 0; j < NIN; j++) x[j] = _fmpz_vec_init(N);
    fmpz *r = _fmpz_vec_init(N);
    fmpz *cof = _fmpz_vec_init(65536);
    long nout = is("sum") ? 1 : N;
    fmpq *red = _fmpq_vec_init(nout);  /* the results in lowest terms: the timed 'red' phase */
    static uint8_t seen[65536];
    fmpz_t D, DO, s, S, t;
    fmpz_init(D); fmpz_init(DO); fmpz_init(s); fmpz_init(S); fmpz_init(t);
    for (int rep = 0; rep < REPS; rep++) {
        memset(seen, 0, sizeof seen);
        for (int d = 0; d < 65536; d++) fmpz_zero(cof + d);
        uint64_t t0 = now();
        fmpz_one(D);
        for (int j = 0; j < NIN; j++)
            for (long i = 0; i < N; i++) {
                uint64_t d = ins[j]->den[i];
                if (d < 65536) { if (seen[d]) continue; seen[d] = 1; }
                fmpz_set_ui(t, d);
                fmpz_lcm(D, D, t);
            }
        for (int j = 0; j < NIN; j++)
            for (long i = 0; i < N; i++) {
                uint64_t d = ins[j]->den[i];
                if (d < 65536) {
                    if (fmpz_is_zero(cof + d)) fmpz_divexact_ui(cof + d, D, d);
                    fmpz_mul_si(x[j] + i, cof + d, ins[j]->num[i]);
                } else {
                    fmpz_divexact_ui(t, D, d);
                    fmpz_mul_si(x[j] + i, t, ins[j]->num[i]);
                }
            }
        uint64_t t1 = now();
        if (is("sum")) { _fmpz_vec_sum(s, x[0], N); fmpz_set(DO, D); }
        else if (is("fma")) {
            for (long i = 0; i < N; i++) { fmpz_mul(r + i, x[0] + i, x[1] + i); fmpz_addmul(r + i, x[2] + i, D); }
            fmpz_mul(DO, D, D);
        } else if (is("add")) { for (long i = 0; i < N; i++) fmpz_add(r + i, x[0] + i, x[1] + i); fmpz_set(DO, D); }
        else if (is("mul")) { for (long i = 0; i < N; i++) fmpz_mul(r + i, x[0] + i, x[1] + i); fmpz_mul(DO, D, D); }
        else {
            fmpz_set(S, D);
            for (int k = 0; k < K; k++) {
                for (long i = 0; i < N; i++) { fmpz_mul(x[0] + i, x[0] + i, x[1] + i); fmpz_addmul(x[0] + i, x[2] + i, S); }
                fmpz_mul(S, S, D);
            }
            fmpz_set(DO, S);
        }
        uint64_t t2 = now();
        if (is("sum")) fmpq_set_fmpz_frac(red, s, DO);
        else { fmpz *res = is("chain") ? x[0] : r; for (long i = 0; i < N; i++) fmpq_set_fmpz_frac(red + i, res + i, DO); }
        uint64_t t3 = now();
        record(t0, t1, t1, t2);
        if (rep == 0 || t3 - t2 < best_red) best_red = t3 - t2;
    }
    FILE *f = out_open();
    for (long i = 0; i < nout; i++) q_print(f, red + i);
    fclose(f);
    printf("conv=%llu decomp=0 op=%llu red=%llu total=%llu ovf=0 Dbits=%lu\n", (unsigned long long)best_conv,
           (unsigned long long)best_op, (unsigned long long)best_red, (unsigned long long)best_tot,
           (unsigned long)fmpz_bits(D));
}

/* ------------------------------------------------------------------ mpq: GMP per element */

static void run_mpq(void) {
    mpq_t *x[3]; for (int j = 0; j < NIN; j++) { x[j] = malloc(sizeof(mpq_t) * N); for (long i = 0; i < N; i++) mpq_init(x[j][i]); }
    mpq_t *r = malloc(sizeof(mpq_t) * N); for (long i = 0; i < N; i++) mpq_init(r[i]);
    mpq_t s; mpq_init(s);
    for (int rep = 0; rep < REPS; rep++) {
        uint64_t t0 = now();
        for (int j = 0; j < NIN; j++)
            for (long i = 0; i < N; i++) { mpq_set_si(x[j][i], ins[j]->num[i], ins[j]->den[i]); mpq_canonicalize(x[j][i]); }
        uint64_t t1 = now();
        if (is("sum")) { mpq_set_ui(s, 0, 1); for (long i = 0; i < N; i++) mpq_add(s, s, x[0][i]); }
        else if (is("fma")) for (long i = 0; i < N; i++) { mpq_mul(r[i], x[0][i], x[1][i]); mpq_add(r[i], r[i], x[2][i]); }
        else if (is("add")) for (long i = 0; i < N; i++) mpq_add(r[i], x[0][i], x[1][i]);
        else if (is("mul")) for (long i = 0; i < N; i++) mpq_mul(r[i], x[0][i], x[1][i]);
        else for (int k = 0; k < K; k++) for (long i = 0; i < N; i++) { mpq_mul(x[0][i], x[0][i], x[1][i]); mpq_add(x[0][i], x[0][i], x[2][i]); }
        uint64_t t2 = now();
        record(t0, t1, t1, t2);
    }
    FILE *f = out_open();
    if (is("sum")) gmp_fprintf(f, "%Zd %Zd\n", mpq_numref(s), mpq_denref(s));
    else { mpq_t *res = is("chain") ? x[0] : r; for (long i = 0; i < N; i++) gmp_fprintf(f, "%Zd %Zd\n", mpq_numref(res[i]), mpq_denref(res[i])); }
    fclose(f);
    printf("conv=%llu decomp=0 op=%llu red=0 total=%llu ovf=0\n", (unsigned long long)best_conv, (unsigned long long)best_op,
           (unsigned long long)best_tot);
}

/* ------------------------------------------------------------------ fmpq: FLINT per element */

static void run_fmpq(int addmul) {
    fmpq *x[3]; for (int j = 0; j < NIN; j++) x[j] = _fmpq_vec_init(N);
    fmpq *r = _fmpq_vec_init(N);
    fmpq_t s; fmpq_init(s);
    for (int rep = 0; rep < REPS; rep++) {
        uint64_t t0 = now();
        for (int j = 0; j < NIN; j++) for (long i = 0; i < N; i++) fmpq_set_si(x[j] + i, ins[j]->num[i], ins[j]->den[i]);
        uint64_t t1 = now();
        if (is("sum")) { fmpq_zero(s); for (long i = 0; i < N; i++) fmpq_add(s, s, x[0] + i); }
        else if (is("fma") && addmul) for (long i = 0; i < N; i++) { fmpq_set(r + i, x[2] + i); fmpq_addmul(r + i, x[0] + i, x[1] + i); }
        else if (is("fma")) for (long i = 0; i < N; i++) { fmpq_mul(r + i, x[0] + i, x[1] + i); fmpq_add(r + i, r + i, x[2] + i); }
        else if (is("add")) for (long i = 0; i < N; i++) fmpq_add(r + i, x[0] + i, x[1] + i);
        else if (is("mul")) for (long i = 0; i < N; i++) fmpq_mul(r + i, x[0] + i, x[1] + i);
        else for (int k = 0; k < K; k++) for (long i = 0; i < N; i++) { fmpq_mul(x[0] + i, x[0] + i, x[1] + i); fmpq_add(x[0] + i, x[0] + i, x[2] + i); }
        uint64_t t2 = now();
        record(t0, t1, t1, t2);
    }
    FILE *f = out_open();
    if (is("sum")) { fmpz_fprint(f, fmpq_numref(s)); fputc(' ', f); fmpz_fprint(f, fmpq_denref(s)); fputc('\n', f); }
    else {
        fmpq *res = is("chain") ? x[0] : r;
        for (long i = 0; i < N; i++) { fmpz_fprint(f, fmpq_numref(res + i)); fputc(' ', f); fmpz_fprint(f, fmpq_denref(res + i)); fputc('\n', f); }
    }
    fclose(f);
    printf("conv=%llu decomp=0 op=%llu red=0 total=%llu ovf=0\n", (unsigned long long)best_conv, (unsigned long long)best_op,
           (unsigned long long)best_tot);
}

int main(int argc, char **argv) {
    if (argc < 6) { fprintf(stderr, "usage: pg_bench contender job indir outfile reps [K]\n"); return 2; }
    CON = argv[1]; JOB = argv[2]; DIR = argv[3]; OUT = argv[4]; REPS = atoi(argv[5]); K = argc > 6 ? atoi(argv[6]) : 0;
    if (is("sum")) NIN = 1;
    else if (is("add") || is("mul")) NIN = 2;
    else if (is("fma") || is("chain")) NIN = 3;
    else { fprintf(stderr, "unknown job %s\n", JOB); return 2; }
    read_arr("a", &IA); ins[0] = &IA;
    if (NIN > 1) { read_arr("b", &IB); ins[1] = &IB; }
    if (NIN > 2) { read_arr("c", &IC); ins[2] = &IC; }
    N = IA.n;
    if (!strcmp(CON, "pg")) run_pg(0);
    else if (!strcmp(CON, "pgu")) run_pg(1);
    else if (!strcmp(CON, "vec")) run_vec();
    else if (!strcmp(CON, "fz")) run_fz();
    else if (!strcmp(CON, "mpq")) run_mpq();
    else if (!strcmp(CON, "fmpq")) run_fmpq(0);
    else if (!strcmp(CON, "fmpq-am")) run_fmpq(1);
    else { fprintf(stderr, "unknown contender %s\n", CON); return 2; }
    return 0;
}
