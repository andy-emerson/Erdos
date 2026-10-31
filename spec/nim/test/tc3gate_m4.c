/* tc3gate_m4.c: the TC gate of round m4 (spec v0.4 §17.13; v0.1 §4; §17.10's TC-gate row), for the Nim build: a copy
 * of test/tc3gate_m2.c (unchanged, m2's) with only what §17.13 asks:
 *   - tc3.c (unchanged, compiled as part of this unit) with the new gcd: built with lib/vendor08/m4 first on the
 *     include path, as m4_libconf.py builds the m4 side;
 *   - §17.6's summation: each product cell starts from its first product, s = x[i][0] y[0][j], then s = s + p for
 *     k = 1..n-1 (n - 1 additions per cell; no 0/1 entered);
 *   - it runs up to the first overflow and stops there (§17.10: "tc3.c is never run past an overflow"): the records
 *     of the operations before it are written, the overflowing operation's is not, and the output line says where it
 *     stopped. A job that completes writes the read-out as before.
 *
 *   tc3gate_m4 <workload> <readout> <percall>
 *
 * stdout: "ok tc3gate_m4 n=.. set=.. ops=<records> list=.." and either "complete" or "stopped at=<record> step=<..>".
 * Built in the counting build (SK_COUNT). */
#define _GNU_SOURCE
#include "m1.h" /* first: r3.h, round 3's cases */

#include "../../lib/src/tc3.c" /* unchanged, pinned by m1_libconf (tc3_check) */

#include "m1int.h"

#include <inttypes.h>

static void die(const char *m) {
    fprintf(stderr, "tc3gate_m4: %s\n", m);
    exit(1);
}

static void check(tc_class *c, int rc, const char *where) {
    if (rc == SK_OK) return;
    sk_overflow o;
    tc_overflow(c, &o);
    printf("error where=%s code=%d step=%s\n", where, rc, o.step ? o.step : "?");
    exit(2);
}

/* the steps c = a op b (op 0 addition, 1 multiplication) over ids 0-3 (inputs), 4-5 (intermediates), 6 (result) */
static const int NSTEP[4] = {1, 1, 3, 3};
static const int STEPS[4][3][4] = {
    {{0, 0, 1, 6}}, {{1, 0, 1, 6}}, {{1, 0, 1, 4}, {1, 2, 3, 5}, {0, 4, 5, 6}}, {{0, 0, 1, 4}, {0, 2, 3, 5}, {1, 4, 5, 6}}};

