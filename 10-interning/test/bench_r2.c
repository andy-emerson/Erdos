/* bench_r2.c: one invocation of one row of round 2 (Q32 or TC) on one job (research/rule-r2.md), driven by
 * r2_runner.py.  Adapted from round 1's bench.c (kept unchanged): the scalar rows only, no library rows, no forms, and
 * a profiling mode (spec v0.2 §13.4).  Build with -DIMPL_Q32 or -DIMPL_TC, and the build's settings (none: timing;
 * SK_COUNT: counting; SK_COUNT SK_AUDIT: test; SK_PROFILE: profiling).
 *
 *   bench calib <workload> <readout>                         timing build: one rep at r = 1, 2, 4, ... until a rep
 *                                                            lasts >= 50 ms (or > 10 s: over cap)
 *   bench time  <workload> <readout> <r> <reps>              timing build: timed reps of r runs each
 *   bench mem   <workload> <readout>                         timing build: the memory run (rule-r2 item 7)
 *   bench count <workload> <readout> <percall>               counting build: one run, its counters and memory reports,
 *                                                            and the core arithmetic of every call (§14.1 item 3)
 *   bench gate  <workload> <readout> <wholes>                test build: one run audited on the gate schedule
 *   bench prof  <workload> <readout> <r> <reps> <K> <seed>   profiling build: reps with batch totals on every call and
 *                                                            the phase probes on one call in K (random gaps)
 *   bench version
 *
 * A run: create a new, empty class; every call of the workload in order; read out every stored value; destroy the
 * class.  Times (timing build: CLOCK_MONOTONIC; profiling build: fenced time-stamp-counter reads; both read only
 * between calls): entry = the calls that enter, operations = the other calls, read-out = reading every stored value.
 * create/destroy and reading the workload are not timed.  A rep runs the job r times and prints the sums.
 *
 * stdout:  calib <r> <total> <entry> <ops> <read>     rep <i> <total> <entry> <ops> <read>     overcap <total>
 *          ctr/xctr/mem/memN/wholes/memrun lines    prof lines (pf, pfcalls, pfm, prep, probe_cost, tsc_hz)
 *          ok ...
 * failures: error call=<i> op=<op> code=<c> step=<...>  exit 2       auditfail call=<i> ...  exit 3
 * read-out: int64 num[N], int64 den[N] in handle order, from the last run (the reference's format). */
#define _GNU_SOURCE
#include <inttypes.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "api_r2.h"

#define CALIB_MIN_NS 50000000ull
#define CAP_NS 10000000000ull
enum { ENTER = 0, ADD = 1, SUB = 2, MUL = 3, DIV = 4 };
static const char *const OPN[] = {"enter", "add", "sub", "mul", "div"};

static int64_t *OP, *A, *B;
static int64_t NC, NIN;
static int64_t *ON, *OD; /* read-out */
static int64_t LAST_ENTER = -1;
static uint32_t *HD;     /* the handle of each call */
static int64_t *PERCALL;
static enum { M_TIME, M_COUNT, M_GATE, M_MEM, M_PROF } MODE;
static const char *WHOLES_PATH;

static uint64_t now_ns(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec;
}
/* the clock read between calls: CLOCK_MONOTONIC in the timing build, the fenced TSC in the profiling build (§13.4
 * item 3.1; converted to ns by the rate measured over the invocation) */
#ifdef SK_PROFILE
#define CLOCK() r2_tsc()
#else
#define CLOCK() now_ns()
#endif

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

static void load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) die("cannot open workload");
    char magic[8];
    int32_t h[6];
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, "SKWLOAD1", 8)) die("bad workload magic");
    if (fread(h, 4, 6, f) != 6 || fread(&NC, 8, 1, f) != 1 || fread(&NIN, 8, 1, f) != 1) die("short header");
    if (h[1] != 64) die("width 64 only");
    OP = xmalloc(8 * (size_t)NC);
    A = xmalloc(8 * (size_t)NC);
    B = xmalloc(8 * (size_t)NC);
    if (fread(OP, 8, NC, f) != (size_t)NC || fread(A, 8, NC, f) != (size_t)NC || fread(B, 8, NC, f) != (size_t)NC)
        die("short workload");
    fclose(f);
    for (int64_t i = 0; i < NC; i++) {
        if (OP[i] < 0 || OP[i] > 4) die("bad op code");
        if (OP[i] == ENTER) {
            LAST_ENTER = i;
            if (B[i] <= 0) die("an entered denominator is not positive");
        } else if (A[i] < 0 || A[i] >= i || B[i] < 0 || B[i] >= i) die("operand is not an earlier call");
    }
    ON = xmalloc(8 * (size_t)NC);
    OD = xmalloc(8 * (size_t)NC);
    HD = xmalloc(4 * (size_t)NC);
    PERCALL = xmalloc(5 * 8 * (size_t)NC);
}

