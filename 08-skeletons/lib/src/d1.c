/* d1.c: Design 1, common divisors, in the note's implementation form (spec §7).  Seven structures: numerators,
 * denominators, the lookup (spec §5), divisors, the pair table, incidence bits and residual rows. */
#include "sk_d1.h"

#include "sk_internal.h"

typedef struct d1_res { /* one residual row; this header is the per-row header of spec §12.2 */
    sk_int *r;
    uint32_t len, cap;
} d1_res;

struct sk_d1 {
    /* numerators: the numerator and its denominator index (two parallel arrays, so an entry is w/8 + 4 bytes) */
    sk_int *num;
    uint32_t *nk;
    uint32_t N, Ncap;
    /* denominators */
    sk_int *den;
    uint32_t D, Dcap;
    sk_lookup look;
    /* divisors, strictly increasing, entry 0 is 1 */
    sk_int *dv;
    uint32_t C, Ccap;
    /* pair table: bit j(j-1)/2 + i for the pair i < j, packed column by column */
    uint64_t *pair;
    uint64_t pair_cap; /* words allocated */
    /* incidence bits: D rows of W words, one contiguous array with stride W */
    uint64_t *inc;
    uint32_t W;
    uint32_t inc_cap; /* rows allocated */
    /* residual rows */
    d1_res *res;
    uint32_t res_cap;

    sk_opctx ctx;
    sk_overflow ovf;
#ifdef SK_COUNT
    sk_counters cnt;
#endif
#ifdef SK_AUDIT
    /* the change list (spec §13.2): read only by the audits */
    uint32_t mark_N, mark_D;
    sk_int *chg_div; /* divisors inserted since the last audit (by value: columns move) */
    uint32_t n_chg_div, cap_chg_div;
    uint32_t *chg_row; /* rows that gained a bit or a residual */
    uint32_t n_chg_row, cap_chg_row;
    uint8_t *row_dirty;
    uint32_t row_dirty_cap;
    uint32_t *chg_pair; /* pair bits set, as (i, j) */
    uint32_t n_chg_pair, cap_chg_pair;
    char msg[200];
#endif
};

#define OVF(step) return sk_overflow_at(&c->ovf, &c->ctx, (step))

sk_d1 *sk_d1_create(void) {
    sk_d1 *c = sk_xcalloc(1, sizeof *c);
    sk_lookup_init(&c->look);
    c->Ccap = 16;
    c->dv = sk_xmalloc(sizeof(sk_int) * c->Ccap);
    c->dv[0] = 1;
    c->C = 1;
    c->W = 1;
    return c;
}

void sk_d1_destroy(sk_d1 *c) {
    if (!c) return;
    free(c->num);
    free(c->nk);
    free(c->den);
    sk_lookup_free(&c->look);
    free(c->dv);
    free(c->pair);
    free(c->inc);
    for (uint32_t k = 0; k < c->D; k++) free(c->res[k].r);
    free(c->res);
#ifdef SK_AUDIT
    free(c->chg_div);
    free(c->chg_row);
    free(c->row_dirty);
    free(c->chg_pair);
#endif
#ifdef SK_COUNT
    sk_cur = &sk_sink;
#endif
    free(c);
}

/* ----------------------------------------------------------------------------------------- change list */

