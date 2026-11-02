/* superopt.c: exhaustive search over straight-line programs for the complement layout.

   Layout on an N-bit word: bit N-1 = sign start S, bit N-2 = axis G, bits 0..N-3 = count.
   Value (count - 2^(N-2)*s) * i^axis.

   Operations (each one instruction):
     unary   not, neg, shl k, shr k, sar k, rol k            (k = 1..N-1; ror k = rol N-k)
     binary  add, sub, and, or, xor, andn (x AND NOT y)       on two computed values or inputs
     const   add c, sub c (x - c), rsub c (c - x), and c, or c, xor c, candn (c AND NOT x)
             with c in {1, ~1, G, ~G, S, ~S, K = S|G, C = G-1}  (closed under NOT)
   A program is a DAG of L instructions over the inputs; the last one is the output.

   Pruning (all safe, so "no program found" is a proof within the operation set):
     - a new value equal (on every test case) to an input, an earlier value or a set constant is dropped;
     - every computed value must be used later (live-value count bound);
     - adjacent independent instructions are kept in one canonical order;
     - commutative operands are ordered.
   Every test case is a lane, and all lanes are used (N = 4: 128 same-axis pairs; N = 6: 2048), so
   value equality is exact.

   Targets: add, sub (same-axis pairs, exact word), addz / subz (a zero result may carry either axis),
            neg, negz (one input, every word), full (complex add: same axis, or different axes with one
            operand zero; result must be the right value; any zero allowed).

   usage: superopt N target L [maxprint] [flagmode]
          searches lengths 1..L and stops after the first length with hits.
          Each hit is tagged: OF@vK = instruction K is an add/sub whose machine signed-overflow flag
          (x86 OF, ARM64 V) equals the true overflow of the (N-1)-bit (sign, count) value on every lane;
          signtest-ok = the branch-free test ((w1^r)&(w2^r))<0 (add) / ((w1^w2)&(w1^r))<0 (sub) is exact.
          flagmode=1 keeps only hits with some OF@vK.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAXC 4096
#define MAXV 12
typedef uint8_t u8;

static int N, NL, NIN;
static u8 MSK, G, S, K, C;
static u8 val[MAXV][MAXC];
static u8 tgt[MAXC], alt[MAXC], ovf[MAXC];
static int flagmode;   /* 1: some add/sub in the program must set the machine's signed-overflow flag exactly on (N-1)-bit overflow */
static u8 consts[8];
static const char *cname[8] = {"1", "~1", "G", "~G", "S", "~S", "K", "C"};

enum { U_NOT, U_NEG, U_SHL, U_SHR, U_SAR, U_ROL, B_ADD, B_SUB, B_AND, B_OR, B_XOR, B_ANDN,
       C_ADD, C_SUB, C_RSUB, C_AND, C_OR, C_XOR, C_CANDN, NOPS };
static const char *opname[NOPS] = {"not", "neg", "shl", "shr", "sar", "rol", "add", "sub", "and", "or", "xor",
                                   "andn", "add", "sub", "rsub", "and", "or", "xor", "candn"};

typedef struct { int op, a, b; } Ins;   /* b = k for shifts, const index for C_*, value index for B_* */

