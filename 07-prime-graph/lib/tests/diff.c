/* diff.c: the C side of the differential test (tests/diff.py writes a script, this runs it, diff.py checks every
 * output line against Python's fractions).
 *
 *   diff [--audit]        reads commands from stdin, one per line, and prints one line per command
 *
 * Values live in slots v0..v4095, arrays in slots a0..a1023 (the script names slots by number).  Output lines:
 *   "= num den"   a value                    "E code"      an error status (1 overflow, 2 divzero, 3 invalid)
 *   "a D nexc deferred"  an array's layout   "x n d n d ..."  an array's elements
 *   "c k" / "s sign zero" / "t text" / "ok" / "st ..." as below
 * With --audit, pg_audit runs after every command and a failure stops the run ("AUDIT <reason>", exit 2).
 *
 * Commands (slots are integers):
 *   V dst num den | I dst n | S dst text | B op dst a b (op + - * /) | N dst a | C dst a | R a
 *   Q a b (cmp) | G a (sign, is_zero) | T a (to_string)
 *   AF dst n num den ... | AV dst n v ... | AO op dst a b [c] (op + - * / f) | AS dst a | AD dst a b
 *   AG a i | AGV dst a i | AX a | AR a | EB | EE | AU | ST | END
 */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "primegraph.h"

#define NV 4096
#define NA 1024
static pg_value *VS[NV];
static pg_array *AS[NA];
static pg_graph *g;

static void put_value(int dst, pg_status s, pg_value *v) {
    if (s != PG_OK) { printf("E %d\n", (int)s); return; }
    if (VS[dst]) pg_value_release(g, VS[dst]);
    VS[dst] = v;
    int64_t n, d;
    pg_value_get(g, v, &n, &d);
    printf("= %" PRId64 " %" PRId64 "\n", n, d);
}
static void put_array(int dst, pg_status s, pg_array *a) {
    if (s != PG_OK) { printf("E %d\n", (int)s); return; }
    if (AS[dst]) pg_array_release(g, AS[dst]);
    AS[dst] = a;
    pg_array_info inf;
    pg_array_info_get(a, &inf);
    printf("a %" PRId64 " %zu %d\n", inf.D, inf.exceptions, inf.deferred);
}
static int64_t rd(char **p) { return strtoll(strtok_r(NULL, " \n", p), NULL, 10); }

