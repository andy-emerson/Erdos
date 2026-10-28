/* g_s.c: S, the graph with every scalability requirement of research/idea.md ("Scalability requirements").
 * #included by scale.c after s_core.c when VAR_S is defined.
 *
 * Nodes.  Each live node holds a list of ATOMS: pairwise-coprime pieces, each a prime (k 0), a leftover whose
 * factors are unknown (k 1, only in lazy mode) or a pending leftover not yet checked by the idle pass (k 2).  The node
 * is the product of its atoms' powers, the exponents having gcd 1 (so a single-atom node is the atom itself and the
 * power goes on the edge weight).  The seen-prime dictionary maps each atom to the node holding it: a direct array
 * below 2^16, an open-addressing hash above (with deletion, since nodes are freed).
 *
 * Two-sided edges.  A holder (a numerator: s_core.c) keeps its (node, weight) edges, each with its position in the
 * node's numerator list; the node keeps the list of its holders.  Linking and unlinking a holder is O(its edges).
 *
 * Reference counts.  rc counts a node's holders (it always equals the list length; the audit checks it).  When it
 * reaches 0 the node is freed: its atoms leave the dictionary, its list and atoms are freed, its id is reused.
 *
 * Entry (pass 1, per new distinct denominator d): 2 by ctz; trial division by the odd primes below 2^16 with the
 * adaptive stop; the leftover n (no factor below 2^16) is looked up in the dictionary as a whole; else it is prime if
 * the stop fired or n < 2^32; else (rho mode) the known large primes are tried, but only a bounded hot list of the 16
 * most recent ones (an unbounded scan of every known large prime grows with the graph: prior-art-3 section 4), then
 * Miller-Rabin, then rho; (lazy mode) n becomes a pending atom in its own node, for the idle pass.  Then each atom
 * of d is looked up: atoms seen for the first time go together into one new node; for each node h already holding
 * some of d's atoms, h's atoms are grouped by the ratio (exponent in d) / (exponent in h), absent atoms having ratio
 * 0, and h is split until one class remains (the coarsest correct graph: 30 then 45 gives 2, 3, 5; 30 then 900
 * splits nothing).  Pass 2 builds each distinct denominator's edges by dictionary lookups, as a transient holder
 * (kind 3, linked, so any later split keeps it current through the same loop as every other holder).
 *
 * Splits by copy and loop (split_out): to move a set Q of N's atoms to a new node P, N's numerator list is copied
 * as P's list, N keeps the rest R (renormalised), and one loop over the list gives each holder its edge to P at the
 * same list position, with the weights carried over: N^w = R^(w gR) P^(w gQ), gQ and gR being the gcds of the
 * exponents moved out of each part.  No history is kept.
 *
 * Ops: when a node with several atoms (or a leftover) shares part of a numerator, it is split into its atoms (or the
 * leftover refined by gcd), and the op restarts on fresh views (s_partial, the hook in s_core.c's fin and core_mul).
 *
 * The idle pass (lazy mode): the pending atoms X are checked against the settled large atoms K by a remainder tree
 * (Z_K mod x for every x, Z_K the product of K) and against each other by a batch gcd (s_core.c).  The hits are
 * resolved by descending the product trees, refined by a coprime base per connected component, and the nodes
 * sharing a base element are split (copy and loop) and merged (one loop over the absorbed node's list).
 */

static int s_lazy;            /* 1: no Miller-Rabin or rho at entry; leftovers wait for the idle pass */
static uint32_t *dsm;         /* atoms below 2^16: node + 1 (0: none) */
static uint64_t *dbk;         /* atoms above: open addressing, key 0 empty */
static uint32_t *dbv;
static size_t dbcap, dbcnt;
static uint32_t *nfree;       /* free node ids */
static uint32_t nfree_n, nfree_cap;
static uint32_t *pq;          /* nodes holding a pending atom */
static uint32_t npq, pqcap;
static uint64_t n_multi, n_unk;  /* live nodes with several atoms; live atoms not known prime */
static uint64_t hot[16];
static int nhot, hotpos;
static int s_phase;           /* 0 entry, 1 ops, 2 idle: attributes splits */
static uint32_t corrupt_result;
static uint64_t st_split[3], st_split_num, st_split_ns, st_split_max_ns, st_merge, st_refine, st_restart;
static uint64_t st_node_new, st_node_freed, st_idle_ns, st_idle_hits, st_idle_x, st_idle_k;
static atom_t *dat;           /* pass 1: each distinct denominator's atoms (pool), for pass 2 */
static uint32_t *dato;
static uint8_t *datc;
static size_t ndat, datcap, datocap;

/* ---------------------------------------------------------------- the dictionary */

