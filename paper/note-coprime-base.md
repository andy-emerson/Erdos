# Note: coprime bases and valuation notation

*A working note, not part of the paper. It explains one idea, the coprime base, and the notation used with it, independently of any of our designs.*

## 1. Start from something familiar: prime factorization

Every whole number greater than 1 is a product of primes in exactly one way:

$$
12 = 2 \cdot 2 \cdot 3 = 2^2 \cdot 3^1, \qquad 18 = 2 \cdot 3 \cdot 3 = 2^1 \cdot 3^2, \qquad 30 = 2^1 \cdot 3^1 \cdot 5^1.
$$

The small raised number, the **exponent**, says how many times a factor appears: $2^2$ means "2, twice". A factor that does not appear has exponent $0$, since $x^0 = 1$, which multiplies nothing. So we can just as well write

$$
12 = 2^2 \cdot 3^1 \cdot 5^0.
$$

Factorizations make gcds and lcms easy to see:
- **The gcd** takes each prime the *fewest* times it appears in either number. Comparing $12 = 2^2 3^1$ and $18 = 2^1 3^2$: for $2$ the fewest is $1$, and for $3$ it is $1$. So $\gcd = 2^1 3^1 = 6$.
- **The lcm** takes each prime the *most* times it appears. For $2$ the most is $2$, and for $3$ it is $2$. So $\text{lcm} = 2^2 3^2 = 36$.
- **The product** adds the counts: $12 \cdot 18 = 2^{2+1} 3^{1+2} = 2^3 3^3 = 216$.

That is the whole trick: once numbers are written as factor counts, gcd, lcm and product become *fewest*, *most* and *sum*, done on small counts instead of on the numbers.

## 2. The valuation $\nu_q(n)$

Fix a list of factors, which we call **pieces**: for now, $2$, $3$ and $5$. For a piece $q$ and a number $n$, write

$$
\nu_q(n) = \text{the number of times } q \text{ divides } n,
$$

that is, the largest $k$ with $q^k$ dividing $n$. Read it as "the valuation of $n$ at $q$", or simply "the exponent of $q$ in $n$". The $\nu$ is the Greek letter nu; the $q$ below it says which piece we are counting.

For example, $4$ divides $12$ but $8$ does not, so $\nu_2(12) = 2$. Over the pieces $2$, $3$, $5$:

| Number | Factorization | $\nu_2$ | $\nu_3$ | $\nu_5$ |
| --- | --- | --- | --- | --- |
| 12 | $2^2 \cdot 3^1 \cdot 5^0$ | $2$ | $1$ | $0$ |
| 18 | $2^1 \cdot 3^2 \cdot 5^0$ | $1$ | $2$ | $0$ |
| 30 | $2^1 \cdot 3^1 \cdot 5^1$ | $1$ | $1$ | $1$ |

**Turning valuations back into a number.** Raise each piece to its valuation and multiply:

$$
n = \prod_q q^{\nu_q(n)}.
$$

For $12$: $2^2 \cdot 3^1 \cdot 5^0 = 4 \cdot 3 \cdot 1 = 12$. This is the only product we need; everything else is done one piece at a time.

## 3. The rules, piece by piece

Section 1's rules (fewest, most, sum), plus subtraction, become four rules. For every piece $q$, and for numbers $a$ and $b$ that are products of the pieces:
- $\nu_q(\gcd(a, b)) = \min(\nu_q(a), \nu_q(b))$;
- $\nu_q(\text{lcm}(a, b)) = \max(\nu_q(a), \nu_q(b))$;
- $\nu_q(ab) = \nu_q(a) + \nu_q(b)$;
- $\nu_q(a/b) = \nu_q(a) - \nu_q(b)$, when $b$ divides $a$ (so no result is below $0$).

For $12$ and $18$, writing min and max for $\min(\nu_q(12), \nu_q(18))$ and $\max(\nu_q(12), \nu_q(18))$:

| Piece $q$ | $\nu_q(12)$ | $\nu_q(18)$ | min | max | sum | $\nu_q(12) - \min$ | $\nu_q(18) - \min$ |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2 | 2 | 1 | 1 | 2 | 3 | 1 | 0 |
| 3 | 1 | 2 | 1 | 2 | 3 | 0 | 1 |
| 5 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

Reading each column back as a number:
- min: $2^1 3^1 = 6$, the gcd;
- max: $2^2 3^2 = 36$, the lcm;
- sum: $2^3 3^3 = 216$, the product;
- $\nu_q(12) - \min$: $2^1 = 2$, what 12 has that 18 lacks: 12's residual, $12/6$;
- $\nu_q(18) - \min$: $3^1 = 3$, what 18 has that 12 lacks: 18's residual, $18/6$.

