/* bench.c: one invocation of one row on one job (research/rule.md item 4), driven by runner.py.  Adapted from the 08
 * driver (08-skeletons/test/bench.c) and, for the library rows, the M3 driver (07-prime-graph/test/m3/m3_bench.c).
 *
 *   bench calib <workload> <readout> [A|B]              one rep at r = 1, 2, 4, ... until a rep lasts >= 50 ms
 *   bench time  <workload> <readout> <r> <reps> [A|B]   timed reps of r runs each
 *   bench count <workload> <readout> <percall>          counting build: one run, counters, memory reports, and the
 *                                                       core counts of every call (spec §14.1 item 3)
 *   bench gate  <workload> <readout> <wholes>           test build: one run audited on the gate schedule (§14.3)
 *   bench mem   <workload> <readout>                    timing build: the memory run (rule item 6), heap growth by
 *                                                       mallinfo2; library rows in form A, with allocator hooks
 *   bench version
 *
 * A run: create a new, empty class (library rows: a new holder array of exactly N elements); every call of the
 * workload in order; read out every stored value in lowest terms; destroy the class (clear the holders).  Times
 * (CLOCK_MONOTONIC, read only between calls): entry = the calls that enter, operations = the other calls, read-out =
 * reading every stored value; total = their sum.  create/destroy, L1-opt's reserve (O-7), holder allocation, form B's
 * initialisation and the clears, and reading the workload are not timed.  A rep runs the job r times and prints the
 * sums over its r runs.
 *
 * stdout:   calib <r> <total> <entry> <ops> <read> [lib k=v]     rep <i> <total> <entry> <ops> <read> [lib k=v]
 *           overcap <total>                                       (a rep over 10 s; nothing more is timed)
 *           ctr/xctr/hist/mem/memN ...  wholes <when> <W>  memrun <when> k=v ...    ok ...
 * failures: error call=<i> op=<op> code=<c> step=<...>  exit 2    auditfail call=<i> kind=<..> msg=<..>  exit 3
 * read-out: int64 num[N], int64 den[N] in handle order, from the last run.
 */
#define _GNU_SOURCE
#include <inttypes.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

#include "api.h"

#define CALIB_MIN_NS 50000000ull
#define CAP_NS 10000000000ull
enum { ENTER = 0, ADD = 1, SUB = 2, MUL = 3, DIV = 4 };
__attribute__((unused)) static const char *const OPN[] = {"enter", "add", "sub", "mul", "div"};

static int64_t *OP, *A, *B;
static int64_t NC, NIN;
static int32_t JOB, WIDTH, FZK, NCH, KST;
static int64_t *ON, *OD; /* read-out */
static int64_t LAST_ENTER = -1;
static enum { M_TIME, M_COUNT, M_GATE, M_MEM } MODE;
static char FORM = 'A';
__attribute__((unused)) static const char *WHOLES_PATH;

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
    JOB = h[0]; WIDTH = h[1]; FZK = h[2]; NCH = h[3]; KST = h[4];
    if (WIDTH != 64) die("width 64 only");
    OP = xmalloc(8 * (size_t)NC); A = xmalloc(8 * (size_t)NC); B = xmalloc(8 * (size_t)NC);
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
    ON = xmalloc(8 * (size_t)NC); OD = xmalloc(8 * (size_t)NC);
}

static void write_out(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) die("cannot write read-out");
    if (fwrite(ON, 8, NC, f) != (size_t)NC || fwrite(OD, 8, NC, f) != (size_t)NC) die("short write");
    fclose(f);
}

/* the in-use heap of the C library (rule item 6): uordblks + hblkhd */
static uint64_t heap_now(void) {
    struct mallinfo2 mi = mallinfo2();
    return (uint64_t)mi.uordblks + (uint64_t)mi.hblkhd;
}
static uint64_t HEAP0;

