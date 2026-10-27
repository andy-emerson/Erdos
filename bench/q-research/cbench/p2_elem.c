/* Phase two, element-by-element operations: the format against GMP and FLINT.

   usage: p2_elem REP OP W N REPS INDIR OUTFILE [K DEN]
     REP   the format: bi, lc, lr (its compile-time and run-time routines; which one the format uses at each
                  width is decided by the harness from the threshold test), and the libraries:
           mpz    GMP integers over the shared denominator (a careful GMP user's vector form)
           mpq    GMP rationals, one per element, always in lowest terms
           fmpz   FLINT integers over the shared denominator
           fmpq   FLINT rationals, one per element
     OP    as in p1_arith.c (add mul dot div1 div2 sort scan chain), read as rationals over the denominator 1000:
           a + b, ha * hb, the dot product, (999 a)/999, (1000 a)/1000, sorting, the bit-length scan (shared-
           denominator forms only), and x <- x (b/1000) + c/1000; plus
           dec_in   parse N decimal strings ("-123.456") into the representation
           dec_out  write the vector a as N decimal strings
   Results are written as the integers value * 1000^k (k = the operation's denominator power), so one Python
   check serves every contender; dec_out writes the strings themselves. Everything else as in p1_arith.c. */
#define main p1_arith_main
#include "p1_arith.c"
#undef main
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/fmpz_vec.h>
#include <flint/fmpq_vec.h>
#include <ctype.h>

enum { DEC_IN = 100, DEC_OUT = 101 };
static char **DSTR; static char *DOUT; static size_t DOUTW;   /* decimal strings in; output buffer (N * DOUTW) */

/* a decimal string "-ddd.ddd" (exactly 3 places) to sign + digit values without the point, the same helper for all */
static int split_dec(const char *s, unsigned char *dig, int *neg) {
    int n = 0; *neg = 0; if (*s == '-') { *neg = 1; s++; }
    for (; *s; s++) if (*s != '.') dig[n++] = (unsigned char)(*s - '0');
    while (n > 1 && dig[0] == 0) { memmove(dig, dig + 1, --n); }
    return n;
}
static int split_chars(const char *s, char *out, int *neg) {     /* the libraries' path: characters, point removed */
    int n = 0; *neg = 0; if (*s == '-') { *neg = 1; s++; }
    for (; *s; s++) if (*s != '.') out[n++] = *s;
    out[n] = 0; return n;
}
static char *join_chars(char *out, const char *dig, size_t n, int neg) {   /* "ddddd" -> "-dd.ddd" */
    char *p = out; if (neg) *p++ = '-';
    if (n <= 3) { *p++ = '0'; *p++ = '.'; for (size_t i = n; i < 3; i++) *p++ = '0'; memcpy(p, dig, n); p += n; }
    else { memcpy(p, dig, n - 3); p += n - 3; *p++ = '.'; memcpy(p, dig + n - 3, 3); p += 3; }
    *p = 0; return out;
}
static char *join_dec(char *out, const unsigned char *dig, size_t n, int neg) {   /* digits (values) -> "-ddd.ddd" */
    char *p = out; if (neg) *p++ = '-';
    if (n <= 3) { *p++ = '0'; *p++ = '.'; for (size_t i = n; i < 3; i++) *p++ = '0'; for (size_t i = 0; i < n; i++) *p++ = (char)('0' + dig[i]); }
    else { for (size_t i = 0; i < n - 3; i++) *p++ = (char)('0' + dig[i]); *p++ = '.'; for (size_t i = n - 3; i < n; i++) *p++ = (char)('0' + dig[i]); }
    *p = 0; return out;
}

