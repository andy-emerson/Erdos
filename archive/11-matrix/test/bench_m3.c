/* bench_m3.c: one invocation of one row of round m3 (TC, Hash-V-pack, Pack-primes or Pack-coprime) on one job
 * (research/rule-m3.md), driven by m3_runner.py.  From bench_m2.c (unchanged, m2's), with its whole timing method
 * (CLOCK_MONOTONIC read only between calls; reps of r runs; the process's own CPU time per rep, reported only; the static
 * stdout buffer; mallopt), its job sets, parts and memory build.  What changes (spec v0.3 §16.10): the rows (api_m3.h);
 * no growth (no grow row); the factoring hook: each matrix's factoring counts, printed in the counting and test builds;
 * each result's factored-arithmetic counts and every matrix's (f) and (q) sizes beside its store figures; the gate's
 * dump of every factored matrix's (f) and (q) at its end.  The driver still reads the clock only between calls: a
 * factoring falls in its input's build (inside the last enter) or in the operation (inside madd or mmul).
 *
 *   bench calib <workload> <readout>                      timing build: one rep at r = 1, 2, 4, ... until a rep lasts
 *                                                         >= 50 ms (or > 10 s: over cap)
 *   bench time  <workload> <readout> <r> <reps>           timing build: timed reps of r runs each
 *   bench mem   <workload> <readout>                      memory build: the memory run (one run)
 *   bench count <workload> <readout> <percall>            counting build: one run, its counters, store figures, every
 *                                                         packing and factoring, and the per-operation record
 *   bench gate  <workload> <readout> <percall> <dump>     test build: one run with the audits, the per-operation
 *                                                         record, every list, cells, (f) and (q) dumped, (h) at every
 *                                                         packing; each intermediate's read-out to <readout>.<name>
 *   bench version
 *
 * stdout:  calib <r> <total> <op> <read> builds=<b>,<b>[,<b>,<b>] row=.. cpu=.. span=..
 *          rep <i> <total> <op> <read> builds=... row=.. cpu=.. span=..       overcap <total>
 *          (total = the builds + op + read)
 *          ctr/xctr/stor/opc/ffig/facc/fac/pack/memrun lines     ok ...
 * failures: error where=<enter|op|read|acct> code=<c> step=<...>  exit 2      auditfail ...  exit 3
 * read-out: int64 num[n^2], int64 den[n^2], the result's cells row-major, from the last run.
 * dump (gate): sections of an 8-byte name, an int64 count and count int64 values: tab<X> (every entry of X's list),
 * cel<X> (every handle field), hb<X> (every slot of X's build table (h), read at its packing); factored rows: ff<X>
 * (for each whole number of the list in order: its number of entries, then each entry's factor and exponent) and fq<X>
 * (Pack-coprime: every piece of the base, in order), at the matrix's end. */
#define _GNU_SOURCE
#include <inttypes.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "api_m3.h"

#define CALIB_MIN_NS 50000000ull
#define CAP_NS 10000000000ull

static int SET;             /* 0 add, 1 mul, 2 sop, 3 pos */
static int NIN;             /* inputs: 2 or 4 */
static uint32_t NN;         /* n */
static int64_t *IN[4];      /* (in-A) .. (in-D): n^2 pairs (m, d) each, as entered */
static int64_t *ON, *OD;    /* (out): the read-out */
static int16_t *PERCALL;
static uint64_t NOPS;
static enum { M_TIME, M_COUNT, M_GATE, M_MEM } MODE;
static const char *DUMP_PATH, *READOUT;
static FILE *DUMP;
static mat *M[M2_NMAT];     /* the matrices of the current run, by id (m2.h) */

/* the job sets (spec v0.2 §15.5): names by id, and the steps c = a op b (op 0 addition, 1 multiplication) */
static const char *const NAMES[4][M2_NMAT] = {{"A", "B", "", "", "", "", "C"},
                                              {"A", "B", "", "", "", "", "C"},
                                              {"A", "B", "C", "D", "P", "Q", "R"},
                                              {"A", "B", "C", "D", "S", "U", "R"}};
