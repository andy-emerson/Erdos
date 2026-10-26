# The plane variant: research findings

Oct 26, 2026 · Andrew Emerson

This note evaluates a possible sibling of the Q family, raised in the [Q v0.3](../spec/q-v0.3.md) directions for research: a format holding a whole Gaussian rational $(a + bi)/c$, three integers sharing one word, instead of the Q family's values on one axis, where a complex number is a real value plus an imaginary one. The question is whether the plane variant has a significant advantage in memory or speed, enough to justify a second spec.

The code is in [`bench/q-research`](../../bench/q-research/) (`plane.py`, `plane_format.py`, `cbench/cplx.c`); the full results are in `bench/q-research/results/research.md`. Every result was checked against an exact computation, and an independent review of the code and the fairness of the comparisons came before the measured run.

## The format

A plane value has no axis bit: the axis is implied by which of $a$ and $b$ is nonzero. The header instead holds two cut positions in the word and a sign for each of $a$ and $b$. In the word come $|a|$, then $|b|$, then $c - 1$, so, as in Q72, every pattern is a value.

For a 64-bit word each cut needs 7 bits, because a field may be empty, so the header is $2 + 7 + 7 = 16$ bits: **P80**, an 80-bit value. For a 128-bit word the header is 18 bits, in practice 24: **P152**. The fair comparisons at equal size are P80 against two Q40s (80 bits), and P152 against two Q72s (144 bits; both carry 128 data bits).

Share of 20,000 values that fit, and median bits needed (without headers):

| Values | P80 | 2 × Q40 | P152 | 2 × Q72 | bits, plane | bits, pair |
| --- | --- | --- | --- | --- | --- | --- |
| complex decimals, 3 places, parts below $10^3$ | 100% | 100% | 100% | 100% | 47 | 53 |
| complex decimals, 6 places, parts below $10^6$ | 0% | 0% | 100% | 100% | 97 | 112 |
| products of two 3-place complex decimals | 0.01% | 0% | 100% | 100% | 94 | 110 |
| quotients of two 3-place complex decimals | 0.005% | 0% | 100% | 2.4% | 113 | 151 |
| quotients of Gaussian integers below 1000 | 100% | 20% | 100% | 100% | 53 | 72 |
| quotients of Gaussian integers below $10^6$ | 0.01% | 0% | 100% | 2.7% | 113 | 151 |
| products of three Gaussian-integer quotients below 100 | 3.9% | 0.25% | 100% | 59% | 93 | 123 |

The plane format needs fewer bits whenever both parts are present, because it stores their common denominator once. The difference is large for quotients, whose two parts always share the denominator $c^2 + d^2$.

It needs more where values are on one axis, which is most data. A real value pays a 16-bit header instead of 8, and its numerator and denominator compete with an empty field for the cut positions.

## Speed

In arrays, a plane vector is two integer vectors over one shared denominator, $(A + Bi)/D$. The Q family's vector form holds a complex array as a real vector and an imaginary vector, each over its own denominator. Both keep the fast path: complex multiplication is four integer products and two sums.

Time for $10^6$ complex decimals (3 places) and $2.5 \times 10^5$ (6 places, where products need 128 bits):

| Operation | plane vector | axis pair, same code | GMP `mpq_t` pairs (C) | FLINT `fmpzi` over one denominator (C) | checked int64 pairs (C) |
| --- | --- | --- | --- | --- | --- |
| multiply, 3 places | 11.6 ms | 12.5 ms | 609 ms | 13.8 ms | 4.9 ms |
| $x y + w$, 3 places | 16.3 ms | 16.8 ms | 798 ms | 19.6 ms | 5.3 ms |
| dot product, 3 places | 11.8 ms | 12.5 ms | 832 ms | 17.1 ms | 2.2 ms |
| sum of $\lvert x \rvert^2$, 3 places | 5.6 ms | 6.1 ms | 221 ms | 6.3 ms | 2.8 ms |
| multiply, 6 places | 4.5 ms | 4.7 ms | 172 ms | 3.9 ms | overflow |
| $x y + w$, 6 places | 6.5 ms | 6.9 ms | 232 ms | 10.9 ms | overflow |
| dot product, 6 places | 4.9 ms | 5.1 ms | 244 ms | 8.3 ms | overflow |
| sum of $\lvert x \rvert^2$, 6 places | 2.4 ms | 2.5 ms | 70 ms | 4.1 ms | overflow |

Written the same way, plane vectors and axis pairs are within 2–10% of each other, which is within the spread of the measurements (on minimum times the axis pair is sometimes faster). Both use 16 bytes per complex element.

Both are 29–70× faster than GMP rationals from C. Against FLINT's Gaussian integers over a shared denominator, they range from 1.2× slower to 1.7× faster. Checked int64 arithmetic in C is 2–5.4× faster where it does not overflow.

## Finding

The plane variant has no speed advantage, and no memory advantage in arrays. Its one advantage is compact storage of single complex values whose two parts are both present, above all complex quotients. Even there it is a storage format, since arithmetic on arrays goes through the vector form either way. It costs a larger header, fewer bits per part, and a worse fit for values on one axis.

By the criterion set for it, a significant advantage in memory or speed, the plane variant does not justify a second spec. The axis family keeps its distinguishing property: a complex number is the sum of a real value and an imaginary one, as in mathematics, and an array of values on one axis carries no unused second part.

The one open item is complex quotients. A program that stores many of them as single values will find they promote from two Q72s to two Q144s, where a plane format would have kept them in 152 bits.
