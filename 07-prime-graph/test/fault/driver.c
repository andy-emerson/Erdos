/* driver.c: the fault-injection trial driver (one binary per variant: build/drv_N, drv_O1, drv_O2, drv_O3).
 *
 *   drv_X < job.txt        runs a job: one workload, its oracle, and a list of trials; one result line per trial
 *   drv_X --script [--K k] < script.txt
 *                          runs a script with the test commands below, printing one line per command (test_fault.py)
 *
 * Job format (written by workload.py):
 *   K <k>                  production checks every k operations (0: never)
 *   WORKEND <w>            commands [0, w) are the workload; the rest read every live result (the readout)
 *   CMDS <n>  then n command lines
 *   EXP <n>   then n expectations: "I" (ignore), "X <line>" (exact), "P <prefix>", "A <line>|<line>" (either)
 *   TRIALS <m> then m lines: id class moment seed phase
 *              moment: the command before which the fault is injected (F1-F7, F9); F8: a fraction (ppb) of the
 *              workload's refinements, the one to interrupt; phase: the trial's offset of the K counter
 *
 * The driver first runs a control (no fault) in a child: it must match the oracle on every line with no repair and
 * no halt, else the job is rejected.  Then it runs the workload once in the parent and, before each trial's moment,
 * forks: the child injects, runs the rest of the workload and the readout, and compares each output line with the
 * oracle as it goes (stopping at the first difference).  The parent waits (with a timeout) and prints the record.
 *
 * Commands (values in slots v0..v4095, arrays in a0..a1023), as lib/tests/diff.c:
 *   V dst num den | I dst n | B op dst a b | N dst a | C dst a | R a | Q a b | G a | P a (print a value)
 *   AF dst n num den ... | AV dst n v ... | AO op dst a b [c] | AS dst a | AD dst a b | AG a i | AGV dst a i
 *   AX a | AR a | EB | EE
 * Test commands (--script only): Z cls seed (inject now), ZC cls seed (inject and self-check the effect), Z8 n (arm F8
 *   for the n-th refinement from now), ZL (describe the last injection), AU (light audit), VER (a production check
 *   now), SEGV (raise SIGSEGV), REC (print the trial record).
 */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

#include "primegraph.h"
#include "src/pgfault.h"

#define NV 4096
#define NA 1024
static pg_value *VS[NV];
static pg_array *AS[NA];
static pg_graph *g;
static int64_t *nb, *db;
static pg_value **vb;
static char *outbuf;
#define OUTCAP (1 << 22)

/* ------------------------------------------------------------------------------------------------- commands */

static int64_t rd(char **p) {
    char *t = strtok_r(NULL, " \n", p);
    return t ? strtoll(t, NULL, 10) : 0;
}
static int put_value(int dst, pg_status s, pg_value *v) {
    if (s != PG_OK) return sprintf(outbuf, "E %d", (int)s);
    if (VS[dst]) pg_value_release(g, VS[dst]);
    VS[dst] = v;
    int64_t n, d;
    pg_value_get(g, v, &n, &d);
    return sprintf(outbuf, "= %" PRId64 " %" PRId64, n, d);
}
static int put_array(int dst, pg_status s, pg_array *a) {
    if (s != PG_OK) return sprintf(outbuf, "E %d", (int)s);
    if (AS[dst]) pg_array_release(g, AS[dst]);
    AS[dst] = a;
    pg_array_info inf;
    pg_array_info_get(a, &inf);
    return sprintf(outbuf, "a %" PRId64 " %zu %d", inf.D, inf.exceptions, inf.deferred);
}

extern int pgf_selfcheck(pg_graph *g, int cls, char *msg, size_t cap);
extern void pgf_periodic(pg_graph *g);

