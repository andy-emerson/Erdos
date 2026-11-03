/* superopt2.c: round 2 searches for the complement layout (multiply/divide corrections, full add).

   Same search as superopt.c (exhaustive over straight-line DAG programs, same safe pruning: no value equal to
   an input / earlier value / set constant; every value used; canonical order of adjacent independent
   instructions; ordered commutative operands), generalised:
     - up to 3 inputs, given per lane (e.g. the machine's product p beside w1, w2);
     - a per-lane care mask (bits the result must match), and an alternative target (either zero);
     - optional operations, each one instruction:
         M  mul (low half, commutative)
         D  div (signed, truncating; RISC-V rules for x/0 = -1 and MIN/-1 = MIN)
         Z  seqz, snez (unary: 0/1)
         Q  czero.eqz x,y = (y == 0 ? 0 : x), czero.nez x,y = (y != 0 ? 0 : x)   (RISC-V Zicond)
         U  maxu, minu (RISC-V Zbb), sltu (0/1)
     Base operations as superopt.c: not, neg, shl/shr/sar/rol k, add, sub, and, or, xor, andn,
     and add/sub/rsub/and/or/xor/candn with c in {1, ~1, G, ~G, S, ~S, K, C}.

   usage: superopt2 N target Lmax [maxprint] [ext]      ext = letters from "MDZQU"
   resumable: SO_LMIN=L SO_START=k starts length L at first-instruction chunk k and logs each chunk done
   targets:
     sext   w -> v as an N-bit signed integer                 (every word)
     mod    the same, top bit free (v mod 2^(N-1))
     twov   2v mod 2^N          ntwov  -2v mod 2^N
     mmul   mask of a1 AND a2   mdiv   mask of a2 AND NOT a1  (every pair)
     fix    (p, w1, w2) -> enc(v1 v2, a1^a2), p = the N-bit product of the sign-extended values
     postmul  as fix, result (-1)^(a1 a2) v1 v2              (lanes: the product fits)
     postdiv  (q, w1, w2) -> enc((-1)^(a2 !a1) q, a1^a2), q = trunc(v1/v2), v2 != 0, fits
     mul    (w1, w2) -> enc((-1)^(a1 a2) v1 v2, a1^a2)       (needs M)
     div    (w1, w2) -> enc((-1)^(a2 !a1) trunc(v1/v2), a1^a2), v2 != 0 (needs D)
     full   complex add: same axis -> sum; different axes with one operand zero -> the other; any zero ok
     fullsub  the same for w1 - w2 (w1 zero, cross -> -w2 on w2's axis)
     faxis  (w1, w2) -> any word with the full add's result axis in bit N-2 (other bits free)
     cnegm, cnegd  (x, w1, w2) -> (-1)^k x, x = w1's sign-extended value, k = a1 a2 / a2 AND NOT a1
     eflag  (w1, w2) -> 1 (or all ones) when both are nonzero on different axes, else 0
     fullb  (w1, w2, b) -> full, with b = (w1 AND ~G) + (w2 OR G) given (the bridge on every input)
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAXC 4096
#define MAXV 16
typedef uint8_t u8;

static int N, NL, NIN;
static u8 MSK, G, S, K, C;
static u8 val[MAXV][MAXC];
static u8 tgt[MAXC], alt[MAXC], care[MAXC];
static u8 consts[8];
static const char *cname[8] = {"1", "~1", "G", "~G", "S", "~S", "K", "C"};
static int useM, useD, useZ, useQ, useU;

enum { U_NOT, U_NEG, U_SEQZ, U_SNEZ, U_SHL, U_SHR, U_SAR, U_ROL,
       B_ADD, B_SUB, B_AND, B_OR, B_XOR, B_ANDN, B_MUL, B_DIV, B_CZE, B_CZN, B_MAXU, B_MINU, B_SLTU,
       C_ADD, C_SUB, C_RSUB, C_AND, C_OR, C_XOR, C_CANDN, NOPS };
#define ISUN(op) ((op) <= U_ROL)
#define ISBIN(op) ((op) >= B_ADD && (op) <= B_SLTU)
static const char *opname[NOPS] = {"not", "neg", "seqz", "snez", "shl", "shr", "sar", "rol",
    "add", "sub", "and", "or", "xor", "andn", "mul", "div", "czero.eqz", "czero.nez", "maxu", "minu", "sltu",
    "add", "sub", "rsub", "and", "or", "xor", "candn"};

typedef struct { int op, a, b; } Ins;

static inline int sx(u8 x) { return (x & S) ? (int)x - (1 << N) : x; }
static inline u8 ap1(int op, int k, u8 x) {
    switch (op) {
    case U_NOT: return ~x & MSK;
    case U_NEG: return (u8)(-x) & MSK;
    case U_SEQZ: return x == 0;
    case U_SNEZ: return x != 0;
    case U_SHL: return (u8)(x << k) & MSK;
    case U_SHR: return x >> k;
    case U_SAR: return (u8)(sx(x) >> k) & MSK;
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
    case B_MUL: return (u8)(x * y) & MSK;
    case B_DIV: { int a = sx(x), b = sx(y);
        if (b == 0) return MSK;
        if (a == -(1 << (N - 1)) && b == -1) return x;
        int q = abs(a) / abs(b); if ((a < 0) != (b < 0)) q = -q; return (u8)q & MSK; }
    case B_CZE: return y == 0 ? 0 : x;
    case B_CZN: return y != 0 ? 0 : x;
    case B_MAXU: return x > y ? x : y;
    case B_MINU: return x < y ? x : y;
    case B_SLTU: return x < y;
    }
    return 0;
}
static void compute(const Ins *I, u8 *out) {
    int op = I->op;
    if (ISUN(op)) { const u8 *x = val[I->a]; int k = I->b; for (int i = 0; i < NL; i++) out[i] = ap1(op, k, x[i]); }
    else if (ISBIN(op)) { const u8 *x = val[I->a], *y = val[I->b]; for (int i = 0; i < NL; i++) out[i] = ap2(op, x[i], y[i]); }
    else { const u8 *x = val[I->a]; u8 c = consts[I->b]; for (int i = 0; i < NL; i++) out[i] = ap2(op, x[i], c); }
}
static inline u8 apply(const Ins *I, int lane) {
    int op = I->op;
    if (ISUN(op)) return ap1(op, I->b, val[I->a][lane]);
    if (ISBIN(op)) return ap2(op, val[I->a][lane], val[I->b][lane]);
    return ap2(op, val[I->a][lane], consts[I->b]);
}

static int L;
static Ins prog[12];
static int uses[MAXV];
static long long nodes, hits;
static int maxprint = 40;
static const char *inname[3];

static void pv(int idx, char *buf) {
    if (idx < NIN) strcpy(buf, inname[idx]); else sprintf(buf, "v%d", idx - NIN + 1);
}
static void print_prog(int len) {
    char a[16], b[16], dst[16];
    printf("  ");
    for (int i = 0; i < len; i++) {
        const Ins *I = &prog[i];
        pv(I->a, a); pv(NIN + i, dst); if (i == len - 1) strcpy(dst, "r");
        if (ISUN(I->op)) {
            if (I->op <= U_SNEZ) printf("%s=%s %s; ", dst, opname[I->op], a);
            else printf("%s=%s %s %d; ", dst, a, opname[I->op], I->b);
        } else if (ISBIN(I->op)) { pv(I->b, b); printf("%s=%s %s %s; ", dst, a, opname[I->op], b); }
        else printf("%s=%s %s %s; ", dst, a, opname[I->op], cname[I->b]);
    }
    printf("\n");
}
static inline int okfinal(int lane, u8 r) {
    return !((r ^ tgt[lane]) & care[lane]) || !((r ^ alt[lane]) & care[lane]);
}
static int is_dup(const u8 *v, int nv) {
    for (int j = 0; j < nv; j++) if (!memcmp(v, val[j], NL)) return 1;
    int c = 1; for (int i = 1; i < NL; i++) if (v[i] != v[0]) { c = 0; break; }
    if (c) for (int j = 0; j < 8; j++) if (v[0] == consts[j]) return 1;
    return 0;
}
static int code(const Ins *I) { return (I->op * 64 + I->a) * 64 + I->b; }
static int depends(const Ins *I, int idx) { return I->a == idx || (ISBIN(I->op) && I->b == idx); }

static void try_final(Ins I, int pos) {
    prog[pos] = I;
    for (int lane = 0; lane < NL; lane++) if (!okfinal(lane, apply(&I, lane))) return;
    hits++;
    if (hits <= maxprint) print_prog(pos + 1);
}
static int live_count(int nv) { int c = 0; for (int j = NIN; j < nv; j++) if (!uses[j]) c++; return c; }
static void gen(int pos);
static void emit(Ins I, int pos) {
    if (pos > 0 && !depends(&I, NIN + pos - 1) && code(&I) < code(&prog[pos - 1])) return;
    int nv = NIN + pos;
    if (pos == L - 1) {
        int lc = live_count(nv), consumed = 0;
        for (int j = NIN; j < nv; j++) if (!uses[j] && depends(&I, j)) consumed++;
        if (consumed != lc) return;
        try_final(I, pos);
        return;
    }
    compute(&I, val[nv]); nodes++;
    if (is_dup(val[nv], nv)) return;
    prog[pos] = I;
    uses[I.a]++; if (ISBIN(I.op)) uses[I.b]++;
    uses[nv] = 0;
    if (live_count(nv + 1) <= L - 1 - pos) gen(pos + 1);
    uses[I.a]--; if (ISBIN(I.op)) uses[I.b]--;
}
static long firstidx, startidx;        /* resumable runs: chunks = first instructions (env SO_START) */
static void emit0(Ins I, int pos) {
    if (pos != 0) { emit(I, pos); return; }
    long i = firstidx++;
    if (i < startidx) return;
    emit(I, pos);
    if (startidx >= 0) printf("  [chunk %ld done: %lld hits, %lld nodes so far]\n", i, hits, nodes); fflush(stdout);
}
#define emit emit0
static void gen(int pos) {
    int nv = NIN + pos; Ins I;
    for (int a = 0; a < nv; a++) {
        for (int op = U_NOT; op <= U_SNEZ; op++) {
            if ((op == U_SEQZ || op == U_SNEZ) && !useZ) continue;
            I.op = op; I.a = a; I.b = 0; emit(I, pos); }
        for (int op = U_SHL; op <= U_ROL; op++) for (int k = 1; k < N; k++) { I.op = op; I.a = a; I.b = k; emit(I, pos); }
        for (int op = C_ADD; op <= C_CANDN; op++) for (int c = 0; c < 8; c++) { I.op = op; I.a = a; I.b = c; emit(I, pos); }
        for (int b = 0; b < nv; b++) {
            if (b == a) continue;
            for (int op = B_ADD; op <= B_SLTU; op++) {
                if (op == B_MUL && !useM) continue;
                if (op == B_DIV && !useD) continue;
                if ((op == B_CZE || op == B_CZN) && !useQ) continue;
                if ((op == B_MAXU || op == B_MINU || op == B_SLTU) && !useU) continue;
                int comm = (op == B_ADD || op == B_AND || op == B_OR || op == B_XOR || op == B_MUL || op == B_MAXU || op == B_MINU);
                if (comm && b < a) continue;
                I.op = op; I.a = a; I.b = b; emit(I, pos);
            }
        }
    }
}

