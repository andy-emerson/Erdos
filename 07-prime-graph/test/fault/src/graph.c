/* graph.c: the graph (spec "The representation", "Lifetimes", "Entering a denominator", "Splitting a node").
 *
 *   - nodes: the hot and cold tables, atoms, the free list (S5: a node is freed when its last holder leaves);
 *   - the dictionary: atom -> node, a direct array below 2^16 and open addressing above, entries removed with
 *     their node (S4: sharing is found by lookups, never by scanning nodes or known primes);
 *   - the edge pool (size classes 1..16, freed blocks reused within their class) and holders in fixed chunks;
 *   - two-sided edges: link and unlink are O(edges), each edge knowing its position in its node's list (S2);
 *   - splits by copy and loop (S3): one pass over the split node's list, no history;
 *   - entry steps 1-4, with a bounded factor cache per distinct denominator (the parity fix for small graphs);
 *   - the reduction hook (pgi_partial): the g < N case of the spec's reduction.
 */
#include "pg_internal.h"

/* ------------------------------------------------------------------------------------------------- nodes */

static uint32_t node_alloc(pg_graph *g) {
    uint32_t id;
    if (g->nfree_n) id = g->nfree[--g->nfree_n];
    else {
        if (g->nnodes == g->ncap) {
            uint32_t oc = g->ncap;
            g->ncap = g->ncap ? 2 * g->ncap : 64;
            g->nh = pgi_realloc(g, g->nh, (size_t)g->ncap * sizeof *g->nh);
            g->nc = pgi_realloc(g, g->nc, (size_t)g->ncap * sizeof *g->nc);
            g->wsa = pgi_realloc(g, g->wsa, g->ncap);
            g->wsb = pgi_realloc(g, g->wsb, g->ncap);
            memset(g->wsa + oc, 0, g->ncap - oc);
            memset(g->wsb + oc, 0, g->ncap - oc);
        }
        if (g->nnodes >= MAX_NODES) pgi_die("too many nodes (2^24)");
        id = g->nnodes++;
    }
    memset(&g->nh[id], 0, sizeof g->nh[id]);
    memset(&g->nc[id], 0, sizeof g->nc[id]);
    g->nh[id].alive = 1;
    g->nlive++;
    g->st_nodes_new++;
    return id;
}

static uint32_t norm_atoms(atom_t *at, int na) {  /* divide the exponents by their gcd; returns it */
    uint32_t d = 0;
    for (int i = 0; i < na; i++) d = (uint32_t)pgi_gcd(d, at[i].e);
    if (d > 1)
        for (int i = 0; i < na; i++) at[i].e /= d;
    return d ? d : 1;
}

static void dict_set(pg_graph *g, uint64_t a, uint32_t id);
static void dict_del(pg_graph *g, uint64_t a);

/* set node id's atoms (exponents already normalised): its value, the dictionary, the multi flag */
static void node_set_atoms(pg_graph *g, uint32_t id, const atom_t *at, int na) {
    node_cold *C = &g->nc[id];
    for (int i = 0; i < C->na; i++)
        if (pgi_dict_get(g, C->at[i].p) == id) dict_del(g, C->at[i].p);
    if (C->na > 1) g->nmulti--;
    atom_t tmp[MAXE];
    memcpy(tmp, at, (size_t)na * sizeof *at);
    C->at = pgi_realloc(g, C->at, (size_t)na * sizeof(atom_t));
    u128 v = 1;
    for (int i = 0; i < na; i++) {
        atom_t A = tmp[i];
        A.inv = (A.p & 1) ? pgi_inv64(A.p) : 1;
        A.lim = UINT64_MAX / A.p;
        C->at[i] = A;
        for (uint32_t j = 0; j < A.e; j++) {
            v *= A.p;
            if (v >> 64) pgi_die("node value above 2^64");
        }
        dict_set(g, A.p, id);
    }
    C->na = (uint8_t)na;
    if (na > 1) g->nmulti++;
    node_hot *N = &g->nh[id];
    N->v = (uint64_t)v;
    N->s = (uint8_t)__builtin_ctzll(N->v);
    uint64_t m = N->v >> N->s;
    N->inv = pgi_inv64(m);
    N->lim = UINT64_MAX / m;
    N->multi = na > 1;
}

