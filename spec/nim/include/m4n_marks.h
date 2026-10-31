/* m4n_marks.h: one operation's overflow record (spec v0.4 §17.10), counting builds, held by its result (m4n.h).  Its
 * own header, so that the Nim models can take its layout without the models' own declarations. */
#ifndef M4N_MARKS_H
#define M4N_MARKS_H

#include <stdint.h>

typedef struct m4n_marks {
    uint64_t made;   /* "new": the cell's own arithmetic overflowed */
    uint64_t passed; /* "passed on": an operand was marked */
    uint64_t tests;  /* "mark tests": operand cells tested for d = 0 */
} m4n_marks;

#endif