/* Execute one command line (a copy: strtok writes into it); the output line goes to outbuf. */
static void exec_cmd(char *line) {
    char *p, *cmd = strtok_r(line, " \n", &p);
    pg_value *v = NULL;
    pg_array *a = NULL;
    pg_status s;
    outbuf[0] = 0;
    if (!cmd) return;
    if (!strcmp(cmd, "V")) {
        int dst = (int)rd(&p);
        int64_t n = rd(&p), d = rd(&p);
        s = pg_value_new(g, n, d, &v);
        put_value(dst, s, v);
    } else if (!strcmp(cmd, "I")) {
        int dst = (int)rd(&p);
        s = pg_value_from_int(g, rd(&p), &v);
        put_value(dst, s, v);
    } else if (!strcmp(cmd, "B")) {
        char op = *strtok_r(NULL, " \n", &p);
        int dst = (int)rd(&p), x = (int)rd(&p), y = (int)rd(&p);
        s = op == '+' ? pg_add(g, VS[x], VS[y], &v) : op == '-' ? pg_sub(g, VS[x], VS[y], &v)
          : op == '*' ? pg_mul(g, VS[x], VS[y], &v) : pg_div(g, VS[x], VS[y], &v);
        put_value(dst, s, v);
    } else if (!strcmp(cmd, "N")) {
        int dst = (int)rd(&p), x = (int)rd(&p);
        put_value(dst, pg_neg(g, VS[x], &v), v);
    } else if (!strcmp(cmd, "C")) {
        int dst = (int)rd(&p), x = (int)rd(&p);
        put_value(dst, pg_value_clone(g, VS[x], &v), v);
    } else if (!strcmp(cmd, "R")) {
        int x = (int)rd(&p);
        pg_value_release(g, VS[x]);
        VS[x] = NULL;
        strcpy(outbuf, "ok");
    } else if (!strcmp(cmd, "P")) {
        int x = (int)rd(&p);
        int64_t n, d;
        s = pg_value_get(g, VS[x], &n, &d);
        if (s) sprintf(outbuf, "E %d", (int)s);
        else sprintf(outbuf, "= %" PRId64 " %" PRId64, n, d);
    } else if (!strcmp(cmd, "Q")) {
        int x = (int)rd(&p), y = (int)rd(&p);
        sprintf(outbuf, "c %d", pg_cmp(g, VS[x], VS[y]));
    } else if (!strcmp(cmd, "G")) {
        int x = (int)rd(&p);
        sprintf(outbuf, "s %d %d", pg_sign(VS[x]), pg_is_zero(VS[x]));
    } else if (!strcmp(cmd, "AF")) {
        int dst = (int)rd(&p);
        size_t n = (size_t)rd(&p);
        for (size_t i = 0; i < n; i++) { nb[i] = rd(&p); db[i] = rd(&p); }
        put_array(dst, pg_array_from_fracs(g, nb, db, n, &a), a);
    } else if (!strcmp(cmd, "AV")) {
        int dst = (int)rd(&p);
        size_t n = (size_t)rd(&p);
        for (size_t i = 0; i < n; i++) vb[i] = VS[rd(&p)];
        put_array(dst, pg_array_from_values(g, vb, n, &a), a);
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
        put_value(dst, pg_array_sum(g, AS[x], &v), v);
    } else if (!strcmp(cmd, "AD")) {
        int dst = (int)rd(&p), x = (int)rd(&p), y = (int)rd(&p);
        put_value(dst, pg_array_dot(g, AS[x], AS[y], &v), v);
    } else if (!strcmp(cmd, "AG")) {
        int x = (int)rd(&p);
        size_t i = (size_t)rd(&p);
        int64_t n, d;
        s = pg_array_get_frac(g, AS[x], i, &n, &d);
        if (s) sprintf(outbuf, "E %d", (int)s);
        else sprintf(outbuf, "= %" PRId64 " %" PRId64, n, d);
    } else if (!strcmp(cmd, "AGV")) {
        int dst = (int)rd(&p), x = (int)rd(&p);
        size_t i = (size_t)rd(&p);
        put_value(dst, pg_array_get(g, AS[x], i, &v), v);
    } else if (!strcmp(cmd, "AX")) {
        int x = (int)rd(&p);
        size_t n = pg_array_len(AS[x]);
        int64_t *on = malloc(8 * n + 8), *od = malloc(8 * n + 8);
        s = pg_array_to_fracs(g, AS[x], on, od);
        if (s) sprintf(outbuf, "E %d", (int)s);
        else {
            char *o = outbuf;
            *o++ = 'x';
            for (size_t i = 0; i < n && o - outbuf < OUTCAP - 64; i++)
                o += sprintf(o, " %" PRId64 " %" PRId64, on[i], od[i]);
            *o = 0;
        }
        free(on);
        free(od);
    } else if (!strcmp(cmd, "AR")) {
        int x = (int)rd(&p);
        pg_array_release(g, AS[x]);
        AS[x] = NULL;
        strcpy(outbuf, "ok");
    } else if (!strcmp(cmd, "EB")) {
        strcpy(outbuf, pg_expr_begin(g) == PG_OK ? "ok" : "E 3");
    } else if (!strcmp(cmd, "EE")) {
        s = pg_expr_end(g);
        if (s) sprintf(outbuf, "E %d", (int)s);
        else strcpy(outbuf, "ok");
    /* ---- test commands */
    } else if (!strcmp(cmd, "Z")) {
        int cls = (int)rd(&p);
        pgf_rng = (uint64_t)rd(&p);
        if (cls == 8) { pgf_arm_f8(1); strcpy(outbuf, "z 1"); }
        else sprintf(outbuf, "z %d", pgf_inject(g, cls));
    } else if (!strcmp(cmd, "Z8")) {
        pgf_arm_f8((uint64_t)rd(&p));
        strcpy(outbuf, "ok");
    } else if (!strcmp(cmd, "ZC")) {
        int cls = (int)rd(&p);
        pgf_rng = (uint64_t)rd(&p);
        char m[200];
        int r = pgf_selfcheck(g, cls, m, sizeof m);
        const char *au = pgi_audit_opt(g, 1, 0);
        sprintf(outbuf, "zc %d audit=%s | %s", r, au ? "fail" : "pass", m);
    } else if (!strcmp(cmd, "ZL")) {
        sprintf(outbuf, "zl cls=%d pos=%u old=%u", pgf_last.cls, pgf_last.pos, pgf_last.old);
    } else if (!strcmp(cmd, "AU")) {
        const char *m = pgi_audit_opt(g, 1, 0);
        sprintf(outbuf, "au %s", m ? m : "ok");
    } else if (!strcmp(cmd, "VER")) {
        pgf_periodic(g);
        strcpy(outbuf, "ok");
    } else if (!strcmp(cmd, "SEGV")) {
        raise(SIGSEGV);
    } else if (!strcmp(cmd, "REC")) {
        sprintf(outbuf, "rec nverify=%" PRIu64 " nrepair=%" PRIu64 " halt=%d", pgf_rec ? pgf_rec->nverify : 0,
                pgf_rec ? pgf_rec->nrepair : 0, pgf_rec ? pgf_rec->halt_kind : 0);
    } else {
        fprintf(stderr, "driver: unknown command %s\n", cmd);
        exit(3);
    }
}

