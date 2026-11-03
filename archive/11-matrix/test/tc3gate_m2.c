/* tc3gate_m2.c: the TC gate of round m2 (rule-m2 item 3, carried over from rule-m1: "the TC gate, with tc3.c run on
 * the job's operations, every step"): a job's sequence of operations run through tc3.c itself, results appended to
 * tc3.c's own list (g), and each operation's case and core counts written in the per-operation record's format (m1.h),
 * so that the runner can check them against tcm.c's, operation for operation.  From tc3gate.c (unchanged, m1's), for
 * the four job sets; tc3.c is compiled unchanged, as part of this translation unit, and is hash-checked.
 *
 *   tc3gate_m2 <workload> <readout> <percall>
 *
 * Every input entered in order (A, B, then C, D); if a step multiplies, 0/1 entered once (s's start, v0.1 §6.2); then
 * the steps in order: an addition step x + y for each cell; a multiplication step, for each cell in row order, s = 0/1
 * and, for k = 1..n, p = x[i][k] * y[k][j], s = s + p.  Each step's results are tc3.c handles, the next step's
 * operands.  The read-out is the result's cells, as the bench's.  Built in the counting build (SK_COUNT). */
#define _GNU_SOURCE
#include "m1.h" /* first: r3.h, round 3's cases */

#include "../lib/src/tc3.c"

#include "m1int.h"

#include <inttypes.h>

static void die(const char *m) {
    fprintf(stderr, "tc3gate_m2: %s\n", m);
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
    if (argc < 4) die("usage: tc3gate_m2 <workload> <readout> <percall>");
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
    sk_handle h0 = 0;
    if (muls) check(c, tc_enter(c, 0, 1, &h0), "enter");
    M1_SNAP(s0);
    for (int s = 0; s < NSTEP[set]; s++) {
        const int *st = STEPS[set][s];
        sk_handle *A = H[st[1]], *B = H[st[2]], *R = H[st[3]];
        if (!st[0]) {
            for (size_t i = 0; i < q; i++) {
                M1_OP_BEGIN(c, SK_PH_ADD, s0);
                check(c, tc_add(c, A[i], B[i], &R[i]), "op");
                M1_OP_END(c, SK_PH_ADD, s0);
            }
        } else {
            for (uint32_t i = 0; i < n; i++)
                for (uint32_t j = 0; j < n; j++) {
                    sk_handle sh = h0, p;
                    for (uint32_t k = 0; k < n; k++) {
                        M1_OP_BEGIN(c, SK_PH_MUL, s0);
                        check(c, tc_mul(c, A[i * n + k], B[k * n + j], &p), "op");
                        M1_OP_END(c, SK_PH_MUL, s0);
                        M1_OP_BEGIN(c, SK_PH_ADD, s0);
                        check(c, tc_add(c, sh, p, &sh), "op");
                        M1_OP_END(c, SK_PH_ADD, s0);
                    }
                    R[(size_t)i * n + j] = sh;
                }
        }
    }
    for (size_t i = 0; i < q; i++) {
        sk_int m = 0, d = 0;
        check(c, tc_read(c, H[6][i], &m, &d), "read");
        ON[i] = m;
        OD[i] = d;
    }
    if (m1_percall_n != nops) die("not one record per operation");
    f = fopen(argv[2], "wb");
    if (!f || fwrite(ON, 8, q, f) != q || fwrite(OD, 8, q, f) != q) die("cannot write the read-out");
    fclose(f);
    f = fopen(argv[3], "wb");
    if (!f || fwrite(m1_percall, sizeof(int16_t), M1_PERCALL * nops, f) != M1_PERCALL * nops)
        die("cannot write the per-operation record");
    fclose(f);
    static const char *const SETNAME[4] = {"add", "mul", "sop", "pos"};
    printf("ok tc3gate_m2 n=%u set=%s ops=%" PRIu64 " list=%u\n", n, SETNAME[set], nops, tc_size(c));
    tc_destroy(c);
    return 0;
}
