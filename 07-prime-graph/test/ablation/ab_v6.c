/* ab_v6.c: V6 = V0' + the seen-prime dictionary as the entry procedure (round 2).  #included after ab_l1.c when
 * VARIANT == VAR_V6; the layout and every op are V0' (ab_l1.c, ab_core.c).
 *
 * Round 3 also #includes this file after ab_v3.c for D0..D3 (V3's layout with this entry).  The parts written for
 * ab_l1.c's layout (the fill hooks, the prefix rewrite, the edge dump) are compiled for V6 only; ab_d.c supplies the
 * V3-layout versions.  D0 compiles v6_enter_den and v6_den_edges below unchanged; D1-D3 take their own
 * v6_enter_den (and D2 its own v6_den_edges) from ab_d.c.  V6 compiles to the same code as in round 2.
 *
 * Entry, pass 1 (each distinct input denominator once, as in V0'), in place of V0' gcd scan of every live node:
 *   1. trial division: 2, then the odd primes below 2^16 with the adaptive stop (pg_factor's stage 2, on a copy of
 *      pg.c's table: pg.c keeps it static and is left unchanged); a leftover below 2^32, or one left when the stop
 *      fired, is prime;
 *   1b. a larger leftover is divided by the primes above 2^16 already in the dictionary (exact-division checks);
 *   4. only what remains goes to Miller-Rabin (pg_is_prime) and, if composite, pg_factor (rho);
 *   2. for each prime p found, look p up in the dictionary (a direct-mapped array for p < 2^16, a hash above):
 *        first sighting: don't split; p joins this denominator's new node, its "remaining part" (the product of its
 *                        first-sighted primes' powers), which becomes p's holder;
 *        already seen:   if p's holder holds other primes too, split p out of it: the holder dies and is forwarded
 *                        to p and the rest; the new denominator's p-part goes on node p.
 *   (Step 3, gcd refinement of unfactored leftovers, never triggers: step 4 factors every leftover.)
 * A node that holds one prime p is stored as p, with the power on the edge weight; a node holding several primes is
 * stored as their product.  So 100 then 35 end as nodes 2, 5, 7 with 100 = 2^2 * 5^2 and 35 = 5 * 7.
 * Pass 2: each distinct denominator's edges by dictionary lookups, not V0' scan of every node (timed separately as
 * v6_t_edges).  Then the fill reduces each value as V0' does.
 *
 * A node holding several primes is cancelled against by gcd, in hooked copies of fin, core_add and core_mul below:
 * a proper common part splits it by coprime refinement.  Every split kills the node and records its parts (node,
 * exponent).  During the fill, a split re-expands every distinct denominator's edges at once and the elements
 * filled before it are rewritten at the end of the entry (timed); during the ops, a split sets v6_dirty and every
 * operand is re-expanded if it names a dead node.  The hooked copies run only while v6_slow (a live node is not
 * prime, or v6_dirty); otherwise V6 runs the control's fin and ops unchanged.
 */

#define V6_NONE UINT32_MAX

#define V6_NSMALL 6600  /* the odd primes below 2^16 with exact-division constants, built as pg.c's sieve does */
static uint64_t v6sp[V6_NSMALL], v6spi[V6_NSMALL], v6spl[V6_NSMALL];
static int v6ns;
static void v6_sieve(void) {
    static uint8_t comp[65536];
    v6ns = 0;
    for (uint32_t i = 3; i < 65536; i += 2) {
        if (comp[i]) continue;
        v6sp[v6ns] = i; v6spi[v6ns] = inv64(i); v6spl[v6ns] = UINT64_MAX / i; v6ns++;
        for (uint32_t j = i * i; j < 65536; j += 2 * i) comp[j] = 1;
    }
}
static uint64_t *bigp, *bigi, *bigl;  /* the primes above 2^16 in the dictionary, with exact-division constants */
static size_t nbig, bigcap;