/* ================================================================ mpq: GMP rationals */
static mpq_t *qa, *qb, *qe1, *qe2, *qha, *qhb, *qo, qs, qt, q999, q1000; static mpq_t *qx;
static mpq_t *qconv(int which, unsigned long den) { mpq_t *x = malloc(sizeof(mpq_t) * N); long l = which >= 4 ? LH : L;
    for (long i = 0; i < N; i++) { mpq_init(x[i]); mpz_import(mpq_numref(x[i]), l, -1, 8, 0, 0, MAG[which] + i * l);
        if (SG[which][i]) mpz_neg(mpq_numref(x[i]), mpq_numref(x[i])); mpz_set_ui(mpq_denref(x[i]), den); mpq_canonicalize(x[i]); }
    return x; }
static int q_less(int x, int y) { return mpq_cmp(qa[x], qa[y]) < 0; }
MSORT(q_sortk, q_less)
static void q_setup(void) { qa = qconv(0, 1000); qb = qconv(1, 1000); qe1 = qconv(2, 1000); qe2 = qconv(3, 1000); qha = qconv(4, 1000); qhb = qconv(5, 1000);
    qo = malloc(sizeof(mpq_t) * N); for (long i = 0; i < N; i++) mpq_init(qo[i]); mpq_init(qs); mpq_init(qt);
    mpq_init(q999); mpq_set_ui(q999, 999, 1); mpq_init(q1000); mpq_set_ui(q1000, 1000, 1); }
static void q_run(void) {
    switch (OP) {
    case ADD: for (long i = 0; i < N; i++) mpq_add(qo[i], qa[i], qb[i]); break;
    case MUL: for (long i = 0; i < N; i++) mpq_mul(qo[i], qha[i], qhb[i]); break;
    case DOT: mpq_set_ui(qs, 0, 1); for (long i = 0; i < N; i++) { mpq_mul(qt, qha[i], qhb[i]); mpq_add(qs, qs, qt); } break;
    case DIV1: for (long i = 0; i < N; i++) mpq_div(qo[i], qe1[i], q999); break;
    case DIV2: for (long i = 0; i < N; i++) mpq_div(qo[i], qe2[i], q1000); break;
    case SORT: q_sortk(); break;
    }
}
static void q_scaled(mpz_t z, const mpq_t v, int k) {   /* v * 1000^k as an integer (untimed export) */
    mpz_t p; mpz_init(p); mpz_ui_pow_ui(p, 1000, k); mpz_mul(z, mpq_numref(v), p);
    if (!mpz_divisible_p(z, mpq_denref(v))) { fprintf(stderr, "not a multiple\n"); exit(5); }
    mpz_divexact(z, z, mpq_denref(v)); mpz_clear(p); }
static void q_out(void) {
    if (OP == DOT) { q_scaled(ZONE, qs, 2); out_kind = 1; return; }
    if (OP == SORT) { out_kind = 2; return; }
    int k = OP == MUL ? 2 : 1;
    for (long i = 0; i < N; i++) q_scaled(ZOUT[i], qo[i], k); out_kind = 0;
}
static mpq_t *bq, *cq;
static void q_chain_setup(void) {                    /* the constants b/1000 and c/1000, before timing */
    bq = malloc(sizeof(mpq_t) * N); cq = malloc(sizeof(mpq_t) * N);
    for (long i = 0; i < N; i++) { mpq_init(bq[i]); mpq_set_si(bq[i], CB[i], 1000); mpq_canonicalize(bq[i]);
        mpq_init(cq[i]); mpq_set_si(cq[i], CC[i], 1000); mpq_canonicalize(cq[i]); } }
static int q_chain(void) {
    chain_bounds(); int bx = bx0();
    for (long i = 0; i < N; i++) { mpq_set_si(qx[i], X0[i], 1000); mpq_canonicalize(qx[i]); }
    for (long k = 0; k < K; k++) {
        bx = chain_next(bx); if (bx + 1 > CAP) return 0;
        for (long i = 0; i < N; i++) { mpq_mul(qx[i], qx[i], bq[i]); mpq_add(qx[i], qx[i], cq[i]); }
    }
    final_width = 0; return 1;
}

