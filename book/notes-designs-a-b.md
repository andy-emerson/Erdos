# Notes: two designs for signed and complex numbers

*Temporary notes (Nov 1, 2026). The mathematics of spec 13's two designs, with no code, for the author to compare. To be taken into the book or archived.*

Machine arithmetic is modular arithmetic. A word of $n$ bits holds the values 0 to $2^n - 1$, and every result is taken mod $2^n$: a sum that passes the top wraps around to the bottom, and a subtraction that would go below zero wraps around to the top. It works like a clock with $2^n$ hours, and these notes use that picture. For example, with $n = 4$, $3 - 5 = -2 \equiv 14 \pmod{16}$: on a 16-hour clock, $3 - 5$ lands on 14. The arithmetic we need is on the number line, which has negative numbers, and sometimes on the complex plane, which has numbers off the line altogether. Both designs bridge the gap the same way: they split a number into a **magnitude**, how large it is, and a **direction**, which way it points.

**Magnitude.** A magnitude is a whole number, $0, 1, 2, \ldots$. Magnitudes can be added and multiplied, and the result is again a magnitude. They cannot always be subtracted: $3 - 5$ is not a magnitude. What can always be taken is the **absolute difference** $|a - b|$, the larger minus the smaller, which is a magnitude. In this, magnitudes are like modular arithmetic: neither has negative values, and neither has a direction. So the machine's arithmetic suits magnitudes, as long as a subtraction is never asked to go below zero.

**Direction on the line.** The number line has two directions, positive and negative. One bit $s$ names them: 0 for positive, 1 for negative, and a number is $m \cdot (-1)^s$. Multiplying two numbers multiplies their directions, and $(-1)^{s_1} \cdot (-1)^{s_2} = (-1)^{s_1 + s_2}$, so the bits add mod 2: two negatives make a positive, since $1 + 1 \equiv 0$. Negating flips the bit, which is adding 1 mod 2.

**Direction in the plane.** The complex plane has four directions along its two axes, the units $1, i, -1, -i$. Each is a power of $i$: $1 = i^0$, $i = i^1$, $-1 = i^2$ and $-i = i^3$, and $i^4 = 1$ again. So a direction is named by its exponent $d$, a number from 0 to 3, and multiplying two directions adds their exponents mod 4:

$$i^{d_1} \cdot i^{d_2} = i^{d_1 + d_2}, \quad \text{with } d_1 + d_2 \text{ taken mod } 4.$$

Negating is multiplying by $-1 = i^2$, so it adds 2 to the exponent. The directions 1 and $-1$ lie on one line, the real axis, and $i$ and $-i$ on another, the imaginary axis. The line's two directions are contained in the plane's four: $d = 0$ and $d = 2$ are the line's $s = 0$ and $s = 1$. Negating is a half turn in both. The line has two directions, so a half turn is one step, adding 1 mod 2; the plane has four, so a half turn is two steps, adding 2 mod 4.

## Design A: one magnitude with one direction

A number is a pair $(m, d)$ of a magnitude $m$ and a direction $d$. Its value is

$$m \cdot i^d.$$

So $(5, 0)$ is 5, $(5, 2)$ is $-5$ and $(5, 1)$ is $5i$.

**Zero** is any pair with magnitude 0. Zero is both real and imaginary, since it lies on both axes, and it is neither positive nor negative, since it lies in no direction. So its direction is irrelevant: $(0, d)$ is zero for every $d$. A zero keeps whatever direction the operation that produced it left, and no step sets it. Every number other than zero has exactly one pair.

On each axis, 1 and $i$ are the **positive** directions, and $-1$ and $-i$ the **negative** ones. Two directions are **opposite** when they differ by 2, as 1 and $-1$ do, and lie on **different axes** when they differ by 1, as 1 and $i$ do. A difference of 1 can run either way: 1 and $-i$ differ by $-1$, which is 3 mod 4.

### Comparison

Addition has to know how its two numbers relate before it can work. The numbers are split into a magnitude and a direction, and the checks follow the same split: one check on the directions, and one on the magnitudes.