static void write_out(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) die("cannot write read-out");
    if (fwrite(ON, 8, NC, f) != (size_t)NC || fwrite(OD, 8, NC, f) != (size_t)NC) die("short write");
    fclose(f);
}

/* the in-use heap of the C library (rule-r2 item 7): uordblks + hblkhd */
static uint64_t heap_now(void) {
    struct mallinfo2 mi = mallinfo2();
    return (uint64_t)mi.uordblks + (uint64_t)mi.hblkhd;
}
static uint64_t HEAP0;

static void op_error(api_class *c, int64_t i, int rc) {
    printf("error call=%" PRId64 " op=%s code=%d step=", i, OPN[OP[i]], rc);
    api_last_error(c, stdout);
    printf("\n");
    fflush(stdout);
    exit(2);
}

static long n_full, n_changed;
#ifdef SK_AUDIT
/* the gate schedule (v0.1 §14.3 item 2): a full audit after each of the first 200 calls, after every 64th, after the
 * last; audit_changed after every call */
static void gate_audit(api_class *c, int64_t call) {
    const char *msg = "";
    int64_t k = call + 1;
    n_changed++;
    if (API(audit_changed)(c, &msg)) {
        printf("auditfail call=%" PRId64 " kind=changed msg=%s\n", call, msg);
        exit(3);
    }
    if (k <= 200 || k % 64 == 0 || k == NC) {
        n_full++;
        if (API(audit)(c, &msg)) {
            printf("auditfail call=%" PRId64 " kind=full msg=%s\n", call, msg);
            exit(3);
        }
    }
}
#endif

static void memrun_line(api_class *c, const char *when, int64_t stored) {
    sk_memory r;
    API(memory)(c, &r);
    printf("memrun %s heap=%" PRIu64 " stored=%" PRId64 " used=%" PRIu64 " allocated=%" PRIu64 "\n", when,
           heap_now() - HEAP0, stored, r.used, r.allocated);
}

/* ----------------------------------------------------------------------------------------- the profile's sampling */