static void node_release(pg_graph *g, uint32_t id) {  /* its last holder left (I3, S5) */
    node_cold *C = &g->nc[id];
    for (int i = 0; i < C->na; i++)
        if (pgi_dict_get(g, C->at[i].p) == id) dict_del(g, C->at[i].p);
    if (C->na > 1) g->nmulti--;
    pgi_free(g, C->list);
    pgi_free(g, C->at);
    memset(C, 0, sizeof *C);
    memset(&g->nh[id], 0, sizeof g->nh[id]);
    if (g->nfree_n == g->nfree_cap) {
        g->nfree_cap = g->nfree_cap ? 2 * g->nfree_cap : 64;
        g->nfree = pgi_realloc(g, g->nfree, (size_t)g->nfree_cap * 4);
    }
    g->nfree[g->nfree_n++] = id;
    g->nlive--;
    g->st_nodes_freed++;
}

/* ------------------------------------------------------------------------------------------------- the dictionary */

static inline size_t hsh(uint64_t x) {
    x ^= x >> 33; x *= 0xff51afd7ed558ccdULL; x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL; x ^= x >> 33;
    return (size_t)x;
}

uint32_t pgi_dict_get(const pg_graph *g, uint64_t a) {
    if (a < 65536) return g->dsm[a] - 1;
    size_t m = g->dbcap - 1, i = hsh(a) & m;
    while (g->dbk[i]) {
        if (g->dbk[i] == a) return g->dbv[i];
        i = (i + 1) & m;
    }
    return NONE;
}

static void dict_set(pg_graph *g, uint64_t a, uint32_t id) {
    if (a < 65536) { g->dsm[a] = id + 1; return; }
    size_t m = g->dbcap - 1, i = hsh(a) & m;
    while (g->dbk[i] && g->dbk[i] != a) i = (i + 1) & m;
    if (g->dbk[i]) { g->dbv[i] = id; return; }
    if (2 * (g->dbcnt + 1) > g->dbcap) {
        uint64_t *ok = g->dbk;
        uint32_t *ov = g->dbv;
        size_t oc = g->dbcap;
        g->dbcap *= 2;
        m = g->dbcap - 1;
        g->dbk = pgi_calloc(g, g->dbcap * sizeof *g->dbk);
        g->dbv = pgi_alloc(g, g->dbcap * sizeof *g->dbv);
        for (size_t j = 0; j < oc; j++)
            if (ok[j]) {
                size_t t = hsh(ok[j]) & m;
                while (g->dbk[t]) t = (t + 1) & m;
                g->dbk[t] = ok[j];
                g->dbv[t] = ov[j];
            }
        pgi_free(g, ok);
        pgi_free(g, ov);
        i = hsh(a) & m;
        while (g->dbk[i]) i = (i + 1) & m;
    }
    g->dbk[i] = a;
    g->dbv[i] = id;
    g->dbcnt++;
}

static void dict_del(pg_graph *g, uint64_t a) {  /* backward-shift deletion (linear probing) */
    if (a < 65536) { g->dsm[a] = 0; return; }
    size_t m = g->dbcap - 1, i = hsh(a) & m;
    while (g->dbk[i] && g->dbk[i] != a) i = (i + 1) & m;
    if (!g->dbk[i]) return;
    g->dbk[i] = 0;
    g->dbcnt--;
    for (size_t j = i;;) {
        j = (j + 1) & m;
        if (!g->dbk[j]) break;
        size_t k = hsh(g->dbk[j]) & m;
        if (i <= j ? (i < k && k <= j) : (i < k || k <= j)) continue;
        g->dbk[i] = g->dbk[j];
        g->dbv[i] = g->dbv[j];
        g->dbk[j] = 0;
        i = j;
    }
}

/* ------------------------------------------------------------------------------------------------- the edge pool */

static void pool_grow(pg_graph *g, uint32_t c) {
    while (g->pool_n + c > g->pool_cap) g->pool_cap = g->pool_cap ? 2 * g->pool_cap : 1024;
    if (g->pool_cap >= (1ull << 32)) pgi_die("edge pool too large");
    g->EPL = pgi_realloc(g, g->EPL, g->pool_cap * 4);
#if !PGF_O2
    g->PPL = pgi_realloc(g, g->PPL, g->pool_cap * 4);  /* O2 keeps list positions in the records */
#endif
}
static inline uint32_t pool_get(pg_graph *g, int cls) {
    uint32_t c = 1u << cls;
    g->pool_used += c;
    if (g->pfree[cls] != NONE) {
        uint32_t o = g->pfree[cls];
        g->pfree[cls] = g->EPL[o];
        return o;
    }
    if (UNLIKELY(g->pool_n + c > g->pool_cap)) pool_grow(g, c);
    uint32_t o = (uint32_t)g->pool_n;
    g->pool_n += c;
    return o;
}
static inline void pool_put(pg_graph *g, uint32_t o, int cls) {
    g->EPL[o] = g->pfree[cls];
    g->pfree[cls] = o;
    g->pool_used -= 1u << cls;
}
static inline int cls_for(int ne) { return ne <= 1 ? 0 : 32 - __builtin_clz((unsigned)ne - 1); }