What 12 must be multiplied by to reach the lcm has valuations $\max - \nu_q(12)$, which are $0, 1, 0$: the number $3$. What 18 must be multiplied by has $\max - \nu_q(18)$, which are $1, 0, 0$: the number $2$. Each is the *other* number's residual, since $\max - \nu_q(a) = \nu_q(b) - \min$ at every piece.

So formulas written this way read in plain words:
- $\nu_q(\gcd) = \min(\ldots)$: "the gcd uses each factor the fewer number of times";
- $\nu_q(\text{lcm}) = \max(\ldots)$: "the lcm uses each factor the greater number of times".

## 4. Why the factors do not have to be prime

Everything above needs only one property of the list of pieces: **no two of them share a factor**. Pieces with that property are called **pairwise coprime**. Primes have it, but so do other numbers. The list $(4, 15, 7)$ is pairwise coprime: no two of $4$, $15$ and $7$ share a factor, even though only $7$ is prime.

Such a list, when every number we care about is a product of its pieces' powers, is called a **coprime base**.

For a piece $q$ of a coprime base, $\nu_q(n)$ as defined in §2 (the largest $k$ with $q^k$ dividing $n$) equals the exponent of $q$ in $n$'s unique expression over the base. The reason: the pieces are pairwise coprime, so $q^k$ divides $n$ exactly when it divides $n$'s own power of $q$.

Why not just use primes? Because finding the primes of a large number (factoring) can be very expensive. But splitting two numbers into pairwise coprime pieces needs only gcds and divisions, which are cheap.

### Building a coprime base with gcds alone

Repeat until nothing changes:
1. Find two numbers in the list that share a factor (their gcd is greater than 1).
2. Replace them by their gcd and their two residuals (each divided by the gcd).
3. Drop any 1s and merge duplicates.

**Example.** Start with $12$ and $18$:
- $\gcd(12, 18) = 6$; the residuals are $12/6 = 2$ and $18/6 = 3$. The list is $\{6, 2, 3\}$.
- $6$ and $2$ share $2$: $\gcd = 2$, residuals $6/2 = 3$ and $2/2 = 1$. Replace them by $2$, $3$, $1$; dropping $1$ and merging the duplicate $3$ gives $\{2, 3\}$.
- No two share a factor any more. The base is $(2, 3)$.

**Where the pieces stay composite.** Suppose the numbers were $15$ and $45$. Then $\gcd(15, 45) = 15$, with residuals $1$ and $3$. Now $15$ and $3$ share $3$: $\gcd = 3$, residuals $5$ and $1$. The base is $(3, 5)$. But if the only numbers were $15$ and $225 = 15^2$, then $\gcd = 15$ with residuals $1$ and $15$, and the base is just $(15)$. Then $\nu_{15}(15) = 1$ and $\nu_{15}(225) = 2$. The $3$ and the $5$ always occur together here, so nothing forces them apart, and no factoring is ever done.

## 5. Why the "no shared factor" rule matters

If two pieces of the list share a factor, the rules break.
- **Counts are no longer unique.** With the list $(2, 4)$, the number $16$ is $2^4$, or $4^2$, or $2^2 \cdot 4$: three different sets of counts for one number.
- **Fewest and most give wrong answers.** With the list $(4, 6)$, the number $4$ is $4^1 \cdot 6^0$ and $6$ is $4^0 \cdot 6^1$. "Most of each" gives $4^1 \cdot 6^1 = 24$, but the real lcm of $4$ and $6$ is $12$. The shared factor $2$ was counted twice.

Pairwise coprimality is exactly what prevents this. Each prime factor of each number belongs to exactly one piece, so counting per piece never double counts.

## 6. Reading an addition in this notation

Fraction addition, written in this notation, is

$$
\frac{m_x}{d_x} + \frac{m_y}{d_y} = \frac{m_x\, r_y + m_y\, r_x}{\text{lcm}(d_x, d_y)},
$$

where the residuals $r_x$, $r_y$ and the lcm are given by their valuations: for every piece $q$,

$$
\nu_q(r_x) = \nu_q(d_x) - \min(\nu_q(d_x), \nu_q(d_y)), \qquad \nu_q(r_y) = \nu_q(d_y) - \min(\nu_q(d_x), \nu_q(d_y)),
$$

$$
\nu_q(\text{lcm}(d_x, d_y)) = \max(\nu_q(d_x), \nu_q(d_y)).
$$

In words: put both fractions over the lcm by multiplying each numerator by what its denominator lacks; then add the numerators. What $d_x$ lacks is exactly $r_y$, because $\nu_q(d_x) + \nu_q(r_y) = \max(\nu_q(d_x), \nu_q(d_y))$ at every piece (§3), and likewise $d_y$ lacks $r_x$. That is ordinary schoolbook fraction addition. The only difference is that the lcm and the multipliers are read off the valuations rather than computed with a gcd.