#ifdef SK_AUDIT
static void note_row(sk_d1 *c, uint32_t k) {
    if (c->row_dirty_cap < c->D) {
        uint32_t nc = c->res_cap;
        c->row_dirty = sk_xrealloc(c->row_dirty, nc);
        memset(c->row_dirty + c->row_dirty_cap, 0, nc - c->row_dirty_cap);
        c->row_dirty_cap = nc;
    }
    if (c->row_dirty[k]) return;
    c->row_dirty[k] = 1;
    SK_RESERVE(c->chg_row, c->n_chg_row, c->cap_chg_row, 16);
    c->chg_row[c->n_chg_row++] = k;
}
static void note_div(sk_d1 *c, sk_int g) {
    SK_RESERVE(c->chg_div, c->n_chg_div, c->cap_chg_div, 16);
    c->chg_div[c->n_chg_div++] = g;
}
static void note_pair(sk_d1 *c, uint32_t i, uint32_t j) {
    if (c->n_chg_pair + 2 > c->cap_chg_pair) {
        c->cap_chg_pair = c->cap_chg_pair ? 2 * c->cap_chg_pair : 32;
        c->chg_pair = sk_xrealloc(c->chg_pair, sizeof(uint32_t) * c->cap_chg_pair);
    }
    c->chg_pair[c->n_chg_pair++] = i;
    c->chg_pair[c->n_chg_pair++] = j;
}
#define NOTE_ROW(k) note_row(c, (k))
#define NOTE_DIV(g) note_div(c, (g))
#define NOTE_PAIR(i, j) note_pair(c, (i), (j))
#else
#define NOTE_ROW(k) ((void)0)
#define NOTE_DIV(g) ((void)0)
#define NOTE_PAIR(i, j) ((void)0)
#endif

/* ----------------------------------------------------------------------------------------- pair table */

static inline uint64_t pair_bit(uint32_t i, uint32_t j) { /* i < j; index arithmetic */
    return (uint64_t)j * (j - 1) / 2 + i;
}

static inline bool pair_get(const sk_d1 *c, uint32_t i, uint32_t j) {
    if (i > j) {
        uint32_t t = i;
        i = j;
        j = t;
    }
    CNT(pair_read, 1);
    uint64_t b = pair_bit(i, j);
    return (c->pair[b >> 6] >> (b & 63)) & 1;
}

static inline void pair_set(sk_d1 *c, uint32_t i, uint32_t j) {
    if (i > j) {
        uint32_t t = i;
        i = j;
        j = t;
    }
    uint64_t b = pair_bit(i, j);
    c->pair[b >> 6] |= UINT64_C(1) << (b & 63);
    NOTE_PAIR(i, j);
}

/* ----------------------------------------------------------------------------------------- incidence, residuals */

static inline uint64_t *row_of(const sk_d1 *c, uint32_t k) { return c->inc + (size_t)k * c->W; }

static inline bool bit_get(const sk_d1 *c, uint32_t k, uint32_t col) {
    return (row_of(c, k)[col >> 6] >> (col & 63)) & 1;
}

/* search(i, j) (spec §7.2): the largest column set in both rows. */
static uint32_t search(const sk_d1 *c, uint32_t i, uint32_t j) {
    const uint64_t *ri = row_of(c, i), *rj = row_of(c, j);
    for (uint32_t w = c->W; w-- > 0;) {
        CNT(scan_and_words, 1);
        uint64_t x = ri[w] & rj[w];
        if (x != 0) return 64 * w + (uint32_t)sk_hsb(x);
    }
    sk_die("d1: search found no common column (column 0 is not set)");
}

/* The position in residual row k of column col: the popcount of the row's bits below col (spec §7.1 item 3). */
static uint32_t res_pos(const sk_d1 *c, uint32_t k, uint32_t col) {
    const uint64_t *row = row_of(c, k);
    uint32_t full = col >> 6, b = col & 63, pos = 0;
    for (uint32_t w = 0; w < full; w++) {
        CNT(scan_popcount_words, 1);
        pos += (uint32_t)sk_popcount(row[w]);
    }
    if (b) {
        CNT(scan_popcount_words, 1);
        pos += (uint32_t)sk_popcount(row[full] & ((UINT64_C(1) << b) - 1));
    }
    return pos;
}

static inline sk_int residual(const sk_d1 *c, uint32_t k, uint32_t col) { return c->res[k].r[res_pos(c, k, col)]; }

/* Shift row bits at and above column col up by one (spec §7.3, immediate shifting). */
static void shift_row(uint64_t *row, uint32_t W, uint32_t col) {
    uint32_t w0 = col >> 6, b = col & 63;
    for (uint32_t w = W - 1; w > w0; w--) row[w] = (row[w] << 1) | (row[w - 1] >> 63);
    uint64_t low = b ? (UINT64_C(1) << b) - 1 : 0;
    row[w0] = (row[w0] & low) | ((row[w0] & ~low) << 1);
}