/* ------------------------------------------------------------------------------------------------- O2 records */

#if PGF_O2
static uint32_t er_new(pg_graph *g, uint32_t h, uint32_t n, uint32_t w) {
    uint32_t r;
    if (g->erfree != NONE && g->erfree != 0xFFFFFFFEu) {
        r = g->erfree;
        g->erfree = g->ER[r].h;
    } else {
        if (g->ern == g->ercap) {
            g->ercap = g->ercap ? 2 * g->ercap : 1024;
            g->ER = pgi_realloc(g, g->ER, (size_t)g->ercap * sizeof *g->ER);
        }
        r = g->ern++;
    }
    g->ER[r] = (er_t){h, n, 0, w};
    g->erlive++;
    return r;
}
static void er_free(pg_graph *g, uint32_t r) {
    g->ER[r].w = 0;
    g->ER[r].n = NONE;
    g->ER[r].h = g->erfree;
    g->erfree = r;
    g->erlive--;
}
#endif

/* ------------------------------------------------------------------------------------------------- holders */

static void holder_chunk(pg_graph *g) {
    if (g->hn >= 0xFFFFF000u) pgi_die("too many holders");
    if ((g->nhch & (g->nhch - 1)) == 0)  /* the chunk table doubles at powers of two */
        g->hch = pgi_realloc(g, g->hch, (size_t)(g->nhch ? 2 * g->nhch : 1) * sizeof *g->hch);
    g->hch[g->nhch++] = pgi_alloc(g, HCHUNK * sizeof(pg_value));
}
static inline pg_value *holder_alloc(pg_graph *g) {
    uint32_t id;
    pg_value *h;
    if (g->hfree != NONE) {
        id = g->hfree;
        h = HP(g, id);
        g->hfree = h->eo;
    } else {
        if (UNLIKELY(g->hn == ((uint64_t)g->nhch << HCHUNK_BITS))) holder_chunk(g);
        id = g->hn++;
        h = HP(g, id);
    }
    h->id = id;
    g->hlive++;
    return h;
}

static inline int h_find(const pg_graph *g, const pg_value *h, uint32_t node) {
    const uint32_t *e = g->EPL + h->eo;
    for (int k = 0; k < h->ne; k++)
        if (EN(e[k]) == node) return k;
    return -1;
}

static void list_grow(pg_graph *g, node_cold *C) {
    C->lcap = C->lcap ? 2 * C->lcap : 4;
    C->list = pgi_realloc(g, C->list, (size_t)C->lcap * 4);
}
#if !PGF_O2
static inline void link_holder(pg_graph *g, pg_value *h) {
    const uint32_t *e = g->EPL + h->eo;
    uint32_t *pp = g->PPL + h->eo, hid = h->id;
    for (int k = 0; k < h->ne; k++) {
        node_cold *C = &g->nc[EN(e[k])];
        uint32_t len = C->len;
        if (UNLIKELY(len == C->lcap)) list_grow(g, C);
        C->list[len] = hid;
        C->len = len + 1;
        C->rc++;
        pp[k] = len;
    }
}
#else
/* O2: h's block is allocated; give it one record per packed edge e[k] and link each record into its node's list */
static inline void link_holder_o2(pg_graph *g, pg_value *h, const uint32_t *e) {
    for (int k = 0; k < h->ne; k++) {
        uint32_t id = EN(e[k]);
        uint32_t r = er_new(g, h->id, id, EW(e[k]));
        g->EPL[h->eo + k] = r;
        node_cold *C = &g->nc[id];
        uint32_t len = C->len;
        if (UNLIKELY(len == C->lcap)) list_grow(g, C);
        C->list[len] = r;
        C->len = len + 1;
        C->rc++;
        g->ER[r].pos = len;
    }
}
#endif

/* remove holder h's edge k from its node's list (swap with the list's last entry, whose edge learns its new
 * position), and free the node if that was its last holder */