**Zero.** Mathematically, zero is a special case: it lies on both axes and in no direction. A machine could check for it and skip the arithmetic when a number is zero, but it would gain nothing. Checking a magnitude for zero takes an instruction, and so does the addition or subtraction the check would skip. When the check comes back false, which is nearly always, the machine has done one instruction more for nothing. So addition never checks for zero, and the rules below give the right answer for a zero in any direction without it. The one exception is directions on different axes, where a zero must be caught: otherwise $5 + (0, 1)$ would become an expression instead of 5.

**The direction check** is a single XOR. Written in two bits, the directions are 00 for 1, 01 for $i$, 10 for $-1$ and 11 for $-i$. XOR compares them bit by bit and gives a 1 wherever they differ:

| $d_1$ \ $d_2$ | 00 | 01 | 10 | 11 |
| --- | --- | --- | --- | --- |
| **00** | 00 same | 01 diff | 10 opp | 11 diff |
| **01** | 01 diff | 00 same | 11 diff | 10 opp |
| **10** | 10 opp | 11 diff | 00 same | 01 diff |
| **11** | 11 diff | 10 opp | 01 diff | 00 same |

A result of 00 means the same direction. 10 means opposite: only the high bit differs, so the axis matches and the sign does not. A low bit of 1 means different axes. Integers use only 00 and 10, so for them the result is always 00 or 10.

**The magnitude check** is the subtraction itself. Opposite directions mean the magnitudes must be subtracted, and the subtraction also reports whether it had to wrap past zero, its **borrow** $b$. The borrow is exactly the answer to "was the first magnitude smaller?", and it is what turns the modular answer into an answer on the line:

| Subtraction | Borrow $b$ | Mathematical meaning | Machine meaning |
| --- | --- | --- | --- |
| $5 - 3 \equiv 2 \pmod{16}$ | 0 | magnitude $(-1)^0 \cdot 2 = 2$; answer $(-1)^0 \cdot 2 = 2$ | the subtraction gives 2: the magnitude, and the direction stays |
| $4 - 4 \equiv 0 \pmod{16}$ | 0 | magnitude $(-1)^0 \cdot 0 = 0$; answer 0 | the subtraction gives 0: the magnitude, and the answer is zero |
| $3 - 5 \equiv 14 \pmod{16}$ | 1 | magnitude $(-1)^1 \cdot 14 = -14 \equiv 2 \pmod{16}$; answer $(-1)^1 \cdot 2 = -2$ | the subtraction gives 14; a negation turns 14 into 2, the magnitude, and the direction flips |

In the first two rows the subtraction is all there is: the modular answer is already the answer on the line. The third row is where modular arithmetic and the line part ways. The subtraction wrapped, and the borrow says so. The borrow then separates the two parts of the answer. The factor $(-1)^b$, applied to the modular result and reduced mod 16 again, gives the magnitude, 2. Applied to the direction, it turns the result to point the other way, giving $-2$. On the machine, the first is one negation instruction, and the second is taking the other number's direction. So no comparison of magnitudes is made before subtracting: the subtraction answers the question as it works.

**The tree.** The checks run in the order the machine finds cheapest:

```text
direction check (XOR)
├── same           → add the magnitudes
├── opposite       → subtract; the borrow is the magnitude check
│   ├── no borrow  → the difference, first direction
│   └── borrow     → negate it, second direction
└── different axes → zero check
    ├── a zero     → it takes the other's direction, then add
    └── no zero    → an expression
```

**Ordering** two numbers, deciding which is larger, uses the same two checks and one more, for zero. Two zeros with different directions are equal, and a zero against a number on the other axis still compares, since zero lies on both: $0 < 3i$. There the zero check is needed for the right answer, not as a shortcut.

### Addition

Addition is the most involved operation here, and the reason is the problem these notes began with. The arithmetic we want lies on a line, which runs forever in both directions, but the machine's arithmetic lies on a circle, which wraps around. Going from the line to the circle is easy: every number simply lands somewhere on the clock. Coming back, from a result on the clock to the right place on the line, takes care, and addition is where that care is needed. The comparison's tree supplies it, and each of its branches has a rule.

