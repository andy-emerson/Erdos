# Note: coprime bases and how to read the notation

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

## 2. Writing the counts as a list (a vector)

Fix a list of factors, say $p = (2, 3, 5)$. Then each number is described by its list of counts, in the same order:

| Number | Factorization | Counts over $(2, 3, 5)$ |
| --- | --- | --- |
| 12 | $2^2 \cdot 3^1 \cdot 5^0$ | $(2, 1, 0)$ |
| 18 | $2^1 \cdot 3^2 \cdot 5^0$ | $(1, 2, 0)$ |
| 30 | $2^1 \cdot 3^1 \cdot 5^1$ | $(1, 1, 1)$ |

The list of counts is called an **exponent vector**. We name such vectors with Greek letters: $\alpha$ (alpha) for one number, $\beta$ (beta) for another. So for $12$ we write $\alpha = (2, 1, 0)$, meaning $\alpha_1 = 2$, $\alpha_2 = 1$, $\alpha_3 = 0$.

**Turning a vector back into a number.** The notation $p^{\alpha}$ is shorthand for "raise each factor to its count and multiply":

$$
p^{\alpha} = p_1^{\alpha_1} \cdot p_2^{\alpha_2} \cdot p_3^{\alpha_3}.
$$

With $p = (2, 3, 5)$ and $\alpha = (2, 1, 0)$: $p^{\alpha} = 2^2 \cdot 3^1 \cdot 5^0 = 4 \cdot 3 \cdot 1 = 12$. It is ordinary exponentiation, done once per entry of the list.

## 3. Operations on vectors

Section 1's rules (fewest, most, sum), plus subtraction, apply entry by entry:
- $\alpha \wedge \beta$, read "alpha meet beta": the **smaller** entry in each position.
- $\alpha \vee \beta$, read "alpha join beta": the **larger** entry in each position.
- $\alpha + \beta$: the entries **added**.
- $\alpha - \beta$: the entries **subtracted**. We only use this where every entry stays at least 0.

For $12$ and $18$, with $\alpha = (2,1,0)$ and $\beta = (1,2,0)$:

| Expression | Vector | Number | Meaning |
| --- | --- | --- | --- |
| $\alpha \wedge \beta$ | $(1, 1, 0)$ | $p^{\alpha \wedge \beta} = 6$ | the gcd |
| $\alpha \vee \beta$ | $(2, 2, 0)$ | $p^{\alpha \vee \beta} = 36$ | the lcm |
| $\alpha + \beta$ | $(3, 3, 0)$ | $p^{\alpha + \beta} = 216$ | the product |
| $\alpha - (\alpha \wedge \beta)$ | $(1, 0, 0)$ | $2$ | what 12 has that 18 lacks: 12's residual, $12/6$ |
| $\beta - (\alpha \wedge \beta)$ | $(0, 1, 0)$ | $3$ | what 18 has that 12 lacks: 18's residual, $18/6$ |
| $(\alpha \vee \beta) - \alpha$ | $(0, 1, 0)$ | $3$ | what 12 must be multiplied by to reach the lcm |
| $(\alpha \vee \beta) - \beta$ | $(1, 0, 0)$ | $2$ | what 18 must be multiplied by to reach the lcm |

So formulas written this way read in plain words:
- $\gcd = p^{\alpha \wedge \beta}$: "the gcd uses each factor the fewer number of times";
- $\text{lcm} = p^{\alpha \vee \beta}$: "the lcm uses each factor the greater number of times".

## 4. Why the factors do not have to be prime

Everything above needs only one property of the list $p$: **no two of its entries share a factor**. Entries with that property are called **pairwise coprime**. Primes have it, but so do other numbers. The list $(4, 15, 7)$ is pairwise coprime: no two of $4$, $15$ and $7$ share a factor, even though only $7$ is prime.

Such a list, when every number we care about is a product of its entries' powers, is called a **coprime base**.

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

**Where the pieces stay composite.** Suppose the numbers were $15$ and $45$. Then $\gcd(15, 45) = 15$, with residuals $1$ and $3$. Now $15$ and $3$ share $3$: $\gcd = 3$, residuals $5$ and $1$. The base is $(3, 5)$. But if the only numbers were $15$ and $225 = 15^2$, then $\gcd = 15$ with residuals $1$ and $15$, and the base is just $(15)$. Then $15$ has vector $(1)$ and $225$ has vector $(2)$. The $3$ and the $5$ always occur together here, so nothing forces them apart, and no factoring is ever done.

