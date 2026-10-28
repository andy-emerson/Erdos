/* ab_d.c: round 3's dictionary rows on V3's layout (round3_bench.py).  #included after ab_v3.c and ab_v6.c when
 * VARIANT >= VAR_D0.
 *
 *   D0  V3 + V6's seen-prime dictionary entry as it stands (ab_v6.c's v6_enter_den and v6_den_edges, unchanged).
 *   D1  D0 + a faster small-prime test: the table of odd primes below 2^16 is tested in blocks of 8, each prime by
 *       one multiply and one compare (the exact-division test, branch-free, so the block vectorises), with the
 *       adaptive stop checked once per block (n below the square of the block's first prime); a composite large
 *       leftover goes to Miller-Rabin and rho without repeating the table (D0 calls pg_factor, which runs it again).
 *       GMP-style packing (n mod a product of small primes, then the tests on the residue) was tried for the primes
 *       3..53 and did not help on one-word denominators (see round3_bench.py's notes), so it is not used.
 *   D2  D0 + no Miller-Rabin or rho at entry: a large leftover (at least 2^32 after the table and the known large
 *       primes) enters as a leftover node, not factored, kept coprime to the other leftover nodes by gcd refinement
 *       (one gcd per live leftover node), and every new large prime is tested against the leftover nodes.  The
 *       leftovers are factored in the idle pass (d2_idle), run between entry's pass 1 and pass 2 in the decomposition
 *       slot of the driver: timed, reported apart from entry, included in the total.
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

/* ---------------------------------------------------------------- D1: the small-prime table in blocks of 8 */

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
/* pg.c's Miller-Rabin / Pollard-Brent stages for a leftover with no factor below 2^16 (pg.c keeps rho static; this
 * is the same code), so D1 does not run the table a second time as pg_factor would. */
