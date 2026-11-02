/* rsuper.c: exhaustive search over straight-line programs for complex rationals on an axis.

   Value (num/den) * i^a, two N-bit words, T = 2^(N-1):
     num: signed integer (two's complement)
     den: layout "off": D + T*a      layout "neg": (-1)^a * D       (1 <= D < T; top bit = a in both)

   Lanes: every (n1, D1, a1, n2, D2, a2) with D1*D2 < T and the true numerator in N signed bits
   (add/sub: a1 = a2).  Expected words: num = true numerator mod 2^N over D1*D2 (no reduction),
   den = the layout's word for (D1*D2, axis); a zero result may carry either axis.

   Inputs:
     mode 0 (pre and post corrections):  n1 d1 n2 d2                       (multiplies are searched, 1 each)
     mode 1 (post corrections only):     n1 d1 n2 d2 P Q   with the raw products given free:
                                         add P = n1*d2 + n2*d1, Q = d1*d2;  mul P = n1*n2, Q = d1*d2;
                                         sub P = n1*d2 - n2*d1.
   op "am" (mode 1 only): the lanes of add AND of mul together, each with its own P, Q: a hit is ONE
   correction program that fixes both.

   Operations (one instruction each):
     unary   not, neg, shl 1, shl N-1, shr N-1, sar N-1
     binary  add, sub, mul, and, or, xor, andn (x AND NOT y)     (mul only if allowed)
     const   and c, or c, xor c, add c, rsub c (c - x)            c in {1, T, ~T, -1}
   Targets: "num", "den", or "both" (the last instruction gives one output, the other output is an
   input or any earlier value).

   Pruning (safe, so "no hit" is a proof within the operation set and N):
     - a value equal on every lane to an input, an earlier value or a constant is dropped;
     - every computed value must be used later or be an output;
     - adjacent independent instructions are kept in one canonical order;
     - commutative operands are ordered.

   usage: rsuper N layout(off|neg) op(add|sub|mul|am) target(num|den|both) mode(0|1) Lmax [maxprint] [nomul]
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAXC 65536
#define MAXV 16
typedef uint8_t u8;

static int N, NL, NIN, MODE, TGT;   /* TGT: 0 num, 1 den, 2 both */
static u8 MSK, T;
static u8 val[MAXV][MAXC];
static u8 tn[MAXC], td[MAXC], tdalt[MAXC];
static u8 consts[4];
static const char *cname[4] = {"1", "T", "~T", "-1"};
static const char *inname[6] = {"n1", "d1", "n2", "d2", "P", "Q"};
static int allow_mul = 1;

enum { U_NOT, U_NEG, U_SHL1, U_SHLT, U_SHRT, U_SART,
       B_ADD, B_SUB, B_MUL, B_AND, B_OR, B_XOR, B_ANDN,
       C_AND, C_OR, C_XOR, C_ADD, C_RSUB, NOPS };
static const char *opname[NOPS] = {"not", "neg", "shl 1", "shl N-1", "shr N-1", "sar N-1",
    "+", "-", "*", "and", "or", "xor", "andn", "and", "or", "xor", "+", "rsub"};

typedef struct { int op, a, b; } Ins;