static void unlink_edge(pg_graph *g, pg_value *h, int k) {
#if PGF_O2
    uint32_t r = g->EPL[h->eo + k], id = g->ER[r].n, pos = g->ER[r].pos;
    node_cold *C = &g->nc[id];
    uint32_t last = C->list[C->len - 1];
    C->list[pos] = last;
    if (pos != C->len - 1) {
        if (g->ER[last].n != id) pgi_die("unlink: a list names a record of another node");
        g->ER[last].pos = pos;
    }
    er_free(g, r);
#else
    uint32_t id = EN(g->EPL[h->eo + k]), pos = g->PPL[h->eo + k];
    node_cold *C = &g->nc[id];
    uint32_t last = C->list[C->len - 1];
    C->list[pos] = last;
    if (pos != C->len - 1) {
        pg_value *L = HP(g, last);
        int kk = h_find(g, L, id);
        if (kk < 0) pgi_die("unlink: a list names a holder without the edge");
        g->PPL[L->eo + kk] = pos;
    }
#endif
    C->len--;
    C->rc--;
    if (C->lcap > 16 && C->len < C->lcap / 4) {
        C->lcap /= 2;
        C->list = pgi_realloc(g, C->list, (size_t)C->lcap * 4);
    }
    if (C->rc == 0) node_release(g, id);
}

static void unlink_holder(pg_graph *g, pg_value *h) {
    for (int k = 0; k < h->ne; k++) unlink_edge(g, h, k);
}

/* h becomes r, whose edges are h's with weights lowered or removed (a reduction of h): weights change in place and
 * only the removed edges leave their nodes' lists (a new holder would relink every edge) */
void pgi_holder_reduce(pg_graph *g, pg_value *h, const raw *r) {
    int j = 0, w = 0;
    for (int k = 0; k < h->ne; k++) {
#if PGF_O2
        uint32_t rid = g->EPL[h->eo + k];
        if (j < r->ne && EN(r->e[j]) == g->ER[rid].n) {
            if (EW(r->e[j]) > g->ER[rid].w) pgi_die("holder_reduce: a weight grew");
            g->ER[rid].w = EW(r->e[j++]);  /* the shared record: one exponent for both sides */
            g->EPL[h->eo + w] = rid;
            w++;
        } else
            unlink_edge(g, h, k);
#else
        uint32_t ek = g->EPL[h->eo + k];
        if (j < r->ne && EN(r->e[j]) == EN(ek)) {
            if (EW(r->e[j]) > EW(ek)) pgi_die("holder_reduce: a weight grew");
            g->EPL[h->eo + w] = r->e[j++];
            g->PPL[h->eo + w] = g->PPL[h->eo + k];  /* the node lists keep their positions */
            w++;
        } else
            unlink_edge(g, h, k);
#endif
    }
    if (j != r->ne) pgi_die("holder_reduce: a new edge");
    h->ne = (uint8_t)w;
    if (!w && h->cls != 0xFF) {
        pool_put(g, h->eo, h->cls);
        h->cls = 0xFF;
        h->eo = NONE;
    }
    h->num = r->num;
    h->den = r->den;
}

/* a new linked holder; e must not point into the edge pool (pool_get may move it) */
static inline pg_value *holder_make(pg_graph *g, int64_t num, uint64_t den, const uint32_t *e, int ne, int kind) {
    if (UNLIKELY(ne > MAXE)) pgi_die("too many edges on a holder");
    pg_value *h = holder_alloc(g);
    h->num = num;
    h->den = den;
    h->ne = (uint8_t)ne;
    h->kind = (uint8_t)kind;
    g->hkind[kind]++;
    if (ne) {
        h->cls = (uint8_t)cls_for(ne);
        uint32_t o = pool_get(g, h->cls);
        h->eo = o;
#if PGF_O2
        link_holder_o2(g, h, e);
#else
        for (int k = 0; k < ne; k++) g->EPL[o + k] = e[k];
        link_holder(g, h);
#endif
    } else {
        h->cls = 0xFF;
        h->eo = NONE;
    }
    return h;
}

pg_value *pgi_holder_new(pg_graph *g, int64_t num, uint64_t den, const uint32_t *e, int ne, int kind) {
    uint32_t tmp[MAXE];
    if (UNLIKELY(ne > MAXE)) pgi_die("too many edges on a holder");
    for (int k = 0; k < ne; k++) tmp[k] = e[k];  /* e may point into the pool */
    return holder_make(g, num, den, tmp, ne, kind);
}

pg_value *pgi_holder_from_raw(pg_graph *g, const raw *r, int kind) {  /* a raw is never in the pool */
    return holder_make(g, r->num, r->den, r->e, r->ne, kind);
}