/* ================================================================ fmpz and fmpq: FLINT */
static fmpz *fa, *fb, *fe1, *fe2, *fha, *fhb, *fo, *fx; static fmpz_t fs; static long fmax;
static fmpz *fconv(int which) { fmpz *x = _fmpz_vec_init(N); long l = which >= 4 ? LH : L; mpz_t z; mpz_init(z);
    for (long i = 0; i < N; i++) { mpz_import(z, l, -1, 8, 0, 0, MAG[which] + i * l); if (SG[which][i]) mpz_neg(z, z); fmpz_set_mpz(x + i, z); }
    mpz_clear(z); return x; }
static int f_less(int x, int y) { return fmpz_cmp(fa + x, fa + y) < 0; }
MSORT(f_sortk, f_less)
static void f_setup(void) { fa = fconv(0); fb = fconv(1); fe1 = fconv(2); fe2 = fconv(3); fha = fconv(4); fhb = fconv(5);
    fo = _fmpz_vec_init(N); fmpz_init(fs); }
static void f_run(void) {
    switch (OP) {
    case ADD: for (long i = 0; i < N; i++) fmpz_add(fo + i, fa + i, fb + i); break;
    case MUL: for (long i = 0; i < N; i++) fmpz_mul(fo + i, fha + i, fhb + i); break;
    case DOT: fmpz_zero(fs); for (long i = 0; i < N; i++) fmpz_addmul(fs, fha + i, fhb + i); break;
    case DIV1: for (long i = 0; i < N; i++) fmpz_divexact_ui(fo + i, fe1 + i, 999); break;
    case DIV2: for (long i = 0; i < N; i++) fmpz_divexact_ui(fo + i, fe2 + i, 1000); break;
    case SORT: f_sortk(); break;
    case SCAN: { long m = 0; for (long i = 0; i < N; i++) { long b = (long)fmpz_bits(fa + i); if (b > m) m = b; } fmax = m; break; }
    }
}
static void f_out(void) {
    if (OP == DOT) { fmpz_get_mpz(ZONE, fs); out_kind = 1; return; }
    if (OP == SCAN) { mpz_set_ui(ZONE, (unsigned long)fmax); out_kind = 1; return; }
    if (OP == SORT) { out_kind = 2; return; }
    for (long i = 0; i < N; i++) fmpz_get_mpz(ZOUT[i], fo + i); out_kind = 0;
}
static int f_chain(void) {
    chain_bounds(); int bx = bx0();
    for (long i = 0; i < N; i++) fmpz_set_si(fx + i, X0[i]);
    fmpz_t cs; fmpz_init_set_ui(cs, 1);
    for (long k = 0; k < K; k++) {
        bx = chain_next(bx); if (bx + 1 > CAP) { fmpz_clear(cs); return 0; }
        fmpz_mul_ui(cs, cs, (ulong)DEN);
        for (long i = 0; i < N; i++) { fmpz_mul_si(fx + i, fx + i, CB[i]); fmpz_addmul_si(fx + i, cs, CC[i]); }
    }
    fmpz_clear(cs); final_width = 0; return 1;
}
static fmpq *ga, *gb, *ge1, *ge2, *gha, *ghb, *go, *gx; static fmpq_t gs, gt; static fmpz_t g999, g1000;
static fmpq *gconv(int which) { fmpq *x = _fmpq_vec_init(N); long l = which >= 4 ? LH : L; mpz_t z; mpz_init(z); fmpz_t num, den;
    fmpz_init(num); fmpz_init_set_ui(den, 1000);
    for (long i = 0; i < N; i++) { mpz_import(z, l, -1, 8, 0, 0, MAG[which] + i * l); if (SG[which][i]) mpz_neg(z, z); fmpz_set_mpz(num, z);
        fmpq_set_fmpz_frac(x + i, num, den); }
    mpz_clear(z); fmpz_clear(num); fmpz_clear(den); return x; }
