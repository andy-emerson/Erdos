/* bench_m1.c: one invocation of one row of the matrix round (TC or a hash row) on one job (research/rule-m1.md),
 * driven by m1_runner.py.  From round 3's bench_r3.c, with its whole timing method (CLOCK_MONOTONIC read only between
 * calls; reps of r runs; the process's own CPU time per rep, reported only; the static stdout buffer; mallopt).  What
 * changes: a job is two matrices and one operation (addition or multiplication, set by the workload); three parts per
 * run (spec §8): creation (create A, B, C; enter every cell of A, then of B), operation (tau in the M rows, then the
 * addition or the multiply, then tau freed) and read-out (every cell of C); tau's build time reported on its own;
 * the memory run's heap figures after creation, at the end of the operation (before tau is freed) and inside each
 * growth (the growth hook); the counting and test builds' per-operation record.  Build with IMPL_TC, or IMPL_HASH
 * and MH_M, MH_GROW, MH_MAP, and the build's settings (none: timing; SK_COUNT: counting; SK_COUNT SK_AUDIT: test).
 *
 *   bench calib <workload> <readout>                      timing build: one rep at r = 1, 2, 4, ... until a rep lasts
 *                                                         >= 50 ms (or > 10 s: over cap)
 *   bench time  <workload> <readout> <r> <reps>           timing build: timed reps of r runs each
 *   bench mem   <workload> <readout>                      timing build: the memory run (rule-m1 item 7)
 *   bench count <workload> <readout> <percall>            counting build: one run, its counters, store figures and
 *                                                         memory reports, every growth, and the per-operation record
 *   bench gate  <workload> <readout> <percall> <dump>     test build: one run with the audits; the per-operation record;
 *                                                         the tables, cells and tau dumped (spec §10 item 2)
 *   bench version
 *
 * stdout:  calib <r> <total> <creation> <op> <read> <tau> row=.. cpu=.. span=..
 *          rep <i> <total> <creation> <op> <read> <tau> row=.. cpu=.. span=..      overcap <total>
 *          (total = creation + op + read; tau is part of op)
 *          ctr/xctr/stor/opc/growth/mem/memtau/memN/memrun lines     ok ...
 * failures: error where=<enter|tau|op|read> code=<c> step=<...>  exit 2      auditfail ...  exit 3
 * read-out: int64 num[n^2], int64 den[n^2], C's cells row-major, from the last run (the reference's format).
 * dump (gate): sections of an 8-byte name, an int64 count and count int64 values: tabA tabB tabC (every slot, -1
 * empty), celA celB celC (every handle field), tau (every position; M rows). */
#define _GNU_SOURCE
#include <inttypes.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "api_m1.h"

#define CALIB_MIN_NS 50000000ull
#define CAP_NS 10000000000ull

static int OPK;          /* 0 addition, 1 multiplication */
static uint32_t NN;      /* n */
static int64_t *IA, *IB; /* (in-A), (in-B): n^2 pairs (m, d) each, as entered */
static int64_t *ON, *OD; /* (out): the read-out */
static int16_t *PERCALL;
static uint64_t NOPS;
static enum { M_TIME, M_COUNT, M_GATE, M_MEM } MODE;
static const char *DUMP_PATH;
static mat *M[3];        /* A, B, C of the current run (the growth hook reads them) */

static uint64_t now_ns(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec;
}
#define CLOCK() now_ns()

static void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    return p;
}
static void die(const char *m) {
    fprintf(stderr, "bench: %s\n", m);
    exit(1);
}

/* workload: "M1WLOAD1", int32 op (0 add, 1 mul), int32 n, int32 job, int32 0, then int64 A[n^2][2], int64 B[n^2][2] */
static void load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) die("cannot open workload");
    char magic[8];
    int32_t h[4];
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, "M1WLOAD1", 8)) die("bad workload magic");
    if (fread(h, 4, 4, f) != 4) die("short header");
    if ((h[0] != 0 && h[0] != 1) || h[1] < 1 || h[1] > 4096) die("bad workload header");
    OPK = h[0];
    NN = (uint32_t)h[1];
    size_t q = (size_t)NN * NN;
    IA = xmalloc(16 * q);
    IB = xmalloc(16 * q);
    if (fread(IA, 8, 2 * q, f) != 2 * q || fread(IB, 8, 2 * q, f) != 2 * q) die("short workload");
    fclose(f);
    for (size_t i = 0; i < q; i++)
        if (IA[2 * i + 1] <= 0 || IB[2 * i + 1] <= 0) die("an entered denominator is not positive");
    ON = xmalloc(8 * q);
    OD = xmalloc(8 * q);
    NOPS = OPK ? 2 * (uint64_t)NN * NN * NN : (uint64_t)q;
}

