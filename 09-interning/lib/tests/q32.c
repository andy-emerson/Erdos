/* q32.c: the Q32 rows against the Q64 rows, the lookup gate, and capacity (spec §18.3; test build).
 *
 *   1. Q32 against Q64 (spec §18.3 item 1): on random workloads, L1-Q32 against L1 and L1-opt-Q32 against L1-opt
 *      (the same trick mask): every stored value equal to the exact reference; the same whole-number list in order;
 *      the same fields (tag, numerator index, denominator index) for every rational; every counter equal, O-6's
 *      included (both keep 32-bit fingerprints); the memory report as §18.1 item 6 states it.
 *   2. The lookup gate (spec §18.1 item 4), in miniature: the build of L1 whose lookup is the parameterized lookup at
 *      32-bit slots gives every counter, probes included, the values, the fields and the memory report of L1, whose
 *      lookup is 08's sk_lookup.  (The gate itself runs on every job of the rule: test/runner.py.)
 *   3. Capacity (spec §18.1 items 2-3, §18.3 item 2): each Q32 row filled to exactly 32,768 whole numbers (indices 0
 *      to 32,767) and fully audited, every field read back; then, in a child process, the next new whole number, by
 *      entry and by an operation, aborts with the capacity message and stores nothing; L1 and the gate's build go on
 *      past 32,768.
 *
 *   q32 [--seeds N]   */
#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "t_common.h"

enum { NMAX = 3000 };

typedef struct wl {
    int n;
    int op[NMAX];
    sk_int m[NMAX], d[NMAX];
    int x[NMAX], y[NMAX];
    ref_q v[NMAX];
} wl;

static i128 H(ref_q q) { return ref_abs(q.n) > q.d ? ref_abs(q.n) : q.d; }

/* as diff.c: shape 0 tiny whole numbers, 1 a pool of composites with zeros, 2 wide values, 3 money */
static void make(wl *w, int shape, int nin, int nops) {
    w->n = 0;
    sk_int pool[40];
    for (int k = 0; k < 40; k++) pool[k] = 1 + (sk_int)rndn(60) * (sk_int)(1 + rndn(30));
    for (int i = 0; i < nin; i++) {
        sk_int a, b;
        switch (shape) {
        case 0: a = (sk_int)rndn(8); b = 1 + (sk_int)rndn(8); break;
        case 1: a = pool[rndn(40)]; b = pool[rndn(40)]; if (rndn(8) == 0) a = 0; break;
        case 2: a = (sk_int)rndn(1u << 20); b = 1 + (sk_int)rndn(1u << 20); break;
        default: a = (sk_int)rndn(2000000) - 1000000; b = 100; break;
        }
        if (rndn(2)) a = -a;
        if (rndn(4) == 0) b = -b;
        w->op[w->n] = 0;
        w->m[w->n] = a;
        w->d[w->n] = b;
        w->v[w->n++] = ref_make(a, b);
    }
    int mix = (int)rndn(4);
    for (int t = 0; t < nops && w->n < NMAX; t++) {
        for (int tries = 0; tries < 400; tries++) {
            int op = 1 + (int)rndn(4);
            if (mix == 1) op = 1 + (int)rndn(2);
            if (mix == 2) op = 3 + (int)rndn(2);
            int x = (int)rndn((uint64_t)w->n), y = (int)rndn((uint64_t)w->n);
            if (mix == 3 && w->n > nin) x = w->n - 1;
            if (rndn(6) == 0) y = x;
            if (op == SK_PH_DIV && w->v[y].n == 0) continue;
            if (H(w->v[x]) * H(w->v[y]) > (i128)2147483647 * 2147483647) continue;
            w->op[w->n] = op;
            w->x[w->n] = x;
            w->y[w->n] = y;
            w->v[w->n] = ref_op(op, w->v[x], w->v[y]);
            w->n++;
            break;
        }
    }
}

