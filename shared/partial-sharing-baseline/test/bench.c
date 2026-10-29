/* bench.c: one invocation of one row on one job (research/rule.md item 4), driven by runner.py.
 *
 *   bench calib <workload> <readout>             calibration: one rep at r = 1, 2, 4, ... until a rep lasts >= 50 ms
 *   bench time  <workload> <readout> <r> <reps>  timed reps of r runs each
 *   bench count <workload> <readout>             counting build: one run, counters and memory reports
 *   bench gate  <workload> <readout>             test build: one run audited on the gate schedule (§13.3)
 *
 * Built once per implementation (IMPL_CTL, IMPL_D1, IMPL_D2, IMPL_CEIL), width and build mode; see api.h.
 *
 * A run: create a new, empty class; every call of the workload in order (entries and operations; every call stores
 * one value, its handle is the call index); read out every stored value in lowest terms; destroy the class.
 * Times (CLOCK_MONOTONIC, read only between calls): entry = the calls that enter, operations = the other calls,
 * read-out = reading every stored value; total = entry + operations + read-out.  create and destroy are not timed,
 * and neither is reading the workload file.  A rep runs the job r times; it prints the sums over its r runs.
 * The ceiling (IMPL_CEIL) runs the vector plan of rule.md (Ceiling jobs) for the same workload; gathers are timed as
 * operations.
 *
 * stdout lines:  calib <r> <total> <entry> <ops> <read>      rep <i> <total> <entry> <ops> <read>
 *                overcap <total>                              (a rep over 10 s; nothing more is timed)
 *                ctr/hist/mem/memN ...                        (count; api.h)
 *                audits full=<n> changed=<n>                  (gate)      ok
 * failures:      error call=<i> op=<op> code=<c> step=<...>  exit 2      auditfail call=<i> kind=<..> msg=<..>  exit 3
 * readout file:  int64 num[N], int64 den[N] in handle order, from the last run.
 */
#define _GNU_SOURCE
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "api.h"

#define CALIB_MIN_NS 50000000ull
#define CAP_NS 10000000000ull
enum { ENTER = 0, ADD = 1, SUB = 2, MUL = 3, DIV = 4 };
static const char *const OPN[] = {"enter", "add", "sub", "mul", "div"};

static int64_t *OP, *A, *B;
static int64_t NC, NIN;
static int32_t JOB, WIDTH, CEILK, NCH, KST;
static uint32_t *HD;            /* handle returned by each call */
static int64_t *ON, *OD;        /* read-out */
static int64_t LAST_ENTER = -1;
static enum { M_TIME, M_COUNT, M_GATE } MODE;

static uint64_t now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec;
}
static void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) { fprintf(stderr, "out of memory\n"); exit(1); }
    return p;
}
static void die(const char *m) { fprintf(stderr, "bench: %s\n", m); exit(1); }

static void load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) die("cannot open workload");
    char magic[8];
    int32_t h[6];
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, "SKWLOAD1", 8)) die("bad workload magic");
    if (fread(h, 4, 6, f) != 6 || fread(&NC, 8, 1, f) != 1 || fread(&NIN, 8, 1, f) != 1) die("short header");
    JOB = h[0]; WIDTH = h[1]; CEILK = h[2]; NCH = h[3]; KST = h[4];
    if (WIDTH != SK_WIDTH) die("workload width differs from the build's SK_WIDTH");
    OP = xmalloc(8 * (size_t)NC); A = xmalloc(8 * (size_t)NC); B = xmalloc(8 * (size_t)NC);
    if (fread(OP, 8, NC, f) != (size_t)NC || fread(A, 8, NC, f) != (size_t)NC || fread(B, 8, NC, f) != (size_t)NC)
        die("short workload");
    fclose(f);
    for (int64_t i = 0; i < NC; i++) {
        if (OP[i] < 0 || OP[i] > 4) die("bad op code");
        if (OP[i] == ENTER) LAST_ENTER = i;
        else if (A[i] < 0 || A[i] >= i || B[i] < 0 || B[i] >= i) die("operand is not an earlier call");
    }
    HD = xmalloc(4 * (size_t)NC);
    ON = xmalloc(8 * (size_t)NC); OD = xmalloc(8 * (size_t)NC);
}