int main(int argc, char **argv) {
    int audit = argc > 1 && !strcmp(argv[1], "--audit");
    g = pg_graph_create();
    static char line[1 << 22];
    int64_t *nb = malloc(8 * 200000), *db = malloc(8 * 200000);
    pg_value **vb = malloc(sizeof *vb * 200000);
    while (fgets(line, sizeof line, stdin)) {
        char *p, *cmd = strtok_r(line, " \n", &p);
        if (!cmd) continue;
        pg_value *v = NULL;
        pg_array *a = NULL;
        pg_status s;
        if (!strcmp(cmd, "END")) break;
        if (!strcmp(cmd, "V")) {
            int dst = (int)rd(&p);
            int64_t n = rd(&p), d = rd(&p);
            s = pg_value_new(g, n, d, &v);
            put_value(dst, s, v);
        } else if (!strcmp(cmd, "I")) {
            int dst = (int)rd(&p);
            s = pg_value_from_int(g, rd(&p), &v);
            put_value(dst, s, v);
        } else if (!strcmp(cmd, "S")) {
            int dst = (int)rd(&p);
            char *t = strtok_r(NULL, " \n", &p);
            s = pg_value_from_string(g, t ? t : "", &v);
            put_value(dst, s, v);
        } else if (!strcmp(cmd, "B")) {
            char op = *strtok_r(NULL, " \n", &p);
            int dst = (int)rd(&p), x = (int)rd(&p), y = (int)rd(&p);
            s = op == '+' ? pg_add(g, VS[x], VS[y], &v) : op == '-' ? pg_sub(g, VS[x], VS[y], &v)
              : op == '*' ? pg_mul(g, VS[x], VS[y], &v) : pg_div(g, VS[x], VS[y], &v);
            put_value(dst, s, v);
        } else if (!strcmp(cmd, "N")) {
            int dst = (int)rd(&p), x = (int)rd(&p);
            s = pg_neg(g, VS[x], &v);
            put_value(dst, s, v);
        } else if (!strcmp(cmd, "C")) {
            int dst = (int)rd(&p), x = (int)rd(&p);
            s = pg_value_clone(g, VS[x], &v);
            put_value(dst, s, v);
        } else if (!strcmp(cmd, "R")) {
            int x = (int)rd(&p);
            pg_value_release(g, VS[x]);
            VS[x] = NULL;
            printf("ok\n");
        } else if (!strcmp(cmd, "Q")) {
            int x = (int)rd(&p), y = (int)rd(&p);
            printf("c %d\n", pg_cmp(g, VS[x], VS[y]));
        } else if (!strcmp(cmd, "G")) {
            int x = (int)rd(&p);
            printf("s %d %d\n", pg_sign(VS[x]), pg_is_zero(VS[x]));
        } else if (!strcmp(cmd, "T")) {
            int x = (int)rd(&p);
            char buf[PG_STRING_MAX];
            s = pg_value_to_string(g, VS[x], buf, sizeof buf);
            if (s) printf("E %d\n", (int)s);
            else printf("t %s\n", buf);
        } else if (!strcmp(cmd, "AF")) {
            int dst = (int)rd(&p);
            size_t n = (size_t)rd(&p);
            for (size_t i = 0; i < n; i++) { nb[i] = rd(&p); db[i] = rd(&p); }
            s = pg_array_from_fracs(g, nb, db, n, &a);
            put_array(dst, s, a);
        } else if (!strcmp(cmd, "AV")) {
            int dst = (int)rd(&p);
            size_t n = (size_t)rd(&p);
            for (size_t i = 0; i < n; i++) vb[i] = VS[rd(&p)];
            s = pg_array_from_values(g, vb, n, &a);
            put_array(dst, s, a);
        } else if (!strcmp(cmd, "AO")) {
            char op = *strtok_r(NULL, " \n", &p);
            int dst = (int)rd(&p), x = (int)rd(&p), y = (int)rd(&p);
            if (op == 'f') {
                int z = (int)rd(&p);
                s = pg_array_fma(g, AS[x], AS[y], AS[z], &a);
            } else
                s = op == '+' ? pg_array_add(g, AS[x], AS[y], &a) : op == '-' ? pg_array_sub(g, AS[x], AS[y], &a)
                  : op == '*' ? pg_array_mul(g, AS[x], AS[y], &a) : pg_array_div(g, AS[x], AS[y], &a);
            put_array(dst, s, a);
        } else if (!strcmp(cmd, "AS")) {
            int dst = (int)rd(&p), x = (int)rd(&p);
            s = pg_array_sum(g, AS[x], &v);
            put_value(dst, s, v);
        } else if (!strcmp(cmd, "AD")) {
            int dst = (int)rd(&p), x = (int)rd(&p), y = (int)rd(&p);
            s = pg_array_dot(g, AS[x], AS[y], &v);
            put_value(dst, s, v);
        } else if (!strcmp(cmd, "AG")) {
            int x = (int)rd(&p);
            size_t i = (size_t)rd(&p);
            int64_t n, d;
            s = pg_array_get_frac(g, AS[x], i, &n, &d);
            if (s) printf("E %d\n", (int)s);
            else printf("= %" PRId64 " %" PRId64 "\n", n, d);
        } else if (!strcmp(cmd, "AGV")) {
            int dst = (int)rd(&p), x = (int)rd(&p);
            size_t i = (size_t)rd(&p);
            s = pg_array_get(g, AS[x], i, &v);
            put_value(dst, s, v);
        } else if (!strcmp(cmd, "AX")) {
            int x = (int)rd(&p);
            size_t n = pg_array_len(AS[x]);
            int64_t *on = malloc(8 * n + 8), *od = malloc(8 * n + 8);
            s = pg_array_to_fracs(g, AS[x], on, od);
            if (s) printf("E %d\n", (int)s);
            else {
                printf("x");
                for (size_t i = 0; i < n; i++) printf(" %" PRId64 " %" PRId64, on[i], od[i]);
                printf("\n");
            }
            free(on);
            free(od);
        } else if (!strcmp(cmd, "AR")) {
            int x = (int)rd(&p);
            pg_array_release(g, AS[x]);
            AS[x] = NULL;
            printf("ok\n");
        } else if (!strcmp(cmd, "EB")) {
            printf(pg_expr_begin(g) == PG_OK ? "ok\n" : "E 3\n");
        } else if (!strcmp(cmd, "EE")) {
            s = pg_expr_end(g);
            if (s) printf("E %d\n", (int)s);
            else printf("ok\n");
        } else if (!strcmp(cmd, "AU")) {
            const char *m = pg_audit(g);
            printf(m ? "audit FAIL %s\n" : "ok\n", m);
        } else if (!strcmp(cmd, "ST")) {
            pg_stats st;
            pg_stats_get(g, &st);
            printf("st %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n", st.nodes_live, st.holders_live, st.values_live,
                   st.arrays_live, st.edges_live, st.splits_entry, st.splits_reduce, st.restarts, st.v4_retries,
                   st.rho_calls);
        } else {
            fprintf(stderr, "diff: unknown command %s\n", cmd);
            return 3;
        }
        if (audit) {
            const char *m = pg_audit(g);
            if (m) {
                printf("AUDIT %s\n", m);
                fflush(stdout);
                return 2;
            }
        }
    }
    fflush(stdout);
    pg_graph_destroy(g);
    free(nb);
    free(db);
    free(vb);
    return 0;
}