static inline uint32_t dict_get(uint64_t a) {
    if (a < 65536) return dsm[a] - 1;
    size_t i = hsh(a) & (dbcap - 1);
    while (dbk[i]) {
        if (dbk[i] == a) return dbv[i];
        i = (i + 1) & (dbcap - 1);
    }
    return NONE;
}
static void dict_set(uint64_t a, uint32_t id) {
    if (a < 65536) { dsm[a] = id + 1; return; }
    size_t i = hsh(a) & (dbcap - 1);
    while (dbk[i] && dbk[i] != a) i = (i + 1) & (dbcap - 1);
    if (dbk[i]) { dbv[i] = id; return; }
    if (2 * (dbcnt + 1) > dbcap) {
        uint64_t *ok = dbk;
        uint32_t *ov = dbv;
        size_t oc = dbcap;
        dbcap *= 2;
        dbk = xcalloc(dbcap * sizeof *dbk);
        dbv = xmalloc(dbcap * sizeof *dbv);
        for (size_t j = 0; j < oc; j++)
            if (ok[j]) {
                size_t g = hsh(ok[j]) & (dbcap - 1);
                while (dbk[g]) g = (g + 1) & (dbcap - 1);
                dbk[g] = ok[j]; dbv[g] = ov[j];
            }
        xfree(ok); xfree(ov);
        i = hsh(a) & (dbcap - 1);
        while (dbk[i]) i = (i + 1) & (dbcap - 1);
    }
    dbk[i] = a; dbv[i] = id; dbcnt++;
}
static void dict_del(uint64_t a) {  /* backward-shift deletion (linear probing) */
    if (a < 65536) { dsm[a] = 0; return; }
    size_t m = dbcap - 1, i = hsh(a) & m;
    while (dbk[i] && dbk[i] != a) i = (i + 1) & m;
    if (!dbk[i]) return;
    dbk[i] = 0; dbcnt--;
    for (size_t j = i;;) {
        j = (j + 1) & m;
        if (!dbk[j]) break;
        size_t k = hsh(dbk[j]) & m;
        if (i <= j ? (i < k && k <= j) : (i < k || k <= j)) continue;
        dbk[i] = dbk[j]; dbv[i] = dbv[j]; dbk[j] = 0;
        i = j;
    }
}

/* ---------------------------------------------------------------- nodes and their atoms */

static void g_init(void) {
    sieve();
    if (!dsm) {
        dsm = xcalloc(65536 * sizeof *dsm);
        dbcap = 1024;
        dbk = xcalloc(dbcap * sizeof *dbk);
        dbv = xmalloc(dbcap * sizeof *dbv);
    }
}
static uint32_t s_node_alloc(void) {
    uint32_t id = nfree_n ? nfree[--nfree_n] : nd_slot();
    memset(&nd[id], 0, sizeof(node_t));
    nd[id].alive = 1;
    nlive++;
    st_node_new++;
    return id;
}
static uint32_t norm_atoms(atom_t *at, int na) {  /* divide the exponents by their gcd; returns it */
    uint32_t g = 0;
    for (int i = 0; i < na; i++) g = (uint32_t)gcd64(g, at[i].e);
    if (g > 1) for (int i = 0; i < na; i++) at[i].e = (uint8_t)(at[i].e / g);
    return g ? g : 1;
}
/* set a node's atoms (normalised): its value, the dictionary, the counters, the hook flag, the pending queue */
static void node_set_atoms(uint32_t id, const atom_t *at, int na) {
    node_t *N = &nd[id];
    for (int i = 0; i < N->na; i++) {
        if (N->at[i].k) n_unk--;
        if (dict_get(N->at[i].a) == id) dict_del(N->at[i].a);
    }
    if (N->na > 1) n_multi--;
    if (N->nacap < na) { N->at = xrealloc(N->at, (size_t)na * sizeof(atom_t)); N->nacap = (uint8_t)na; }
    memmove(N->at, at, (size_t)na * sizeof(atom_t));
    N->na = (uint8_t)na;
    u128 v = 1;
    int unk = 0, pend = 0;
    for (int i = 0; i < na; i++) {
        for (int j = 0; j < N->at[i].e; j++) { v *= N->at[i].a; if (v >> 64) die("node value above 2^64"); }
        dict_set(N->at[i].a, id);
        if (N->at[i].k) { n_unk++; unk = 1; }
        if (N->at[i].k == 2) pend = 1;
    }
    if (na > 1) n_multi++;
    nd_setv(id, (uint64_t)v);
    N = &nd[id];
    N->chk = (uint8_t)(na > 1 || unk);
    if (pend && !N->pend) { GROW(pq, npq, pqcap, 1); pq[npq++] = id; }
    N->pend = (uint8_t)pend;
}
static void s_node_release(uint32_t id) {  /* rc reached 0 */
    node_t *N = &nd[id];
    for (int i = 0; i < N->na; i++) {
        if (N->at[i].k) n_unk--;
        if (dict_get(N->at[i].a) == id) dict_del(N->at[i].a);
    }
    if (N->na > 1) n_multi--;
    if (corrupt_leak == 2) corrupt_leak = 0;  /* test hook: the list block is never freed (a leak) */
    else xfree(N->list);
    xfree(N->at);
    memset(N, 0, sizeof *N);
    GROW(nfree, nfree_n, nfree_cap, 1);
    nfree[nfree_n++] = id;
    nlive--;
    st_node_freed++;
}
static inline void list_push(uint32_t id, uint32_t h) {
    node_t *N = &nd[id];
    if (N->len == N->lcap) {
        N->lcap = N->lcap ? 2 * N->lcap : 4;
        N->list = xrealloc(N->list, (size_t)N->lcap * 4);
    }
    N->list[N->len++] = h;
}

/* ---------------------------------------------------------------- two-sided edges */

static void g_link(uint32_t h) {
    const holder_t *X = &H[h];
    for (int k = 0; k < X->ne; k++) {
        uint32_t id = EN(EPL[X->eo + k]);
        PPL[X->eo + k] = nd[id].len;
        list_push(id, h);
        nd[id].rc++;
    }
}
static void g_unlink(uint32_t h) {
    const holder_t *X = &H[h];
    for (int k = 0; k < X->ne; k++) {
        uint32_t id = EN(EPL[X->eo + k]), pos = PPL[X->eo + k];
        node_t *N = &nd[id];
        uint32_t last = N->list[N->len - 1];
        N->list[pos] = last;
        if (pos != N->len - 1) {
            int kk = h_find(last, id);
            if (kk < 0) die("unlink: list names a holder without the edge");
            PPL[H[last].eo + kk] = pos;
        }
        N->len--;
        N->rc--;
        if (N->lcap > 16 && N->len < N->lcap / 4) { N->lcap /= 2; N->list = xrealloc(N->list, (size_t)N->lcap * 4); }
        if (N->rc == 0) s_node_release(id);
    }
}

