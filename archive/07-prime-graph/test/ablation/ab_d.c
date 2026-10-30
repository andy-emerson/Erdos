/* ab_d.c: round 3's dictionary rows on V3's layout (round3_bench.py).  #included after ab_v3.c and ab_v6.c when
 * VARIANT >= VAR_D0.
 *
 *   D0  V3 + V6's seen-prime dictionary entry as it stands (ab_v6.c's v6_enter_den and v6_den_edges, unchanged).
 *   D1  D0 with the small-prime table tested in branch-free blocks of 8.  D0 already tests each prime with the
 *       multiply-by-inverse exact-division test, but one prime at a time, with a multiply and a branch per prime for
 *       the adaptive stop; D1 tests 8 primes without branches (clang vectorises the block), takes the stop once per
 *       block (n below the square of the block's first prime), and branches once per block.  Everything else is D0's
 *       code, including pg_factor (and its second pass over the table) for a composite large leftover.  GMP-style
 *       packing (n mod a product of small primes, the tests on the residue) did not help one-word denominators in
 *       an isolated measurement (round3_bench.py's notes), so D1 does not pack.
 *   D2  D0 without rho at entry.  Miller-Rabin stays: a large leftover (2^32 or more after the table and the known
 *       large primes) that is prime is entered as D0 enters it; a composite one is only appended to a list of
 *       leftovers (deduplicated by a hash), with no gcd and no factoring.  The idle pass (d2_idle, in the driver's
 *       decomposition slot, between entry's pass 1 and pass 2: timed, reported apart, included in the total) runs a
 *       batch gcd over the leftovers (GMP product tree, remainder tree modulo the squares), splits each leftover
 *       that shares with another into gcd and cofactor, divides a piece still composite by the large primes known
 *       so far (D0's step 1b loop), factors only what is still composite (pg_factor's rho), and merges each
 *       leftover's primes through the dictionary with D0's rule, in entry order.
 *   D3  D0 + a product check of large leftovers: in place of D0's step 1b (exact division by every large prime in
 *       the dictionary), a product forest of those primes (perfect subtrees of at most 2^D3_HMAX leaves); a new
 *       leftover c is checked with one remainder of the product (each root's limbs mod c, combined) and one gcd,
 *       and the forest is searched (child remainders and gcds) only on a hit.
 *
 * Every row also needs V3-layout glue for the dictionary's splits (d_fresh, d_fill_end, the edge dump).
 */

/* ---------------------------------------------------------------- V3-layout glue (D0-D3) */

static uint64_t st_dfresh;  /* arrays whose edges were re-expanded after a split */

static int d_dead(const uint32_t *e, int ne) {
    for (int i = 0; i < ne; i++) if (!nodes[EN(e[i])].alive) return 1;
    return 0;
}
/* e (possibly naming dead nodes, possibly from lists of different split epochs) -> out on live nodes, ascending,
 * one entry per node (weights of the same node added).  -1 if a weight passes 255. */