static inline u8 ap1(int op, u8 x) {
    switch (op) {
    case U_NOT: return ~x & MSK;
    case U_NEG: return (u8)(-x) & MSK;
    case U_SHL1: return (u8)(x << 1) & MSK;
    case U_SHLT: return (u8)(x << (N - 1)) & MSK;
    case U_SHRT: return x >> (N - 1);
    case U_SART: return (x & T) ? MSK : 0;
    }
    return 0;
}
static inline u8 ap2(int op, u8 x, u8 y) {
    switch (op) {
    case B_ADD: case C_ADD: return (u8)(x + y) & MSK;
    case B_SUB: return (u8)(x - y) & MSK;
    case C_RSUB: return (u8)(y - x) & MSK;
    case B_MUL: return (u8)(x * y) & MSK;
    case B_AND: case C_AND: return x & y;
    case B_OR: case C_OR: return x | y;
    case B_XOR: case C_XOR: return x ^ y;
    case B_ANDN: return x & ~y & MSK;
    }
    return 0;
}
static inline int isbin(int op) { return op >= B_ADD && op <= B_ANDN; }
static inline u8 apply(const Ins *I, int lane) {
    if (I->op <= U_SART) return ap1(I->op, val[I->a][lane]);
    if (isbin(I->op)) return ap2(I->op, val[I->a][lane], val[I->b][lane]);
    return ap2(I->op, val[I->a][lane], consts[I->b]);
}
#define LOOP(expr) for (int i = 0; i < NL; i++) out[i] = (expr)
static void compute(const Ins *I, u8 *out) {
    int op = I->op; const u8 *x = val[I->a];
    if (op <= U_SART) {
        int sh = N - 1;
        switch (op) {
        case U_NOT: LOOP(~x[i] & MSK); break;
        case U_NEG: LOOP((u8)(-x[i]) & MSK); break;
        case U_SHL1: LOOP((u8)(x[i] << 1) & MSK); break;
        case U_SHLT: LOOP((u8)(x[i] << sh) & MSK); break;
        case U_SHRT: LOOP(x[i] >> sh); break;
        case U_SART: LOOP((u8)(0 - (x[i] >> sh)) & MSK); break;
        }
    } else if (isbin(op)) {
        const u8 *y = val[I->b];
        switch (op) {
        case B_ADD: LOOP((u8)(x[i] + y[i]) & MSK); break;
        case B_SUB: LOOP((u8)(x[i] - y[i]) & MSK); break;
        case B_MUL: LOOP((u8)(x[i] * y[i]) & MSK); break;
        case B_AND: LOOP(x[i] & y[i]); break;
        case B_OR: LOOP(x[i] | y[i]); break;
        case B_XOR: LOOP(x[i] ^ y[i]); break;
        case B_ANDN: LOOP(x[i] & ~y[i] & MSK); break;
        }
    } else {
        u8 c = consts[I->b];
        switch (op) {
        case C_AND: LOOP(x[i] & c); break;
        case C_OR: LOOP(x[i] | c); break;
        case C_XOR: LOOP(x[i] ^ c); break;
        case C_ADD: LOOP((u8)(x[i] + c) & MSK); break;
        case C_RSUB: LOOP((u8)(c - x[i]) & MSK); break;
        }
    }
}

static int L;
static Ins prog[12];
static int uses[MAXV];
static int isnum[MAXV], isden[MAXV];
static long long nodes, hits;
static int maxprint = 40;

static int match_num(const u8 *v) { return !memcmp(v, tn, NL); }
static int match_den(const u8 *v) { for (int i = 0; i < NL; i++) if (v[i] != td[i] && v[i] != tdalt[i]) return 0; return 1; }

static void pv(int idx, char *buf) {
    if (idx < NIN) strcpy(buf, inname[idx]); else sprintf(buf, "v%d", idx - NIN + 1);
}
static void print_prog(int len, const char *outs) {
    char a[16], b[16], dst[16];
    printf("  ");
    for (int i = 0; i < len; i++) {
        const Ins *I = &prog[i]; pv(I->a, a); pv(NIN + i, dst);
        if (I->op <= U_SART) printf("%s=%s %s; ", dst, opname[I->op], a);
        else if (isbin(I->op)) { pv(I->b, b); printf("%s=%s %s %s; ", dst, a, opname[I->op], b); }
        else printf("%s=%s %s %s; ", dst, a, opname[I->op], cname[I->b]);
    }
    printf(" [%s]\n", outs);
}

static int code(const Ins *I) { return (I->op * 64 + I->a) * 64 + I->b; }
static int depends(const Ins *I, int idx) { return I->a == idx || (isbin(I->op) && I->b == idx); }
static int is_dup(const u8 *v, int nv) {
    for (int j = 0; j < nv; j++) if (!memcmp(v, val[j], NL)) return 1;
    for (int i = 1; i < NL; i++) if (v[i] != v[0]) return 0;
    for (int j = 0; j < 4; j++) if (v[0] == consts[j]) return 1;
    return 0;
}

