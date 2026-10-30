/* fault_impl.h: the consistency variants' production checks (verify, rebuild, audit) and the fault injectors.
 * Included at the end of graph.c, so that it reaches graph.c's static helpers (dictionary, lists, records). */
#include <unistd.h>
#include <stddef.h>

#include "pgfault.h"

pgf_trial *pgf_rec = NULL;
uint64_t pgf_K = 0, pgf_ops = 0, pgf_nsplits = 0, pgf_rng = 1;
pgf_last_t pgf_last;
static uint64_t f8_left = 0;  /* F8 armed: the refinement count until the interrupted one */

const char *pgf_variant(void) {
    static const char *v[] = {"N", "O1", "O2", "O3", "J"};
    return v[PGF_POLICY];
}

static uint64_t rnd(void) {  /* splitmix64 */
    uint64_t z = (pgf_rng += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}
static uint64_t rndn(uint64_t n) { return n ? rnd() % n : 0; }

static void rec_str(char *dst, size_t cap, const char *s) {
    size_t n = strlen(s);
    if (n >= cap) n = cap - 1;
    memcpy(dst, s, n);
    dst[n] = 0;
}

void pgf_on_die(const char *msg) {
    if (pgf_rec && !pgf_rec->halt_kind) {
        pgf_rec->halt_kind = 2;
        rec_str(pgf_rec->halt_msg, sizeof pgf_rec->halt_msg, msg);
    }
}

__attribute__((noreturn)) static void pgf_halt(const char *what, const char *msg) {
    char b[440];
    snprintf(b, sizeof b, "%s: %s", what, msg);
    if (pgf_rec && !pgf_rec->halt_kind) {
        pgf_rec->halt_kind = 1;
        rec_str(pgf_rec->halt_msg, sizeof pgf_rec->halt_msg, b);
    }
    fprintf(stderr, "primegraph: integrity check failed: %s\n", b);
    fflush(stderr);
    _exit(PGF_EXIT_HALT);
}

/* ------------------------------------------------------------------------------------------------- rebuild */

#if PGF_REPAIR
/* Rebuild the derived index from the truth: the holders' factorizations (O2: the records they reference) and the
 * nodes' atoms.  Derived: node liveness, values and division constants, the dictionary, holder lists, reference
 * counts, list positions, each holder's cached p^a, each array's cached D, and the counters.  Anything that cannot be
 * rebuilt (an edge to a node without atoms, an atom in two nodes, edges out of order) is left for the audit that
 * follows, which then halts. */
static void pgf_rebuild(pg_graph *g, int midop) {
    /* 1. nodes: alive iff they have atoms; their derived data; empty lists */
    g->nlive = 0;
    g->nmulti = 0;
    g->nfree_n = 0;
    for (uint32_t id = 0; id < g->nnodes; id++) {
        node_cold *C = &g->nc[id];
        node_hot *N = &g->nh[id];
        if (!C->na || !C->at || C->na > MAXE) {
            if (N->alive || C->list) {  /* a node marked live without atoms: nothing to keep */
                pgi_free(g, C->list);
                if (C->na <= MAXE) pgi_free(g, C->at);
            }
            memset(C, 0, sizeof *C);
            memset(N, 0, sizeof *N);
            if (g->nfree_n == g->nfree_cap) {
                g->nfree_cap = g->nfree_cap ? 2 * g->nfree_cap : 64;
                g->nfree = pgi_realloc(g, g->nfree, (size_t)g->nfree_cap * 4);
            }
            g->nfree[g->nfree_n++] = id;
            continue;
        }
        u128 v = 1;
        for (int i = 0; i < C->na && v; i++) {
            atom_t *A = &C->at[i];
            if (A->p < 2) { v = 0; break; }
            A->inv = (A->p & 1) ? pgi_inv64(A->p) : 1;
            A->lim = UINT64_MAX / A->p;
            for (uint32_t j = 0; j < A->e && v; j++) {
                v *= A->p;
                if (v >> 64) v = 0;
            }
        }
        N->alive = 1;
        if (v > 1) {
            N->v = (uint64_t)v;
            N->s = (uint8_t)__builtin_ctzll(N->v);
            uint64_t m = N->v >> N->s;
            N->inv = pgi_inv64(m);
            N->lim = UINT64_MAX / m;
        }
        N->multi = C->na > 1;
        C->len = 0;
        C->rc = 0;
        if (C->lcap && !C->list) C->lcap = 0;
        g->nlive++;
        g->nmulti += C->na > 1;
    }
    /* 2. the dictionary */
    memset(g->dsm, 0, 65536 * sizeof *g->dsm);
    memset(g->dbk, 0, g->dbcap * sizeof *g->dbk);
    g->dbcnt = 0;
    for (uint32_t id = 0; id < g->nnodes; id++)
        if (g->nh[id].alive)
            for (int i = 0; i < g->nc[id].na; i++)
                if (pgi_dict_get(g, g->nc[id].at[i].p) == NONE) dict_set(g, g->nc[id].at[i].p, id);
    /* 3. the lists, reference counts and positions, from the holders */
#if PGF_O2
    uint8_t *mark = calloc(g->ern + 1, 1);
#endif
    g->hlive = 0;
    memset(g->hkind, 0, sizeof g->hkind);
    g->pool_used = 0;
    for (uint32_t hid = 0; hid < g->hn; hid++) {
        pg_value *h = HP(g, hid);
        if (h->kind == HK_FREE) continue;
        g->hlive++;
        if (h->kind <= HK_TMP) g->hkind[h->kind]++;
        if (!h->ne || h->ne > MAXE || h->eo >= g->pool_n || h->eo + h->ne > g->pool_n) continue;
        if (h->cls <= 4) g->pool_used += 1u << h->cls;
        for (int k = 0; k < h->ne; k++) {
#if PGF_O2
            uint32_t r = g->EPL[h->eo + k];
            if (r >= g->ern) continue;
            mark[r] = 1;
            g->ER[r].h = hid;  /* the holder's reference names the record's holder */
            uint32_t id = g->ER[r].n;
            if (id >= g->nnodes || !g->nh[id].alive) continue;
            node_cold *C = &g->nc[id];
            if (C->len == C->lcap) list_grow(g, C);
            g->ER[r].pos = C->len;
            C->list[C->len++] = r;
            C->rc++;
#else
            uint32_t id = EN(g->EPL[h->eo + k]);
            if (id >= g->nnodes || !g->nh[id].alive) continue;
            node_cold *C = &g->nc[id];
            if (C->len == C->lcap) list_grow(g, C);
            g->PPL[h->eo + k] = C->len;
            C->list[C->len++] = hid;
            C->rc++;
#endif
        }
    }
#if PGF_O2
    /* records referenced by no holder (strays) are freed; the free list is rebuilt */
    g->erfree = NONE;
    g->erlive = 0;
    for (uint32_t r = g->ern; r-- > 0;) {
        if (mark[r]) { g->erlive++; continue; }
        g->ER[r] = (er_t){g->erfree, NONE, 0, 0};
        g->erfree = r;
    }
    free(mark);
#endif
    /* 4. nodes left without holders are freed (S5); not in the middle of an operation, which may still be about to
     *    link holders to them (a refinement's new part, found by the dictionary) */
    for (uint32_t id = 0; id < g->nnodes && !midop; id++)
        if (g->nh[id].alive && g->nc[id].rc == 0) node_release(g, id);
    /* 5. each holder's cached p^a, and each array's cached D */
    for (uint32_t hid = 0; hid < g->hn; hid++) {
        pg_value *h = HP(g, hid);
        if (h->kind == HK_FREE || h->ne > MAXE || (h->ne && h->eo + h->ne > g->pool_n)) continue;
        const uint32_t *e = HE(g, h);
        u128 p = 1;
        int ok = 1;
        for (int k = 0; k < h->ne && ok; k++) {
            uint32_t id = EN(e[k]);
            if (id >= g->nnodes || !g->nh[id].alive) { ok = 0; break; }
            for (uint32_t j = 0; j < EW(e[k]) && ok; j++) {
                p *= g->nh[id].v;
                if (p >> 64) ok = 0;
            }
        }
        if (ok) h->den = (uint64_t)p;
    }
    for (size_t k = 0; k < g->narr; k++) {
        pg_array *A = g->arrs[k];
        A->gpos = k;
        A->D = A->dh ? A->dh->den : 1;
    }
}
#endif

/* ------------------------------------------------------------------------------------------------- the checks */

#if PGF_JOURNAL
static int same_atoms(const node_cold *C, const atom_t *A, int n) {
    if (C->na != n) return 0;
    for (int i = 0; i < n; i++)
        if (C->at[i].p != A[i].p || C->at[i].e != A[i].e) return 0;
    return 1;
}
/* J: finish an interrupted refinement from its journal, idempotently: the atoms as journaled, then every holder of
 * N that has no edge to P yet gets the update (a holder that already has its P edge is done).  P's list is N's
 * list copied (M1's copy step), so a holder's position in P's list is its position in N's. */
static int pgf_rollforward(pg_graph *g) {
    if (!g->jr.active) return 0;
    uint32_t N = g->jr.N, P = g->jr.P, gQ = g->jr.gQ, gR = g->jr.gR;
    if (!same_atoms(&g->nc[P], g->jr.Q, g->jr.nq)) node_set_atoms(g, P, g->jr.Q, g->jr.nq);
    if (!same_atoms(&g->nc[N], g->jr.R, g->jr.nr)) node_set_atoms(g, N, g->jr.R, g->jr.nr);
    node_cold *CN = &g->nc[N], *CP = &g->nc[P];
    if (!CP->list || CP->len != CN->len) {  /* the copy step itself did not finish */
        CP->lcap = CN->len < 4 ? 4 : CN->len;
        CP->list = pgi_realloc(g, CP->list, (size_t)CP->lcap * 4);
        memcpy(CP->list, CN->list, (size_t)CN->len * 4);
        CP->len = CN->len;
        CP->rc = CN->rc;
    }
    for (uint32_t i = 0; i < CN->len; i++) {
        pg_value *h = HP(g, g->nc[N].list[i]);
        if (h_find(g, h, P) >= 0) continue;  /* done before the interruption */
        int k = h_find(g, h, N);
        if (k < 0) continue;  /* not this refinement's damage: left to the audit */
        uint32_t w = EW(g->EPL[h->eo + k]);
        if (w * gR > 255 || w * gQ > 255) continue;
        g->EPL[h->eo + k] = EP(N, w * gR);
        h_insert_edge(g, h, P, w * gQ, i);
    }
    pin_patch(g, N, P, gQ, gR);
    g->jr.active = 0;
    return 1;
}
#endif


static void pgf_verify(pg_graph *g, int midop) {
    if (pgf_rec) { pgf_rec->nverify++; pgf_rec->in_check = 1; }
#if PGF_JOURNAL
    if (pgf_rollforward(g) && pgf_rec) {
        if (!pgf_rec->nrepair) {
            rec_str(pgf_rec->repair_msg, sizeof pgf_rec->repair_msg, "journal: an interrupted refinement rolled forward");
            pgf_rec->repair_cmd = pgf_rec->cur_cmd;
        }
        pgf_rec->nrepair++;
        pgf_rec->nroll++;
    }
#endif
    const char *m = pgi_audit_opt(g, 1, midop);
    if (!m) { if (pgf_rec) pgf_rec->in_check = 0; return; }
#if PGF_REPAIR
    char first[200];
    rec_str(first, sizeof first, m);
    pgf_rebuild(g, midop);
    const char *m2 = pgi_audit_opt(g, 1, midop);
    if (m2) {
        char b[440];
        snprintf(b, sizeof b, "%s (after rebuild: %s)", first, m2);
        pgf_halt("verify: unrepairable", b);
    }
    if (pgf_rec) {
        if (!pgf_rec->nrepair) {
            rec_str(pgf_rec->repair_msg, sizeof pgf_rec->repair_msg, first);
            pgf_rec->repair_cmd = pgf_rec->cur_cmd;
        }
        pgf_rec->nrepair++;
    }
    if (pgf_rec) pgf_rec->in_check = 0;
#else
    pgf_halt("audit", m);
#endif
}

void pgf_final_check(pg_graph *g) {  /* the verification before the results are read out */
#if PGF_POLICY != 0
    pgf_verify(g, 0);
#else
    (void)g;
#endif
}

void pgf_periodic(pg_graph *g) {
#if PGF_POLICY != 0
    pgf_verify(g, 0);
#else
    (void)g;
#endif
}

void pgf_before_split(pg_graph *g) {
#if PGF_POLICY != 0
    pgf_verify(g, 1);
#else
    (void)g;
#endif
}

int pgf_split_interrupt(uint32_t *len) {
    pgf_nsplits++;
    if (!f8_left || --f8_left) return 0;
    uint32_t full = *len;
    *len = full / 2;
    if (pgf_rec) {
        pgf_rec->injected = 1;
        pgf_rec->inj_cmd = pgf_rec->cur_cmd;
        snprintf(pgf_rec->inj_desc, sizeof pgf_rec->inj_desc, "F8 refinement stopped after %u of %u holders",
                 *len, full);
    }
    pgf_last.cls = 8;
    pgf_last.pos = *len;
    pgf_last.old = full;
    return 1;
}

void pgf_arm_f8(uint64_t n) { f8_left = n; }

/* ------------------------------------------------------------------------------------------------- injectors */

static int pick_node(pg_graph *g, uint32_t minlen, uint32_t *out) {
    uint32_t n = 0;
    for (uint32_t id = 0; id < g->nnodes; id++)
        if (g->nh[id].alive && g->nc[id].len >= minlen) n++;
    if (!n) return 0;
    uint32_t t = (uint32_t)rndn(n);
    for (uint32_t id = 0; id < g->nnodes; id++)
        if (g->nh[id].alive && g->nc[id].len >= minlen && !t--) { *out = id; return 1; }
    return 0;
}
static int pick_holder(pg_graph *g, int minedges, pg_value **out) {
    uint32_t n = 0;
    for (uint32_t hid = 0; hid < g->hn; hid++) {
        pg_value *h = HP(g, hid);
        if (h->kind != HK_FREE && h->ne >= minedges) n++;
    }
    if (!n) return 0;
    uint32_t t = (uint32_t)rndn(n);
    for (uint32_t hid = 0; hid < g->hn; hid++) {
        pg_value *h = HP(g, hid);
        if (h->kind != HK_FREE && h->ne >= minedges && !t--) { *out = h; return 1; }
    }
    return 0;
}
static uint32_t edge_node(pg_graph *g, pg_value *h, int k) {
#if PGF_O2
    return g->ER[g->EPL[h->eo + k]].n;
#else
    return EN(g->EPL[h->eo + k]);
#endif
}
static uint32_t edge_w(pg_graph *g, pg_value *h, int k) {
#if PGF_O2
    return g->ER[g->EPL[h->eo + k]].w;
#else
    return EW(g->EPL[h->eo + k]);
#endif
}
static int has_edge(pg_graph *g, pg_value *h, uint32_t node) {
    for (int k = 0; k < h->ne; k++)
        if (edge_node(g, h, k) == node) return 1;
    return 0;
}
static uint32_t wrong_weight(uint32_t w) {  /* w - 1, w + 1 or w + 2, at least 1, never w */
    for (;;) {
        uint32_t c = (uint32_t)rndn(3), x = c == 0 ? w - 1 : c == 1 ? w + 1 : w + 2;
        if (x >= 1 && x <= 255 && x != w) return x;
    }
}

/* F9: every byte of the graph's live content, by region (nodes, atoms, lists, holders, factorizations, the
 * dictionary, numerators, arrays).  Free capacity, empty dictionary slots and the entry caches are not included. */
typedef void (*region_cb)(void *ctx, void *p, size_t n, const char *tag);
static void enum_regions(pg_graph *g, region_cb cb, void *ctx) {
    for (uint32_t id = 0; id < g->nnodes; id++) {
        if (!g->nh[id].alive) continue;
        node_cold *C = &g->nc[id];
        cb(ctx, &g->nh[id], sizeof g->nh[id], "node");
        cb(ctx, C, sizeof *C, "node");
        if (C->na && C->at) cb(ctx, C->at, (size_t)C->na * sizeof(atom_t), "atoms");
        if (C->len && C->list) cb(ctx, C->list, (size_t)C->len * 4, "list");
    }
    for (uint32_t hid = 0; hid < g->hn; hid++) {
        pg_value *h = HP(g, hid);
        if (h->kind == HK_FREE) continue;
        cb(ctx, &h->num, sizeof h->num, "numerator");
        cb(ctx, &h->den, sizeof h->den, "cached_den");
        cb(ctx, &h->eo, sizeof *h - offsetof(pg_value, eo), "holder_meta");
        if (!h->ne) continue;
        cb(ctx, g->EPL + h->eo, (size_t)h->ne * 4, "factorization");
#if PGF_O2
        for (int k = 0; k < h->ne; k++) cb(ctx, &g->ER[g->EPL[h->eo + k]], sizeof(er_t), "record");
#else
        cb(ctx, g->PPL + h->eo, (size_t)h->ne * 4, "positions");
#endif
    }
    for (uint32_t a = 0; a < 65536; a++)
        if (g->dsm[a]) cb(ctx, &g->dsm[a], sizeof g->dsm[a], "dictionary");
    for (size_t i = 0; i < g->dbcap; i++)
        if (g->dbk[i]) {
            cb(ctx, &g->dbk[i], sizeof g->dbk[i], "dictionary");
            cb(ctx, &g->dbv[i], sizeof g->dbv[i], "dictionary");
        }
    for (size_t k = 0; k < g->narr; k++) {
        pg_array *A = g->arrs[k];
        cb(ctx, A, sizeof *A, "array_meta");
        if (A->n) cb(ctx, A->num, A->n * 8, "numerator");
        if (A->nx) {
            cb(ctx, A->xi, A->nx * 4, "array_meta");
            cb(ctx, A->xh, A->nx * sizeof *A->xh, "array_meta");
        }
    }
}
typedef struct { size_t total, target; uint8_t *hit; const char *tag; } f9ctx;
static void f9_count(void *c, void *p, size_t n, const char *tag) {
    (void)p; (void)tag;
    ((f9ctx *)c)->total += n;
}
static void f9_find(void *c, void *p, size_t n, const char *tag) {
    f9ctx *x = c;
    if (!x->hit && x->target < n) { x->hit = (uint8_t *)p + x->target; x->tag = tag; }
    else if (!x->hit) x->target -= n;
}

int pgf_inject(pg_graph *g, int cls) {
    char d[200] = "";
    memset(&pgf_last, 0, sizeof pgf_last);
    pgf_last.cls = cls;
    switch (cls) {
    case 1: {  /* drop one holder id from a node's list (as a buggy unlink would: the list stays compact) */
        uint32_t N;
        if (!pick_node(g, 1, &N)) return 0;
        node_cold *C = &g->nc[N];
        uint32_t p = (uint32_t)rndn(C->len), victim = C->list[p], last = C->list[C->len - 1];
        C->list[p] = last;
        if (p != C->len - 1) {
#if PGF_O2
            g->ER[last].pos = p;
#else
            pg_value *L = HP(g, last);
            int kk = h_find(g, L, N);
            if (kk >= 0) g->PPL[L->eo + kk] = p;
#endif
        }
        C->len--;
        C->rc--;
#if PGF_O2
        pgf_last.holder = g->ER[victim].h;
#else
        pgf_last.holder = victim;
#endif
        pgf_last.node = N;
        pgf_last.pos = p;
        snprintf(d, sizeof d, "F1 node %u: dropped list entry %u (holder %u); len %u", N, p, pgf_last.holder, C->len);
        break;
    }
    case 2: {  /* a stray holder id in a node's list */
        uint32_t N;
        pg_value *h = NULL;
        int found = 0;
        for (int t = 0; t < 64 && !found; t++) {
            if (!pick_node(g, 0, &N) || !pick_holder(g, 0, &h)) return 0;
            found = !has_edge(g, h, N);
        }
        if (!found) return 0;
        node_cold *C = &g->nc[N];
        if (C->len == C->lcap) list_grow(g, C);
#if PGF_O2
        uint32_t r = er_new(g, h->id, N, 1 + (uint32_t)rndn(3));
        g->ER[r].pos = C->len;
        C->list[C->len++] = r;
#else
        C->list[C->len++] = h->id;
#endif
        C->rc++;
        pgf_last.node = N;
        pgf_last.holder = h->id;
        snprintf(d, sizeof d, "F2 node %u: stray holder %u appended; len %u", N, h->id, C->len);
        break;
    }
    case 3: {  /* a wrong node id in a holder's factorization */
        pg_value *h;
        uint32_t M;
        int found = 0;
        for (int t = 0; t < 64 && !found; t++) {
            if (!pick_holder(g, 1, &h) || !pick_node(g, 1, &M)) return 0;
            found = !has_edge(g, h, M);
        }
        if (!found) return 0;
        int k = (int)rndn(h->ne);
        uint32_t old = edge_node(g, h, k);
#if PGF_O2
        g->ER[g->EPL[h->eo + k]].n = M;
#else
        g->EPL[h->eo + k] = EP(M, EW(g->EPL[h->eo + k]));
#endif
        pgf_last.holder = h->id;
        pgf_last.old = old;
        pgf_last.new_ = M;
        pgf_last.pos = (uint32_t)k;
        snprintf(d, sizeof d, "F3 holder %u edge %d: node %u -> %u", h->id, k, old, M);
        break;
    }
    case 4: {  /* a wrong exponent in a holder's factorization */
        pg_value *h;
        if (!pick_holder(g, 1, &h)) return 0;
        int k = (int)rndn(h->ne);
        uint32_t w = edge_w(g, h, k), x = wrong_weight(w), n = edge_node(g, h, k);
#if PGF_O2
        g->ER[g->EPL[h->eo + k]].w = x;
#else
        g->EPL[h->eo + k] = EP(n, x);
#endif
        pgf_last.holder = h->id;
        pgf_last.node = n;
        pgf_last.old = w;
        pgf_last.new_ = x;
        snprintf(d, sizeof d, "F4 holder %u edge to node %u: weight %u -> %u", h->id, n, w, x);
        break;
    }
    case 5: {  /* a wrong list position (index side only): the position of one edge in its node's list, which M1
                * keeps in the holder's position array (PPL) and O2 in the shared record (ER.pos); the exponent and
                * the membership are untouched */
        pg_value *h;
        if (!pick_holder(g, 1, &h)) return 0;
        int k = (int)rndn(h->ne);
        uint32_t n = edge_node(g, h, k), len = g->nc[n].len;
#if PGF_O2
        uint32_t *pp = &g->ER[g->EPL[h->eo + k]].pos;
#else
        uint32_t *pp = &g->PPL[h->eo + k];
#endif
        uint32_t old = *pp, x;
        if (len >= 2) { do x = (uint32_t)rndn(len); while (x == old); }
        else x = old + 1;  /* a list of one: the only wrong position is out of range */
        *pp = x;
        pgf_last.holder = h->id;
        pgf_last.node = n;
        pgf_last.pos = (uint32_t)k;
        pgf_last.old = old;
        pgf_last.new_ = x;
        snprintf(d, sizeof d, "F5 holder %u edge to node %u: list position %u -> %u (len %u)", h->id, n, old, x, len);
        break;
    }
    case 6: {  /* a corrupted reference count */
        uint32_t N;
        if (!pick_node(g, 1, &N)) return 0;
        uint32_t rc = g->nc[N].rc, x;
        for (;;) {
            int64_t dl = (int64_t)rndn(4);
            dl = dl < 2 ? dl - 2 : dl - 1;  /* -2, -1, +1, +2 */
            if ((int64_t)rc + dl >= 0) { x = (uint32_t)((int64_t)rc + dl); break; }
        }
        g->nc[N].rc = x;
        pgf_last.node = N;
        pgf_last.old = rc;
        pgf_last.new_ = x;
        snprintf(d, sizeof d, "F6 node %u: reference count %u -> %u", N, rc, x);
        break;
    }
    case 7: {  /* a dictionary entry names the wrong node */
        if (g->nlive < 2) return 0;
        uint32_t A, B;
        if (!pick_node(g, 0, &A)) return 0;
        do { if (!pick_node(g, 0, &B)) return 0; } while (B == A);
        uint64_t q = g->nc[A].at[rndn(g->nc[A].na)].p;
        dict_set(g, q, B);
        pgf_last.node = A;
        pgf_last.node2 = B;
        pgf_last.atom = q;
        snprintf(d, sizeof d, "F7 atom %llu: node %u -> %u", (unsigned long long)q, A, B);
        break;
    }
    case 9: {  /* one bit flipped anywhere in the graph's live content */
        f9ctx c = {0, 0, NULL, NULL};
        enum_regions(g, f9_count, &c);
        if (!c.total) return 0;
        c.target = rndn(c.total);
        enum_regions(g, f9_find, &c);
        if (!c.hit) return 0;
        int bit = (int)rndn(8);
        uint8_t ob = *c.hit;
        *c.hit = (uint8_t)(ob ^ (1u << bit));
        pgf_last.addr = c.hit;
        pgf_last.ob = ob;
        pgf_last.nb = *c.hit;
        snprintf(d, sizeof d, "F9 %s: bit %d of a byte (0x%02x -> 0x%02x), %zu live bytes", c.tag, bit, ob, *c.hit,
                 c.total);
        if (pgf_rec) rec_str(pgf_rec->inj_region, sizeof pgf_rec->inj_region, c.tag);
        break;
    }
    default:
        return 0;
    }
    if (pgf_rec) {
        pgf_rec->injected = 1;
        pgf_rec->inj_cmd = pgf_rec->cur_cmd;
        rec_str(pgf_rec->inj_desc, sizeof pgf_rec->inj_desc, d);
    }
    return 1;
}

/* ------------------------------------------------------------------------------------------------- test probes */

int pgf_probe(pg_graph *g, uint32_t node, uint32_t hid, uint32_t *len, uint32_t *rc, uint32_t *w, int *in_list) {
    if (node >= g->nnodes || !g->nh[node].alive) return 0;
    node_cold *C = &g->nc[node];
    *len = C->len;
    *rc = C->rc;
    *w = 0;
    *in_list = 0;
    if (hid < g->hn) {
        pg_value *h = HP(g, hid);
        for (int k = 0; k < h->ne; k++)
            if (edge_node(g, h, k) == node) *w = edge_w(g, h, k);
    }
    for (uint32_t i = 0; i < C->len; i++) {
#if PGF_O2
        if (g->ER[C->list[i]].h == hid) *in_list = 1;
#else
        if (C->list[i] == hid) *in_list = 1;
#endif
    }
    return 1;
}
uint32_t pgf_dict(pg_graph *g, uint64_t atom) { return pgi_dict_get(g, atom); }
uint32_t pgf_value_id(const pg_value *v) { return v->id; }

/* ------------------------------------------------------------------------------------------------- self-check */

/* Inject class cls and confirm, against a snapshot taken just before, that exactly the intended change happened and
 * nothing else in the index or the truth changed.  Returns 1 (msg: what changed) or 0 (msg: why not); -1 if the class
 * does not apply.  Used by test_fault.py through the driver's ZC command. */
typedef struct { uint8_t alive; uint32_t len, rc, na; uint32_t *hl; } snode;
typedef struct { uint8_t kind, ne; uint32_t e[MAXE]; } shold;
static uint32_t list_holder(pg_graph *g, uint32_t entry) {
#if PGF_O2
    return entry < g->ern ? g->ER[entry].h : NONE;
#else
    (void)g;
    return entry;
#endif
}
static int cmp32(const void *a, const void *b) {
    uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    return (x > y) - (x < y);
}
int pgf_selfcheck(pg_graph *g, int cls, char *msg, size_t cap) {
    uint32_t nn = g->nnodes, hn = g->hn;
    snode *S = calloc(nn + 1, sizeof *S);
    shold *H = calloc(hn + 1, sizeof *H);
    for (uint32_t id = 0; id < nn; id++) {
        S[id].alive = g->nh[id].alive;
        S[id].len = g->nc[id].len;
        S[id].rc = g->nc[id].rc;
        S[id].na = g->nc[id].na;
        S[id].hl = malloc(4 * (size_t)S[id].len + 4);
        for (uint32_t i = 0; i < S[id].len; i++) S[id].hl[i] = list_holder(g, g->nc[id].list[i]);
        qsort(S[id].hl, S[id].len, 4, cmp32);
    }
    for (uint32_t hid = 0; hid < hn; hid++) {
        pg_value *h = HP(g, hid);
        H[hid].kind = h->kind;
        H[hid].ne = h->ne;
        if (h->kind != HK_FREE && h->ne) memcpy(H[hid].e, HE(g, h), 4 * (size_t)h->ne);
    }
    /* the dictionary over every live atom */
    size_t nat = 0;
    for (uint32_t id = 0; id < nn; id++) if (S[id].alive) nat += S[id].na;
    uint64_t *at = malloc(8 * nat + 8);
    uint32_t *dv = malloc(4 * nat + 4);
    nat = 0;
    for (uint32_t id = 0; id < nn; id++)
        if (S[id].alive)
            for (int i = 0; i < g->nc[id].na; i++) { at[nat] = g->nc[id].at[i].p; dv[nat] = pgi_dict_get(g, at[nat]); nat++; }
    int rc = pgf_inject(g, cls);
    int ok = 1;
    char why[160] = "";
    if (rc != 1) { ok = rc; snprintf(why, sizeof why, "inject returned %d", rc); goto out; }
    if (cls == 9) goto f9;  /* a flipped bit may break any pointer: only the byte itself is checked */
    if (g->nnodes != nn || g->hn != hn) { ok = 0; snprintf(why, sizeof why, "node or holder table changed"); goto out; }
    int dn = 0, dl = 0, dh = 0, dd = 0;  /* nodes whose len/rc changed, lists changed, holders changed, dict changed */
    uint32_t cn = NONE, ch = NONE;
    for (uint32_t id = 0; id < nn; id++) {
        if (!S[id].alive) continue;
        uint32_t len = g->nc[id].len, *cur = malloc(4 * (size_t)len + 4);
        for (uint32_t i = 0; i < len; i++) cur[i] = list_holder(g, g->nc[id].list[i]);
        qsort(cur, len, 4, cmp32);
        if (len != S[id].len || g->nc[id].rc != S[id].rc) { dn++; cn = id; }
        if (len != S[id].len || memcmp(cur, S[id].hl, 4 * (size_t)len)) dl++;
        free(cur);
    }
    for (uint32_t hid = 0; hid < hn; hid++) {
        pg_value *h = HP(g, hid);
        if (h->kind != H[hid].kind || h->ne != H[hid].ne ||
            (h->ne && h->kind != HK_FREE && memcmp(HE(g, h), H[hid].e, 4 * (size_t)h->ne))) { dh++; ch = hid; }
    }
    for (size_t i = 0; i < nat; i++) if (pgi_dict_get(g, at[i]) != dv[i]) dd++;
    uint32_t L = pgf_last.node, V = pgf_last.holder;
    switch (cls) {
    case 1: {
        uint32_t len, rcn, w;
        int inl;
        pgf_probe(g, L, V, &len, &rcn, &w, &inl);
        ok = dn == 1 && cn == L && len == S[L].len - 1 && rcn == S[L].rc - 1 && dl == 1 && dh == 0 && dd == 0 &&
             w > 0 && !inl;
        snprintf(why, sizeof why, "node %u len %u->%u rc %u->%u; holder %u keeps its edge (w %u), listed %d; %d/%d/%d/%d",
                 L, S[L].len, len, S[L].rc, rcn, V, w, inl, dn, dl, dh, dd);
        break;
    }
    case 2: {
        uint32_t len, rcn, w;
        int inl;
        pgf_probe(g, L, V, &len, &rcn, &w, &inl);
        ok = dn == 1 && cn == L && len == S[L].len + 1 && rcn == S[L].rc + 1 && dl == 1 && dh == 0 && dd == 0 &&
             w == 0 && inl;
        snprintf(why, sizeof why, "node %u len %u->%u; stray holder %u listed %d without an edge (w %u); %d/%d/%d/%d",
                 L, S[L].len, len, V, inl, w, dn, dl, dh, dd);
        break;
    }
    case 5: {
        pg_value *h = HP(g, V);
#if PGF_O2
        uint32_t cur = g->ER[g->EPL[h->eo + pgf_last.pos]].pos;
#else
        uint32_t cur = g->PPL[h->eo + pgf_last.pos];
#endif
        ok = cur == pgf_last.new_ && pgf_last.new_ != pgf_last.old && dn == 0 && dl == 0 && dh == 0 && dd == 0;
        snprintf(why, sizeof why, "holder %u edge %u: position %u -> %u, nothing else changed; %d/%d/%d/%d", V,
                 pgf_last.pos, pgf_last.old, cur, dn, dl, dh, dd);
        break;
    }
    case 3: case 4: {
        int k = 0, nd = 0;
        pg_value *h = HP(g, ch == NONE ? 0 : ch);
        const uint32_t *e = HE(g, h);
        for (k = 0; k < h->ne; k++)
            if (e[k] != H[ch].e[k]) {
                nd++;
                if (cls == 3) ok = EN(e[k]) == pgf_last.new_ && EN(H[ch].e[k]) == pgf_last.old && EW(e[k]) == EW(H[ch].e[k]);
                else ok = EN(e[k]) == EN(H[ch].e[k]) && EW(e[k]) == pgf_last.new_ && EW(H[ch].e[k]) == pgf_last.old;
            }
        ok = ok && nd == 1 && dh == 1 && ch == V && dn == 0 && dl == 0 && dd == 0;
        snprintf(why, sizeof why, "holder %u: %d edge changed (%u -> %u); %d/%d/%d/%d", ch, nd, pgf_last.old,
                 pgf_last.new_, dn, dl, dh, dd);
        break;
    }
    case 6:
        ok = dn == 1 && cn == L && g->nc[L].rc == pgf_last.new_ && S[L].rc == pgf_last.old && pgf_last.new_ != pgf_last.old &&
             g->nc[L].len == S[L].len && dl == 0 && dh == 0 && dd == 0;
        snprintf(why, sizeof why, "node %u rc %u->%u, len %u; %d/%d/%d/%d", L, S[L].rc, g->nc[L].rc, g->nc[L].len, dn,
                 dl, dh, dd);
        break;
    case 7:
        ok = dd == 1 && pgi_dict_get(g, pgf_last.atom) == pgf_last.node2 && pgf_last.node2 != pgf_last.node &&
             g->nh[pgf_last.node2].alive && dn == 0 && dl == 0 && dh == 0;
        snprintf(why, sizeof why, "atom %llu -> node %u (was %u); %d/%d/%d/%d", (unsigned long long)pgf_last.atom,
                 pgf_last.node2, pgf_last.node, dn, dl, dh, dd);
        break;
    }
f9:
    switch (cls) {
    case 9: {
        uint8_t x = pgf_last.ob ^ pgf_last.nb;
        ok = x && !(x & (x - 1)) && *(uint8_t *)pgf_last.addr == pgf_last.nb;
        snprintf(why, sizeof why, "one bit: 0x%02x -> 0x%02x (%s)", pgf_last.ob, pgf_last.nb,
                 pgf_rec ? pgf_rec->inj_region : "");
        break;
    }
    }
out:
    for (uint32_t id = 0; id < nn; id++) free(S[id].hl);
    free(S); free(H); free(at); free(dv);
    snprintf(msg, cap, "%s", why);
    return ok;
}
