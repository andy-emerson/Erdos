# Notes: two designs for signed and complex numbers

*Temporary notes (Nov 1, 2026). The mathematics of spec 13's two designs, with no code, for the author to compare. To be taken into the book or archived.*

Both designs are built from two kinds of number.

A **magnitude** is a whole number, $0, 1, 2, \ldots$. Magnitudes can be added and multiplied, and the result is again a magnitude. They cannot always be subtracted: $3 - 5$ is not a magnitude. What can always be taken is the **absolute difference** $|a - b|$, the larger minus the smaller, which is a magnitude.

A machine keeps a magnitude in a word, and a word's arithmetic is a clock: arithmetic mod $2^w$, whose values run from 0 to $2^w - 1$. Neither magnitudes nor the clock have negative values. On the clock, a subtraction that would go below zero wraps around to the top instead: on a 16-hour clock, $3 - 5$ lands on 14. So the absolute difference, which looks like a subtraction with an extra step, suits the machine: the clock never produces a negative, and a magnitude never needs one.

A **direction** is one of the four units $1, i, -1, -i$. Each is a power of $i$: $1 = i^0$, $i = i^1$, $-1 = i^2$ and $-i = i^3$, and $i^4 = 1$ again. So a direction is named by its exponent $d$, a number from 0 to 3. Multiplying two directions adds their exponents mod 4:

$$i^{d_1} \cdot i^{d_2} = i^{d_1 + d_2}, \quad \text{with } d_1 + d_2 \text{ taken mod } 4.$$

Negating is multiplying by $-1 = i^2$, so it adds 2 to the exponent. The directions 1 and $-1$ lie on one line, the real axis, and $i$ and $-i$ on another, the imaginary axis.

## Design A: one magnitude with one direction

A number is a pair $(m, d)$ of a magnitude $m$ and a direction $d$. Its value is

$$m \cdot i^d.$$

So $(5, 0)$ is 5, $(5, 2)$ is $-5$ and $(5, 1)$ is $5i$.

**Zero** is any pair with magnitude 0. Zero is both real and imaginary, since it lies on both axes, and it is neither positive nor negative, since it lies in no direction. So its direction is irrelevant: $(0, d)$ is zero for every $d$. A zero keeps whatever direction the operation that produced it left, and no step sets it. Every number other than zero has exactly one pair.

On each axis, 1 and $i$ are the **positive** directions, and $-1$ and $-i$ the **negative** ones. Two directions are **opposite** when they differ by 2, as 1 and $-1$ do, and lie on **different axes** when they differ by 1, as 1 and $i$ do. A difference of 1 can run either way: 1 and $-i$ differ by $-1$, which is 3 mod 4.

### Negation

Negation adds 2 to the direction:

$$-(m, d) = (m,\ d + 2),$$

since $-1 \cdot i^d = i^2 \cdot i^d = i^{d+2}$. The magnitude is untouched. Negating zero gives a zero.

### Comparison

Two numbers on the same axis can be compared, in both magnitude and direction. On the real axis, $-7 < -3 < 3 < 7$; on the imaginary axis, in the same way, $-7i < -3i < 3i < 7i$. Numbers on different axes cannot be ordered, but their magnitudes can: dropping the direction gives the **absolute value**, $|(m, d)| = m$, and absolute values compare as magnitudes.

A comparison makes two checks. The magnitude check comes first, since the zero cases are the ones that most need catching. It finds whether both magnitudes are 0, one is 0, or neither, and if neither, whether they are equal or which is larger. Zero is neither positive nor negative, since it lies in no direction, and it is both real and imaginary, since it lies on both axes. So when a magnitude is 0, its direction tells nothing. The direction check comes second, and it finds whether the directions are the same, opposite, or on different axes.

Together the two checks place a pair of numbers in one cell of a grid: the magnitude check picks the row, and the direction check the column.

| Magnitudes | Same direction | Opposite directions | Different axes |
| --- | --- | --- | --- |
| both 0 | equal | equal | equal |
| one is 0 | the other's direction decides | the other's direction decides | the other's direction decides |
| equal, not 0 | equal | the positive one is larger | not ordered; equal absolute values |
| $m_1 > m_2$ | the magnitudes decide | the positive one is larger | not ordered; the first has the larger absolute value |
| $m_1 < m_2$ | the magnitudes decide | the positive one is larger | not ordered; the second has the larger absolute value |

In the first two rows the answer is the same in every column, so no direction needs checking: it is either irrelevant or simply carried along. "The other's direction decides" means that a positive number is larger than zero and a negative one smaller. This holds even when zero's own direction lies on the other axis, as in $(0, 0)$ against $3i$: zero lies on both axes, so $0 < 3i$. "The magnitudes decide" means that in a positive direction the larger magnitude is the larger number ($7 > 3$), and in a negative direction the smaller magnitude is ($-3 > -7$).

### Addition

The first step of an addition is the comparison's two checks. They place the pair in the same grid, and each cell now gives the sum. The cells fall into six cases, A to F:

