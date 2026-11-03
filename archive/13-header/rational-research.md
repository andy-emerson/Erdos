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

## Milestone 3: superoptimizer

**Files.**
- [`test/rational/rsuper.c`](test/rational/rsuper.c): exhaustive search over straight-line DAG programs, adapted from `complement/superopt.c`.
- [`test/rational/logs/`](test/rational/logs/): every run's output.
- [`test/rational/generalize.py`](test/rational/generalize.py): re-checks every printed hit on $n = 3 \ldots 6$ (exhaustive) and 64 (random). All 200 printed hits hold; 0 failures.
- [`test/rational/m3.py`](test/rational/m3.py): the winners written as full routines, checked like milestones 1–2.
- [`test/rational/collide.py`](test/rational/collide.py): the impossibility check below.

**Search set.**
- Unary: `not`, `neg`, `shl 1`, `shl N-1`, `shr N-1`, `sar N-1`.
- Binary: `+ - * and or xor andn`.
- Constant ops: `and/or/xor/+/rsub` with $\{1, T, \lnot T, -1\}$.
- The **reduced set**, used for the length-6 runs: unary `not, neg, shl N-1, sar N-1`; constant ops `and T`, `and ~T`, `or 1`, `xor T`; all binaries.
- Every program in this report uses only the reduced set.

**Lanes.** Every case of milestone 1 at that $N$: 780 multiply / 312 add lanes at $N = 3$; 6,464 / 2,596 at $N = 4$. Value equality is exact. A zero result may carry either axis.

**Pruning.** Safe, as in `superopt.c`: duplicate values, unused values, canonical order, ordered commutative operands. So "no hit" is a proof within the set at that $N$.

**What "no hit" covers.** The pruning is safe at that $N$. A 64-bit program carries over to the 3-bit test only if it is correct at 3 bits too. That holds for every program in this report, but it is an assumption, not a theorem.

**Modes.**
- **post**: inputs $n_1, d_1, n_2, d_2$ and the raw products $P$, $Q$ given free (add: $P = n_1 d_2 + n_2 d_1$; mul: $P = n_1 n_2$; both: $Q = d_1 d_2$). The count is the correction only.
- **pre**: inputs $n_1, d_1, n_2, d_2$ only; multiplies are searched and counted (1 each). The count is the whole routine.
- **both** means both output words. The other output may be an input, e.g. $\text{den} = Q$.

### Results

| Search | Layout | Shortest | Hits / searched |
| --- | --- | --- | --- |
| post add, num only | offset | **4** | none ≤ 3 ($N = 4$); 48 hits |
| post add, den only | offset | **2**: $Q \text{ OR } (d_1 \text{ AND } T)$ | none ≤ 1 ($N = 4$) |
| post add, both | offset | **5** (raw 4 → **9**) | none ≤ 4; 30 hits ($N = 3$), one family |
| post add, both, no `*` | offset | **6** (→ 10) | none ≤ 5 ($N = 3$) |
| post mul, num only | offset | 4 | none ≤ 3 ($N = 4$) |
| post mul, den only | offset | 4 | none ≤ 3 ($N = 4$); 16 hits, all conditional choice |
| post mul, both | offset | **≥ 6**; best known 8 (→ 10) | none ≤ 5 (full set, $N = 3$); none ≤ 5 reduced; length 6 stopped (about 3 h) |
| post add, both | negated | **4** (→ 8) | none ≤ 3; 2 hits |
| post mul, both | negated | **4** (→ 6); $\text{den} = Q$ | none ≤ 3 ($N = 3$ and 4); 9 hits |
| pre add, both | offset | **≥ 7**; best known 8 | none ≤ 6 (reduced; full set ≤ 5) |
| pre add, both | negated | **≥ 7**; best known 8 | none ≤ 6 (reduced; full set ≤ 5) |
| pre mul, both | offset | **≥ 7**; best known 10 | none ≤ 6 (reduced; full set ≤ 5) |
| pre mul, both | negated | **6**: optimal | none ≤ 5 (full set); 51 hits at 6, one family |
| **one post fix for add and mul** | both | **impossible**, any length | none ≤ 5; proof below |
| one fix, told the op ($k = 0 / -1$) | negated | 7 known | none ≤ 5 |
| one fix, told the op ($k = -1 / +1$) | negated | **6**: optimal | none ≤ 5 ($N = 3$) |
| one fix, told the op ($k = 0 / -1$) | offset | > 5 | none ≤ 5 |

### The new formulas

**Offset add, post form, 9** (raw 4 + 5):

$$
A = d_1 \text{ AND } T, \qquad \text{den} = Q \text{ OR } A, \qquad \text{num} = P \text{ XOR } \big((n_1 \text{ XOR } n_2) \cdot A\big)
$$