static inline u8 sx(u8 x) { return x; }
static inline u8 ap1(int op, int k, u8 x) {
    switch (op) {
    case U_NOT: return ~x & MSK;
    case U_NEG: return (u8)(-x) & MSK;
    case U_SHL: return (u8)(x << k) & MSK;
    case U_SHR: return x >> k;
    case U_SAR: { int v = (x & S) ? (int)x - (1 << N) : x; return (u8)(v >> k) & MSK; }
    case U_ROL: return (u8)((x << k) | (x >> (N - k))) & MSK;
    }
    return 0;
}
static inline u8 ap2(int op, u8 x, u8 y) {
    switch (op) {
    case B_ADD: case C_ADD: return (u8)(x + y) & MSK;
    case B_SUB: case C_SUB: return (u8)(x - y) & MSK;
    case C_RSUB: return (u8)(y - x) & MSK;
    case B_AND: case C_AND: return x & y;
    case B_OR: case C_OR: return x | y;
    case B_XOR: case C_XOR: return x ^ y;
    case B_ANDN: return x & ~y & MSK;
    case C_CANDN: return y & ~x & MSK;
    }
    return 0;
}
static inline u8 apply(const Ins *I, int lane, u8 (*v)[MAXC]) {
    int op = I->op;
    if (op <= U_ROL) return ap1(op, I->b, v[I->a][lane]);
    if (op <= B_ANDN) return ap2(op, v[I->a][lane], v[I->b][lane]);
    return ap2(op, v[I->a][lane], consts[I->b]);
}
static void compute(const Ins *I, u8 *out) {
    int op = I->op;
    if (op <= U_ROL) { const u8 *x = val[I->a]; int k = I->b;
        for (int i = 0; i < NL; i++) out[i] = ap1(op, k, x[i]); }
    else if (op <= B_ANDN) { const u8 *x = val[I->a], *y = val[I->b];
        for (int i = 0; i < NL; i++) out[i] = ap2(op, x[i], y[i]); }
    else { const u8 *x = val[I->a]; u8 c = consts[I->b];
        for (int i = 0; i < NL; i++) out[i] = ap2(op, x[i], c); }
}

static int L;
static int sub_target;
static Ins prog[8];
static int uses[MAXV];          /* use counts of each value index */
static long long nodes, hits;
static int maxprint = 60;

static void pv(int idx, char *buf) {
    if (idx < NIN) sprintf(buf, NIN == 1 ? "w" : "w%d", idx + 1);
    else sprintf(buf, "v%d", idx - NIN + 1);
}
static void kname(int k, char *buf) {
    /* shift amounts written both ways */
    sprintf(buf, "%d(=N-%d)", k, N - k);
}
static void print_prog(int len) {
    char a[16], b[16], kk[32];
    printf("  ");
    for (int i = 0; i < len; i++) {
        const Ins *I = &prog[i];
        pv(I->a, a);
        char dst[16]; pv(NIN + i, dst);
        if (i == len - 1) strcpy(dst, "r");
        if (I->op <= U_ROL) {
            if (I->op <= U_NEG) printf("%s=%s %s; ", dst, opname[I->op], a);
            else { kname(I->b, kk); printf("%s=%s %s %s; ", dst, a, opname[I->op], kk); }
        } else if (I->op <= B_ANDN) { pv(I->b, b); printf("%s=%s %s %s; ", dst, a, opname[I->op], b); }
        else printf("%s=%s %s %s; ", dst, a, opname[I->op], cname[I->b]);
    }
    printf("\n");
}

static inline int okfinal(int lane, u8 r) { return r == tgt[lane] || r == alt[lane]; }

static int is_dup(const u8 *v, int nv) {
    for (int j = 0; j < nv; j++) if (!memcmp(v, val[j], NL)) return 1;
    int c = 1; for (int i = 1; i < NL; i++) if (v[i] != v[0]) { c = 0; break; }
    if (c) for (int j = 0; j < 8; j++) if (v[0] == consts[j]) return 1;
    return 0;
}

static int code(const Ins *I) { return (I->op * 64 + I->a) * 64 + I->b; }
static int depends(const Ins *I, int idx) {
    if (I->a == idx) return 1;
    if (I->op >= B_ADD && I->op <= B_ANDN && I->b == idx) return 1;
    return 0;
}

