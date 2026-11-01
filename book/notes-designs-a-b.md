# Notes: two designs for signed and complex numbers

*Temporary notes (Nov 1, 2026). The mathematics of spec 13's two designs, with no code, for the author to compare. To be taken into the book or archived.*

Both designs are built from two kinds of number.

A **magnitude** is a whole number, $0, 1, 2, \ldots$. Magnitudes can be added and multiplied, and the result is again a magnitude. They cannot always be subtracted: $3 - 5$ is not a magnitude. What can always be taken is the **absolute difference** $|a - b|$, the larger minus the smaller, which is a magnitude.

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

Two numbers on the same axis can be compared, in both direction and magnitude. On the real axis, $-7 < -3 < 3 < 7$; on the imaginary axis, in the same way, $-7i < -3i < 3i < 7i$.

**Equality.** Two numbers are equal when their magnitudes are equal and their directions are equal, or when both magnitudes are 0, whatever their directions.

**Order** takes three steps, magnitudes first, since zero is the case that most needs catching.

1. **Zero.** If both magnitudes are 0, the numbers are equal, whatever their directions. If one magnitude is 0, the other number's direction alone decides: a positive number is larger than zero, and a negative number smaller. Zero lies on both axes, so this holds even when zero's direction lies on the other axis: $(0, 0)$ against $3i$ gives $0 < 3i$.
2. **Directions.** Otherwise, if the directions are opposite, the number with the positive direction is larger.
3. **Magnitudes.** If the directions are the same, the magnitudes decide. In a positive direction, the larger magnitude is the larger number: $7 > 3$. In a negative direction, the smaller magnitude is the larger number: $-3 > -7$.

Without step 1, step 2 would call $(0, 0)$ larger than $(0, 2)$, though both are zero.

**Different axes.** Directions on different axes cannot be compared: $-i$ is not less than 1, and $-1$ is not less than $i$. Their magnitudes can be. Dropping the direction gives the **absolute value**,

$$|(m, d)| = m,$$

and absolute values compare as magnitudes. So the absolute values of 3 and $-5i$ compare as $3 < 5$, though the numbers themselves do not compare.

### Multiplication

Multiplication multiplies the magnitudes and adds the directions:

$$(m_1, d_1) \cdot (m_2, d_2) = (m_1 m_2,\ d_1 + d_2).$$

This holds because $m_1 i^{d_1} \cdot m_2 i^{d_2} = m_1 m_2 \cdot i^{d_1 + d_2}$. For example, $(-3) \cdot (-5)$ is $(3, 2) \cdot (5, 2) = (15, 0)$, which is 15, since $2 + 2 = 4 \equiv 0$. A product with a zero factor has magnitude 0, so it is zero, whatever its direction.

### Exact division

Exact division divides the magnitudes and subtracts the directions, when $m_2$ is not 0 and divides $m_1$:

$$(m_1, d_1) \div (m_2, d_2) = (m_1 / m_2,\ d_1 - d_2).$$

### Addition

Addition depends on how the two directions relate.

- **The same direction**, $d_1 = d_2$. The magnitudes add:
  $$(m_1, d) + (m_2, d) = (m_1 + m_2,\ d),$$
  since $m_1 i^d + m_2 i^d = (m_1 + m_2) i^d$.
- **Opposite directions**, $d_2 = d_1 + 2$. Then $m_1 i^{d_1} + m_2 i^{d_1 + 2} = (m_1 - m_2) \cdot i^{d_1}$. If $m_1 \geq m_2$, that is $(m_1 - m_2, d_1)$. Otherwise it is $(m_2 - m_1) \cdot (-i^{d_1}) = (m_2 - m_1) \cdot i^{d_2}$. In both cases the magnitude is $|m_1 - m_2|$, and the direction is that of the larger magnitude:
  $$(m_1, d_1) + (m_2, d_2) = (|m_1 - m_2|,\ \text{the direction of the larger}).$$
  If $m_1 = m_2$, the sum has magnitude 0, so it is zero.
- **Different axes**, such as 3 and $4i$. Their sum $3 + 4i$ is not a magnitude times a single direction, so no pair holds it. Design A cannot add them; a sum across axes needs something beyond the design.

Both of the first two cases give the right sum when one number is zero, whatever zero's direction: $(0, d) + (m, d) = (m, d)$, and $(0, d) + (m, d + 2) = (m, d + 2)$. The third case does not: zero lies on both axes, so $5 + (0, 1)$ is 5, though the directions lie on different axes. With directions on different axes, a check that a magnitude is 0 comes first: if it is, the sum is the other number. Integers never reach this case, since their directions are always 1 and $-1$.

For example, $5 + (-3)$ is $(5, 0) + (3, 2)$: opposite directions, and 5 is larger, so the sum is $(2, 0)$, which is 2. And $3 + (-5)$ is $(3, 0) + (5, 2)$: the larger magnitude is 5, along direction 2, so the sum is $(2, 2)$, which is $-2$.

### Subtraction

Subtraction is addition of the negation: $a - b = a + (-b)$. The negation only changes the direction of $b$, so subtraction is addition with the second direction turned by 2.

The opposite-direction case of addition is the one place where a magnitude is subtracted. It is also the one place where the two arithmetics meet: the direction of the result depends on which magnitude is larger.

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