static int g_less(int x, int y) { return fmpq_cmp(ga + x, ga + y) < 0; }
MSORT(g_sortk, g_less)
static void g_setup(void) { ga = gconv(0); gb = gconv(1); ge1 = gconv(2); ge2 = gconv(3); gha = gconv(4); ghb = gconv(5);
    go = _fmpq_vec_init(N); fmpq_init(gs); fmpq_init(gt); fmpz_init_set_ui(g999, 999); fmpz_init_set_ui(g1000, 1000); }
static void g_run(void) {
    switch (OP) {
    case ADD: for (long i = 0; i < N; i++) fmpq_add(go + i, ga + i, gb + i); break;
    case MUL: for (long i = 0; i < N; i++) fmpq_mul(go + i, gha + i, ghb + i); break;
    case DOT: fmpq_zero(gs); for (long i = 0; i < N; i++) fmpq_addmul(gs, gha + i, ghb + i); break;
    case DIV1: for (long i = 0; i < N; i++) fmpq_div_fmpz(go + i, ge1 + i, g999); break;
    case DIV2: for (long i = 0; i < N; i++) fmpq_div_fmpz(go + i, ge2 + i, g1000); break;
    case SORT: g_sortk(); break;
    }
}
static void g_scaled(mpz_t z, const fmpq *v, int k) { mpq_t q; mpq_init(q); fmpq_get_mpq(q, v); q_scaled(z, q, k); mpq_clear(q); }
static void g_out(void) {
    if (OP == DOT) { g_scaled(ZONE, gs, 2); out_kind = 1; return; }
    if (OP == SORT) { out_kind = 2; return; }
    int k = OP == MUL ? 2 : 1;
    for (long i = 0; i < N; i++) g_scaled(ZOUT[i], go + i, k); out_kind = 0;
}
static fmpq *gbq, *gcq;
static void g_chain_setup(void) { gbq = _fmpq_vec_init(N); gcq = _fmpq_vec_init(N);
    for (long i = 0; i < N; i++) { fmpq_set_si(gbq + i, CB[i], 1000); fmpq_set_si(gcq + i, CC[i], 1000); } }
static int g_chain(void) {
    chain_bounds(); int bx = bx0();
    for (long i = 0; i < N; i++) fmpq_set_si(gx + i, X0[i], 1000);
    for (long k = 0; k < K; k++) {
        bx = chain_next(bx); if (bx + 1 > CAP) return 0;
        for (long i = 0; i < N; i++) { fmpq_mul(gx + i, gx + i, gbq + i); fmpq_add(gx + i, gx + i, gcq + i); }
    }
    final_width = 0; return 1;
}