static u8 fin[MAXC];
static void try_final(Ins I, int pos) {
    int nv = NIN + pos;
    /* quick lane test before full compute */
    int fnum = (TGT != 1), fden = (TGT != 0);
    for (int lane = 0; lane < NL && (fnum || fden); lane++) {
        u8 r = apply(&I, lane);
        if (fnum && r != tn[lane]) fnum = 0;
        if (fden && r != td[lane] && r != tdalt[lane]) fden = 0;
    }
    if (!fnum && !fden) return;
    /* unused computed values not consumed by the final */
    int unused[MAXV], nu = 0;
    for (int j = NIN; j < nv; j++) if (!uses[j] && !depends(&I, j)) unused[nu++] = j;
    prog[pos] = I;
    char outs[64];
    if (TGT == 0 || TGT == 1) {
        if (nu) return;
        hits++; if (hits <= maxprint) print_prog(pos + 1, TGT ? "den = last" : "num = last");
        return;
    }
    /* both: the last is one output, the other is an earlier value (unused ones must be it) */
    for (int which = 0; which < 2; which++) {
        if (which == 0 && !fnum) continue;
        if (which == 1 && !fden) continue;
        int found = -1;
        if (nu > 1) continue;
        if (nu == 1) { int j = unused[0]; if (which == 0 ? isden[j] : isnum[j]) found = j; else continue; }
        else for (int j = 0; j < nv; j++) if (which == 0 ? isden[j] : isnum[j]) { found = j; break; }
        if (found < 0) continue;
        char o[16]; pv(found, o);
        sprintf(outs, which == 0 ? "num = last, den = %s" : "den = last, num = %s", o);
        hits++; if (hits <= maxprint) print_prog(pos + 1, outs);
        return;
    }
}

static int live_count(int nv) { int c = 0; for (int j = NIN; j < nv; j++) if (!uses[j]) c++; return c; }
static void gen(int pos);
static void emit(Ins I, int pos) {
    if (pos > 0 && !depends(&I, NIN + pos - 1) && code(&I) < code(&prog[pos - 1])) return;
    int nv = NIN + pos;
    if (pos == L - 1) { try_final(I, pos); return; }
    compute(&I, val[nv]);
    nodes++;
    if (is_dup(val[nv], nv)) return;
    prog[pos] = I;
    isnum[nv] = (TGT == 2) && match_num(val[nv]);
    isden[nv] = (TGT == 2) && match_den(val[nv]);
    uses[I.a]++; if (isbin(I.op)) uses[I.b]++;
    uses[nv] = 0;
    int live = live_count(nv + 1), remaining = L - 1 - pos;
    if (live <= remaining + (TGT == 2)) gen(pos + 1);
    uses[I.a]--; if (isbin(I.op)) uses[I.b]--;
}
static void gen(int pos) {
    int nv = NIN + pos; Ins I;
    for (int a = 0; a < nv; a++) {
        for (int op = U_NOT; op <= U_SART; op++) { I.op = op; I.a = a; I.b = 0; emit(I, pos); }
        for (int op = C_AND; op <= C_RSUB; op++) for (int c = 0; c < 4; c++) {
            if (op == C_ADD && c == 1) continue;              /* add T = xor T */
            I.op = op; I.a = a; I.b = c; emit(I, pos); }
        for (int b = 0; b < nv; b++) {
            if (b == a) continue;
            for (int op = B_ADD; op <= B_ANDN; op++) {
                if (op == B_MUL && !allow_mul) continue;
                int comm = (op == B_ADD || op == B_MUL || op == B_AND || op == B_OR || op == B_XOR);
                if (comm && b < a) continue;
                I.op = op; I.a = a; I.b = b; emit(I, pos);
            }
        }
    }
}