/* ---------------------------------------------------------------- splits: copy and loop */

/* Move the atoms flagged in mv out of node Nid into a new node P (returned). */
static uint32_t split_out(uint32_t Nid, const uint8_t *mv) {
    uint64_t t0 = now_ns();
    atom_t Q[MAXE], R[MAXE];
    int nq = 0, nr = 0;
    for (int i = 0; i < nd[Nid].na; i++) {
        if (mv[i]) Q[nq++] = nd[Nid].at[i];
        else R[nr++] = nd[Nid].at[i];
    }
    if (!nq || !nr) die("split_out: nothing to split");
    uint32_t gQ = norm_atoms(Q, nq), gR = norm_atoms(R, nr);
    uint32_t P = s_node_alloc();  /* may move nd */
    node_set_atoms(P, Q, nq);
    node_set_atoms(Nid, R, nr);
    uint32_t len = nd[Nid].len;
    /* 1. the copy: N's numerator list duplicated as P's */
    nd[P].lcap = len < 4 ? 4 : len;
    nd[P].list = xmalloc((size_t)nd[P].lcap * 4);
    if (len) memcpy(nd[P].list, nd[Nid].list, (size_t)len * 4);
    nd[P].len = len;
    nd[P].rc = nd[Nid].rc;
    /* 2. the loop: each holder gets its edge to P at the same list position; weights carried over */
    const uint32_t *L = nd[Nid].list;
    for (uint32_t i = 0; i < len; i++) {
        uint32_t h = L[i];
        int k = h_find(h, Nid);
        if (k < 0) die("split_out: list names a holder without the edge");
        uint32_t w = EW(EPL[H[h].eo + k]);
        if (gR != 1) {
            if (w * gR > 255) die("split_out: weight above 255");
            EPL[H[h].eo + k] = EP(Nid, w * gR);
        }
        h_insert_edge(h, P, w * gQ, i);
    }
    uint64_t dt = now_ns() - t0;
    st_split[s_phase]++;
    st_split_num += len;
    st_split_ns += dt;
    if (dt > st_split_max_ns) st_split_max_ns = dt;
    return P;
}
static void split_all(uint32_t id) {  /* into single atoms, one copy-and-loop split each */
    while (nd[id].na > 1) {
        uint8_t mv[MAXE] = {0};
        mv[nd[id].na - 1] = 1;
        split_out(id, mv);
    }
}
/* reset a node's atoms (not normalised): renormalise, and scale its holders' weights by the gcd (one loop) */
static void node_reatom(uint32_t id, atom_t *at, int na) {
    uint32_t g = norm_atoms(at, na);
    node_set_atoms(id, at, na);
    if (g == 1) return;
    for (uint32_t i = 0; i < nd[id].len; i++) {
        uint32_t h = nd[id].list[i];
        int k = h_find(h, id);
        uint32_t w = EW(EPL[H[h].eo + k]) * g;
        if (w > 255) die("node_reatom: weight above 255");
        EPL[H[h].eo + k] = EP(id, w);
    }
}
/* atom idx of node id (a leftover) shares g with a numerator: replace it by its coprime pieces */
static void refine_atom(uint32_t id, int idx, uint64_t g) {
    atom_t A = nd[id].at[idx], nat[64];
    int n2 = 0;
    for (int i = 0; i < nd[id].na; i++) if (i != idx) nat[n2++] = nd[id].at[i];
    uint64_t base[64], chk = 1;
    int nb = coprime_base2(A.a, g, base);
    for (int b = 0; b < nb; b++) {
        uint32_t f = vlog(A.a, base[b]);
        if (!f) continue;
        for (uint32_t j = 0; j < f; j++) chk *= base[b];
        nat[n2++] = (atom_t){base[b], (uint8_t)(A.e * f), (uint8_t)(A.k == 2 ? 2 : (is_prime(base[b]) ? 0 : 1))};
    }
    if (chk != A.a) die("refine_atom: pieces do not multiply back");
    node_reatom(id, nat, n2);
    st_refine++;
}
/* The op hook: node id (several atoms, or not prime) left a power on a denominator while sharing part of m (m is
 * not divisible by the node).  Several atoms, one of which divides m: split into atoms.  A leftover with a nontrivial
 * gcd: refine it (then split into atoms).  Returns 1 if the graph changed (the caller restarts). */
static int s_partial(uint32_t id, u128 m) {
    for (int i = 0; i < nd[id].na; i++) {
        atom_t A = nd[id].at[i];
        uint64_t r = (m >> 64) ? (uint64_t)(m % A.a) : (uint64_t)m % A.a;
        if (r == 0) {
            if (nd[id].na > 1) { split_all(id); st_restart++; return 1; }
            continue;
        }
        if (A.k) {
            uint64_t g = gcd64(r, A.a);
            if (g > 1) {
                refine_atom(id, i, g);
                if (nd[id].na > 1) split_all(id);
                st_restart++;
                return 1;
            }
        }
    }
    return 0;
}
/* merge node src into dst (equal values): one loop over src's list; weights add where a holder has both */
static void merge_into(uint32_t dst, uint32_t src) {
    uint64_t t0 = now_ns();
    uint32_t len = nd[src].len;
    for (uint32_t i = 0; i < len; i++) {
        uint32_t h = nd[src].list[i];
        int k = h_find(h, src);
        uint32_t w = EW(EPL[H[h].eo + k]);
        h_remove_edge(h, k);
        int kd = h_find(h, dst);
        if (kd >= 0) {
            uint32_t w2 = EW(EPL[H[h].eo + kd]) + w;
            if (w2 > 255) die("merge: weight above 255");
            EPL[H[h].eo + kd] = EP(dst, w2);
        } else {
            h_insert_edge(h, dst, w, nd[dst].len);
            list_push(dst, h);
            nd[dst].rc++;
        }
    }
    nd[src].len = 0; nd[src].rc = 0;
    s_node_release(src);
    st_merge++;
    st_split_num += len;
    st_split_ns += now_ns() - t0;
}