static int d_resolve(const uint32_t *e, int ne, uint32_t *out) {
    uint32_t tmp[4 * AB_W];
    int k = 0;
    for (int i = 0; i < ne && k >= 0; i++) k = v6_expand(EN(e[i]), EW(e[i]), tmp, k);
    if (k < 0) return -1;
    for (int a = 1; a < k; a++)
        for (int b = a; b > 0 && EN(tmp[b - 1]) > EN(tmp[b]); b--) { uint32_t t = tmp[b]; tmp[b] = tmp[b - 1]; tmp[b - 1] = t; }
    int m = 0;
    for (int i = 0; i < k; i++) {
        if (m && EN(out[m - 1]) == EN(tmp[i])) {
            uint32_t w = EW(out[m - 1]) + EW(tmp[i]);
            if (w > 255) return -1;
            out[m - 1] = EP(EN(tmp[i]), w);
        } else out[m++] = tmp[i];
    }
    return m;
}
static void d_fresh_edges(uint32_t *e, int *ne) {
    if (!d_dead(e, *ne)) return;
    uint32_t out[AB_W];
    int k = d_resolve(e, *ne, out);
    if (k < 0 || k > AB_MAXE) die("d: edges after a split");
    memcpy(e, out, sizeof(uint32_t) * k);
    *ne = k;
}
static void d_fresh(ab_arr *a) {  /* D and every exception onto live nodes, if a split happened since */
    if (a->ep == v6_epoch) return;
    st_dfresh++;
    d_fresh_edges(a->de, &a->dne);
    for (size_t j = 0; j < a->nx; j++) {
        xval *x = &a->xv[j];
        if (!x->den) continue;
        int ne = x->ne;
        d_fresh_edges(x->e, &ne);
        x->ne = (uint8_t)ne;
    }
    a->ep = v6_epoch;
}
static void d_fill_end(void) {  /* elements entered before a fill-time split: re-expanded now (timed) */
    for (int p = 0; p < npend; p++) d_fresh(pending[p].a);
    v6_phase = 1;
    if (getenv("AB_V6_DUMP")) v6_snapshot();
}
static size_t d_snap_edges(size_t o) {  /* AB_V6_DUMP: the first 16 distinct denominators' edges, in entry order */
    o += snprintf(v6dump + o, sizeof v6dump - o, " v6edges=");
    for (uint32_t k = 0, c = 0; k < ndd && c < 16 && o < sizeof v6dump - 256; k++) {
        const dden *D = &dd[k];
        if (D->bad) continue;
        o += snprintf(v6dump + o, sizeof v6dump - o, "%s%llu:", c++ ? ";" : "", (unsigned long long)D->den);
        for (int j = 0; j < D->ne; j++)
            o += snprintf(v6dump + o, sizeof v6dump - o, "%s%llu^%u", j ? "*" : "",
                          (unsigned long long)nodes[EN(D->e[j])].v, EW(D->e[j]));
    }
    return o;
}

/* ---------------------------------------------------------------- D1: the small-prime table in branch-free blocks of 8 */

#if VARIANT == VAR_D1
#define D1_NB ((V6_NSMALL + 7) / 8)
static uint64_t d1i[D1_NB * 8], d1l[D1_NB * 8], d1sq[D1_NB];  /* per prime: inverse, limit; per block: first p^2 */
static uint32_t d1p[D1_NB * 8];
static int d1nb;
static void d1_init(void) {  /* after v6_sieve: the same primes, padded with tests that never pass */
    d1nb = (v6ns + 7) / 8;
    for (int k = 0; k < d1nb * 8; k++) {
        if (k < v6ns) { d1p[k] = (uint32_t)v6sp[k]; d1i[k] = v6spi[k]; d1l[k] = v6spl[k]; }
        else { d1p[k] = 0; d1i[k] = 1; d1l[k] = 0; }  /* n * 1 <= 0 never holds for n >= 1 */
    }
    for (int b = 0; b < d1nb; b++) d1sq[b] = (uint64_t)d1p[8 * b] * d1p[8 * b];
}
static inline unsigned d1_block(uint64_t n, int b) {  /* bit j: prime 8b+j divides n (one multiply, one compare) */
    const uint64_t *I = d1i + 8 * b, *L = d1l + 8 * b;
    unsigned m = 0;
    for (int j = 0; j < 8; j++) m |= (unsigned)(n * I[j] <= L[j]) << j;
    return m;
}
#endif

/* ---------------------------------------------------------------- D3: the product forest of the large primes */

#if VARIANT == VAR_D3
#define D3_HMAX 6  /* a root holds at most 2^6 = 64 leaves (products of a few dozen limbs) */
typedef struct { uint32_t lo, nl; int32_t l, r; uint8_t h; } d3node;
static d3node *d3n;
static size_t d3nn, d3ncap;
static uint64_t *d3limb;
static size_t d3nlimb, d3lcap;
static int32_t *d3root;
static size_t d3nroot, d3rcap;
static uint64_t st3_check, st3_hit, st3_desc, st3_leaves;

