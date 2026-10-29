/* d2.c: Design 2, the coprime base, with on-demand, local refinement (spec §8, as amended Oct 29, 2026).  Six
 * structures: numerators, denominators, the lookup (spec §5), the base, the exponent rows and the remainders.
 *
 * Row k holds the known part of d_k, (piece, nu) entries, and a remainder u_k: d_k = prod q^nu * u_k (spec §8.1).
 * u_k may share factors with any piece, its own row's included.  The base stays pairwise coprime.
 *
 *  - Entry and store compute no gcd against the base: a new denominator is stored with the known row R the
 *    operation produced and the rest in u (spec §8.3).
 *  - add, sub and div first meet rows i and j (meet() below, spec §8.2), which leaves
 *        (M1) u_i coprime to every piece of row j,  (M2) u_j coprime to every piece of row i,  (M3) gcd(u_i,u_j) = 1.
 *    Then every prime p dividing both d_i and d_j lies in a piece held by both rows (by M1-M3 it divides neither
 *    remainder, and the base is pairwise coprime), so gcd(d_i, d_j) = prod over shared pieces q^min(a,b), and the
 *    walk of the two rows gives the residuals, with r_x and r_y multiplied by u_i and u_j (spec §8.7).
 *  - mul needs no meeting: g_2 = gcd(m_y, d_i) is taken piece by piece over row i and then u_i, dividing m_y as it
 *    goes, so overlaps between u_i and row i's pieces are harmless; likewise g_1 (spec §8.5).
 *
 * meet() is a worklist.  Queue 0 holds the pieces u_i must be refined against (row j's), queue 1 those for u_j
 * (row i's).  A piece pulled out of u_i into row i is a piece row i gains, so it goes on queue 1: the extra
 * refinement, without which M2 fails (u_j was refined against row i's pieces as they were, not against the pieces
 * row i gains during the meeting; unit test test_d2_extra).  Likewise for u_j.  A split of a queued piece queues
 * its new pieces.  When both queues are done and g = gcd(u_i, u_j) > 1, g is refined against the whole base (one
 * scan, as the former entry scan) and every piece of g goes on both queues. */
#include "sk_d2.h"

#include "sk_internal.h"

typedef struct d2_ent { /* (piece index, nu_q(d)): two 32-bit fields, 8 bytes (D2-6) */
    uint32_t q, nu;
} d2_ent;

typedef struct d2_row { /* one exponent row; this header is the per-row header of spec §12.2 */
    d2_ent *e;
    uint32_t len, cap;
} d2_row;

typedef struct d2_l { /* an entry (q, n, p, h) of L (spec §8.3): q is a piece index */
    uint32_t q, n;
    sk_int p, h;
} d2_l;

typedef struct d2_eq { /* an entry (q, a, q^a) of "equal" (spec §8.4) */
    uint32_t q, a;
    sk_int p;
} d2_eq;

struct sk_d2 {
    sk_int *num;
    uint32_t *nk;
    uint32_t N, Ncap;
    sk_int *den;
    sk_int *u; /* the remainders, one per denominator (spec §8.1); den and u share the capacity Dcap */
    uint32_t D, Dcap;
    sk_lookup look;
    sk_int *base; /* the pieces, pairwise coprime, each > 1 */
    uint32_t C, Ccap;
    d2_row *rows; /* the known parts: exponent rows, one per denominator */
    uint32_t rows_cap;

    /* temporaries of one call (not structures): R, L, "equal", and a meeting's two rows and two queues */
    d2_row R;
    d2_l *L;
    uint32_t nL, capL;
    d2_eq *eq;
    uint32_t neq, capeq;
    uint32_t mk[2];
    uint32_t *Q[2];
    uint32_t nQ[2], hQ[2], capQ[2];

    sk_opctx ctx;
    sk_overflow ovf;
#ifdef SK_COUNT
    sk_counters cnt;
#endif
#ifdef SK_AUDIT
    uint32_t mark_N, mark_D;
    uint32_t *chg_piece; /* pieces created or rewritten since the last audit */
    uint32_t n_chg_piece, cap_chg_piece;
    uint8_t *piece_dirty;
    uint32_t piece_dirty_cap;
    uint32_t *chg_row; /* existing rows rewritten (by a split or a meeting) since the last audit */
    uint32_t n_chg_row, cap_chg_row;
    uint8_t *row_dirty;
    uint32_t row_dirty_cap;
    char msg[200];
    uint64_t meet_steps, meet_bound; /* the meeting's iteration bound (DEC-49) */
#endif
};

/* Test hook: 1 drops the extra refinement (a piece a row gains in a meeting is not queued for the other remainder),
 * so that the unit test can show M1/M2 need it.  Read only in the test build; never set outside the unit test. */
int sk_d2_t_skip_extra = 0;
#ifdef SK_AUDIT
#define SKIP_EXTRA sk_d2_t_skip_extra
#else
#define SKIP_EXTRA 0
#endif

/* Test hook: 1 injects a fault into reduce (a piece that divides the remainder is not divided out), so that the
 * unit test can show the meeting's iteration bound turns the resulting endless loop into a loud failure.  Read only
 * in the test build; never set outside the unit test. */
int sk_d2_t_fault_stuck = 0;
#ifdef SK_AUDIT
#define FAULT_STUCK sk_d2_t_fault_stuck
#else
#define FAULT_STUCK 0
#endif