static void run(const row *R, void *c, const wl *w, sk_handle *h) {
    for (int i = 0; i < w->n; i++) {
        sk_status s = w->op[i] ? R->op[w->op[i]](c, h[w->x[i]], h[w->y[i]], &h[i]) : R->enter(c, w->m[i], w->d[i], &h[i]);
        CHECK(s == SK_OK);
        if (s != SK_OK) return;
        sk_int m, d;
        R->read(c, h[i], &m, &d);
        if ((i128)m != w->v[i].n || (i128)d != w->v[i].d) {
            fprintf(stderr, "  %s call %d: %lld/%lld, want %lld/%lld\n", R->name, i, (long long)m, (long long)d,
                    (long long)w->v[i].n, (long long)w->v[i].d);
            CHECK(0);
        }
    }
}

static void audit_full(const row *R, void *c) {
    const char *why = NULL;
    sk_status s = lw_audit(R, c, 1, &why);
    if (s != SK_OK) fprintf(stderr, "  %s audit: %s\n", R->name, why ? why : "?");
    CHECK(s == SK_OK);
}

/* B against A on one workload: the same whole-number list, the same fields, every counter (o6 included) */
static void same_structure(const row *A, void *a, const sk_handle *ha, const row *B, void *b, const sk_handle *hb,
                           int n, const char *what) {
    uint32_t w = lw_wholes(A, a);
    CHECK_EQ(lw_wholes(B, b), w);
    for (uint32_t k = 0; k < w; k++)
        if (lw_whole(A, a, k) != lw_whole(B, b, k)) {
            fprintf(stderr, "  %s: whole number %u differs\n", what, k);
            CHECK(0);
            break;
        }
    for (int i = 0; i < n; i++) {
        int na, nb;
        uint32_t ia, ja, ib, jb;
        lw_rat(A, a, ha[i], &na, &ia, &ja);
        lw_rat(B, b, hb[i], &nb, &ib, &jb);
        if (na != nb || ia != ib || ja != jb) {
            fprintf(stderr, "  %s: rational %d fields differ\n", what, i);
            CHECK(0);
            break;
        }
    }
    sk_counters ka, kb;
    l1_xcounters xa, xb;
    lw_counters(A, a, &ka, &xa);
    lw_counters(B, b, &kb, &xb);
    /* O-7's bytes avoided are rational-list bytes: B's entry size over A's times A's (spec §18.3 item 1) */
    for (int ph = 0; ph < SK_NPHASE; ph++)
        for (int p = 0; p < SK_NPART; p++)
            xa.c[ph][p].o7_bytes_avoided = xa.c[ph][p].o7_bytes_avoided / A->rat_bytes * B->rat_bytes;
    if (memcmp(&ka, &kb, sizeof ka)) {
        fprintf(stderr, "  %s: 08 counters differ\n", what);
        CHECK(0);
    }
    if (memcmp(&xa, &xb, sizeof xa)) {
        fprintf(stderr, "  %s: layer-1 counters differ\n", what);
        CHECK(0);
    }
}

/* the memory report of B against A's, for slot and entry sizes of B */
static void memory_as(const row *A, void *a, const row *B, void *b) {
    sk_memory ma, mb;
    lw_memory(A, a, &ma);
    lw_memory(B, b, &mb);
    uint64_t S = ma.item[1].used / A->slot_bytes, N = A->size(a);
    CHECK_EQ(mb.item[0].used, ma.item[0].used);             /* whole numbers: 8W */
    CHECK_EQ(mb.item[1].used, S * B->slot_bytes);           /* lookup: the same S slots */
    CHECK_EQ(mb.item[2].used, N * B->rat_bytes);            /* rationals */
    double want = (double)B->rat_bytes + (double)(ma.item[0].used + S * B->slot_bytes) / (double)N;
    CHECK(mb.used_per_rational > want - 1e-9 && mb.used_per_rational < want + 1e-9);
}