/* ---- layout ---- */
static int value(u8 w) { return (w & C) - (((w & S) ? 1 : 0) << (N - 2)); }
static int axis(u8 w) { return (w & G) ? 1 : 0; }
static int fits(int v) { return v >= -(1 << (N - 2)) && v < (1 << (N - 2)); }
static u8 enc(int v, int ax) {
    int m = 1 << (N - 1); v = ((v % m) + m) % m;
    return ((v >> (N - 2)) & 1 ? S : 0) | (ax ? G : 0) | (v & C);
}
static int iszero(u8 w) { return (w & ~G & MSK) == 0; }
static void lane(u8 a, u8 b, u8 c, u8 t, u8 cm, int relaxed) {
    val[0][NL] = a; val[1][NL] = b; val[2][NL] = c; tgt[NL] = t; care[NL] = cm;
    alt[NL] = (relaxed && iszero(t)) ? t ^ G : t; NL++;
}
static int tdiv(int a, int b) { int q = abs(a) / abs(b); return ((a < 0) != (b < 0)) ? -q : q; }

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: superopt2 N target Lmax [maxprint] [ext]\n"); return 1; }
    N = atoi(argv[1]); const char *t = argv[2]; int Lmax = atoi(argv[3]);
    if (argc > 4) maxprint = atoi(argv[4]);
    if (argc > 5) { useM = !!strchr(argv[5], 'M'); useD = !!strchr(argv[5], 'D');
                    useZ = !!strchr(argv[5], 'Z'); useQ = !!strchr(argv[5], 'Q'); useU = !!strchr(argv[5], 'U'); }
    MSK = (1 << N) - 1; S = 1 << (N - 1); G = 1 << (N - 2); K = S | G; C = G - 1;
    u8 cs[8] = {1, (u8)(~1 & MSK), G, (u8)(~G & MSK), S, (u8)(~S & MSK), K, C};
    memcpy(consts, cs, 8);
    NL = 0; inname[0] = "w1"; inname[1] = "w2"; inname[2] = "x";
    int W = 1 << N;
    if (!strcmp(t, "sext") || !strcmp(t, "mod") || !strcmp(t, "twov") || !strcmp(t, "ntwov")) {
        NIN = 1; inname[0] = "w";
        for (int w = 0; w < W; w++) {
            int v = value(w), m = !strcmp(t, "twov") ? 2 * v : !strcmp(t, "ntwov") ? -2 * v : v;
            lane(w, 0, 0, (u8)m & MSK, !strcmp(t, "mod") ? (MSK & ~S) : MSK, 0);
        }
    } else if (!strcmp(t, "mmul") || !strcmp(t, "mdiv")) {
        NIN = 2;
        for (int a = 0; a < W; a++) for (int b = 0; b < W; b++) {
            int k = !strcmp(t, "mmul") ? (axis(a) && axis(b)) : (axis(b) && !axis(a));
            lane(a, b, 0, k ? MSK : 0, MSK, 0);
        }
    } else if (!strcmp(t, "fix") || !strcmp(t, "postmul") || !strcmp(t, "postdiv")) {
        NIN = 3; inname[0] = "p"; inname[1] = "w1"; inname[2] = "w2";
        int isdiv = !strcmp(t, "postdiv"), neg = strcmp(t, "fix") != 0;
        for (int a = 0; a < W; a++) for (int b = 0; b < W; b++) {
            int v1 = value(a), v2 = value(b), a1 = axis(a), a2 = axis(b), r, k;
            if (isdiv) { if (!v2) continue; r = tdiv(v1, v2); k = a2 && !a1; }
            else { r = v1 * v2; k = a1 && a2; }
            int res = (neg && k) ? -r : r;
            if (!fits(res)) continue;
            lane((u8)r & MSK, a, b, enc(res, a1 ^ a2), MSK, 0);
        }
    } else if (!strcmp(t, "mul") || !strcmp(t, "div")) {
        NIN = 2; int isdiv = !strcmp(t, "div");
        for (int a = 0; a < W; a++) for (int b = 0; b < W; b++) {
            int v1 = value(a), v2 = value(b), a1 = axis(a), a2 = axis(b), r;
            if (isdiv) { if (!v2) continue; r = tdiv(v1, v2); if (a2 && !a1) r = -r; }
            else { r = v1 * v2; if (a1 && a2) r = -r; }
            if (!fits(r)) continue;
            lane(a, b, 0, enc(r, a1 ^ a2), MSK, 0);
        }
    } else if (!strcmp(t, "full") || !strcmp(t, "fullsub") || !strcmp(t, "fullb")) {
        NIN = !strcmp(t, "fullb") ? 3 : 2; if (NIN == 3) inname[2] = "b";
        int sub = !strcmp(t, "fullsub");
        for (int a = 0; a < W; a++) for (int b = 0; b < W; b++) {
            int same = axis(a) == axis(b), z1 = iszero(a), z2 = iszero(b), v, ax;
            if (!same && !z1 && !z2) continue;
            if (same) { v = sub ? value(a) - value(b) : value(a) + value(b); ax = axis(a); }
            else if (z1) { v = sub ? -value(b) : value(b); ax = axis(b); }
            else { v = value(a); ax = axis(a); }
            u8 bb = (u8)((a & ~G & MSK) + (b | G)) & MSK;
            lane(a, b, bb, enc(v, ax), MSK, 1);
        }
    } else if (!strcmp(t, "cnegm") || !strcmp(t, "cnegd")) {
        /* (x, w1, w2) -> (-1)^k x, x = w1's value sign-extended; k = a1 a2 (cnegm) or a2 AND NOT a1 (cnegd) */
        NIN = 3; inname[0] = "x"; inname[1] = "w1"; inname[2] = "w2";
        for (int a = 0; a < W; a++) for (int b = 0; b < W; b++) {
            int k = !strcmp(t, "cnegm") ? (axis(a) && axis(b)) : (axis(b) && !axis(a)), x = value(a);
            lane((u8)x & MSK, a, b, (u8)(k ? -x : x) & MSK, MSK, 0);
        }
    } else if (!strcmp(t, "eflag")) {
        /* the expression flag: 1 exactly when both are nonzero on different axes (0/1 in bit 0, or any mask) */
        NIN = 2;
        for (int a = 0; a < W; a++) for (int b = 0; b < W; b++) {
            int e = axis(a) != axis(b) && !iszero(a) && !iszero(b);
            lane(a, b, 0, e, MSK, 0); alt[NL - 1] = e ? MSK : 0;
        }
    } else if (!strcmp(t, "faxis")) {
        /* any word whose axis bit is the full add's (and full subtract's) result axis; zero results: free */
        NIN = 2;
        for (int a = 0; a < W; a++) for (int b = 0; b < W; b++) {
            int same = axis(a) == axis(b), z1 = iszero(a), z2 = iszero(b);
            if (!same && !z1 && !z2) continue;
            int ax = (!same && z1) ? axis(b) : axis(a);
            int v = same ? value(a) + value(b) : z1 ? value(b) : value(a);
            int m = 1 << (N - 1), zres = ((v % m) + m) % m == 0;   /* a zero result may have either axis */
            lane(a, b, 0, ax ? G : 0, zres ? 0 : G, 0);
        }
    } else { fprintf(stderr, "unknown target\n"); return 1; }
    srand(12345);
    for (int i = NL - 1; i > 0; i--) {
        int j = rand() % (i + 1); u8 x;
        for (int q = 0; q < 3; q++) { x = val[q][i]; val[q][i] = val[q][j]; val[q][j] = x; }
        x = tgt[i]; tgt[i] = tgt[j]; tgt[j] = x; x = alt[i]; alt[i] = alt[j]; alt[j] = x;
        x = care[i]; care[i] = care[j]; care[j] = x;
    }
    printf("N=%d target=%s lanes=%d ext=%s\n", N, t, NL, argc > 5 ? argv[5] : "-");
    int Lmin = getenv("SO_LMIN") ? atoi(getenv("SO_LMIN")) : 1;
    long st = getenv("SO_START") ? atol(getenv("SO_START")) : 0;
    int chunks = getenv("SO_START") != NULL;
    for (L = Lmin; L <= Lmax; L++) {
        nodes = hits = 0; memset(uses, 0, sizeof uses);
        firstidx = 0; startidx = (L == Lmax) ? st : 0;
        if (!chunks || L < Lmax) startidx = -1;   /* no chunk log unless asked */
        gen(0);
        printf("L=%d: %lld hits, %lld nodes\n", L, hits, nodes); fflush(stdout);
        if (hits) break;
    }
    return 0;
}