static void write_out(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) die("cannot write read-out");
    if (fwrite(ON, 8, NC, f) != (size_t)NC || fwrite(OD, 8, NC, f) != (size_t)NC) die("short write");
    fclose(f);
}

/* the gate schedule (§13.3): full audit after each of the first 200 calls, after every 64th, after the last;
 * audit_changed after every call (MAY in general, MUST for Design 2 above 1,024 pieces: run always). */
static long n_full, n_changed;
#ifdef BENCH_AUDIT
static void gate_audit(api_class *c, int64_t call, int64_t ncalls) {
    const char *msg = "";
    int64_t k = call + 1;
    n_changed++;
    if (api_audit_changed(c, &msg)) {
        printf("auditfail call=%" PRId64 " kind=changed msg=%s\n", call, msg);
        exit(3);
    }
    if (k <= 200 || k % 64 == 0 || k == ncalls) {
        n_full++;
        if (api_audit(c, k == ncalls, &msg)) {
            printf("auditfail call=%" PRId64 " kind=full msg=%s\n", call, msg);
            exit(3);
        }
    }
}
#endif

static void op_error(api_class *c, int64_t i, int rc) {
    printf("error call=%" PRId64 " op=%s code=%d step=", i, OPN[OP[i]], rc);
    api_last_error(c, stdout);
    printf("\n");
    fflush(stdout);
    exit(2);
}

#ifndef IMPL_CEIL
/* ------------------------------------------------------------------------------------------- scalar rows */
static void run(uint64_t t[3]) {
    api_class *c = API(create)();
    uint64_t acc[2] = {0, 0};
    int cur = OP[0] == ENTER;
    uint64_t t0 = now();
    for (int64_t i = 0; i < NC; i++) {
        int isE = OP[i] == ENTER;
        if (isE != cur) { uint64_t u = now(); acc[cur] += u - t0; t0 = u; cur = isE; }
        uint32_t h;
        int rc;
        switch (OP[i]) {
        case ENTER: rc = API(enter)(c, (sk_int)A[i], (sk_int)B[i], &h); break;
        case ADD: rc = API(add)(c, HD[A[i]], HD[B[i]], &h); break;
        case SUB: rc = API(sub)(c, HD[A[i]], HD[B[i]], &h); break;
        case MUL: rc = API(mul)(c, HD[A[i]], HD[B[i]], &h); break;
        default: rc = API(div)(c, HD[A[i]], HD[B[i]], &h); break;
        }
        if (rc != SK_OK) op_error(c, i, rc);
        HD[i] = h;
#ifdef BENCH_COUNTING
        if (MODE == M_COUNT && i == LAST_ENTER) {
            api_dump_memory(c, "after_entry", stdout);
        }
#endif
#ifdef BENCH_AUDIT
        if (MODE == M_GATE) gate_audit(c, i, NC);
#endif
    }
    uint64_t t1 = now();
    acc[cur] += t1 - t0;
    for (int64_t i = 0; i < NC; i++) {
        sk_int m, d;
        if (API(read)(c, HD[i], &m, &d) != SK_OK) op_error(c, i, -1);
        ON[i] = m; OD[i] = d;
    }
    uint64_t t2 = now();
    t[0] += acc[1]; t[1] += acc[0]; t[2] += t2 - t1;
    if (MODE == M_COUNT) {
        api_dump_memory(c, "end", stdout);
        api_dump_counters(c, stdout);
    }
    API(destroy)(c);
}
static void plan(void) {}
#else
/* ------------------------------------------------------------------------------------------- the ceiling */
/* Where each call's value lives: vector slot (0 = inputs, 1.. = stored results) and position. */
static int32_t *VS;
static uint32_t *VP;
static sk_int *EM;               /* entered numerators */
static sk_int ED;
static uint32_t *P1, *P2, *P3, *P4; /* ALL-add: two-input operands (inputs), result-op operands (R1, inputs) */
static uint32_t N1, N2;
static unsigned char *NEG;       /* CH-all: per step, per chain */
static uint32_t *YP, *X0;
static uint32_t VEC[64];

