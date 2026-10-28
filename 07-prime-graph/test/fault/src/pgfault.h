/* pgfault.h: the fault-injection interface of the test/fault copy of primegraph (not part of the library's API).
 *
 * The driver (driver.c) shares one pgf_trial record with each forked trial (anonymous shared memory): the library
 * writes into it what it injected, what it verified and repaired, and any halt; the driver adds the comparison with
 * the oracle.  Fault classes are numbered as in the test plan: 1..9 = F1..F9. */
#ifndef PGFAULT_H
#define PGFAULT_H

#include <stdint.h>
#include "primegraph.h"

typedef struct {
    int injected;            /* 1: the fault was applied; -1: not applicable to this variant */
    int inj_cmd;             /* the command before which it was applied (F8: the command of the interrupted split) */
    char inj_desc[200];
    char inj_region[24];     /* F9: the region of the flipped bit */
    int halt_kind;           /* 0 none; 1 a production check halted (verify or audit); 2 an internal diagnostic */
    char halt_msg[200];
    uint64_t nverify, nrepair;
    char repair_msg[200];    /* the first mismatch that a rebuild repaired */
    int repair_cmd;          /* the command during which the first repair ran (-1 none) */
    int cur_cmd;             /* the command being executed */
    int mismatch_cmd;        /* first command whose output differs from the oracle (-1 none) */
    int mismatch_kind;       /* 1 a wrong result returned as success; 2 an error status where a result was expected */
    char got[240], want[240];
    int completed;           /* the workload ran to its end */
    int latent;              /* after completion, the full audit (production mode) fails */
    char latent_msg[200];
    uint64_t done_cmds;
} pgf_trial;

extern pgf_trial *pgf_rec;       /* NULL: no recording */
extern uint64_t pgf_K, pgf_ops;  /* production checks every K operations (0: never); the counter */
extern uint64_t pgf_nsplits;     /* refinements (split_out calls) so far */
extern uint64_t pgf_rng;         /* the injectors' random state */

#define PGF_EXIT_HALT 42         /* exit status of a halt by a production check */

/* Inject one fault of class cls (1..7, 9) now.  Returns 1 if applied, 0 if the graph has no target for it now,
 * -1 if the class does not apply to this variant (F5 without node-side exponents).  The description goes to
 * pgf_rec.  F8 is armed instead with pgf_arm_f8. */
int pgf_inject(pg_graph *g, int cls);
/* F8: the n-th refinement from now (n >= 1) stops halfway through its copy and loop; the program continues. */
void pgf_arm_f8(uint64_t n);
const char *pgi_audit_opt(pg_graph *g, int light, int midop);
const char *pgf_variant(void);

/* test hooks: the targets of the last injection (for test_fault.py's checks) */
typedef struct { int cls; uint32_t node, holder, pos, old, new_, node2; uint64_t atom; void *addr; uint8_t ob, nb; } pgf_last_t;
extern pgf_last_t pgf_last;
/* inspection for the tests: node id's list length and reference count, whether holder hid has an edge to it (and
 * its weight), whether hid appears in the node's list; 0 if the node is not live */
int pgf_probe(pg_graph *g, uint32_t node, uint32_t hid, uint32_t *len, uint32_t *rc, uint32_t *w, int *in_list);
uint32_t pgf_dict(pg_graph *g, uint64_t atom);
uint32_t pgf_value_id(const pg_value *v);
int pgf_selfcheck(pg_graph *g, int cls, char *msg, size_t cap);

#endif