static uint32_t d3_alloc(uint32_t nl) {
    GROW(d3limb, d3nlimb, d3lcap, (size_t)nl);
    uint32_t lo = (uint32_t)d3nlimb;
    d3nlimb += nl;
    return lo;
}
static int32_t d3_newnode(uint32_t lo, uint32_t nl, int32_t l, int32_t r, uint8_t h) {
    GROW(d3n, d3nn, d3ncap, 1);
    d3n[d3nn] = (d3node){lo, nl, l, r, h};
    return (int32_t)d3nn++;
}
static int32_t d3_mul(int32_t a, int32_t b) {  /* a new node: the product of a and b (schoolbook) */
    uint32_t na = d3n[a].nl, nb = d3n[b].nl, nl = na + nb, lo = d3_alloc(nl);
    uint64_t *R = d3limb + lo;
    const uint64_t *A = d3limb + d3n[a].lo, *B = d3limb + d3n[b].lo;  /* after d3_alloc: the pool may have moved */
    memset(R, 0, sizeof(uint64_t) * nl);
    for (uint32_t i = 0; i < na; i++) {
        uint64_t c = 0;
        for (uint32_t j = 0; j < nb; j++) {
            u128 t = (u128)A[i] * B[j] + R[i + j] + c;
            R[i + j] = (uint64_t)t;
            c = (uint64_t)(t >> 64);
        }
        R[i + nb] = c;
    }
    while (nl > 1 && !R[nl - 1]) nl--;
    d3nlimb = lo + nl;
    return d3_newnode(lo, nl, a, b, (uint8_t)(d3n[a].h + 1));
}
static void d3_leaf(uint64_t p) {  /* a new large prime in the dictionary: a leaf, merged with equal-height roots */
    st3_leaves++;
    uint32_t lo = d3_alloc(1);
    d3limb[lo] = p;
    int32_t x = d3_newnode(lo, 1, -1, -1, 0);
    GROW(d3root, d3nroot, d3rcap, 1);
    d3root[d3nroot++] = x;
    while (d3nroot >= 2 && d3n[d3root[d3nroot - 1]].h == d3n[d3root[d3nroot - 2]].h &&
           d3n[d3root[d3nroot - 1]].h < D3_HMAX) {
        int32_t m = d3_mul(d3root[d3nroot - 2], d3root[d3nroot - 1]);
        d3nroot--;
        d3root[d3nroot - 1] = m;
    }
}
/* Remainders modulo d = c << s (d normalized) with a precomputed reciprocal v (Moller-Granlund): (u1:u0) mod d for
 * u1 < d.  Any multiple of d is a multiple of c, so a residue mod d gives the residue mod c by one more division. */
static inline uint64_t d3_rem(uint64_t u1, uint64_t u0, uint64_t d, uint64_t v) {
    u128 q = (u128)v * u1 + (((u128)u1 << 64) | u0);
    uint64_t q1 = (uint64_t)(q >> 64) + 1, q0 = (uint64_t)q, r = u0 - q1 * d;
    if (r > q0) r += d;
    if (r >= d) r -= d;
    return r;
}
static inline uint64_t d3_mod(int32_t x, uint64_t d, uint64_t v) {
    const uint64_t *L = d3limb + d3n[x].lo;
    uint64_t r = 0;
    for (uint32_t i = d3n[x].nl; i-- > 0;) r = d3_rem(r, L[i], d, v);
    return r;
}
static int d3_shares(uint64_t r, uint64_t c) { return gcd64(r % c, c) != 1; }
static void d3_descend(int32_t x, uint64_t c, uint64_t d, uint64_t v, uint64_t *out, int *k) {
    st3_desc++;
    if (d3n[x].l < 0) { out[(*k)++] = d3limb[d3n[x].lo]; return; }  /* a leaf: a prime that divides c */
    if (d3_shares(d3_mod(d3n[x].l, d, v), c)) d3_descend(d3n[x].l, c, d, v, out, k);
    if (d3_shares(d3_mod(d3n[x].r, d, v), c)) d3_descend(d3n[x].r, c, d, v, out, k);
}
/* The known large primes dividing c (c >= 2^32, no factor below 2^16), in insertion order: one remainder of the whole
 * product (each root mod d, multiplied together mod d), one gcd; on a hit, the roots and subtrees that share. */
static int d3_check(uint64_t c, uint64_t *out) {
    if (!d3nroot) return 0;
    st3_check++;
    int s = __builtin_clzll(c), k = 0;
    uint64_t d = c << s, v = (uint64_t)(~(u128)0 / d - ((u128)1 << 64)), acc = 1 % d;
    uint64_t res[256], *rs = d3nroot <= 256 ? res : xmalloc(d3nroot * sizeof *rs);
    for (size_t j = 0; j < d3nroot; j++) {
        rs[j] = d3_mod(d3root[j], d, v);
        u128 t = (u128)acc * rs[j];
        acc = d3_rem((uint64_t)(t >> 64), (uint64_t)t, d, v);
    }
    if (d3_shares(acc, c)) {
        st3_hit++;
        for (size_t j = 0; j < d3nroot; j++)
            if (d3_shares(rs[j], c)) d3_descend(d3root[j], c, d, v, out, &k);
    }
    if (rs != res) xfree(rs);
    return k;
}
#endif