| Magnitudes | Same direction | Opposite directions | Different axes |
| --- | --- | --- | --- |
| both 0 | **A** zero | **A** zero | **A** zero |
| one is 0 | **B** the other number | **B** the other number | **B** the other number |
| equal, not 0 | **C** add the magnitudes | **A** zero | **F** an expression |
| $m_1 > m_2$ | **C** add the magnitudes | **D** subtract, first direction | **F** an expression |
| $m_1 < m_2$ | **C** add the magnitudes | **E** subtract and negate, second direction | **F** an expression |

**Case A, zero.** The sum is zero: either both numbers are zero, or two equal magnitudes point in opposite directions and cancel. The checks have already found this, so no arithmetic is done.

**Case B, one zero.** The sum is the other number, as it stands. For example, $5 + (0, 1)$ is 5, though zero's direction lies on the other axis.

**Case C, the same direction.** The magnitudes add, and the direction stays:

$$(m_1, d) + (m_2, d) = (m_1 + m_2,\ d),$$

since $m_1 i^d + m_2 i^d = (m_1 + m_2) i^d$.

**Case D, opposite directions, the first magnitude larger.** The numbers pull against each other along one axis, and the first wins. Its magnitude loses the second's, by the clock's subtraction, and the sum keeps the first direction:

$$(m_1, d_1) + (m_2, d_2) = (m_1 - m_2,\ d_1).$$

Since $m_1 > m_2$, the subtraction does not wrap, and the clock lands on the true difference. The rule holds because $m_1 i^{d_1} + m_2 i^{d_1 + 2} = m_1 i^{d_1} - m_2 i^{d_1} = (m_1 - m_2) \cdot i^{d_1}$.

**Case E, opposite directions, the first magnitude smaller.** The second number wins, so the sum takes the second direction. Its magnitude is $m_2 - m_1$, which is $-(m_1 - m_2)$: the clock's subtraction $m_1 - m_2$, followed by the clock's negation.

$$(m_1, d_1) + (m_2, d_2) = (-(m_1 - m_2),\ d_2), \quad \text{both steps on the clock}.$$

The subtraction wraps, and the negation undoes the wrap. For example, on a 16-hour clock, $3 + (-5)$ is $(3, 0) + (5, 2)$, case E. The clock's $3 - 5$ lands on 14, its negation of 14 lands on 2, and the direction is the second's, 2. The sum is $(2, 2)$, which is $-2$.

**Case F, different axes.** A sum such as $3 + 4i$ is not a magnitude times a single direction, so no pair holds it. Design A holds it as an expression, which is still to be worked out (see the end of these notes). Integers never reach this case, since their directions are always 1 and $-1$.

Cases D and E are the only places a magnitude is subtracted, and the only places where the magnitudes decide the direction.

### Subtraction

Subtraction is addition of the negation: $a - b = a + (-b)$. Negating $b$ turns its direction by 2 and leaves its magnitude alone, so subtraction runs addition with the second direction turned. Addition's two checks then find the case, once, as for any addition. For example, $3 - 5$ is $(3, 0) + (5, 2)$, case E, which gives $-2$ as above.

### Multiplication

Multiplication multiplies the magnitudes and adds the directions:

$$(m_1, d_1) \cdot (m_2, d_2) = (m_1 m_2,\ d_1 + d_2).$$

This holds because $m_1 i^{d_1} \cdot m_2 i^{d_2} = m_1 m_2 \cdot i^{d_1 + d_2}$. For example, $(-3) \cdot (-5)$ is $(3, 2) \cdot (5, 2) = (15, 0)$, which is 15, since $2 + 2 = 4 \equiv 0$. A product with a zero factor has magnitude 0, so it is zero, whatever its direction. The additions here are the machine's own: of magnitudes, and of directions mod 4. None of them is the addition of numbers above, so multiplication does not depend on it.

### Exact division

Exact division divides the magnitudes and subtracts the directions, when $m_2$ is not 0 and divides $m_1$:

$$(m_1, d_1) / (m_2, d_2) = (m_1 / m_2,\ d_1 - d_2).$$

## Design B: one magnitude per direction

### Integers

An integer is a pair $(P, N)$ of magnitudes. Its value is

$$P - N.$$

$P$ collects what lies along the direction 1, and $N$ what lies along $-1$. So $(5, 3)$ is 2 and $(3, 5)$ is $-2$.

The same integer has many pairs: $(2, 0)$, $(5, 3)$ and $(7, 5)$ are all 2. Two pairs have the same value exactly when their crossed sums agree:

$$P_1 - N_1 = P_2 - N_2 \quad \text{exactly when} \quad P_1 + N_2 = P_2 + N_1.$$

This follows by adding $N_1 + N_2$ to both sides of the first equation. The right-hand test uses only addition of magnitudes. This construction, integers as pairs of whole numbers, is how mathematicians build the integers from the whole numbers.

**Negation** swaps the two parts: $-(P, N) = (N, P)$, since $-(P - N) = N - P$.

