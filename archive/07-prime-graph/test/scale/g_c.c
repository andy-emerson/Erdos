/* g_c.c: C, the control (a reference curve only).  #included by scale.c after s_core.c when VAR_C is defined.
 *
 * Round two's V0' entry, unchanged in substance (ab_core.c refine and ab_decompose): each new distinct denominator is
 * gcd-refined against EVERY live node (a scan), every live node that is not prime is then factored (a scan of the
 * node table), and each distinct denominator's edges are found by trial division by EVERY live node (a scan).
 * Edges are one-sided: holders keep their edges, nodes keep no lists and no reference counts, and nodes are never
 * freed.  A split kills the node and records its parts; after the entry, EVERY live holder is scanned and the edges
 * naming a dead node are rewritten (pg.c's pg_sync_pools does the same), then the record is dropped.
 */

static uint32_t *fwo;    /* per dead node: its parts (EP(part, exponent)) in fw, until the rewrite */
static uint8_t *fwc;
static uint32_t fwocap;
static uint32_t *fw;
static size_t nfw, fwcap;
static int c_killed;
static uint32_t corrupt_result;
static uint64_t st_c_splits, st_c_rewrite_scanned, st_c_rewrite_ns, st_c_rewrites, st_c_scan_dens;

static void g_init(void) { sieve(); }
static uint32_t c_node_new(uint64_t v, int prime) {
    uint32_t id = nd_slot();
    memset(&nd[id], 0, sizeof(node_t));
    nd_setv(id, v);
    nd[id].alive = 1;
    nd[id].prime = (uint8_t)prime;
    nlive++;
    return id;
}
static void c_kill(uint32_t id, const uint32_t *parts, int np) {
    if (ncap > fwocap) {
        fwo = xrealloc(fwo, (size_t)ncap * sizeof *fwo);
        fwc = xrealloc(fwc, ncap);
        fwocap = ncap;
    }
    nd[id].alive = 0;
    nlive--;
    GROW(fw, nfw, fwcap, (size_t)np);
    fwo[id] = (uint32_t)nfw; fwc[id] = (uint8_t)np;
    for (int i = 0; i < np; i++) fw[nfw++] = parts[i];
    c_killed++;
    st_c_splits++;
}

/* ab_core.c's refine: den (1 < den < 2^63) made a power product of live nodes by gcd against every live node */
static void g_enter_den(uint32_t k, uint64_t den) {
    (void)k;
    if (den <= 1) return;
    uint64_t left = den;
    uint32_t nn = nnodes;
    for (uint32_t id = 0; id < nn && left > 1; id++) {
        if (!nd[id].alive) continue;
        uint64_t v = nd[id].v;
        if (gcd64(left, v) == 1) continue;
        uint64_t base[64], pv[64];
        uint32_t pe[64];
        int nb = coprime_base2(v, left, base), np = 0;
        for (int b = 0; b < nb; b++) {
            uint32_t e = 0;
            for (uint64_t t = v; t % base[b] == 0; t /= base[b]) e++;
            if (e) { pv[np] = base[b]; pe[np++] = e; }
        }
        if (!(np == 1 && pe[0] == 1)) {
            uint32_t parts[64];
            for (int j = 0; j < np; j++) parts[j] = EP(c_node_new(pv[j], 0), pe[j]);
            c_kill(id, parts, np);
        }
        for (int j = 0; j < np; j++)
            while (left % pv[j] == 0) left /= pv[j];
    }
    if (left > 1) c_node_new(left, 0);
}

static int c_factor(uint64_t n, uint64_t *P, uint32_t *E) {  /* pg_factor */
    int k = 0, s = __builtin_ctzll(n), i;
    if (s) { P[k] = 2; E[k++] = (uint32_t)s; n >>= s; }
    for (i = 0; i < nsp; i++) {
        uint64_t p = sp[i];
        if (p * p > n) break;
        uint64_t q = n * spi[i];
        if (q <= spl[i]) {
            uint32_t e = 0;
            do { n = q; e++; q = n * spi[i]; } while (q <= spl[i]);
            P[k] = p; E[k++] = e;
        }
    }
    if (n == 1) return k;
    if (i < nsp || n < (1ull << 32) || is_prime(n)) { P[k] = n; E[k++] = 1; return k; }
    return k + factor_rest(n, P + k, E + k);
}

static int c_expand(uint32_t x, uint32_t w, uint32_t *out, int k) {
    if (nd[x].alive) {
        if (w > 255) die("c: weight above 255");
        if (k == AW) die("c: too many edges");
        out[k] = EP(x, w);
        return k + 1;
    }
    for (uint32_t i = 0; i < fwc[x]; i++) {
        uint32_t f = fw[fwo[x] + i];
        k = c_expand(EN(f), w * EW(f), out, k);
    }
    return k;
}
static int cmp_u32c(const void *a, const void *b) {
    uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    return (x > y) - (x < y);
}