#ifdef SCALAR
/* ============================================ scalar rows: L1, L1-opt, L1-Q32, L1-opt-Q32, ctl (and the lookup gate) */
static uint32_t *HD; /* handle returned by each call */
static int64_t *PERCALL;

static long n_full, n_changed;
#ifdef BENCH_AUDIT
/* the gate schedule (spec §14.3 item 2): a full audit after each of the first 200 calls, after every 64th, after the
 * last; audit_changed after every call */
static void gate_audit(api_class *c, int64_t call, int64_t ncalls) {
    const char *msg = "";
    int64_t k = call + 1;
    n_changed++;
    if (api_audit(c, 0, &msg)) {
        printf("auditfail call=%" PRId64 " kind=changed msg=%s\n", call, msg);
        exit(3);
    }
    if (k <= 200 || k % 64 == 0 || k == ncalls) {
        n_full++;
        if (api_audit(c, 1, &msg)) {
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

static void memrun_line(api_class *c, const char *when, int64_t stored) {
    sk_memory r;
    API(memory)(c, &r);
    printf("memrun %s heap=%" PRIu64 " stored=%" PRId64 " used=%" PRIu64 " allocated=%" PRIu64 "\n", when,
           heap_now() - HEAP0, stored, r.used, r.allocated);
}

static void run(uint64_t t[3]) {
    if (MODE == M_MEM) HEAP0 = heap_now();
    api_class *c = API(create)();
    api_reserve(c, (uint32_t)NC);
    uint64_t acc[2] = {0, 0};
    int cur = OP[0] == ENTER;
    sk_counters k0, k1;
#ifdef BENCH_COUNTING
    if (MODE == M_COUNT) api_counters(c, &k0, NULL);
#else
    (void)k0, (void)k1;
#endif
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
        if (MODE == M_COUNT) { /* the core counts of this call (untimed; spec §14.1 item 3) */
            api_counters(c, &k1, NULL);
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
#ifdef LAYER1
            if (MODE == M_GATE) printf("wholes after_entry %u\n", API(wholes)(c));
#endif
        }
#ifdef BENCH_AUDIT
        if (MODE == M_GATE) gate_audit(c, i, NC);
#endif
    }
    uint64_t t1 = now();
    acc[cur] += t1 - t0;
    if (MODE == M_MEM) memrun_line(c, "end", NC);
    for (int64_t i = 0; i < NC; i++) {
        sk_int m, d;
        if (API(read)(c, HD[i], &m, &d) != SK_OK) op_error(c, i, -1);
        ON[i] = m; OD[i] = d;
    }
    uint64_t t2 = now();
    t[0] += acc[1]; t[1] += acc[0]; t[2] += t2 - t1;
    if (MODE == M_COUNT || MODE == M_GATE) {
        api_dump_memory(c, "end", stdout);
        api_dump_counters(c, stdout);
    }
#ifdef LAYER1
    if (MODE == M_GATE) printf("wholes end %u\n", API(wholes)(c));
#endif
#if defined(LAYER1) && defined(BENCH_AUDIT)
    if (MODE == M_GATE && WHOLES_PATH) { /* the whole-number list in index order, for L1-opt against L1 (§17.3) */
        FILE *f = fopen(WHOLES_PATH, "wb");
        if (!f) die("cannot write the whole-number list");
        for (uint32_t k = 0; k < API(wholes)(c); k++) {
            sk_int v;
            API(t_whole)(c, k, &v);
            int64_t w = v;
            fwrite(&w, 8, 1, f);
        }
        fclose(f);
    }
#endif
    API(destroy)(c);
}

static void setup(void) {
    HD = xmalloc(4 * (size_t)NC);
    PERCALL = xmalloc(5 * 8 * (size_t)NC);
}
static void version(void) { printf(" row=%s", IMPL_NAME); }
#endif /* SCALAR */

#ifdef LIBROW
/* ======================================================================================= library rows: fmpq, mpq, fz */

/* ------------------------------------------------------------------ allocator hooks (memory run only, rule item 6) */
/* A table from block to requested size, held in memory obtained with mmap, outside the heap that is measured.
 * The hooks call the C library's allocator unchanged. */
typedef struct hent { void *p; size_t n; } hent;
static hent *HT;
static size_t HCAP, HUSED;
static uint64_t H_REQ, H_USABLE, H_BLOCKS;

static void *hmap(size_t bytes) {
    void *p = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { fprintf(stderr, "mmap failed\n"); abort(); }
    return p;
}
static size_t hslot(void *p, size_t cap) { return (size_t)(((uintptr_t)p >> 4) * 0x9E3779B97F4A7C15ull) & (cap - 1); }
static void hput(void *p, size_t n);
static void hgrow(void) {
    hent *old = HT;
    size_t oc = HCAP;
    HCAP = HCAP ? 2 * HCAP : 1u << 16;
    HT = hmap(HCAP * sizeof(hent)); /* zeroed */
    HUSED = 0;
    for (size_t i = 0; i < oc; i++)
        if (old[i].p && old[i].p != (void *)1) {
            size_t s = hslot(old[i].p, HCAP);
            while (HT[s].p) s = (s + 1) & (HCAP - 1);
            HT[s] = old[i];
            HUSED++;
        }
    if (old) munmap(old, oc * sizeof(hent));
}
static void hput(void *p, size_t n) {
    if (!p) return;
    if (2 * (HUSED + 1) > HCAP) hgrow();
    size_t s = hslot(p, HCAP);
    while (HT[s].p && HT[s].p != (void *)1) s = (s + 1) & (HCAP - 1);
    HT[s].p = p;
    HT[s].n = n;
    HUSED++;
    H_REQ += n;
    H_USABLE += malloc_usable_size(p);
    H_BLOCKS++;
}
static void hdel(void *p) {
    if (!p || !HCAP) return;
    size_t s = hslot(p, HCAP);
    while (HT[s].p) {
        if (HT[s].p == p) {
            H_REQ -= HT[s].n;
            H_USABLE -= malloc_usable_size(p);
            H_BLOCKS--;
            HT[s].p = (void *)1; /* tombstone: keeps the probe chain */
            return;
        }
        s = (s + 1) & (HCAP - 1);
    }
}
static void *h_alloc(size_t n) { void *p = malloc(n); hput(p, n); return p; }
static void *h_calloc(size_t k, size_t n) { void *p = calloc(k, n); hput(p, k * n); return p; }
static void *h_realloc(void *p, size_t n) { hdel(p); void *q = realloc(p, n); hput(q, n); return q; }
static void h_free(void *p) { hdel(p); free(p); }
static void *g_realloc(void *p, size_t o, size_t n) { (void)o; return h_realloc(p, n); }
static void g_free(void *p, size_t n) { (void)n; h_free(p); }
static void install_hooks(void) {
    hgrow();
    mp_set_memory_functions(h_alloc, g_realloc, g_free);
    __flint_set_memory_functions(h_alloc, h_calloc, h_realloc, h_free);
}

static void version(void) {
    printf(" row=%s gmp=%s flint=%s threads=%d form=%c", IMPL_NAME, gmp_version, FLINT_VERSION, flint_get_num_threads(),
           FORM);
}

static long toolong;
static void memrun_line(const char *when, int64_t stored, size_t elsize, int64_t extra) {
    printf("memrun %s heap=%" PRIu64 " stored=%" PRId64 " elsize=%zu N=%" PRId64 " extra=%" PRId64 " hook_req=%" PRIu64
           " hook_usable=%" PRIu64 " hook_blocks=%" PRIu64 "\n", when, heap_now() - HEAP0, stored, elsize, NC, extra,
           H_REQ, H_USABLE, H_BLOCKS);
}

#if defined(IMPL_FMPQ) || defined(IMPL_MPQ)
#ifdef IMPL_FMPQ
typedef fmpq elem;
#define E_INIT(x) fmpq_init(x)
#define E_CLEAR(x) fmpq_clear(x)
#define E_ENTER(x, m, d) fmpq_set_si((x), (slong)(m), (ulong)(d)) /* canonicalises */
#define E_ADD fmpq_add
#define E_SUB fmpq_sub
#define E_MUL fmpq_mul
#define E_DIV fmpq_div
#define E_FITS(x) (fmpz_fits_si(fmpq_numref(x)) && fmpz_fits_si(fmpq_denref(x)))
#define E_NUM(x) fmpz_get_si(fmpq_numref(x))
#define E_DEN(x) fmpz_get_si(fmpq_denref(x))
#else
typedef __mpq_struct elem;
#define E_INIT(x) mpq_init(x)
#define E_CLEAR(x) mpq_clear(x)
#define E_ENTER(x, m, d) (mpq_set_si((x), (long)(m), (unsigned long)(d)), mpq_canonicalize(x))
#define E_ADD mpq_add
#define E_SUB mpq_sub
#define E_MUL mpq_mul
#define E_DIV mpq_div
#define E_FITS(x) (mpz_fits_slong_p(mpq_numref(x)) && mpz_fits_slong_p(mpq_denref(x)))
#define E_NUM(x) mpz_get_si(mpq_numref(x))
#define E_DEN(x) mpz_get_si(mpq_denref(x))
#endif

/* form A: each element initialised inside the timed phase in which its value is stored; form B: every element
 * initialised before the timed phases of the run, untimed, and cleared after it (spec §10 item 1) */
static void run(uint64_t t[3]) {
    if (MODE == M_MEM) HEAP0 = heap_now();
    elem *H = xmalloc(sizeof(elem) * (size_t)NC); /* the holder array: exactly N elements, untimed */
    int formB = FORM == 'B';
    if (formB)
        for (int64_t i = 0; i < NC; i++) E_INIT(H + i);
    uint64_t acc[2] = {0, 0};
    int cur = OP[0] == ENTER;
    uint64_t t0 = now();
    for (int64_t i = 0; i < NC; i++) {
        int isE = OP[i] == ENTER;
        if (isE != cur) { uint64_t u = now(); acc[cur] += u - t0; t0 = u; cur = isE; }
        if (!formB) E_INIT(H + i);
        switch (OP[i]) {
        case ENTER: E_ENTER(H + i, A[i], B[i]); break;
        case ADD: E_ADD(H + i, H + A[i], H + B[i]); break;
        case SUB: E_SUB(H + i, H + A[i], H + B[i]); break;
        case MUL: E_MUL(H + i, H + A[i], H + B[i]); break;
        default: E_DIV(H + i, H + A[i], H + B[i]); break;
        }
        if (MODE == M_MEM && i == LAST_ENTER) {
            uint64_t a = now();
            memrun_line("after_entry", i + 1, sizeof(elem), 0);
            t0 += now() - a;
        }
    }
    uint64_t t1 = now();
    acc[cur] += t1 - t0;
    if (MODE == M_MEM) memrun_line("end", NC, sizeof(elem), 0);
    uint64_t t1b = now();
    for (int64_t i = 0; i < NC; i++) {
        if (E_FITS(H + i)) { ON[i] = E_NUM(H + i); OD[i] = E_DEN(H + i); }
        else { ON[i] = OD[i] = 0; toolong++; }
    }
    uint64_t t2 = now();
    t[0] += acc[1]; t[1] += acc[0]; t[2] += t2 - t1b;
    for (int64_t i = 0; i < NC; i++) E_CLEAR(H + i); /* untimed */
    free(H);
}
static void setup(void) {}
#else
/* ------------------------------------------------------------------ fz: fmpz numerators over one shared denominator */
/* Planned from the workload before timing (as 08 harness R-f): the distinct input denominators and, for each entry,
 * its denominator's position.  Timed entry: D = the lcm of the distinct input denominators (one fmpz_lcm each),
 * each distinct cofactor D/d once, each numerator scaled by its cofactor with fmpz_mul_si.  Operations: fmpz_init(r)
 * and fmpz_add or fmpz_sub.  Read-out: fmpq_set_fmpz_frac(q, n, D), then fmpz_get_si after a fits test. */
static uint64_t *DD;
static int64_t ND;
static uint32_t *DK;
static void setup(void) {
    DD = xmalloc(8 * (size_t)NIN + 8);
    DK = xmalloc(4 * (size_t)NC + 4);
    for (int64_t i = 0; i < NC; i++) {
        if (OP[i] == ENTER) {
            if (i >= NIN) die("fz: entries must come first");
            int64_t k = 0;
            while (k < ND && DD[k] != (uint64_t)B[i]) k++;
            if (k == ND) DD[ND++] = (uint64_t)B[i];
            DK[i] = (uint32_t)k;
        } else if (OP[i] != ADD && OP[i] != SUB) {
            die("fz runs only on jobs of additions and subtractions (spec §10 item 2)");
        }
    }
    if (ND != 1) die("fz runs only on jobs whose inputs share one denominator (rule item 1)");
}
static void run(uint64_t t[3]) {
    if (MODE == M_MEM) HEAP0 = heap_now();
    fmpz *X = xmalloc(sizeof(fmpz) * (size_t)NC); /* the holder array, untimed */
    fmpz *cof = xmalloc(sizeof(fmpz) * (size_t)ND);
    for (int64_t k = 0; k < ND; k++) fmpz_init(cof + k);
    fmpz_t D;
    fmpq_t q;
    fmpz_init(D);
    fmpq_init(q);
    uint64_t t0 = now();
    fmpz_one(D);
    for (int64_t k = 0; k < ND; k++) {
        fmpz_set_ui(cof + k, DD[k]);
        fmpz_lcm(D, D, cof + k);
    }
    for (int64_t k = 0; k < ND; k++) fmpz_divexact_ui(cof + k, D, DD[k]);
    for (int64_t i = 0; i < NIN; i++) {
        fmpz_init(X + i);
        fmpz_mul_si(X + i, cof + DK[i], (slong)A[i]);
    }
    uint64_t t1 = now();
    if (MODE == M_MEM) memrun_line("after_entry", NIN, sizeof(fmpz), (int64_t)sizeof(fmpz));
    t1 = now();
    for (int64_t i = NIN; i < NC; i++) {
        fmpz_init(X + i);
        if (OP[i] == ADD) fmpz_add(X + i, X + A[i], X + B[i]);
        else fmpz_sub(X + i, X + A[i], X + B[i]);
    }
    uint64_t t2 = now();
    if (MODE == M_MEM) memrun_line("end", NC, sizeof(fmpz), (int64_t)sizeof(fmpz));
    uint64_t t2b = now();
    for (int64_t i = 0; i < NC; i++) {
        fmpq_set_fmpz_frac(q, X + i, D);
        if (fmpz_fits_si(fmpq_numref(q)) && fmpz_fits_si(fmpq_denref(q))) {
            ON[i] = fmpz_get_si(fmpq_numref(q));
            OD[i] = fmpz_get_si(fmpq_denref(q));
        } else { ON[i] = OD[i] = 0; toolong++; }
    }
    uint64_t t3 = now();
    t[0] += t1 - t0; t[1] += t2 - t1; t[2] += t3 - t2b;
    for (int64_t i = 0; i < NC; i++) fmpz_clear(X + i); /* untimed */
    for (int64_t k = 0; k < ND; k++) fmpz_clear(cof + k);
    fmpz_clear(D);
    fmpq_clear(q);
    free(X);
    free(cof);
}
#endif
#endif /* LIBROW */

/* ======================================================================================= main */

static void rep(uint64_t r, uint64_t t[3]) {
    t[0] = t[1] = t[2] = 0;
    for (uint64_t i = 0; i < r; i++) run(t);
}

static void line(const char *kind, uint64_t a, uint64_t t[3]) {
    uint64_t tot = t[0] + t[1] + t[2];
    printf("%s %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64, kind, a, tot, t[0], t[1], t[2]);
    version();
    printf("\n");
}

int main(int argc, char **argv) {
    /* every row (as M3): freed memory is kept by malloc, so later runs reuse it */
    mallopt(M_MMAP_THRESHOLD, 1 << 30);
    mallopt(M_TRIM_THRESHOLD, 1 << 30);
    if (argc >= 2 && !strcmp(argv[1], "version")) {
#ifdef LIBROW
        flint_set_num_threads(1);
#endif
        printf("version");
        version();
        printf("\n");
        return 0;
    }
    if (argc < 4) die("usage: bench calib|time|count|gate|mem <workload> <readout> [...]");
    const char *mode = argv[1];
#ifdef LIBROW
    if (!strcmp(mode, "mem")) install_hooks(); /* before the first library call (rule item 6) */
    flint_set_num_threads(1);                  /* single-threaded, recorded in every output line */
    if (flint_get_num_threads() != 1) die("FLINT is not single-threaded");
#endif
    load(argv[2]);
    setup();
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
    uint64_t t[3];
    if ((!strcmp(mode, "calib") || !strcmp(mode, "time") || !strcmp(mode, "mem")) && !timing_build)
        die("calib, time and mem need the timing build");
    if (!strcmp(mode, "count") && (timing_build || test_build)) die("count needs the counting build");
    if (!strcmp(mode, "gate") && !test_build) die("gate needs the test build");
    if (!strcmp(mode, "calib")) {
        if (argc > 4) FORM = argv[4][0];
        MODE = M_TIME;
        for (uint64_t r = 1; r <= (1ull << 40); r *= 2) {
            rep(r, t);
            uint64_t tot = t[0] + t[1] + t[2];
            line("calib", r, t);
            if (tot > CAP_NS) { printf("overcap %" PRIu64 "\n", tot); break; }
            if (tot >= CALIB_MIN_NS) break;
        }
    } else if (!strcmp(mode, "time")) {
        if (argc < 6) die("time needs r and reps");
        if (argc > 6) FORM = argv[6][0];
        MODE = M_TIME;
        uint64_t r = strtoull(argv[4], 0, 10), reps = strtoull(argv[5], 0, 10);
        for (uint64_t i = 0; i < reps; i++) {
            rep(r, t);
            uint64_t tot = t[0] + t[1] + t[2];
            line("rep", i, t);
            if (tot > CAP_NS) { printf("overcap %" PRIu64 "\n", tot); break; }
        }
    } else if (!strcmp(mode, "mem")) {
        MODE = M_MEM;
        FORM = 'A';
        rep(1, t);
#ifdef LIBROW
        printf("memlib toolong=%ld", toolong);
        version();
        printf("\n");
#endif
    } else if (!strcmp(mode, "count")) {
#ifdef SCALAR
        if (argc < 5) die("count needs the per-call output path");
        MODE = M_COUNT;
        rep(1, t);
        FILE *f = fopen(argv[4], "wb");
        if (!f || fwrite(PERCALL, 8, 5 * (size_t)NC, f) != 5 * (size_t)NC) die("cannot write the per-call counts");
        fclose(f);
#else
        die("count: the library rows have no counters");
#endif
    } else if (!strcmp(mode, "gate")) {
#ifdef SCALAR
        MODE = M_GATE;
        if (argc > 4) WHOLES_PATH = argv[4];
        rep(1, t);
        printf("audits full=%ld changed=%ld\n", n_full, n_changed);
#else
        die("gate: the library rows have no gate");
#endif
    } else die("unknown mode");
#ifdef LIBROW
    if (toolong) { printf("error call=-1 op=read code=-1 step=a result did not fit 64 bits (%ld)\n", toolong); return 2; }
#endif
    write_out(argv[3]);
    printf("ok impl=%s job=%d calls=%" PRId64 "\n", IMPL_NAME, JOB, NC);
    return 0;
}
