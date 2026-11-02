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
| add | 4 | 4 + 3 | **11** | 4 | 11 + 5 `mov` (11 + 4 with `rcr`) | **8** |
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
- `imul r,r/m` is destructive, so each product and each fix temporary costs a `mov` when the inputs must be kept (fewer when they may be overwritten).
- The top-bit copy can be `shl den,1; bt d1,63; rcr den,1` (3, no `mov`). `rcr` is multi-uop; this is *from memory*.
- The conditional negate can be `mov t,num; neg t; test d1,d2; cmovs num,t` (3 + `mov`): `test` sets SF to $a_1 a_2$.
- $\lnot T$ is not an imm32, so $T$ sits in a register (or use `btr`/`bts`/`bt` on bit 63).

**Where the cost is.** Against a plain rational, the axis costs +7 on add and +8 on multiply (+4 each on ARM64). Each operation pays twice: once for the numerator, once for the denominator's top bit.

Reproduce: `python3 -I test/rational/m1.py` (5 s).

## Milestone 2: better formulas by reasoning

Script: [`test/rational/m2.py`](test/rational/m2.py), with 24 formula checks and 0 failures. Each is checked exhaustively on $n = 3 \ldots 6$ (division included), on random 64-bit cases, and with the axis also checked on zero results.

### Two ideas

1. **Strip the axis before the products, not after.** In the offset layout (the design), $D = d \text{ AND } \lnot T$ is one instruction, and then the plain rational arithmetic is exact. Putting the axis back costs 1–2.
2. **Make the axis a sign: the negated layout.** Store $d = (-1)^a D$ in two's complement. Since $1 \le D < T$, the top bit is still $a$, so the axis test is unchanged.
   - $\pm 1$ is a multiplicative unit of order 2, so $d_1 d_2 = (-1)^{a_1 \oplus a_2} D_1 D_2$ **exactly**: the multiply's denominator needs no fix.
   - This is the "leak becomes the fix" choice. With $D + Ta$ the axis is additive, so the product leaks $T(a_1 D_2 + a_2 D_1)$. With $(-1)^a D$ it is multiplicative, so the product carries it.

**What no sign layout can remove.** The signs of (num, den) form $\mathbb{Z}_2 \times \mathbb{Z}_2$ under multiplication. The axis needs $\mathbb{Z}_4$ ($i^2 = -1$). So multiply always needs one conditional negate, keyed on $a_1 a_2$.

A unit of order 4 does exist mod $2^n$: $u = 1 + 2^{n-2}$, with $u^2 = 1 + T$. But $D u \equiv D$ when $4 \mid D$, so an encoding $D u^a$ loses the axis on even $D$.

### Offset layout ($d = D + Ta$)

**Add, 8** (was 11). Sub is the same with $p - q$:

$$
D_1 = d_1 \text{ AND } \lnot T,\quad D_2 = d_2 \text{ AND } \lnot T,\quad A = d_1 \text{ XOR } D_1 \ (= Ta)
$$
$$
\text{num} = n_1 D_2 + n_2 D_1, \qquad \text{den} = D_1 D_2 \text{ OR } A
$$

- $A = d_1 \text{ AND } T$ and $\text{den} = D_1 D_2 + A$ also work.
- ARM64 *(hand)*: `and D1; and D2; and A,d1,#T; mul p,n1,D2; madd num,n2,D1,p; madd den,D1,D2,A`, so **6**.

**Multiply, 10** (unchanged; nothing better found by reasoning):
- The numerator needs a conditional negate (4).
- The denominator's top bit must be *set* to $a_1 \oplus a_2$ (4).
- The leak $a_1 D_2 \oplus a_2 D_1$ depends on the parities of the $D$s, so no operand pre-fix removes it. Stripping first costs 6 for the denominator, against 5 for raw-then-set.

**Division, 15.** The quotient is $q = n_1 D_2 / (D_1 |n_2|)$ on axis $a_1 \oplus a_2$, with numerator sign $\sigma(n_2) \cdot (-1)^{a_2 \bar a_1}$, since $i^{-1} = -i$.

$$
\begin{aligned}
\text{den} &= D_1 \cdot |n_2| \text{ OR } \big((d_1 \text{ XOR } d_2) \text{ AND } T\big) \\
\nu &= \big(n_2 \text{ XOR } (d_2 \text{ AND NOT } d_1)\big) \text{ sar } (n-1) \\
\text{num} &= \big((n_1 D_2) \text{ XOR } \nu\big) - \nu
\end{aligned}
$$

### Negated layout ($d = (-1)^a D$)

Write $s = (-1)^a$ and $m = $ its mask.

**Multiply, 6** (was 10): only $i \cdot i = -1$ is fixed.

$$
m = (d_1 \text{ AND } d_2) \text{ sar } (n-1), \qquad \text{num} = \big((n_1 n_2) \text{ XOR } m\big) - m, \qquad \text{den} = d_1 d_2
$$