void pgi_holder_release(pg_graph *g, pg_value *h) {
    if (h->kind == HK_FREE) pgi_die("release of a released value");
    unlink_holder(g, h);
    if (h->cls != 0xFF) pool_put(g, h->eo, h->cls);
    g->hkind[h->kind]--;
    h->kind = HK_FREE;
    h->ne = 0;
    h->cls = 0xFF;
    h->num = 0;
    h->den = 0;
    h->eo = g->hfree;
    g->hfree = h->id;
    g->hlive--;
}

/* insert the edge (node, w) with list position pos into holder h, keeping its edges ascending */
static uint32_t h_insert_edge(pg_graph *g, pg_value *h, uint32_t node, uint32_t w, uint32_t pos) {
    if (w == 0 || w > 255) pgi_die("edge weight out of range");
    int cap = h->cls == 0xFF ? 0 : (1 << h->cls);
    if (h->ne == cap) {
        int c = cls_for(h->ne + 1);
        if (c > 4) pgi_die("too many edges on a holder");
        uint32_t o = pool_get(g, c);
        if (h->ne) {
            memcpy(g->EPL + o, g->EPL + h->eo, 4 * (size_t)h->ne);
#if !PGF_O2
            memcpy(g->PPL + o, g->PPL + h->eo, 4 * (size_t)h->ne);
#endif
            pool_put(g, h->eo, h->cls);
        }
        h->eo = o;
        h->cls = (uint8_t)c;
    }
#if PGF_O2
    uint32_t r = er_new(g, h->id, node, w);
    g->ER[r].pos = pos;
    uint32_t *e = g->EPL + h->eo;
    int k = h->ne;
    while (k > 0 && g->ER[e[k - 1]].n > node) { e[k] = e[k - 1]; k--; }
    e[k] = r;
    h->ne++;
    return r;
#else
    uint32_t *e = g->EPL + h->eo, *p = g->PPL + h->eo;
    int k = h->ne;
    while (k > 0 && EN(e[k - 1]) > node) { e[k] = e[k - 1]; p[k] = p[k - 1]; k--; }
    e[k] = EP(node, w);
    p[k] = pos;
    h->ne++;
    return 0;
#endif
}

/* ------------------------------------------------------------------------------------------------- pins */

void pgi_pin(pg_graph *g, uint32_t *e, int *ne) {
    if (g->npins == 8) pgi_die("too many pins");
    g->pins[g->npins++] = (pin_t){e, ne};
}
void pgi_unpin(pg_graph *g, uint32_t *e) {
    for (int i = 0; i < g->npins; i++)
        if (g->pins[i].e == e) { g->pins[i] = g->pins[--g->npins]; return; }
    pgi_die("unpin of a list not pinned");
}

static void pin_patch(pg_graph *g, uint32_t N, uint32_t P, uint32_t gQ, uint32_t gR) {
    for (int i = 0; i < g->npins; i++) {
        uint32_t *e = g->pins[i].e;
        int ne = *g->pins[i].ne, k;
        for (k = 0; k < ne && EN(e[k]) != N; k++) {}
        if (k == ne) continue;
        uint32_t w = EW(e[k]);
        if (w * gR > 255 || w * gQ > 255 || ne >= AW) pgi_die("pin patch out of range");
        e[k] = EP(N, w * gR);
        int j = ne;
        while (j > 0 && EN(e[j - 1]) > P) { e[j] = e[j - 1]; j--; }
        e[j] = EP(P, w * gQ);
        *g->pins[i].ne = ne + 1;
    }
}

/* ------------------------------------------------------------------------------------------------- splits */