typedef struct { uint32_t po, fo; uint8_t pc, fc; } v6meta;  /* its primes; its forwarding (dead) */
static v6meta *vm;
static uint32_t vmcap;
static uint64_t *v6pp;  /* prime lists of the nodes */
static size_t v6npp, v6ppcap;
static uint64_t *v6ppi, *v6ppl;  /* each listed prime's exact-division constants (odd primes; 2 by shifts) */
static size_t v6ppicap, v6pplcap;
static uint32_t *v6fw;  /* forwarding: the parts of each dead node, EP(part, exponent) */
static size_t v6nfw, v6fwcap;
static uint32_t *dsm;   /* primes below 2^16: holder node + 1 (0: not seen) */
static uint64_t *dbk;   /* primes above 2^16: open addressing, key 0 empty */
static uint32_t *dbv;
static size_t dbcap, dbcnt;
static int v6_handed, v6_phase;  /* edges handed out (pass 2 began); 0 entry, 1 ops */
static uint64_t st6_split[2], st6_recheck, st6_hooked;  /* hooked: calls of the hooked copies */
static uint32_t *ddpo;  /* per distinct denominator of this entry: its primes (pool) */
static uint8_t *ddpc;
static size_t dd6cap;
static uint64_t *ddpp;
static size_t nddpp, ddppcap;
static char v6dump[4096];
static size_t v6_stale[8];  /* per entered array: its elements below this index may name dead nodes */
static int v6_cur;          /* the entered array being filled */

#define GROW(p, n, cap, need) do { if ((n) + (need) > (cap)) { while ((n) + (need) > (cap)) (cap) = (cap) ? 2 * (cap) : 256; \
                                   (p) = xrealloc((p), (cap) * sizeof *(p)); } } while (0)

static inline void v6_upd(void) { v6_slow = v6_nonprime || v6_dirty; }

#if VARIANT == VAR_D3
static void d3_leaf(uint64_t p);  /* ab_d.c: a new large prime joins the product forest */
#endif
#if VARIANT >= VAR_D0
static size_t d_snap_edges(size_t o);  /* ab_d.c: the edge dump for V3's layout */
#endif

static void v6_reset(void) {  /* per rep, untimed (with ab_reset; the memsets are outside the timer) */
    if (!dsm) {
        v6_sieve();
        dsm = xcalloc(65536 * sizeof *dsm);
        dbcap = 1024;
        dbk = xcalloc(dbcap * sizeof *dbk);
        dbv = xmalloc(dbcap * sizeof *dbv);
    }
    memset(dsm, 0, 65536 * sizeof *dsm);
    memset(dbk, 0, dbcap * sizeof *dbk);
    dbcnt = 0;
    v6npp = 0; v6nfw = 0; nddpp = 0; nbig = 0;
    v6_epoch = 1; v6_handed = 0; v6_phase = 0; v6_dirty = 0; v6_nonprime = 0;
    v6_upd();
    st6_split[0] = st6_split[1] = 0; st6_recheck = 0; st6_hooked = 0;
    memset(v6_stale, 0, sizeof v6_stale);
    v6_cur = 0;
}

/* ---------------------------------------------------------------- the dictionary */

static inline uint32_t dict_get(uint64_t p) {
    if (p < 65536) return dsm[p] - 1;  /* 0 -> V6_NONE */
    size_t i = hsh(p) & (dbcap - 1);
    while (dbk[i]) {
        if (dbk[i] == p) return dbv[i];
        i = (i + 1) & (dbcap - 1);
    }
    return V6_NONE;
}
static void dict_set(uint64_t p, uint32_t id) {
    if (p < 65536) { dsm[p] = id + 1; return; }
    size_t i = hsh(p) & (dbcap - 1);
    while (dbk[i] && dbk[i] != p) i = (i + 1) & (dbcap - 1);
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
        i = hsh(p) & (dbcap - 1);
        while (dbk[i]) i = (i + 1) & (dbcap - 1);
    }
    dbk[i] = p; dbv[i] = id; dbcnt++;
    if (nbig == bigcap) {  /* a new prime above 2^16: kept for step 1b */
        bigcap = bigcap ? 2 * bigcap : 256;
        bigp = xrealloc(bigp, bigcap * 8); bigi = xrealloc(bigi, bigcap * 8); bigl = xrealloc(bigl, bigcap * 8);
    }
    bigp[nbig] = p; bigi[nbig] = inv64(p); bigl[nbig] = UINT64_MAX / p; nbig++;
