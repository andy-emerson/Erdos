# Notes: two designs for signed and complex numbers

*Temporary notes (Nov 1, 2026). The mathematics of spec 13's two designs, with no code, for the author to compare. To be taken into the book or archived.*

Machine arithmetic is modular arithmetic. A word of $n$ bits is like an odometer with $n$ binary digits. It counts up from $0000$ to $1111$; one more rolls it over to $0000$, and rolling back below $0000$ shows $1111$. Read as numbers, $n$ binary digits show the $2^n$ values from 0 to $2^n - 1$, and the rollover is arithmetic mod $2^n$. That is the same situation in a more useful form, and from here on these notes picture it as a clock with $2^n$ hours. On a 16-hour clock ($n = 4$), $15 + 1 = 16 \equiv 0$ and $3 - 5 = -2 \equiv 14 \pmod{16}$.

Modular arithmetic is circular: like the face of an analog clock, it comes back around to where it began. The arithmetic we want is linear: the number line runs forever in both directions. Turning modular arithmetic into linear arithmetic meets two problems, one at each end of the machine's range.

- **Below zero.** The number line has negative numbers, and modular arithmetic has none: $3 - 5$ lands on 14, not on $-2$.
- **Above $2^n - 1$.** The number line has no largest number, and modular arithmetic does: $15 + 1$ lands on 0, not on 16. This is **overflow**.

Splitting a number into a **magnitude**, how large it is, and a **direction**, which way it points, lets each problem be solved on its own. These notes solve the first: they build arithmetic operations that account for direction, so that results below zero come out right. The second needs a number system for large magnitudes, spec 13's size classes and pools, and is left aside here. In these notes a magnitude is treated as the machine already treats it, as a single word, and the examples keep magnitudes small, which is why the clock has only 16 hours.

**Magnitude.** A magnitude is a whole number, $0, 1, 2, \ldots$. Magnitudes can be added and multiplied, and the result is again a magnitude. They cannot always be subtracted: $3 - 5$ is not a magnitude. What can always be taken is the **absolute difference** $|a - b|$, the larger minus the smaller, which is a magnitude. In this, magnitudes are like modular arithmetic: neither has negative values, and neither has a direction. Adding magnitudes can overflow but never goes below zero. Subtracting them is where the first problem lives.

**Direction on the line.** The number line has two directions, positive and negative. A direction can be split off from a number as a power of $-1$: every number on the line is a magnitude times $(-1)^s$,

$$m \cdot (-1)^s,$$

with $s = 0$ for a positive number and $s = 1$ for a negative one, so $-5 = 5 \cdot (-1)^1$. Only whether $s$ is even or odd matters: $(-1)^s$ is 1 for every even $s$ and $-1$ for every odd $s$. So $s$ can be kept mod 2, as 0 for even and 1 for odd. Multiplying two numbers adds their exponents, $(-1)^{s_1} \cdot (-1)^{s_2} = (-1)^{s_1 + s_2}$, and two negatives make a positive because $1 + 1 \equiv 0 \pmod 2$. Negating adds 1 to $s$.

This is why the split works on a machine. Neither the magnitude nor the exponent is ever negative: the magnitude is kept mod $2^n$ and the exponent mod 2, and both suit modular arithmetic. The $-1$ itself is never computed; it is a fixed base that only says how to read the exponent. Two modular numbers, read together as $m \cdot (-1)^s$, give a value on the line, negative or not.

**Direction in the plane.** The complex plane has four directions along its two axes, the units $1, i, -1, -i$, and the same trick works with $i$ as the base. Every unit is a power of $i$, and the powers repeat every four steps, since $i^4 = 1$. So the exponent can be kept mod 4, and multiplying two directions adds their exponents:

$$i^{d_1} \cdot i^{d_2} = i^{d_1 + d_2}, \quad \text{with } d_1 + d_2 \text{ taken mod } 4.$$

Stored in a machine, the exponent is two bits, so a direction can be read three ways:

| Binary | 00 | 01 | 10 | 11 |
| --- | --- | --- | --- | --- |
| Exponent $d$ (mod 4) | 0 | 1 | 2 | 3 |
| Direction $i^d$ | 1 | $i$ | $-1$ | $-i$ |

The directions 1 and $-1$ lie on one line, the real axis, and $i$ and $-i$ on another, the imaginary axis. The line's two directions are contained in the plane's four: $d = 0$ and $d = 2$ are the line's $s = 0$ and $s = 1$. Negating is a half turn in both. The line has two directions, so a half turn is one step, adding 1 mod 2; the plane has four, so a half turn is two steps, adding 2 mod 4.

**A number** is then a pair $(m, d)$ of a magnitude $m$ and a direction $d$, with value

$$m \cdot i^d.$$