/* ================================================================ decimal text, for every representation */
static unsigned char *DIG; static uint64_t *DLIMB;      /* scratch */
static void dec_in(const char *rep) {
    int neg; mpz_t z; (void)z;
    if (is(rep, "lr") || is(rep, "lc")) {                /* the format's limbs, sign apart */
        for (long i = 0; i < N; i++) { int n = split_dec(DSTR[i], DIG, &neg); uint64_t *x = lra + i * L;
            for (int j = 0; j < n; j++) if (DIG[j] > 9) { fprintf(stderr, "not a decimal digit\n"); exit(6); }
            mp_size_t sz = mpn_set_str(DLIMB, DIG, n, 10);
            if (sz > L || (sz == L && (DLIMB[L - 1] >> 63))) { fprintf(stderr, "value exceeds the width\n"); exit(6); }
            memset(x, 0, 8 * L); memcpy(x, DLIMB, 8 * sz); lrsa[i] = (unsigned char)neg; }
    } else if (is(rep, "bi")) {                          /* through limbs into _BitInt(W): two's complement */
        for (long i = 0; i < N; i++) { int n = split_dec(DSTR[i], DIG, &neg);
            for (int j = 0; j < n; j++) if (DIG[j] > 9) { fprintf(stderr, "not a decimal digit\n"); exit(6); }
            mp_size_t sz = mpn_set_str(DLIMB, DIG, n, 10); uint64_t *x = lra + i * L;
            if (sz > L || (sz == L && (DLIMB[L - 1] >> 63))) { fprintf(stderr, "value exceeds the width\n"); exit(6); }
            memset(x, 0, 8 * L); memcpy(x, DLIMB, 8 * sz);
            if (neg) { unsigned c = 1; for (long j = 0; j < L; j++) { x[j] = ~x[j] + c; c = c && x[j] == 0; } } }
    } else if (is(rep, "mpz")) {
        for (long i = 0; i < N; i++) { split_chars(DSTR[i], (char *)DIG, &neg); mpz_set_str(za[i], (char *)DIG, 10); if (neg) mpz_neg(za[i], za[i]); }
    } else if (is(rep, "mpq")) {
        for (long i = 0; i < N; i++) { split_chars(DSTR[i], (char *)DIG, &neg);
            mpz_set_str(mpq_numref(qa[i]), (char *)DIG, 10); if (neg) mpz_neg(mpq_numref(qa[i]), mpq_numref(qa[i]));
            mpz_set_ui(mpq_denref(qa[i]), 1000); mpq_canonicalize(qa[i]); }
    } else if (is(rep, "fmpz")) {
        for (long i = 0; i < N; i++) { split_chars(DSTR[i], (char *)DIG, &neg); fmpz_set_str(fa + i, (char *)DIG, 10); if (neg) fmpz_neg(fa + i, fa + i); }
    } else {
        fmpz_t num, den; fmpz_init(num); fmpz_init_set_ui(den, 1000);
        for (long i = 0; i < N; i++) { split_chars(DSTR[i], (char *)DIG, &neg);
            fmpz_set_str(num, (char *)DIG, 10); if (neg) fmpz_neg(num, num); fmpq_set_fmpz_frac(ga + i, num, den); }
        fmpz_clear(num); fmpz_clear(den);
    }
}
static void dec_out(const char *rep) {
    if (is(rep, "lr") || is(rep, "lc") || is(rep, "bi")) {
        for (long i = 0; i < N; i++) {
            uint64_t *x = lra + i * L; int neg = lrsa[i];
            memcpy(DLIMB, x, 8 * L);
            if (is(rep, "bi")) { neg = (int64_t)x[L - 1] < 0;               /* two's complement storage */
                if (neg) { unsigned c = 1; for (long j = 0; j < L; j++) { DLIMB[j] = ~DLIMB[j] + c; c = c && DLIMB[j] == 0; } } }
            mp_size_t n = L; while (n > 1 && !DLIMB[n - 1]) n--;
            size_t nd = (n == 1 && !DLIMB[0]) ? (DIG[0] = 0, 1) : mpn_get_str(DIG, 10, DLIMB, n);
            join_dec(DOUT + i * DOUTW, DIG, nd, neg && !(nd == 1 && DIG[0] == 0));
        }
    } else {                                             /* the libraries: their own decimal conversion */
        mpz_t z, t, sc; mpz_init(z); mpz_init(t); mpz_init(sc); mpq_t q; mpq_init(q);
        char *buf = malloc(DOUTW + 8);
        for (long i = 0; i < N; i++) {
            if (is(rep, "mpz")) mpz_set(z, za[i]);
            else if (is(rep, "fmpz")) fmpz_get_mpz(z, fa + i);
            else {                                       /* p/q in lowest terms with q | 1000: scale to over 1000 */
                if (is(rep, "mpq")) { mpz_set_ui(t, 1000); mpz_divexact(sc, t, mpq_denref(qa[i])); mpz_mul(z, mpq_numref(qa[i]), sc); }
                else { fmpq_get_mpq(q, ga + i); mpz_set_ui(t, 1000); mpz_divexact(sc, t, mpq_denref(q)); mpz_mul(z, mpq_numref(q), sc); }
            }
            int neg = mpz_sgn(z) < 0; mpz_abs(z, z);
            mpz_get_str(buf, 10, z); join_chars(DOUT + i * DOUTW, buf, strlen(buf), neg);
        }
        free(buf); mpz_clear(z); mpz_clear(t); mpz_clear(sc); mpq_clear(q);
    }
}