static const int NSTEP[4] = {1, 1, 3, 3};
static const int STEPS[4][3][4] = {
    {{0, M2_A, M2_B, M2_R}},
    {{1, M2_A, M2_B, M2_R}},
    {{1, M2_A, M2_B, M2_X1}, {1, M2_C, M2_D, M2_X2}, {0, M2_X1, M2_X2, M2_R}},
    {{0, M2_A, M2_B, M2_X1}, {0, M2_C, M2_D, M2_X2}, {1, M2_X1, M2_X2, M2_R}},
};

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

/* ----------------------------------------------------------------------------------------- the heap (memory build) */

/* The memory build's allocation wrapper (spec v0.2 §15.6): the live sum of malloc_usable_size and its maximum, a
 * realloc counted as the free of the old block and the allocation of the new.  Every row, no source changed. */
static uint64_t HEAP_LIVE, HEAP_PEAK;
#ifdef M2_MEM
void *__real_malloc(size_t n);
void *__real_calloc(size_t n, size_t s);
void *__real_realloc(void *p, size_t n);
void __real_free(void *p);
static void heap_add(void *p) {
    if (!p) return;
    HEAP_LIVE += malloc_usable_size(p);
    if (HEAP_LIVE > HEAP_PEAK) HEAP_PEAK = HEAP_LIVE;
}
void *__wrap_malloc(size_t n) {
    void *p = __real_malloc(n);
    heap_add(p);
    return p;
}
void *__wrap_calloc(size_t n, size_t s) {
    void *p = __real_calloc(n, s);
    heap_add(p);
    return p;
}
void *__wrap_realloc(void *q, size_t n) {
    size_t old = q ? malloc_usable_size(q) : 0;
    void *p = __real_realloc(q, n);
    if (p) {
        HEAP_LIVE -= old;
        heap_add(p);
    }
    return p;
}
void __wrap_free(void *p) {
    if (p) HEAP_LIVE -= malloc_usable_size(p);
    __real_free(p);
}
#endif

/* ----------------------------------------------------------------------------------------- the workload */

/* "M1WLOAD1" (add, mul: m1's), int32 op (0, 1), int32 n, int32 job, int32 0, then int64 A[n^2][2], B[n^2][2]; or
 * "M2WLOAD1" (sop, pos), int32 set (2, 3), n, job, 0, then A, B, C, D */
static void load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) die("cannot open workload");
    char magic[8];
    int32_t h[4];
    if (fread(magic, 1, 8, f) != 8) die("short header");
    int m1 = !memcmp(magic, "M1WLOAD1", 8), m2 = !memcmp(magic, "M2WLOAD1", 8);
    if (!m1 && !m2) die("bad workload magic");
    if (fread(h, 4, 4, f) != 4) die("short header");
    if ((m1 && h[0] != 0 && h[0] != 1) || (m2 && h[0] != 2 && h[0] != 3) || h[1] < 1 || h[1] > 4096)
        die("bad workload header");
    SET = h[0];
    NIN = SET < 2 ? 2 : 4;
    NN = (uint32_t)h[1];
    size_t q = (size_t)NN * NN;
    for (int x = 0; x < NIN; x++) {
        IN[x] = xmalloc(16 * q);
        if (fread(IN[x], 8, 2 * q, f) != 2 * q) die("short workload");
        for (size_t i = 0; i < q; i++)
            if (IN[x][2 * i + 1] <= 0) die("an entered denominator is not positive");
    }
    fclose(f);
    ON = xmalloc(8 * q);
    OD = xmalloc(8 * q);
    NOPS = 0;
    for (int s = 0; s < NSTEP[SET]; s++) NOPS += STEPS[SET][s][0] ? 2 * (uint64_t)NN * NN * NN : (uint64_t)q;
}

#ifdef SK_AUDIT
/* a matrix's cells, in the read-out's format (the gate: each intermediate) */
static void write_pairs(const char *path, mat *m) {
    size_t q = (size_t)NN * NN;
    int64_t *v = xmalloc(16 * q);
    for (size_t i = 0; i < q; i++) {
        sk_int a = 0, d = 0;
        if (API(read)(m, (uint32_t)i, &a, &d)) die("read");
        v[i] = a;
        v[q + i] = d;
    }
    FILE *f = fopen(path, "wb");
    if (!f || fwrite(v, 8, 2 * q, f) != 2 * q) die("cannot write a read-out");
    fclose(f);
    free(v);
}
#endif

