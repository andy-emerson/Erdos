/* STUB -- NOT THE LIBRARY.  test/stub/ is a tiny stand-in for the API of spec v0.1 (§4.1, §9, §12, §13), written
 * only so that the harness (test/) can be exercised end to end before lib/ exists.  "D1" and "D2" here are the
 * control under other names; nothing measured with this stub means anything.  Never use it for a measured run:
 * runner.py refuses a measured run with the stub.
 *
 * The C names below are the harness's assumption about lib/'s API; test/api.h is the one place that maps the
 * harness onto them, and is what to reconcile when lib/README.md appears.
 */
#ifndef SK_STUB_H
#define SK_STUB_H
#include <stddef.h>
#include <stdint.h>

#ifndef SK_WIDTH
#define SK_WIDTH 64
#endif
#if SK_WIDTH == 64
typedef int64_t sk_int;
#define SK_MAX INT64_MAX
#elif SK_WIDTH == 32
typedef int32_t sk_int;
#define SK_MAX INT32_MAX
#else
#error "SK_WIDTH must be 32 or 64"
#endif

enum { SK_OK = 0, SK_EOVERFLOW = 1, SK_EDIVZERO = 2, SK_EINVAL = 3 };
enum { SK_PH_ENTRY, SK_PH_ADD, SK_PH_SUB, SK_PH_MUL, SK_PH_DIV, SK_NPHASE };
enum { SK_CORE, SK_STORE, SK_NPART };
enum { SK_C_GCDS, SK_C_GCD_WORK, SK_C_DIVISIONS, SK_C_MULTIPLICATIONS, SK_C_ADDITIONS, SK_C_LOOKUP_SEARCHES,
       SK_C_LOOKUP_PROBES, SK_C_PAIRBIT_READS, SK_C_SCANS, SK_C_REFINEMENTS, SK_C_EVENTS, SK_NCTR };
extern const char *const sk_phase_names[SK_NPHASE];
extern const char *const sk_counter_names[SK_NCTR];

typedef struct {
    uint64_t c[SK_NPHASE][SK_NPART][SK_NCTR];
    uint64_t gcd_hist[SK_NPHASE][SK_NPART][65];
} sk_counters;

#define SK_MAXSTRUCT 16
typedef struct {
    int nstruct;
    struct { const char *name; uint64_t used, alloc; } s[SK_MAXSTRUCT];
    uint64_t nvalues;           /* N: stored values (ceiling: elements of stored vectors) */
} sk_memory;

#define SK_SCALAR_API(P)                                                                     \
    typedef struct P##_class P##_class;                                                      \
    P##_class *P##_create(void);                                                             \
    void P##_destroy(P##_class *c);                                                          \
    int P##_enter(P##_class *c, sk_int m, sk_int d, uint32_t *h);                            \
    int P##_add(P##_class *c, uint32_t x, uint32_t y, uint32_t *h);                          \
    int P##_sub(P##_class *c, uint32_t x, uint32_t y, uint32_t *h);                          \
    int P##_mul(P##_class *c, uint32_t x, uint32_t y, uint32_t *h);                          \
    int P##_div(P##_class *c, uint32_t x, uint32_t y, uint32_t *h);                          \
    int P##_read(const P##_class *c, uint32_t x, sk_int *m, sk_int *d);                      \
    uint32_t P##_size(const P##_class *c);                                                   \
    void P##_memory(const P##_class *c, sk_memory *r);                                       \
    void P##_counters(const P##_class *c, sk_counters *r);                                   \
    const char *P##_last_error(const P##_class *c);                                          \
    /* test build: 0 pass, else nonzero with a message in *msg.  last != 0: the full audit after the last call,    \
       complete (Design 2's pairwise check included at any base size, §13.3). */                                  \
    int P##_audit(P##_class *c, int last, const char **msg);                                 \
    int P##_audit_changed(P##_class *c, const char **msg);

SK_SCALAR_API(ctl)
SK_SCALAR_API(d1)
SK_SCALAR_API(d2)

/* The ceiling (§9).  Vectors and working copies are named by 32-bit ids.  gather's neg[] (NULL: none) negates
 * chosen elements of the working copy: the rule's CH-all steps gather "the 1,000 y_k, negated where the step
 * subtracts" (see DECISIONS.md). */
typedef struct ceil_class ceil_class;
ceil_class *ceil_create(void);
void ceil_destroy(ceil_class *c);
int ceil_enter_vector(ceil_class *c, const sk_int *m, uint32_t len, sk_int d, uint32_t *v);
int ceil_gather(ceil_class *c, uint32_t v, const uint32_t *pos, const unsigned char *neg, uint32_t len, uint32_t *w);
int ceil_add(ceil_class *c, uint32_t x, uint32_t y, uint32_t *v);
int ceil_sub(ceil_class *c, uint32_t x, uint32_t y, uint32_t *v);
int ceil_mul(ceil_class *c, uint32_t x, uint32_t y, uint32_t *v);
int ceil_div(ceil_class *c, uint32_t x, uint32_t y, uint32_t *v);
int ceil_read(const ceil_class *c, uint32_t v, uint32_t k, sk_int *m, sk_int *d);
void ceil_memory(const ceil_class *c, sk_memory *r);
void ceil_counters(const ceil_class *c, sk_counters *r);
const char *ceil_last_error(const ceil_class *c);
int ceil_audit(ceil_class *c, int last, const char **msg);
int ceil_audit_changed(ceil_class *c, const char **msg);

#endif
