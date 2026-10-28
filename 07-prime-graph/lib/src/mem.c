/* mem.c: allocation with per-graph accounting (pg_stats mem_bytes / mem_peak).  Each block carries its size in a
 * 16-byte header.  Allocation failure is fatal (pgi_die): an M1 choice, see the README. */
#include "pg_internal.h"

void pgi_die(const char *msg) {
    fprintf(stderr, "primegraph: %s\n", msg);
    fflush(stderr);
    abort();
}

void *pgi_alloc(pg_graph *g, size_t n) {
    size_t *p = malloc(n + 16);
    if (!p) pgi_die("out of memory");
    p[0] = n;
    g->mem_cur += n;
    if (g->mem_cur > g->mem_peak) g->mem_peak = g->mem_cur;
    return p + 2;
}

void *pgi_calloc(pg_graph *g, size_t n) {
    void *p = pgi_alloc(g, n);
    memset(p, 0, n);
    return p;
}

void *pgi_realloc(pg_graph *g, void *q, size_t n) {
    if (!q) return pgi_alloc(g, n);
    size_t *p = (size_t *)q - 2, old = p[0];
    p = realloc(p, n + 16);
    if (!p) pgi_die("out of memory");
    p[0] = n;
    g->mem_cur = g->mem_cur - old + n;
    if (g->mem_cur > g->mem_peak) g->mem_peak = g->mem_cur;
    return p + 2;
}

void pgi_free(pg_graph *g, void *q) {
    if (!q) return;
    size_t *p = (size_t *)q - 2;
    g->mem_cur -= p[0];
    free(p);
}