/* Move the atoms of node Nid flagged in mv into a new node P (copy and loop).  Returns P. */
static uint32_t split_out(pg_graph *g, uint32_t Nid, const uint8_t *mv, int cause) {
    atom_t Q[MAXE], R[MAXE];
    int nq = 0, nr = 0;
    for (int i = 0; i < g->nc[Nid].na; i++) {
        if (mv[i]) Q[nq++] = g->nc[Nid].at[i];
        else R[nr++] = g->nc[Nid].at[i];
    }
    if (!nq || !nr) pgi_die("split: an empty part");
    uint32_t gQ = norm_atoms(Q, nq), gR = norm_atoms(R, nr);
    uint32_t P = node_alloc(g);  /* may move the node tables */
    node_set_atoms(g, P, Q, nq);
    node_set_atoms(g, Nid, R, nr);
    uint32_t len = g->nc[Nid].len, stop = len;
    int interrupted = pgf_split_interrupt(&stop);  /* F8: an update stopped halfway */
#if PGF_O2
    /* 1+2. O2 cannot share the list (records differ): one new record per holder of N, at the same position */
    node_cold *CP = &g->nc[P];
    CP->lcap = len < 4 ? 4 : len;
    CP->list = pgi_alloc(g, (size_t)CP->lcap * 4);
    CP->len = 0;
    CP->rc = 0;
    for (uint32_t i = 0; i < stop; i++) {
        uint32_t r = g->nc[Nid].list[i];
        if (g->ER[r].n != Nid) pgi_die("split: a list names a record of another node");
        pg_value *h = HP(g, g->ER[r].h);
        uint32_t w = g->ER[r].w;
        if (gR != 1) {
            if (w * gR > 255) pgi_die("split: weight above 255");
            g->ER[r].w = w * gR;
        }
        uint32_t nr = h_insert_edge(g, h, P, w * gQ, i);
        g->nc[P].list[i] = nr;
        g->nc[P].len = i + 1;
        g->nc[P].rc = i + 1;
    }
#else
    /* 1. the copy: N's holder list duplicated as P's */
    node_cold *CP = &g->nc[P];
    CP->lcap = len < 4 ? 4 : len;
    CP->list = pgi_alloc(g, (size_t)CP->lcap * 4);
    memcpy(CP->list, g->nc[Nid].list, (size_t)len * 4);
    CP->len = len;
    CP->rc = g->nc[Nid].rc;
    /* 2. the loop: each holder gets its edge to P at the same list position, the weights carried over */
    const uint32_t *L = g->nc[Nid].list;
    for (uint32_t i = 0; i < stop; i++) {
        pg_value *h = HP(g, L[i]);
        int k = h_find(g, h, Nid);
        if (k < 0) pgi_die("split: a list names a holder without the edge");
        uint32_t w = EW(g->EPL[h->eo + k]);
        if (gR != 1) {
            if (w * gR > 255) pgi_die("split: weight above 255");
            g->EPL[h->eo + k] = EP(Nid, w * gR);
        }
        h_insert_edge(g, h, P, w * gQ, i);
    }
#endif
    if (interrupted) return P;  /* the rest of the update never happens; the program continues */
    pin_patch(g, Nid, P, gQ, gR);
    if (cause) g->st_split_reduce++;
    else g->st_split_entry++;
    g->st_split_holders += len;
    return P;
}

/* Split node id until its atoms form one ratio class: atom p's ratio is amt(p) / e(p), where amt(p) is the amount
 * listed for p in (ap, aamt) and 0 for an atom not listed.  The largest class stays in id. */
static void split_classes(pg_graph *g, uint32_t id, const uint64_t *ap, const uint32_t *aamt, int nap, int cause) {
    for (;;) {
        int nn = g->nc[id].na, cls[MAXE], ncls = 0, csize[MAXE];
        uint64_t rn[MAXE], rd[MAXE];
        if (nn < 2) return;
        for (int a = 0; a < nn; a++) {
            uint64_t amt = 0, eh = g->nc[id].at[a].e;
            for (int j = 0; j < nap; j++)
                if (ap[j] == g->nc[id].at[a].p) { amt = aamt[j]; break; }
            int c;
            for (c = 0; c < ncls; c++)
                if (amt * rd[c] == rn[c] * eh) break;
            if (c == ncls) { rn[c] = amt; rd[c] = eh; csize[c] = 0; ncls++; }
            cls[a] = c;
            csize[c]++;
        }
        if (ncls == 1) return;
        int keep = 0;
        for (int c = 1; c < ncls; c++)
            if (csize[c] > csize[keep]) keep = c;
        int mvc = keep == 0 ? 1 : 0;
        uint8_t mv[MAXE];
        for (int a = 0; a < nn; a++) mv[a] = cls[a] == mvc;
#if PGF_POLICY == 1 || PGF_POLICY == 2
        pgf_before_split(g);  /* verify (and repair) before the copy and loop */
        if (!g->nh[id].alive || g->nc[id].na != nn) continue;  /* a rebuild changed the node: recompute */
#endif
        split_out(g, id, mv, cause);
    }
}

void pgi_split_amounts(pg_graph *g, uint32_t id, const uint64_t *ap, const uint32_t *amt, int na) {
    split_classes(g, id, ap, amt, na, 1);
}

