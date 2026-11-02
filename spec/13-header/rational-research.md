# Spec 13: complex rationals on an axis

Nov 2, 2026. Research for the author: cheaper corrections for the raw machine arithmetic on complex rationals, $(\text{num}/\text{den}) \cdot i^a$.

## The design

$n$-bit words ($n = 64$), $T = 2^{n-1}$. A value is two words:

| Word | Contents |
| --- | --- |
| numerator $n$ | signed integer (two's complement), carries the sign |
| denominator $d$ | $D + T a$: $D \in 1 \ldots T-1$ in bits $0 \ldots n-2$, axis $a$ in bit $n-1$ |

Raw arithmetic is off by multiples of $T$, and $T k \equiv T (k \bmod 2) \pmod{2^n}$:

- $n_1 d_2 + n_2 d_1 = n_1 D_2 + n_2 D_1 + T a (n_1 + n_2)$ (same axis);
- $d_1 d_2 = D_1 D_2 + T (a_1 D_2 + a_2 D_1)$, since $T^2 \equiv 0$;
- $n_1 n_2$ lacks the sign of $i \cdot i = -1$.

**Conventions** (`book/logical-arithmetic.md`): one instruction per operator; inputs and the constants $T$, $\lnot T$, $1$ are free. x86-64 and ARM64 notes are hand counts, not assembled, marked *(hand)*.

**Checks.** Every formula is a straight-line program in [`test/rational/rat.py`](test/rational/rat.py)'s small DSL, one tuple per instruction, so the count is the length of the code that is run. Each is checked:
- exhaustively on $n = 3, 4, 5, 6$: every numerator pair, every denominator pair with $D_1 D_2 < T$, every axis pair, kept when the true numerator fits in $n$ signed bits (the cases of `fixes.py`; 124,352 add and 266,228 multiply cases at $n = 6$);
- on about 218,000 random 64-bit cases per operation (skewed sizes, plus the extremes: true numerator $-T$ and $T-1$, $D_1 D_2 = T-1$, $D = 1$).

The result must equal the expected words exactly: numerator $n_1 D_2 + n_2 D_1$ (or $\pm n_1 n_2$) mod $2^n$ over $D_1 D_2$, no reduction, and $\text{den} = D_1 D_2 + T \cdot \text{axis}$. A zero result may have either axis; every formula below also passes with the axis checked on zeros.

**Range assumed.** $D_1 D_2 < T$ and the true numerator fits in $n$ signed bits. The cross products $n_1 d_2$, $n_2 d_1$ may wrap: only the final sum must fit, as everything is mod $2^n$.

## Milestone 1: the current corrections, counted

Script: [`test/rational/m1.py`](test/rational/m1.py). All pass with 0 failures on $n = 3 \ldots 6$ and 64 (the raw, unfixed versions fail, as they should).

### Add, same axis: 11

$$
\begin{aligned}
\text{num} &= (n_1 d_2 + n_2 d_1) \text{ XOR } \big(((n_1 \text{ XOR } n_2) \text{ shl } (n-1)) \text{ AND } d_1\big) && 3 + 4 \\
\text{den} &= e_0 \text{ XOR } \big((e_0 \text{ XOR } d_1) \text{ AND } T\big), \quad e_0 = d_1 d_2 && 1 + 3
\end{aligned}
$$

- $(n_1 \text{ XOR } n_2) \text{ shl } (n-1) = T \cdot \text{parity}(n_1 + n_2)$, and AND with $d_1$ keeps it when $a = 1$. Neither the flag $a$ nor a mask is needed: the axis bit of $d_1$ is already in place.
- The denominator fix *sets* the top bit to $a$ (conditional choice, 3). `fixes.py`'s *flip* by $a \cdot [D_1 + D_2 \text{ even}]$ is 5 as written: `xor, not, shl, and, xor`.
- `fixes.py` written literally (flags extracted, then shifted back): 16. The estimate of 12 was one too high.

### Multiply: 10

$$
\begin{aligned}
m &= (d_1 \text{ AND } d_2) \text{ sar } (n-1) && 2 \quad (\text{mask of } a_1 a_2) \\
\text{num} &= \big((n_1 n_2) \text{ XOR } m\big) - m && 1 + 2 \\
\text{den} &= e_0 \text{ XOR } \big((e_0 \text{ XOR } d_1 \text{ XOR } d_2) \text{ AND } T\big), \quad e_0 = d_1 d_2 && 1 + 4
\end{aligned}
$$

- The denominator fix sets the top bit to $a_1 \text{ XOR } a_2$ (4). `fixes.py`'s flip by $(a_1 \land D_2 \text{ even}) \text{ XOR } (a_2 \land D_1 \text{ even})$ is 8 as written. `fixes.py` written literally: 14.

### Summary and machine notes

| | Raw | Fix | Total | Plain rational | x86-64 *(hand)* | ARM64 *(hand)* |
| --- | --- | --- | --- | --- | --- | --- |
| add | 4 | 4 + 3 | **11** | 4 | 11 + 4 `mov` (11 + 3 with `rcr`) | **8** |
| multiply | 2 | 4 + 4 | **10** | 2 | 10 + 4 `mov` (9 + 4 with `cmov`) | **7** |

**ARM64 add** *(hand)*, 8:
`mul p,n1,d2; madd t,n2,d1,p; eor f,n1,n2; and f,d1,f,lsl #63; eor num,t,f; mul den,d1,d2; lsr t,d1,#63; bfi den,t,#63,#1`.
- `madd` folds the cross add.
- The shifted-register operand folds the `shl` into the `and`.
- `lsr; bfi` writes the axis bit (2 in place of 3).

**ARM64 multiply** *(hand)*, 7:
`mul num,n1,n2; ands t,d1,d2; cneg num,num,mi; mul den,d1,d2; eor e,d1,d2; lsr e,e,#63; bfi den,e,#63,#1`.
- `ands` puts $a_1 a_2$ in the N flag, so `cneg` is the conditional negate (2 in place of 4).

**x86-64** *(hand)*:
- `imul r,r/m` is destructive, so each product and each fix temporary costs a `mov`.
- The top-bit copy can be `shl den,1; bt d1,63; rcr den,1` (3, no `mov`). `rcr` is multi-uop; this is *from memory*.
- The conditional negate can be `mov t,num; neg t; test d1,d2; cmovs num,t` (3 + `mov`): `test` sets SF to $a_1 a_2$.
- $\lnot T$ is not an imm32, so $T$ sits in a register (or use `btr`/`bts`/`bt` on bit 63).

**Where the cost is.** Against a plain rational, the axis costs +7 on add and +8 on multiply (+4 each on ARM64). Each operation pays twice: once for the numerator, once for the denominator's top bit.

Reproduce: `python3 -I test/rational/m1.py` (5 s).