/* ---------------------------------------------------------------- D2: leftovers, batch gcd in the idle pass */

#if VARIANT == VAR_D2
#include <gmp.h>
#define D2_TAG (1ull << 63)  /* a ddpp entry naming a leftover (its index), not a prime */
static uint64_t *d2v;        /* the distinct composite leftovers of this entry, in entry order */
static size_t d2n, d2cap;
static uint64_t *d2hk;       /* leftover value -> index + 1 (open addressing; 0 empty) */
static uint32_t *d2hv;
static size_t d2hcap;
static uint64_t *d2pp;       /* after the idle pass: each leftover's primes */
static uint32_t *d2po;
static uint8_t *d2pc;
static size_t d2npp, d2ppcap, d2pocap;
static uint64_t st2_left, st2_dup, st2_shared, st2_rho, st2_idle;

static uint32_t d2_add(uint64_t c) {  /* entry: append a composite leftover once (hash), return its index */
    if (2 * (d2n + 1) > d2hcap) {
        size_t oc = d2hcap;
        uint64_t *ok = d2hk;
        uint32_t *ov = d2hv;
        d2hcap = d2hcap ? 2 * d2hcap : 1024;
        d2hk = xcalloc(d2hcap * sizeof *d2hk);
        d2hv = xmalloc(d2hcap * sizeof *d2hv);
        for (size_t j = 0; j < oc; j++)
            if (ok[j]) {
                size_t g = hsh(ok[j]) & (d2hcap - 1);
                while (d2hk[g]) g = (g + 1) & (d2hcap - 1);
                d2hk[g] = ok[j]; d2hv[g] = ov[j];
            }
        xfree(ok); xfree(ov);
    }
    size_t h = hsh(c) & (d2hcap - 1);
    while (d2hk[h]) {
        if (d2hk[h] == c) { st2_dup++; return d2hv[h]; }
        h = (h + 1) & (d2hcap - 1);
    }
    GROW(d2v, d2n, d2cap, 1);
    d2hk[h] = c; d2hv[h] = (uint32_t)d2n;
    d2v[d2n] = c;
    st2_left++;
    return (uint32_t)d2n++;
}

/* g[i] = gcd(L[i], product of the other leftovers): a product tree, then the remainder tree modulo the squares
 * (Bernstein's batch gcd; FactHacks' form): g[i] = gcd((P mod L[i]^2) / L[i], L[i]).  The leftovers are distinct. */
static void d2_batch_gcd(const uint64_t *L, size_t m, uint64_t *g) {
    if (m == 1) { g[0] = 1; return; }
    mpz_t *lev[64], *R, *Rn, sq, t;
    size_t sz[64], nl = 1;
    lev[0] = malloc(m * sizeof(mpz_t));
    for (size_t i = 0; i < m; i++) mpz_init_set_ui(lev[0][i], L[i]);
    sz[0] = m;
    while (sz[nl - 1] > 1) {
        size_t n = sz[nl - 1], k = (n + 1) / 2;
        lev[nl] = malloc(k * sizeof(mpz_t));
        for (size_t j = 0; j < k; j++) {
            mpz_init(lev[nl][j]);
            if (2 * j + 1 < n) mpz_mul(lev[nl][j], lev[nl - 1][2 * j], lev[nl - 1][2 * j + 1]);
            else mpz_set(lev[nl][j], lev[nl - 1][2 * j]);
        }
        sz[nl++] = k;
    }
    mpz_init(sq); mpz_init(t);
    R = malloc(sizeof(mpz_t));
    mpz_init_set(R[0], lev[nl - 1][0]);
    for (size_t l = nl - 1; l-- > 0;) {
        Rn = malloc(sz[l] * sizeof(mpz_t));
        for (size_t i = 0; i < sz[l]; i++) {
            mpz_init(Rn[i]);
            mpz_mul(sq, lev[l][i], lev[l][i]);
            mpz_mod(Rn[i], R[i / 2], sq);
        }
        for (size_t i = 0; i < sz[l + 1]; i++) mpz_clear(R[i]);
        free(R);
        R = Rn;
    }
    for (size_t i = 0; i < m; i++) {
        mpz_divexact_ui(t, R[i], L[i]);
        g[i] = mpz_gcd_ui(NULL, t, L[i]);
        mpz_clear(R[i]);
    }
    free(R);
    for (size_t l = 0; l < nl; l++) {
        for (size_t i = 0; i < sz[l]; i++) mpz_clear(lev[l][i]);
        free(lev[l]);
    }
    mpz_clear(sq); mpz_clear(t);
}