/* ---------------------------------------------------------------- entry */

static void hot_add(uint64_t p) {
    for (int j = 0; j < nhot; j++) if (hot[j] == p) return;
    hot[hotpos] = p;
    hotpos = (hotpos + 1) & 15;
    if (nhot < 16) nhot++;
}
static int atom_kind(uint32_t id, uint64_t a) {
    for (int i = 0; i < nd[id].na; i++) if (nd[id].at[i].a == a) return nd[id].at[i].k;
    die("atom not in its node");
    return 0;
}
static int atom_exp(uint32_t id, uint64_t a) {
    for (int i = 0; i < nd[id].na; i++) if (nd[id].at[i].a == a) return nd[id].at[i].e;
    return 0;
}

/* pass 1: d's atoms, then the dictionary: first sightings make one node, nodes already seen split by ratio */
static void g_enter_den(uint32_t k, uint64_t den) {
    if (k >= datocap) {
        while (k >= datocap) datocap = datocap ? 2 * datocap : 256;
        dato = xrealloc(dato, datocap * sizeof *dato);
        datc = xrealloc(datc, datocap * sizeof *datc);
    }
    dato[k] = (uint32_t)ndat; datc[k] = 0;
    if (den == 1) return;
    atom_t at[64];
    int na = 0, s = __builtin_ctzll(den), i;
    uint64_t n = den;
    if (s) { at[na++] = (atom_t){2, (uint8_t)s, 0}; n >>= s; }
    for (i = 0; i < nsp; i++) {  /* trial division with the adaptive stop */
        uint64_t p = sp[i];
        if (p * p > n) break;
        uint64_t q = n * spi[i];
        if (q <= spl[i]) {
            uint32_t e = 0;
            do { n = q; e++; q = n * spi[i]; } while (q <= spl[i]);
            at[na++] = (atom_t){p, (uint8_t)e, 0};
        }
    }
    if (n > 1) {  /* the leftover: no factor below 2^16 (or the stop fired: prime) */
        uint32_t id = dict_get(n);
        if (id != NONE) at[na++] = (atom_t){n, 1, (uint8_t)atom_kind(id, n)};
        else if (i < nsp || n < (1ull << 32)) {
            uint8_t kind = (n >= 65536 && s_lazy && n_unk) ? 2 : 0;  /* may divide a leftover: pending */
            at[na++] = (atom_t){n, 1, kind};
            if (!s_lazy && n >= 65536) hot_add(n);
        } else if (s_lazy) at[na++] = (atom_t){n, 1, 2};
        else {
            for (int j = 0; j < nhot && n > 1; j++) {  /* the known large primes: a bounded hot list */
                if (n % hot[j]) continue;
                uint32_t e = 0;
                while (n % hot[j] == 0) { n /= hot[j]; e++; }
                at[na++] = (atom_t){hot[j], (uint8_t)e, 0};
            }
            if (n > 1) {
                if ((id = dict_get(n)) != NONE) at[na++] = (atom_t){n, 1, 0};
                else if (n < (1ull << 32) || is_prime(n)) { at[na++] = (atom_t){n, 1, 0}; hot_add(n); }
                else {  /* Miller-Rabin said composite: rho */
                    uint64_t P[64];
                    uint32_t E[64];
                    int nf = factor_rest(n, P, E);
                    for (int j = 0; j < nf; j++) { at[na++] = (atom_t){P[j], (uint8_t)E[j], 0}; hot_add(P[j]); }
                }
            }
        }
    }
    GROW(dat, ndat, datcap, (size_t)na);
    memcpy(dat + ndat, at, (size_t)na * sizeof(atom_t));
    ndat += (size_t)na;
    datc[k] = (uint8_t)na;
    /* the dictionary */
    atom_t fresh[64];
    int nf = 0, nhs = 0;
    uint32_t hs[64];
    for (int j = 0; j < na; j++) {
        uint32_t id = dict_get(at[j].a);
        if (id == NONE) {
            if (at[j].k == 2) {  /* pending: its own node until the idle pass */
                atom_t a1 = {at[j].a, 1, 2};
                node_set_atoms(s_node_alloc(), &a1, 1);
            } else fresh[nf++] = at[j];
            continue;
        }
        int dup = 0;
        for (int t = 0; t < nhs && !dup; t++) dup = hs[t] == id;
        if (!dup) hs[nhs++] = id;
    }
    for (int t = 0; t < nhs; t++) {  /* split each node already seen until one ratio class remains */
        uint32_t id = hs[t];
        for (;;) {
            int nn = nd[id].na, cls[MAXE], ncls = 0;
            uint32_t rn[MAXE], rd[MAXE], csize[MAXE];
            for (int a = 0; a < nn; a++) {
                uint32_t ed = 0, eh = nd[id].at[a].e;
                for (int j = 0; j < na; j++) if (at[j].a == nd[id].at[a].a) ed = at[j].e;
                int c;
                for (c = 0; c < ncls; c++) if ((uint64_t)ed * rd[c] == (uint64_t)rn[c] * eh) break;
                if (c == ncls) { rn[c] = ed; rd[c] = eh; csize[c] = 0; ncls++; }
                cls[a] = c;
                csize[c]++;
            }
            if (ncls == 1) break;
            int keep = 0;
            for (int c = 1; c < ncls; c++) if (csize[c] > csize[keep]) keep = c;
            int mvc = keep == 0 ? 1 : 0;
            uint8_t mv[MAXE];
            for (int a = 0; a < nn; a++) mv[a] = cls[a] == mvc;
            split_out(id, mv);
        }
    }
    if (nf) {
        norm_atoms(fresh, nf);
        node_set_atoms(s_node_alloc(), fresh, nf);
    }
}