## 5. Why the "no shared factor" rule matters

If two entries of the list share a factor, the rules break.
- **Counts are no longer unique.** With the list $(2, 4)$, the number $16$ is $2^4$, or $4^2$, or $2^2 \cdot 4$: three different count vectors for one number.
- **Fewest and most give wrong answers.** With the list $(4, 6)$, the number $4$ has counts $(1, 0)$ and $6$ has $(0, 1)$. "Most of each" gives $(1, 1)$, which means $4 \cdot 6 = 24$, but the real lcm of $4$ and $6$ is $12$. The shared factor $2$ was counted twice.

Pairwise coprimality is exactly what prevents this. Each prime factor of each number belongs to exactly one entry of the list, so counting per entry never double counts.

## 6. Reading an addition in this notation

Fraction addition, written in this notation, is

$$
\frac{m_x}{p^{\alpha}} + \frac{m_y}{p^{\beta}} = \frac{m_x\, p^{(\alpha \vee \beta) - \alpha} + m_y\, p^{(\alpha \vee \beta) - \beta}}{p^{\alpha \vee \beta}}.
$$

In words: put both fractions over the lcm, $p^{\alpha \vee \beta}$, by multiplying each numerator by what its denominator lacks, $p^{(\alpha \vee \beta) - \alpha}$ and $p^{(\alpha \vee \beta) - \beta}$; then add the numerators. That is ordinary schoolbook fraction addition. The only difference is that the lcm and the multipliers are read off the count vectors rather than computed with a gcd.

**Example without cancellation:** $\tfrac{1}{12} + \tfrac{5}{18}$, base $(2, 3)$, with $\alpha = (2,1)$ and $\beta = (1,2)$.
- Lcm: $\alpha \vee \beta = (2, 2)$, which is $36$.
- Multipliers: $(\alpha \vee \beta) - \alpha = (0, 1)$, which is $3$; $(\alpha \vee \beta) - \beta = (1, 0)$, which is $2$.
- Sum: $\dfrac{1 \cdot 3 + 5 \cdot 2}{36} = \dfrac{13}{36}$.

**When can the result still be reduced?** Assume both fractions start in lowest terms. Then the result can only be reduced by a base entry that both denominators contain *the same number of times* (equal counts). Suppose one denominator has more of entry $p_i$ than the other. Then, in the new numerator, one term is a multiple of $p_i$, and the other shares no factor with $p_i$ (its original numerator was coprime to its denominator, and its multiplier contains no $p_i$). So every prime factor of $p_i$ divides one term but not the other, and therefore does not divide their sum.

In the example the counts are $(2,1)$ and $(1,2)$, unequal in both positions, so $13/36$ is guaranteed to be in lowest terms, with no gcd computed.

**Example with cancellation:** $\tfrac{1}{12} + \tfrac{1}{20}$, base $(2, 3, 5)$, with $12 = (2, 1, 0)$ and $20 = 2^2 \cdot 5 = (2, 0, 1)$.
- Lcm: $(2, 1, 1)$, which is $60$. Multipliers: $(0, 0, 1) = 5$ and $(0, 1, 0) = 3$.
- Sum: $\dfrac{1 \cdot 5 + 1 \cdot 3}{60} = \dfrac{8}{60}$.
- Equal counts: only the entry $2$, which both denominators contain twice. So the only possible cancellation is by a divisor of $2^2 = 4$; a gcd of $8$ with $4$ finds which one. Here it is $4$ itself, giving $\tfrac{2}{15}$. The factors $3$ and $5$ did not even need checking.

## 7. Symbols at a glance

| Symbol | Read as | Meaning |
| --- | --- | --- |
| $p = (p_1, p_2, \ldots)$ | "the base" | the list of pairwise coprime pieces |
| $\alpha = (\alpha_1, \alpha_2, \ldots)$ | "alpha" | a number's count of each piece (its exponent vector) |
| $p^{\alpha}$ | "p to the alpha" | the number those counts describe: $p_1^{\alpha_1} p_2^{\alpha_2} \cdots$ |
| $\alpha \wedge \beta$ | "alpha meet beta" | smaller count in each position, giving the gcd |
| $\alpha \vee \beta$ | "alpha join beta" | larger count in each position, giving the lcm |
| $\alpha + \beta$ | "alpha plus beta" | counts added, giving the product |
| $\alpha - \beta$ | "alpha minus beta" | counts subtracted, giving a quotient |