static void write_out(const char *path) {
    FILE *f = fopen(path, "wb");
    size_t q = (size_t)NN * NN;
    if (!f) die("cannot write read-out");
    if (fwrite(ON, 8, q, f) != q || fwrite(OD, 8, q, f) != q) die("short write");
    fclose(f);
}

/* the in-use heap of the C library (rule-m1 item 7, as rule-r3): uordblks + hblkhd */
static uint64_t heap_now(void) {
    struct mallinfo2 mi = mallinfo2();
    return (uint64_t)mi.uordblks + (uint64_t)mi.hblkhd;
}
static uint64_t HEAP0, PEAK_HEAP, PEAK_USED, NGROW;

static void fail(const char *where, mat *c, int rc) {
    printf("error where=%s code=%d step=", where, rc);
    api_last_error(c, stdout);
    printf("\n");
    fflush(stdout);
    exit(2);
}

/* the growth hook (spec §9): inside each growth, just before the old table is freed.  The memory run reads the heap
 * and the accounted bytes (every matrix as it stands, the old table included, plus (n), (r) and tau); the counting
 * and test runs print the growth. */
static void on_growth(const m1_growth *g) {
    if (MODE == M_MEM) {
        uint64_t h = heap_now() - HEAP0;
        uint64_t u = 0;
        for (int r = 0; r < 3; r++)
            if (M[r]) {
                sk_memory rep;
                API(memory)(M[r], &rep);
                u += rep.used;
            }
        if (M[2]) u += m_tau_bytes(M[2]);
        u += g->bytes_new + g->bytes_map;
        if (h > PEAK_HEAP) PEAK_HEAP = h;
        if (u > PEAK_USED) PEAK_USED = u;
        NGROW++;
    } else if (MODE == M_COUNT || MODE == M_GATE) {
        printf("growth %s fields=%u W=%u T_old=%u reinserted=%llu rprobes=%llu rewritten=%llu wprobes=%llu "
               "bytes_new=%llu bytes_map=%llu\n",
               ROLE[g->role], g->fields, g->W, g->T_old, (unsigned long long)g->reinserted,
               (unsigned long long)g->rprobes, (unsigned long long)g->rewritten, (unsigned long long)g->wprobes,
               (unsigned long long)g->bytes_new, (unsigned long long)g->bytes_map);
    }
}

static void memrun_line(const char *when) {
    printf("memrun %s heap=%" PRIu64 " used=%" PRIu64 " cells=%" PRIu64 "\n", when, heap_now() - HEAP0, api_used(M),
           3 * (uint64_t)NN * NN);
}

#ifdef SK_AUDIT
static void audit_all(const char *when) {
    const char *msg = "";
    for (int r = 0; r < 3; r++)
        if (API(audit)(M[r], &msg)) {
            printf("auditfail when=%s matrix=%s msg=%s\n", when, ROLE[r], msg);
            exit(3);
        }
}

#ifdef IMPL_Q
static void dump_section(FILE *f, const char *name, const int64_t *v, int64_t n) {
    char tag[8] = {0};
    memcpy(tag, name, strlen(name) < 8 ? strlen(name) : 8);
    if (fwrite(tag, 1, 8, f) != 8 || fwrite(&n, 8, 1, f) != 1 || (n && fwrite(v, 8, (size_t)n, f) != (size_t)n))
        die("cannot write the dump");
}

static void dump_store(FILE *f, int r) {
    m1_fig g;
    API(figures)(M[r], &g);
    int64_t *v = xmalloc(8 * ((size_t)g.T > g.nf ? g.T : g.nf));
    for (uint32_t s = 0; s < g.T; s++) {
        sk_int x = -1;
        if (API(t_whole)(M[r], s, &x) != SK_OK) die("t_whole");
        v[s] = x;
    }
    char name[8];
    snprintf(name, sizeof name, "tab%s", ROLE[r]);
    dump_section(f, name, v, g.T);
    for (uint32_t i = 0; i < g.nf; i++) {
        uint32_t x = 0;
        if (API(t_field)(M[r], i, &x) != SK_OK) die("t_field");
        v[i] = x;
    }
    snprintf(name, sizeof name, "cel%s", ROLE[r]);
    dump_section(f, name, v, g.nf);
    free(v);
}
#endif
#endif

/* ----------------------------------------------------------------------------------------- a run */

