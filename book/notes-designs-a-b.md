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

So $(5, 0)$ is 5, $(5, 2)$ is $-5$ and $(5, 1)$ is $5i$. Zero is the magnitude 0, which has no direction, so it is always written $(0, 0)$. Every other number has exactly one pair.

**Multiplication** multiplies the magnitudes and adds the directions:

$$(m_1, d_1) \cdot (m_2, d_2) = (m_1 m_2,\ d_1 + d_2).$$

This holds because $m_1 i^{d_1} \cdot m_2 i^{d_2} = m_1 m_2 \cdot i^{d_1 + d_2}$. For example, $(-3) \cdot (-5)$ is $(3, 2) \cdot (5, 2) = (15, 0)$, which is 15, since $2 + 2 = 4 \equiv 0$.

**Exact division** divides the magnitudes and subtracts the directions, when $m_2$ divides $m_1$:

$$(m_1, d_1) \div (m_2, d_2) = (m_1 / m_2,\ d_1 - d_2).$$

**Negation** adds 2 to the direction: $-(m, d) = (m, d + 2)$, since $-1 \cdot i^d = i^{d+2}$.

**Addition** depends on how the two directions relate. If either number is zero, the sum is the other number. Otherwise there are three cases.

- **The same direction**, $d_1 = d_2$. The magnitudes add:
  $$(m_1, d) + (m_2, d) = (m_1 + m_2,\ d),$$
  since $m_1 i^d + m_2 i^d = (m_1 + m_2) i^d$.
- **Opposite directions**, $d_2 = d_1 + 2$. Then $m_1 i^{d_1} + m_2 i^{d_1 + 2} = (m_1 - m_2) \cdot i^{d_1}$. If $m_1 \geq m_2$, that is $(m_1 - m_2, d_1)$. Otherwise it is $(m_2 - m_1) \cdot (-i^{d_1}) = (m_2 - m_1) \cdot i^{d_2}$. In both cases the magnitude is $|m_1 - m_2|$, and the direction is that of the larger magnitude:
  $$(m_1, d_1) + (m_2, d_2) = (|m_1 - m_2|,\ \text{the direction of the larger}).$$
  If $m_1 = m_2$, the sum is zero, $(0, 0)$.
- **Different axes**, such as 3 and $4i$. Their sum $3 + 4i$ is not a magnitude times a single direction, so no pair holds it. Design A cannot add them; a sum across axes needs something beyond the design.

For example, $5 + (-3)$ is $(5, 0) + (3, 2)$: opposite directions, and 5 is larger, so the sum is $(2, 0)$, which is 2. And $3 + (-5)$ is $(3, 0) + (5, 2)$: the larger magnitude is 5, along direction 2, so the sum is $(2, 2)$, which is $-2$.

**Subtraction** is addition of the negation: $a - b = a + (-b)$. The negation only changes the direction of $b$, so subtraction is addition with the second direction turned by 2.

**Comparison** of two integers, with directions 0 and 2: a positive number is larger than a negative one; two positive numbers compare by magnitude; and two negative numbers compare by magnitude, reversed, since $-7 < -3$ although $7 > 3$.

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