static void g_resolve(void) {
    uint32_t nn = nnodes;  /* ab_decompose: every live node that is not prime, factored */
    for (uint32_t id = 0; id < nn; id++) {
        if (!nd[id].alive || nd[id].prime) continue;
        uint64_t p[64];
        uint32_t e[64];
        int k = c_factor(nd[id].v, p, e);
        if (k == 1 && e[0] == 1) { nd[id].prime = 1; continue; }
        uint32_t parts[64];
        for (int j = 0; j < k; j++) parts[j] = EP(c_node_new(p[j], 1), e[j]);
        c_kill(id, parts, k);
    }
    if (c_killed) {  /* the rewrite: every live holder scanned, edges naming a dead node re-expanded */
        uint64_t t0 = now_ns();
        for (uint32_t h = 0; h < nh; h++) {
            holder_t *X = &H[h];
            if (!X->alive || !X->ne) continue;
            st_c_rewrite_scanned++;
            const uint32_t *e = EPL + X->eo;
            int dead = 0;
            for (int k = 0; k < X->ne && !dead; k++) dead = !nd[EN(e[k])].alive;
            if (!dead) continue;
            uint32_t out[AW];
            int k2 = 0;
            for (int k = 0; k < X->ne; k++) k2 = c_expand(EN(e[k]), EW(e[k]), out, k2);
            qsort(out, (size_t)k2, 4, cmp_u32c);
            h_set_edges(h, out, k2);
            st_c_rewrites++;
        }
        st_c_rewrite_ns += now_ns() - t0;
        nfw = 0;
        c_killed = 0;
    }
    for (uint32_t k = 0; k < ndd; k++) {  /* each distinct denominator's edges: trial division by every live node */
        dden *D = &dd[k];
        D->hd = NONE;
        if (D->bad || D->den == 1) continue;
        uint64_t left = D->den, q;
        uint32_t e[MAXE];
        int ne = 0;
        st_c_scan_dens++;
        for (uint32_t id = 0; id < nnodes && left > 1; id++) {
            if (!nd[id].alive) continue;
            int c = 0;
            while (ndiv(left, &nd[id], &q)) { left = q; c++; }
            if (c) {
                if (ne == MAXE) die("c: too many edges");
                e[ne++] = EP(id, c);
            }
        }
        if (left != 1) die("c: unresolved denominator");
        D->hd = h_new(0, D->den, e, ne, 3);
    }
}
static void g_link(uint32_t h) { (void)h; }
static void g_unlink(uint32_t h) { (void)h; }
static void g_entry_done(void) {
    for (uint32_t k = 0; k < ndd; k++)
        if (dd[k].hd != NONE) { h_free(dd[k].hd); dd[k].hd = NONE; }
}
static uint64_t g_idle(void) { return 0; }

static const char *g_audit(const char *what) {
    int all = !what || !strcmp(what, "all");
    uint32_t live = 0;
    for (uint32_t id = 0; id < nnodes; id++) {
        if (!nd[id].alive) continue;
        live++;
        if ((all || strstr(what, "nodes")) && !nd[id].prime) return "node_not_prime_after_entry";
    }
    if (live != nlive) return "live_node_count";
    if (all || strstr(what, "den"))
        for (uint32_t h = 0; h < nh; h++) {
            const holder_t *X = &H[h];
            if (!X->alive) continue;
            if (X->den) { const char *m = audit_edges(X->den, X->ne, EPL + X->eo); if (m) return m; }
            else if (X->ne) return "bad_value_has_edges";
            if ((X->kind == 0 || X->kind == 2) && X->den && X->num && gcd64(mag(X->num), X->den) != 1) return "value_not_reduced";
        }
    if (all || strstr(what, "coprime")) {
        uint64_t *v = malloc(((size_t)nlive + 1) * 8);
        size_t n = 0;
        for (uint32_t id = 0; id < nnodes; id++) if (nd[id].alive) v[n++] = nd[id].v;
        const char *m = coprime_check(v, n, what && strstr(what, "pairwise"));
        free(v);
        if (m) return m;
    }
    return NULL;
}
static int g_corrupt(const char *what) {
    if (!strcmp(what, "result")) { corrupt_result = 1; return 1; }
    if (!strcmp(what, "leak")) { corrupt_leak = 1; return 1; }
    if (!strcmp(what, "den")) {
        for (uint32_t h = 0; h < nh; h++) if (H[h].alive && H[h].ne) { H[h].den += 2; return 1; }
        return 0;
    }
    if (!strcmp(what, "coprime")) {
        uint32_t a = NONE, b = NONE;
        for (uint32_t id = 0; id < nnodes; id++) if (nd[id].alive) { if (a == NONE) a = id; else if (b == NONE) b = id; }
        uint64_t x;
        if (b == NONE || __builtin_mul_overflow(nd[b].v, nd[a].v, &x)) return 0;
        nd_setv(b, x);
        return 1;
    }
    return 0;
}
static void g_stats(FILE *f) {
    fprintf(f, "\"nodes_live\": %u, \"node_slots\": %u, \"splits_entry\": %llu, \"rewrite_scanned\": %llu, "
               "\"rewrites\": %llu, \"rewrite_ns\": %llu, \"scan_dens\": %llu, \"rho_calls\": %llu",
            nlive, nnodes, (unsigned long long)st_c_splits, (unsigned long long)st_c_rewrite_scanned,
            (unsigned long long)st_c_rewrites, (unsigned long long)st_c_rewrite_ns, (unsigned long long)st_c_scan_dens,
            (unsigned long long)st_rho_calls);
}
static int cmp_u64c(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}
static void g_dump_nodes(FILE *f) {
    uint64_t *v = malloc(((size_t)nlive + 1) * 8);
    size_t n = 0;
    for (uint32_t id = 0; id < nnodes; id++) if (nd[id].alive) v[n++] = nd[id].v;
    qsort(v, n, 8, cmp_u64c);
    fprintf(f, "N");
    for (size_t i = 0; i < n; i++) fprintf(f, " %llu", (unsigned long long)v[i]);
    fprintf(f, "\n");
    free(v);
}
static uint32_t g_degree(uint64_t v) { (void)v; return 0; }
static uint32_t g_refcount(uint64_t v) { (void)v; return 0; }