/* pass 2: each distinct denominator's edges by dictionary lookups, as a linked transient holder */
static void g_resolve(void) {
    for (uint32_t k = 0; k < ndd; k++) {
        dden *D = &dd[k];
        D->hd = NONE;
        if (D->bad || D->den == 1) continue;
        uint32_t e[MAXE];
        int ne = 0;
        for (uint32_t j = 0; j < datc[k]; j++) {
            atom_t A = dat[dato[k] + j];
            uint32_t id = dict_get(A.a);
            if (id == NONE) die("resolve: atom not in the dictionary");
            int eh = atom_exp(id, A.a);
            if (!eh || A.e % eh) die("resolve: exponent not a multiple");
            uint32_t w = A.e / (uint32_t)eh;
            int t;
            for (t = 0; t < ne; t++) if (EN(e[t]) == id) break;
            if (t < ne) { if (EW(e[t]) != w) die("resolve: inconsistent ratio"); continue; }
            if (ne == MAXE) die("resolve: too many edges");
            e[ne++] = EP(id, w);
        }
        qsort(e, (size_t)ne, 4, cmp_u32);
        if (audit_edges(D->den, ne, e)) die("resolve: denominator does not match its edges");
        D->hd = h_new(0, D->den, e, ne, 3);
        g_link(D->hd);
    }
}
static void g_entry_done(void) {
    for (uint32_t k = 0; k < ndd; k++)
        if (dd[k].hd != NONE) { g_unlink(dd[k].hd); h_free(dd[k].hd); dd[k].hd = NONE; }
    ndat = 0;
}

/* ---------------------------------------------------------------- the idle pass (batch refinement) */

/* Within one idle pass only: each atom refined during the pass -> its pieces (dropped when the pass ends). */
typedef struct { uint64_t *k; uint32_t *off; uint8_t *cnt; size_t cap, n; uint64_t *pv; size_t npv, pvcap; } rmap;
static void rm_init(rmap *R, size_t want) {
    R->cap = 1024;
    while (R->cap < 2 * want) R->cap *= 2;
    R->k = calloc(R->cap, 8); R->off = malloc(R->cap * 4); R->cnt = malloc(R->cap);
    R->n = 0; R->npv = 0; R->pvcap = 1024; R->pv = malloc(R->pvcap * 8);
}
static void rm_free(rmap *R) { free(R->k); free(R->off); free(R->cnt); free(R->pv); }
static int rm_get(const rmap *R, uint64_t a, const uint64_t **pv) {
    size_t i = hsh(a) & (R->cap - 1);
    while (R->k[i]) { if (R->k[i] == a) { *pv = R->pv + R->off[i]; return R->cnt[i]; } i = (i + 1) & (R->cap - 1); }
    return -1;
}
static void rm_put(rmap *R, uint64_t a, const uint64_t *pv, int n) {
    if (2 * (R->n + 1) > R->cap) {  /* grow */
        rmap O = *R;
        R->cap *= 2;
        R->k = calloc(R->cap, 8); R->off = malloc(R->cap * 4); R->cnt = malloc(R->cap);
        for (size_t i = 0; i < O.cap; i++)
            if (O.k[i]) {
                size_t j = hsh(O.k[i]) & (R->cap - 1);
                while (R->k[j]) j = (j + 1) & (R->cap - 1);
                R->k[j] = O.k[i]; R->off[j] = O.off[i]; R->cnt[j] = O.cnt[i];
            }
        free(O.k); free(O.off); free(O.cnt);
    }
    while (R->npv + (size_t)n > R->pvcap) { R->pvcap *= 2; R->pv = realloc(R->pv, R->pvcap * 8); }
    size_t i = hsh(a) & (R->cap - 1);
    while (R->k[i]) i = (i + 1) & (R->cap - 1);
    R->k[i] = a; R->off[i] = (uint32_t)R->npv; R->cnt[i] = (uint8_t)n; R->n++;
    memcpy(R->pv + R->npv, pv, (size_t)n * 8);
    R->npv += (size_t)n;
}
/* the current atoms that an original value (an atom when the pass began) has become */
static int rm_expand(const rmap *R, uint64_t o, uint64_t *out, int k) {
    if (dict_get(o) != NONE) {
        for (int i = 0; i < k; i++) if (out[i] == o) return k;
        if (k == 64) die("idle: too many pieces");
        out[k] = o;
        return k + 1;
    }
    const uint64_t *pv;
    int n = rm_get(R, o, &pv);
    if (n < 0) { fprintf(stderr, "o=%llu\n", (unsigned long long)o); die("idle: value neither an atom nor refined"); }
    for (int i = 0; i < n; i++) k = rm_expand(R, pv[i], out, k);
    return k;
}
/* replace atom a (in its node) by its pieces over the base B; record the refinement.  A piece that is already an
 * atom of another node is recorded in conf (piece, that node): the caller merges the two. */
