/* ovh.c: the time and memory overhead of a variant's production checks on two normal workloads (no fault).
 *
 *   ovh_X <workload> <K> <reps> [scale]     prints: ovh <variant> <workload> <K> <best seconds> <median seconds> <mem_peak bytes>
 *                                           <checks per run> <operations per run> <checksum>
 *
 * Workloads (fixed seeds, identical across variants):
 *   dec   a decimal array expression: three arrays of 20,000 decimals (denominators 10^1..10^4); 1,000 times
 *         { begin; t = a*b; r = t + c; release t; end; s = sum(r); release r, s }
 *   dens  many distinct denominators: 5,000 values with distinct denominators (products of shared small primes and
 *         some large primes, so refinements happen), then 300,000 random add/sub/mul/div operations among them, each
 *         result replacing a random value
 * The checksum (over every result read) must be equal across variants.  scale divides REPS, NV and NOPS. */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "primegraph.h"
#include "src/pgfault.h"

static uint64_t rs = 88172645463325252ull;
static uint64_t xr(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + 1e-9 * (double)t.tv_nsec;
}
static uint64_t ck;
static void mix(int64_t n, int64_t d) { ck = (ck ^ (uint64_t)n) * 0x100000001B3ull; ck = (ck ^ (uint64_t)d) * 0x100000001B3ull; }

static uint64_t nops;
static int scale = 1;  /* divides the workload sizes (smoke runs) */  /* public calls that count as operations for the K counter */

static void w_dec(pg_graph *g) {
    const int N = 20000, REPS = 1000 / scale;
    int64_t *nm = malloc(8 * N), *dn = malloc(8 * N);
    pg_array *A[3];
    for (int k = 0; k < 3; k++) {
        for (int i = 0; i < N; i++) {
            nm[i] = (int64_t)(xr() % 2000001) - 1000000;
            int64_t p = 10;
            for (int e = (int)(xr() % 4); e > 0; e--) p *= 10;
            dn[i] = p;
        }
        pg_array_from_fracs(g, nm, dn, N, &A[k]);
        nops++;
    }
    for (int r = 0; r < REPS; r++) {
        pg_array *t, *s;
        pg_value *v;
        pg_expr_begin(g);
        pg_array_mul(g, A[0], A[1], &t);
        pg_array_add(g, t, A[2], &s);
        pg_array_release(g, t);
        pg_expr_end(g);
        if (pg_array_sum(g, s, &v) == PG_OK) {
            int64_t n, d;
            pg_value_get(g, v, &n, &d);
            mix(n, d);
            pg_value_release(g, v);
        }
        pg_array_release(g, s);
        nops += 7;
    }
    for (int k = 0; k < 3; k++) pg_array_release(g, A[k]);
    free(nm);
    free(dn);
}

static void w_dens(pg_graph *g) {
    const int NV = 5000 / scale, NOPS = 300000 / scale;
    static const uint64_t P[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79,
                                 83, 89, 97, 101, 103, 107, 109, 113};
    static const uint64_t BIG[] = {65537, 1000003, 1000033, 2147483647, 4294967311ull};
    pg_value **V = calloc(NV, sizeof *V);
    for (int i = 0; i < NV; i++) {
        uint64_t d;
        do {
            d = 1;
            int k = 1 + (int)(xr() % 4);
            for (int j = 0; j < k; j++) {
                uint64_t p = P[xr() % 30];
                int e = 1 + (int)(xr() % 2);
                while (e-- && d < (1ull << 40)) d *= p;
            }
            if (xr() % 8 == 0 && d < (1ull << 30)) d *= BIG[xr() % 5];
        } while (d < 2);
        pg_value_new(g, (int64_t)(xr() % 1000) + 1, (int64_t)d, &V[i]);
        nops++;
    }
    for (int i = 0; i < NOPS; i++) {
        int a = (int)(xr() % NV), b = (int)(xr() % NV), dst = (int)(xr() % NV), op = (int)(xr() % 4);
        pg_value *r = NULL;
        pg_status s = op == 0 ? pg_add(g, V[a], V[b], &r) : op == 1 ? pg_sub(g, V[a], V[b], &r)
                    : op == 2 ? pg_mul(g, V[a], V[b], &r) : pg_div(g, V[a], V[b], &r);
        nops++;
        if (s == PG_OK && r) {
            int64_t n, d;
            pg_value_get(g, r, &n, &d);
            mix(n, d);
            if (pg_value_get(g, r, &n, &d) == PG_OK && (d > (1ll << 40) || n > (1ll << 40) || n < -(1ll << 40))) {
                pg_value_release(g, r);  /* keep the values small: replace with a fresh entry instead */
                nops++;
                pg_value_new(g, (int64_t)(xr() % 1000) + 1, (int64_t)(2 + xr() % 5000), &r);
                nops++;
            }
            pg_value_release(g, V[dst]);
            nops++;
            V[dst] = r;
        }
    }
    for (int i = 0; i < NV; i++) pg_value_release(g, V[i]);
    free(V);
}

static int cmpd(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: ovh dec|dens K reps [scale]\n"); return 2; }
    if (argc > 4) scale = atoi(argv[4]) > 0 ? atoi(argv[4]) : 1;
    const char *w = argv[1];
    uint64_t K = strtoull(argv[2], NULL, 10);
    int reps = atoi(argv[3]);
    double *t = calloc((size_t)reps, sizeof *t);
    size_t peak = 0;
    uint64_t checks = 0, ops = 0, ck0 = 0;
    static pgf_trial rec;
    for (int r = 0; r < reps; r++) {
        memset(&rec, 0, sizeof rec);
        pgf_rec = &rec;
        pgf_K = K;
        pgf_ops = 0;
        rs = 88172645463325252ull;
        ck = 1469598103934665603ull;
        nops = 0;
        pg_graph *g = pg_graph_create();
        double t0 = now();
        if (!strcmp(w, "dec")) w_dec(g);
        else w_dens(g);
        t[r] = now() - t0;
        pg_stats st;
        pg_stats_get(g, &st);
        peak = st.mem_peak;
        checks = rec.nverify;
        ops = nops;
        if (rec.nrepair || rec.halt_kind) { fprintf(stderr, "ovh: a check failed without a fault\n"); return 1; }
        if (r && ck != ck0) { fprintf(stderr, "ovh: results differ between reps\n"); return 1; }
        ck0 = ck;
        pg_graph_destroy(g);
    }
    qsort(t, (size_t)reps, sizeof *t, cmpd);
    printf("ovh %s %s %" PRIu64 " %.6f %.6f %zu %" PRIu64 " %" PRIu64 " %016" PRIx64 "\n", pgf_variant(), w, K, t[0],
           t[reps / 2], peak, checks, ops, ck0);
    return 0;
}