ARM64 *(hand)*: `mul num,n1,n2; ands t,d1,d2; cneg num,num,mi; mul den,d1,d2`, so **4**.

**Add, 8: negate operand 2 as a pair first.** $(n_2, d_2) \to (s n_2, s d_2)$ is the same fraction, now with $s d_2 = D_2$. Then the raw add is exact: $n_1 D_2 + (s n_2)(s D_1) = n_1 D_2 + n_2 D_1$ and $d_1 D_2 = s D_1 D_2$.

$$
m = d_2 \text{ sar } (n-1),\quad s = m \text{ OR } 1,\quad D_2 = d_2 \cdot s,\quad \text{num} = n_1 D_2 + (n_2 s)\, d_1,\quad \text{den} = d_1 D_2
$$

- With $(x \text{ XOR } m) - m$ in place of $\cdot s$: 9. Raw add then negating both result words: 9.
- Sub is the same with $-$ (`msub` on ARM64).
- ARM64 *(hand)*: `cmp d2,#0; cneg n2s,n2,lt; cneg D2,d2,lt; mul p,n1,D2; madd num,n2s,d1,p; mul den,d1,D2`, so **6**.
- The $\cdot s$ form puts two multiplies in series on the path (`sar` → `or` → `mul` → `mul` → `add`). The mask or `cneg` form is shorter in latency.

**Division, 11** (offset: 15). With $p = n_1 d_2$ and $q = d_1 n_2$:

$$
\mu = (d_2 \text{ XOR } n_2) \text{ sar } (n-1),\quad \text{den} = (q \text{ XOR } \mu) - \mu
$$
$$
\nu = \big((d_1 \text{ AND } d_2) \text{ XOR } n_2\big) \text{ sar } (n-1),\quad \text{num} = (p \text{ XOR } \nu) - \nu
$$

- Here $\text{den} = \sigma(n_2)\, s_2\, q$ and $\text{num} = \sigma(n_2)(-1)^{a_1 a_2} p$.
- $\nu$ is the multiply's mask with the sign of $n_2$ folded in.
- ARM64 *(hand)*: about 9 (`eor`/`and`, `cmp`, `cneg` for each word).
- $\text{tD} = D_1 |n_2| < T$ is assumed, as for the other operations.

### The shared correction

In the negated layout, one prefix serves both operations:

$$
m = (d_1 \text{ AND } d_2) \text{ sar } (n-1), \qquad \text{negate operand 2 by } m
$$

- Multiply negates $n_2$ only: $m$ is the mask of $a_1 a_2$, so $\text{num} = n_1 (s n_2)$ and $\text{den} = d_1 d_2$. Total 6.
- Add (same axis, so $m$ is the mask of $a$) negates $n_2$ and $d_2$, then does the raw add. Total 9, or 8 with $m = d_2 \text{ sar } (n-1)$.

So both operations are plain rational arithmetic plus a conditional negate of operand 2 by a mask made from the denominators' top bits. Multiply negates one word, add negates two. Division uses the same mask with $n_2$'s sign XORed in.

### Negate and conjugate (either layout)

- Negate: $\text{num} = -n$, 1 instruction (den unchanged; $n = -T$ wraps, as with int64).
- Conjugate ($i \to -i$): $\text{num} = (n \text{ XOR } m) - m$ with $m = d \text{ sar } (n-1)$, so 3 (ARM64 `cmp; cneg`: 2).

### Summary

| | Current (m1) | Offset, best | Negated, best | Plain rational | ARM64 *(hand)* offset / negated |
| --- | --- | --- | --- | --- | --- |
| add / sub | 11 | **8** | **8** | 4 | 6 / 6 |
| multiply | 10 | 10 | **6** | 2 | 7 / **4** |
| divide | — | 15 | **11** | 2 (+ sign of $n_2$) | — / ~9 |
| negate | 1 | 1 | 1 | 1 | 1 / 1 |
| conjugate | — | 3 | 3 | — | 2 / 2 |

- **Recommendation: the negated layout.** It matches the offset layout on add (8 / ARM64 6) and saves 4 on multiply (6 / ARM64 4) and 4 on division. The axis stays in the top bit.
- **Cost elsewhere.** $D = |d|$ is 3 (`sar; xor; sub`; ARM64 `cmp; cneg`, 2) instead of 1 (`and`). That matters for gcd/reduction (out of scope). The zero test (num = 0) and the axis test (top bit) are unchanged.
- **x86-64** *(hand)*:
  - multiply (negated): `imul; mov,neg; test d1,d2; cmovs; imul`, which is 5 + about 3 `mov`;
  - add (negated): 8 + up to 5 `mov`, or the pair negate by `neg`/`neg`/`test`/`cmovs`/`cmovs` (5 for 4).
- **Overflow.** As in milestone 1, only the final words must fit: $D_1 D_2 < T$ (division: $D_1 |n_2| < T$) and the true numerator in $n$ signed bits. The pre-negate $s n_2$ and the cross products may wrap.

Reproduce: `python3 -I test/rational/m2.py` (15 s).