static void plan(void) {
    if (CEILK != 1 && CEILK != 2) die("the ceiling runs only on its two jobs (ALL-add, CH-all)");
    VS = xmalloc(4 * (size_t)NC); VP = xmalloc(4 * (size_t)NC);
    EM = xmalloc(sizeof(sk_int) * (size_t)NIN);
    for (int64_t i = 0; i < NIN; i++) {
        if (OP[i] != ENTER) die("ceiling: entries must come first");
        if (i == 0) ED = (sk_int)B[0];
        if ((sk_int)B[i] != ED) die("ceiling: entries over more than one denominator");
        EM[i] = (sk_int)A[i]; VS[i] = 0; VP[i] = (uint32_t)i;
    }
    if (CEILK == 1) {
        P1 = xmalloc(4 * (size_t)NC); P2 = xmalloc(4 * (size_t)NC); P3 = xmalloc(4 * (size_t)NC); P4 = xmalloc(4 * (size_t)NC);
        for (int64_t i = NIN; i < NC; i++) {
            if (OP[i] != ADD) die("ceiling: ALL-add has additions only");
            if (A[i] < NIN && B[i] < NIN) { VS[i] = 1; VP[i] = N1; P1[N1] = (uint32_t)A[i]; P2[N1++] = (uint32_t)B[i]; }
            else if (A[i] >= NIN && VS[A[i]] == 1 && B[i] < NIN) { VS[i] = 2; VP[i] = N2; P3[N2] = VP[A[i]]; P4[N2++] = (uint32_t)B[i]; }
            else die("ceiling: an operation is neither two-input nor a result operation");
        }
    } else {
        if (NIN + (int64_t)NCH * KST != NC || KST + 1 > 64) die("ceiling: CH-all shape");
        YP = xmalloc(4 * (size_t)NC); X0 = xmalloc(4 * (size_t)NCH); NEG = xmalloc((size_t)NC);
        for (int k = 0; k < KST; k++)
            for (int c = 0; c < NCH; c++) {
                int64_t i = NIN + (int64_t)k * NCH + c;
                if (OP[i] != ADD && OP[i] != SUB) die("ceiling: CH-all has add and sub only");
                if (k == 0) { if (A[i] >= NIN) die("ceiling: chain start"); X0[c] = (uint32_t)A[i]; }
                else if (A[i] != i - NCH) die("ceiling: chain order");
                if (B[i] >= NIN) die("ceiling: y_k must be an input");
                YP[(int64_t)k * NCH + c] = (uint32_t)B[i];
                NEG[(int64_t)k * NCH + c] = OP[i] == SUB;
                VS[i] = 1 + k; VP[i] = (uint32_t)c;
            }
    }
}

static int64_t step_call(int s) {   /* the call whose value a stored vector holds first: for error reports */
    if (CEILK == 1) return s == 1 ? NIN : NC - 1;
    return NIN + (int64_t)s * NCH;
}
static void cgate(api_class *c, int64_t k, int64_t n) {
#ifdef BENCH_AUDIT
    if (MODE == M_GATE) gate_audit(c, k, n);
#else
    (void)c; (void)k; (void)n;
#endif
}