/* t[0] creation, t[1] operation (tau included), t[2] read-out, t[3] tau alone */
static void run(uint64_t t[4]) {
    size_t q = (size_t)NN * NN;
    if (MODE == M_MEM) HEAP0 = heap_now();
    M[0] = M[1] = M[2] = NULL;
    uint64_t c0 = CLOCK(); /* creation: create A, B, C; enter A, then B (spec §§5.7, 8) */
    M[0] = m_create(NN, M1_A);
    M[1] = m_create(NN, M1_B);
    M[2] = m_create(NN, M1_C);
    for (size_t i = 0; i < q; i++) {
        int rc = API(enter)(M[0], (sk_int)IA[2 * i], (sk_int)IA[2 * i + 1]);
        if (rc) fail("enter", M[0], rc);
    }
    for (size_t i = 0; i < q; i++) {
        int rc = API(enter)(M[1], (sk_int)IB[2 * i], (sk_int)IB[2 * i + 1]);
        if (rc) fail("enter", M[1], rc);
    }
    uint64_t c1 = CLOCK(); /* operation: tau (M rows), the addition or the multiply, tau freed */
    if (MODE == M_MEM) memrun_line("after_creation");
    if (MODE == M_COUNT || MODE == M_GATE) {
        api_dump_memory(M, "after_creation", stdout);
        api_dump_storage(M, "after_creation", stdout);
    }
#ifdef SK_AUDIT
    if (MODE == M_GATE) audit_all("after_creation");
#endif
    uint64_t c1b = CLOCK();
    int rc = m_tau_build(M[0], M[1], M[2]);
    if (rc) fail("tau", M[2], rc);
    uint64_t c2 = CLOCK();
#ifdef SK_AUDIT
#ifdef IMPL_Q
    if (MODE == M_GATE) {
        const char *msg = "";
        if (MH_M && API(audit_tau)(M[0], M[1], M[2], &msg)) {
            printf("auditfail when=tau matrix=C msg=%s\n", msg);
            exit(3);
        }
    }
#endif
#endif
#ifdef SK_COUNT
    if (MODE == M_COUNT || MODE == M_GATE) {
        m1_percall = PERCALL;
        m1_percall_n = 0;
        m1_percall_cap = NOPS;
    }
#endif
    rc = OPK ? API(mmul)(M[0], M[1], M[2]) : API(madd)(M[0], M[1], M[2]);
    if (rc) fail("op", M[2], rc);
#ifdef SK_COUNT
    m1_percall = NULL;
#endif
    if (MODE == M_MEM) memrun_line("end"); /* before tau is freed */
    if (MODE == M_COUNT || MODE == M_GATE) api_dump_memory(M, "end", stdout);
#ifdef SK_AUDIT
    FILE *df = NULL;
    if (MODE == M_GATE && DUMP_PATH) {
        df = fopen(DUMP_PATH, "wb");
        if (!df) die("cannot write the dump");
#ifdef IMPL_Q
        m1_fig gb;
        API(figures)(M[1], &gb);
        if (MH_M) {
            int64_t *v = xmalloc(8 * (size_t)gb.T);
            for (uint32_t k = 0; k < gb.T; k++) {
                uint32_t x = 0;
                if (API(t_tau)(M[2], k, &x) != SK_OK) die("t_tau");
                v[k] = x;
            }
            dump_section(df, "tau", v, gb.T);
            free(v);
        }
#endif
    }
#endif
    m_tau_free(M[2]);
    uint64_t c3 = CLOCK(); /* read-out: every cell of C into (out) */
    for (size_t i = 0; i < q; i++) {
        sk_int m = 0, d = 0;
        rc = API(read)(M[2], (uint32_t)i, &m, &d);
        if (rc) fail("read", M[2], rc);
        ON[i] = m;
        OD[i] = d;
    }
    uint64_t c4 = CLOCK();
    t[0] += c1 - c0;
    t[1] += c3 - c1b;
    t[2] += c4 - c3;
    t[3] += c2 - c1b;
    if (MODE == M_COUNT || MODE == M_GATE) {
        api_dump_storage(M, "end", stdout);
        api_dump_counters(M, stdout);
    }
#ifdef SK_AUDIT
    if (MODE == M_GATE) {
        audit_all("end");
#ifdef IMPL_Q
        if (df)
            for (int r = 0; r < 3; r++) dump_store(df, r);
#endif
        if (df) fclose(df);
    }
#endif
    for (int r = 0; r < 3; r++) API(destroy)(M[r]);
    M[0] = M[1] = M[2] = NULL;
}

/* the process's own CPU time over a whole rep, and CLOCK_MONOTONIC over the same span (rule-m1 item 4, as rule-r3:
 * reported only); read once before and once after the rep's r runs, outside every timed part */
static uint64_t REP_CPU, REP_SPAN;

static uint64_t cpu_ns(void) {
    struct timespec t;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
    return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec;
}