/* try the final instruction I at position pos */
static void try_final(Ins I, int pos) {
    prog[pos] = I;
    for (int lane = 0; lane < NL; lane++)
        if (!okfinal(lane, apply(&I, lane, val))) return;
    /* which add/sub instructions leave the machine's OF equal to the true overflow on every lane? */
    char tag[64] = ""; int any = 0;
    for (int i = 0; i <= pos; i++) {
        int op = prog[i].op;
        if (!(op == B_ADD || op == B_SUB || op == C_ADD || op == C_SUB || op == C_RSUB)) continue;
        int ok = 1;
        for (int lane = 0; lane < NL && ok; lane++) {
            u8 x = (i == pos) ? 0 : 0, y, r;
            x = val[prog[i].a][lane];
            y = (op == B_ADD || op == B_SUB) ? val[prog[i].b][lane] : consts[prog[i].b];
            if (op == C_RSUB) { u8 t = x; x = y; y = t; }
            int isadd = (op == B_ADD || op == C_ADD);
            r = isadd ? (u8)(x + y) & MSK : (u8)(x - y) & MSK;
            int of = isadd ? (((x ^ r) & (y ^ r) & S) != 0) : (((x ^ y) & (x ^ r) & S) != 0);
            if (of != ovf[lane]) ok = 0;
        }
        if (ok) { any = 1; sprintf(tag + strlen(tag), " OF@v%d", i + 1); }
    }
    /* branch-free sign test on the result: ((w1^r)&(w2^r)) (add) or ((w1^w2)&(w1^r)) (sub) */
    int bt = (NIN == 2);
    for (int lane = 0; lane < NL && bt; lane++) {
        u8 r = apply(&prog[pos], lane, val), x = val[0][lane], y = val[1][lane];
        int of = sub_target ? (((x ^ y) & (x ^ r) & S) != 0) : (((x ^ r) & (y ^ r) & S) != 0);
        if (of != ovf[lane]) bt = 0;
    }
    if (bt) strcat(tag, " signtest-ok");
    if (flagmode && !any) return;
    hits++;
    if (hits <= maxprint) { printf("%s\n", tag[0] ? tag : " (no valid flag)"); print_prog(pos + 1); }
}

static int live_count(int nv) {   /* computed values (not inputs) with zero uses */
    int c = 0; for (int j = NIN; j < nv; j++) if (!uses[j]) c++; return c;
}

static void gen(int pos);

static void emit(Ins I, int pos) {
    /* canonical order of adjacent independent instructions */
    if (pos > 0 && !depends(&I, NIN + pos - 1) && code(&I) < code(&prog[pos - 1])) return;
    int nv = NIN + pos;
    if (pos == L - 1) {
        /* final must consume all live computed values */
        int lc = live_count(nv);
        int consumed = 0;
        for (int j = NIN; j < nv; j++) if (!uses[j] && depends(&I, j)) consumed++;
        if (consumed != lc) return;
        try_final(I, pos);
        return;
    }
    compute(&I, val[nv]);
    nodes++;
    if (is_dup(val[nv], nv)) return;
    prog[pos] = I;
    uses[I.a]++; if (I.op >= B_ADD && I.op <= B_ANDN) uses[I.b]++;
    uses[nv] = 0;
    /* live bound: live values after this <= remaining instructions (L-1-pos) ... each later ins consumes net <=1 */
    int live = live_count(nv + 1);
    int remaining = L - 1 - pos;
    if (live <= remaining) gen(pos + 1);
    uses[I.a]--; if (I.op >= B_ADD && I.op <= B_ANDN) uses[I.b]--;
}

static void gen(int pos) {
    int nv = NIN + pos;
    Ins I;
    for (int a = 0; a < nv; a++) {
        for (int op = U_NOT; op <= U_NEG; op++) { I.op = op; I.a = a; I.b = 0; emit(I, pos); }
        for (int op = U_SHL; op <= U_ROL; op++) for (int k = 1; k < N; k++) {
            I.op = op; I.a = a; I.b = k; emit(I, pos); }
        for (int op = C_ADD; op <= C_CANDN; op++) for (int c = 0; c < 8; c++) {
            I.op = op; I.a = a; I.b = c; emit(I, pos); }
        for (int b = 0; b < nv; b++) {
            if (b == a) continue;
            for (int op = B_ADD; op <= B_ANDN; op++) {
                int comm = (op == B_ADD || op == B_AND || op == B_OR || op == B_XOR);
                if (comm && b < a) continue;
                I.op = op; I.a = a; I.b = b; emit(I, pos);
            }
        }
    }
}