So $(5, 0)$ is 5, $(5, 2)$ is $-5$ and $(5, 1)$ is $5i$. On each axis, 1 and $i$ are the **positive** directions, and $-1$ and $-i$ the **negative** ones. Two directions are **opposite** when they differ by 2, as 1 and $-1$ do, and lie on **different axes** when they differ by 1, as 1 and $i$ do. A difference of 1 can run either way: 1 and $-i$ differ by $-1$, which is 3 mod 4.

**Zero** is any pair with magnitude 0. Zero is both real and imaginary, since it lies on both axes, and it is neither positive nor negative, since it lies in no direction. So its direction is irrelevant: $(0, d)$ is zero for every $d$. A zero keeps whatever direction the operation that produced it left, and no step sets it. Every number other than zero has exactly one pair.

## Design A: one magnitude with one direction

Design A stores each number as one such pair. Its addition begins with a case check, which finds the case before any arithmetic is done.

### The case check

Adding two numbers in the same direction adds their magnitudes. Adding two in opposite directions subtracts them, since one pulls against the other. That subtraction is the only place a magnitude can go below zero, so it is where the first problem has to be solved. Addition therefore needs two answers before it can work: how the two directions relate, and, when they are opposite, which magnitude is larger. Each is one check.

**The direction check** is one XOR of the two directions' bits, from the table above. It gives a 1 wherever the bits differ:

| $d_1$ \ $d_2$ | 00 | 01 | 10 | 11 |
| --- | --- | --- | --- | --- |
| **00** | 00 same | 01 diff | 10 opp | 11 diff |
| **01** | 01 diff | 00 same | 11 diff | 10 opp |
| **10** | 10 opp | 11 diff | 00 same | 01 diff |
| **11** | 11 diff | 10 opp | 01 diff | 00 same |

00 means the same direction. 10 means opposite: the axis matches and the sign does not. A low bit of 1 means different axes. Integers only ever give 00 or 10.

**The magnitude check** is needed only for opposite directions, and there the subtraction answers it. The machine's subtraction instruction reports one more thing as it works: whether it had to wrap below zero, as a schoolbook subtraction would need to borrow past its leftmost column. That report is the **borrow**:

$$b = \begin{cases} 1 & \text{if } m_1 < m_2, \\ 0 & \text{otherwise.} \end{cases}$$

The borrow gives the result's direction at once. A borrow means the second number won, so the first direction turns halfway round:

$$d \equiv d_1 + 2b \pmod 4.$$

It also gives the magnitude, from the same subtraction. A borrow means the clock wrapped, and negating on the clock unwraps it:

$$m \equiv (-1)^b \cdot (m_1 - m_2) \pmod{2^n}.$$

Both are modular, the direction mod 4 and the magnitude mod $2^n$, yet together, as $m \cdot i^d$, they give the answer on the line, even when it is below zero. They do nothing for overflow: a result of $2^n$ or more is still the second problem. The examples below are $5 + (-3)$, $4 + (-4)$ and $3 + (-5)$, on a 16-hour clock, so the first direction is 1 and all modular arithmetic in the table is mod 16:

| Subtraction | Borrow $b$ | Modular direction | Modular magnitude | Linear result |
| --- | --- | --- | --- | --- |
| $5 - 3 \equiv 2$ | 0 | $(-1)^0 \cdot 1 = 1$ | $(-1)^0 \cdot 2 = 2$ | 2 |
| $4 - 4 \equiv 0$ | 0 | $(-1)^0 \cdot 1 = 1$ | $(-1)^0 \cdot 0 = 0$ | 0 |
| $3 - 5 \equiv 14$ | 1 | $(-1)^1 \cdot 1 = -1$ | $(-1)^1 \cdot 14 = -14 \equiv 2$ | $-2$ |

The machine never computes $(-1)^b$. The subtraction gives its result and $b$ together. With no borrow, the result is the magnitude and nothing more is done. With a borrow, two more instructions run: a negation, which turns 14 into 2, and taking the second number's direction.

**Zero** needs no check here. A machine could test for it and skip the arithmetic, but the test costs as much as the addition or subtraction it would skip, so it would only add an instruction. The rules above already give the right answer for a zero in any direction. The one exception is different axes, where $5 + (0, 1)$ must give 5 and not an expression.

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

**Not an order.** The same checks, with one more for zero (two zeros with different directions are equal), can say which of two numbers is larger, but only when they share an axis. For two nonzero numbers on different axes there is no answer. This is the **component order**: real parts are compared with real parts and imaginary with imaginary, and when both pairs differ, nothing is decided. It is not a total order. No total order can agree with multiplication either, since such an order makes every nonzero square positive, and $i^2 = -1$. So the case check is a helper inside addition, not a standalone comparison. Addition's two halves, the case check and the rule for each case, are worked out separately here, before they are joined in one routine.

### Addition

Addition is more involved here than on paper, because of the problem these notes began with: it has to bring a result on the circle back to the right place on the line. The case check's tree does that, and each of its branches is a case with its own rule.

**Same direction.** The magnitudes add, and the direction stays. With $d_1 = d_2 = d$,