/* D0's dictionary rule for one set of primes first seen together (ab_v6.c's v6_enter_den, its last loop). */
static void d2_merge(const uint64_t *pr, const uint32_t *ex, int np) {
    uint64_t nv = 1, newp[AB_MAXE];
    int nn = 0;
    for (int j = 0; j < np; j++) {
        uint64_t p = pr[j];
        uint32_t h = dict_get(p);
        if (h == V6_NONE) {
            newp[nn++] = p;
            for (uint32_t e = 0; e < ex[j]; e++) nv *= p;
        } else if (!nodes[h].prime) {
            v6_split_prime(h, p);
        }
    }
    if (nn) { uint32_t e1; v6_part_node(nv, newp, nn, &e1); }
}

/* The idle pass: batch gcd over the leftovers; each leftover's pieces (gcd and cofactor if it shares, else itself)
 * that are still composite are divided by the large primes known so far (D0's step 1b loop; primes merged earlier in
 * this pass count), and only what is still composite goes to rho (pg_factor, D0's code); the leftover's primes are
 * merged through the dictionary, in entry order.  Returns the number of leftovers. */
static uint32_t d2_idle(void) {
    size_t m = d2n;
    if (!m) return 0;
    uint64_t *g = xmalloc(m * sizeof *g);
    d2_batch_gcd(d2v, m, g);
    if (m > d2pocap) {
        d2pocap = m;
        d2po = xrealloc(d2po, m * sizeof *d2po);
        d2pc = xrealloc(d2pc, m);
    }
    for (size_t i = 0; i < m; i++) {
        uint64_t L = d2v[i], piece[2], pr[64];
        uint32_t ex[64];
        int npc = 0, np = 0;
        if (g[i] > 1 && g[i] < L) { st2_shared++; piece[npc++] = g[i]; piece[npc++] = L / g[i]; }
        else { st2_shared += g[i] > 1; piece[npc++] = L; }
        for (int q = 0; q < npc; q++) {
            uint64_t x = piece[q], fp[64];
            uint32_t fe[64];
            int k;
            if (x == 1) continue;
            k = 0;
            if (!pg_is_prime(x)) {  /* still composite: the large primes known so far (D0's step 1b loop) */
                for (size_t j = 0; j < nbig && x >= (1ull << 32); j++) {
                    uint64_t q = x * bigi[j];
                    if (q > bigl[j]) continue;
                    do { x = q; q = x * bigi[j]; } while (q <= bigl[j]);
                    fp[k++] = bigp[j];
                }
            }
            if (x > 1) {  /* below 2^32 x is prime (no factor below 2^16) */
                if (x < (1ull << 32) || pg_is_prime(x)) fp[k++] = x;
                else { st2_rho++; k += pg_factor(x, fp + k, fe + k); }
            }
            for (int j = 0; j < k; j++) {
                int dup = 0;
                for (int t = 0; t < np && !dup; t++) dup = pr[t] == fp[j];
                if (!dup) pr[np++] = fp[j];
            }
        }
        for (int j = 0; j < np; j++) {  /* the exponents in L (the pieces may share primes) */
            uint64_t x = L;
            ex[j] = 0;
            while (x % pr[j] == 0) { x /= pr[j]; ex[j]++; }
        }
        GROW(d2pp, d2npp, d2ppcap, (size_t)np);
        d2po[i] = (uint32_t)d2npp; d2pc[i] = (uint8_t)np;
        for (int j = 0; j < np; j++) d2pp[d2npp++] = pr[j];
        d2_merge(pr, ex, np);
    }
    xfree(g);
    st2_idle += m;
    return (uint32_t)m;
}