static inline uint64_t d1_mulmod(uint64_t a, uint64_t b, uint64_t n) { return (uint64_t)((u128)a * b % n); }
static uint64_t d1_rho(uint64_t n) {
    for (uint64_t c = 1;; c++) {
        uint64_t y = 2, x = 2, ys = 2, q = 1, g = 1, r = 1;
        const uint64_t m = 128;
#define F(z) ((uint64_t)(((u128)d1_mulmod((z), (z), n) + c) % n))
        do {
            x = y;
            for (uint64_t i = 0; i < r; i++) y = F(y);
            uint64_t k = 0;
            do {
                ys = y;
                uint64_t lim = m < r - k ? m : r - k;
                for (uint64_t i = 0; i < lim; i++) {
                    y = F(y);
                    q = d1_mulmod(q, x > y ? x - y : y - x, n);
                }
                g = gcd64(q, n);
                k += m;
            } while (k < r && g == 1);
            r *= 2;
        } while (g == 1);
        if (g == n) {
            do {
                ys = F(ys);
                g = gcd64(x > ys ? x - ys : ys - x, n);
            } while (g == 1);
        }
#undef F
        if (g != n) return g;
    }
}
static void d1_factor_big(uint64_t n, uint64_t *out, int *k) {
    if (n == 1) return;
    if (pg_is_prime(n)) { out[(*k)++] = n; return; }
    uint64_t d = d1_rho(n);
    d1_factor_big(d, out, k);
    d1_factor_big(n / d, out, k);
}
static int d1_factor_leftover(uint64_t n, uint64_t *P, uint32_t *E) {  /* primes ascending, as pg_factor */
    uint64_t f[64];
    int nf = 0, k = 0;
    d1_factor_big(n, f, &nf);
    for (int a = 1; a < nf; a++)
        for (int b = a; b > 0 && f[b - 1] > f[b]; b--) { uint64_t t = f[b]; f[b] = f[b - 1]; f[b - 1] = t; }
    for (int a = 0; a < nf;) {
        int b = a;
        while (b < nf && f[b] == f[a]) b++;
        P[k] = f[a]; E[k++] = (uint32_t)(b - a);
        a = b;
    }
    return k;
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

/* ---------------------------------------------------------------- D2: leftover nodes and the idle pass */

#if VARIANT == VAR_D2
#define D2_TAG (1ull << 63)  /* a ddpp entry naming a node id (a leftover or one of its parts), not a prime */
static uint32_t *d2lo;       /* live leftover nodes (not factored), compacted lazily */
static size_t d2nlo, d2locap;
static uint64_t st2_left, st2_gcd, st2_idle, st2_split;

static void d2_meta(uint32_t id) {  /* metadata for a node created without v6_node: no known primes yet */
    if (id >= vmcap) {
        while (id >= vmcap) vmcap = vmcap ? 2 * vmcap : 256;
        vm = xrealloc(vm, vmcap * sizeof *vm);
    }
    vm[id].fc = 0; vm[id].fo = 0; vm[id].pc = 0; vm[id].po = (uint32_t)v6npp;
}
static uint32_t d2_leftover_node(uint64_t x) {  /* x >= 2^32 with no factor below 2^16 and no known prime */
    uint32_t id = node_new(x, 0);
    d2_meta(id);
    v6_nonprime++;
    v6_upd();
    GROW(d2lo, d2nlo, d2locap, 1);
    d2lo[d2nlo++] = id;
    st2_left++;
    return id;
}
/* A part of a leftover: below 2^32 it is prime (it has no factor below 2^16), a new prime for the dictionary; else
 * it is another leftover node. */
static uint32_t d2_part(uint64_t x) { return x < (1ull << 32) ? v6_node(x, &x, 1) : d2_leftover_node(x); }

/* A first-sighted large prime p (from this denominator) dividing a live leftover splits it: p becomes its own node
 * (so the leftover's parts are pairwise coprime nodes), the rest a part.  Leftovers are pairwise coprime, so at most
 * one leftover holds p. */
static void d2_prime_vs_leftovers(uint64_t p) {
    uint64_t pi = inv64(p), pl = UINT64_MAX / p;
    for (size_t j = 0; j < d2nlo; j++) {
        uint32_t L = d2lo[j];
        if (!nodes[L].alive) continue;
        uint64_t v = nodes[L].v, q = v * pi;
        if (q > pl) continue;
        uint32_t e = 0;
        do { v = q; e++; q = v * pi; } while (q <= pl);
        uint32_t parts[2];
        int np = 0;
        parts[np++] = EP(v6_node(p, &p, 1), e);
        if (v > 1) parts[np++] = EP(d2_part(v), 1);
        v6_kill(L, parts, np);
        st2_split++;
        return;
    }
}
/* The large leftover c of a denominator: gcd-refined against every live leftover node (V0' refine, over leftovers
 * only); the node ids of c's parts are written to ids. */
static int d2_refine(uint64_t c, uint64_t *ids) {
    uint64_t left = c;
    int nid = 0;
    size_t n0 = d2nlo;
    for (size_t j = 0; j < n0 && left > 1; j++) {
        uint32_t L = d2lo[j];
        if (!nodes[L].alive) continue;
        uint64_t v = nodes[L].v;
        st2_gcd++;
        if (gcd64(left, v) == 1) continue;
        uint64_t base[64], pv[64];
        uint32_t pe[64];
        int nb = coprime_base2(v, left, base), np = 0;
        for (int b = 0; b < nb; b++) {
            uint32_t e = 0;
            for (uint64_t t = v; t % base[b] == 0; t /= base[b]) e++;
            if (e) { pv[np] = base[b]; pe[np++] = e; }
        }
        if (np == 1 && pe[0] == 1) {  /* L itself is a common part: it divides c */
            ids[nid++] = D2_TAG | L;
            while (left % v == 0) left /= v;
            continue;
        }
        uint32_t parts[64], pid[64];
        for (int q = 0; q < np; q++) { pid[q] = d2_part(pv[q]); parts[q] = EP(pid[q], pe[q]); }
        v6_kill(L, parts, np);
        st2_split++;
        for (int q = 0; q < np; q++) {
            if (left % pv[q]) continue;
            ids[nid++] = D2_TAG | pid[q];
            while (left % pv[q] == 0) left /= pv[q];
        }
    }
    if (left > 1) ids[nid++] = D2_TAG | d2_part(left);
    return nid;
}
static void d2_collect(uint32_t x, uint32_t *cand, int *nc) {  /* a node id -> the live nodes it now consists of */
    if (nodes[x].alive) {
        for (int t = 0; t < *nc; t++) if (cand[t] == x) return;
        if (*nc == AB_W) die("d2: too many candidates");
        cand[(*nc)++] = x;
        return;
    }
    for (uint32_t i = 0; i < vm[x].fc; i++) d2_collect(EN(v6fw[vm[x].fo + i]), cand, nc);
}
static void v6_den_edges(uint32_t k) {  /* pass 2 (after the idle pass): primes by lookup, leftover ids expanded */
    v6_handed = 1;
    dden *D = &dd[k];
    uint32_t cand[AB_W];
    int nc = 0;
    for (uint32_t j = 0; j < ddpc[k]; j++) {
        uint64_t x = ddpp[ddpo[k] + j];
        if (x & D2_TAG) { d2_collect((uint32_t)(x & ~D2_TAG), cand, &nc); continue; }
        uint32_t id = dict_get(x);
        if (id == V6_NONE) die("v6: prime not in the dictionary");
        int dup = 0;
        for (int t = 0; t < nc && !dup; t++) dup = cand[t] == id;
        if (!dup) cand[nc++] = id;
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
/* The idle pass: every live leftover node factored (Miller-Rabin, then pg_factor's rho, as D0 does at entry).  A prime
 * leftover becomes a prime node; p^e becomes node p (weight e); several primes stay one node holding them all (V6's
 * rule: primes first seen together stay together).  Returns the number of leftovers factored. */
static uint32_t d2_idle(void) {
    uint32_t n = 0;
    for (size_t j = 0; j < d2nlo; j++) {
        uint32_t L = d2lo[j];
        if (!nodes[L].alive) continue;
        n++;
        uint64_t x = nodes[L].v, fp[64];
        uint32_t fe[64];
        int k;
        if (pg_is_prime(x)) { fp[0] = x; fe[0] = 1; k = 1; }
        else k = pg_factor(x, fp, fe);
        if (k == 1 && fe[0] > 1) {  /* p^e: the node becomes p */
            uint32_t part = EP(v6_node(fp[0], fp, 1), fe[0]);
            v6_kill(L, &part, 1);
            continue;
        }
        v6meta *M = &vm[L];
        GROW(v6pp, v6npp, v6ppcap, (size_t)k);
        GROW(v6ppi, v6npp, v6ppicap, (size_t)k);
        GROW(v6ppl, v6npp, v6pplcap, (size_t)k);
        M->po = (uint32_t)v6npp; M->pc = (uint8_t)k;
        for (int i = 0; i < k; i++) {
            v6ppi[v6npp] = inv64(fp[i]);  /* odd: no factor below 2^16 */
            v6ppl[v6npp] = UINT64_MAX / fp[i];
            v6pp[v6npp++] = fp[i];
            dict_set(fp[i], L);
        }
        if (k == 1) { nodes[L].prime = 1; v6_nonprime--; }
    }
    d2nlo = 0;
    st2_idle += n;
    v6_upd();
    return n;
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
#if VARIANT == VAR_D1  /* step 1 in blocks of 8: one multiply and one compare per prime, the stop once per block */
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
    uint64_t c = 0;  /* D2: a large leftover, not factored at entry */
#endif
    if (n > 1) {  /* no factor below 2^16 is left, so below 2^32 n is prime */
        if (stopped || n < (1ull << 32)) { pr[np] = n; ex[np++] = 1; }
#if VARIANT == VAR_D2
        else c = n;
#else
        else if (pg_is_prime(n)) { pr[np] = n; ex[np++] = 1; }
        else {  /* step 4: rho, only on a composite no known prime divides */
            uint64_t fp[64];
            uint32_t fe[64];
#if VARIANT == VAR_D1
            int nf = d1_factor_leftover(n, fp, fe);  /* without pg_factor's second pass over the table */
#else
            int nf = pg_factor(n, fp, fe);
#endif
            for (int j = 0; j < nf; j++) { pr[np] = fp[j]; ex[np++] = fe[j]; }
        }
#endif
    }
#if VARIANT == VAR_D2  /* a first-sighted large prime may divide a leftover node: split it out first */
    if (d2nlo)
        for (int j = 0; j < np; j++)
            if (pr[j] >= 65536 && dict_get(pr[j]) == V6_NONE) d2_prime_vs_leftovers(pr[j]);
#endif
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
#if VARIANT == VAR_D2  /* the leftover: refined against the leftover nodes; its parts' ids follow the primes */
    if (c) {
        uint64_t ids[64];
        int ni = d2_refine(c, ids);
        GROW(ddpp, nddpp, ddppcap, (size_t)ni);
        for (int j = 0; j < ni; j++) ddpp[nddpp++] = ids[j];
        ddpc[k] = (uint8_t)(np + ni);
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
    d2nlo = 0; st2_left = 0; st2_gcd = 0; st2_idle = 0; st2_split = 0;
#endif
#if VARIANT == VAR_D3
    d3nn = 0; d3nlimb = 0; d3nroot = 0; st3_check = 0; st3_hit = 0; st3_desc = 0; st3_leaves = 0;
#endif
}
static void d_stats(FILE *f) {
    fprintf(f, " d_fresh=%llu", (unsigned long long)st_dfresh);
#if VARIANT == VAR_D2
    fprintf(f, " d2_leftovers=%llu d2_gcds=%llu d2_leftover_splits=%llu d2_idle_factored=%llu",
            (unsigned long long)st2_left, (unsigned long long)st2_gcd, (unsigned long long)st2_split,
            (unsigned long long)st2_idle);
#endif
#if VARIANT == VAR_D3
    fprintf(f, " d3_checks=%llu d3_hits=%llu d3_descents=%llu d3_leaves=%llu d3_limbs=%zu d3_roots=%zu",
            (unsigned long long)st3_check, (unsigned long long)st3_hit, (unsigned long long)st3_desc,
            (unsigned long long)st3_leaves, d3nlimb, d3nroot);
#endif
}