**Example without cancellation:** $\tfrac{1}{12} + \tfrac{5}{18}$, base $(2, 3)$. Here $\nu_2(12) = 2$, $\nu_3(12) = 1$, and $\nu_2(18) = 1$, $\nu_3(18) = 2$.
- Lcm: $\nu_2 = \max(2, 1) = 2$ and $\nu_3 = \max(1, 2) = 2$, which is $36$.
- Residuals: $r_x$ has $\nu_2 = 1$, $\nu_3 = 0$, which is $2$; $r_y$ has $\nu_2 = 0$, $\nu_3 = 1$, which is $3$. So the multipliers are $3$ for $\tfrac{1}{12}$ and $2$ for $\tfrac{5}{18}$.
- Sum: $\dfrac{1 \cdot 3 + 5 \cdot 2}{36} = \dfrac{13}{36}$.

**When can the result still be reduced?** Assume both fractions start in lowest terms. Then the result can only be reduced at a piece $q$ with $\nu_q(d_x) = \nu_q(d_y) > 0$: one that both denominators contain *the same number of times*. Suppose instead $\nu_q(d_x) > \nu_q(d_y)$. Then $\nu_q(r_x) > 0$, so in the new numerator the term $m_y r_x$ is a multiple of $q$, while the term $m_x r_y$ shares no factor with $q$ (its original numerator $m_x$ was coprime to $d_x$, which $q$ divides, and $r_y$ is a product of other pieces, all coprime to $q$). So every prime factor of $q$ divides one term but not the other, and therefore does not divide their sum. The case $\nu_q(d_y) > \nu_q(d_x)$ is the same with $x$ and $y$ swapped.

In the example $\nu_2$ is $2$ and $1$, and $\nu_3$ is $1$ and $2$: unequal at both pieces, so $13/36$ is guaranteed to be in lowest terms, with no gcd computed.

**Example with cancellation:** $\tfrac{1}{12} + \tfrac{1}{20}$, base $(2, 3, 5)$. Over the pieces $2$, $3$, $5$, the valuations of $12$ are $2, 1, 0$, and those of $20 = 2^2 \cdot 5$ are $2, 0, 1$.
- Lcm: valuations $2, 1, 1$, which is $60$. Residuals: $r_x$ has valuations $0, 1, 0$, which is $3$, and $r_y$ has $0, 0, 1$, which is $5$.
- Sum: $\dfrac{1 \cdot 5 + 1 \cdot 3}{60} = \dfrac{8}{60}$.
- Equal valuations: only at the piece $2$, where $\nu_2(12) = \nu_2(20) = 2$. So the only possible cancellation is by a divisor of $2^2 = 4$; a gcd of $8$ with $4$ finds which one. Here it is $4$ itself, giving $\tfrac{2}{15}$. The factors $3$ and $5$ did not even need checking.

## 7. Symbols at a glance

| Symbol | Read as | Meaning |
| --- | --- | --- |
| $q$ | "q" | a piece (an entry of the coprime base) |
| $\nu_q(n)$ | "the valuation of $n$ at $q$", or "the exponent of $q$ in $n$" | the number of times $q$ divides $n$: the largest $k$ with $q^k$ dividing $n$ |
| $n = \prod_q q^{\nu_q(n)}$ | "$n$ is the product of each $q$ to the $\nu_q(n)$" | recovering a number from its valuations |
| $\min(\nu_q(a), \nu_q(b))$ | "the min of the valuations" | smaller count at each piece, giving $\nu_q(\gcd(a, b))$ |
| $\max(\nu_q(a), \nu_q(b))$ | "the max of the valuations" | larger count at each piece, giving $\nu_q(\text{lcm}(a, b))$ |
| $\nu_q(a) + \nu_q(b)$ | "the valuations added" | counts added, giving $\nu_q(ab)$ |
| $\nu_q(a) - \nu_q(b)$ | "the valuations subtracted" | counts subtracted, giving $\nu_q(a/b)$ when $b$ divides $a$ |

## 8. The same idea in other notations

Books write the same per-piece counting in several ways, all meaning what §3 says:
- **Product notation:** $\gcd(a, b) = \prod_q q^{\min(\nu_q(a),\, \nu_q(b))}$, as in §2, with the exponents written out in full.
- **Multi-index:** the base is a list $p = (p_1, p_2, \ldots)$, a number is $p^{\alpha} = p_1^{\alpha_1} p_2^{\alpha_2} \cdots$, and $\alpha \wedge \beta$ ("meet") and $\alpha \vee \beta$ ("join") are the entrywise min and max, so $\gcd = p^{\alpha \wedge \beta}$ and $\text{lcm} = p^{\alpha \vee \beta}$.
- **Brackets:** $(a, b)$ means $\gcd(a, b)$ and $[a, b]$ means $\text{lcm}(a, b)$.
- **Exponent vectors:** a number is written as the list of its valuations, such as $12 \mapsto (2, 1, 0)$ over $2, 3, 5$, and gcd and lcm are the entrywise min and max of two lists.