typedef struct { uint64_t b; uint32_t id; } conflict;
static uint32_t idle_replace(rmap *R, uint64_t a, const uint64_t *B, int nb, conflict *conf, int *nconf) {
    uint32_t id = dict_get(a);
    atom_t nat[64];
    int n2 = 0, ea = 0, np = 0;
    for (int i = 0; i < nd[id].na; i++) {
        if (nd[id].at[i].a == a) ea = nd[id].at[i].e;
        else nat[n2++] = nd[id].at[i];
    }
    uint64_t chk = 1, pcs[64];
    for (int b = 0; b < nb; b++) {
        uint32_t f = vlog(a, B[b]);
        if (!f) continue;
        for (uint32_t t = 0; t < f; t++) chk *= B[b];
        uint32_t prev = dict_get(B[b]);
        if (prev != NONE && prev != id) { if (*nconf == 128) die("idle: too many conflicts"); conf[(*nconf)++] = (conflict){B[b], prev}; }
        nat[n2++] = (atom_t){B[b], (uint8_t)(ea * f), (uint8_t)(is_prime(B[b]) ? 0 : 1)};
        pcs[np++] = B[b];
    }
    if (chk != a) die("idle: pieces do not multiply back");
    node_reatom(id, nat, n2);
    if (!(np == 1 && pcs[0] == a)) rm_put(R, a, pcs, np);
    return id;
}
static int node_has(uint32_t id, uint64_t b) {
    if (!nd[id].alive) return 0;
    for (int i = 0; i < nd[id].na; i++) if (nd[id].at[i].a == b) return 1;
    return 0;
}
/* atom value b is held by several of the nodes cand: split it out of each (copy and loop), merge the singles */
static void resolve_piece(uint64_t b, const uint32_t *cand, int nc) {
    uint32_t hs[8];
    int n = 0;
    for (int j = 0; j < nc; j++) {
        if (!node_has(cand[j], b)) continue;
        int dup = 0;
        for (int t = 0; t < n && !dup; t++) dup = hs[t] == cand[j];
        if (!dup) hs[n++] = cand[j];
    }
    if (n == 0) die("idle: piece in no node");
    if (n > 1) {
        for (int j = 0; j < n; j++)
            if (nd[hs[j]].na > 1) {
                uint8_t mv[MAXE];
                for (int i = 0; i < nd[hs[j]].na; i++) mv[i] = nd[hs[j]].at[i].a == b;
                hs[j] = split_out(hs[j], mv);
            }
        for (int j = 1; j < n; j++) merge_into(hs[0], hs[j]);
    }
    dict_set(b, hs[0]);
}
/* two current atoms a, b (different nodes) share a factor: refine both by their coprime base; a base element now
 * held by several nodes is split out of them (copy and loop) and the single nodes merged */
static void idle_refine_pair(rmap *R, uint64_t a, uint64_t b) {
    uint64_t B[64];
    conflict conf[128];
    int nconf = 0, nb = coprime_base2(a, b, B);
    uint32_t ia = idle_replace(R, a, B, nb, conf, &nconf);
    uint32_t ib = idle_replace(R, b, B, nb, conf, &nconf);
    for (int t = 0; t < nb; t++) {
        uint32_t cand[130];
        int nc = 0;
        cand[nc++] = ia; cand[nc++] = ib;
        for (int j = 0; j < nconf; j++) if (conf[j].b == B[t]) cand[nc++] = conf[j].id;
        resolve_piece(B[t], cand, nc);
    }
}