$$(m_1, d_1) + (m_2, d_2) = (m_1, d) + (m_2, d) = (m_1 + m_2,\ d),$$

since $m_1 i^d + m_2 i^d = (m_1 + m_2) i^d$.

**Opposite directions.** The magnitudes subtract, and the borrow settles the rest, by the two formulas of the magnitude check:

$$(m_1, d_1) + (m_2, d_2) = \big((-1)^b \cdot (m_1 - m_2),\ d_1 + 2b\big).$$

With no borrow, the first number is at least as large, and the sum is $m_1 - m_2$ in the first direction; equal magnitudes give zero. With a borrow, the second is larger, and the sum is the negated difference in the second direction, since $d_1 + 2 = d_2$. The rule holds because $m_1 i^{d_1} + m_2 i^{d_1 + 2} = (m_1 - m_2) \cdot i^{d_1}$.

**Different axes.** If a magnitude is zero, that zero takes the other number's direction, and the addition becomes one in the same direction. With $m_2 = 0$,

$$\begin{aligned} (m_1, d_1) + (m_2, d_2) &= (m_1, d_1) + (0, d_2) \\ &= (m_1, d_1) + (0, d_1) \\ &= (m_1 + 0,\ d_1) = (m_1, d_1), \end{aligned}$$

and similarly when $m_1 = 0$. For example, $5 + (0, 1)$ becomes $5 + (0, 0) = 5$. If neither magnitude is zero, a sum such as $3 + 4i$ is not a magnitude times a single direction, and Design A holds it as an expression, still to be worked out (see the end of these notes). Integers never reach this case.

The rules were checked against the true sum of every pair of small numbers in all four directions, including zeros in every direction ([`verify-notes/addition.py`](verify-notes/addition.py)).

### Negation

Negation adds 2 to the direction:

$$-(m, d) = (m,\ d + 2),$$

since $-1 \cdot i^d = i^2 \cdot i^d = i^{d+2}$. The magnitude is untouched. Negating zero gives a zero.

### Subtraction

Subtraction is addition of the negation: $a - b = a + (-b)$. Negating $b$ turns its direction by 2 and leaves its magnitude alone, so subtraction runs addition with the second direction turned. Addition's checks then find the branch, once, as for any addition. For example, $3 - 5$ is $(3, 0) + (5, 2)$: opposite directions with a borrow, which gives $-2$, as in the table.

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

### Rationals, an open problem

Design B nests the wrong way round for rationals. A rational in B would be a ratio of two of its complex numbers, rather than a complex number whose parts are rationals. Both give the same field, since every ratio of Gaussian integers can be written as $p + qi$ with $p$ and $q$ rational, but they differ in reduction to lowest terms. A pair of rationals reduces with the ordinary gcd of integers. A ratio of Gaussian integers needs a gcd of Gaussian integers, a different and slower algorithm, unique only up to a factor of $1$, $i$, $-1$ or $-i$.

## Where they differ

| | Design A | Design B |
| --- | --- | --- |
| A number | one magnitude, one direction | one magnitude per direction: two for integers, four for complex numbers |
| Forms of one number | exactly one | many, until collapsed |
| Addition | a direction check; for opposite directions, a subtraction and its borrow | one formula, additions only |
| Multiplication | one product, directions added | four products for integers, sixteen for complex numbers |
| Where magnitudes are subtracted | in every addition of opposite directions | only in collapse |
| Sums across axes | not held | held directly |

## To return to: expressions in Design A

Points raised (Nov 1, 2026) and set aside until the addition discussion is settled:

- **Two 1-bit fields or one 2-bit field.** A direction $d$ can be read as two bits, $d = 2s + a$: an axis bit $a$ (0 real, 1 imaginary) and a sign bit $s$ (0 positive, 1 negative). Multiplying directions is then $a = a_1 \text{ XOR } a_2$ and $s = s_1 \text{ XOR } s_2 \text{ XOR } (a_1 \text{ AND } a_2)$, the rule of the 2002 Clifford coprocessor in the survey. This is separate from storing an expression's terms in fixed slots by axis (slot 0 real, slot 1 imaginary), which the author accepted.
- **A mandatory plain form.** Whether an expression whose real or imaginary term becomes zero must become a plain (non-expression) number, so that each value has one form. The author doubts it fits a design with headers; equality would otherwise have to treat both forms as the same.
- **Don't build an expression only to collapse it.** For example $(3 + 4i)(3 - 4i) = 25$: work out both axis results first, and produce a plain number when one is zero.
- **Keeping the common case fast** (both numbers plain, on the same axis): agreed, method open.
- **Ordering expressions.** Once a number can carry both a real and an imaginary part, two numbers bring four parts to compare. Whether any order is wanted then (the lexicographic one, real part first, is total and agrees with addition but not multiplication) is open.
- **Zero's direction.** The survey's prior art fixes it; the author keeps it as it falls, since the operations here don't need it fixed.

Sources: [`research/expressions-machine-level.md`](research/expressions-machine-level.md).