int pgi_partial(pg_graph *g, uint32_t id, u128 m, uint32_t wr) {
    const node_cold *C = &g->nc[id];
    uint64_t r = (m >> 64) ? (uint64_t)(m % g->nh[id].v) : (uint64_t)m;  /* p | m iff p | m mod N, for p | N */
    int any = 0;
    for (int i = 0; i < C->na && !any; i++) {
        const atom_t *A = &C->at[i];
        any = A->p == 2 ? !(r & 1) : r * A->inv <= A->lim;
    }
    if (!any) return 0;
    uint64_t ap[MAXE];
    uint32_t amt[MAXE];
    int na = C->na;
    for (int i = 0; i < na; i++) {  /* t(p) = min(v_p(m), wr * e(p)) */
        uint64_t p = C->at[i].p;
        uint32_t cap = wr * C->at[i].e, t = 0;
        u128 x = m;
        while (t < cap && x % p == 0) { x /= p; t++; }
        ap[i] = p;
        amt[i] = t;
    }
    split_classes(g, id, ap, amt, na, 1);
    return 1;
}

__attribute__((noinline)) int pgi_partial_edges(pg_graph *g, const uint32_t *e, int ne, u128 m) {
    for (int k = 0; k < ne; k++)
        if (g->nh[EN(e[k])].multi && pgi_partial(g, EN(e[k]), m, EW(e[k]))) return 1;
    return 0;
}