static void v6_den_edges(uint32_t k) {  /* pass 2 (after the idle pass): primes by lookup, leftovers by their primes */
    v6_handed = 1;
    dden *D = &dd[k];
    uint32_t cand[AB_W];
    int nc = 0;
    for (uint32_t j = 0; j < ddpc[k]; j++) {
        uint64_t x = ddpp[ddpo[k] + j];
        uint32_t n1 = 1, li = 0;
        const uint64_t *ps = &x;
        if (x & D2_TAG) { li = (uint32_t)(x & ~D2_TAG); ps = d2pp + d2po[li]; n1 = d2pc[li]; }
        for (uint32_t t = 0; t < n1; t++) {
            uint32_t id = dict_get(ps[t]);
            if (id == V6_NONE) die("v6: prime not in the dictionary");
            int dup = 0;
            for (int u = 0; u < nc && !dup; u++) dup = cand[u] == id;
            if (!dup) {
                if (nc == AB_W) die("d2: too many candidates");
                cand[nc++] = id;
            }
        }
    }
    if (nc > 1) qsort(cand, (size_t)nc, sizeof *cand, cmp_u32);
    uint64_t left = D->den, q;
    int ne = 0;
    for (int t = 0; t < nc; t++) {
        int c = 0;
        while (ndiv(left, &nodes[cand[t]], &q)) { left = q; c++; }
        if (c) {
            if (ne == AB_MAXE) die("too many edges");
            D->e[ne++] = EP(cand[t], c);
        }
    }
    if (left != 1) die("v6: unresolved denominator");
    D->ne = (uint8_t)ne;
}
#endif

/* ---------------------------------------------------------------- entry: D1-D3 (D0 is ab_v6.c's, unchanged) */