static void write_out(const char *path) {
    FILE *f = fopen(path, "wb");
    size_t q = (size_t)NN * NN;
    if (!f) die("cannot write read-out");
    if (fwrite(ON, 8, q, f) != q || fwrite(OD, 8, q, f) != q) die("short write");
    fclose(f);
}

static void fail(const char *where, mat *c, int rc) {
    printf("error where=%s code=%d step=", where, rc);
    if (c) api_last_error(c, stdout);
    printf("\n");
    fflush(stdout);
    exit(2);
}

/* ----------------------------------------------------------------------------------------- accounting */

/* The accounted running total (spec v0.2 §15.6): the hash rows keep it themselves (m2.h, memory build); TC's is
 * computed here from the counts and the call order: its only structures are each matrix's pairs, allocated at create
 * and freed with the matrix, so the total is the sum of the pairs of every matrix alive.  No code in tcm.c. */
static uint64_t TC_LIVE, TC_PEAK;
static uint64_t acct_live(void) {
#ifdef IMPL_Q
    return m2_acct_live;
#else
    return TC_LIVE;
#endif
}
static uint64_t acct_peak(void) {
#ifdef IMPL_Q
    return m2_acct_peak;
#else
    return TC_PEAK;
#endif
}
static void acct_reset(void) {
    m2_acct_peak = m2_acct_live;
    TC_PEAK = TC_LIVE;
}
static uint64_t HEAP0;

/* the accounted bytes of the live matrices, by their memory reports; in the memory build it must equal the running
 * total (a check of the accounting itself) */
static uint64_t reports(uint64_t *cells) {
    uint64_t u = 0, c = 0;
    for (int x = 0; x < M2_NMAT; x++)
        if (M[x]) {
            u += api_used(M[x]);
            c += (uint64_t)NN * NN;
        }
    if (cells) *cells = c;
    if (MODE == M_MEM && u != acct_live()) {
        printf("error where=acct code=0 step=running-total-%" PRIu64 "-reports-%" PRIu64 "\n", acct_live(), u);
        fflush(stdout);
        exit(2);
    }
    return u;
}

/* ----------------------------------------------------------------------------------------- hooks */

#ifdef IMPL_F
static void on_factor(const m3_factor *x) {
    if (MODE == M_COUNT || MODE == M_GATE) api_dump_fcount(NAMES[SET][x->id], &x->fig, &x->fc, stdout);
}
#endif

#if defined(SK_AUDIT) && defined(IMPL_Q)
static void dump_section(const char *name, const int64_t *v, int64_t n) {
    char tag[8] = {0};
    memcpy(tag, name, strlen(name) < 8 ? strlen(name) : 8);
    if (fwrite(tag, 1, 8, DUMP) != 8 || fwrite(&n, 8, 1, DUMP) != 1 || (n && fwrite(v, 8, (size_t)n, DUMP) != (size_t)n))
        die("cannot write the dump");
}
#endif

static void on_pack(const m2_pack *p) {
    if (MODE != M_COUNT && MODE != M_GATE) return;
    const char *name = NAMES[SET][p->id];
    printf("pack %s W=%u T=%u w=%u whb=%u whp=%u rewritten=%llu bytes_list=%llu bytes_map=%llu bytes_table=%llu "
           "bytes_cells_build=%llu bytes_cells_pack=%llu\n",
           name, p->W, p->T, p->w, p->wh_build, p->wh_pack, (unsigned long long)p->rewritten,
           (unsigned long long)p->bytes_list, (unsigned long long)p->bytes_map, (unsigned long long)p->bytes_table,
           (unsigned long long)p->bytes_cells_build, (unsigned long long)p->bytes_cells_pack);
#if defined(SK_AUDIT) && defined(IMPL_Q)
    if (MODE == M_GATE && DUMP) { /* (h), slot by slot, just before it is freed */
        int64_t *v = xmalloc(8 * (size_t)p->T);
        for (uint32_t s = 0; s < p->T; s++) {
            sk_int x = -1;
            if (API(t_whole)((const mat *)p->x, s, &x) != SK_OK) die("t_whole (h)");
            v[s] = x;
        }
        char sec[8];
        snprintf(sec, sizeof sec, "hb%s", name);
        dump_section(sec, v, p->T);
        free(v);
    }
#endif
}

