/* tc3gate.c: the TC gate (spec 11-matrix v0.1 §4; rule-m1 item 3): a job's sequence of operations run through tc3.c
 * itself, results appended to tc3.c's own list (g), and each operation's case and core counts (gcds, gcd work, exact
 * divisions, multiplications, additions) written in the per-operation record's format (m1.h), so that the runner can
 * check them against tcm.c's, operation for operation.  tc3.c is compiled unchanged, as part of this translation unit
 * (so that its counters can be read between operations without a copy of every counter), and is hash-checked.
 *
 *   tc3gate <workload> <readout> <percall>
 *
 * An addition job: every cell of A entered, then every cell of B (the order of entry, spec §5.7); then for each cell
 * x + y.  A multiplication job: the same entries, and 0/1 entered once (s's start, §6.2); then for each cell of C in
 * row order, s = 0/1 and, for k = 1..n, p = A[i][k] * B[k][j], s = s + p.  The read-out is C's cells, as the bench's.
 * Built in the counting build (SK_COUNT). */
#define _GNU_SOURCE
#include "m1.h" /* first: r3.h, round 3's cases */

#include "../lib/src/tc3.c"

#include "m1int.h"

#include <inttypes.h>

static void die(const char *m) {
    fprintf(stderr, "tc3gate: %s\n", m);
    exit(1);
}

static void check(tc_class *c, int rc, const char *where) {
    if (rc == SK_OK) return;
    sk_overflow o;
    tc_overflow(c, &o);
    printf("error where=%s code=%d step=%s\n", where, rc, o.step ? o.step : "?");
    exit(2);
}

int main(int argc, char **argv) {
    if (argc < 4) die("usage: tc3gate <workload> <readout> <percall>");
    FILE *f = fopen(argv[1], "rb");
    if (!f) die("cannot open workload");
    char magic[8];
    int32_t h[4];
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, "M1WLOAD1", 8) || fread(h, 4, 4, f) != 4) die("bad workload");
    int opk = h[0];
    uint32_t n = (uint32_t)h[1];
    size_t q = (size_t)n * n;
    int64_t *IA = malloc(16 * q), *IB = malloc(16 * q);
    if (!IA || !IB || fread(IA, 8, 2 * q, f) != 2 * q || fread(IB, 8, 2 * q, f) != 2 * q) die("short workload");
    fclose(f);
    uint64_t nops = opk ? 2 * (uint64_t)n * n * n : (uint64_t)q;
    m1_percall = malloc(sizeof(int16_t) * M1_PERCALL * nops);
    m1_percall_cap = nops;
    m1_percall_n = 0;
    int64_t *ON = malloc(8 * q), *OD = malloc(8 * q);
    sk_handle *hA = malloc(sizeof(sk_handle) * q), *hB = malloc(sizeof(sk_handle) * q);
    if (!m1_percall || !ON || !OD || !hA || !hB) die("out of memory");
    tc_class *c = tc_create();
    for (size_t i = 0; i < q; i++) check(c, tc_enter(c, IA[2 * i], IA[2 * i + 1], &hA[i]), "enter");
    for (size_t i = 0; i < q; i++) check(c, tc_enter(c, IB[2 * i], IB[2 * i + 1], &hB[i]), "enter");
    sk_handle h0 = 0, r;
    if (opk) check(c, tc_enter(c, 0, 1, &h0), "enter");
    M1_SNAP(s0);
    if (!opk) {
        for (size_t i = 0; i < q; i++) {
            M1_OP_BEGIN(c, SK_PH_ADD, s0);
            check(c, tc_add(c, hA[i], hB[i], &r), "op");
            M1_OP_END(c, SK_PH_ADD, s0);
            sk_int m = 0, d = 0;
            check(c, tc_read(c, r, &m, &d), "read");
            ON[i] = m;
            OD[i] = d;
        }
    } else {
        for (uint32_t i = 0; i < n; i++)
            for (uint32_t j = 0; j < n; j++) {
                sk_handle s = h0, p;
                for (uint32_t k = 0; k < n; k++) {
                    M1_OP_BEGIN(c, SK_PH_MUL, s0);
                    check(c, tc_mul(c, hA[i * n + k], hB[k * n + j], &p), "op");
                    M1_OP_END(c, SK_PH_MUL, s0);
                    M1_OP_BEGIN(c, SK_PH_ADD, s0);
                    check(c, tc_add(c, s, p, &s), "op");
                    M1_OP_END(c, SK_PH_ADD, s0);
                }
                sk_int m = 0, d = 0;
                check(c, tc_read(c, s, &m, &d), "read");
                ON[(size_t)i * n + j] = m;
                OD[(size_t)i * n + j] = d;
            }
    }
    if (m1_percall_n != nops) die("not one record per operation");
    f = fopen(argv[2], "wb");
    if (!f || fwrite(ON, 8, q, f) != q || fwrite(OD, 8, q, f) != q) die("cannot write the read-out");
    fclose(f);
    f = fopen(argv[3], "wb");
    if (!f || fwrite(m1_percall, sizeof(int16_t), M1_PERCALL * nops, f) != M1_PERCALL * nops)
        die("cannot write the per-operation record");
    fclose(f);
    printf("ok tc3gate n=%u op=%s ops=%" PRIu64 " list=%u\n", n, opk ? "mul" : "add", nops, tc_size(c));
    tc_destroy(c);
    return 0;
}