#if VARIANT == VAR_D3
    d3_leaf(p);
#endif
}

/* ---------------------------------------------------------------- nodes, splits, forwarding */

/* A new node holding the primes pr (pr must not point into v6pp): prime iff it holds one. */
static uint32_t v6_node(uint64_t v, const uint64_t *pr, int npr) {
    uint32_t id = node_new(v, npr == 1);
    if (id >= vmcap) {
        while (id >= vmcap) vmcap = vmcap ? 2 * vmcap : 256;
        vm = xrealloc(vm, vmcap * sizeof *vm);
    }
    v6meta *M = &vm[id];
    M->fc = 0; M->fo = 0; M->pc = (uint8_t)npr;
    GROW(v6pp, v6npp, v6ppcap, (size_t)npr);
    GROW(v6ppi, v6npp, v6ppicap, (size_t)npr);
    GROW(v6ppl, v6npp, v6pplcap, (size_t)npr);
    M->po = (uint32_t)v6npp;
    for (int i = 0; i < npr; i++) {
        v6ppi[v6npp] = pr[i] & 1 ? inv64(pr[i]) : 0;
        v6ppl[v6npp] = UINT64_MAX / pr[i];
        v6pp[v6npp++] = pr[i];
        dict_set(pr[i], id);
    }
    if (npr > 1) { v6_nonprime++; v6_upd(); }
    return id;
}
static inline uint32_t vlog(uint64_t x, uint64_t p) { uint32_t e = 0; while (x % p == 0) { x /= p; e++; } return e; }
/* A part x holding the primes pr: one prime is stored as that prime (the power goes to *ex), several as x. */
static uint32_t v6_part_node(uint64_t x, const uint64_t *pr, int npr, uint32_t *ex) {
    if (npr == 1) { *ex = vlog(x, pr[0]); return v6_node(pr[0], pr, 1); }
    *ex = 1;
    return v6_node(x, pr, npr);
}
static int v6_resolve(const uint32_t *e, int ne, uint32_t *out);
static void v6_refresh_dd(void) {  /* a split during the fill: every distinct denominator's edges re-expanded now */
    uint32_t out[AB_W];
    for (uint32_t k = 0; k < ndd; k++) {
        dden *D = &dd[k];
        if (D->bad || D->den <= 1) continue;
        int dead = 0;
        for (int i = 0; i < D->ne && !dead; i++) dead = !nodes[EN(D->e[i])].alive;
        if (!dead) continue;
        int c = v6_resolve(D->e, D->ne, out);
        if (c < 0 || c > AB_MAXE) die("v6: entry edges");
        memcpy(D->e, out, sizeof(uint32_t) * c);
        D->ne = (uint8_t)c;
    }
}
static void v6_kill(uint32_t id, const uint32_t *parts, int np) {
    if (!nodes[id].prime) v6_nonprime--;
    nodes[id].alive = 0;
    GROW(v6fw, v6nfw, v6fwcap, (size_t)np);
    vm[id].fo = (uint32_t)v6nfw; vm[id].fc = (uint8_t)np;
    for (int i = 0; i < np; i++) v6fw[v6nfw++] = parts[i];
    v6_epoch++;
    st6_split[v6_phase]++;
    if (v6_phase) v6_dirty = 1;          /* ops: operands may name the dead node from now on */
    else if (v6_handed) v6_refresh_dd(); /* fill: the denominators' edges now; filled elements at fill_end */
    v6_upd();
}

/* Split the prime p out of its multi-prime holder h: h = p^e * R; h dies, forwarded to (p, e), (R's node, k). */
static void v6_split_prime(uint32_t h, uint64_t p) {
    uint64_t R = nodes[h].v, rp[AB_MAXE];
    uint32_t e = 0, nr = 0, ex;
    while (R % p == 0) { R /= p; e++; }
    for (uint32_t i = 0; i < vm[h].pc; i++)
        if (v6pp[vm[h].po + i] != p) rp[nr++] = v6pp[vm[h].po + i];
    uint32_t parts[2];
    parts[0] = EP(v6_node(p, &p, 1), e);
    parts[1] = EP(v6_part_node(R, rp, (int)nr, &ex), ex);  /* R > 1: h held several primes */
    v6_kill(h, parts, 2);
}