/* ----------------------------------------------------------------------------------------- one matrix */

#ifdef SK_AUDIT
static void audit_one(int id, const char *when) {
    const char *msg = "";
    if (API(audit)(M[id], &msg)) {
        printf("auditfail when=%s matrix=%s msg=%s\n", when, NAMES[SET][id], msg);
        exit(3);
    }
}

#ifdef IMPL_Q
static void dump_store(int id) {
    m2_fig g;
    API(figures)(M[id], &g);
    uint32_t nv = g.packed ? g.W : g.T;
    int64_t *v = xmalloc(8 * ((size_t)nv > g.nf ? nv : g.nf) + 8);
    for (uint32_t s = 0; s < nv; s++) {
        sk_int x = -1;
        if (API(t_whole)(M[id], s, &x) != SK_OK) die("t_whole");
        v[s] = x;
    }
    char name[8];
    snprintf(name, sizeof name, "tab%s", NAMES[SET][id]);
    dump_section(name, v, nv);
    for (uint32_t i = 0; i < g.nf; i++) {
        uint32_t x = 0;
        if (API(t_field)(M[id], i, &x) != SK_OK) die("t_field");
        v[i] = x;
    }
    snprintf(name, sizeof name, "cel%s", NAMES[SET][id]);
    dump_section(name, v, g.nf);
    free(v);
#ifdef IMPL_F
    m3_ffig ff;
    API(ffigures)(M[id], &ff);
    int64_t *u = xmalloc(8 * (g.W + 2 * (size_t)ff.E + ff.nq + 1));
    size_t k = 0;
    for (uint32_t i = 0; i < g.W; i++) { /* (f): each number's entries */
        uint32_t ne = 0;
        if (API(t_nfac)(M[id], i, &ne) != SK_OK) die("t_nfac");
        u[k++] = ne;
        for (uint32_t j = 0; j < ne; j++) {
            sk_int q = 0;
            unsigned e = 0;
            if (API(t_fac)(M[id], i, j, &q, &e) != SK_OK) die("t_fac");
            u[k++] = q;
            u[k++] = e;
        }
    }
    snprintf(name, sizeof name, "ff%s", NAMES[SET][id]);
    dump_section(name, u, (int64_t)k);
    k = 0;
    for (uint32_t i = 0; i < ff.nq; i++) { /* (q): the pieces in order */
        sk_int q = 0;
        if (API(t_piece)(M[id], i, &q) != SK_OK) die("t_piece");
        u[k++] = q;
    }
    snprintf(name, sizeof name, "fq%s", NAMES[SET][id]);
    dump_section(name, u, (int64_t)k);
    free(u);
#endif
}
#endif
#endif

static void mk(int id) {
    M[id] = m_create(NN, id);
#ifndef IMPL_Q
    if (MODE == M_MEM) {
        TC_LIVE += api_used(M[id]);
        if (TC_LIVE > TC_PEAK) TC_PEAK = TC_LIVE;
    }
#endif
}

/* a matrix's last figures (counting and test builds), then freed with all its structures */
static void rm(int id) {
    if (MODE == M_COUNT || MODE == M_GATE) {
        const char *name = NAMES[SET][id];
        api_dump_storage(M[id], name, "end", !M2_INPUT(id), stdout);
        api_dump_counters(M[id], name, stdout);
#ifdef SK_AUDIT
        if (MODE == M_GATE) {
            audit_one(id, "end");
#ifdef IMPL_Q
            if (DUMP) dump_store(id);
#endif
            if (id == M2_X1 || id == M2_X2) { /* each intermediate against the exact reference (spec v0.2 §15.7) */
                char p[4096];
                snprintf(p, sizeof p, "%s.%s", READOUT, name);
                write_pairs(p, M[id]);
            }
        }
#endif
    }
#ifndef IMPL_Q
    if (MODE == M_MEM) TC_LIVE -= api_used(M[id]);
#endif
    API(destroy)(M[id]);
    M[id] = NULL;
}

/* ----------------------------------------------------------------------------------------- a run */

