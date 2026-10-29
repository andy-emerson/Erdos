/* STUB -- NOT THE LIBRARY'S UNIT TESTS.  A few checks of the stub, so that the harness's unit-test step (rule,
 * Order of work, step 2) has something to run at widths 64 and 32 before lib/ exists. */
#include <stdio.h>

#include "sk.h"

#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); return 1; } } while (0)

int main(void) {
    ctl_class *c = ctl_create();
    uint32_t a, b, r;
    sk_int m, d;
    CHECK(ctl_enter(c, 2, -4, &a) == SK_OK);
    CHECK(ctl_enter(c, 1, 3, &b) == SK_OK);
    CHECK(ctl_add(c, a, b, &r) == SK_OK && ctl_read(c, r, &m, &d) == SK_OK && m == -1 && d == 6);
    CHECK(ctl_sub(c, a, a, &r) == SK_OK && ctl_read(c, r, &m, &d) == SK_OK && m == 0 && d == 1);
    CHECK(ctl_div(c, b, r, &r) == SK_EDIVZERO);
    CHECK(ctl_enter(c, 1, 0, &r) == SK_EDIVZERO);
    ctl_destroy(c);
    printf("stub unit tests: pass (width %d)\n", SK_WIDTH);
    return 0;
}