/* Insert divisor g at column col: grow the stride if C passes 64W, then shift every row if col is below the old C
 * (spec §7.3; D1-c: an insertion at the end shifts nothing). */
static void insert_divisor(sk_d1 *c, uint32_t col, sk_int g) {
    sk_check_limit(c->C, "divisors");
    uint32_t oldC = c->C;
    SK_RESERVE(c->dv, c->C, c->Ccap, 16);
    memmove(c->dv + col + 1, c->dv + col, sizeof(sk_int) * (oldC - col));
    c->dv[col] = g;
    c->C++;
    CNT(ref_div_inserted, 1);
    NOTE_DIV(g);
    if ((uint64_t)c->C > 64 * (uint64_t)c->W) { /* one more word of stride; copy to the new stride */
        uint32_t W = c->W, nW = W + 1;
        uint64_t *n = sk_xcalloc((size_t)(c->inc_cap ? c->inc_cap : 1) * nW, sizeof(uint64_t));
        for (uint32_t k = 0; k < c->D; k++) memcpy(n + (size_t)k * nW, c->inc + (size_t)k * W, sizeof(uint64_t) * W);
        free(c->inc);
        c->inc = n;
        c->W = nW;
        CNT(ref_stride_growths, 1);
    }
    if (col < oldC) {
        for (uint32_t k = 0; k < c->D; k++) shift_row(row_of(c, k), c->W, col);
        CNT(ref_words_shifted, (uint64_t)c->D * (c->W - (col >> 6)));
    }
}

/* Set bit (k, col) and insert residual s at its position. */
static void add_bit(sk_d1 *c, uint32_t k, uint32_t col, sk_int s) {
    row_of(c, k)[col >> 6] |= UINT64_C(1) << (col & 63);
    uint32_t pos = res_pos(c, k, col);
    d1_res *r = &c->res[k];
    SK_RESERVE(r->r, r->len, r->cap, 2);
    memmove(r->r + pos + 1, r->r + pos, sizeof(sk_int) * (r->len - pos));
    r->r[pos] = s;
    r->len++;
    NOTE_ROW(k);
}

/* compare(i, j) (spec §7.3): the column of gcd(d_i, d_j) and the residuals at it, recording the gcd if the pair
 * has not been compared.  i != j. */
static sk_status compare(sk_d1 *c, uint32_t i, uint32_t j, uint32_t *col_out, sk_int *ri_out, sk_int *rj_out) {
    uint32_t col = search(c, i, j);
    sk_int ri = residual(c, i, col);
    sk_int rj = residual(c, j, col);
    if (pair_get(c, i, j)) { /* the gcd, by spec §7.6 */
        CNT(ev_pair_hits, 1);
        *col_out = col;
        *ri_out = ri;
        *rj_out = rj;
        return SK_OK;
    }
    CNT(ev_compares, 1);
    sk_int cd = c->dv[col];
    sk_int gp = sk_gcd(ri, rj);
    if (gp == 1) { /* g is c (D1-b) */
        pair_set(c, i, j);
        *col_out = col;
        *ri_out = ri;
        *rj_out = rj;
        return SK_OK;
    }
    sk_int g;
    if (sk_mul(cd, gp, &g)) OVF("compare: g = c*g'");
    /* find g in the divisors by binary search (comparisons only) */
    uint32_t lo = 0, hi = c->C;
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2;
        if (c->dv[mid] < g) lo = mid + 1;
        else hi = mid;
    }
    uint32_t colg = lo;
    if (!(colg < c->C && c->dv[colg] == g)) insert_divisor(c, colg, g);
    sk_int si = sk_divexact(ri, gp), sj = sk_divexact(rj, gp); /* the residuals at col_g (D1-2) */
    if (!bit_get(c, i, colg)) add_bit(c, i, colg, si);
    if (!bit_get(c, j, colg)) add_bit(c, j, colg, sj);
    pair_set(c, i, j);
    *col_out = colg;
    *ri_out = si;
    *rj_out = sj;
    return SK_OK;
}