/* tb[x]: input x's build; t[0] the operation, t[1] the read-out */
static void run(uint64_t tb[4], uint64_t t[2]) {
    size_t q = (size_t)NN * NN;
    memset(M, 0, sizeof M);
    if (MODE == M_MEM) HEAP0 = HEAP_LIVE;
    for (int x = 0; x < NIN; x++) { /* each input's build: its create and every enter, timed alone (§15.6) */
        uint64_t h0 = HEAP_LIVE, a0 = acct_live();
        if (MODE == M_MEM) {
            acct_reset();
            HEAP_PEAK = HEAP_LIVE;
        }
        uint64_t c0 = CLOCK();
        mk(x);
        for (size_t i = 0; i < q; i++) {
            int rc = API(enter)(M[x], (sk_int)IN[x][2 * i], (sk_int)IN[x][2 * i + 1]);
            if (rc) fail("enter", M[x], rc);
        }
        uint64_t c1 = CLOCK();
        tb[x] += c1 - c0;
        if (MODE == M_MEM)
            printf("memrun built_%s acct=%" PRIu64 " heap=%" PRIu64 " peak_acct=%" PRIu64 " base_acct=%" PRIu64
                   " peak_heap=%" PRIu64 " base_heap=%" PRIu64 " cells=%" PRIu64 "\n",
                   NAMES[SET][x], api_used(M[x]), HEAP_LIVE - h0, acct_peak(), a0, HEAP_PEAK - HEAP0, h0 - HEAP0,
                   (uint64_t)q);
        if (MODE == M_COUNT || MODE == M_GATE) api_dump_storage(M[x], NAMES[SET][x], "built", 0, stdout);
#ifdef SK_AUDIT
        if (MODE == M_GATE) audit_one(x, "built");
#endif
    }
    if (MODE == M_MEM) {
        uint64_t cells, u = reports(&cells);
        printf("memrun after_creation acct=%" PRIu64 " heap=%" PRIu64 " cells=%" PRIu64 "\n", u, HEAP_LIVE - HEAP0,
               cells);
        acct_reset(); /* the operation's peak: from its start (§15.6) */
        HEAP_PEAK = HEAP_LIVE;
    }
#ifdef SK_COUNT
    if (MODE == M_COUNT || MODE == M_GATE) {
        m1_percall = PERCALL;
        m1_percall_n = 0;
        m1_percall_cap = NOPS;
    }
#endif
    uint64_t c2 = CLOCK(); /* the operation: each result created just before its own step; intermediates freed after
                              their last use (§15.5 items 5-6) */
    for (int s = 0; s < NSTEP[SET]; s++) {
        const int *st = STEPS[SET][s];
        mk(st[3]);
        int rc = st[0] ? API(mmul)(M[st[1]], M[st[2]], M[st[3]]) : API(madd)(M[st[1]], M[st[2]], M[st[3]]);
        if (rc) fail("op", M[st[3]], rc);
    }
    if (NSTEP[SET] == 3) {
        rm(M2_X1);
        rm(M2_X2);
    }
    uint64_t c3 = CLOCK();
#ifdef SK_COUNT
    m1_percall = NULL;
#endif
    if (MODE == M_MEM) {
        printf("memrun op peak_acct=%" PRIu64 " peak_heap=%" PRIu64 " cells=%" PRIu64 "\n", acct_peak(),
               HEAP_PEAK - HEAP0, (uint64_t)(NIN + NSTEP[SET]) * q);
        uint64_t cells, u = reports(&cells);
        printf("memrun end acct=%" PRIu64 " heap=%" PRIu64 " cells=%" PRIu64 "\n", u, HEAP_LIVE - HEAP0, cells);
    }
    for (size_t i = 0; i < q; i++) { /* the read-out: the result only */
        sk_int m = 0, d = 0;
        int rc = API(read)(M[M2_R], (uint32_t)i, &m, &d);
        if (rc) fail("read", M[M2_R], rc);
        ON[i] = m;
        OD[i] = d;
    }
    uint64_t c4 = CLOCK();
    t[0] += c3 - c2;
    t[1] += c4 - c3;
    for (int x = 0; x < M2_NMAT; x++)
        if (M[x]) rm(x);
    if (MODE == M_MEM && acct_live() != 0) fail("acct", NULL, 0);
}

/* the process's own CPU time over a whole rep, and CLOCK_MONOTONIC over the same span (reported only); read once before
 * and once after the rep's r runs, outside every timed part */
static uint64_t REP_CPU, REP_SPAN;

