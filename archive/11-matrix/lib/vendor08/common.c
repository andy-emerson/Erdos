/* common.c: failure and allocation, the counting cell, names, the denominator lookup (spec §5), the control's divide
 * shared with the ceiling (spec §6, §9 item 4), and memory-report helpers. */
#include "sk_internal.h"

#ifdef SK_COUNT
sk_count sk_sink;
sk_count *sk_cur = &sk_sink;
#endif

_Noreturn void sk_die(const char *msg) {
    fprintf(stderr, "skeletons: %s\n", msg);
    fflush(stderr);
    abort();
}

void *sk_xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) sk_die("out of memory");
    return p;
}

void *sk_xcalloc(size_t n, size_t size) {
    void *p = calloc(n ? n : 1, size ? size : 1);
    if (!p) sk_die("out of memory");
    return p;
}

void *sk_xrealloc(void *p, size_t n) {
    void *q = realloc(p, n ? n : 1);
    if (!q) sk_die("out of memory");
    return q;
}

int sk_build_width(void) { return SK_WIDTH; }
int sk_build_counting(void) {
#ifdef SK_COUNT
    return 1;
#else
    return 0;
#endif
}
int sk_build_audit(void) {
#ifdef SK_AUDIT
    return 1;
#else
    return 0;
#endif
}

const char *sk_status_name(sk_status s) {
    switch (s) {
    case SK_OK: return "ok";
    case SK_EOVERFLOW: return "overflow";
    case SK_EDIVZERO: return "division by zero";
    case SK_EINVAL: return "invalid argument";
    case SK_EAUDIT: return "audit failure";
    case SK_ENOTBUILT: return "not built";
    }
    return "?";
}

const char *sk_phase_name(sk_phase p) {
    switch (p) {
    case SK_PH_ENTRY: return "entry";
    case SK_PH_ADD: return "add";
    case SK_PH_SUB: return "sub";
    case SK_PH_MUL: return "mul";
    case SK_PH_DIV: return "div";
    case SK_PH_READ: return "read";
    case SK_NPHASE: break;
    }
    return "?";
}

/* ----------------------------------------------------------------------------------------- the lookup */

#define SK_PHI UINT64_C(0x9E3779B97F4A7C15)

/* The starting slot (spec §5 item 2, G-5): unsigned 64-bit wrapping arithmetic at every width; index arithmetic,
 * counted as one lookup search (counting rule 4). */
static inline uint32_t start_slot(const sk_lookup *t, sk_int v) {
    CNT(lookup_search, 1);
    return (uint32_t)(((uint64_t)v * SK_PHI) >> (64 - t->log2S));
}

void sk_lookup_init(sk_lookup *t) {
    t->S = 16;
    t->log2S = 4;
    t->slot = sk_xmalloc(sizeof(uint32_t) * t->S);
    for (uint32_t s = 0; s < t->S; s++) t->slot[s] = SK_NONE;
#ifdef SK_AUDIT
    t->filled = 0;
#endif
}

void sk_lookup_free(sk_lookup *t) {
    free(t->slot);
    t->slot = NULL;
}

uint32_t sk_lookup_find(const sk_lookup *t, const sk_int *dens, sk_int v, uint32_t *empty) {
    uint32_t mask = t->S - 1, s = start_slot(t, v);
    for (;;) {
        CNT(lookup_probe, 1);
        uint32_t k = t->slot[s];
        if (k == SK_NONE) {
            if (empty) *empty = s;
            return SK_NONE;
        }
        if (dens[k] == v) return k;
        s = (s + 1) & mask;
    }
}

static void place(sk_lookup *t, const sk_int *dens, uint32_t k) {
    uint32_t mask = t->S - 1, s = start_slot(t, dens[k]);
    for (;;) {
        CNT(lookup_probe, 1);
        if (t->slot[s] == SK_NONE) break;
        s = (s + 1) & mask;
    }
    t->slot[s] = k;
}

void sk_lookup_insert(sk_lookup *t, const sk_int *dens, uint32_t k, uint32_t empty) {
    uint64_t count = (uint64_t)k + 1;
    if (2 * count > t->S) {
        /* the insertion would make the table more than half full: double S and place every index again, in index
         * order (spec §5 item 2) */
        if (t->log2S >= 32) sk_die("lookup too large");
        t->S *= 2;
        t->log2S++;
        t->slot = sk_xrealloc(t->slot, sizeof(uint32_t) * t->S);
        for (uint32_t s = 0; s < t->S; s++) t->slot[s] = SK_NONE;
        for (uint32_t i = 0; i <= k; i++) place(t, dens, i);
#ifdef SK_AUDIT
        t->filled = k + 1;
#endif
        return;
    }
    t->slot[empty] = k;
#ifdef SK_AUDIT
    t->filled++;
#endif
}

const char *sk_lookup_audit_full(const sk_lookup *t, const sk_int *dens, uint32_t D) {
    uint64_t held = 0;
    for (uint32_t s = 0; s < t->S; s++) {
        if (t->slot[s] == SK_NONE) continue;
        if (t->slot[s] >= D) return "lookup: a slot holds an index not below D";
        held++;
    }
    if (held != D) return "lookup: does not hold exactly D indices";
    if (2 * (uint64_t)D > t->S) return "lookup: more than half full";
    for (uint32_t k = 0; k < D; k++)
        if (sk_lookup_find(t, dens, dens[k], NULL) != k)
            return "lookup: a denominator index is not found at its value (or the list has a duplicate)";
    return NULL;
}

const char *sk_lookup_audit_count(const sk_lookup *t, uint32_t D) {
#ifdef SK_AUDIT
    if (t->filled != D) return "lookup: does not hold exactly D indices";
#endif
    if (2 * (uint64_t)D > t->S) return "lookup: more than half full";
    return NULL;
}

/* ----------------------------------------------------------------------------------------- the control's divide */

const char *sk_ctl_divide_pair(sk_int mx, sk_int dx, sk_int my, sk_int dy, sk_int *n, sk_int *e, bool *fast) {
    /* set s to the sign of m_y and t to |m_y| (neither counted: counting rule 1, DECISIONS.md) */
    bool neg = my < 0;
    sk_int t = neg ? -my : my;
    sk_int g1 = sk_gcd(mx, t);
    sk_int a = sk_divexact(mx, g1);
    sk_int tg = sk_divexact(t, g1);
    if (neg) a = -a;
    if (dx == dy) {
        *fast = true;
        *n = a;
        *e = tg;
        return NULL;
    }
    *fast = false;
    sk_int g2 = sk_gcd(dx, dy);
    if (sk_mul(a, sk_divexact(dy, g2), n)) return "divide: s*(m_x/g_1)*(d_y/g_2)";
    if (sk_mul(sk_divexact(dx, g2), tg, e)) return "divide: (d_x/g_2)*(t/g_1)";
    return NULL;
}

/* ----------------------------------------------------------------------------------------- memory */

void sk_memory_add(sk_memory *m, const char *name, uint64_t used, uint64_t allocated) {
    if (m->n >= SK_MEM_MAX) sk_die("memory report: too many items");
    m->item[m->n].name = name;
    m->item[m->n].used = used;
    m->item[m->n].allocated = allocated;
    m->n++;
    m->used += used;
    m->allocated += allocated;
}

void sk_memory_finish(sk_memory *m, uint64_t values) {
    m->values = values;
    m->used_per_rational = values ? (double)m->used / (double)values : 0.0;
    m->allocated_per_rational = values ? (double)m->allocated / (double)values : 0.0;
}