- **The denominator fix is 2, not 3.** When $a = 0$ there is no leak. When $a = 1$ the OR sets the bit whatever $D_1 + D_2$ is.
- $(n_1 \text{ XOR } n_2) \cdot A = T \cdot a \cdot \text{parity}(n_1 + n_2)$ is the numerator's leak, made with one multiply and the same $A$.
- Variants in the family: $n_1 \pm n_2$ in place of XOR; $P \pm$ in place of XOR; $d_2$ in place of $d_1$.
- It is one longer than the strip form of milestone 2 (8). The searches put the whole routine at 7 or more.

**Negated multiply, 6 (optimal).** All 51 hits are the milestone 2 formula, up to form:
- $m = (d_1 \text{ AND } d_2) \text{ sar } (n-1)$;
- or $m = (d_1 \text{ AND NOT } d_1 d_2) \text{ sar } (n-1)$: the top bit of $d_1 \land \lnot(d_1 d_2)$ is $a_1 \land \lnot(a_1 \oplus a_2) = a_1 a_2$, so it reuses the product;
- the negate applied to $n_1$, $n_2$ or the product, by `xor/sub`, `add/xor` or $\cdot (m \text{ OR } 1)$.

**Negated add, post form, 8:** $s = (d_1 \text{ sar } (n-1)) \text{ OR } 1$, then $\text{num} = P s$, $\text{den} = Q s$. It ties the pre-negate form of milestone 2.

### A single shared correction: impossible

A post-correction sees only $(n_1, d_1, n_2, d_2, P, Q)$, and some add lanes and multiply lanes have **identical** inputs but **different** required outputs. So no program of any length fixes both, unless it is told which operation ran. `collide.py` finds such pairs in both layouts for every $n$ checked.

| Layout | Example | Add needs | Mul needs |
| --- | --- | --- | --- |
| offset ($n \ge 4$) | $n_1 = n_2 = 2$, $D_1 = D_2 = 1$, both imaginary: $P = 4$, $Q = 1$ for both | $4i$: num 4, den $1 + T$ | $-4$: num $-4$, den 1 |
| negated ($n \ge 3$) | $n_1 = n_2 = -2$, $D = 1$, both imaginary: $P = 4$, $Q = 1$ for both | $-4i$: den $-1$ | $-4$: den $+1$ |

Collisions: 3 / 21 / 76 / 258 / 676 / 1,839 lanes (offset, $n = 3 \ldots 8$); 3 / 17 / 68 / 230 / 676 / 1,851 (negated).

**What can be shared** is the work before the raw operation, or a fix that is told the operation.

**Told the operation** (negated layout), as a word $k = -1$ for add and $+1$ for multiply, 6 (optimal: none ≤ 5):

$$
s = \big((d_1 \text{ AND } d_2) \text{ sar } (n-1)\big) \text{ OR } 1, \qquad \text{num} = P \cdot s, \qquad \text{den} = Q \cdot (s \text{ AND } k)
$$

- $s = \pm 1$ is $(-1)^a$ for add and $(-1)^{a_1 a_2}$ for multiply. $s \text{ AND } k$ is $s$ for add and $1$ for multiply.
- With $k$ as a mask ($0 / -1$) the shortest known is 7: $\text{den} = Q \cdot ((m \text{ AND NOT } k) \text{ OR } 1)$.
- The routines cost 4 + 6 = 10 (add) and 2 + 6 = 8 (multiply), against the separate 8 and 6. **Sharing costs 2 here.** It pays only where one code path must serve both operations (SIMD lanes of mixed operations, or a table-free dispatcher).

### Conclusions

1. **Negated layout, multiply: 6, proved optimal** within the search set. The offset layout's multiply cannot get below 7 (pre) or 6 + 2 raw (post); the best known is 10.
2. **Add: 8 in either layout.** The offset strip form ($D_i = d_i \text{ AND } \lnot T$; $\text{den} = D_1 D_2 \text{ OR } A$) and the negated pre-negate form both reach it. None ≤ 6 exists in the reduced set; length 7 is open.
3. **The offset add's denominator fix is 2**: $Q \text{ OR } (d_1 \text{ AND } T)$. Milestone 1 used 3.
4. **No single post-correction can serve both add and multiply.** Identical inputs require different outputs. The shared idea must sit before the raw operation (milestone 2's mask $m = (d_1 \text{ AND } d_2) \text{ sar } (n-1)$ negating operand 2), or be told the operation (6 above).
5. **Recommendation, unchanged from milestone 2:** the negated layout. Add/sub 8, multiply 6, divide 11; ARM64 *(hand)* 6 / 4 / about 9.

**Not done.**
- Lengths 7 and up are not searched; length 6 only with the reduced set.
- The offset post-multiply at length 6 was stopped.
- Sub and division were not searched (sub numerator searches at $N = 4$ match add's: 4 offset, 3 negated).

**Reproduce:** `gcc -O3 -march=native -o rs rsuper.c`, then:
- `./rs 3 neg mul both 0 6 60 2` (14 min);
- `./rs 3 off add both 1 5 60` (4 min);
- `./rs 3 neg am both 3 5 60` (8 min).

Then run `python3 -I generalize.py logs/*.txt`, `python3 -I m3.py` and `python3 -I collide.py` (2 min).