/* ----------------------------------------------------------------------------------------- store */

/* A new denominator k with value e: an island (spec §7.4).  Incidence row with column 0 only, residual row [e],
 * pair-table column k of k zero bits.  No gcd, division or multiplication. */
static void new_row(sk_d1 *c, uint32_t k, sk_int e) {
    if (k >= c->inc_cap) {
        c->inc_cap = c->inc_cap ? 2 * c->inc_cap : 16;
        c->inc = sk_xrealloc(c->inc, sizeof(uint64_t) * (size_t)c->inc_cap * c->W);
    }
    uint64_t *row = row_of(c, k);
    memset(row, 0, sizeof(uint64_t) * c->W);
    row[0] = 1;
    SK_RESERVE(c->res, k, c->res_cap, 16);
    c->res[k].r = sk_xmalloc(sizeof(sk_int));
    c->res[k].r[0] = e;
    c->res[k].len = 1;
    c->res[k].cap = 1;
    uint64_t bits = (uint64_t)(k + 1) * k / 2, words = (bits + 63) / 64;
    if (words > c->pair_cap) {
        uint64_t nc = c->pair_cap ? 2 * c->pair_cap : 16;
        while (nc < words) nc *= 2;
        c->pair = sk_xrealloc(c->pair, sizeof(uint64_t) * nc);
        memset(c->pair + c->pair_cap, 0, sizeof(uint64_t) * (nc - c->pair_cap));
        c->pair_cap = nc;
    }
}

/* store(n over e) (spec §5). */
static sk_status store(sk_d1 *c, sk_int n, sk_int e, sk_handle *out) {
    CNT_SET(c->cnt, c->ctx.op, SK_STORE);
    if (n == 0) e = 1;
    uint32_t empty;
    uint32_t k = sk_lookup_find(&c->look, c->den, e, &empty);
    if (k == SK_NONE) {
        sk_check_limit(c->D, "denominators");
        SK_RESERVE(c->den, c->D, c->Dcap, 16);
        k = c->D;
        c->den[k] = e;
        sk_lookup_insert(&c->look, c->den, k, empty);
        new_row(c, k, e);
        c->D++;
        CNT(ev_new_dens, 1);
    }
    sk_check_limit(c->N, "stored values");
    if (c->N >= c->Ncap) {
        c->Ncap = c->Ncap ? (c->Ncap > UINT32_MAX / 2 ? UINT32_MAX : 2 * c->Ncap) : 16;
        c->num = sk_xrealloc(c->num, sizeof(sk_int) * c->Ncap);
        c->nk = sk_xrealloc(c->nk, sizeof(uint32_t) * c->Ncap);
    }
    c->num[c->N] = n;
    c->nk[c->N] = k;
    *out = c->N++;
    return SK_OK;
}

/* ----------------------------------------------------------------------------------------- operations */

static sk_status begin(sk_d1 *c, sk_phase op, sk_handle x, sk_handle y) {
    if (x >= c->N || y >= c->N) return SK_EINVAL;
    c->ctx.op = op;
    c->ctx.x = x;
    c->ctx.y = y;
    CNT_SET(c->cnt, op, SK_CORE);
    return SK_OK;
}

sk_status sk_d1_enter(sk_d1 *c, sk_int m, sk_int d, sk_handle *out) {
    if (!sk_valid_input(m) || !sk_valid_input(d)) return SK_EINVAL;
    if (d == 0) return SK_EDIVZERO;
    c->ctx.op = SK_PH_ENTRY;
    c->ctx.x = c->ctx.y = SK_NONE;
    CNT_SET(c->cnt, SK_PH_ENTRY, SK_CORE);
    if (d < 0) {
        m = -m;
        d = -d;
    }
    sk_int g = sk_gcd(m, d);
    return store(c, sk_divexact(m, g), sk_divexact(d, g), out);
}