/* The meeting's iteration bound (DEC-49), audit and test builds only: every loop step of a meeting (queue takes,
 * reduce and absorb steps, repeats) counts one, and a meeting that passes the bound aborts, so that a fault fails
 * loudly instead of hanging.  The bound is generous: with B = bits(u_i) + bits(u_j) at the start, at most B + 1
 * repeats, B absorbs and B divisions happen, at most B pieces are split, each into at most w pieces, so the base
 * grows by at most (B + 1)(w + 1) pieces, and every scan is at most the base plus the pieces created.  The timing
 * build has no bound and no step count. */
#ifdef SK_AUDIT
#define MEET_STEP()                                                                                      \
    do {                                                                                                 \
        if (++c->meet_steps > c->meet_bound) sk_die("d2: meeting passed its iteration bound (a fault)"); \
    } while (0)
#else
#define MEET_STEP() ((void)0)
#endif

#define OVF(step) return sk_overflow_at(&c->ovf, &c->ctx, (step))

sk_d2 *sk_d2_create(void) {
    sk_d2 *c = sk_xcalloc(1, sizeof *c);
    sk_lookup_init(&c->look);
    return c;
}

void sk_d2_destroy(sk_d2 *c) {
    if (!c) return;
    free(c->num);
    free(c->nk);
    free(c->den);
    free(c->u);
    sk_lookup_free(&c->look);
    free(c->base);
    for (uint32_t k = 0; k < c->D; k++) free(c->rows[k].e);
    free(c->rows);
    free(c->R.e);
    free(c->L);
    free(c->eq);
    free(c->Q[0]);
    free(c->Q[1]);
#ifdef SK_AUDIT
    free(c->chg_piece);
    free(c->piece_dirty);
    free(c->chg_row);
    free(c->row_dirty);
#endif
#ifdef SK_COUNT
    sk_cur = &sk_sink;
#endif
    free(c);
}

/* ----------------------------------------------------------------------------------------- change list */

#ifdef SK_AUDIT
static void note_flag(uint8_t **flags, uint32_t *flags_cap, uint32_t need, uint32_t **list, uint32_t *n,
                      uint32_t *cap, uint32_t k) {
    if (*flags_cap <= k) {
        uint32_t nc = *flags_cap ? *flags_cap : 64;
        while (nc <= k || nc < need) nc *= 2;
        *flags = sk_xrealloc(*flags, nc);
        memset(*flags + *flags_cap, 0, nc - *flags_cap);
        *flags_cap = nc;
    }
    if ((*flags)[k]) return;
    (*flags)[k] = 1;
    if (*n >= *cap) {
        *cap = *cap ? 2 * *cap : 32;
        *list = sk_xrealloc(*list, sizeof(uint32_t) * *cap);
    }
    (*list)[(*n)++] = k;
}
#define NOTE_PIECE(p) \
    note_flag(&c->piece_dirty, &c->piece_dirty_cap, c->C, &c->chg_piece, &c->n_chg_piece, &c->cap_chg_piece, (p))
#define NOTE_ROW(k) note_flag(&c->row_dirty, &c->row_dirty_cap, c->D, &c->chg_row, &c->n_chg_row, &c->cap_chg_row, (k))
#else
#define NOTE_PIECE(p) ((void)0)
#define NOTE_ROW(k) ((void)0)
#endif

/* ----------------------------------------------------------------------------------------- rows */

static inline void row_reserve(d2_row *r, uint32_t extra) {
    if (r->len + extra > r->cap) {
        uint32_t nc = r->cap ? r->cap : 4;
        while (nc < r->len + extra) nc *= 2;
        r->e = sk_xrealloc(r->e, sizeof(d2_ent) * nc);
        r->cap = nc;
    }
}

static inline void row_append(d2_row *r, uint32_t q, uint32_t nu) {
    row_reserve(r, 1);
    r->e[r->len].q = q;
    r->e[r->len].nu = nu;
    r->len++;
}