**The same direction.** The magnitudes add, and the direction stays:

$$(m_1, d) + (m_2, d) = (m_1 + m_2,\ d),$$

since $m_1 i^d + m_2 i^d = (m_1 + m_2) i^d$.

**Opposite directions, no borrow.** The numbers pull against each other along one axis, and the first is at least as large. The subtraction did not wrap, so the clock landed on the true difference, and the sum keeps the first direction:

$$(m_1, d_1) + (m_2, d_2) = (m_1 - m_2,\ d_1).$$

The rule holds because $m_1 i^{d_1} + m_2 i^{d_1 + 2} = m_1 i^{d_1} - m_2 i^{d_1} = (m_1 - m_2) \cdot i^{d_1}$. If the magnitudes are equal, the difference is 0, and the sum is zero.

**Opposite directions, a borrow.** The second number wins. The subtraction wrapped, the clock's negation recovers the magnitude, $-(m_1 - m_2) = m_2 - m_1$, and the sum takes the second direction:

$$(m_1, d_1) + (m_2, d_2) = (-(m_1 - m_2),\ d_2), \quad \text{both steps mod } 2^n.$$

This negation is the clock's, an operation on magnitudes, not the negation of numbers below. For example, on a 16-hour clock, $3 + (-5)$ is $(3, 0) + (5, 2)$: the subtraction gives 14 with a borrow, the negation gives 2, and the direction is the second's, 2. The sum is $(2, 2)$, which is $-2$.

**Different axes.** If a magnitude is zero, that zero takes the other number's direction, and the addition continues as one in the same direction: $5 + (0, 1)$ becomes $5 + (0, 0) = 5$. If neither is zero, a sum such as $3 + 4i$ is not a magnitude times a single direction, so no pair holds it. Design A holds it as an expression, which is still to be worked out (see the end of these notes). Integers never reach this branch, since their directions are always 1 and $-1$.

The subtraction is the only place a magnitude is subtracted, and the borrow is the only place the magnitudes decide the direction.

**In pseudocode**, one line per machine instruction, following the tree:

```text
ROUTINE add(m1, d1, m2, d2)
    t ← d1 XOR d2                    direction check
    IF t = 0 THEN GO TO same
    u ← t AND 1
    IF u ≠ 0 THEN GO TO axes
    r ← m1 − m2                      opposite: subtract
    IF m1 < m2 THEN GO TO borrow
    RETURN r, d1
borrow:
    r ← 0 − r
    d1 ← d2
    RETURN r, d1
axes:                                different axes: zero check
    IF m1 = 0 THEN GO TO zero_first
    IF m2 = 0 THEN GO TO same
    RETURN an expression
zero_first:
    d1 ← d2
same:                                same direction: add
    r ← m1 + m2
    RETURN r, d1
```

`IF m1 < m2` reads the borrow of the subtraction just above it. On RISC-V, which has no borrow flag, the same question is asked by one compare-and-jump instruction placed just before the subtraction, so the count is the same. Not counting the final return, a sum in the same direction takes 3 instructions, and opposite directions take 6, or 8 when the subtraction wraps, on x86-64, ARM64 and RISC-V alike. The routine was checked against the true sum of every pair of small numbers in all four directions, including zeros in every direction ([`verify-notes/addition.py`](verify-notes/addition.py)).

### Negation

Negation adds 2 to the direction:

$$-(m, d) = (m,\ d + 2),$$

since $-1 \cdot i^d = i^2 \cdot i^d = i^{d+2}$. The magnitude is untouched. Negating zero gives a zero.

### Subtraction

Subtraction is addition of the negation: $a - b = a + (-b)$. Negating $b$ turns its direction by 2 and leaves its magnitude alone, so subtraction runs addition with the second direction turned. Addition's checks then find the branch, once, as for any addition. For example, $3 - 5$ is $(3, 0) + (5, 2)$: opposite directions with a borrow, which gives $-2$ as above.

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