/* spec §7.5 add; subtraction is addition of the negated second operand */
static sk_status addsub(sk_d1 *c, sk_phase op, sk_handle x, sk_handle y, sk_handle *out) {
    sk_status st = begin(c, op, x, y);
    if (st) return st;
    sk_int mx = c->num[x], my = c->num[y];
    uint32_t i = c->nk[x], j = c->nk[y];
    if (op == SK_PH_SUB) my = -my;
    sk_int t, a, b, e;
    if (i == j) {
        CNT(ev_fast_paths, 1);
        if (sk_add(mx, my, &t)) OVF("add: t = m_x + m_y");
        sk_int h = sk_gcd(t, c->den[i]);
        return store(c, sk_divexact(t, h), sk_divexact(c->den[i], h), out);
    }
    uint32_t col;
    sk_int rx, ry;
    st = compare(c, i, j, &col, &rx, &ry);
    if (st) return st;
    sk_int g = c->dv[col];
    if (sk_mul(mx, ry, &a)) OVF("add: m_x*r_y");
    if (sk_mul(my, rx, &b)) OVF("add: m_y*r_x");
    if (sk_add(a, b, &t)) OVF("add: t = m_x*r_y + m_y*r_x");
    if (g == 1) { /* h is 1: no division by it (change log CL-1) */
        if (sk_mul(rx, c->den[j], &e)) OVF("add: r_x*(d_j/h)");
        return store(c, t, e, out);
    }
    sk_int h = sk_gcd(t, g);
    sk_int n = sk_divexact(t, h);
    if (sk_mul(rx, sk_divexact(c->den[j], h), &e)) OVF("add: r_x*(d_j/h)");
    return store(c, n, e, out);
}

sk_status sk_d1_add(sk_d1 *c, sk_handle x, sk_handle y, sk_handle *out) { return addsub(c, SK_PH_ADD, x, y, out); }
sk_status sk_d1_sub(sk_d1 *c, sk_handle x, sk_handle y, sk_handle *out) { return addsub(c, SK_PH_SUB, x, y, out); }

sk_status sk_d1_mul(sk_d1 *c, sk_handle x, sk_handle y, sk_handle *out) {
    sk_status st = begin(c, SK_PH_MUL, x, y);
    if (st) return st;
    sk_int mx = c->num[x], my = c->num[y];
    uint32_t i = c->nk[x], j = c->nk[y];
    sk_int n, e;
    if (i == j) {
        CNT(ev_fast_paths, 1);
        if (sk_mul(mx, my, &n)) OVF("multiply: m_x*m_y");
        if (sk_mul(c->den[i], c->den[i], &e)) OVF("multiply: d_i*d_i");
        return store(c, n, e, out);
    }
    uint32_t col = search(c, i, j); /* no compare, no pair bit */
    sk_int rx = residual(c, i, col);
    sk_int ry = residual(c, j, col);
    sk_int g1 = sk_gcd(mx, ry);
    sk_int g2 = sk_gcd(my, rx);
    if (sk_mul(sk_divexact(mx, g1), sk_divexact(my, g2), &n)) OVF("multiply: (m_x/g_1)*(m_y/g_2)");
    if (sk_mul(sk_divexact(c->den[i], g2), sk_divexact(c->den[j], g1), &e)) OVF("multiply: (d_i/g_2)*(d_j/g_1)");
    return store(c, n, e, out);
}

sk_status sk_d1_div(sk_d1 *c, sk_handle x, sk_handle y, sk_handle *out) {
    if (x >= c->N || y >= c->N) return SK_EINVAL;
    if (c->num[y] == 0) return SK_EDIVZERO;
    sk_status st = begin(c, SK_PH_DIV, x, y);
    if (st) return st;
    sk_int mx = c->num[x], my = c->num[y];
    uint32_t i = c->nk[x], j = c->nk[y];
    bool neg = my < 0; /* s and t = |m_y|: not counted (counting rule 1) */
    sk_int t = neg ? -my : my;
    sk_int g1 = sk_gcd(mx, t);
    sk_int n, e;
    if (i == j) {
        CNT(ev_fast_paths, 1);
        n = sk_divexact(mx, g1);
        if (neg) n = -n;
        return store(c, n, sk_divexact(t, g1), out);
    }
    uint32_t col;
    sk_int rx, ry;
    st = compare(c, i, j, &col, &rx, &ry); /* r_x = d_i/gcd, r_y = d_j/gcd */
    if (st) return st;
    sk_int a = sk_divexact(mx, g1);
    if (neg) a = -a;
    if (sk_mul(a, ry, &n)) OVF("divide: s*(m_x/g_1)*r_y");
    if (sk_mul(rx, sk_divexact(t, g1), &e)) OVF("divide: r_x*(t/g_1)");
    return store(c, n, e, out);
}