__attribute__((noinline)) int pgi_partial_mul(pg_graph *g, const uint32_t *n, int k, const uint8_t *wa,
                                              const uint8_t *wb, uint64_t ua, uint64_t ub) {
    for (int i = 0; i < k; i++) {
        if (!g->nh[n[i]].multi) continue;
        if (wb[i] && pgi_partial(g, n[i], ua, wb[i])) return 1;
        if (wa[i] && pgi_partial(g, n[i], ub, wa[i])) return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------------------------------------- entry */

/* the bounded list of the 16 most recently seen large primes (most recent first): p moves to the front */
static void hot_see(pg_graph *g, uint64_t p) {
    if (p < 65536) return;
    int j = 0;
    while (j < g->nhot && g->hot[j] != p) j++;
    if (j == g->nhot && g->nhot < 16) g->nhot++;
    if (j == 16) j = 15;  /* not present and full: the least recent drops out */
    memmove(g->hot + 1, g->hot, (size_t)j * sizeof *g->hot);
    memmove(g->hinv + 1, g->hinv, (size_t)j * sizeof *g->hinv);
    memmove(g->hlim + 1, g->hlim, (size_t)j * sizeof *g->hlim);
    g->hot[0] = p;
    g->hinv[0] = pgi_inv64(p);  /* p is odd */
    g->hlim[0] = UINT64_MAX / p;
}

/* steps 1 and 2: d's atoms (primes and exponents) */
static int den_atoms(pg_graph *g, uint64_t d, uint64_t *P, uint32_t *E) {
    int k = 0, stopped;
    int s = __builtin_ctzll(d);
    if (s) { P[k] = 2; E[k++] = (uint32_t)s; }
    uint64_t n = d >> s;
    if (n == 1) return k;
    dcache_t *set = NULL;
    if (n >= 65536) {  /* the factor cache: 2-way sets, so its cost and size do not grow with the graph */
        if (!g->dc) g->dc = pgi_calloc(g, 2 * DCACHE_SETS * sizeof *g->dc);
        set = &g->dc[2 * (hsh(d) & (DCACHE_SETS - 1))];
        for (int w = 0; w < 2; w++)
            if (set[w].d == d) {
                g->st_dc_hit++;
                if (w) { dcache_t t = set[0]; set[0] = set[1]; set[1] = t; }  /* most recent in way 0 */
                for (int j = 0; j < set[0].na; j++) { P[j] = set[0].p[j]; E[j] = set[0].e[j]; hot_see(g, P[j]); }
                return set[0].na;
            }
        g->st_dc_miss++;
    }
    n = pgi_trial(n, P, E, &k, &stopped);  /* step 1 */
    if (n > 1) {                            /* step 2: the leftover */
        if (pgi_dict_get(g, n) != NONE || stopped || n < (1ull << 32)) { P[k] = n; E[k++] = 1; }
        else {
            for (int j = 0; j < g->nhot && n > 1; j++) {  /* the bounded list of recent large primes */
                uint64_t q = n * g->hinv[j];
                if (q > g->hlim[j]) continue;  /* exact-division test: multiply, compare */
                uint32_t e = 0;
                do { n = q; e++; q = n * g->hinv[j]; } while (q <= g->hlim[j]);
                P[k] = g->hot[j]; E[k++] = e;
            }
            if (n > 1) {
                if (pgi_dict_get(g, n) != NONE || n < (1ull << 32) || pgi_is_prime(n)) { P[k] = n; E[k++] = 1; }
                else {  /* Miller-Rabin said composite: rho */
                    uint64_t fp[64];
                    uint32_t fe[64];
                    g->st_rho++;
                    int nf = pgi_factor_rest(n, fp, fe);
                    for (int j = 0; j < nf; j++) { P[k] = fp[j]; E[k++] = fe[j]; }
                }
            }
        }
    }
    for (int j = 0; j < k; j++) hot_see(g, P[j]);  /* every large prime of d has now been seen */
    if (set && k <= 7) {
        set[1] = set[0];
        set[0].d = d;
        set[0].na = (uint8_t)k;
        for (int j = 0; j < k; j++) { set[0].p[j] = P[j]; set[0].e[j] = (uint8_t)E[j]; }
    }
    return k;
}

pg_value *pgi_den_enter(pg_graph *g, uint64_t d) {
    if (d == 0) pgi_die("zero denominator entered");
    if (d == 1) return pgi_holder_new(g, 0, 1, NULL, 0, HK_TMP);
    uint64_t P[64];
    uint32_t E[64];
    int na = den_atoms(g, d, P, E);
    /* step 3: sharing */
    atom_t fresh[MAXE];
    uint32_t hs[MAXE];
    int nf = 0, nhs = 0;
    for (int j = 0; j < na; j++) {
        uint32_t id = pgi_dict_get(g, P[j]);
        if (id == NONE) {
            fresh[nf++] = (atom_t){P[j], 0, 0, E[j]};
            continue;
        }
        int dup = 0;
        for (int t = 0; t < nhs && !dup; t++) dup = hs[t] == id;
        if (!dup) hs[nhs++] = id;
    }
    for (int t = 0; t < nhs; t++) split_classes(g, hs[t], P, E, na, 0);
    if (nf) {
        norm_atoms(fresh, nf);
        uint32_t id = node_alloc(g);
        node_set_atoms(g, id, fresh, nf);
    }
    /* step 4: the edges, by lookups */
    uint32_t e[MAXE];
    int ne = 0;
    for (int j = 0; j < na; j++) {
        uint32_t id = pgi_dict_get(g, P[j]);
        uint32_t eh = 0;
        for (int i = 0; i < g->nc[id].na; i++)
            if (g->nc[id].at[i].p == P[j]) { eh = g->nc[id].at[i].e; break; }
        if (!eh || E[j] % eh) pgi_die("entry: exponent not a multiple of the node's");
        uint32_t w = E[j] / eh;
        int t;
        for (t = 0; t < ne; t++)
            if (EN(e[t]) == id) break;
        if (t < ne) {
            if (EW(e[t]) != w) pgi_die("entry: inconsistent ratio");
            continue;
        }
        e[ne++] = EP(id, w);
    }
    for (int a = 1; a < ne; a++)
        for (int b = a; b > 0 && EN(e[b - 1]) > EN(e[b]); b--) { uint32_t t = e[b]; e[b] = e[b - 1]; e[b - 1] = t; }
    return pgi_holder_new(g, 0, d, e, ne, HK_TMP);
}

/* ------------------------------------------------------------------------------------------------- context */

pg_graph *pg_graph_create(void) {
    pgi_tables_init();
    pg_graph *g = calloc(1, sizeof *g);
    if (!g) pgi_die("out of memory");
    for (int c = 0; c < 5; c++) g->pfree[c] = NONE;
    g->hfree = NONE;
    g->erfree = NONE;
    g->dsm = pgi_calloc(g, 65536 * sizeof *g->dsm);
    g->dbcap = 1024;
    g->dbk = pgi_calloc(g, g->dbcap * sizeof *g->dbk);
    g->dbv = pgi_alloc(g, g->dbcap * sizeof *g->dbv);
    return g;
}

void pg_graph_destroy(pg_graph *g) {
    if (!g) return;
    pgi_array_free_all(g);
    for (uint32_t id = 0; id < g->nnodes; id++) {
        pgi_free(g, g->nc[id].list);
        pgi_free(g, g->nc[id].at);
    }
    for (uint32_t c = 0; c < g->nhch; c++) pgi_free(g, g->hch[c]);
    pgi_free(g, g->hch);
    pgi_free(g, g->nh); pgi_free(g, g->nc); pgi_free(g, g->nfree);
    pgi_free(g, g->wsa); pgi_free(g, g->wsb);
    pgi_free(g, g->EPL); pgi_free(g, g->PPL);
    pgi_free(g, g->dsm); pgi_free(g, g->dbk); pgi_free(g, g->dbv);
    pgi_free(g, g->dc);
    pgi_free(g, g->arrs); pgi_free(g, g->pend);
    pgi_free(g, g->ER);
    free(g);
}


/* ------------------------------------------------------------------------------------------------- fault test */
#include "fault_impl.h"