static void test_against_q64(int seeds) {
    static wl w;
    static sk_handle h[5][NMAX];
    long pairs = 0;
    uint64_t o6 = 0;
    for (int seed = 0; seed < seeds; seed++) {
        rng_s = 0x9E3779B97F4A7C15ull * (uint64_t)(seed + 7) + 99991;
        make(&w, seed % 4, 50 + (int)rndn(400), 200 + (int)rndn(2000));
        unsigned mask = seed % 3 == 0 ? L1O_ALL : (unsigned)rndn(L1O_ALL + 1);
        const row *R[5] = {&ROW_L1_64, &ROW_L1Q, &ROW_L1O_64, &ROW_L1OQ, &ROW_L1G};
        void *c[5];
        for (int r = 0; r < 5; r++) c[r] = R[r]->create();
        l1o_t_tricks((l1o_class *)c[2], mask);
        l1oq_t_tricks((l1oq_class *)c[3], mask);
        l1o_reserve((l1o_class *)c[2], (uint32_t)w.n);
        l1oq_reserve((l1oq_class *)c[3], (uint32_t)w.n);
        for (int r = 0; r < 5; r++) run(R[r], c[r], &w, h[r]);
        same_structure(R[0], c[0], h[0], R[1], c[1], h[1], w.n, "L1-Q32 against L1");
        same_structure(R[2], c[2], h[2], R[3], c[3], h[3], w.n, "L1-opt-Q32 against L1-opt");
        same_structure(R[0], c[0], h[0], R[4], c[4], h[4], w.n, "lookup gate: PLK at 32-bit slots against sk_lookup");
        memory_as(R[0], c[0], R[1], c[1]);
        memory_as(R[2], c[2], R[3], c[3]);
        memory_as(R[0], c[0], R[4], c[4]);
        l1_xcounters x;
        lw_counters(R[3], c[3], NULL, &x);
        for (int ph = 0; ph < SK_NPHASE; ph++) o6 += x.c[ph][SK_STORE].o6_reads_avoided;
        for (int r = 0; r < 5; r++) {
            audit_full(R[r], c[r]);
            R[r]->destroy(c[r]);
        }
        pairs += 3;
    }
    CHECK(o6 > 0); /* O-6 fired in L1-opt-Q32, and equal to L1-opt's */
    printf("  Q32 against Q64, and the lookup gate: %d workloads, %ld comparisons\n", seeds, pairs);
}

/* ----------------------------------------------------------------------------------------- capacity */

enum { CAP = 32768 };

/* run fn(c) in a child; true if it aborted (SIGABRT) and its stderr holds msg */
static bool aborts_with(const row *R, void *c, int which, sk_handle a, sk_handle b, const char *msg) {
    int fd[2];
    if (pipe(fd)) return false;
    fflush(stdout);
    fflush(stderr);
    pid_t pid = fork();
    if (pid == 0) {
        close(fd[0]);
        dup2(fd[1], 2);
        sk_handle h;
        if (which == 0) R->enter(c, CAP, 1, &h);  /* a new whole number by entry: 32768 */
        else R->op[SK_PH_ADD](c, a, b, &h);        /* by an operation: 32767/1 + 1/1 = 32768/1 */
        _exit(0);                                  /* not reached if the capacity check aborts */
    }
    close(fd[1]);
    char buf[4096];
    size_t n = 0;
    ssize_t k;
    while (n < sizeof buf - 1 && (k = read(fd[0], buf + n, sizeof buf - 1 - n)) > 0) n += (size_t)k;
    buf[n] = 0;
    close(fd[0]);
    int st = 0;
    waitpid(pid, &st, 0);
    bool ok = WIFSIGNALED(st) && WTERMSIG(st) == SIGABRT && strstr(buf, msg) != NULL;
    if (!ok) fprintf(stderr, "  %s: child status %d, stderr: %s\n", R->name, st, buf);
    return ok;
}