static uint64_t g_idle(void) {
    uint64_t t0 = now_ns();
    int ph = s_phase;
    s_phase = 2;
    size_t nx = 0, xcap = 256, nk = 0, kcap = 256;
    uint64_t *X = malloc(xcap * 8), *K = malloc(kcap * 8);
    uint32_t *xn = malloc(xcap * 4);
    for (uint32_t q = 0; q < npq; q++) {  /* X: the pending atoms */
        uint32_t id = pq[q];
        if (!nd[id].alive || nd[id].pend != 1) continue;
        nd[id].pend = 3;  /* collected */
        for (int i = 0; i < nd[id].na; i++)
            if (nd[id].at[i].k == 2) {
                if (nx == xcap) { xcap *= 2; X = realloc(X, xcap * 8); xn = realloc(xn, xcap * 4); }
                xn[nx] = id;
                X[nx++] = nd[id].at[i].a;
            }
    }
    npq = 0;
    if (nx) {
        for (uint32_t id = 0; id < nnodes; id++) {  /* K: the settled large atoms */
            if (!nd[id].alive) continue;
            for (int i = 0; i < nd[id].na; i++)
                if (nd[id].at[i].k != 2 && nd[id].at[i].a >= 65536) {
                    if (nk == kcap) { kcap *= 2; K = realloc(K, kcap * 8); }
                    K[nk++] = nd[id].at[i].a;
                }
        }
        st_idle_x += nx; st_idle_k += nk;
        ptree TX, TK;
        pt_build(&TX, X, nx);
        uint8_t *hk = calloc(nx, 1), *hx = malloc(nx);
        uint64_t *r = malloc(nx * 8);
        if (nk) {  /* Z_K mod x for every pending x: one remainder tree */
            pt_build(&TK, K, nk);
            pt_mod_leaves(&TX, TK.lv[TK.nlev - 1][0], r);
            for (size_t i = 0; i < nx; i++) hk[i] = gcd64(r[i], X[i]) != 1;
        }
        batch_gcd(X, nx, hx);  /* the pending atoms against each other */
        size_t nhit = 0;
        for (size_t i = 0; i < nx; i++) nhit += hk[i] || hx[i];
        st_idle_hits += nhit;
        if (nhit) {
            rmap R;
            rm_init(&R, 4 * nhit);
            size_t *out = malloc((nx + nk + 1) * sizeof(size_t)), no;
            for (size_t i = 0; i < nx; i++) {
                if (!hk[i] && !hx[i]) continue;
                for (int side = 0; side < 2; side++) {  /* partners in K, then in X (descending the trees) */
                    if (side == 0 && !hk[i]) continue;
                    if (side == 1 && !hx[i]) continue;
                    no = 0;
                    if (side == 0) pt_partners(&TK, TK.nlev - 1, 0, X[i], (size_t)-1, out, &no, nk);
                    else pt_partners(&TX, TX.nlev - 1, 0, X[i], i, out, &no, nx);
                    for (size_t j = 0; j < no; j++) {
                        uint64_t o = side == 0 ? K[out[j]] : X[out[j]];
                        for (;;) {  /* refine the current atoms of the two originals until coprime */
                            uint64_t A1[64], A2[64];
                            int n1 = rm_expand(&R, X[i], A1, 0), n2 = rm_expand(&R, o, A2, 0), done = 1;
                            for (int u = 0; u < n1 && done; u++)
                                for (int v = 0; v < n2 && done; v++)
                                    if (A1[u] != A2[v] && gcd64(A1[u], A2[v]) != 1) { idle_refine_pair(&R, A1[u], A2[v]); done = 0; }
                            if (done) break;
                        }
                    }
                }
            }
            free(out);
            rm_free(&R);
        }
        if (nk) pt_free(&TK);
        pt_free(&TX);
        free(hk); free(hx); free(r);
    }
    for (size_t i = 0; i < nx; i++) {  /* settle what is still pending: Miller-Rabin says prime or leftover */
        uint32_t id = xn[i];
        if (!nd[id].alive || !nd[id].pend) continue;
        atom_t nat[MAXE];
        int n2 = nd[id].na, any = 0;
        memcpy(nat, nd[id].at, (size_t)n2 * sizeof(atom_t));
        for (int j = 0; j < n2; j++) if (nat[j].k == 2) { nat[j].k = is_prime(nat[j].a) ? 0 : 1; any = 1; }
        nd[id].pend = 0;
        if (any) node_set_atoms(id, nat, n2);
    }
    free(X); free(K); free(xn);
    s_phase = ph;
    uint64_t dt = now_ns() - t0;
    st_idle_ns += dt;
    return dt;
}

/* ---------------------------------------------------------------- audit */

/* what: "all", or any of nodes, dict, rc, sym, den, coprime (pairwise: gcds of every pair instead of batch gcd) */
static const char *g_audit(const char *what) {
    int all = !what || !strcmp(what, "all");
#define W(x) (all || strstr(what, x))
    uint64_t multi = 0, unk = 0, live = 0, enodes = 0, eholders = 0;
    for (uint32_t id = 0; id < nnodes; id++) {
        const node_t *N = &nd[id];
        if (!N->alive) {
            if (W("rc") && (N->rc || N->len || N->list)) return "dead_node_holds_list";
            continue;
        }
        live++;
        if (W("rc") && N->rc != N->len) return "refcount_ne_list_length";
        if (W("rc") && N->rc == 0) return "unused_node_not_freed";
        if (W("nodes")) {
            if (!N->na) return "node_without_atoms";
            u128 v = 1;
            uint32_t g = 0;
            int u = 0;
            for (int i = 0; i < N->na; i++) {
                for (int j = 0; j < N->at[i].e; j++) v *= N->at[i].a;
                g = (uint32_t)gcd64(g, N->at[i].e);
                if (N->at[i].k) { unk++; u = 1; }
                if (N->at[i].k == 0 && !is_prime(N->at[i].a)) return "atom_marked_prime_is_not";
            }
            if (v != N->v || N->v < 2) return "node_not_its_atoms";
            if (g != 1) return "node_exponents_not_normalised";
            if (N->chk != (N->na > 1 || u)) return "node_hook_flag";
            multi += N->na > 1;
        }
        if (W("dict"))
            for (int i = 0; i < N->na; i++) if (dict_get(N->at[i].a) != id) return "dict_mismatch";
        enodes += N->len;
        if (W("sym"))
            for (uint32_t i = 0; i < N->len; i++) {
                uint32_t h = N->list[i];
                if (h >= nh || !H[h].alive) return "list_names_dead_holder";
                int k = h_find(h, id);
                if (k < 0) return "list_entry_without_edge";
                if (PPL[H[h].eo + k] != i) return "edge_position_mismatch";
            }
    }
    if (W("nodes") && (multi != n_multi || unk != n_unk)) return "node_counters";
    if (live != nlive) return "live_node_count";
    for (uint32_t h = 0; h < nh; h++) {
        const holder_t *X = &H[h];
        if (!X->alive) continue;
        const uint32_t *e = EPL + X->eo;
        eholders += X->ne;
        if (W("sym"))
            for (int k = 0; k < X->ne; k++) {
                uint32_t id = EN(e[k]), pos = PPL[X->eo + k];
                if (id >= nnodes || !nd[id].alive) return "edge_to_dead_node";
                if (pos >= nd[id].len || nd[id].list[pos] != h) return "edge_not_in_node_list";
            }
        if (W("den")) {
            if (X->den) { const char *m = audit_edges(X->den, X->ne, e); if (m) return m; }
            else if (X->ne) return "bad_value_has_edges";
            if ((X->kind == 0 || X->kind == 2) && X->den && X->num && gcd64(mag(X->num), X->den) != 1) return "value_not_reduced";
        }
    }
    if (W("sym") && eholders != enodes) return "edge_count_mismatch";
    if (W("dict")) {
        for (uint64_t a = 2; a < 65536; a++)
            if (dsm[a] && (!nd[dsm[a] - 1].alive || !atom_exp(dsm[a] - 1, a))) return "dict_stale";
        for (size_t i = 0; i < dbcap; i++)
            if (dbk[i] && (!nd[dbv[i]].alive || !atom_exp(dbv[i], dbk[i]))) return "dict_stale";
    }
    if (W("coprime")) {
        uint64_t *v = malloc(((size_t)nlive + 1) * 8);
        size_t n = 0;
        for (uint32_t id = 0; id < nnodes; id++) if (nd[id].alive) v[n++] = nd[id].v;
        const char *m = coprime_check(v, n, what && strstr(what, "pairwise"));
        free(v);
        if (m) return m;
    }
#undef W
    return NULL;
}