/* ---- lanes ---- */
static int LAYOUT;   /* 0 off, 1 neg */
static u8 dw(int D, int a) { return LAYOUT == 0 ? (u8)((D + (a ? T : 0)) & MSK) : (u8)((a ? -D : D) & MSK); }
static void add_lanes(int op) {   /* op 0 add, 1 sub, 2 mul */
    int Tv = 1 << (N - 1);
    for (int D1 = 1; D1 < Tv; D1++) for (int D2 = 1; D2 < Tv; D2++) {
        if (D1 * D2 >= Tv) continue;
        for (int a1 = 0; a1 < 2; a1++) for (int a2 = 0; a2 < 2; a2++) {
            if (op != 2 && a1 != a2) continue;
            for (int n1 = -Tv; n1 < Tv; n1++) for (int n2 = -Tv; n2 < Tv; n2++) {
                int t, ta;
                if (op == 0) { t = n1 * D2 + n2 * D1; ta = a1; }
                else if (op == 1) { t = n1 * D2 - n2 * D1; ta = a1; }
                else { t = n1 * n2 * ((a1 & a2) ? -1 : 1); ta = a1 ^ a2; }
                if (t < -Tv || t >= Tv) continue;
                if (NL >= MAXC) { fprintf(stderr, "too many lanes\n"); exit(1); }
                u8 w1 = dw(D1, a1), w2 = dw(D2, a2);
                val[0][NL] = n1 & MSK; val[1][NL] = w1; val[2][NL] = n2 & MSK; val[3][NL] = w2;
                if (op == 2) { val[4][NL] = (n1 * n2) & MSK; }
                else { val[4][NL] = (op == 0 ? n1 * w2 + n2 * w1 : n1 * w2 - n2 * w1) & MSK; }
                val[5][NL] = (w1 * w2) & MSK;
                tn[NL] = t & MSK; td[NL] = dw(D1 * D2, ta);
                tdalt[NL] = t == 0 ? dw(D1 * D2, 1 - ta) : td[NL];
                NL++;
            }
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 7) { fprintf(stderr, "usage: rsuper N off|neg add|sub|mul|am num|den|both 0|1 Lmax [maxprint] [nomul]\n"); return 1; }
    N = atoi(argv[1]); LAYOUT = !strcmp(argv[2], "neg");
    const char *op = argv[3];
    TGT = !strcmp(argv[4], "num") ? 0 : !strcmp(argv[4], "den") ? 1 : 2;
    MODE = atoi(argv[5]); int Lmax = atoi(argv[6]);
    if (argc > 7) maxprint = atoi(argv[7]);
    if (argc > 8) allow_mul = !atoi(argv[8]);
    MSK = (1 << N) - 1; T = 1 << (N - 1);
    consts[0] = 1; consts[1] = T; consts[2] = ~T & MSK; consts[3] = MSK;
    NIN = MODE ? 6 : 4;
    NL = 0;
    if (!strcmp(op, "add")) add_lanes(0);
    else if (!strcmp(op, "sub")) add_lanes(1);
    else if (!strcmp(op, "mul")) add_lanes(2);
    else if (!strcmp(op, "am")) { add_lanes(0); add_lanes(2); }
    else { fprintf(stderr, "bad op\n"); return 1; }
    /* shuffle lanes (deterministic) so the final test rejects early */
    srand(12345);
    for (int i = NL - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        for (int q = 0; q < 6; q++) { u8 x = val[q][i]; val[q][i] = val[q][j]; val[q][j] = x; }
        u8 x = tn[i]; tn[i] = tn[j]; tn[j] = x; x = td[i]; td[i] = td[j]; td[j] = x;
        x = tdalt[i]; tdalt[i] = tdalt[j]; tdalt[j] = x;
    }
    for (int j = 0; j < NIN; j++) { isnum[j] = match_num(val[j]); isden[j] = match_den(val[j]); }
    printf("N=%d layout=%s op=%s target=%s mode=%d mul=%d lanes=%d\n", N, argv[2], op, argv[4], MODE, allow_mul, NL);
    for (int j = 0; j < NIN; j++) if (isnum[j] || isden[j]) printf("  input %s already is %s\n", inname[j] ? inname[j] : "?", isnum[j] ? "num" : "den");
    if (TGT == 2) {   /* both already present among inputs? */
        int a = 0, b = 0; for (int j = 0; j < NIN; j++) { a |= isnum[j]; b |= isden[j]; }
        if (a && b) { printf("L=0: both outputs are inputs\n"); return 0; }
    }
    for (L = 1; L <= Lmax; L++) {
        nodes = hits = 0; memset(uses, 0, sizeof uses);
        gen(0);
        printf("L=%d: %lld hits, %lld nodes\n", L, hits, nodes); fflush(stdout);
        if (hits) break;
    }
    return 0;
}