/* Split node id by g, a proper common part of id and a numerator: the coprime base of {v, g} (V0' coprime_base2);
 * each of v's parts becomes a node holding its primes. */
static void v6_split_by(uint32_t id, uint64_t g) {
    uint64_t v = nodes[id].v, base[64], pv[64];
    uint32_t pe[64];
    int n = coprime_base2(v, g, base), np = 0;
    for (int b = 0; b < n; b++) {
        uint32_t e = 0;
        for (uint64_t t = v; t % base[b] == 0; t /= base[b]) e++;
        if (e) { pv[np] = base[b]; pe[np++] = e; }
    }
    if (np == 1 && pe[0] == 1) die("v6: a proper part did not split its node");
    uint32_t parts[64];
    const v6meta M = vm[id];
    uint64_t pr[AB_MAXE];
    for (uint32_t i = 0; i < M.pc; i++) pr[i] = v6pp[M.po + i];  /* copy: v6_node may move the pool */
    for (int j = 0; j < np; j++) {
        uint64_t sub[AB_MAXE];
        uint32_t ns = 0, ex;
        for (uint32_t i = 0; i < M.pc; i++) if (pv[j] % pr[i] == 0) sub[ns++] = pr[i];
        uint32_t nid = v6_part_node(pv[j], sub, (int)ns, &ex);
        if (pe[j] * ex > 255) die("v6: weight above 255");
        parts[j] = EP(nid, pe[j] * ex);
    }
    v6_kill(id, parts, np);
}
/* A proper common part of m and a node holding several primes (0: none): the product of the node's primes that
 * divide m, each tested by exact division (the primes are known, so no gcd).  It is below the node's value, since
 * the caller found that the node itself does not divide m. */
static inline uint64_t v6_part(uint64_t m, const ab_node *N) {
    const v6meta *M = &vm[N - nodes];
    uint64_t g = 1;
    for (uint32_t j = 0; j < M->pc; j++) {
        uint64_t p = v6pp[M->po + j];
        if (p & 1 ? m * v6ppi[M->po + j] <= v6ppl[M->po + j] : !(m & 1)) g *= p;
    }
    return (g > 1 && g < N->v) ? g : 0;
}

/* ---------------------------------------------------------------- re-expanding edge lists after splits */

static int v6_expand(uint32_t x, uint32_t w, uint32_t *out, int k) {
    if (k < 0) return k;
    if (nodes[x].alive) {
        if (w > 255) return -1;
        if (k == AB_W) die("v6: too many edges");
        out[k] = EP(x, w);
        return k + 1;
    }
    for (uint32_t i = 0; i < vm[x].fc && k >= 0; i++) {
        uint32_t f = v6fw[vm[x].fo + i];
        k = v6_expand(EN(f), w * EW(f), out, k);
    }
    return k;
}
/* e (ne edges, possibly naming dead nodes) -> out, every node live, ascending.  -1 if a weight passes 255. */
static int v6_resolve(const uint32_t *e, int ne, uint32_t *out) {
    uint32_t tmp[AB_W];
    int k = 0;
    for (int i = 0; i < ne && k >= 0; i++) k = v6_expand(EN(e[i]), EW(e[i]), tmp, k);
    if (k < 0) return -1;
    for (int a = 1; a < k; a++)
        for (int b = a; b > 0 && EN(tmp[b - 1]) > EN(tmp[b]); b--) { uint32_t t = tmp[b]; tmp[b] = tmp[b - 1]; tmp[b - 1] = t; }
    memcpy(out, tmp, sizeof(uint32_t) * k);
    return k;
}
static int v6_fresh(view *v, uint32_t *buf) {  /* 0: a weight passed 255 */
    int dead = 0;
    for (int i = 0; i < v->ne && !dead; i++) dead = !nodes[EN(v->e[i])].alive;
    if (!dead) return 1;
    int k = v6_resolve(v->e, v->ne, buf);
    if (k < 0) return 0;
    v->e = buf; v->ne = k;
    return 1;
}