sk_status sk_d1_read(const sk_d1 *c, sk_handle x, sk_int *m, sk_int *d) {
    if (x >= c->N) return SK_EINVAL;
    *m = c->num[x];
    *d = c->den[c->nk[x]];
    return SK_OK;
}

uint32_t sk_d1_size(const sk_d1 *c) { return c->N; }

void sk_d1_memory(const sk_d1 *c, sk_memory *r) {
    memset(r, 0, sizeof *r);
    uint64_t w = sizeof(sk_int);
    sk_memory_add(r, "numerators", (uint64_t)c->N * (w + 4), (uint64_t)c->Ncap * (w + 4));
    sk_memory_add(r, "denominators", (uint64_t)c->D * w, (uint64_t)c->Dcap * w);
    sk_memory_add(r, "lookup", 4 * (uint64_t)c->look.S, 4 * (uint64_t)c->look.S);
    sk_memory_add(r, "divisors", (uint64_t)c->C * w, (uint64_t)c->Ccap * w);
    uint64_t bits = (uint64_t)c->D * (c->D ? c->D - 1 : 0) / 2;
    sk_memory_add(r, "pair table", 8 * ((bits + 63) / 64), 8 * c->pair_cap);
    sk_memory_add(r, "incidence bits", 8 * (uint64_t)c->D * c->W, 8 * (uint64_t)c->inc_cap * c->W);
    uint64_t used = 0, alloc = 0;
    for (uint32_t k = 0; k < c->D; k++) {
        used += c->res[k].len;
        alloc += c->res[k].cap;
    }
    sk_memory_add(r, "residual rows", used * w + (uint64_t)c->D * sizeof(d1_res),
                  alloc * w + (uint64_t)c->res_cap * sizeof(d1_res));
    sk_memory_finish(r, c->N);
}

sk_status sk_d1_counters(const sk_d1 *c, sk_counters *r) {
#ifdef SK_COUNT
    *r = c->cnt;
    return SK_OK;
#else
    (void)c;
    memset(r, 0, sizeof *r);
    return SK_ENOTBUILT;
#endif
}

sk_status sk_d1_overflow(const sk_d1 *c, sk_overflow *r) {
    *r = c->ovf;
    return c->ovf.count ? SK_OK : SK_EINVAL;
}

/* ----------------------------------------------------------------------------------------- audits (spec §13.2) */

#ifdef SK_AUDIT
static const char *check_values(const sk_d1 *c, uint32_t from) {
    for (uint32_t x = from; x < c->N; x++) {
        if (c->nk[x] >= c->D) return "value: denominator index not below D";
        sk_int m = c->num[x], d = c->den[c->nk[x]];
        if (d < 1) return "value: denominator not positive";
        if (m == SK_INT_MIN) return "value: numerator is the most negative integer";
        if (m == 0 && d != 1) return "value: zero not stored as 0/1";
        if (sk_gcd(m, d) != 1) return "value: not in lowest terms";
    }
    return NULL;
}