static void rep(uint64_t r, uint64_t t[4]) {
    t[0] = t[1] = t[2] = t[3] = 0;
    uint64_t s0 = now_ns(), c0 = cpu_ns();
    for (uint64_t i = 0; i < r; i++) run(t);
    uint64_t c1 = cpu_ns(), s1 = now_ns();
    REP_CPU = c1 - c0;
    REP_SPAN = s1 - s0;
}

static void line(const char *kind, uint64_t a, uint64_t t[4]) {
    uint64_t tot = t[0] + t[1] + t[2];
    printf("%s %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " row=%s cpu=%" PRIu64
           " span=%" PRIu64 "\n",
           kind, a, tot, t[0], t[1], t[2], t[3], IMPL_NAME, REP_CPU, REP_SPAN);
}

/* stdout's buffer, static so that it is never on the heap (rule-r3 item 7, carried over) */
static char OUTBUF[BUFSIZ];

int main(int argc, char **argv) {
    setvbuf(stdout, OUTBUF, _IOFBF, sizeof OUTBUF);
    /* as round 3: freed memory is kept by malloc, so later runs reuse it */
    mallopt(M_MMAP_THRESHOLD, 1 << 30);
    mallopt(M_TRIM_THRESHOLD, 1 << 30);
    if (argc >= 2 && !strcmp(argv[1], "version")) {
        printf("version row=%s build=%s\n", IMPL_NAME, BUILD_NAME);
        return 0;
    }
    if (argc < 4) die("usage: bench calib|time|mem|count|gate <workload> <readout> [...]");
    const char *mode = argv[1];
    load(argv[2]);
    uint64_t t[4];
    int is = 0;
#define NEED(m, b)                                                                                                \
    if (!strcmp(mode, m) && strcmp(BUILD_NAME, b)) die(m " needs the " b " build");
    NEED("calib", "timing")
    NEED("time", "timing")
    NEED("mem", "timing")
    NEED("count", "counting")
    NEED("gate", "test")
    m1_growth_hook = on_growth;
    if (!strcmp(mode, "calib")) {
        is = 1;
        MODE = M_TIME;
        m1_growth_hook = NULL;
        for (uint64_t r = 1; r <= (1ull << 40); r *= 2) {
            rep(r, t);
            uint64_t tot = t[0] + t[1] + t[2];
            line("calib", r, t);
            if (tot > CAP_NS) {
                printf("overcap %" PRIu64 "\n", tot);
                break;
            }
            if (tot >= CALIB_MIN_NS) break;
        }
    }
    if (!strcmp(mode, "time")) {
        is = 1;
        if (argc < 6) die("time needs r and reps");
        MODE = M_TIME;
        m1_growth_hook = NULL;
        uint64_t r = strtoull(argv[4], 0, 10), reps = strtoull(argv[5], 0, 10);
        for (uint64_t i = 0; i < reps; i++) {
            rep(r, t);
            uint64_t tot = t[0] + t[1] + t[2];
            line("rep", i, t);
            if (tot > CAP_NS) {
                printf("overcap %" PRIu64 "\n", tot);
                break;
            }
        }
    }
    if (!strcmp(mode, "mem")) {
        is = 1;
        MODE = M_MEM;
        rep(1, t);
        printf("memrun peak heap=%" PRIu64 " used=%" PRIu64 " growths=%" PRIu64 " cells=%" PRIu64 "\n", PEAK_HEAP,
               PEAK_USED, NGROW, 3 * (uint64_t)NN * NN);
    }
    if (!strcmp(mode, "count") || !strcmp(mode, "gate")) {
        is = 1;
        MODE = !strcmp(mode, "count") ? M_COUNT : M_GATE;
        if (argc < 5) die("count and gate need the per-operation output path");
        if (MODE == M_GATE && argc > 5) DUMP_PATH = argv[5];
        PERCALL = xmalloc(sizeof(int16_t) * M1_PERCALL * (size_t)NOPS);
        rep(1, t);
#ifdef SK_COUNT
        if (m1_percall_n != NOPS) die("the per-operation record is not one record per operation");
#endif
        FILE *f = fopen(argv[4], "wb");
        if (!f || fwrite(PERCALL, sizeof(int16_t), M1_PERCALL * (size_t)NOPS, f) != M1_PERCALL * (size_t)NOPS)
            die("cannot write the per-operation record");
        fclose(f);
        free(PERCALL);
    }
    if (!is) die("unknown mode, or not in this build");
    write_out(argv[3]);
    printf("ok impl=%s n=%u op=%s ops=%" PRIu64 "\n", IMPL_NAME, NN, OPK ? "mul" : "add", NOPS);
    return 0;
}
