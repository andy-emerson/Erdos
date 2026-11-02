# Spec 13: prior art for the header and packed forms

Nov 2, 2026. Research for the author: the best known integer algorithms for add, subtract, negate, multiply and exact division, in two forms of a number $m \cdot i^d$:

- **Header form:** $d$ in bits 6–7 of a header byte, $m$ a full 64-bit word (two parallel lists). This is Design A.
- **Packed form:** $d$ and a 62-bit $m$ in one 64-bit word, in one of three layouts:
  - top: $w = d \cdot 2^{62} + m$;
  - bottom: $w = 4m + d$;
  - split (added Nov 2 at the author's request): $w = s \cdot 2^{63} + 2m + a$, with $d = 2s + a$.

**Counting.** The notes' convention (`book/logical-arithmetic.md`): one instruction per operator, including `sltu` (a compare written as a value) and every mask made inside the routine (`0 − k` or `sar`, 1 each). Loads and stores are not counted. Under this convention the count is the RV64IM step count of round 2. The x86-64 and ARM64 notes are hand counts and were not assembled; they are marked *(hand)*.

**Checks.** Every formula below was checked exhaustively in Python. The scripts are in [`test/prior-art/`](test/prior-art/): `check.py`, `extra.py` and `rand64.py` for the header, top and bottom forms, and `split.py` for the split layout and the fast paths. The core forms are also in the Appendix. Coverage:
- header form on 8-bit registers (8-bit magnitudes, the full register as in the real design, the header a byte), all 4×256 × 4×256 operand pairs, add and subtract;
- packed form on 6-, 8- and 10-bit words (4-, 6- and 8-bit magnitudes), every pair and every operation;
- 300,000 random 64-bit cases (skewed toward 0, the maximum and near-equal magnitudes).

No failures. Each check is against the Gaussian-integer value. Expressions must set the error flag. Inputs whose true result overflows are skipped (the spec's precondition). A zero may have any direction (spec §3, "a zero keeps whatever direction its operation gives"); two formulas rely on this, and they are marked.

## Sources

- H. S. Warren, *Hacker's Delight*, 2nd ed., Addison-Wesley 2013 ([publisher](https://www.informit.com/store/hackers-delight-9780321842688)). Used: §2-2 (`a − b = NOT(NOT a + b)`), §2-4 (abs), §2-8 (sign), §2-12 (comparison predicates), §2-19 (doz, max, min), §2-20 (exchange, conditional swap), §10-16 (exact division by constants), §17-3 (comparing sign-magnitude floats as integers). *Section numbers from memory of the table of contents; not checked against the book.*
- H. Massalin, "Superoptimizer: a look at the smallest program", ASPLOS II, 1987 ([pdf](https://www.cse.iitm.ac.in/~krishna/courses/2022/odd-cs6013/MassalinCANews87.pdf)). 68020 results: signum in 4 (`add; subx; negx; addx`), abs in 5, unsigned min and max in 4 each (`sub; subx; and/or; add/addx`), both in 6. The carry idiom: `subx r,r` turns a borrow into a mask.
- T. Granlund, R. Kenner, "Eliminating branches using a superoptimizer and the GNU C compiler", PLDI 1992 ([pdf](https://gmplib.org/~tege/gso-pldi92.pdf); [GNU superopt](https://www.gnu.org/s/superopt)). RS/6000: every comparison predicate in ≤ 3 instructions using the carry; `a + (b ≤u c)` in 2; unsigned min and max in 4 without `doz`; select written as `c AND −(a ≥ b)`.
- R. Schkufza, R. Sharma, A. Aiken, "Stochastic superoptimization", ASPLOS 2013 ([arXiv](https://arxiv.org/abs/1211.0557)). STOKE, for x86-64 loop-free code.
- R. Sasnauskas et al., "Souper: a synthesizing superoptimizer", 2017 ([arXiv](https://arxiv.org/abs/1711.04422)). An LLVM IR synthesizer, the practical tool for the searches in "What to superoptimize next".
- S. Anderson, *Bit Twiddling Hacks* ([web](https://graphics.stanford.edu/~seander/bithacks.html)): conditional negate `(x XOR −f) + f`, min/max by mask, sign extension. All are also in Warren. *Not re-fetched.*
- T. Granlund, P. Montgomery, "Division by invariant integers using multiplication", PLDI 1994 ([pdf](https://gmplib.org/~tege/divcnst-pldi94.pdf)). §9: exact division by the inverse mod $2^N$. *Section number from memory.*
- T. Jebelean, "An algorithm for exact division", J. Symbolic Computation 15 (1993) ([record](https://portal.mardi4nfdi.de/wiki/Item:Q1260759)). Multiprecision exact division, low-order digits first, by the inverse mod $\beta$. One word is the trivial case.
- J. Hurchalla, "An improved integer modular multiplicative inverse (modulo $2^w$)", 2022 ([arXiv](https://arxiv.org/abs/2204.04342)). Builds on Dumas 2012 and Montgomery's start $x_0 = 3a \text{ XOR } 2$. The 64-bit inverse in 8 multiplies, about 19 cycles in the paper's model, against about 30 for Newton.
- P.-M. Seidel, G. Even, "On the design of fast IEEE floating-point adders", ARITH 2001 ([record](https://cris.tau.ac.il/en/publications/on-the-design-of-fast-ieee-floating-point-adders/)): the two-path adder (swap, effective subtraction, conditional complement). The ideas used below are textbook FP-adder material. *Not read in full.*
- Wikipedia, "Signed number representations" ([web](https://en.wikipedia.org/wiki/Signed_number_representations)): IBM 704/709/7090 sign-magnitude; PDP-1, CDC 6000 and UNIVAC 1100 ones' complement with end-around carry.
- Searches for recent work on branch-free software sign-magnitude or tagged-integer addition found only hardware (adder patents, the ROHD-HCL `SignMagnitudeAdder`) and the 6502 notes of [bbcelite](https://www.bbcelite.com/deep_dives/adding_sign-magnitude_numbers.html), which branch. I found no software prior art for the add below.

## Two facts that shorten every add

Both are derived here from the FP adder's rules (see "Ideas from floating point"). Both are checked exhaustively.

1. **The result takes the direction of the operand with the larger magnitude**, in all three of the tree's cases: same direction (either will do), opposite directions (the borrow rule), and different axes with a zero (the nonzero one's direction; with two zeros, either). So one select on $L = 0 - (m_1 <_u m_2)$ replaces both the borrow's direction rule and the cross-axis zero rule. In the header form, $h_d = h_1 \text{ XOR } (t \text{ AND } L)$ with $t = h_1 \text{ XOR } h_2$.
2. **Only the sign bits decide add or subtract.** On different axes with a zero operand, $m_1 + m_2 = |m_1 - m_2|$. So the mask $s$ can be "sign bits differ", $s = $ bit 7 of $t$ spread to a mask, which ignores the axis. The current test $o = (t = \texttt{0x80})$, which checks the axis too, is unnecessary. When neither operand is zero on different axes, the result is an expression and is flagged anyway.

## Header form

### Add (one routine for integers and complex numbers)

Headers loaded **sign-extended** (RISC-V `lb`, ARM64 `ldrsb`, x86-64 `movsx`; the same cost as the zero-extending load), so bit 7 fills bits 8–63:

| Step | Formula | Count |
| --- | --- | --- |
| relation | $t = h_1 \text{ XOR } h_2$ | 1 |
| signs differ | $s = t \text{ sar } 63$ | 1 |
| $m_1 \pm m_2$ | $r = m_1 + (m_2 \text{ XOR } s) - s$ | 3 |
| larger | $L = 0 - (m_1 <_u m_2)$ | 2 |
| borrow mask | $b = s \text{ AND } L$ | 1 |
| unwrap | $r = (r \text{ XOR } b) - b$ | 2 |
| direction | $h_d = h_1 \text{ XOR } (t \text{ AND } L)$, stored as a byte | 2 |
| **integers** | | **12** |
| expression flag (complex only) | $e = (t \text{ shr } 6) \text{ AND } (m_1 \ne 0) \text{ AND } (m_2 \ne 0)$ | 5 (+1 if sticky) |
| **complex** | | **17** |

- With zero-extending loads (`lbu`), $s = 0 - (t \text{ shr } 7)$ costs 2: 13 and 18.
- A variant with the same count: $r = (m_1 \text{ XOR } b) + (m_2 \text{ XOR } q) - s$ with $q = s \text{ AND NOT } L$ (checked).
- **Against the current row.**
  - Integers: 12 against 15. Round 2's program is 17 compute steps, of which 2 are `andi 192` steps now dropped.
  - Complex: 17 against 27, both computing the flag fresh. Round 2's program is 29, of which 2 are `andi` steps; `aAddBF` in `designa.nim` is about 27 by operator count.
  - The saving comes from the two facts above: no $o$, no $a_x$-based selection of the direction, and no `b shl 7` add.
- **x86-64** *(hand)*: $L$ is `cmp m1,m2; sbb L,L` (2, the Massalin/GSO carry idiom) instead of `cmp; setb; movzx; neg` (4). Add about 3 `mov`s for two-operand forms. Roughly 13–15.
- **ARM64** *(hand)*: a shifted second operand (`t, asr #63`) folds $s$ into the `eor`, `add`, `sub` and `and` that use it, so $s$ costs 0. $L$ is `cmp; csetm` (2). Integers 11.
- **With selects** (not in RV64I; ARM64 `csel/cneg`, x86-64 `cmov`, RISC-V Zbb `maxu/minu` or Zicond): take $W = \max(m_1, m_2)$ and $V = \min(m_1, m_2)$; then $r = W + (V \text{ XOR } s) - s$ and $h_d = (m_1 <_u m_2)\ ?\ h_2 : h_1$ (checked).
  - ARM64 *(hand)*: `eor; cmp; csel; csel; csel; tst; cneg; add` = 8 for integers.
  - RV64 with Zbb: 11.
- **Branching.** The notes' tree stays fastest when signs and axes are predictable (round 3: 0.65 ns against 2.04). Neither the literature nor I found a better tree. A mixed form that branches only on the expression flag (rare) and runs the 12-step body otherwise keeps the cross-axis path out of the common case.

### Subtract
Add with $h_2 \text{ XOR } \texttt{0x80}$, or with $h_2 \text{ XOR } {-128}$ when headers are sign-extended (this keeps bits 8–63 equal to bit 7). Add + 1: 13 integers, 18 complex. The same as now, relative to add.

### Negate
$(h \text{ XOR } \texttt{0x80},\ m)$: 1. This is minimal, and it is the current rule.

### Multiply
$(h_1 + h_2,\ m_1 m_2)$, byte arithmetic: 2. This is minimal (each output needs one operation) and current. The two-field rule ($a = a_1 \text{ XOR } a_2$, $s = s_1 \text{ XOR } s_2 \text{ XOR } (a_1 \text{ AND } a_2)$) costs more than one byte add.

### Exact division
$(h_1 - h_2,\ m_1 /_u m_2)$: 2 (x86-64 3: `xor edx,edx; div`). This is current.

**Prior art: division by the inverse.** For odd $a$, $q = n \cdot a^{-1} \bmod 2^{64}$ whenever $a \mid n$ (Jebelean; Granlund–Montgomery §9; Warren §10-16). For a general divisor, with $k = \text{ctz}(m_2)$:

$$q = (m_1 \text{ shr } k) \cdot \text{inv}(m_2 \text{ shr } k) \bmod 2^{64}$$

The inverse, Hurchalla's form (checked on 200,000 random odd $a$):

$$x_0 = 3a \text{ XOR } 2 \ (\text{5 bits}),\quad y = 1 - a x_0,\quad \text{then 4 times: } x \leftarrow x(1 + y),\ y \leftarrow y^2$$

This takes 8 multiplies. Exact division through the inverse was checked exhaustively on 8 bits.

**Verdict:** one division costs about 19 operations, with a multiply chain of about 20 cycles in the paper's model. That is no better than `divu`, which round 3 timed at 2.62 ns. When one divisor serves many dividends (a list divided by one number), the setup runs once and each division is `shr; mul`, 2 operations, about 4 cycles. *Not timed.*

## Packed form

### What each placement gives

| | Top: $w = d \cdot 2^{62} + m$ | Bottom: $w = 4m + d$ |
| --- | --- | --- |
| Sign mask | $w \text{ sar } 63$, 1 | $(w \text{ shl } 62) \text{ sar } 63$, 2 (ARM64 `sbfx`: 1) |
| Magnitude compare | needs both magnitudes stripped: $L = ((w_1 \text{ AND } M) - (w_2 \text{ AND } M)) \text{ sar } 63$, 4 | $w_1 <_u (w_2 \text{ AND NOT } 3)$, 2 (also $(w_1 \text{ OR } 3) <_u w_2$) |
| Zero test | $w \text{ AND } M$, or `shl 2`, then test | $w <_u 4$, 1 |
| Direction arithmetic | free mod 4: carries leave the word | must be masked with `AND 3`; carries go into $m$ |
| Magnitude ± into a word that keeps its $d$ | yes, while there is no overflow | yes, adding $4m$ never touches $d$ |
| Negate | $w \text{ XOR } 2^{63}$ (x86 `btc` or a constant in a register; RISC-V a hoisted constant or Zbs `binvi`; ARM64 immediate) | $w \text{ XOR } 2$, a small immediate everywhere |
| Multiply / divide | strip both ($M$ is not an x86 immediate; BMI2 `bzhi` does it in 1) | shift one operand, mask the other; $4m$ lands in place |
| Overflow of $m$ | runs into $d$ and corrupts it | leaves the word at the top, as for plain integers |

$M = 2^{62} - 1$. On ARM64, `sbfx`/`ubfx` take any bit as a mask or flag in 1, so placement matters mostly on x86-64 and RISC-V.

### Add (both placements, 13 for integers; complex 16)

The FP adder's operand swap: select the whole word of the operand with the larger magnitude, $W$, which carries the result's direction, and add or subtract the smaller magnitude into it. $W$'s magnitude is the larger, so the subtraction never borrows into $d$, and under no overflow the addition never carries into it. No conditional negation is needed at the end.

| Step | Top | Bottom | Count |
| --- | --- | --- | --- |
| relation | $t = w_1 \text{ XOR } w_2$ | same | 1 |
| signs differ | $s = t \text{ sar } 63$ | $s = (t \text{ shl } 62) \text{ sar } 63$ | 1 / 2 |
| larger | $L = ((w_1 \text{ AND } M) - (w_2 \text{ AND } M)) \text{ sar } 63$ | $L = 0 - (w_1 <_u (w_2 \text{ AND NOT } 3))$ | 4 / 3 |
| larger word | $W = w_1 \text{ XOR } (t \text{ AND } L)$ | same | 2 |
| smaller magnitude | $v = (W \text{ XOR } t) \text{ AND } M$ | $v = (W \text{ XOR } t) \text{ AND NOT } 3$ | 2 |
| result | $r = W + (v \text{ XOR } s) - s$ | same | 3 |
| **integers** | | | **13 / 13** |
| expression flag | $e = (t \text{ shr } 62) \text{ AND } (v \ne 0)$ | $e = t \text{ AND } 1 \text{ AND } (v \ne 0)$ | 3 |

- For the top placement, $L = (((w_1 - w_2) \text{ XOR } t) \text{ shl } 1) \text{ sar } 63$ also works: bit 62 of $w_1 - w_2$, XOR the axis bits, is the borrow out of the magnitudes (Warren §2-12/2-13 style). It is also 4 steps (checked).
- **Against the header form:** 13 against 12 compute, but 2 loads and 1 store against 4 and 2. With memory: 16 against 18 for integers, 19 against 23 for complex.
- There is no current packed sign-magnitude routine to compare with. Design C, the existing one-word design, has no branch-free add.
- **ARM64** *(hand)*:
  - top: `eor; lsl; cmp x, w2, lsl #2; csetm; and; eor; eor; and; eor (asr #63); add; sub (asr #63)` = 11; with `csel` for $W$ and $V$, 9;
  - bottom with `sbfx` and `csel`: 10.
- **x86-64** *(hand)*: $L$ is `and; cmp; sbb` (bottom, 3); `sarx` (BMI2) avoids the `mov` for $s$.
- **Rejected:** convert to two's complement ($v = (m \text{ XOR } s) - s$), add, then convert back with abs and sign. That is about 14 or more, and it needs a 63-bit value.

### Subtract
$w_2 \text{ XOR } 2^{63}$ (top) or $w_2 \text{ XOR } 2$ (bottom), then add: 14.

### Negate
The same XOR: 1 for both placements.

### Multiply
- **Bottom, 6:** $r = \big((w_1 \text{ shr } 2) \cdot (w_2 \text{ AND NOT } 3)\big) \text{ OR } \big((w_1 + w_2) \text{ AND } 3\big)$. On ARM64, `madd` adds the direction in the multiply: 5 *(hand)*.
- **Top, 7:** $a = w_1 \text{ AND } M$, $b = w_2 \text{ AND } M$, $r = ab \text{ OR } \big((w_1 \text{ XOR } a) + (w_2 \text{ XOR } b)\big)$.
  - Alternative, also 7: $r = ab \text{ OR } \big((w_1 + w_2 - 1) \text{ AND NOT } M\big)$, which is 6 on x86-64 with one `lea` for $w_1 + w_2 - 1$ *(hand)*.
  - The $-1$ is needed. Without it, $m_1 + m_2 = 2^{62}$ (only for $m_1 = 1$, $m_2 = 2^{62} - 1$, a nonzero product) carries into $d$. With it, the direction can be wrong only when a factor is 0, and then the product is zero. **Relies on any direction for zero.**
- **Against Design C's multiply:** 11 compute.

### Exact division
- **Bottom, 6:** $r = \big((w_1 \text{ AND NOT } 3) /_u (w_2 \text{ shr } 2)\big) \text{ OR } \big((w_1 - w_2) \text{ AND } 3\big)$. Here $4m_1 / m_2 = 4q$ exactly, so the quotient lands in place. Unsigned, so there is no trap.
- **Top, 6:** $r = \big((w_1 \text{ AND } M) /_u (w_2 \text{ AND } M)\big) \text{ OR } \big((w_1 - w_2) \text{ AND NOT } M\big)$.
  - $m_2 \mid m_1$ with $m_1 \ne 0$ gives $m_1 \ge m_2$, so nothing borrows into $d$. $m_1 = 0$ gives a zero.
  - **Relies on any direction for zero.**
- **Against Design C's division:** 11 compute.

## Packed, split layout

### The layout

$w = s \cdot 2^{63} + 2m + a$:
- sign $s$ in bit 63;
- axis $a$ in bit 0;
- magnitude $0 \le m < 2^{62}$ in bits 1–62.

The direction is $d = 2s + a$ and the value is $m \cdot i^d$. The constants below are 64-bit:

| Constant | Value | Bits |
| --- | --- | --- |
| $S$ | $2^{63}$ = `0x8000_0000_0000_0000` | the sign |
| $F$ | $2^{63} - 2$ = `0x7FFF_FFFF_FFFF_FFFE` | the magnitude field, bits 1–62; $w \text{ AND } F = 2m$ |
| $K$ | $2^{63} + 1$ = `0x8000_0000_0000_0001` | sign and axis |

- On ARM64 all three are bitmask immediates.
- On x86-64 and RISC-V each is a constant in a register, loaded once outside the loop and not counted. RISC-V builds $S$ as `li 1; slli 63`; Zbs `binvi`/`bclri` reach bit 63 directly.

**Checks:** [`test/prior-art/split.py`](test/prior-art/split.py), exhaustive on 8- and 10-bit words (6- and 8-bit magnitudes; 65,536 and 1,048,576 operand pairs, add and subtract each), plus 300,000 random 64-bit cases. No failures. A mutation run (multiply and divide without the axis carry or borrow; add without the select) fails as it should.

### What the layout gives

| Property | Cost | Notes |
| --- | --- | --- |
| Sign mask | $w \text{ sar } 63$, 1 | as top |
| Axis bit | $w \text{ AND } 1$, 1 | as bottom |
| Negate | $w \text{ XOR } S$, 1 | x86-64 `btc w, 63`, 1; ARM64 `eor` with an immediate, 1; RISC-V `xor` with the constant register, or Zbs `binvi w, w, 63` |
| Zero test | $(w \text{ shl } 1) <_u 4$, 2 | |
| Direction arithmetic mod 4 | not free | the carry from $a$ (bit 0) must reach $s$ (bit 63) across the magnitude; this is what costs in multiply and divide |
| Magnitude overflow | into the sign bit | see below |

Axis on top with sign at the bottom (the author's reverse) would make `sar` give the axis mask, which the add does not use.

### Add (13; complex 16)

$$
\begin{aligned}
t &= w_1 \text{ XOR } w_2 && 1 \\
s &= t \text{ sar } 63 && 1 \quad \text{(signs differ)} \\
L &= 0 - \big((w_1 \text{ shl } 1) <_u (w_2 \text{ shl } 1)\big) && 4 \\
W &= w_1 \text{ XOR } (t \text{ AND } L) && 2 \quad \text{(the word with the larger magnitude)} \\
v &= (W \text{ XOR } t) \text{ AND } F && 2 \quad (= 2 \cdot \text{the smaller magnitude}) \\
r &= W + (v \text{ XOR } s) - s && 3 \\
e &= t \text{ AND } 1 \text{ AND } (v \ne 0) && 3 \quad \text{(expression flag, complex only)}
\end{aligned}
$$

- **The compare.** `shl 1` drops the sign. $(w \text{ shl } 1) = 4m + 2a$, so $L$ is all ones when $m_1 < m_2$, and also on the tie $m_1 = m_2$, $a_1 < a_2$. That tie happens only on different axes, where it is either an expression (flagged) or two zeros (any direction). Both outcomes are correct; checked.
- **Alternative compare, also 4 (checked):** $L = 0 - \big((w_1 \text{ XOR } (t \text{ AND } S)) <_u w_2\big)$. It gives $w_1$ the sign of $w_2$, then compares.
- **Why no correction is needed.** $W \pm v$ adds or subtracts an even number, so bit 0 (the axis) is kept. It never borrows, since $W$'s magnitude is the larger. It carries into bit 63 only on overflow.
- **x86-64** *(hand)*: $L$ is `lea x,[w1+w1]; lea y,[w2+w2]; cmp x,y; sbb L,L`, which is 4 with no `mov`. $s$ is `sarx` (BMI2, 63 in a register) or `mov; sar`.
- **ARM64** *(hand)*: `eor t; lsl x,w1,#1; cmp x,w2,lsl #1; csetm L,lo; and; eor W; eor; and v,#F; eor y,v,t,asr #63; add; sub r,r,t,asr #63` = 11; with `csel` for $W$ and the other word, 9. This is the same as top.

### Subtract (14; complex 17)
Add with $w_2 \text{ XOR } S$.

### Negate (1)
$w \text{ XOR } S$.

### Multiply (9)

$$
\begin{aligned}
x_1 &= w_1 \text{ AND } F, \quad x_2 = w_2 \text{ AND } F && 2 \quad (2m_1,\ 2m_2) \\
p &= (x_1 \cdot x_2) \text{ shr } 1 && 2 \quad (2 m_1 m_2;\ 4 m_1 m_2 < 2^{64} \text{ by no overflow}) \\
D &= \big((w_1 \text{ OR } F) + (w_2 \text{ XOR } x_2)\big) \text{ AND } K && 4 \\
r &= p \text{ OR } D && 1
\end{aligned}
$$

- **How $D$ works.** $w_1 \text{ OR } F$ fills bits 1–62 with ones and $w_2 \text{ XOR } x_2 = w_2 \text{ AND } K$ empties them. So $a_1 + a_2 = 2$ carries through the ones into bit 63: $D$ is $d_1 + d_2 \bmod 4$ in bits 63 and 0.
- **Alternative, 10 (checked):** $D = \big(t \text{ XOR } ((w_1 \text{ AND } w_2) \text{ shl } 63)\big) \text{ AND } K$, from $a = a_1 \text{ XOR } a_2$ and $s = s_1 \text{ XOR } s_2 \text{ XOR } (a_1 \text{ AND } a_2)$ (the two-field rule of the notes).
- **ARM64** *(hand)*: `orr r, D, p0, lsr #1` folds the shift: 8.
- **Against the other layouts:** top 7, bottom 6. **Split is the worst layout for multiply.**

### Exact division (9)

$$
\begin{aligned}
x_1 &= w_1 \text{ AND } F, \quad x_2 = w_2 \text{ AND } F && 2 \\
q &= (x_1 /_u x_2) \text{ shl } 1 && 2 \quad (2m_1 / 2m_2 = m_1/m_2, \text{ doubled}) \\
D &= \big((w_1 \text{ XOR } x_1) - (w_2 \text{ XOR } x_2)\big) \text{ AND } K && 4 \\
r &= q \text{ OR } D && 1
\end{aligned}
$$

- **How $D$ works.** Bits 1–62 of both operands are 0, so $a_1 < a_2$ borrows through them into bit 63: $D$ is $d_1 - d_2 \bmod 4$. No division by zero: $x_2 = 2m_2 \ne 0$. The divide is unsigned, so there is no trap.
- **Alternatives (checked):**
  - $q = x_1 /_u (x_2 \text{ shr } 1)$, which is $2m_1/m_2 = 2q$, same count.
  - $D = \big(t \text{ XOR } ((w_2 \text{ AND NOT } w_1) \text{ shl } 63)\big) \text{ AND } K$: 10, or 9 with x86 `andn` or ARM64 `bic`.
- **x86-64 and ARM64** *(hand)*: `shl 1; or` is one `lea r,[D+q*2]` (x86-64) or `orr r,D,q,lsl #1` (ARM64), since the fields are disjoint: 8. x86-64 adds `xor edx,edx` for `div`.
- **Against the other layouts:** top and bottom 6.

### Overflow
- **What happens.** A same-sign add whose magnitudes sum past $2^{62} - 1$ carries out of bit 62 into bit 63. That flips the sign and leaves a plausible wrong number: the same axis, the opposite sign, and the magnitude mod $2^{62}$.
- **Detection: $o = (r \text{ XOR } W) \text{ sar } 63$**, 2. It is all ones exactly on overflow, the same test as signed overflow. Checked: no false alarms, and every overflowing input is caught.
- Opposite signs cannot overflow.
- In multiply, $x_1 x_2 \ge 2^{64}$ is lost silently, as in every layout.

### Branching: a same-direction fast path (all three packed layouts; checked)
- **Test:** the direction bits of $t = w_1 \text{ XOR } w_2$ are 0:
  - top: $t \text{ shr } 62$;
  - bottom: $t \text{ AND } 3$;
  - split: $t \text{ AND } K$.
- **Then:** $r = w_1 + (w_2 \text{ AND field})$, where the field is $2^{62} - 1$ (top), NOT 3 (bottom) or $F$ (split).
- **Cost:** 4 and one branch on the common path; otherwise run the 13-step body. This is worth it only when directions are predictable (round 3's lesson).
- **Overflow on this path (checked):**
  - top: $(r \text{ XOR } w_1) \text{ shr } 62 \ne 0$;
  - bottom: $r <_u w_1$;
  - split: $(r \text{ XOR } w_1) \text{ sar } 63$.

## Comparison: header form and the three packed layouts

Counts are under the notes' convention (RV64 steps, constants hoisted), compute only. Memory operations are listed separately.

| | Header (A) | Packed top | Packed bottom | Packed split |
| --- | --- | --- | --- | --- |
| Bytes per value | 9 | 8 | 8 | 8 |
| Largest magnitude | $2^{64} - 1$ | $2^{62} - 1$ | $2^{62} - 1$ | $2^{62} - 1$ |
| Loads + stores per binary operation | 4 + 2 | 2 + 1 | 2 + 1 | 2 + 1 |
| Add, integers | 12 (13 with `lbu`) | 13 | 13 | 13 |
| Add, complex (with fresh flag $e$) | 17 (18) | 16 | 16 | 16 |
| Add, integers, with memory | 18 | 16 | 16 | 16 |
| Add, complex, with memory | 23 | 19 | 19 | 19 |
| Subtract | add + 1 | add + 1 | add + 1 | add + 1 |
| Negate | 1 (`xori 0x80`; plus copying $m$) | 1 (constant $2^{63}$) | 1 (`xori 2`) | 1 (constant $2^{63}$; x86 `btc`) |
| Multiply | 2 | 7 (x86-64 `lea`: 6) | 6 (ARM64 `madd`: 5) | 9 (ARM64: 8) |
| Exact division | 2 | 6 | 6 | 9 (x86-64/ARM64: 8) |
| Sign mask | 1 (sign-extended load) | 1 | 2 (ARM64 `sbfx`: 1) | 1 |
| Axis bit | $(t \text{ shr } 6)$, AND 1 | $(t \text{ shr } 62)$, AND 1 | AND 1 | AND 1 |
| Magnitude compare, as a mask | 2 | 4 | 3 | 4 (ARM64 3) |
| Zero test | $m = 0$, 1 | 2 | 1 ($w <_u 4$) | 2 |
| Direction mod 4 | byte add, free | free off the top | AND 3 | carry or borrow across the field |
| Magnitude overflow goes | out of the word (lost) | into the axis bit | out of the word (lost) | into the sign bit |
| Overflow test, same-sign add | $r <_u m_1$, 1 (*not checked*) | $(r \text{ XOR } W) \text{ shr } 62 \ne 0$, 2 | $r <_u W$, 1 | $(r \text{ XOR } W) \text{ sar } 63$, 2 |
| Same-direction fast path | 2 and one branch (the tree) | 4 and one branch | 4 and one branch | 4 and one branch |

**In short:**
- All three packed layouts tie on add and subtract (13), and all beat the header form once memory is counted.
- Bottom is cheapest for multiply, divide and the zero test, and its overflow leaves the direction intact.
- Split matches top on add (sign by `sar`), gets the axis as cheaply as bottom, and its overflow test is the familiar signed one. But it pays 3 more on multiply and divide, because the carry or borrow from $a$ to $s$ must cross the magnitude.
- The header form keeps the full 64-bit magnitude and 2-step multiply and divide, at the price of a second list.

## Ideas from floating point (integer only)

- **Effective subtraction** = sign XOR sign (XOR the operation). This is $s$ above. As the FP adder does, it ignores everything but the sign bits.
- **Operand swap by magnitude compare.** FP adders swap so that the larger operand comes first. The result's sign is then the larger operand's, and no end correction is needed. This gives fact 1 and the packed add, where $W$ carries the direction.
- **Conditional complement of the result** (FP close path, when the compare is skipped). This is the header form's $(r \text{ XOR } b) - b$. In the header form, the conditional negation (3 steps given $L$) beats the swap (4 steps), because $W$ carries no direction there.
- **Ones' complement with end-around carry.** $e = m_1 + \text{NOT } m_2$ carries out exactly when $m_1 > m_2$. The result is $e + 1$ with a carry, else $\text{NOT } e$. Hardware uses this to avoid both a compare and a +1 in the critical path. In software it is the same count as `sub` with its borrow and a conditional negate (checked by hand). x86 `sub; sbb` and ARM64 `subs; cneg` are its software form. **No gain.**
- **Sign-magnitude to an integer order key** (Warren §17-3): $k = w \text{ XOR } ((w \text{ sar } 63) \text{ shr } 1)$ orders packed-top integers as signed words, except that $\pm 0$ differ by one. Useful for a compare on one axis, not for add. *Not checked.*
- **Historical machines.** The IBM 704/7090 (sign-magnitude) added unlike signs by complementing one operand and recomplementing the result when there was no end carry. The CDC 6000 and UNIVAC 1100 (ones' complement) used end-around carry; the CDC 6600 is said to have used a subtractive adder to avoid producing −0. *From memory; not checked against the manuals.* All of these are the same three hardware ideas, and they add nothing beyond the above.

## What to superoptimize next (≤ 7 instructions each)

The operation set is RV64I plus `sltu`. A second run should add carry and select (x86 `sbb/adc/cmov`, ARM64 `csel/csetm/cneg/sbfx`, RISC-V Zbb/Zicond). Souper or a brute-force enumerator should manage the shorter ones.

1. **Header unwrap:** given $m_1, m_2, s$, produce $(r, L)$: $r = m_1 \pm m_2$, unwrapped. Currently 8 (`xor, add, sub, sltu, neg, and, xor, sub`). Is there a 7?
2. **Header direction:** given $h_1$, $t$ and $L$, the byte $h_d$. Currently 2. Can it share work with $s$ or $b$?
3. **Packed top, larger mask:** $L$ from $w_1, w_2$. Currently 4. Is there a 3?
4. **Packed bottom, larger mask:** $L$ from $w_1, w_2$. Currently 3. Is there a 2 with carry or select?
5. **Packed:** $(W, v)$ from $w_1, w_2, t, L$. Currently 4.
6. **Packed:** $r = W \pm v$ by $s$. Currently 3 ($W + (v \text{ XOR } s) - s$). Is it 2 with `adc/sbb`?
7. **Expression flag:** $(t \text{ shr } 62) \text{ AND } (v \ne 0)$. Currently 3.
8. **Packed top multiply direction:** 3 (or 2 with `lea`).
9. **The whole packed add** at 13: a stochastic search (STOKE) is more likely to find a shorter whole than an exhaustive one.
10. **Split, direction of a product:** $d_1 + d_2$ into bits 63 and 0 from $w_1, w_2$ (and $x_2$). Currently 4: `or, xor, add, and`.
11. **Split, direction of a quotient:** $d_1 - d_2$. Currently 4 given $x_1$ and $x_2$.
12. **Split, larger mask:** $L$. Currently 4; ARM64 does it in 3 with a shifted `cmp`.

## Open questions

- Is a sign-extending header load acceptable? It is the same cost, and it changes what $h$ holds in the register, not the stored byte.
- Should the expression flag be fresh or sticky (+1)?
- The packed formulas for multiply (top, `−1` form) and division (top) depend on "a zero has any direction". Keep that rule, or pay 1 more step for a clean direction?
- Selects (`csel`, `cmov`, Zicond) give the shortest code (8 on ARM64), but round 3's rule excludes them (RV64I lacks them). Should there be a select row?
- The packed form's top and bottom placements tie at 13 for add. Bottom wins multiply on ARM64 and has a cheap negate immediate and a cheap zero test. Top's magnitude overflow corrupts $d$, which matters once overflow (the second problem) is in scope.
- The division-by-inverse row pays only for a reused divisor. Is that a case the spec wants?
- **Split layout:** is a 3-step loss on multiply and divide worth the signed-style overflow test and the cheap sign and axis? It depends on the operation mix.
- **Split layout, the add's compare:** `shl 1` ties on equal magnitudes across axes. This is harmless (expression or zero), but the Lean round 1 check should state it as such, not assume a strict $m_1 < m_2$.
- None of this is timed. The x86-64 and ARM64 counts are by hand, not from an assembler.

## Appendix: the checker

The checks ran at register widths 6, 8 and 10 (packed) and 8 (header), plus a random 64-bit run. The full scripts are in `test/prior-art/`; the split layout's formulas are in `split.py` (class `Word`), with step counts in comments. The core forms of the other layouts, as checked:

```python
# R-bit registers, F = 2**R - 1, sar = arithmetic shift right, sx8 = sign-extend a byte
def h_add(h1, m1, h2, m2):                 # header, sign-extended headers; R = 8
    t = sx8(h1) ^ sx8(h2); s = sar(t, R-1)
    r = (m1 + (m2 ^ s) - s) & F
    L = -(m1 < m2) & F; b = s & L; r = ((r ^ b) - b) & F
    return (h1 ^ (t & L)) & 0xFF, r, (t >> 6) & (m1 != 0) & (m2 != 0) & 1
def p_add_top(w1, w2):                     # n = R-2, M = 2**n - 1
    t = w1 ^ w2; s = sar(t, R-1); L = sar(((w1 & M) - (w2 & M)) & F, R-1)
    W = w1 ^ (t & L); v = (W ^ t) & M
    return (W + (v ^ s) - s) & F, (t >> n) & (v != 0) & 1
def p_add_bot(w1, w2):
    t = w1 ^ w2; s = sar((t << (R-2)) & F, R-1); L = -(w1 < (w2 & ~3 & F)) & F
    W = w1 ^ (t & L); v = (W ^ t) & ~3 & F
    return (W + (v ^ s) - s) & F, t & 1 & (v != 0)
p_mul_bot = lambda w1, w2: (((w1 >> 2) * (w2 & ~3 & F)) & F) | ((w1 + w2) & 3)
p_mul_top = lambda w1, w2: ((w1 & M) * (w2 & M) & F) | ((w1 + w2 - 1) & F & ~M)
p_div_bot = lambda w1, w2: ((w1 & ~3 & F) // (w2 >> 2)) | ((w1 - w2) & 3)
p_div_top = lambda w1, w2: ((w1 & M) // (w2 & M)) | ((w1 - w2) & F & ~M)
```

Each result was decoded and compared with $v_1 \pm v_2$, $v_1 v_2$ or $v_1 / v_2$ as Gaussian integers, for every pair of operands, in both add and subtract (subtract by flipping $y$'s sign bit first).