static const char *check_row(const sk_d1 *c, uint32_t k) {
    const uint64_t *row = row_of(c, k);
    if (!(row[0] & 1)) return "row: bit 0 not set";
    uint64_t pop = 0;
    for (uint32_t w = 0; w < c->W; w++) {
        uint64_t x = row[w];
        uint64_t lo = 64 * (uint64_t)w;
        if (lo + 64 > c->C) { /* bits at or above column C must be clear */
            uint32_t keep = c->C > lo ? (uint32_t)(c->C - lo) : 0;
            uint64_t mask = keep >= 64 ? ~UINT64_C(0) : ((UINT64_C(1) << keep) - 1);
            if (x & ~mask) return "row: a bit at or above column C is set";
        }
        pop += (uint64_t)__builtin_popcountll(x);
    }
    if (pop != c->res[k].len) return "row: residual row length differs from the row's popcount";
    uint32_t pos = 0;
    for (uint32_t w = 0; w < c->W; w++) {
        uint64_t x = row[w];
        while (x) {
            uint32_t col = 64 * w + (uint32_t)__builtin_ctzll(x);
            x &= x - 1;
            sk_int p;
            if (sk_mul_raw(c->dv[col], c->res[k].r[pos], &p) || p != c->den[k])
                return "row: divisor times residual is not the denominator";
            pos++;
        }
    }
    return NULL;
}

static const char *check_pair(const sk_d1 *c, uint32_t i, uint32_t j) {
    if (c->dv[search(c, i, j)] != sk_gcd(c->den[i], c->den[j]))
        return "pair: the divisor at search(i, j) is not gcd(d_i, d_j)";
    return NULL;
}

static const char *check_divisors(const sk_d1 *c) {
    if (c->C < 1 || c->dv[0] != 1) return "divisors: divisor 0 is not 1";
    for (uint32_t k = 1; k < c->C; k++)
        if (c->dv[k] <= c->dv[k - 1]) return "divisors: not strictly increasing";
    if ((uint64_t)c->C > 64 * (uint64_t)c->W) return "incidence: C exceeds 64W";
    return NULL;
}

static const char *full(const sk_d1 *c) {
    const char *f;
    if ((f = check_values(c, 0))) return f;
    if ((f = sk_lookup_audit_full(&c->look, c->den, c->D))) return f;
    if ((f = check_divisors(c))) return f;
    for (uint32_t k = 0; k < c->D; k++)
        if ((f = check_row(c, k))) return f;
    /* every set pair bit: walk the words, skipping zero words */
    uint64_t bits = (uint64_t)c->D * (c->D ? c->D - 1 : 0) / 2, words = (bits + 63) / 64;
    uint32_t j = 1;
    for (uint64_t w = 0; w < words; w++) {
        uint64_t x = c->pair[w];
        while (x) {
            uint64_t b = 64 * w + (uint64_t)__builtin_ctzll(x);
            x &= x - 1;
            while ((uint64_t)(j + 1) * j / 2 <= b) j++; /* column j holds bits j(j-1)/2 .. j(j+1)/2 - 1 */
            uint32_t i = (uint32_t)(b - (uint64_t)j * (j - 1) / 2);
            if ((f = check_pair(c, i, j))) return f;
        }
    }
    return NULL;
}

static const char *changed(const sk_d1 *c) {
    const char *f;
    if ((f = check_values(c, c->mark_N))) return f;
    for (uint32_t k = c->mark_D; k < c->D; k++)
        if (sk_lookup_find(&c->look, c->den, c->den[k], NULL) != k)
            return "lookup: a new denominator is not found at its value";
    if ((f = sk_lookup_audit_count(&c->look, c->D))) return f;
    for (uint32_t t = 0; t < c->n_chg_div; t++) {
        sk_int g = c->chg_div[t];
        uint32_t lo = 0, hi = c->C;
        while (lo < hi) {
            uint32_t mid = lo + (hi - lo) / 2;
            if (c->dv[mid] < g) lo = mid + 1;
            else hi = mid;
        }
        if (lo >= c->C || c->dv[lo] != g) return "divisors: an inserted divisor is missing";
        if (lo == 0 || c->dv[lo - 1] >= g) return "divisors: an inserted divisor is not above its lower neighbour";
        if (lo + 1 < c->C && c->dv[lo + 1] <= g) return "divisors: an inserted divisor is not below its upper neighbour";
    }
    if (c->dv[0] != 1) return "divisors: divisor 0 is not 1";
    if ((uint64_t)c->C > 64 * (uint64_t)c->W) return "incidence: C exceeds 64W";
    for (uint32_t k = c->mark_D; k < c->D; k++)
        if ((f = check_row(c, k))) return f;
    for (uint32_t t = 0; t < c->n_chg_row; t++)
        if ((f = check_row(c, c->chg_row[t]))) return f;
    for (uint32_t t = 0; t + 1 < c->n_chg_pair; t += 2)
        if ((f = check_pair(c, c->chg_pair[t], c->chg_pair[t + 1]))) return f;
    return NULL;
}