/* test hooks: break one invariant */
static int g_corrupt(const char *what) {
    uint32_t a = NONE, b = NONE;
    for (uint32_t id = 0; id < nnodes; id++)
        if (nd[id].alive) { if (a == NONE) a = id; else if (b == NONE) b = id; }
    if (!strcmp(what, "coprime")) {  /* node b takes a factor of a (its atoms and the dictionary too) */
        if (b == NONE) return 0;
        uint64_t x;
        if (__builtin_mul_overflow(nd[b].v, nd[a].at[0].a, &x)) return 0;
        nd_setv(b, x);
        return 1;
    }
    if (!strcmp(what, "pos")) {  /* an edge's list position */
        for (uint32_t id = 0; id < nnodes; id++)
            if (nd[id].alive && nd[id].len >= 2) {
                uint32_t h = nd[id].list[0];
                PPL[H[h].eo + h_find(h, id)] = 1;
                return 1;
            }
        return 0;
    }
    if (!strcmp(what, "list")) {  /* a list entry dropped (the holder keeps its edge) */
        for (uint32_t id = 0; id < nnodes; id++)
            if (nd[id].alive && nd[id].len >= 2) { nd[id].len--; return 1; }
        return 0;
    }
    if (!strcmp(what, "rc")) { if (a == NONE) return 0; nd[a].rc++; return 1; }
    if (!strcmp(what, "dict")) { if (b == NONE) return 0; dict_set(nd[a].at[0].a, b); return 1; }
    if (!strcmp(what, "den")) {
        for (uint32_t h = 0; h < nh; h++) if (H[h].alive && H[h].ne) { H[h].den += 2; return 1; }
        return 0;
    }
    if (!strcmp(what, "leak")) { corrupt_leak = 1; return 1; }
    if (!strcmp(what, "leaklist")) { corrupt_leak = 2; return 1; }
    if (!strcmp(what, "result")) { corrupt_result = 1; return 1; }
    return 0;
}

static void g_stats(FILE *f) {
    fprintf(f, "\"nodes_live\": %u, \"node_slots\": %u, \"nodes_multi\": %llu, \"atoms_not_prime\": %llu, "
               "\"nodes_new\": %llu, \"nodes_freed\": %llu, \"splits_entry\": %llu, \"splits_op\": %llu, "
               "\"splits_idle\": %llu, \"split_numerators\": %llu, \"split_ns\": %llu, \"split_max_ns\": %llu, "
               "\"merges\": %llu, \"refines\": %llu, \"restarts\": %llu, \"rho_calls\": %llu, \"idle_ns_total\": %llu, "
               "\"idle_hits\": %llu, \"idle_pending\": %llu, \"idle_known\": %llu, \"dict_entries\": %zu",
            nlive, nnodes, (unsigned long long)n_multi, (unsigned long long)n_unk, (unsigned long long)st_node_new,
            (unsigned long long)st_node_freed, (unsigned long long)st_split[0], (unsigned long long)st_split[1],
            (unsigned long long)st_split[2], (unsigned long long)st_split_num, (unsigned long long)st_split_ns,
            (unsigned long long)st_split_max_ns, (unsigned long long)st_merge, (unsigned long long)st_refine,
            (unsigned long long)st_restart, (unsigned long long)st_rho_calls, (unsigned long long)st_idle_ns,
            (unsigned long long)st_idle_hits, (unsigned long long)st_idle_x, (unsigned long long)st_idle_k, dbcnt);
}
/* the nodes, for tests: "value=atom^e*atom^e/k" sorted by value */
static int cmp_u64(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}
static void g_dump_nodes(FILE *f) {
    uint64_t *v = malloc(((size_t)nlive + 1) * 8);
    size_t n = 0;
    for (uint32_t id = 0; id < nnodes; id++) if (nd[id].alive) v[n++] = nd[id].v;
    qsort(v, n, 8, cmp_u64);
    fprintf(f, "N");
    for (size_t i = 0; i < n; i++) fprintf(f, " %llu", (unsigned long long)v[i]);
    fprintf(f, "\n");
    free(v);
}
static uint32_t node_of_value(uint64_t v) {
    for (uint32_t id = 0; id < nnodes; id++) if (nd[id].alive && nd[id].v == v) return id;
    return NONE;
}
static uint32_t g_degree(uint64_t v) { uint32_t id = node_of_value(v); return id == NONE ? 0 : nd[id].len; }
static uint32_t g_refcount(uint64_t v) { uint32_t id = node_of_value(v); return id == NONE ? 0 : nd[id].rc; }