/* ------------------------------------------------------------------------------------------------- the oracle */

static int is_error(const char *s) { return (s[0] == 'E' && s[1] == ' ') || !strcmp(s, "c 2"); }

/* 1 if the output matches the expectation */
static int expect_ok(const char *exp, const char *got) {
    switch (exp[0]) {
    case 'I': return 1;
    case 'X': return !strcmp(exp + 2, got);
    case 'P': return !strncmp(exp + 2, got, strlen(exp + 2));
    case 'A': {
        const char *a = exp + 2, *bar = strchr(a, '|');
        if (!bar) return !strcmp(a, got);
        size_t n = (size_t)(bar - a);
        return (strlen(got) == n && !strncmp(a, got, n)) || !strcmp(bar + 1, got);
    }
    }
    return 0;
}
static int expects_error(const char *exp) { return (exp[0] == 'X' || exp[0] == 'P') && is_error(exp + 2); }

/* ------------------------------------------------------------------------------------------------- jobs */

static char **cmds, **exps;
static int ncmd, workend;
static uint64_t K;
static uint64_t *cumsplit;  /* shared: refinements after each command, from the control */

static void reset_state(void) {
    memset(VS, 0, sizeof VS);
    memset(AS, 0, sizeof AS);
}

static void trunc_copy(char *dst, size_t cap, const char *s) {
    size_t n = strlen(s);
    if (n >= cap) n = cap - 1;
    memcpy(dst, s, n);
    dst[n] = 0;
}

/* run commands [from, ncmd) comparing with the oracle; the record says how it ended (the child exits after) */
static void run_compare(int from, int inject_cls, int pending) {
    static char line[1 << 22];
    for (int c = from; c < ncmd; c++) {
        pgf_rec->cur_cmd = c;
        if (pending && c < workend) {  /* a fault still to inject (no target until now): try before each command */
            int r = pgf_inject(g, inject_cls);
            if (r == 1) pending = 0;
            else if (r < 0) { pgf_rec->injected = -1; return; }
        }
        strcpy(line, cmds[c]);
        exec_cmd(line);
        pgf_rec->done_cmds++;
        if (!expect_ok(exps[c], outbuf)) {
            pgf_rec->mismatch_cmd = c;
            pgf_rec->mismatch_kind = is_error(outbuf) && !expects_error(exps[c]) ? 2 : 1;
            trunc_copy(pgf_rec->got, sizeof pgf_rec->got, outbuf);
            trunc_copy(pgf_rec->want, sizeof pgf_rec->want, exps[c]);
            return;
        }
        if (cumsplit && c < ncmd) cumsplit[c] = pgf_nsplits;
    }
    pgf_rec->completed = 1;
    const char *m = pgi_audit_opt(g, 1, 0);  /* not a production check: is the graph left inconsistent? */
    if (m) {
        pgf_rec->latent = 1;
        trunc_copy(pgf_rec->latent_msg, sizeof pgf_rec->latent_msg, m);
    }
}