/* The position of piece q in a sorted row, or of where it would go (index arithmetic). */
static inline uint32_t row_find(const d2_row *r, uint32_t q) {
    uint32_t lo = 0, hi = r->len;
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2;
        if (r->e[mid].q < q) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

/* Add k to nu_q in a sorted row, inserting (q, k) if q is absent.  True if q was absent. */
static bool row_add(d2_row *r, uint32_t q, uint32_t k) {
    uint32_t pos = row_find(r, q);
    if (pos < r->len && r->e[pos].q == q) {
        r->e[pos].nu += k;
        return false;
    }
    row_reserve(r, 1);
    memmove(r->e + pos + 1, r->e + pos, sizeof(d2_ent) * (r->len - pos));
    r->e[pos].q = q;
    r->e[pos].nu = k;
    r->len++;
    return true;
}

/* q^n by repeated multiplication (spec §8.4 item 1), n >= 1: n - 1 multiplications.  True on overflow. */
static bool power_of(sk_int q, uint32_t n, sk_int *out) {
    sk_int r = q;
    for (uint32_t k = 1; k < n; k++)
        if (sk_mul(r, q, &r)) return true;
    *out = r;
    return false;
}

/* a*b for two remainders whose product divides the result's denominator e, already computed and checked, so it
 * cannot overflow (spec §8.3).  A factor 1 is not multiplied (no multiplication counted). */
static inline sk_int umul(sk_int a, sk_int b) {
    if (a == 1) return b;
    if (b == 1) return a;
    sk_int r;
    if (sk_mul(a, b, &r)) sk_die("d2: a remainder product overflows (cannot happen: it divides a denominator)");
    return r;
}

/* ----------------------------------------------------------------------------------------- refinement (§8.2) */

/* The coprime base of {a, b} by pair refinement (spec §8.2): gcds and exact divisions only.  A worklist: each
 * pending number is tested against the numbers already known pairwise coprime (DECISIONS.md).  Every number
 * divides q, and their product only falls, so there are at most 63 of them. */
#define SK_PMAX 128
static uint32_t pair_refine(sk_int a, sk_int b, sk_int *out) {
    sk_int pend[SK_PMAX];
    uint32_t np = 0, nd = 0;
    pend[np++] = a;
    pend[np++] = b;
    while (np > 0) {
        sk_int x = pend[--np];
        bool merged = false;
        for (uint32_t t = 0; t < nd; t++) {
            sk_int h = sk_gcd(x, out[t]);
            if (h > 1) {
                sk_int y = out[t];
                out[t] = out[--nd];
                if (np + 3 > SK_PMAX) sk_die("d2: pair refinement list too long");
                pend[np++] = h;
                sk_int xh = sk_divexact(x, h), yh = sk_divexact(y, h);
                if (xh != 1) pend[np++] = xh;
                if (yh != 1) pend[np++] = yh;
                merged = true;
                break;
            }
        }
        if (!merged) {
            if (nd >= SK_PMAX) sk_die("d2: pair refinement list too long");
            out[nd++] = x;
        }
    }
    return nd;
}

/* Queue s of the meeting gets piece p, unless it already holds it, done or not (index work: a linear search,
 * since the queues are short). */
static void q_push(sk_d2 *c, int s, uint32_t p) {
    for (uint32_t t = 0; t < c->nQ[s]; t++)
        if (c->Q[s][t] == p) return;
    SK_RESERVE(c->Q[s], c->nQ[s], c->capQ[s], 16);
    c->Q[s][c->nQ[s]++] = p;
}

static bool q_has(const sk_d2 *c, int s, uint32_t p) {
    for (uint32_t t = 0; t < c->nQ[s]; t++)
        if (c->Q[s][t] == p) return true;
    return false;
}

/* split(q, g) (spec §8.2): q = piece p, 1 < g < q.  Rewrites every row holding p.  Splits happen only in meetings,
 * before the operation builds its result's row R (R is empty then), so R is not visited (DEC-48).  In a meeting, a
 * queue that holds p (done or not) also gets the new pieces. */
static void split(sk_d2 *c, uint32_t p, sk_int g) {
    sk_int q = c->base[p];
    sk_int P[SK_PMAX];
    uint32_t nP = pair_refine(g, sk_divexact(q, g), P);
    for (uint32_t s = 1; s < nP; s++) { /* sort P increasingly (insertion sort; comparisons only) */
        sk_int v = P[s];
        uint32_t t = s;
        while (t > 0 && P[t - 1] > v) {
            P[t] = P[t - 1];
            t--;
        }
        P[t] = v;
    }
    uint32_t nu[SK_PMAX];
    for (uint32_t s = 0; s < nP; s++) { /* nu_s(q): divide while it goes, counting every attempt */
        sk_int rest = q, rem;
        uint32_t k = 0;
        for (;;) {
            sk_int quo = sk_divrem(rest, P[s], &rem);
            if (rem != 0) break;
            rest = quo;
            k++;
        }
        nu[s] = k;
    }
    uint32_t first = c->C; /* index of P[1] */
    c->base[p] = P[0];
    NOTE_PIECE(p);
    for (uint32_t s = 1; s < nP; s++) {
        sk_check_limit(c->C, "pieces");
        SK_RESERVE(c->base, c->C, c->Ccap, 16);
        c->base[c->C] = P[s];
        NOTE_PIECE(c->C);
        c->C++;
    }
    CNT(ref_splits, 1);
    CNT(ref_pieces_created, nP - 1);
    /* every row */
    CNT(scan_split_rows, c->D);
    for (uint32_t k = 0; k < c->D; k++) {
        d2_row *r = &c->rows[k];
        uint32_t pos = row_find(r, p);
        if (pos >= r->len || r->e[pos].q != p) continue;
        uint32_t t = r->e[pos].nu;
        r->e[pos].nu = t * nu[0];
        row_reserve(r, nP - 1);
        for (uint32_t s = 1; s < nP; s++) { /* the new indices are the largest: appended, the row stays sorted */
            r->e[r->len].q = first + s - 1;
            r->e[r->len].nu = t * nu[s];
            r->len++;
        }
        CNT(ref_entries_rewritten, nP);
        NOTE_ROW(k);
    }
    for (int s = 0; s < 2; s++)
        if (q_has(c, s, p))
            for (uint32_t t = first; t < c->C; t++) q_push(c, s, t);
}

/* reduce(s, p) (spec §8.2): refine the remainder of the meeting's side s against piece p.  Every power of p is
 * pulled out of u into the row; where u holds only part of p, p is split, and the test stays at index p.  A piece
 * the row did not hold before goes on the other side's queue: the extra refinement. */
static void reduce(sk_d2 *c, int s, uint32_t p) {
    uint32_t k = c->mk[s];
    while (c->u[k] != 1) {
        MEET_STEP();
        CNT(scan_remainder_pieces, 1);
        sk_int q = c->base[p];
        sk_int g = sk_gcd(c->u[k], q);
        if (g == 1) return;
        if (g == q) {
            if (FAULT_STUCK) continue; /* injected fault: u is not divided, and the loop cannot end */
            uint32_t n = 0;
            for (;;) { /* divide u by q as many times as it goes, counting every attempt (counting rule 2) */
                sk_int rem, quo = sk_divrem(c->u[k], q, &rem);
                if (rem != 0) break;
                c->u[k] = quo;
                n++;
            }
            CNT(ref_remainder_pulls, 1);
            if (row_add(&c->rows[k], p, n) && !SKIP_EXTRA) {
                CNT(ev_extra_queued, 1);
                q_push(c, 1 - s, p);
            }
            NOTE_ROW(k);
        } else {
            split(c, p, g); /* the first new piece takes index p; the test stays at p */
        }
    }
}

/* absorb(g) (spec §8.2): g = gcd(u_i, u_j) > 1.  One scan of the base, as the former entry scan: g's factors in
 * existing pieces are divided out (staying at a piece after dividing by it or splitting it), and what remains, if
 * not 1, is a new piece.  Every piece of g goes on both queues. */
static void absorb(sk_d2 *c, sk_int g) {
    sk_int rest = g;
    uint32_t p = 0;
    CNT(ev_absorbs, 1);
    while (rest != 1 && p < c->C) {
        MEET_STEP();
        CNT(scan_refine_pieces, 1);
        sk_int q = c->base[p];
        sk_int h = sk_gcd(rest, q);
        if (h == 1) {
            p++;
        } else if (h == q) {
            for (;;) { /* counting every attempt (counting rule 2) */
                sk_int rem, quo = sk_divrem(rest, q, &rem);
                if (rem != 0) break;
                rest = quo;
            }
            q_push(c, 0, p);
            q_push(c, 1, p);
        } else {
            split(c, p, h); /* the scan stays at p */
        }
    }
    if (rest != 1) {
        sk_check_limit(c->C, "pieces");
        SK_RESERVE(c->base, c->C, c->Ccap, 16);
        c->base[c->C] = rest;
        NOTE_PIECE(c->C);
        q_push(c, 0, c->C);
        q_push(c, 1, c->C);
        c->C++;
        CNT(ref_pieces_created, 1);
    }
}

/* meet(i, j), i != j (spec §8.2): establishes M1, M2 and M3 (top of the file). */
static void meet(sk_d2 *c, uint32_t i, uint32_t j) {
    CNT(ev_meetings, 1);
    c->mk[0] = i;
    c->mk[1] = j;
    c->nQ[0] = c->nQ[1] = c->hQ[0] = c->hQ[1] = 0;
    if (c->u[i] == 1 && c->u[j] == 1) return;
#ifdef SK_AUDIT
    {
        uint64_t B = (uint64_t)sk_bits_u_raw((uint64_t)c->u[i]) + (uint64_t)sk_bits_u_raw((uint64_t)c->u[j]);
        uint64_t K = (uint64_t)c->C + c->rows[i].len + c->rows[j].len + (B + 1) * (SK_WIDTH + 1);
        c->meet_steps = 0;
        c->meet_bound = 8 * (B + 2) * K;
    }
#endif
    /* queue 0 (for u_i): row j's pieces; queue 1 (for u_j): row i's.  The pieces of a row are distinct. */
    for (int s = 0; s < 2; s++) {
        const d2_row *r = &c->rows[c->mk[1 - s]];
        if (c->capQ[s] < r->len) {
            c->Q[s] = sk_xrealloc(c->Q[s], sizeof(uint32_t) * r->len);
            c->capQ[s] = r->len;
        }
        for (uint32_t t = 0; t < r->len; t++) c->Q[s][t] = r->e[t].q;
        c->nQ[s] = r->len;
    }
    for (;;) {
        MEET_STEP();
        while (c->hQ[0] < c->nQ[0] || c->hQ[1] < c->nQ[1]) {
            MEET_STEP();
            int s = c->hQ[0] < c->nQ[0] ? 0 : 1;
            uint32_t p = c->Q[s][c->hQ[s]++];
            if (c->u[c->mk[s]] != 1) reduce(c, s, p);
        }
        if (c->u[i] == 1 || c->u[j] == 1) break;
        sk_int g = sk_gcd(c->u[i], c->u[j]);
        if (g == 1) break;
        absorb(c, g);
    }
}

/* power(h, q) (spec §8.3): k if h = q^k, otherwise -1.  h = 1 costs no division. */
static int64_t power_test(sk_int h, sk_int q) {
    int64_t k = 0;
    while (h != 1) {
        sk_int rem, quo = sk_divrem(h, q, &rem);
        if (rem != 0) return -1;
        h = quo;
        k++;
    }
    return k;
}

/* finish(R, L) (spec §8.3): completes R in c->R from L; each leftover multiplies *u.  True on overflow. */
static bool finish(sk_d2 *c, sk_int *u_io) {
    sk_int u = *u_io;
    for (uint32_t t = 0; t < c->nL; t++) {
        const d2_l *l = &c->L[t];
        int64_t k = power_test(l->h, c->base[l->q]);
        if (k >= 0) {
            if ((int64_t)l->n - k > 0) row_add(&c->R, l->q, (uint32_t)((int64_t)l->n - k));
        } else if (sk_mul(u, sk_divexact(l->p, l->h), &u)) { /* a leftover: q is composite */
            return true;
        }
    }
    *u_io = u;
    return false;
}

/* ----------------------------------------------------------------------------------------- store */

/* store(n over e) (spec §5, §8.3), with R in c->R, the remainder u, and L in c->L if has_L.  No refinement. */
static sk_status store(sk_d2 *c, sk_int n, sk_int e, bool has_L, sk_int u, sk_handle *out) {
    CNT_SET(c->cnt, c->ctx.op, SK_STORE);
    if (n == 0) { /* spec §5: 0/1, with R and L empty and u = 1 */
        e = 1;
        c->R.len = 0;
        c->nL = 0;
        has_L = false;
        u = 1;
    }
    uint32_t empty;
    uint32_t k = sk_lookup_find(&c->look, c->den, e, &empty);
    if (k == SK_NONE) {
        /* finish runs only for a new denominator, after the lookup (spec §8.3 item 1).  It is run before the
         * append so that an overflow in it (which the arithmetic rules out: u divides e) stores nothing. */
        if (has_L && finish(c, &u)) OVF("finish: u*(p/h)");
        sk_check_limit(c->D, "denominators");
        if (c->D >= c->Dcap) {
            c->Dcap = c->Dcap ? 2 * c->Dcap : 16;
            c->den = sk_xrealloc(c->den, sizeof(sk_int) * c->Dcap);
            c->u = sk_xrealloc(c->u, sizeof(sk_int) * c->Dcap);
        }
        k = c->D;
        c->den[k] = e;
        c->u[k] = u;
        sk_lookup_insert(&c->look, c->den, k, empty);
        if (u == 1) CNT(ev_rows_u1, 1);
        else CNT(ev_rows_unrefined, 1);
        SK_RESERVE(c->rows, c->D, c->rows_cap, 16);
        d2_row *r = &c->rows[k];
        r->len = r->cap = c->R.len;
        r->e = c->R.len ? sk_xmalloc(sizeof(d2_ent) * c->R.len) : NULL;
        if (c->R.len) memcpy(r->e, c->R.e, sizeof(d2_ent) * c->R.len);
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

static sk_status begin(sk_d2 *c, sk_phase op, sk_handle x, sk_handle y) {
    if (x >= c->N || y >= c->N) return SK_EINVAL;
    c->ctx.op = op;
    c->ctx.x = x;
    c->ctx.y = y;
    c->R.len = 0;
    c->nL = 0;
    c->neq = 0;
    CNT_SET(c->cnt, op, SK_CORE);
    return SK_OK;
}

static inline void L_add(sk_d2 *c, uint32_t q, uint32_t n, sk_int p, sk_int h) {
    SK_RESERVE(c->L, c->nL, c->capL, 16);
    c->L[c->nL].q = q;
    c->L[c->nL].n = n;
    c->L[c->nL].p = p;
    c->L[c->nL].h = h;
    c->nL++;
}

/* Walk rows i and j together (spec §8.4-8.6): yields each piece of either row with a = nu(d_i), b = nu(d_j). */
typedef struct walk {
    const d2_row *A, *B;
    uint32_t ia, ib;
} walk;

static inline bool walk_next(walk *w, uint32_t *q, uint32_t *a, uint32_t *b) {
    bool ha = w->ia < w->A->len, hb = w->ib < w->B->len;
    if (!ha && !hb) return false;
    if (ha && (!hb || w->A->e[w->ia].q < w->B->e[w->ib].q)) {
        CNT(scan_merge_entries, 1);
        *q = w->A->e[w->ia].q;
        *a = w->A->e[w->ia++].nu;
        *b = 0;
    } else if (!ha || w->B->e[w->ib].q < w->A->e[w->ia].q) {
        CNT(scan_merge_entries, 1);
        *q = w->B->e[w->ib].q;
        *a = 0;
        *b = w->B->e[w->ib++].nu;
    } else {
        CNT(scan_merge_entries, 2);
        *q = w->A->e[w->ia].q;
        *a = w->A->e[w->ia++].nu;
        *b = w->B->e[w->ib++].nu;
    }
    return true;
}

sk_status sk_d2_enter(sk_d2 *c, sk_int m, sk_int d, sk_handle *out) {
    if (!sk_valid_input(m) || !sk_valid_input(d)) return SK_EINVAL;
    if (d == 0) return SK_EDIVZERO;
    c->ctx.op = SK_PH_ENTRY;
    c->ctx.x = c->ctx.y = SK_NONE;
    c->R.len = 0;
    c->nL = 0;
    CNT_SET(c->cnt, SK_PH_ENTRY, SK_CORE);
    if (d < 0) {
        m = -m;
        d = -d;
    }
    sk_int g = sk_gcd(m, d);
    sk_int e = sk_divexact(d, g);
    return store(c, sk_divexact(m, g), e, false, e, out); /* R empty, u = e: no gcd against the base */
}

/* spec §8.4 add; subtraction is addition of the negated second operand */
static sk_status addsub(sk_d2 *c, sk_phase op, sk_handle x, sk_handle y, sk_handle *out) {
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
        sk_int u = sk_divexact(c->den[i], h);
        return store(c, sk_divexact(t, h), u, false, u, out); /* R empty, u = d_i/h */
    }
    meet(c, i, j);
    sk_int rx = c->u[i], ry = c->u[j], pw;
    walk w = {&c->rows[i], &c->rows[j], 0, 0};
    uint32_t q, va, vb;
    while (walk_next(&w, &q, &va, &vb)) {
        uint32_t mn = va < vb ? va : vb;
        sk_int qv = c->base[q];
        if (va > mn) {
            if (power_of(qv, va - mn, &pw)) OVF("add: q^(a-min(a,b))");
            if (sk_mul(rx, pw, &rx)) OVF("add: r_x*q^(a-min(a,b))");
        }
        if (vb > mn) {
            if (power_of(qv, vb - mn, &pw)) OVF("add: q^(b-min(a,b))");
            if (sk_mul(ry, pw, &ry)) OVF("add: r_y*q^(b-min(a,b))");
        }
        if (va == vb) {
            SK_RESERVE(c->eq, c->neq, c->capeq, 16);
            if (power_of(qv, va, &c->eq[c->neq].p)) OVF("add: q^a (equal piece)");
            c->eq[c->neq].q = q;
            c->eq[c->neq].a = va;
            c->neq++;
        } else {
            row_append(&c->R, q, va > vb ? va : vb);
        }
    }
    if (sk_mul(mx, ry, &a)) OVF("add: m_x*r_y");
    if (sk_mul(my, rx, &b)) OVF("add: m_y*r_x");
    if (sk_add(a, b, &t)) OVF("add: t = m_x*r_y + m_y*r_x");
    if (sk_mul(c->den[i], ry, &e)) OVF("add: e = d_i*r_y");
    for (uint32_t k = 0; k < c->neq; k++) {
        sk_int h = sk_gcd(t, c->eq[k].p);
        if (h != 1) {
            t = sk_divexact(t, h);
            e = sk_divexact(e, h);
        }
        L_add(c, c->eq[k].q, c->eq[k].a, c->eq[k].p, h);
    }
    /* u_i u_j divides e: each is coprime to the other denominator and to the other remainder (spec §8.7) */
    return store(c, t, e, true, umul(c->u[i], c->u[j]), out);
}

sk_status sk_d2_add(sk_d2 *c, sk_handle x, sk_handle y, sk_handle *out) { return addsub(c, SK_PH_ADD, x, y, out); }
sk_status sk_d2_sub(sk_d2 *c, sk_handle x, sk_handle y, sk_handle *out) { return addsub(c, SK_PH_SUB, x, y, out); }

sk_status sk_d2_mul(sk_d2 *c, sk_handle x, sk_handle y, sk_handle *out) {
    sk_status st = begin(c, SK_PH_MUL, x, y);
    if (st) return st;
    sk_int mx = c->num[x], my = c->num[y];
    uint32_t i = c->nk[x], j = c->nk[y];
    sk_int n, e;
    if (i == j) {
        CNT(ev_fast_paths, 1);
        const d2_row *ri = &c->rows[i];
        for (uint32_t k = 0; k < ri->len; k++) row_append(&c->R, ri->e[k].q, 2 * ri->e[k].nu);
        if (sk_mul(mx, my, &n)) OVF("multiply: m_x*m_y");
        if (sk_mul(c->den[i], c->den[i], &e)) OVF("multiply: d_i*d_i");
        return store(c, n, e, false, umul(c->u[i], c->u[i]), out); /* R = row i doubled, u = u_i^2 */
    }
    sk_int g1 = 1, g2 = 1, p;
    walk w = {&c->rows[i], &c->rows[j], 0, 0};
    uint32_t q, va, vb;
    while (walk_next(&w, &q, &va, &vb)) {
        if (va > 0 && vb > 0) {
            row_append(&c->R, q, va + vb); /* nothing can cancel */
            continue;
        }
        sk_int *m, *G;
        uint32_t nn;
        if (vb > 0) { /* a piece of d_j only */
            m = &mx;
            nn = vb;
            G = &g1;
        } else { /* a piece of d_i only */
            m = &my;
            nn = va;
            G = &g2;
        }
        if (power_of(c->base[q], nn, &p)) OVF("multiply: p = q^n");
        sk_int h = sk_gcd(*m, p);
        *m = sk_divexact(*m, h);
        if (sk_mul(*G, h, G)) OVF("multiply: G*h");
        L_add(c, q, nn, p, h);
    }
    /* then the remainders, with the numerators as already divided: gcd(m_y, u_i) and gcd(m_x, u_j) */
    sk_int ui = c->u[i], uj = c->u[j];
    if (ui != 1) {
        sk_int h = sk_gcd(my, ui);
        if (h != 1) {
            my = sk_divexact(my, h);
            ui = sk_divexact(ui, h);
            if (sk_mul(g2, h, &g2)) OVF("multiply: G*h");
        }
    }
    if (uj != 1) {
        sk_int h = sk_gcd(mx, uj);
        if (h != 1) {
            mx = sk_divexact(mx, h);
            uj = sk_divexact(uj, h);
            if (sk_mul(g1, h, &g1)) OVF("multiply: G*h");
        }
    }
    if (sk_mul(sk_divexact(c->den[i], g2), sk_divexact(c->den[j], g1), &e)) OVF("multiply: e = (d_i/g_2)*(d_j/g_1)");
    if (sk_mul(mx, my, &n)) OVF("multiply: m_x*m_y");
    return store(c, n, e, true, umul(ui, uj), out);
}

sk_status sk_d2_div(sk_d2 *c, sk_handle x, sk_handle y, sk_handle *out) {
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
        sk_int u = sk_divexact(t, g1);
        return store(c, n, u, false, u, out); /* R empty, u = t/g_1 = e */
    }
    meet(c, i, j);
    sk_int rx = c->u[i], ry = c->u[j], pw;
    walk w = {&c->rows[i], &c->rows[j], 0, 0};
    uint32_t q, va, vb;
    while (walk_next(&w, &q, &va, &vb)) {
        uint32_t mn = va < vb ? va : vb;
        sk_int qv = c->base[q];
        if (va > mn) {
            if (power_of(qv, va - mn, &pw)) OVF("divide: q^(a-min(a,b))");
            if (sk_mul(rx, pw, &rx)) OVF("divide: r_x*q^(a-min(a,b))");
        }
        if (vb > mn) {
            if (power_of(qv, vb - mn, &pw)) OVF("divide: q^(b-min(a,b))");
            if (sk_mul(ry, pw, &ry)) OVF("divide: r_y*q^(b-min(a,b))");
        }
        if (va > mn) row_append(&c->R, q, va - mn);
    }
    sk_int a = sk_divexact(mx, g1);
    if (neg) a = -a;
    sk_int u = sk_divexact(t, g1);
    if (sk_mul(a, ry, &n)) OVF("divide: s*(m_x/g_1)*r_y");
    if (sk_mul(rx, u, &e)) OVF("divide: r_x*(t/g_1)");
    return store(c, n, e, false, umul(c->u[i], u), out); /* R = the pieces of d_i/g_2; u = u_i * t/g_1 */
}

sk_status sk_d2_read(const sk_d2 *c, sk_handle x, sk_int *m, sk_int *d) {
    if (x >= c->N) return SK_EINVAL;
    *m = c->num[x];
    *d = c->den[c->nk[x]];
    return SK_OK;
}

uint32_t sk_d2_size(const sk_d2 *c) { return c->N; }

void sk_d2_memory(const sk_d2 *c, sk_memory *r) {
    memset(r, 0, sizeof *r);
    uint64_t w = sizeof(sk_int);
    sk_memory_add(r, "numerators", (uint64_t)c->N * (w + 4), (uint64_t)c->Ncap * (w + 4));
    sk_memory_add(r, "denominators", (uint64_t)c->D * w, (uint64_t)c->Dcap * w);
    sk_memory_add(r, "lookup", 4 * (uint64_t)c->look.S, 4 * (uint64_t)c->look.S);
    sk_memory_add(r, "base", (uint64_t)c->C * w, (uint64_t)c->Ccap * w);
    uint64_t used = 0, alloc = 0;
    for (uint32_t k = 0; k < c->D; k++) {
        used += c->rows[k].len;
        alloc += c->rows[k].cap;
    }
    sk_memory_add(r, "exponent rows", used * sizeof(d2_ent) + (uint64_t)c->D * sizeof(d2_row),
                  alloc * sizeof(d2_ent) + (uint64_t)c->rows_cap * sizeof(d2_row));
    sk_memory_add(r, "remainders", (uint64_t)c->D * w, (uint64_t)c->Dcap * w);
    sk_memory_finish(r, c->N);
}

sk_status sk_d2_counters(const sk_d2 *c, sk_counters *r) {
#ifdef SK_COUNT
    *r = c->cnt;
    return SK_OK;
#else
    (void)c;
    memset(r, 0, sizeof *r);
    return SK_ENOTBUILT;
#endif
}

sk_status sk_d2_overflow(const sk_d2 *c, sk_overflow *r) {
    *r = c->ovf;
    return c->ovf.count ? SK_OK : SK_EINVAL;
}

/* ----------------------------------------------------------------------------------------- audits (spec §13.2) */

#define SK_LARGE_BASE 1024u /* spec §13.3 item 2, T-2 */

#ifdef SK_AUDIT
static const char *check_values(const sk_d2 *c, uint32_t from) {
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

/* row k: sorted, every nu >= 1, every index below C, u >= 1, and (known part) * u = d_k */
static const char *check_row(const sk_d2 *c, uint32_t k) {
    const d2_row *r = &c->rows[k];
    if (c->u[k] < 1) return "row: remainder below 1";
    sk_int prod = c->u[k];
    for (uint32_t t = 0; t < r->len; t++) {
        if (r->e[t].q >= c->C) return "row: piece index not below C";
        if (t > 0 && r->e[t].q <= r->e[t - 1].q) return "row: not sorted by piece index";
        if (r->e[t].nu < 1) return "row: a valuation below 1";
        for (uint32_t s = 0; s < r->e[t].nu; s++)
            if (sk_mul_raw(prod, c->base[r->e[t].q], &prod)) return "row: product of q^nu times u overflows";
    }
    if (prod != c->den[k]) return "row: product of q^nu times u is not the denominator";
    return NULL;
}

static const char *check_piece(const sk_d2 *c, uint32_t p) {
    if (c->base[p] <= 1) return "base: a piece is not greater than 1";
    for (uint32_t s = 0; s < c->C; s++)
        if (s != p && sk_gcd(c->base[p], c->base[s]) != 1) return "base: two pieces are not coprime";
    return NULL;
}

static const char *check_changed_pieces(const sk_d2 *c) {
    const char *f;
    for (uint32_t t = 0; t < c->n_chg_piece; t++)
        if ((f = check_piece(c, c->chg_piece[t]))) return f;
    return NULL;
}

/* The full audit.  capped: the scheduled audit, which above SK_LARGE_BASE pieces checks pairwise coprimality
 * only for the pieces changed since the last audit (spec §13.3 item 2). */
static const char *full(const sk_d2 *c, bool capped) {
    const char *f;
    if ((f = check_values(c, 0))) return f;
    if ((f = sk_lookup_audit_full(&c->look, c->den, c->D))) return f;
    if (capped && c->C > SK_LARGE_BASE) {
        for (uint32_t p = 0; p < c->C; p++)
            if (c->base[p] <= 1) return "base: a piece is not greater than 1";
        if ((f = check_changed_pieces(c))) return f;
    } else {
        for (uint32_t p = 0; p < c->C; p++) {
            if (c->base[p] <= 1) return "base: a piece is not greater than 1";
            for (uint32_t s = p + 1; s < c->C; s++)
                if (sk_gcd(c->base[p], c->base[s]) != 1) return "base: two pieces are not coprime";
        }
    }
    for (uint32_t k = 0; k < c->D; k++)
        if ((f = check_row(c, k))) return f;
    return NULL;
}

static const char *changed(const sk_d2 *c) {
    const char *f;
    if ((f = check_values(c, c->mark_N))) return f;
    for (uint32_t k = c->mark_D; k < c->D; k++)
        if (sk_lookup_find(&c->look, c->den, c->den[k], NULL) != k)
            return "lookup: a new denominator is not found at its value";
    if ((f = sk_lookup_audit_count(&c->look, c->D))) return f;
    if ((f = check_changed_pieces(c))) return f;
    for (uint32_t k = c->mark_D; k < c->D; k++)
        if ((f = check_row(c, k))) return f;
    for (uint32_t t = 0; t < c->n_chg_row; t++)
        if ((f = check_row(c, c->chg_row[t]))) return f;
    return NULL;
}

static sk_status finish_audit(sk_d2 *c, const char *fail, const char **why) {
    if (fail) {
        snprintf(c->msg, sizeof c->msg, "d2: %s", fail);
        if (why) *why = c->msg;
        return SK_EAUDIT;
    }
    c->mark_N = c->N;
    c->mark_D = c->D;
    for (uint32_t t = 0; t < c->n_chg_piece; t++) c->piece_dirty[c->chg_piece[t]] = 0;
    c->n_chg_piece = 0;
    for (uint32_t t = 0; t < c->n_chg_row; t++) c->row_dirty[c->chg_row[t]] = 0;
    c->n_chg_row = 0;
    if (why) *why = NULL;
    return SK_OK;
}

static sk_status run_audit(sk_d2 *c, int kind, const char **why) {
    sk_count *saved = sk_cur;
    sk_cur = &sk_sink;
    const char *f = kind == 2 ? changed(c) : full(c, kind == 1);
    sk_cur = saved;
    return finish_audit(c, f, why);
}
#endif

sk_status sk_d2_audit(sk_d2 *c, const char **why) {
#ifdef SK_AUDIT
    return run_audit(c, 0, why);
#else
    (void)c;
    if (why) *why = "audits not built";
    return SK_ENOTBUILT;
#endif
}

sk_status sk_d2_audit_scheduled(sk_d2 *c, const char **why) {
#ifdef SK_AUDIT
    return run_audit(c, 1, why);
#else
    (void)c;
    if (why) *why = "audits not built";
    return SK_ENOTBUILT;
#endif
}

sk_status sk_d2_audit_changed(sk_d2 *c, const char **why) {
#ifdef SK_AUDIT
    return run_audit(c, 2, why);
#else
    (void)c;
    if (why) *why = "audits not built";
    return SK_ENOTBUILT;
#endif
}

/* ----------------------------------------------------------------------------------------- test hooks */

uint32_t sk_d2_t_D(const sk_d2 *c) { return c->D; }
uint32_t sk_d2_t_C(const sk_d2 *c) { return c->C; }
sk_int sk_d2_t_piece(const sk_d2 *c, uint32_t p) { return c->base[p]; }
sk_int sk_d2_t_den(const sk_d2 *c, uint32_t k) { return c->den[k]; }
sk_int sk_d2_t_u(const sk_d2 *c, uint32_t k) { return c->u[k]; }
uint32_t sk_d2_t_den_index(const sk_d2 *c, sk_handle x) { return c->nk[x]; }
uint32_t sk_d2_t_row_len(const sk_d2 *c, uint32_t k) { return c->rows[k].len; }
void sk_d2_t_row_entry(const sk_d2 *c, uint32_t k, uint32_t t, uint32_t *q, uint32_t *nu) {
    *q = c->rows[k].e[t].q;
    *nu = c->rows[k].e[t].nu;
}
#ifdef SK_AUDIT
/* test build only: break an invariant, so the tests can check that the audits find it */
void sk_d2_t_corrupt_nu(sk_d2 *c, uint32_t k, uint32_t t, uint32_t nu) {
    c->rows[k].e[t].nu = nu;
    NOTE_ROW(k);
}
void sk_d2_t_corrupt_u(sk_d2 *c, uint32_t k, sk_int u) {
    c->u[k] = u;
    NOTE_ROW(k);
}
void sk_d2_t_corrupt_piece(sk_d2 *c, uint32_t p, sk_int v) {
    c->base[p] = v;
    NOTE_PIECE(p);
}
#endif