**Addition** adds part by part:

$$(P_1, N_1) + (P_2, N_2) = (P_1 + P_2,\ N_1 + N_2),$$

since $(P_1 - N_1) + (P_2 - N_2) = (P_1 + P_2) - (N_1 + N_2)$. There are no cases: whatever the signs, two additions of magnitudes.

**Subtraction** adds the crossed parts:

$$(P_1, N_1) - (P_2, N_2) = (P_1 + N_2,\ N_1 + P_2),$$

since $(P_1 - N_1) - (P_2 - N_2) = (P_1 + N_2) - (N_1 + P_2)$. Again there are no cases and no subtraction of magnitudes.

**Multiplication** expands the product:

$$(P_1 - N_1)(P_2 - N_2) = (P_1 P_2 + N_1 N_2) - (P_1 N_2 + N_1 P_2),$$

so

$$(P_1, N_1) \cdot (P_2, N_2) = (P_1 P_2 + N_1 N_2,\ P_1 N_2 + N_1 P_2).$$

Each of the four products lands by the rule for directions: two parts along the same direction give a part along 1, and two along opposite directions give a part along $-1$. There are no cases: four multiplications and two additions of magnitudes.

For example, $5 + (-3)$ is $(5, 0) + (0, 3) = (5, 3)$, which is 2. Nothing has been subtracted yet.

**Collapse** turns a pair into its simplest form, with one part zero:

$$(P, N) \mapsto \begin{cases} (P - N,\ 0) & \text{if } P \geq N, \\ (0,\ N - P) & \text{if } P < N. \end{cases}$$

This is the only place a magnitude is subtracted, and the only place the two parts are compared. It is needed where a single value is needed: to divide exactly, to take a gcd, and to write a number out. Every integer has exactly one collapsed pair, and zero's is $(0, 0)$.

**Comparison** needs no collapse. $P_1 - N_1 < P_2 - N_2$ exactly when $P_1 + N_2 < P_2 + N_1$, again by adding $N_1 + N_2$ to both sides.

Without a collapse, the parts can keep growing while the value does not. Adding 7 and then $-7$ to $(2, 0)$ gives $(9, 7)$, still 2.

### Complex numbers

The same idea extends to all four directions. A number is four magnitudes $(m_0, m_1, m_2, m_3)$, one for each of $1, i, -1, -i$, and its value is

$$m_0 + m_1 i - m_2 - m_3 i.$$

**Addition** adds the four parts in place, so a sum across axes needs nothing new: $3 + 4i$ is $(3, 0, 0, 0) + (0, 4, 0, 0) = (3, 4, 0, 0)$.

**Negation** turns every part by 2: $(m_0, m_1, m_2, m_3) \mapsto (m_2, m_3, m_0, m_1)$.

**Multiplication** multiplies every part of one number by every part of the other. The product of a part along $d_1$ and a part along $d_2$ lies along $d_1 + d_2$ mod 4, by the rule for directions, so it is added into that slot. Sixteen products of magnitudes are added into four slots.

**Collapse** works on each axis separately: the parts along 1 and $-1$ collapse as an integer does, and so do the parts along $i$ and $-i$.

## Where they differ

| | Design A | Design B |
| --- | --- | --- |
| A number | one magnitude, one direction | one magnitude per direction: two for integers, four for complex numbers |
| Forms of one number | exactly one | many, until collapsed |
| Addition | three cases, one with an absolute difference | one formula, additions only |
| Multiplication | one product, directions added | four products for integers, sixteen for complex numbers |
| Where magnitudes are subtracted | in every addition of opposite directions | only in collapse |
| Sums across axes | not held | held directly |

## To return to: expressions in Design A

Points raised (Nov 1, 2026) and set aside until the addition discussion is settled:

- **Two 1-bit fields or one 2-bit field.** A direction $d$ can be read as two bits, $d = 2s + a$: an axis bit $a$ (0 real, 1 imaginary) and a sign bit $s$ (0 positive, 1 negative). Multiplying directions is then $a = a_1 \text{ XOR } a_2$ and $s = s_1 \text{ XOR } s_2 \text{ XOR } (a_1 \text{ AND } a_2)$, the rule of the 2002 Clifford coprocessor in the survey. This is separate from storing an expression's terms in fixed slots by axis (slot 0 real, slot 1 imaginary), which the author accepted.
- **A mandatory plain form.** Whether an expression whose real or imaginary term becomes zero must become a plain (non-expression) number, so that each value has one form. The author doubts it fits a design with headers; equality would otherwise have to treat both forms as the same.
- **Don't build an expression only to collapse it.** For example $(3 + 4i)(3 - 4i) = 25$: work out both axis results first, and produce a plain number when one is zero.
- **Keeping the common case fast** (both numbers plain, on the same axis): agreed, method open.
- **Zero's direction.** The survey's prior art fixes it; the author keeps it as it falls, since the operations here don't need it fixed.

Sources: [`research/expressions-machine-level.md`](research/expressions-machine-level.md).