/* ---------------------------------------------------------------- the hooked copies (only while v6_slow) */

static int v6_fin_restart(auwide m, int neg, aden den, const uint32_t *done, int nd, const uint32_t *rest, int nr,
                          raw *r) {
    uint32_t buf[2 * AB_W], out[AB_W];
    memcpy(buf, done, sizeof(uint32_t) * nd);
    memcpy(buf + nd, rest, sizeof(uint32_t) * nr);
    int k = v6_resolve(buf, nd + nr, out);
    if (k < 0) return 0;
    return fin_v6(neg ? -(awide)m : (awide)m, den, out, k, 1, r);
}
/* fin (ab_core.c) + a dead node restarts on the re-expanded edges; a node that is not prime and shares a proper part
 * with the numerator is split and the reduction restarts. */
static int fin_v6(awide t, aden den, const uint32_t *e, int ne, int red, raw *r) {
    st6_hooked++;
    if (t == 0) { r->num = 0; r->den = 1; r->ne = 0; return 1; }
    int neg = t < 0, k2 = 0;
    auwide m = neg ? (auwide)0 - (auwide)t : (auwide)t;
    if (red) {
        for (int k = 0; k < ne; k++) {
            uint32_t ek = e[k];
            const ab_node *N = &nodes[EN(ek)];
            int w = (int)EW(ek), c = 0;
            if (!N->alive) return v6_fin_restart(m, neg, den, r->e, k2, e + k, ne - k, r);
            while (c < w) {
                if ((m >> 64) == 0) {
                    uint64_t q;
                    if (!ndiv((uint64_t)m, N, &q)) break;
                    m = q;
                } else {
                    if (m % N->v) break;
                    m /= N->v;
                }
                c++;
            }
            for (int j = 0; j < c; j++) den = ediv(den, N);
            if (c < w && !N->prime) {
                uint64_t g = v6_part((m >> 64) ? (uint64_t)(m % N->v) : (uint64_t)m, N);
                if (g) {
                    uint32_t rest[AB_W];
                    rest[0] = EP(EN(ek), w - c);
                    memcpy(rest + 1, e + k + 1, sizeof(uint32_t) * (ne - k - 1));
                    v6_split_by(EN(ek), g);
                    return v6_fin_restart(m, neg, den, r->e, k2, rest, ne - k, r);
                }
            }
            if (c < w) r->e[k2++] = EP(EN(ek), w - c);
        }
    } else {
        if (e != r->e) memmove(r->e, e, sizeof(uint32_t) * ne);
        k2 = ne;
    }
    if (m > (((auwide)AB_NUM_LIM) - (neg ? 0 : 1))) return 0;
    r->num = neg ? (anum)(0 - (uint64_t)m) : (anum)(uint64_t)m;
    r->den = den;
    r->ne = k2;
    return 1;
}
/* core_add (V0' path) + operands re-expanded if they name a dead node */
static int core_add_v6(view a, view b, int neg, int red, raw *r) {
    st6_hooked++;
    if (!a.den || !b.den) { setbad(r); return 1; }
    uint32_t ba[AB_W], bb[AB_W];
    if (v6_dirty && (!v6_fresh(&a, ba) || !v6_fresh(&b, bb))) { setbad(r); return 0; }
    uint64_t ca, cb;
    if (!lcm_plan(&a, &b, &ca, &cb, r)) { setbad(r); return 0; }
    awide x = (awide)a.num * (awide)ca, y = (awide)b.num * (awide)cb;
    if (!fin_v6(neg ? x - y : x + y, r->den, r->e, r->ne, red, r)) { setbad(r); return 0; }
    return 1;
}
/* core_mul + re-expanded operands; a numerator sharing a proper part of a node that is not prime splits it and the
 * product starts again */