/* ---- targets ---- */
static int cnt(u8 w) { return w & C; }
static int sgn(u8 w) { return (w & S) ? 1 : 0; }
static int axis(u8 w) { return (w & G) ? 1 : 0; }
static int value(u8 w) { return cnt(w) - (sgn(w) << (N - 2)); }
static u8 enc(int v, int ax) {   /* wraps like an (N-1)-bit signed integer */
    int m = 1 << (N - 1);
    v = ((v % m) + m) % m;      /* 0..2^(N-1)-1, (N-1)-bit two's complement */
    u8 c = v & C, s = (v >> (N - 2)) & 1;
    return (s ? S : 0) | (ax ? G : 0) | c;
}

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: superopt N target L [maxprint]\n"); return 1; }
    N = atoi(argv[1]); const char *t = argv[2]; int Lmax = atoi(argv[3]);
    if (argc > 4) maxprint = atoi(argv[4]);
    if (argc > 5) flagmode = atoi(argv[5]);
    MSK = (1 << N) - 1; S = 1 << (N - 1); G = 1 << (N - 2); K = S | G; C = G - 1;
    u8 cs[8] = {1, (u8)(~1 & MSK), G, (u8)(~G & MSK), S, (u8)(~S & MSK), K, C};
    memcpy(consts, cs, 8);
    NL = 0;
    int full = !strcmp(t, "full");
    int relaxed = (t[strlen(t) - 1] == 'z') || full;
    if (!strncmp(t, "neg", 3)) {
        NIN = 1;
        for (int w = 0; w <= MSK; w++) {
            val[0][NL] = w; tgt[NL] = enc(-value(w), axis(w));
            alt[NL] = (relaxed && (tgt[NL] & ~G & MSK) == 0) ? (tgt[NL] ^ G) : tgt[NL];
            NL++;
        }
    } else {
        NIN = 2;
        int sub = !strncmp(t, "sub", 3); sub_target = sub;
        for (int w1 = 0; w1 <= MSK; w1++) for (int w2 = 0; w2 <= MSK; w2++) {
            int same = axis(w1) == axis(w2);
            int z1 = (w1 & ~G & MSK) == 0, z2 = (w2 & ~G & MSK) == 0;
            if (!same && !(full && (z1 || z2))) continue;
            int v, ax;
            if (same) { v = sub ? value(w1) - value(w2) : value(w1) + value(w2); ax = axis(w1); }
            else if (z1) { v = value(w2); ax = axis(w2); }
            else { v = value(w1); ax = axis(w1); }
            val[0][NL] = w1; val[1][NL] = w2; tgt[NL] = enc(v, ax);
            ovf[NL] = same && (v >= (1 << (N - 2)) || v < -(1 << (N - 2)));
            u8 r = tgt[NL];
            alt[NL] = (relaxed && (r & ~G & MSK) == 0) ? (r ^ G) : r;
            NL++;
        }
    }
    /* shuffle lanes so early exits happen fast (deterministic) */
    srand(12345);
    for (int i = NL - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        for (int q = 0; q < NIN; q++) { u8 x = val[q][i]; val[q][i] = val[q][j]; val[q][j] = x; }
        u8 x = tgt[i]; tgt[i] = tgt[j]; tgt[j] = x; x = alt[i]; alt[i] = alt[j]; alt[j] = x;
        x = ovf[i]; ovf[i] = ovf[j]; ovf[j] = x;
    }
    printf("N=%d target=%s lanes=%d\n", N, t, NL);
    for (L = 1; L <= Lmax; L++) {
        nodes = hits = 0;
        memset(uses, 0, sizeof uses);
        gen(0);
        printf("L=%d: %lld hits, %lld nodes\n", L, hits, nodes);
        fflush(stdout);
        if (hits) break;
    }
    return 0;
}