static void child_limits(void) {
    struct rlimit r = {0, 0};
    setrlimit(RLIMIT_CORE, &r);
    r.rlim_cur = r.rlim_max = (rlim_t)1536 << 20;
    setrlimit(RLIMIT_AS, &r);
    alarm(30);
    int fd = open("/dev/null", O_WRONLY);
    if (fd >= 0) { dup2(fd, 2); close(fd); }  /* diagnostics are recorded; keep stderr quiet */
}

static void sanitize(char *s) {
    for (; *s; s++)
        if (*s == '\t' || *s == '\n' || *s == '\r') *s = ' ';
}

static void print_record(long id, int cls, int status) {
    pgf_trial *r = pgf_rec;
    char sig[32] = "";
    int exitc = -1;
    if (WIFSIGNALED(status)) snprintf(sig, sizeof sig, "%d", WTERMSIG(status));
    else if (WIFEXITED(status)) exitc = WEXITSTATUS(status);
    sanitize(r->inj_desc); sanitize(r->halt_msg); sanitize(r->got); sanitize(r->want); sanitize(r->repair_msg);
    sanitize(r->latent_msg);
    printf("T\tid=%ld\tcls=%d\texit=%d\tsig=%s\tinjected=%d\tinj_cmd=%d\tinj_region=%s\tinj_desc=%s\thalt=%d\t"
           "halt_msg=%s\tnverify=%" PRIu64 "\tnrepair=%" PRIu64 "\trepair_cmd=%d\trepair_msg=%s\tmismatch_cmd=%d\t"
           "mismatch_kind=%d\tgot=%.120s\twant=%.120s\tcompleted=%d\tlatent=%d\tlatent_msg=%s\tdone=%" PRIu64 "\n",
           id, cls, exitc, sig, r->injected, r->inj_cmd, r->inj_region, r->inj_desc, r->halt_kind, r->halt_msg,
           r->nverify, r->nrepair, r->repair_cmd, r->repair_msg, r->mismatch_cmd, r->mismatch_kind, r->got, r->want,
           r->completed, r->latent, r->latent_msg, r->done_cmds);
    fflush(stdout);
}

static void rec_reset(void) {
    memset(pgf_rec, 0, sizeof *pgf_rec);
    pgf_rec->inj_cmd = -1;
    pgf_rec->mismatch_cmd = -1;
    pgf_rec->repair_cmd = -1;
}

typedef struct { long id; int cls, cmd; uint64_t moment, seed, phase, arm; } trial;
static int tcmp(const void *a, const void *b) {
    const trial *x = a, *y = b;
    return x->cmd != y->cmd ? (x->cmd > y->cmd) - (x->cmd < y->cmd) : (x->id > y->id) - (x->id < y->id);
}