static int core_mul_v6(view a, view b, int cancel, raw *r) {
    st6_hooked++;
    if (!a.den || !b.den) { setbad(r); return 1; }
    if (a.num == 0 || b.num == 0) { r->num = 0; r->den = 1; r->ne = 0; return 1; }
    uint32_t ba[AB_W], bb[AB_W];
    if (v6_dirty && (!v6_fresh(&a, ba) || !v6_fresh(&b, bb))) { setbad(r); return 0; }
    int neg = (a.num < 0) != (b.num < 0);
    uint64_t ua = mag(a.num), ub = mag(b.num), da = a.den, db = b.den, q;
    uint32_t n[AB_W];
    uint8_t wa[AB_W], wb[AB_W];
    int k = merge(&a, &b, n, wa, wb), ne = 0;
    for (int i = 0; i < k; i++) {
        int w = wa[i] + wb[i];
        if (cancel) {
            const ab_node *N = &nodes[n[i]];
            if (!N->alive) { v6_dirty = 1; v6_upd(); return core_mul_v6(a, b, cancel, r); }  /* split during this op */
            int c = 0, d = 0;
            while (c < wb[i] && ndiv(ua, N, &q)) { ua = q; c++; db = ediv(db, N); }
            while (d < wa[i] && ndiv(ub, N, &q)) { ub = q; d++; da = ediv(da, N); }
            if (!N->prime) {
                uint64_t g = 0;
                if (c < wb[i]) g = v6_part(ua, N);
                if (!g && d < wa[i]) g = v6_part(ub, N);
                if (g) {
                    v6_split_by(n[i], g);
                    v6_dirty = 1;  /* a and b now name a dead node: re-expand them */
                    v6_upd();
                    return core_mul_v6(a, b, cancel, r);
                }
            }
            w -= c + d;
        }
        if (w > 255) { setbad(r); return 0; }
        if (w) r->e[ne++] = EP(n[i], w);
    }
    uint64_t p, den;
    if (__builtin_mul_overflow(ua, ub, &p) || p > AB_NUM_LIM - (neg ? 0 : 1)) { setbad(r); return 0; }
    if (__builtin_mul_overflow(da, db, &den) || (den >> AB_DEN_BITS)) { setbad(r); return 0; }
    r->num = neg ? (anum)(0 - p) : (anum)p;
    r->den = den;
    r->ne = ne;
    return 1;
}

/* ---------------------------------------------------------------- entry */

#if VARIANT == VAR_V6 || VARIANT == VAR_D0  /* D1-D3: ab_d.c */
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
    int np = 0, s = __builtin_ctzll(n), i;
    if (s) { pr[np] = 2; ex[np++] = (uint32_t)s; n >>= s; }
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
    if (n > 1 && i == v6ns && n >= (1ull << 32)) {  /* step 1b: the large primes already seen (odd: p is odd) */
        for (size_t j = 0; j < nbig && n >= (1ull << 32); j++) {
            uint64_t q = n * bigi[j];
            if (q > bigl[j]) continue;
            uint32_t e = 0;
            do { n = q; e++; q = n * bigi[j]; } while (q <= bigl[j]);
            pr[np] = bigp[j]; ex[np++] = e;
        }
    }
    if (n > 1) {  /* no factor below 2^16 is left, so below 2^32 n is prime */
        if (i < v6ns || n < (1ull << 32) || pg_is_prime(n)) { pr[np] = n; ex[np++] = 1; }
        else {  /* step 4: rho, only on a composite no known prime divides */
            uint64_t fp[64];
            uint32_t fe[64];
            int nf = pg_factor(n, fp, fe);
            for (int j = 0; j < nf; j++) { pr[np] = fp[j]; ex[np++] = fe[j]; }
        }
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
}
#endif

static int cmp_u32(const void *a, const void *b) { uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b; return (x > y) - (x < y); }