#ifdef SK_PROFILE
/* the gaps between sampled calls: uniform in [1, 2K - 1] from a generator with a fixed seed (spec §13.4 item 3.2) */
static uint64_t PRNG, KSAMP, GAP;
static uint64_t splitmix(void) {
    uint64_t z = (PRNG += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}
static inline uint64_t next_gap(void) { return 1 + splitmix() % (2 * KSAMP - 1); }
static r2_profile PSUM;  /* the accumulators of the r runs of a rep */
static uint64_t SAMPLED; /* sampled calls of the rep */
static void prof_add(api_class *c) {
    r2_profile p;
    API(profile)(c, &p);
    uint64_t *d = (uint64_t *)&PSUM;
    const uint64_t *s = (const uint64_t *)&p;
    for (size_t k = 0; k < sizeof p / sizeof(uint64_t); k++) d[k] += s[k];
}
/* a call, sampled when its gap has run out: the flag set before, cleared after, and the outer pair around it */
#define CALL(c, stmt)                                                                                             \
    do {                                                                                                          \
        if (--GAP == 0) {                                                                                         \
            API(prof_sample)(c, 1);                                                                               \
            uint64_t o0_ = r2_tsc();                                                                              \
            stmt;                                                                                                 \
            uint64_t o1_ = r2_tsc();                                                                              \
            API(prof_sample)(c, 0);                                                                               \
            API(prof_outer)(c, o1_ - o0_);                                                                        \
            SAMPLED++;                                                                                            \
            GAP = next_gap();                                                                                     \
        } else {                                                                                                  \
            stmt;                                                                                                 \
        }                                                                                                         \
    } while (0)
#else
#define CALL(c, stmt)                                                                                             \
    do {                                                                                                          \
        stmt;                                                                                                     \
    } while (0)
#endif

/* ----------------------------------------------------------------------------------------- a run */

static void run(uint64_t t[3]) {
    if (MODE == M_MEM) HEAP0 = heap_now();
    api_class *c = API(create)();
    uint64_t acc[2] = {0, 0};
    int cur = OP[0] == ENTER;
#ifdef SK_COUNT
    sk_counters k0, k1;
    if (MODE == M_COUNT) API(counters)(c, &k0, NULL);
#endif
    uint64_t t0 = CLOCK();
    for (int64_t i = 0; i < NC; i++) {
        int isE = OP[i] == ENTER;
        if (isE != cur) {
            uint64_t u = CLOCK();
            acc[cur] += u - t0;
            t0 = u;
            cur = isE;
        }
        uint32_t h = 0;
        int rc;
        switch (OP[i]) {
        case ENTER: CALL(c, rc = API(enter)(c, (sk_int)A[i], (sk_int)B[i], &h)); break;
        case ADD: CALL(c, rc = API(add)(c, HD[A[i]], HD[B[i]], &h)); break;
        case SUB: CALL(c, rc = API(sub)(c, HD[A[i]], HD[B[i]], &h)); break;
        case MUL: CALL(c, rc = API(mul)(c, HD[A[i]], HD[B[i]], &h)); break;
        default: CALL(c, rc = API(div)(c, HD[A[i]], HD[B[i]], &h)); break;
        }
        if (rc != SK_OK) op_error(c, i, rc);
        HD[i] = h;
#ifdef SK_COUNT
        if (MODE == M_COUNT) { /* the core arithmetic of this call, read after every call (untimed; §14.1 item 3) */
            API(counters)(c, &k1, NULL);
            int ph = (int)OP[i];
            const sk_count *a = &k0.c[ph][SK_CORE], *b = &k1.c[ph][SK_CORE];
            int64_t *pc = PERCALL + 5 * i;
            pc[0] = (int64_t)(b->gcd - a->gcd);
            pc[1] = (int64_t)(b->gcd_work - a->gcd_work);
            pc[2] = (int64_t)(b->div_exact - a->div_exact);
            pc[3] = (int64_t)(b->mul - a->mul);
            pc[4] = (int64_t)(b->add - a->add);
            k0 = k1;
        }
#endif
        if (i == LAST_ENTER) {
            if (MODE == M_COUNT || MODE == M_GATE) api_dump_memory(c, "after_entry", stdout);
            if (MODE == M_MEM) memrun_line(c, "after_entry", i + 1);
            if (MODE == M_GATE) printf("wholes after_entry %u\n", api_wholes(c));
        }
#ifdef SK_AUDIT
        if (MODE == M_GATE) gate_audit(c, i);
#endif
    }
    uint64_t t1 = CLOCK();
    acc[cur] += t1 - t0;
    if (MODE == M_MEM) memrun_line(c, "end", NC);
    uint64_t t1b = CLOCK();
    for (int64_t i = 0; i < NC; i++) {
        sk_int m = 0, d = 0;
        int rc;
        CALL(c, rc = API(read)(c, HD[i], &m, &d));
        if (rc != SK_OK) op_error(c, i, -1);
        ON[i] = m;
        OD[i] = d;
    }
    uint64_t t2 = CLOCK();
    t[0] += acc[1];
    t[1] += acc[0];
    t[2] += t2 - t1b;
    if (MODE == M_COUNT || MODE == M_GATE) {
        api_dump_memory(c, "end", stdout);
        api_dump_counters(c, stdout);
    }
    if (MODE == M_GATE) printf("wholes end %u\n", api_wholes(c));
#ifdef SK_AUDIT
    if (MODE == M_GATE && WHOLES_PATH) { /* Q32's whole-number list in index order */
        FILE *f = fopen(WHOLES_PATH, "wb");
        if (!f) die("cannot write the whole-number list");
        api_write_wholes(c, f);
        fclose(f);
    }
#endif
#ifdef SK_PROFILE
    prof_add(c);
#endif
    API(destroy)(c);
}

static void rep(uint64_t r, uint64_t t[3]) {
    t[0] = t[1] = t[2] = 0;
    for (uint64_t i = 0; i < r; i++) run(t);
}

static void line(const char *kind, uint64_t a, uint64_t t[3]) {
    uint64_t tot = t[0] + t[1] + t[2];
    printf("%s %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " row=%s\n", kind, a, tot, t[0], t[1], t[2],
           IMPL_NAME);
}

#ifdef SK_PROFILE
/* the profile of one rep: every nonzero accumulator, in cycles (the runner converts with tsc_hz) */
static void prof_lines(uint64_t i) {
    for (int jp = 0; jp < R2_NJP; jp++) {
        for (int k = 0; k < R2_NCASEX; k++) {
            if (PSUM.calls[jp][k])
                printf("pfcalls %" PRIu64 " %d %s %" PRIu64 " %" PRIu64 "\n", i, jp, r2_case_name(k), PSUM.calls[jp][k],
                       PSUM.outer[jp][k]);
            for (int ph = 0; ph < R2_NPF; ph++)
                if (PSUM.segs[jp][k][ph])
                    printf("pf %" PRIu64 " %d %s %d %" PRIu64 " %" PRIu64 "\n", i, jp, r2_case_name(k), ph,
                           PSUM.cyc[jp][k][ph], PSUM.segs[jp][k][ph]);
        }
        if (PSUM.m_n[jp]) printf("pfm %" PRIu64 " %d %" PRIu64 " %" PRIu64 "\n", i, jp, PSUM.m_cyc[jp], PSUM.m_n[jp]);
    }
}
#endif

/* stdout's buffer, static so that it is never on the heap: otherwise the C library allocates it (4 KB) at the first
 * printf, which in the memory run comes after the after_entry heap figure is read and before the end figure, so only
 * B_heap at the end would include it.  With this buffer neither figure includes it (rule-r2 item 7). */
static char OUTBUF[BUFSIZ];

int main(int argc, char **argv) {
    setvbuf(stdout, OUTBUF, _IOFBF, sizeof OUTBUF);
    /* as round 1: freed memory is kept by malloc, so later runs reuse it (rule-r2 item 7) */
    mallopt(M_MMAP_THRESHOLD, 1 << 30);
    mallopt(M_TRIM_THRESHOLD, 1 << 30);
    if (argc >= 2 && !strcmp(argv[1], "version")) {
        printf("version row=%s build=%s\n", IMPL_NAME, BUILD_NAME);
        return 0;
    }
    if (argc < 4) die("usage: bench calib|time|mem|count|gate|prof <workload> <readout> [...]");
    const char *mode = argv[1];
    load(argv[2]);
    uint64_t t[3];
    int is = 0;
#define NEED(m, b)                                                                                                \
    if (!strcmp(mode, m) && strcmp(BUILD_NAME, b)) die(m " needs the " b " build");
    NEED("calib", "timing")
    NEED("time", "timing")
    NEED("mem", "timing")
    NEED("count", "counting")
    NEED("gate", "test")
    NEED("prof", "profiling")
    if (!strcmp(mode, "calib")) {
        is = 1;
        MODE = M_TIME;
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
    }
    if (!strcmp(mode, "count")) {
        is = 1;
        if (argc < 5) die("count needs the per-call output path");
        MODE = M_COUNT;
        rep(1, t);
        FILE *f = fopen(argv[4], "wb");
        if (!f || fwrite(PERCALL, 8, 5 * (size_t)NC, f) != 5 * (size_t)NC) die("cannot write the per-call counts");
        fclose(f);
    }
    if (!strcmp(mode, "gate")) {
        is = 1;
        MODE = M_GATE;
        if (argc > 4) WHOLES_PATH = argv[4];
        rep(1, t);
        printf("audits full=%ld changed=%ld\n", n_full, n_changed);
    }
#ifdef SK_PROFILE
    if (!strcmp(mode, "prof")) {
        is = 1;
        if (argc < 8) die("prof needs r, reps, K and the seed");
        MODE = M_PROF;
        uint64_t r = strtoull(argv[4], 0, 10), reps = strtoull(argv[5], 0, 10);
        KSAMP = strtoull(argv[6], 0, 10);
        PRNG = strtoull(argv[7], 0, 10);
        if (KSAMP < 1) die("K must be at least 1");
        double cost = r2_probe_cost(1000, 1000); /* spec §13.4 item 6 */
        uint64_t w0 = now_ns(), c0 = r2_tsc();
        GAP = next_gap();
        uint64_t (*cyc)[3] = xmalloc(sizeof *cyc * reps);
        r2_profile *ps = xmalloc(sizeof *ps * reps);
        uint64_t *ns = xmalloc(sizeof *ns * reps);
        for (uint64_t i = 0; i < reps; i++) {
            memset(&PSUM, 0, sizeof PSUM);
            SAMPLED = 0;
            rep(r, cyc[i]);
            ps[i] = PSUM;
            ns[i] = SAMPLED;
        }
        uint64_t w1 = now_ns(), c1 = r2_tsc();
        double hz = (double)(c1 - c0) / ((double)(w1 - w0) / 1e9); /* the TSC rate over the invocation */
        printf("probe_cost %.4f\n", cost);
        printf("tsc_hz %.1f\n", hz);
        printf("prof_k %" PRIu64 "\n", KSAMP);
        for (uint64_t i = 0; i < reps; i++) {
            double f = 1e9 / hz;
            uint64_t e = (uint64_t)(cyc[i][0] * f + 0.5), o = (uint64_t)(cyc[i][1] * f + 0.5),
                     d = (uint64_t)(cyc[i][2] * f + 0.5);
            printf("prep %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " sampled=%" PRIu64 " row=%s\n", i,
                   e + o + d, e, o, d, ns[i], IMPL_NAME);
            PSUM = ps[i];
            prof_lines(i);
        }
        free(cyc);
        free(ps);
        free(ns);
    }
#endif
    if (!is) die("unknown mode, or not in this build");
    write_out(argv[3]);
    printf("ok impl=%s calls=%" PRId64 "\n", IMPL_NAME, NC);
    return 0;
}