static sk_status finish_audit(sk_d1 *c, const char *fail, const char **why) {
    if (fail) {
        snprintf(c->msg, sizeof c->msg, "d1: %s", fail);
        if (why) *why = c->msg;
        return SK_EAUDIT;
    }
    c->mark_N = c->N;
    c->mark_D = c->D;
    c->n_chg_div = 0;
    for (uint32_t t = 0; t < c->n_chg_row; t++) c->row_dirty[c->chg_row[t]] = 0;
    c->n_chg_row = 0;
    c->n_chg_pair = 0;
    if (why) *why = NULL;
    return SK_OK;
}
#endif

sk_status sk_d1_audit(sk_d1 *c, const char **why) {
#ifdef SK_AUDIT
    sk_count *saved = sk_cur;
    sk_cur = &sk_sink;
    const char *f = full(c);
    sk_cur = saved;
    return finish_audit(c, f, why);
#else
    (void)c;
    if (why) *why = "audits not built";
    return SK_ENOTBUILT;
#endif
}

sk_status sk_d1_audit_scheduled(sk_d1 *c, const char **why) { return sk_d1_audit(c, why); }

sk_status sk_d1_audit_changed(sk_d1 *c, const char **why) {
#ifdef SK_AUDIT
    sk_count *saved = sk_cur;
    sk_cur = &sk_sink;
    const char *f = changed(c);
    sk_cur = saved;
    return finish_audit(c, f, why);
#else
    (void)c;
    if (why) *why = "audits not built";
    return SK_ENOTBUILT;
#endif
}

/* ----------------------------------------------------------------------------------------- test hooks */

/* Read-only views of the structures, for the unit tests (declared in tests/introspect.h, not in the public API). */
uint32_t sk_d1_t_D(const sk_d1 *c) { return c->D; }
uint32_t sk_d1_t_C(const sk_d1 *c) { return c->C; }
sk_int sk_d1_t_divisor(const sk_d1 *c, uint32_t col) { return c->dv[col]; }
sk_int sk_d1_t_den(const sk_d1 *c, uint32_t k) { return c->den[k]; }
uint32_t sk_d1_t_den_index(const sk_d1 *c, sk_handle x) { return c->nk[x]; }
int sk_d1_t_bit(const sk_d1 *c, uint32_t k, uint32_t col) { return col < c->C && bit_get(c, k, col); }
uint32_t sk_d1_t_res_len(const sk_d1 *c, uint32_t k) { return c->res[k].len; }
sk_int sk_d1_t_res(const sk_d1 *c, uint32_t k, uint32_t pos) { return c->res[k].r[pos]; }
int sk_d1_t_pair(const sk_d1 *c, uint32_t i, uint32_t j) {
    if (i == j) return 0;
    if (i > j) {
        uint32_t t = i;
        i = j;
        j = t;
    }
    uint64_t b = pair_bit(i, j);
    return (c->pair[b >> 6] >> (b & 63)) & 1;
}
uint32_t sk_d1_t_W(const sk_d1 *c) { return c->W; }
uint32_t sk_d1_t_S(const sk_d1 *c) { return c->look.S; }
#ifdef SK_AUDIT
/* test build only: break an invariant, so the tests can check that the audits find it */
void sk_d1_t_corrupt_residual(sk_d1 *c, uint32_t k, uint32_t pos, sk_int v) { c->res[k].r[pos] = v; }
void sk_d1_t_corrupt_pair(sk_d1 *c, uint32_t i, uint32_t j) { pair_set(c, i, j); }
#endif