#if VARIANT != VAR_D2  /* D2: ab_d.c */
static void v6_den_edges(uint32_t k) {  /* pass 2: a distinct denominator's edges, by dictionary lookups */
    v6_handed = 1;
    dden *D = &dd[k];
    uint32_t cand[AB_W];
    int nc = 0;
    for (uint32_t j = 0; j < ddpc[k]; j++) {
        uint32_t id = dict_get(ddpp[ddpo[k] + j]);
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
#endif

#if VARIANT == VAR_V6  /* ab_l1.c's layout: D0-D3 use ab_d.c's d_fill_end and d_fresh instead */
/* The fill: the array being filled, and a split while filling its element i (written after the split): only the
 * elements filled before a split can name a dead node, so only those are rewritten at the end of the entry. */
static void v6_fill_arr(const ab_arr *a) {
    for (int p = 0; p < npend; p++) if (pending[p].a == a) v6_cur = p;
}
static void v6_fill_split(size_t i) {
    if (i > v6_stale[v6_cur]) v6_stale[v6_cur] = i;
    for (int q = 0; q < v6_cur; q++) v6_stale[q] = pending[q].a->n;
}
static void v6_rewrite_prefix(ab_arr *a, size_t upto) {  /* in place, or appended to the pool if it grew */
    uint32_t out[AB_W];
    for (size_t i = 0; i < upto; i++) {
        if (!a->den[i]) continue;
        uint32_t *e = a->e + a->eo[i];
        int dead = 0;
        for (int k = 0; k < a->ne[i] && !dead; k++) dead = !nodes[EN(e[k])].alive;
        if (!dead) continue;
        int k = v6_resolve(e, a->ne[i], out);
        if (k < 0) { ovf(); a->num[i] = 0; a->den[i] = 0; a->ne[i] = 0; continue; }
        if (k > a->ne[i]) {
            if (a->elen + (size_t)k > a->ecap) { a->ecap = 2 * a->ecap + (size_t)k; a->e = xrealloc(a->e, a->ecap * 4); }
            a->eo[i] = (uint32_t)a->elen;
            a->elen += (size_t)k;
        }
        memcpy(a->e + a->eo[i], out, sizeof(uint32_t) * k);
        a->ne[i] = (uint8_t)k;
    }
}

static void v6_snapshot(void);
static void v6_fill_end(void) {
    for (int p = 0; p < npend; p++) {
        st6_recheck += v6_stale[p];
        v6_rewrite_prefix(pending[p].a, v6_stale[p]);
    }
    v6_phase = 1;
    if (getenv("AB_V6_DUMP")) v6_snapshot();
}
#endif

/* ---------------------------------------------------------------- reporting and audit */

static uint32_t v6_count(int multi) {  /* live nodes (multi: only those holding several primes) */
    uint32_t c = 0;
    for (uint32_t i = 0; i < nnodes; i++) if (nodes[i].alive) c += !multi || vm[i].pc > 1;
    return c;
}
static void v6_snapshot(void) {  /* AB_V6_DUMP: the live nodes after entry, and the first array's first edges */
    uint64_t v[512];
    uint32_t n = 0;
    for (uint32_t i = 0; i < nnodes && n < 512; i++) if (nodes[i].alive) v[n++] = nodes[i].v;
    for (uint32_t a = 1; a < n; a++)
        for (uint32_t b = a; b > 0 && v[b - 1] > v[b]; b--) { uint64_t t = v[b]; v[b] = v[b - 1]; v[b - 1] = t; }
    size_t o = 0;
    o += snprintf(v6dump + o, sizeof v6dump - o, " v6nodes=");
    for (uint32_t i = 0; i < n && o < sizeof v6dump - 64; i++) o += snprintf(v6dump + o, sizeof v6dump - o, "%s%llu", i ? "," : "", (unsigned long long)v[i]);
#if VARIANT >= VAR_D0
    d_snap_edges(o);
#else
    if (!npend) return;
    const ab_arr *A = pending[0].a;
    o += snprintf(v6dump + o, sizeof v6dump - o, " v6edges=");
    for (size_t i = 0; i < A->n && i < 16 && o < sizeof v6dump - 256; i++) {
        o += snprintf(v6dump + o, sizeof v6dump - o, "%s%llu:", i ? ";" : "", (unsigned long long)A->den[i]);
        for (int k = 0; k < A->ne[i]; k++) {
            uint32_t e = A->e[A->eo[i] + k];
            o += snprintf(v6dump + o, sizeof v6dump - o, "%s%llu^%u", k ? "*" : "", (unsigned long long)nodes[EN(e)].v, EW(e));
        }
    }
#endif
}
static void v6_stats(FILE *f) {
    fprintf(f, " v6_entry_splits=%llu v6_op_splits=%llu v6_nodes=%u v6_multi=%u v6_rechecked=%llu v6_hooked=%llu%s",
            (unsigned long long)st6_split[0], (unsigned long long)st6_split[1], v6_count(0), v6_count(1),
            (unsigned long long)st6_recheck, (unsigned long long)st6_hooked, getenv("AB_V6_DUMP") ? v6dump : "");
}

/* The audit (AB_AUDIT, untimed): re-expand the arrays' edges, then check the graph: live nodes pairwise coprime;
 * each node equals the product of its primes' powers and each of them points back to it in the dictionary; a
 * single-prime node is that prime, flagged prime; every dictionary entry names a live node its prime divides; the
 * count of non-prime nodes is right.  AB_CORRUPT=coprime / dict breaks the invariant / the dictionary first. */
#if VARIANT == VAR_V6
static void v6_rewrite_all(ab_arr *a) { v6_rewrite_prefix(a, a->n); }
#else
#define v6_rewrite_all(a) d_fresh(a)  /* D0-D3: V3's layout */
#endif
static const char *v6_audit(ab_arr **L, int nl, const char *corrupt) {
    for (int k = 0; k < nl; k++) v6_rewrite_all(L[k]);
    uint32_t *live = xmalloc((nnodes + 1) * sizeof *live), nl2 = 0, np = 0;
    for (uint32_t i = 0; i < nnodes; i++) if (nodes[i].alive) live[nl2++] = i;
    if (corrupt && !strcmp(corrupt, "coprime") && nl2 >= 2) {
        uint64_t x;
        for (uint32_t i = 1; i < nl2; i++)
            if (!__builtin_mul_overflow(nodes[live[0]].v, nodes[live[i]].v, &x)) { nodes[live[0]].v = x; break; }
    }
    if (corrupt && !strcmp(corrupt, "dict") && nl2 >= 2)  /* point a node's prime at another live node */
        dict_set(v6pp[vm[live[0]].po], live[1]);
    const char *m = NULL;
    for (uint32_t a = 0; a < nl2 && !m; a++)
        for (uint32_t b = a + 1; b < nl2 && !m; b++)
            if (gcd64(nodes[live[a]].v, nodes[live[b]].v) != 1) m = "v6_nodes_not_coprime";
    for (uint32_t a = 0; a < nl2 && !m; a++) {
        uint32_t id = live[a];
        const v6meta *M = &vm[id];
        uint64_t v = nodes[id].v;
        np += !nodes[id].prime;
        for (uint32_t j = 0; j < M->pc && !m; j++) {
            uint64_t p = v6pp[M->po + j];
            if (v % p) m = "v6_prime_not_in_node";
            while (!m && v % p == 0) v /= p;
            if (!m && dict_get(p) != id) m = "v6_dict_mismatch";
        }
        if (!m && (v != 1 || !M->pc)) m = "v6_node_not_its_primes";
        if (!m && M->pc == 1 && (nodes[id].v != v6pp[M->po] || !nodes[id].prime)) m = "v6_single_prime_not_prime_node";
    }
    if (!m && np != v6_nonprime) m = "v6_nonprime_count";
    for (uint64_t p = 2; p < 65536 && !m; p++)
        if (dsm[p] && (!nodes[dsm[p] - 1].alive || nodes[dsm[p] - 1].v % p)) m = "v6_dict_stale";
    for (size_t i = 0; i < dbcap && !m; i++)
        if (dbk[i] && (!nodes[dbv[i]].alive || nodes[dbv[i]].v % dbk[i])) m = "v6_dict_stale";
    xfree(live);
    return m;
}