static uint64_t cpu_ns(void) {
    struct timespec t;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
    return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec;
}

static void rep(uint64_t r, uint64_t tb[4], uint64_t t[2]) {
    tb[0] = tb[1] = tb[2] = tb[3] = t[0] = t[1] = 0;
    uint64_t s0 = now_ns(), c0 = cpu_ns();
    for (uint64_t i = 0; i < r; i++) run(tb, t);
    uint64_t c1 = cpu_ns(), s1 = now_ns();
    REP_CPU = c1 - c0;
    REP_SPAN = s1 - s0;
}

static uint64_t total(const uint64_t tb[4], const uint64_t t[2]) { return tb[0] + tb[1] + tb[2] + tb[3] + t[0] + t[1]; }

static void line(const char *kind, uint64_t a, const uint64_t tb[4], const uint64_t t[2]) {
    printf("%s %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " builds=", kind, a, total(tb, t), t[0], t[1]);
    for (int x = 0; x < NIN; x++) printf("%s%" PRIu64, x ? "," : "", tb[x]);
    printf(" row=%s cpu=%" PRIu64 " span=%" PRIu64 "\n", IMPL_NAME, REP_CPU, REP_SPAN);
}

/* stdout's buffer, static so that it is never on the heap (carried over) */
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
    READOUT = argv[3];
    load(argv[2]);
    uint64_t tb[4], t[2];
    int is = 0;
#define NEED(m, b)                                                                                                \
    if (!strcmp(mode, m) && strcmp(BUILD_NAME, b)) die(m " needs the " b " build");
    NEED("calib", "timing")
    NEED("time", "timing")
    NEED("mem", "memory")
    NEED("count", "counting")
    NEED("gate", "test")
    m2_pack_hook = on_pack;
#ifdef IMPL_F
    m3_factor_hook = on_factor;
#endif
    if (!strcmp(mode, "calib")) {
        is = 1;
        MODE = M_TIME;
        m2_pack_hook = NULL;
        m3_factor_hook = NULL;
        for (uint64_t r = 1; r <= (1ull << 40); r *= 2) {
            rep(r, tb, t);
            uint64_t tot = total(tb, t);
            line("calib", r, tb, t);
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
        m2_pack_hook = NULL;
        m3_factor_hook = NULL;
        uint64_t r = strtoull(argv[4], 0, 10), reps = strtoull(argv[5], 0, 10);
        for (uint64_t i = 0; i < reps; i++) {
            rep(r, tb, t);
            uint64_t tot = total(tb, t);
            line("rep", i, tb, t);
            if (tot > CAP_NS) {
                printf("overcap %" PRIu64 "\n", tot);
                break;
            }
        }
    }
    if (!strcmp(mode, "mem")) {
        is = 1;
        MODE = M_MEM;
        m2_pack_hook = NULL;
        rep(1, tb, t);
    }
    if (!strcmp(mode, "count") || !strcmp(mode, "gate")) {
        is = 1;
        MODE = !strcmp(mode, "count") ? M_COUNT : M_GATE;
        if (argc < 5) die("count and gate need the per-operation output path");
        if (MODE == M_GATE && argc > 5) DUMP_PATH = argv[5];
        if (DUMP_PATH && !(DUMP = fopen(DUMP_PATH, "wb"))) die("cannot write the dump");
        PERCALL = xmalloc(sizeof(int16_t) * M1_PERCALL * (size_t)NOPS);
        rep(1, tb, t);
#ifdef SK_COUNT
        if (m1_percall_n != NOPS) die("the per-operation record is not one record per operation");
#endif
        if (DUMP) fclose(DUMP);
        FILE *f = fopen(argv[4], "wb");
        if (!f || fwrite(PERCALL, sizeof(int16_t), M1_PERCALL * (size_t)NOPS, f) != M1_PERCALL * (size_t)NOPS)
            die("cannot write the per-operation record");
        fclose(f);
        free(PERCALL);
    }
    if (!is) die("unknown mode, or not in this build");
    write_out(argv[3]);
    static const char *const SETNAME[4] = {"add", "mul", "sop", "pos"};
    printf("ok impl=%s n=%u set=%s ops=%" PRIu64 "\n", IMPL_NAME, NN, SETNAME[SET], NOPS);
    return 0;
}