static void run(uint64_t t[3]) {
    api_class *c = API(create)();
    int rc;
    uint32_t w1, w2;
    int64_t ncalls = CEILK == 1 ? 3 : 1 + KST;   /* storing calls: enter_vector and vector operations */
    uint64_t t0 = now();
    if ((rc = API(enter_vector)(c, EM, (uint32_t)NIN, ED, &VEC[0])) != SK_OK) op_error(c, 0, rc);
    uint64_t t1 = now();
    if (MODE == M_COUNT) api_dump_memory(c, "after_entry", stdout);
    cgate(c, 0, ncalls);
    t1 = now();
    uint64_t gated = 0;    /* time spent in audits between vector operations (gate only), excluded */
    if (CEILK == 1) {
        if ((rc = api_gather(c, VEC[0], P1, NULL, N1, &w1)) || (rc = api_gather(c, VEC[0], P2, NULL, N1, &w2)) ||
            (rc = API(add)(c, w1, w2, &VEC[1]))) op_error(c, step_call(1), rc);
        uint64_t a = now(); cgate(c, 1, ncalls); gated += now() - a;
        if ((rc = api_gather(c, VEC[1], P3, NULL, N2, &w1)) || (rc = api_gather(c, VEC[0], P4, NULL, N2, &w2)) ||
            (rc = API(add)(c, w1, w2, &VEC[2]))) op_error(c, step_call(2), rc);
        a = now(); cgate(c, 2, ncalls); gated += now() - a;
    } else {
        uint32_t x;
        if ((rc = api_gather(c, VEC[0], X0, NULL, (uint32_t)NCH, &x))) op_error(c, step_call(0), rc);
        for (int k = 0; k < KST; k++) {
            if ((rc = api_gather(c, VEC[0], YP + (int64_t)k * NCH, NEG + (int64_t)k * NCH, (uint32_t)NCH, &w2)) ||
                (rc = API(add)(c, x, w2, &VEC[1 + k]))) op_error(c, step_call(k), rc);
            x = VEC[1 + k];
            uint64_t a = now(); cgate(c, 1 + k, ncalls); gated += now() - a;
        }
    }
    uint64_t t2 = now();
    for (int64_t i = 0; i < NC; i++) {
        sk_int m, d;
        if (API(read)(c, VEC[VS[i]], VP[i], &m, &d) != SK_OK) op_error(c, i, -1);
        ON[i] = m; OD[i] = d;
    }
    uint64_t t3 = now();
    t[0] += t1 - t0; t[1] += t2 - t1 - gated; t[2] += t3 - t2;
    if (MODE == M_COUNT) {
        api_dump_memory(c, "end", stdout);
        api_dump_counters(c, stdout);
    }
    API(destroy)(c);
}
#endif

static void rep(uint64_t r, uint64_t t[3]) {
    t[0] = t[1] = t[2] = 0;
    for (uint64_t i = 0; i < r; i++) run(t);
}

int main(int argc, char **argv) {
    if (argc < 4) die("usage: bench calib|time|count|gate <workload> <readout> [r reps]");
    const char *mode = argv[1];
    load(argv[2]);
    plan();
    uint64_t t[3];
#if defined(BENCH_COUNTING) || defined(BENCH_AUDIT)
    int timing_build = 0;
#else
    int timing_build = 1;
#endif
#ifdef BENCH_AUDIT
    int test_build = 1;
#else
    int test_build = 0;
#endif
    if ((!strcmp(mode, "calib") || !strcmp(mode, "time")) && !timing_build) die("calib/time need the timing build");
    if (!strcmp(mode, "count") && (timing_build || test_build)) die("count needs the counting build");
    if (!strcmp(mode, "gate") && !test_build) die("gate needs the test build");
    if (!strcmp(mode, "calib")) {
        MODE = M_TIME;
        for (uint64_t r = 1; r <= (1ull << 40); r *= 2) {
            rep(r, t);
            uint64_t tot = t[0] + t[1] + t[2];
            printf("calib %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 "\n", r, tot, t[0], t[1], t[2]);
            if (tot > CAP_NS) { printf("overcap %" PRIu64 "\n", tot); break; }
            if (tot >= CALIB_MIN_NS) break;
        }
    } else if (!strcmp(mode, "time")) {
        if (argc < 6) die("time needs r and reps");
        MODE = M_TIME;
        uint64_t r = strtoull(argv[4], 0, 10), reps = strtoull(argv[5], 0, 10);
        for (uint64_t i = 0; i < reps; i++) {
            rep(r, t);
            uint64_t tot = t[0] + t[1] + t[2];
            printf("rep %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 "\n", i, tot, t[0], t[1], t[2]);
            if (tot > CAP_NS) { printf("overcap %" PRIu64 "\n", tot); break; }
        }
    } else if (!strcmp(mode, "count")) {
        MODE = M_COUNT;
        rep(1, t);
    } else if (!strcmp(mode, "gate")) {
        MODE = M_GATE;
        rep(1, t);
        printf("audits full=%ld changed=%ld\n", n_full, n_changed);
    } else die("unknown mode");
    write_out(argv[3]);
    printf("ok impl=%s job=%d width=%d calls=%" PRId64 "\n", IMPL_NAME, JOB, SK_WIDTH, NC);
    return 0;
}