int main(int argc, char **argv) {
    if (argc < 4) die("usage: tc3gate_m4 <workload> <readout> <percall>");
    FILE *f = fopen(argv[1], "rb");
    if (!f) die("cannot open workload");
    char magic[8];
    int32_t h[4];
    if (fread(magic, 1, 8, f) != 8 || fread(h, 4, 4, f) != 4) die("bad workload");
    int m1 = !memcmp(magic, "M1WLOAD1", 8), m2 = !memcmp(magic, "M2WLOAD1", 8);
    if ((!m1 && !m2) || h[0] < 0 || h[0] > 3 || (m1 && h[0] > 1) || (m2 && h[0] < 2)) die("bad workload");
    int set = h[0], nin = set < 2 ? 2 : 4;
    uint32_t n = (uint32_t)h[1];
    size_t q = (size_t)n * n;
    int64_t *IN[4];
    for (int x = 0; x < nin; x++) {
        IN[x] = malloc(16 * q);
        if (!IN[x] || fread(IN[x], 8, 2 * q, f) != 2 * q) die("short workload");
    }
    fclose(f);
    uint64_t nops = 0;
    int muls = 0;
    for (int s = 0; s < NSTEP[set]; s++) {
        nops += STEPS[set][s][0] ? 2 * (uint64_t)n * n * n : (uint64_t)q;
        muls += STEPS[set][s][0];
    }
    m1_percall = malloc(sizeof(int16_t) * M1_PERCALL * nops);
    m1_percall_cap = nops;
    m1_percall_n = 0;
    int64_t *ON = malloc(8 * q), *OD = malloc(8 * q);
    sk_handle *H[7];
    for (int x = 0; x < 7; x++) H[x] = malloc(sizeof(sk_handle) * q);
    if (!m1_percall || !ON || !OD) die("out of memory");
    tc_class *c = tc_create();
    for (int x = 0; x < nin; x++)
        for (size_t i = 0; i < q; i++) check(c, tc_enter(c, IN[x][2 * i], IN[x][2 * i + 1], &H[x][i]), "enter");
    (void)muls;
    M1_SNAP(s0);
    int stopped = 0;
    const char *where = "";
/* an operation that overflows stops the gate (§17.10); any other failure is an error */
#define GATE_OP(call)                                                                                             \
    do {                                                                                                          \
        int rc_ = (call);                                                                                         \
        if (rc_ == SK_EOVERFLOW) {                                                                                \
            sk_overflow o_;                                                                                       \
            tc_overflow(c, &o_);                                                                                  \
            where = o_.step ? o_.step : "?";                                                                      \
            stopped = 1;                                                                                          \
            goto done;                                                                                            \
        }                                                                                                         \
        check(c, rc_, "op");                                                                                      \
    } while (0)
    for (int s = 0; s < NSTEP[set]; s++) {
        const int *st = STEPS[set][s];
        sk_handle *A = H[st[1]], *B = H[st[2]], *R = H[st[3]];
        if (!st[0]) {
            for (size_t i = 0; i < q; i++) {
                M1_OP_BEGIN(c, SK_PH_ADD, s0);
                GATE_OP(tc_add(c, A[i], B[i], &R[i]));
                M1_OP_END(c, SK_PH_ADD, s0);
            }
        } else {
            for (uint32_t i = 0; i < n; i++)
                for (uint32_t j = 0; j < n; j++) {
                    sk_handle sh = 0, p;
                    for (uint32_t k = 0; k < n; k++) { /* s from the first product, then n - 1 additions (§17.6) */
                        M1_OP_BEGIN(c, SK_PH_MUL, s0);
                        GATE_OP(tc_mul(c, A[i * n + k], B[k * n + j], k ? &p : &sh));
                        M1_OP_END(c, SK_PH_MUL, s0);
                        if (k == 0) continue;
                        M1_OP_BEGIN(c, SK_PH_ADD, s0);
                        GATE_OP(tc_add(c, sh, p, &sh));
                        M1_OP_END(c, SK_PH_ADD, s0);
                    }
                    R[(size_t)i * n + j] = sh;
                }
        }
    }
done:
    if (!stopped) {
        for (size_t i = 0; i < q; i++) {
            sk_int m = 0, d = 0;
            check(c, tc_read(c, H[6][i], &m, &d), "read");
            ON[i] = m;
            OD[i] = d;
        }
        f = fopen(argv[2], "wb");
        if (!f || fwrite(ON, 8, q, f) != q || fwrite(OD, 8, q, f) != q) die("cannot write the read-out");
        fclose(f);
    }
    nops = m1_percall_n; /* the operations made, up to the first overflow */
    f = fopen(argv[3], "wb");
    if (!f || fwrite(m1_percall, sizeof(int16_t), M1_PERCALL * nops, f) != M1_PERCALL * nops)
        die("cannot write the per-operation record");
    fclose(f);
    static const char *const SETNAME[4] = {"add", "mul", "sop", "pos"};
    printf("ok tc3gate_m4 n=%u set=%s ops=%" PRIu64 " list=%u\n", n, SETNAME[set], nops, tc_size(c));
    if (stopped) printf("stopped at=%" PRIu64 " step=%s\n", nops, where);
    else printf("complete\n");
    tc_destroy(c);
    return 0;
}