static void test_capacity(void) {
    const row *Q[2] = {&ROW_L1Q, &ROW_L1OQ};
    static sk_handle h[CAP];
    for (int r = 0; r < 2; r++) {
        const row *R = Q[r];
        void *c = R->create();
        /* v/1 for v = 0 .. 32767: whole numbers 0 .. 32767 at indices 0 .. 32767 (0, then 1, then one new each) */
        for (sk_int v = 0; v < CAP; v++) {
            CHECK(R->enter(c, v, 1, &h[v]) == SK_OK);
        }
        CHECK_EQ(lw_wholes(R, c), CAP);
        audit_full(R, c);
        bool fields_ok = true;
        for (sk_int v = 0; v < CAP; v++) {
            int neg;
            uint32_t i, j;
            sk_int m, d;
            lw_rat(R, c, h[v], &neg, &i, &j);
            R->read(c, h[v], &m, &d);
            fields_ok &= !neg && i == (uint32_t)v && j == 1 && lw_whole(R, c, i) == v && m == v && d == 1;
        }
        CHECK(fields_ok);
        /* the largest index with the tag set: the numerator field is all ones (0xFFFF), a valid field */
        sk_handle hn, h1 = h[1], hmax = h[CAP - 1];
        CHECK(R->enter(c, -(CAP - 1), 1, &hn) == SK_OK);
        int neg;
        uint32_t i, j;
        lw_rat(R, c, hn, &neg, &i, &j);
        CHECK(neg == 1 && i == CAP - 1 && j == 1);
        sk_int m, d;
        R->read(c, hn, &m, &d);
        CHECK(m == -(CAP - 1) && d == 1);
        /* results whose whole numbers are all found still store at full capacity */
        sk_handle hs;
        CHECK(R->op[SK_PH_ADD](c, h1, h1, &hs) == SK_OK); /* 2/1 */
        CHECK(R->op[SK_PH_DIV](c, h1, hmax, &hs) == SK_OK); /* 1/32767 */
        R->read(c, hs, &m, &d);
        CHECK(m == 1 && d == CAP - 1);
        CHECK_EQ(lw_wholes(R, c), CAP);
        audit_full(R, c);
        sk_memory mem;
        lw_memory(R, c, &mem);
        CHECK_EQ(mem.item[1].used, (uint64_t)65536 * R->slot_bytes); /* S = 2^16: half full at 32,768 */
        /* the next new whole number aborts, by entry and by an operation; nothing wraps */
        uint32_t n = R->size(c);
        CHECK(aborts_with(R, c, 0, 0, 0, "capacity of 32,768 whole numbers exceeded"));
        CHECK(aborts_with(R, c, 1, hmax, h1, "capacity of 32,768 whole numbers exceeded"));
        CHECK_EQ(R->size(c), n); /* the parent's class is untouched */
        R->destroy(c);
    }
    /* L1 (Q64) and the gate's build are not limited at 32,768 */
    const row *Z[2] = {&ROW_L1_64, &ROW_L1G};
    for (int r = 0; r < 2; r++) {
        void *c = Z[r]->create();
        sk_handle hh;
        for (sk_int v = 0; v < CAP + 100; v++) CHECK(Z[r]->enter(c, v, 1, &hh) == SK_OK);
        CHECK_EQ(lw_wholes(Z[r], c), CAP + 100);
        audit_full(Z[r], c);
        Z[r]->destroy(c);
    }
    printf("  capacity: 32,768 whole numbers stored and audited in L1-Q32 and L1-opt-Q32; the next aborts\n");
}

int main(int argc, char **argv) {
    int seeds = 200;
    for (int i = 1; i + 1 < argc; i++)
        if (!strcmp(argv[i], "--seeds")) seeds = atoi(argv[i + 1]);
    test_against_q64(seeds);
    test_capacity();
    printf("q32: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