#if VARIANT != VAR_D0
/* ab_v6.c's v6_enter_den with one change per row, marked D1 / D2 / D3; everything else is the same code. */
static void v6_enter_den(uint32_t k, uint64_t den) {
    if (k >= dd6cap) {
        while (k >= dd6cap) dd6cap = dd6cap ? 2 * dd6cap : 256;
        ddpo = xrealloc(ddpo, dd6cap * sizeof *ddpo);
        ddpc = xrealloc(ddpc, dd6cap * sizeof *ddpc);
    }
    ddpo[k] = (uint32_t)nddpp; ddpc[k] = 0;
    if (den <= 1) return;
    uint64_t pr[64], n = den;
    uint32_t ex[64];
    int np = 0, s = __builtin_ctzll(n), stopped = 0;
    if (s) { pr[np] = 2; ex[np++] = (uint32_t)s; n >>= s; }
#if VARIANT == VAR_D1  /* step 1 in branch-free blocks of 8 (the same test per prime), the stop once per block */
    for (int b = 0; b < d1nb; b++) {
        if (n < d1sq[b]) { stopped = 1; break; }
        unsigned m = d1_block(n, b);
        while (m) {  /* the block's tests were on the n it started with: each hit divides that n, so also this one */
            int j = __builtin_ctz(m), t = 8 * b + j;
            m &= m - 1;
            uint64_t q = n * d1i[t];
            uint32_t e = 0;
            do { n = q; e++; q = n * d1i[t]; } while (q <= d1l[t]);
            pr[np] = d1p[t]; ex[np++] = e;
        }
    }
#else
    int i;
    for (i = 0; i < v6ns; i++) {  /* step 1: the table, with the adaptive stop (pg_factor's stage 2) */
        uint64_t p = v6sp[i];
        if (p * p > n) break;
        uint64_t q = n * v6spi[i];
        if (q <= v6spl[i]) {
            uint32_t e = 0;
            do { n = q; e++; q = n * v6spi[i]; } while (q <= v6spl[i]);
            pr[np] = p; ex[np++] = e;
        }
    }
    stopped = i < v6ns;
#endif
    if (n > 1 && !stopped && n >= (1ull << 32)) {  /* step 1b: the large primes already seen (odd: p is odd) */
#if VARIANT == VAR_D3  /* the product check in place of the loop over every known large prime */
        uint64_t hit[64];
        int nh = d3_check(n, hit);
        for (int j = 0; j < nh; j++) {
            uint64_t pi = inv64(hit[j]), pl = UINT64_MAX / hit[j], q = n * pi;
            uint32_t e = 0;
            while (q <= pl) { n = q; e++; q = n * pi; }
            if (!e) die("d3: a hit that does not divide");
            pr[np] = hit[j]; ex[np++] = e;
        }
#else
        for (size_t j = 0; j < nbig && n >= (1ull << 32); j++) {
            uint64_t q = n * bigi[j];
            if (q > bigl[j]) continue;
            uint32_t e = 0;
            do { n = q; e++; q = n * bigi[j]; } while (q <= bigl[j]);
            pr[np] = bigp[j]; ex[np++] = e;
        }
#endif
    }
#if VARIANT == VAR_D2
    uint64_t c = 0;  /* D2: a composite large leftover, appended for the idle pass */
#endif
    if (n > 1) {  /* no factor below 2^16 is left, so below 2^32 n is prime */
        if (stopped || n < (1ull << 32) || pg_is_prime(n)) { pr[np] = n; ex[np++] = 1; }
#if VARIANT == VAR_D2
        else c = n;  /* no rho at entry */
#else
        else {  /* step 4: rho, only on a composite no known prime divides */
            uint64_t fp[64];
            uint32_t fe[64];
            int nf = pg_factor(n, fp, fe);
            for (int j = 0; j < nf; j++) { pr[np] = fp[j]; ex[np++] = fe[j]; }
        }
#endif
    }
    GROW(ddpp, nddpp, ddppcap, (size_t)np);
    for (int j = 0; j < np; j++) ddpp[nddpp++] = pr[j];
    ddpc[k] = (uint8_t)np;
    uint64_t nv = 1, newp[AB_MAXE];
    int nn = 0;
    for (int j = 0; j < np; j++) {
        uint64_t p = pr[j];
        uint32_t h = dict_get(p);
        if (h == V6_NONE) {  /* first sighting: joins this denominator's remaining part */
            newp[nn++] = p;
            for (uint32_t e = 0; e < ex[j]; e++) nv *= p;
        } else if (!nodes[h].prime) {
            v6_split_prime(h, p);  /* already seen, in a holder with other primes: split it out */
        }
    }
    if (nn) { uint32_t e1; v6_part_node(nv, newp, nn, &e1); }
#if VARIANT == VAR_D2  /* the leftover's index follows the primes; its primes are known after the idle pass */
    if (c) {
        GROW(ddpp, nddpp, ddppcap, 1);
        ddpp[nddpp++] = D2_TAG | d2_add(c);
        ddpc[k] = (uint8_t)(np + 1);
    }
#endif
}
#endif

/* ---------------------------------------------------------------- per rep, stats */

static void d_reset(void) {  /* per rep, untimed (with v6_reset) */
    st_dfresh = 0;
#if VARIANT == VAR_D1
    if (!d1nb) d1_init();
#endif
#if VARIANT == VAR_D2
    d2n = 0; d2npp = 0; st2_left = 0; st2_dup = 0; st2_shared = 0; st2_rho = 0; st2_idle = 0;
    if (d2hk) memset(d2hk, 0, d2hcap * sizeof *d2hk);
#endif
#if VARIANT == VAR_D3
    d3nn = 0; d3nlimb = 0; d3nroot = 0; st3_check = 0; st3_hit = 0; st3_desc = 0; st3_leaves = 0;
#endif
}
static void d_stats(FILE *f) {
    fprintf(f, " d_fresh=%llu", (unsigned long long)st_dfresh);
#if VARIANT == VAR_D2
    fprintf(f, " d2_leftovers=%llu d2_dups=%llu d2_shared=%llu d2_rho=%llu d2_idle=%llu",
            (unsigned long long)st2_left, (unsigned long long)st2_dup, (unsigned long long)st2_shared,
            (unsigned long long)st2_rho, (unsigned long long)st2_idle);
#endif
#if VARIANT == VAR_D3
    fprintf(f, " d3_checks=%llu d3_hits=%llu d3_descents=%llu d3_leaves=%llu d3_limbs=%zu d3_roots=%zu",
            (unsigned long long)st3_check, (unsigned long long)st3_hit, (unsigned long long)st3_desc,
            (unsigned long long)st3_leaves, d3nlimb, d3nroot);
#endif
}
