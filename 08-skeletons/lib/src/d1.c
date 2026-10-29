/* d1.c: Design 1, common divisors, in the note's implementation form (spec §7, as amended Oct 29, 2026).  Six
 * structures: numerators, denominators, the lookup (spec §5), divisors, the pair set and sparse incidence rows. */
#include "sk_d1.h"

#include "sk_internal.h"

/* One sparse incidence row (spec §7.1): len entries (divisor id, residual), sorted by divisor value; entry 0 is
 * divisor 1 (id 0) with residual d_k.  One block holds cap residuals, then cap ids, so an entry is w/8 + 4 bytes
 * with no padding.  This header is the per-row header of spec §12.2. */
typedef struct d1_row {
    sk_int *r;
    uint32_t len, cap;
} d1_row;

static inline uint32_t *ids_of(const d1_row *row) { return (uint32_t *)(row->r + row->cap); }

struct sk_d1 {
    /* numerators: the numerator and its denominator index (two parallel arrays, so an entry is w/8 + 4 bytes) */
    sk_int *num;
    uint32_t *nk;
    uint32_t N, Ncap;
    /* denominators */
    sk_int *den;
    uint32_t D, Dcap;
    sk_lookup look;
    /* divisors: dv[id] is the divisor with stable id `id`, append-only, id 0 is 1; ord lists the ids sorted by
     * value, strictly increasing, for the binary search (spec §7.1, D1-5) */
    sk_int *dv;
    uint32_t *ord;
    uint32_t C, Ccap;
    /* pair set (spec §7.1): the compared pairs only, a hash set of keys (j << 32) | i for i < j, linear probing;
     * a slot holding 0 is empty (j >= 1, so no key is 0).  PS slots, a power of two, 0 until the first insertion,
     * at most half full; PN keys. */
    uint64_t *pair;
    uint32_t PS, PN, log2PS;
    /* incidence rows: one sparse row per denominator */
    d1_row *row;
    uint32_t row_cap;

    sk_opctx ctx;
    sk_overflow ovf;
#ifdef SK_COUNT
    sk_counters cnt;
#endif
#ifdef SK_AUDIT
    /* the change list (spec §13.2): read only by the audits */
    uint32_t mark_N, mark_D;
    uint32_t *chg_div; /* ids of the divisors inserted since the last audit */
    uint32_t n_chg_div, cap_chg_div;
    uint32_t *chg_row; /* rows that gained an entry */
    uint32_t n_chg_row, cap_chg_row;
    uint8_t *row_dirty;
    uint32_t row_dirty_cap;
    uint32_t *chg_pair; /* pairs inserted, as (i, j) */
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
    c->ord = sk_xmalloc(sizeof(uint32_t) * c->Ccap);
    c->dv[0] = 1;
    c->ord[0] = 0;
    c->C = 1;
    return c;
}