int main(int argc, char **argv) {
    if (argc < 8) { fprintf(stderr, "usage: p2_elem REP OP W N REPS INDIR OUTFILE [K DEN]\n"); return 2; }
    const char *rep = argv[1], *op = argv[2];
    int lib = is(rep, "mpq") || is(rep, "fmpz") || is(rep, "fmpq"), dec = is(op, "dec_in") || is(op, "dec_out");
    if (!lib && !dec) return p1_arith_main(argc, argv);           /* the format's routines and mpz, unchanged */
    W = atoi(argv[3]); N = atol(argv[4]); int reps = atoi(argv[5]); const char *dir = argv[6]; L = W / 64; LH = L > 1 ? L / 2 : 1;
    const char *ops[] = {"add", "mul", "dot", "div1", "div2", "sort", "scan", "widen", "chain", "pipe"};
    OP = -1; for (int i = 0; i < 10; i++) if (is(op, ops[i])) OP = i;
    if (is(op, "dec_in")) OP = DEC_IN; if (is(op, "dec_out")) OP = DEC_OUT;
    if (OP < 0 || OP == WIDEN || OP == PIPE || (OP == SCAN && (is(rep, "mpq") || is(rep, "fmpq")))) { fprintf(stderr, "not defined for %s\n", rep); return 2; }
    mpz_init(PW); mpz_init(ZONE);
    perm = malloc(sizeof(int) * N); ptmp = malloc(sizeof(int) * N);
    ZOUT = malloc(sizeof(mpz_t) * N); for (long i = 0; i < N; i++) mpz_init(ZOUT[i]);
    int mq = is(rep, "mpq"), fz = is(rep, "fmpz"), fq = is(rep, "fmpq");
    if (OP == CHAIN) {
        K = atol(argv[8]); DEN = atol(argv[9]);
        X0 = loadf(dir, "x0.bin", 8 * N); CB = loadf(dir, "cb.bin", 8 * N); CC = loadf(dir, "cc.bin", 8 * N);
        if (mq) { qx = malloc(sizeof(mpq_t) * N); for (long i = 0; i < N; i++) mpq_init(qx[i]); q_chain_setup(); }
        if (fz) fx = _fmpz_vec_init(N);
        if (fq) { gx = _fmpq_vec_init(N); g_chain_setup(); }
    } else {
        const char *nm[6] = {"a", "b", "e1", "e2", "ha", "hb"};
        for (int a = 0; a < 6; a++) { char f[16]; snprintf(f, sizeof f, "%s.mag", nm[a]); MAG[a] = loadf(dir, f, 8 * (size_t)N * (a >= 4 ? LH : L));
            snprintf(f, sizeof f, "%s.sgn", nm[a]); SG[a] = loadf(dir, f, N); }
        if (mq) q_setup(); else if (fz) f_setup(); else if (fq) g_setup();
        else if (is(rep, "mpz")) z_setup();
        else if (is(rep, "bi") || is(rep, "lc") || is(rep, "lr")) {
            lr_setup();                                   /* limbs of a with signs: sign-magnitude, or two's complement for bi */
            if (is(rep, "bi")) for (long i = 0; i < N; i++) if (lrsa[i]) { uint64_t *x = lra + i * L; unsigned c = 1;
                for (long j = 0; j < L; j++) { x[j] = ~x[j] + c; c = c && x[j] == 0; } }
        } else { fprintf(stderr, "decimal text: bi, lc, lr, mpz, mpq, fmpz, fmpq\n"); return 2; }
        if (OP == DEC_IN || OP == DEC_OUT) {
            DIG = malloc(W + 64); DLIMB = malloc(8 * (L + 2)); DOUTW = (size_t)(W * 0.302) + 8; DOUT = malloc(N * DOUTW);
            if (OP == DEC_IN) {                           /* strings: one per line in dec.txt (read before timing) */
                char path[4096]; snprintf(path, sizeof path, "%s/dec.txt", dir); FILE *f = fopen(path, "r"); if (!f) { perror(path); return 2; }
                DSTR = malloc(sizeof(char *) * N); char *line = NULL; size_t cap = 0;
                for (long i = 0; i < N; i++) { ssize_t r = getline(&line, &cap, f); if (r <= 0) { fprintf(stderr, "short dec\n"); return 2; }
                    if (line[r - 1] == '\n') line[--r] = 0; DSTR[i] = strdup(line); }
                fclose(f);
            }
        }
    }
    double *t = malloc(sizeof(double) * reps); int ok = 1;
    for (int r = -1; r < reps; r++) {
        double t0 = now();
        if (OP == CHAIN) ok = mq ? q_chain() : fz ? f_chain() : g_chain();
        else if (OP == DEC_IN) dec_in(rep);
        else if (OP == DEC_OUT) dec_out(rep);
        else if (mq) q_run(); else if (fz) f_run(); else g_run();
        double dt = now() - t0; if (r >= 0) t[r] = dt; if (!ok) break;
    }
    if (!ok) { printf("exceeds\n"); return 3; }
    qsort(t, reps, sizeof(double), cmpd);
    printf("%.9g %.9g %d\n", t[reps / 2], t[0], W);
    FILE *f = fopen(argv[7], "w");
    if (OP == DEC_OUT) { for (long i = 0; i < N; i++) fprintf(f, "%s\n", DOUT + i * DOUTW); fclose(f); return 0; }
    if (OP == DEC_IN) {                                   /* the parsed values, as integers over 1000 */
        for (long i = 0; i < N; i++) {
            if (is(rep, "lr") || is(rep, "lc") || is(rep, "bi")) {
                uint64_t *x = lra + i * L; int neg = is(rep, "bi") ? (int64_t)x[L - 1] < 0 : lrsa[i];
                memcpy(DLIMB, x, 8 * L);
                if (is(rep, "bi") && neg) { unsigned c = 1; for (long j = 0; j < L; j++) { DLIMB[j] = ~DLIMB[j] + c; c = c && DLIMB[j] == 0; } }
                mpz_import(ZOUT[i], L, -1, 8, 0, 0, DLIMB); if (neg) mpz_neg(ZOUT[i], ZOUT[i]);
            } else if (is(rep, "mpz")) mpz_set(ZOUT[i], za[i]);
            else if (fz) fmpz_get_mpz(ZOUT[i], fa + i);
            else if (mq) q_scaled(ZOUT[i], qa[i], 1);
            else g_scaled(ZOUT[i], ga + i, 1);
            gmp_fprintf(f, "%Zd\n", ZOUT[i]);
        }
        fclose(f); return 0;
    }
    if (OP == CHAIN) {
        for (long i = 0; i < N; i++) { if (mq) q_scaled(ZOUT[i], qx[i], (int)K + 1); else if (fz) fmpz_get_mpz(ZOUT[i], fx + i); else g_scaled(ZOUT[i], gx + i, (int)K + 1); }
        out_kind = 0;
    } else if (mq) q_out(); else if (fz) f_out(); else g_out();
    if (out_kind == 1) gmp_fprintf(f, "%Zd\n", ZONE);
    else if (out_kind == 2) for (long i = 0; i < N; i++) fprintf(f, "%d\n", perm[i]);
    else for (long i = 0; i < N; i++) gmp_fprintf(f, "%Zd\n", ZOUT[i]);
    fclose(f);
    return 0;
}
