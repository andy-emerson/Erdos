/* pg_bench.c: round three's jobs on the library, in the format of round three's driver (test/ablation/abl_bench.c),
 * so bench/compare.py can put the library beside the R0 and D1 binaries on the same data.
 *
 *   pg_bench <job> <indir> <outfile> <reps> [v3|v4]
 *
 * jobs:    fma (a*b + c, one pg_array_fma), sum (column sum of a)
 * inputs:  <indir>/{a,b,c}.bin: int64 n, then n int64 numerators, then n uint64 denominators (below 2^63)
 * phases:  entry  = pg_array_from_fracs for each input.  One graph serves every rep; each rep releases everything it
 *                   made (untimed), so each starts from an empty graph, as the reference re-enters its values with its
 *                   node table reset;
 *          op     = the job (v4: inside pg_expr_begin / pg_expr_end, the end's reduction included);
 *          red    = reading every result element in lowest terms (pg_array_to_fracs), as the reference's final
 *                   reduction; releases are not timed (the reference frees nothing).
 * memory:  the reference allocates its arrays once, before the reps, so its later reps write to memory already
 *          mapped.  The library allocates its arrays per operation; malloc is told to keep freed memory (mallopt)
 *          so that later reps reuse it the same way, and the minimum over reps compares the work, not page faults.
 * stdout:  entry=<ns> decomp=0 op=<ns> red=<ns> total=<ns> ... (the minimum of each phase over the reps; total is
 *          the minimum over reps of that rep's sum), then the library's statistics
 * outfile: one "num den" line per result ("0 0" for an overflow), as the reference writes it
 */
#include <inttypes.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "primegraph.h"

typedef struct { long n; int64_t *num, *den; } in_arr;

static uint64_t now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec;
}
static void read_arr(const char *dir, const char *name, in_arr *a) {
    char p[4096];
    snprintf(p, sizeof p, "%s/%s.bin", dir, name);
    FILE *f = fopen(p, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", p); exit(1); }
    int64_t n;
    if (fread(&n, 8, 1, f) != 1) exit(1);
    a->n = n;
    a->num = malloc(8 * n + 8);
    a->den = malloc(8 * n + 8);
    if (fread(a->num, 8, n, f) != (size_t)n || fread(a->den, 8, n, f) != (size_t)n) { fprintf(stderr, "short %s\n", p); exit(1); }
    fclose(f);
}

int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: pg_bench job indir outfile reps [v3|v4]\n"); return 2; }
    const char *job = argv[1], *dir = argv[2], *outp = argv[3];
    int reps = atoi(argv[4]), v4 = argc > 5 && !strcmp(argv[5], "v4"), fma = !strcmp(job, "fma");
    if (!fma && strcmp(job, "sum")) { fprintf(stderr, "unknown job %s\n", job); return 2; }
    int nin = fma ? 3 : 1;
    in_arr IN[3];
    const char *names[3] = {"a", "b", "c"};
    for (int j = 0; j < nin; j++) read_arr(dir, names[j], &IN[j]);
    long n = IN[0].n;
    int64_t *on = malloc(8 * n + 8), *od = malloc(8 * n + 8), sn = 0, sd = 0;
    uint64_t be = UINT64_MAX, bo = UINT64_MAX, br = UINT64_MAX, bt = UINT64_MAX;
    pg_stats st = {0}, st0;
    pg_array_info info = {0};
    int failed = 0;
    mallopt(M_MMAP_THRESHOLD, 1 << 30);
    mallopt(M_TRIM_THRESHOLD, 1 << 30);
    pg_graph *g = pg_graph_create();
    for (int rep = 0; rep < reps; rep++) {
        pg_array *X[3], *R = NULL;
        uint64_t t0 = now();
        for (int j = 0; j < nin; j++)
            if (pg_array_from_fracs(g, IN[j].num, IN[j].den, (size_t)n, &X[j]) != PG_OK) { fprintf(stderr, "entry failed\n"); return 1; }
        uint64_t t1 = now();
        pg_status s;
        pg_value *sv = NULL;
        if (v4) pg_expr_begin(g);
        if (fma) s = pg_array_fma(g, X[0], X[1], X[2], &R);
        else s = pg_array_sum(g, X[0], &sv);
        if (v4) pg_expr_end(g);
        uint64_t t2 = now();
        failed = s != PG_OK;
        if (!failed) {
            if (fma) pg_array_to_fracs(g, R, on, od);
            else pg_value_get(g, sv, &sn, &sd);
        }
        uint64_t t3 = now();
        if (t1 - t0 < be) be = t1 - t0;
        if (t2 - t1 < bo) bo = t2 - t1;
        if (t3 - t2 < br) br = t3 - t2;
        if (t3 - t0 < bt) bt = t3 - t0;
        pg_stats_get(g, &st);
        if (R) pg_array_info_get(R, &info);
        pg_array_release(g, R);
        pg_value_release(g, sv);
        for (int j = nin; j-- > 0;) pg_array_release(g, X[j]);
        pg_stats_get(g, &st0);
        if (st0.nodes_live || st0.holders_live) { fprintf(stderr, "pg_bench: the graph is not empty after a rep\n"); return 1; }
    }
    pg_graph_destroy(g);
    if (failed && !fma) { printf("unable sum overflows\n"); return 3; }
    FILE *f = fopen(outp, "w");
    if (!f) { perror(outp); return 1; }
    if (failed) for (long i = 0; i < n; i++) fprintf(f, "0 0\n");  /* the whole operation failed */
    else if (fma) for (long i = 0; i < n; i++) fprintf(f, "%" PRId64 " %" PRId64 "\n", on[i], od[i]);
    else fprintf(f, "%" PRId64 " %" PRId64 "\n", sn, sd);
    fclose(f);
    printf("entry=%llu decomp=0 op=%llu red=%llu total=%llu nodes=%zu splits_entry=%zu splits_reduce=%zu "
           "restarts=%zu v4_retries=%zu rho=%zu cache_hits=%zu exc_out=%zu D=%" PRId64 " mem_peak=%zu mode=%s\n",
           (unsigned long long)be, (unsigned long long)bo, (unsigned long long)br, (unsigned long long)bt,
           st.nodes_live, st.splits_entry, st.splits_reduce, st.restarts, st.v4_retries, st.rho_calls,
           st.den_cache_hits, info.exceptions, info.D, st.mem_peak, v4 ? "v4" : "v3");
    return 0;
}