void sk_d1_destroy(sk_d1 *c) {
    if (!c) return;
    free(c->num);
    free(c->nk);
    free(c->den);
    sk_lookup_free(&c->look);
    free(c->dv);
    free(c->ord);
    free(c->pair);
    for (uint32_t k = 0; k < c->D; k++) free(c->row[k].r);
    free(c->row);
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
        uint32_t nc = c->row_cap;
        c->row_dirty = sk_xrealloc(c->row_dirty, nc);
        memset(c->row_dirty + c->row_dirty_cap, 0, nc - c->row_dirty_cap);
        c->row_dirty_cap = nc;
    }
    if (c->row_dirty[k]) return;
    c->row_dirty[k] = 1;
    SK_RESERVE(c->chg_row, c->n_chg_row, c->cap_chg_row, 16);
    c->chg_row[c->n_chg_row++] = k;
}
static void note_div(sk_d1 *c, uint32_t id) {
    SK_RESERVE(c->chg_div, c->n_chg_div, c->cap_chg_div, 16);
    c->chg_div[c->n_chg_div++] = id;
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

/* ----------------------------------------------------------------------------------------- pair set */

#define SK_PAIR_PHI UINT64_C(0x9E3779B97F4A7C15)

static inline uint64_t pair_key(uint32_t i, uint32_t j) { /* index arithmetic; orders the pair */
    if (i > j) {
        uint32_t t = i;
        i = j;
        j = t;
    }
    return ((uint64_t)j << 32) | i;
}

/* The starting slot of a key: the lookup's hash (spec §5 item 2), on the 64-bit key. */
static inline uint32_t pair_slot(const sk_d1 *c, uint64_t key) {
    return (uint32_t)((key * SK_PAIR_PHI) >> (64 - c->log2PS));
}

/* The slot holding key, or the empty slot where the search stops.  Counted as a lookup (rule 4): one search per
 * hash, one probe per slot read.  Needs PS > 0. */
static uint32_t pair_find(const sk_d1 *c, uint64_t key) {
    CNT(lookup_search, 1);
    uint32_t s = pair_slot(c, key), mask = c->PS - 1;
    for (;;) {
        CNT(lookup_probe, 1);
        if (c->pair[s] == key || c->pair[s] == 0) return s;
        s = (s + 1) & mask;
    }
}

/* Is the pair (i, j) in the set ("compared")?  One pair read, plus the set's search and probes. */
static inline bool pair_get(const sk_d1 *c, uint32_t i, uint32_t j) {
    CNT(pair_read, 1);
    if (c->PS == 0) return false; /* the empty set has no table to search */
    uint64_t key = pair_key(i, j);
    return c->pair[pair_find(c, key)] == key;
}

/* Insert (i, j), exactly where the dense table set its bit.  The caller, compare, has just found the pair absent,
 * so there is no membership search here (CL-3, DEC-47); a duplicate would break the count, which the audit checks.
 * Before an insertion would make the set more than half full, PS doubles and every key is placed again, in the
 * order of the old slots. */
static void pair_set(sk_d1 *c, uint32_t i, uint32_t j) {
    uint64_t key = pair_key(i, j);
    if (2 * ((uint64_t)c->PN + 1) > c->PS) {
        uint64_t *old = c->pair;
        uint32_t oldS = c->PS;
        c->PS = oldS ? 2 * oldS : 16;
        if (c->PS == 0) sk_die("d1: pair set too large");
        c->log2PS = (uint32_t)__builtin_ctz(c->PS);
        c->pair = sk_xcalloc(c->PS, sizeof(uint64_t));
        for (uint32_t t = 0; t < oldS; t++)
            if (old[t]) c->pair[pair_find(c, old[t])] = old[t];
        free(old);
    }
    c->pair[pair_find(c, key)] = key;
    c->PN++;
    NOTE_PAIR((uint32_t)(key & 0xffffffffu), (uint32_t)(key >> 32));
}

/* ----------------------------------------------------------------------------------------- divisors, incidence rows */

/* The position in ord of the first divisor not below g: a binary search, comparisons only (not counted). */
static uint32_t ord_find(const sk_d1 *c, sk_int g) {
    uint32_t lo = 0, hi = c->C;
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2;
        if (c->dv[c->ord[mid]] < g) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

/* search(i, j) (spec §7.2): the entries of the largest divisor in both rows, by a merge of the two rows from their
 * largest divisor down, stopping at the first divisor common to both.  One scan per merge step: one pair of entries
 * compared (scan_and_words).  Divisor 1 is entry 0 of every row, so the merge always stops. */
static void search(const sk_d1 *c, uint32_t i, uint32_t j, uint32_t *pi, uint32_t *pj) {
    const d1_row *a = &c->row[i], *b = &c->row[j];
    const uint32_t *ia = ids_of(a), *ib = ids_of(b);
    uint32_t x = a->len - 1, y = b->len - 1;
    for (;;) {
        CNT(scan_and_words, 1);
        if (ia[x] == ib[y]) {
            *pi = x;
            *pj = y;
            return;
        }
        if (c->dv[ia[x]] > c->dv[ib[y]]) {
            if (x == 0) break;
            x--;
        } else {
            if (y == 0) break;
            y--;
        }
    }
    sk_die("d1: search found no common divisor (divisor 1 is not in a row)");
}

/* Append divisor g with the next id, C, and insert that id into ord at position pos (spec §7.3).  No row changes and
 * nothing is renumbered: ids are stable. */
static uint32_t insert_divisor(sk_d1 *c, uint32_t pos, sk_int g) {
    sk_check_limit(c->C, "divisors");
    if (c->C >= c->Ccap) {
        c->Ccap = c->Ccap > UINT32_MAX / 2 ? UINT32_MAX : 2 * c->Ccap;
        c->dv = sk_xrealloc(c->dv, sizeof(sk_int) * c->Ccap);
        c->ord = sk_xrealloc(c->ord, sizeof(uint32_t) * c->Ccap);
    }
    uint32_t id = c->C;
    c->dv[id] = g;
    memmove(c->ord + pos + 1, c->ord + pos, sizeof(uint32_t) * (c->C - pos));
    c->ord[pos] = id;
    c->C++;
    CNT(ref_div_inserted, 1);
    NOTE_DIV(id);
    return id;
}

/* Record that divisor id (value g) divides d_k, with residual s, unless row k holds it already.  g is above the
 * divisor at entry p of the row, so its place is found by a binary search of the entries above p; one scan per
 * entry read (scan_popcount_words). */
static void place(sk_d1 *c, uint32_t k, uint32_t p, uint32_t id, sk_int g, sk_int s) {
    d1_row *row = &c->row[k];
    uint32_t *ids = ids_of(row);
    uint32_t lo = p + 1, hi = row->len;
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2;
        CNT(scan_popcount_words, 1);
        if (c->dv[ids[mid]] < g) lo = mid + 1;
        else hi = mid;
    }
    if (lo < row->len && ids[lo] == id) return;
    if (row->len == row->cap) { /* a new block of twice the capacity: residuals, then ids */
        uint32_t ncap = row->cap > UINT32_MAX / 2 ? UINT32_MAX : 2 * row->cap;
        sk_int *nr = sk_xmalloc((sizeof(sk_int) + sizeof(uint32_t)) * (size_t)ncap);
        uint32_t *nid = (uint32_t *)(nr + ncap);
        memcpy(nr, row->r, sizeof(sk_int) * row->len);
        memcpy(nid, ids, sizeof(uint32_t) * row->len);
        free(row->r);
        row->r = nr;
        row->cap = ncap;
        ids = nid;
    }
    memmove(row->r + lo + 1, row->r + lo, sizeof(sk_int) * (row->len - lo));
    memmove(ids + lo + 1, ids + lo, sizeof(uint32_t) * (row->len - lo));
    row->r[lo] = s;
    ids[lo] = id;
    row->len++;
    NOTE_ROW(k);
}

/* compare(i, j) (spec §7.3): the id of gcd(d_i, d_j) and the residuals at it, recording the gcd if the pair has not
 * been compared.  i != j. */
static sk_status compare(sk_d1 *c, uint32_t i, uint32_t j, uint32_t *id_out, sk_int *ri_out, sk_int *rj_out) {
    uint32_t pi, pj;
    search(c, i, j, &pi, &pj);
    uint32_t id = ids_of(&c->row[i])[pi];
    sk_int ri = c->row[i].r[pi];
    sk_int rj = c->row[j].r[pj];
    if (pair_get(c, i, j)) { /* the gcd, by spec §7.6 */
        CNT(ev_pair_hits, 1);
        *id_out = id;
        *ri_out = ri;
        *rj_out = rj;
        return SK_OK;
    }
    CNT(ev_compares, 1);
    sk_int cd = c->dv[id];
    sk_int gp = sk_gcd(ri, rj);
    if (gp == 1) { /* g is c (D1-b) */
        pair_set(c, i, j);
        *id_out = id;
        *ri_out = ri;
        *rj_out = rj;
        return SK_OK;
    }
    sk_int g;
    if (sk_mul(cd, gp, &g)) OVF("compare: g = c*g'");
    /* find g in the divisors by binary search (comparisons only) */
    uint32_t pos = ord_find(c, g);
    uint32_t idg = (pos < c->C && c->dv[c->ord[pos]] == g) ? c->ord[pos] : insert_divisor(c, pos, g);
    sk_int si = sk_divexact(ri, gp), sj = sk_divexact(rj, gp); /* the residuals at g (D1-2) */
    place(c, i, pi, idg, g, si);
    place(c, j, pj, idg, g, sj);
    pair_set(c, i, j);
    *id_out = idg;
    *ri_out = si;
    *rj_out = sj;
    return SK_OK;
}

/* ----------------------------------------------------------------------------------------- store */

/* A new denominator k with value e: an island (spec §7.4).  A row with divisor 1 only, residual e; nothing in the
 * pair set.  No gcd, division or multiplication. */
static void new_row(sk_d1 *c, uint32_t k, sk_int e) {
    SK_RESERVE(c->row, k, c->row_cap, 16);
    d1_row *row = &c->row[k];
    row->cap = 1;
    row->len = 1;
    row->r = sk_xmalloc(sizeof(sk_int) + sizeof(uint32_t));
    row->r[0] = e;
    ids_of(row)[0] = 0;
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
    uint32_t id;
    sk_int rx, ry;
    st = compare(c, i, j, &id, &rx, &ry);
    if (st) return st;
    sk_int g = c->dv[id];
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
    uint32_t pi, pj;
    search(c, i, j, &pi, &pj); /* no compare, no pair inserted */
    sk_int rx = c->row[i].r[pi];
    sk_int ry = c->row[j].r[pj];
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
    uint32_t id;
    sk_int rx, ry;
    st = compare(c, i, j, &id, &rx, &ry); /* r_x = d_i/gcd, r_y = d_j/gcd */
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
    sk_memory_add(r, "divisors", (uint64_t)c->C * (w + 4), (uint64_t)c->Ccap * (w + 4)); /* values and ord */
    sk_memory_add(r, "pair set", 8 * (uint64_t)c->PS, 8 * (uint64_t)c->PS); /* like the lookup: the whole table */
    uint64_t used = 0, alloc = 0; /* entries of the incidence rows, each a residual and a 32-bit divisor id */
    for (uint32_t k = 0; k < c->D; k++) {
        used += c->row[k].len;
        alloc += c->row[k].cap;
    }
    sk_memory_add(r, "incidence rows", used * (w + 4) + (uint64_t)c->D * sizeof(d1_row),
                  alloc * (w + 4) + (uint64_t)c->row_cap * sizeof(d1_row));
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

/* Row k: entry 0 is divisor 1 with residual d_k; every id is below C; the entries are strictly increasing by divisor
 * value (so no divisor twice); every divisor times its residual is d_k. */
static const char *check_row(const sk_d1 *c, uint32_t k) {
    const d1_row *row = &c->row[k];
    const uint32_t *ids = ids_of(row);
    if (row->len < 1 || row->len > row->cap) return "row: length not in 1..capacity";
    if (ids[0] != 0) return "row: entry 0 is not divisor 1";
    for (uint32_t t = 0; t < row->len; t++) {
        if (ids[t] >= c->C) return "row: a divisor id is not below C";
        if (t && c->dv[ids[t]] <= c->dv[ids[t - 1]]) return "row: entries not strictly increasing by divisor value";
        sk_int p;
        if (sk_mul_raw(c->dv[ids[t]], row->r[t], &p) || p != c->den[k])
            return "row: divisor times residual is not the denominator";
    }
    return NULL;
}

static const char *check_pair(const sk_d1 *c, uint32_t i, uint32_t j) {
    uint32_t pi, pj;
    search(c, i, j, &pi, &pj);
    if (c->dv[ids_of(&c->row[i])[pi]] != sk_gcd(c->den[i], c->den[j]))
        return "pair: the divisor found by search(i, j) is not gcd(d_i, d_j)";
    return NULL;
}

int sk_d1_t_pair(const sk_d1 *c, uint32_t i, uint32_t j);

/* The set's shape: a power-of-two table at most half full, PN keys, each i < j < D, each found at its value. */
static const char *check_pair_set(const sk_d1 *c) {
    if (c->PS && (c->PS & (c->PS - 1))) return "pair set: size not a power of two";
    if (2 * (uint64_t)c->PN > c->PS) return "pair set: more than half full";
    uint32_t n = 0;
    for (uint32_t s = 0; s < c->PS; s++) {
        uint64_t key = c->pair[s];
        if (!key) continue;
        n++;
        uint32_t i = (uint32_t)(key & 0xffffffffu), j = (uint32_t)(key >> 32);
        if (!(i < j && j < c->D)) return "pair set: a key is not a pair i < j < D";
        if (pair_find(c, key) != s) return "pair set: a key is not found at its slot";
    }
    if (n != c->PN) return "pair set: count differs from the keys held";
    return NULL;
}

/* Divisor id 0 is 1 and first in ord; ord lists C ids below C with strictly increasing values, so it is a
 * permutation of the ids and the divisors are distinct. */
static const char *check_divisors(const sk_d1 *c) {
    if (c->C < 1 || c->dv[0] != 1 || c->ord[0] != 0) return "divisors: divisor 0 is not 1, first by value";
    for (uint32_t k = 1; k < c->C; k++) {
        if (c->ord[k] >= c->C) return "divisors: an id in the order is not below C";
        if (c->dv[c->ord[k]] <= c->dv[c->ord[k - 1]]) return "divisors: not strictly increasing in the order";
    }
    return NULL;
}

static const char *full(const sk_d1 *c) {
    const char *f;
    if ((f = check_values(c, 0))) return f;
    if ((f = sk_lookup_audit_full(&c->look, c->den, c->D))) return f;
    if ((f = check_divisors(c))) return f;
    for (uint32_t k = 0; k < c->D; k++)
        if ((f = check_row(c, k))) return f;
    if ((f = check_pair_set(c))) return f;
    /* every pair in the set */
    for (uint32_t s = 0; s < c->PS; s++)
        if (c->pair[s] && (f = check_pair(c, (uint32_t)(c->pair[s] & 0xffffffffu), (uint32_t)(c->pair[s] >> 32))))
            return f;
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
        uint32_t id = c->chg_div[t];
        if (id >= c->C) return "divisors: an inserted id is not below C";
        sk_int g = c->dv[id];
        uint32_t lo = ord_find(c, g);
        if (lo >= c->C || c->ord[lo] != id) return "divisors: an inserted divisor is not found in the order";
        if (lo == 0 || c->dv[c->ord[lo - 1]] >= g) return "divisors: an inserted divisor is not above its lower neighbour";
        if (lo + 1 < c->C && c->dv[c->ord[lo + 1]] <= g)
            return "divisors: an inserted divisor is not below its upper neighbour";
    }
    if (c->dv[0] != 1 || c->ord[0] != 0) return "divisors: divisor 0 is not 1, first by value";
    for (uint32_t k = c->mark_D; k < c->D; k++)
        if ((f = check_row(c, k))) return f;
    for (uint32_t t = 0; t < c->n_chg_row; t++)
        if ((f = check_row(c, c->chg_row[t]))) return f;
    if (2 * (uint64_t)c->PN > c->PS) return "pair set: more than half full";
    for (uint32_t t = 0; t + 1 < c->n_chg_pair; t += 2) {
        if (!sk_d1_t_pair(c, c->chg_pair[t], c->chg_pair[t + 1])) return "pair set: an inserted pair is not found";
        if ((f = check_pair(c, c->chg_pair[t], c->chg_pair[t + 1]))) return f;
    }
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
/* Divisors by rank: rank `col` is the col-th smallest divisor (rank 0 is 1); its stable id is ord[col]. */
sk_int sk_d1_t_divisor(const sk_d1 *c, uint32_t col) { return c->dv[c->ord[col]]; }
uint32_t sk_d1_t_divisor_id(const sk_d1 *c, uint32_t col) { return c->ord[col]; }
sk_int sk_d1_t_den(const sk_d1 *c, uint32_t k) { return c->den[k]; }
uint32_t sk_d1_t_den_index(const sk_d1 *c, sk_handle x) { return c->nk[x]; }
int sk_d1_t_bit(const sk_d1 *c, uint32_t k, uint32_t col) { /* does row k hold the divisor of rank col? */
    if (col >= c->C) return 0;
    const uint32_t *ids = ids_of(&c->row[k]);
    for (uint32_t t = 0; t < c->row[k].len; t++)
        if (ids[t] == c->ord[col]) return 1;
    return 0;
}
uint32_t sk_d1_t_res_len(const sk_d1 *c, uint32_t k) { return c->row[k].len; }
sk_int sk_d1_t_res(const sk_d1 *c, uint32_t k, uint32_t pos) { return c->row[k].r[pos]; }
int sk_d1_t_pair(const sk_d1 *c, uint32_t i, uint32_t j) {
    if (i == j || c->PS == 0) return 0;
#ifdef SK_COUNT
    sk_count *saved = sk_cur; /* a test view: counts nothing */
    sk_cur = &sk_sink;
#endif
    uint64_t key = pair_key(i, j);
    int in = c->pair[pair_find(c, key)] == key;
#ifdef SK_COUNT
    sk_cur = saved;
#endif
    return in;
}
uint32_t sk_d1_t_pairs(const sk_d1 *c) { return c->PN; }
uint32_t sk_d1_t_S(const sk_d1 *c) { return c->look.S; }
#ifdef SK_AUDIT
/* test build only: break an invariant, so the tests can check that the audits find it */
void sk_d1_t_corrupt_residual(sk_d1 *c, uint32_t k, uint32_t pos, sk_int v) { c->row[k].r[pos] = v; }
void sk_d1_t_corrupt_pair(sk_d1 *c, uint32_t i, uint32_t j) { pair_set(c, i, j); }
#endif
