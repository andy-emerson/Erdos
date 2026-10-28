/* audit.c: the invariant audit (I1-I4 and the structure behind them), statistics, and node introspection. */
#include "pg_internal.h"

static int cmp_u64(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

/* a holder's edges: ascending, on live nodes, positive weights, multiplying to its denominator */
static const char *audit_edges(const pg_graph *g, const pg_value *h) {
    const uint32_t *e = HE(g, h);
    u128 p = 1;
    for (int k = 0; k < h->ne; k++) {
        uint32_t id = EN(e[k]);
        if (k && id <= EN(e[k - 1])) return "edges not ascending";
        if (id >= g->nnodes || !g->nh[id].alive) return "edge to a dead node";
        if (!EW(e[k])) return "edge of weight 0";
        for (uint32_t j = 0; j < EW(e[k]); j++) {
            p *= g->nh[id].v;
            if (p >> 64) return "denominator does not match its edges";
        }
    }
    return p == h->den ? NULL : "denominator does not match its edges";
}

static int lowest(int64_t num, uint64_t den) { return num == 0 ? den == 1 : pgi_gcd(mag64(num), den) == 1; }

const char *pg_audit(pg_graph *g) { return pgi_audit_opt(g, 0, 0); }

#if PGF_O2
static int has_rec(const pg_graph *g, const pg_value *h, uint32_t r) {
    for (int k = 0; k < h->ne; k++)
        if (g->EPL[h->eo + k] == r) return 1;
    return 0;
}
#endif

const char *pgi_audit_opt(pg_graph *g, int light, int midop) {
    const char *m;
    /* ---- nodes: atoms, values, I3, the dictionary */
    uint32_t live = 0, multi = 0;
    size_t edges_nodes = 0, natoms = 0;
    for (uint32_t id = 0; id < g->nnodes; id++) {
        const node_hot *N = &g->nh[id];
        const node_cold *C = &g->nc[id];
        if (!N->alive) {
            if (C->rc || C->len || C->list || C->at) return "a dead node holds a list or atoms";
            continue;
        }
        live++;
        if (C->rc != C->len) return "I3: reference count differs from the list length";
        if (C->rc == 0 && !midop) return "S5: an unused node was not freed";
        if (!C->na || C->na > 15) return "a node without atoms";
        u128 v = 1;
        uint32_t ge = 0;
        for (int i = 0; i < C->na; i++) {
            const atom_t *A = &C->at[i];
            if (!A->e) return "an atom with exponent 0";
            if (!light && !pgi_is_prime(A->p)) return "I1: an atom is not a proved prime";
            if (A->p < 2) return "I1: an atom below 2";
            if (A->lim != UINT64_MAX / A->p || ((A->p & 1) && A->p * A->inv != 1)) return "an atom's constants";
            for (uint32_t j = 0; j < A->e; j++) {
                v *= A->p;
                if (v >> 64) return "a node above 2^64";
            }
            ge = (uint32_t)pgi_gcd(ge, A->e);
            if (pgi_dict_get(g, A->p) != id) return "I1: the dictionary does not map an atom to its node";
            natoms++;
        }
        if (v != N->v || N->v < 2) return "a node's value is not the product of its atoms";
        if (ge != 1) return "a node's exponents are not normalised";
        if (N->multi != (C->na > 1)) return "a node's multi flag";
        uint64_t o = N->v >> N->s;
        if ((N->v & ((1ull << N->s) - 1)) || !(o & 1) || o * N->inv != 1 || N->lim != UINT64_MAX / o)
            return "a node's division constants";
        multi += C->na > 1;
        edges_nodes += C->len;
        if (C->len > C->lcap || (C->len && !C->list)) return "a node's list capacity";
        for (uint32_t i = 0; i < C->len; i++) {  /* I2 from the node side */
#if PGF_O2
            uint32_t r = C->list[i];
            if (r >= g->ern || !g->ER[r].w) return "I2: a list names a record that does not exist";
            if (g->ER[r].n != id) return "I2: a list names a record of another node";
            if (g->ER[r].pos != i) return "I2: a record's list position is wrong";
            uint32_t hid = g->ER[r].h;
            if (hid >= g->hn) return "I2: a record names a holder that does not exist";
            const pg_value *h = HP(g, hid);
            if (h->kind == HK_FREE) return "I2: a record names a released holder";
            if (!has_rec(g, h, r)) return "I2: a list names a record its holder does not reference";
#else
            uint32_t hid = C->list[i];
            if (hid >= g->hn) return "I2: a list names a holder that does not exist";
            const pg_value *h = HP(g, hid);
            if (h->kind == HK_FREE) return "I2: a list names a released holder";
            if (h->ne > MAXE || (h->ne && h->eo + h->ne > g->pool_n)) return "a holder's edge block";
            int k;
            for (k = 0; k < h->ne && EN(g->EPL[h->eo + k]) != id; k++) {}
            if (k == h->ne) return "I2: a list names a holder without the edge";
            if (g->PPL[h->eo + k] != i) return "I2: an edge's list position is wrong";
#endif
        }
    }
    if (live != g->nlive) return "the live node count";
    if (multi != g->nmulti) return "the multi node count";
    /* the dictionary holds exactly the live atoms, each once (so no atom is in two nodes: I1) */
    size_t dsmall = 0, dlarge = 0;
    for (uint32_t a = 0; a < 65536; a++) {
        if (!g->dsm[a]) continue;
        dsmall++;
        uint32_t id = g->dsm[a] - 1;
        if (id >= g->nnodes || !g->nh[id].alive) return "the dictionary names a dead node";
    }
    for (size_t i = 0; i < g->dbcap; i++)
        if (g->dbk[i]) {
            dlarge++;
            uint32_t id = g->dbv[i];
            if (id >= g->nnodes || !g->nh[id].alive) return "the dictionary names a dead node";
            if (pgi_dict_get(g, g->dbk[i]) != id) return "the dictionary's probing";
        }
    if (dlarge != g->dbcnt) return "the dictionary's count";
    if (dsmall + dlarge != natoms) return "I1: the dictionary does not match the atoms";
    /* I1 directly, for small graphs: pairwise gcds of the node values */
    if (!light && live <= 2000) {
        uint64_t *v = malloc((size_t)live * 8 + 8);
        size_t n = 0;
        for (uint32_t id = 0; id < g->nnodes; id++)
            if (g->nh[id].alive) v[n++] = g->nh[id].v;
        int bad = 0;
        for (size_t i = 0; i < n && !bad; i++)
            for (size_t j = i + 1; j < n && !bad; j++) bad = pgi_gcd(v[i], v[j]) != 1;
        free(v);
        if (bad) return "I1: two nodes share a factor";
    }
    /* ---- holders: I2 from the holder side, denominators, I4 for values */
    size_t edges_holders = 0, kinds[5] = {0}, hl = 0, pool = 0;
    for (uint32_t hid = 0; hid < g->hn; hid++) {
        const pg_value *h = HP(g, hid);
        if (h->kind == HK_FREE) continue;
        if (h->kind > HK_TMP) return "a holder of unknown kind";
        if (h->ne > MAXE) return "a holder with too many edges";
        if (h->ne && (h->eo >= g->pool_n || h->eo + h->ne > g->pool_n)) return "a holder's edge block";
        if (h->id != hid) return "a holder's id";
        hl++;
        kinds[h->kind]++;
        edges_holders += h->ne;
        if (h->ne) {
            if (h->cls > 4 || h->ne > (1 << h->cls)) return "a holder's edge block";
            pool += 1u << h->cls;
        } else if (h->cls != 0xFF) return "a holder's edge block";
#if PGF_O2
        for (int k = 0; k < h->ne; k++) {  /* the records before reading them */
            uint32_t r = g->EPL[h->eo + k];
            if (r >= g->ern || !g->ER[r].w) return "I2: a holder references a record that does not exist";
            if (g->ER[r].h != hid) return "I2: a record names another holder";
            if (g->ER[r].n >= g->nnodes || g->ER[r].w > 255) return "a record out of range";
        }
#endif
        if ((m = audit_edges(g, h))) return m;
        for (int k = 0; k < h->ne; k++) {
#if PGF_O2
            uint32_t r = g->EPL[h->eo + k], id = g->ER[r].n, pos = g->ER[r].pos;
            if (pos >= g->nc[id].len || g->nc[id].list[pos] != r) return "I2: an edge is not in its node's list";
#else
            uint32_t id = EN(g->EPL[h->eo + k]), pos = g->PPL[h->eo + k];
            if (pos >= g->nc[id].len || g->nc[id].list[pos] != hid) return "I2: an edge is not in its node's list";
#endif
        }
        if (h->kind == HK_VALUE) {
            if (h->den >= DEN_LIM) return "a value's denominator above the bounds";
            if (!lowest(h->num, h->den)) return "I4: a value not in lowest terms";
        }
        if (h->kind == HK_TMP && !midop) return "a transient holder outlived its entry";
    }
    if (hl != g->hlive) return "the live holder count";
    for (int k = 0; k < 5; k++)
        if (kinds[k] != g->hkind[k] && k != HK_FREE) return "the holder counts by kind";
    if (edges_holders != edges_nodes) return "I2: edge counts differ between the two sides";
#if PGF_O2
    if (g->erlive != edges_holders) return "I2: the live record count differs from the edges";
#endif
    if (pool != g->pool_used) return "the edge pool's accounting";
    /* ---- arrays: layout, I4 per array */
    size_t nden = 0, nexc = 0, npend = 0;
    for (size_t k = 0; k < g->narr; k++) {
        const pg_array *A = g->arrs[k];
        if (A->gpos != k) return "an array's list position";
        if (A->dh) {
            nden++;
            if (A->dh->kind != HK_DEN || A->dh->den != A->D) return "an array's D does not match its holder";
            if (A->D < 2 || A->D >= DEN_LIM) return "an array's D out of range";
        } else if (A->D != 1) return "an array's D without a holder";
        if (A->deferred) {
            npend++;
            if (!g->expr_depth || A->ppos >= g->npend || g->pend[A->ppos] != A) return "a deferred array outside an expression";
        }
        for (size_t j = 0; j < A->nx; j++) {
            nexc++;
            const pg_value *h = A->xh[j];
            if (A->xi[j] >= A->n || (j && A->xi[j] <= A->xi[j - 1])) return "exception indices not ascending";
            if (!midop && A->num[A->xi[j]] != 0) return "an exception's dense slot is not 0";
            if (h->kind != HK_EXC) return "an exception's holder kind";
            if (h->den >= DEN_LIM) return "an exception's denominator above the bounds";
            if (!midop && !A->deferred && !lowest(h->num, h->den)) return "I4: an exception not in lowest terms";
        }
        if (!midop && !A->deferred && A->D > 1) {  /* I4 per array: D and the dense numerators share no factor */
            uint64_t t = A->D;
            for (size_t i = 0; i < A->n && t > 1; i++)
                if (A->num[i]) t = pgi_gcd(t, mag64(A->num[i]));
            if (t != 1) return "I4: an array not in lowest terms per array";
        }
    }
    if (nden != g->hkind[HK_DEN] || nexc != g->hkind[HK_EXC]) return "array holders do not match the holder counts";
    if (npend != g->npend) return "the open expression's array list";
    if (g->npins && !midop) return "a pinned edge list outlived its operation";
    return NULL;
}

void pg_stats_get(const pg_graph *g, pg_stats *s) {
    memset(s, 0, sizeof *s);
    s->nodes_live = g->nlive;
    s->nodes_multi = g->nmulti;
    s->node_slots = g->nnodes;
    s->nodes_created = g->st_nodes_new;
    s->nodes_freed = g->st_nodes_freed;
    s->holders_live = g->hlive;
    s->values_live = g->hkind[HK_VALUE];
    s->arrays_live = g->narr;
    size_t e = 0;
    for (uint32_t id = 0; id < g->nnodes; id++) e += g->nc[id].len;
    s->edges_live = e;
    s->dict_entries = g->dbcnt;
    s->splits_entry = g->st_split_entry;
    s->splits_reduce = g->st_split_reduce;
    s->split_holders = g->st_split_holders;
    s->restarts = g->st_restarts;
    s->v4_retries = g->st_v4_retries;
    s->rho_calls = g->st_rho;
    s->den_cache_hits = g->st_dc_hit;
    s->den_cache_misses = g->st_dc_miss;
    s->mem_bytes = g->mem_cur;
    s->mem_peak = g->mem_peak;
}

size_t pg_debug_nodes(const pg_graph *g, uint64_t *vals, size_t cap) {
    uint64_t *v = malloc((size_t)g->nlive * 8 + 8);
    size_t n = 0;
    for (uint32_t id = 0; id < g->nnodes; id++)
        if (g->nh[id].alive) v[n++] = g->nh[id].v;
    qsort(v, n, 8, cmp_u64);
    for (size_t i = 0; i < n && i < cap; i++) vals[i] = v[i];
    free(v);
    return n;
}