static char *readline_dup(FILE *f) {
    static char buf[1 << 22];
    if (!fgets(buf, sizeof buf, f)) return NULL;
    size_t n = strlen(buf);
    while (n && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[--n] = 0;
    return strdup(buf);
}

static int run_job(FILE *in) {
    char *l;
    int ntr = 0;
    trial *T = NULL;
    while ((l = readline_dup(in))) {
        if (!strncmp(l, "K ", 2)) K = strtoull(l + 2, NULL, 10);
        else if (!strncmp(l, "WORKEND ", 8)) workend = atoi(l + 8);
        else if (!strncmp(l, "CMDS ", 5)) {
            ncmd = atoi(l + 5);
            cmds = calloc((size_t)ncmd, sizeof *cmds);
            for (int i = 0; i < ncmd; i++) cmds[i] = readline_dup(in);
        } else if (!strncmp(l, "EXP ", 4)) {
            exps = calloc((size_t)ncmd, sizeof *exps);
            for (int i = 0; i < ncmd; i++) exps[i] = readline_dup(in);
        } else if (!strncmp(l, "TRIALS ", 7)) {
            ntr = atoi(l + 7);
            T = calloc((size_t)ntr + 1, sizeof *T);
            for (int i = 0; i < ntr; i++) {
                char *t = readline_dup(in);
                sscanf(t, "%ld %d %" SCNu64 " %" SCNu64 " %" SCNu64, &T[i].id, &T[i].cls, &T[i].moment, &T[i].seed,
                       &T[i].phase);
                free(t);
            }
        }
        free(l);
    }
    pgf_rec = mmap(NULL, sizeof *pgf_rec, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    cumsplit = mmap(NULL, 8 * (size_t)(ncmd + 1), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    /* the control: no fault, every line must match, no repair, no halt */
    rec_reset();
    pid_t pid = fork();
    if (!pid) {
        child_limits();
        pgf_K = K;
        pgf_ops = 0;
        g = pg_graph_create();
        run_compare(0, 0, 0);
        _exit(0);
    }
    int st;
    waitpid(pid, &st, 0);
    if (!WIFEXITED(st) || WEXITSTATUS(st) || !pgf_rec->completed || pgf_rec->mismatch_cmd >= 0 || pgf_rec->nrepair ||
        pgf_rec->halt_kind) {
        printf("CONTROL\tfail\t");
        print_record(-1, 0, st);
        return 1;
    }
    uint64_t S = workend > 0 ? cumsplit[workend - 1] : 0;
    printf("CONTROL\tok\tsplits=%" PRIu64 "\tnverify=%" PRIu64 "\tlatent=%d\n", S, pgf_rec->nverify, pgf_rec->latent);
    fflush(stdout);
    /* trial moments */
    for (int i = 0; i < ntr; i++) {
        if (T[i].cls == 8) {
            if (!S) { T[i].cmd = -1; continue; }
            uint64_t s = 1 + (uint64_t)((double)T[i].moment / 1e9 * (double)S);
            if (s > S) s = S;
            int c = 0;
            while (cumsplit[c] < s) c++;
            T[i].cmd = c;
            T[i].arm = s - (c ? cumsplit[c - 1] : 0);
        } else
            T[i].cmd = (int)T[i].moment;
    }
    qsort(T, (size_t)ntr, sizeof *T, tcmp);
    int ti = 0;
    while (ti < ntr && T[ti].cmd < 0) {  /* F8 without any refinement in the workload */
        rec_reset();
        printf("T\tid=%ld\tcls=8\tnosplit=1\n", T[ti].id);
        ti++;
    }
    pgf_rec->cur_cmd = 0;
    pgf_trial *shared = pgf_rec;
    static pgf_trial dummy;
    pgf_rec = &dummy;  /* the parent's own run records nothing */
    pgf_K = K;
    pgf_ops = 0;
    uint64_t *cs = cumsplit;
    cumsplit = NULL;
    g = pg_graph_create();
    reset_state();
    static char line[1 << 22];
    for (int c = 0; c < ncmd && ti < ntr; c++) {
        while (ti < ntr && T[ti].cmd == c) {
            trial *t = &T[ti++];
            pgf_rec = shared;
            rec_reset();
            fflush(stdout);
            pid = fork();
            if (!pid) {
                child_limits();
                pgf_rng = t->seed * 0x9E3779B97F4A7C15ull + 12345;
                if (K) pgf_ops = K - 1 - (t->phase % K);  /* the next check after (phase % K) + 1 operations */
                pgf_rec->cur_cmd = c;
                int pending = 0;
                if (t->cls == 8) pgf_arm_f8(t->arm);
                else {
                    int r = pgf_inject(g, t->cls);
                    if (r < 0) { pgf_rec->injected = -1; _exit(0); }
                    pending = r == 0;
                }
                run_compare(c, t->cls, pending);
                _exit(0);
            }
            waitpid(pid, &st, 0);
            print_record(t->id, t->cls, st);
            pgf_rec = &dummy;
        }
        strcpy(line, cmds[c]);
        exec_cmd(line);
    }
    (void)cs;
    return 0;
}

static int run_script(FILE *in) {
    static pgf_trial rec;
    pgf_rec = &rec;
    memset(&rec, 0, sizeof rec);
    g = pg_graph_create();
    char *l;
    int c = 0;
    while ((l = readline_dup(in))) {
        if (!strcmp(l, "END")) break;
        rec.cur_cmd = c++;
        exec_cmd(l);
        printf("%s\n", outbuf);
        fflush(stdout);
        free(l);
    }
    return 0;
}

int main(int argc, char **argv) {
    nb = malloc(8 * 200000);
    db = malloc(8 * 200000);
    vb = malloc(sizeof *vb * 200000);
    outbuf = malloc(OUTCAP);
    int script = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--script")) script = 1;
        else if (!strcmp(argv[i], "--K") && i + 1 < argc) pgf_K = K = strtoull(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--variant")) { printf("%s\n", pgf_variant()); return 0; }
    }
    return script ? run_script(stdin) : run_job(stdin);
}
