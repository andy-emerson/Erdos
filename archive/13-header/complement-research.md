# Spec 13: the complement layout

Nov 2, 2026. Research for the author: the cheapest correct algorithms for the complement layout of a complex integer on an axis, mainly add and subtract, plus a superoptimizer search.

## The layout

An $n$-bit word ($n = 64$; examples $n = 4$):

| Bits | Field | Name below |
| --- | --- | --- |
| $n-1$ | sign start $s$ (0, or $-2^{n-2}$) | $S = 2^{n-1}$ |
| $n-2$ | axis $a$ (0 real, 1 imaginary) | $G = 2^{n-2}$ |
| $0 \ldots n-3$ | count $c$ | $C = G - 1$ |

Value $(c - 2^{n-2} s) \cdot i^a$. $K = S + G$. On each axis the pair $(s, c)$ is an $(n-1)$-bit two's-complement integer, with the axis bit inserted between its sign bit and the rest.

**Conventions** (`book/logical-arithmetic.md`): one instruction per operator; constants ($G$, $\lnot G$, $S$, $K$) are made once outside the routine and not counted. On ARM64 all of them are bitmask immediates. On x86-64 and RISC-V they sit in registers. The x86-64 and ARM64 notes are hand counts and were not assembled; they are marked *(hand)*.

**Checks.** Every formula below is in [`test/complement/check.py`](test/complement/check.py). It is checked exhaustively on $n = 4, 6, 8$ (every word, and every pair) and on 219,320 random 64-bit pairs (skewed toward 0, the extremes and long carries; plus 20,000 divisible pairs). The script reports no failures. The overflow tests and flags are checked for exactness: no false alarms and no misses. The superoptimizer is [`test/complement/superopt.c`](test/complement/superopt.c).

## Why plain add fails, and what any fix must do

$s = w_1 + w_2$ on the same axis puts $a + a + \gamma$ in the axis column, where $\gamma$ is the carry out of the count. So:

- bit $n-2$ of $s$ is $\gamma$, not $a$;
- the carry into bit $n-1$ is $a$, not $\gamma$.

Both are wrong exactly when $\delta = a \text{ XOR } \gamma = 1$. Then $s$ is off in bits $n-1$ and $n-2$ together: $r = s \text{ XOR } (\delta \cdot K)$.

**One add can never be enough.** For the carry into bit $n-1$ to equal $\gamma$, the two addends' axis bits must differ (one 0, one 1). Then that column's output is $\text{NOT } \gamma$, which is not $a$. So a single add with fixed inputs cannot be right on all inputs. A correct sequence needs one of two things:
- a step after the add that writes $a$ back into bit $n-2$: the bridge, a carry-skip over one bit;
- or a step before it that learns $\gamma$, so that the final add's axis column can be set up per input: the re-add.

**Rotations cannot help.** Cyclically the bits run $c_0, \ldots, c_{n-3}, G, S$ and back to $c_0$. A rotation keeps that cyclic order. So $G$ always either sits inside the carry chain from $c_{n-3}$ to $S$, or the chain is cut by the word's end. **SWAR partitioned add** (Warren §2-18, `(x AND NOT H) + (y AND NOT H) XOR ((x XOR y) AND H)`) stops carries at $H$. Here the carry must *skip* $G$, not stop there, so it does not apply.

## Same-axis add

### Formulas

| Name | Formula | Count | Machine OF/V valid as-is? | Exact overflow test on $r$ |
| --- | --- | --- | --- | --- |
| **bridge** | $b = (w_1 \text{ XOR } G) + w_2$; $\ r = b \text{ XOR } \big((b \text{ XOR } w_2) \text{ AND } G\big)$ | **5** | **yes**, on the `add` (2nd instruction) | yes |
| bridge, merge form | $b$ as above; $\ r = w_1 \text{ XOR } \big((w_1 \text{ XOR } b) \text{ AND NOT } G\big)$ | 5 | yes, 2nd | yes |
| **re-add** (found by the search) | $\delta G = \big(w_1 \text{ XOR } (w_1 + w_2)\big) \text{ AND } G$; $\ r = w_2 + (w_1 \text{ XOR } \delta G)$ | **5** | **yes**, on the **last** `add` | yes |
| pre-carry (found by the search) | $\delta G = \big(w_2 + (w_1 \text{ AND } C)\big) \text{ AND } G$; $\ r = w_2 + (w_1 \text{ XOR } \delta G)$ | 5 | yes, last | yes |
| carry pair, neg form (found by the search) | $s = w_1 + w_2$; $\ r = s \text{ XOR } \big(0 - ((s \text{ XOR } w_1) \text{ AND } G)\big)$ | 5 | **no** (carry into the top is $a$) | yes |
| carry pair, shift form | $s$ as above, $e = (s \text{ XOR } w_1) \text{ AND } G$; $\ r = s \text{ XOR } e \text{ XOR } (e \text{ shl } 1)$ | 6 (x86 `lea` 3e: 5) | no | yes |
| or/and bridge | $b = (w_1 \text{ OR } G) + (w_2 \text{ AND NOT } G)$; $\ r = b \text{ XOR } ((b \text{ XOR } w_1) \text{ AND } G)$ | 6 | yes, 3rd | yes |

- In the bridge, $w_1 \text{ XOR } G$ makes the axis column $(1-a) + a = 1$, so the count's carry $\gamma$ passes to bit $n-1$ and bit $n-2$ comes out as $\text{NOT } \gamma$. The fix copies $a$ back. In the bridge $b \text{ XOR } w_2$, $w_1$ may stand in for $w_2$.
- The bridge's add computes exactly the sign column of an $(n-1)$-bit add: sign bits $s_1$ and $s_2$, carry-in $\gamma$. So:
  - its **x86 OF / ARM64 V is the signed overflow of $v_1 + v_2$**, on either axis;
  - its **CF/C is the $(n-1)$-bit carry out**.
- The fix never touches bit $n-1$. So the sign test may use $b$ or $r$.
- **Re-add and pre-carry: one plain add, done twice.**
  - The first add finds $\delta = a \text{ XOR } \gamma$ (bit $n-2$ of $w_1 + w_2$ is $\gamma$, of $w_2 + c_1$ is $a \text{ XOR } \gamma$).
  - Flipping $w_1$'s axis bit when $\delta = 1$ makes the second, ordinary add correct. When $\delta = 0$ the axis column is $a + a + a$, which gives $a$ and carries $\gamma = a$. When $\delta = 1$ it is $(1-a) + a + \gamma$, which gives $\text{NOT } \gamma = a$ and carries $\gamma$.
  - So the result comes straight out of an `add`, and **that add's OF/V/CF are the true $(n-1)$-bit flags**. `add; jo` are adjacent. (Intel does not macro-fuse `add` with `jo`; `jc`/`jl` do fuse, *from memory*.)
- **Carry pair, neg form.** $-(\delta G) = \delta K \pmod{2^n}$, so one `neg` flips both bits.
- **Overflow behaviour** (every formula in the table): $r$ is the sum wrapped like an $(n-1)$-bit signed integer, on the operands' axis. For example, $3 + 1 = -4$ on 4 bits. The axis is never corrupted. This matches int64's wrap, with $n-1$ bits.

**Branch-free overflow test**, exact on both axes:
- $o = \big((w_1 \text{ XOR } r) \text{ AND } (w_2 \text{ XOR } r)\big) \text{ sar } (n-1)$: 4 as a mask, 3 to a sign for `bltz`/`slt`.
- Also exact: $(\text{NOT}(w_1 \text{ XOR } w_2) \text{ AND } (w_1 \text{ XOR } r))$, the same count with `andn`.

### Ranking: add plus overflow check

| Candidate | Add | + overflow | Total |
| --- | --- | --- | --- |
| **re-add, x86-64** *(hand)*: `lea t,[w1+w2]; xor t,w1; and t,G; xor t,w1; add t,w2; jo …` | **5, no `mov`** | 0 (`jo`) / 1 (`seto`) | **5** |
| bridge, x86-64 *(hand)*: `mov x,w1; xor x,G` (or `btc x,62`)`; add x,w2; jo …; mov t,x; xor t,w2; and t,G; xor x,t` | 5 (+2 `mov`) | 0 / 1 | 5 (+2 `mov`) |
| **bridge, ARM64** *(hand)*: `eor x,w1,#G; adds r,x,w2; lsr t,w1,#62; bfi r,t,#62,#1` | **4** | 0 (`b.vs`) / 1 (`cset`) | **4** |
| re-add, ARM64 *(hand)*: `add; eor; and; eor; adds` | 5 | 0 / 1 | 5 |
| bridge or re-add, RV64 (no flags) | 5 | 3 (`xor; xor; and; bltz`), 4 as a mask | 8–9 |
| carry pair (either form) | 5 | flag invalid; test 3–4 | 8–9 |
| or/and bridge | 6 | 0 / 3 | 6 / 9 |

ARM64 `bfi` copies $a$ in: `lsr` brings bit 62 of $w_1$ to bit 0 and `bfi` inserts it at bit 62. `bfi` cannot copy a bit to the same position from another register without the shift.

**Answer to the author's focus.**
- **No add of length ≤ 4 exists** in the search's operation set, with or without the flag.
- Of the 160 length-5 programs (all of which generalise to 64 bits):
  - 82 leave the machine's signed-overflow flag exact on both axes: 32 bridges (flag on the 2nd instruction; half of them are the `sub` form $(w_1 \text{ XOR } \lnot G) - w_2$, which is $\lnot(v_1 + v_2)$, and its overflow is the same), and 50 re-add/pre-carry programs (flag on the last);
  - 78 are carry-pair forms, whose flags are wrong.
- All 160 pass the branch-free sign test.
- So add + overflow check is **5 on x86-64** (re-add: no `mov`, flag on the final `add`), **4 on ARM64** (bridge with `bfi`; outside the search's operation set), and **8 on RV64** (5 + the 3-instruction test).

### Against the C layout and int64

| | Add | Overflow |
| --- | --- | --- |
| int64 | 1 (`add`) | OF free / 3 by sign test |
| C layout (sign by two's complement at top, axis at bit 0): $w_1 + (w_2 \text{ AND NOT } 1)$ | 2 | OF of that add valid, since the axis bit is below the value; test 3 |
| complement layout, bridge | 5 (ARM64 4) | OF of the bridge add valid; test 3 |

The extra 3 are the fix that writes the axis back. They are the price of having the axis between the sign and the count.

## Same-axis subtract (4)

$$d = w_1 - w_2, \qquad r = d \text{ XOR } \big((d \text{ XOR } w_1) \text{ AND } G\big)$$

- **No bridge is needed.** The axis column is $a - a - \beta$, which outputs $\beta$ and borrows $\beta$. So the borrow passes straight to bit $n-1$.
- **The `sub`'s OF/V is the exact signed overflow of $v_1 - v_2$ as-is**, and its borrow is the $(n-1)$-bit borrow.
- The test without flags: $((w_1 \text{ XOR } w_2) \text{ AND } (w_1 \text{ XOR } r))$, sign, 3.
- Merge form: $r = w_1 \text{ XOR } ((w_1 \text{ XOR } d) \text{ AND NOT } G)$.
- ARM64 *(hand)*: `subs d,w1,w2; lsr t,w1,#62; bfi d,t,#62,#1` = **3**.
- **Shortest within the search** (below). So subtract is one cheaper than add; computing add as $w_1 - (-w_2)$ costs more.

## Negate (4)

$$z = w \text{ AND } G, \qquad d = z - w, \qquad r = (d \text{ AND NOT } G) \text{ OR } z$$

(XOR or $+$ in place of OR also works.)

- $d = -(\text{sign and count})$, with the borrow passed through the axis column.
- **The `sub`'s OF/V is set exactly when $w$ is $-2^{n-2}$ on either axis** (no negation). The wrap gives $-(-2^{n-2}) = -2^{n-2}$, like int64's `neg`.
- Shortest within the search, and unique up to the final operator.
- A variant with $z = w \text{ AND } K$ is also 4 but **its flag is not valid** (checked).
- ARM64 *(hand)*: `and z,w,#G; subs d,z,w; and d,d,#~G; orr r,d,z` = 4. x86-64: 4 (+1 `mov`).

## Full add on the axes

Rules:
- Same axis: the sum.
- Different axes, one operand zero: the other operand.
- Both nonzero on different axes: set the expression flag $e$.

Either zero (0 or $0i$) is accepted where the result is zero.

**Fact used.** If $w_1$ is a zero (either axis), the answer is $w_2$. If $w_2$ is a zero, the answer is $w_1$. On the same axis this is just $0 + w = w$. So only "is it zero" matters, never the axis of the zero. A zero test is unavoidable: in the cross case, $w_1 \text{ OR } w_2$, $w_1 + w_2$ and $w_1 \text{ XOR } w_2$ all lose which operand was nonzero.

**Branch-free, no selects (RV64-style): 11, plus 5 for $e$ = 16.**

$$
\begin{aligned}
t &= w_1 \text{ XOR } w_2 && 1 \\
z_1 &= w_1 \text{ AND NOT } G && 1 \\
b &= z_1 + (w_2 \text{ OR } G) && 2 \quad \text{(a bridge on every input: axis column } 0 + 1\text{)} \\
Z_1 &= 0 - (z_1 = 0) && 2 \\
\textit{src} &= w_1 \text{ XOR } (t \text{ AND } Z_1) && 2 \quad (w_2 \text{ if } w_1 \text{ is zero, else } w_1) \\
r &= b \text{ XOR } \big((b \text{ XOR } \textit{src}) \text{ AND } G\big) && 3 \\
e &= \big((t \text{ AND NOT } Z_1) \text{ shr } (n-2)\big) \text{ AND } (z_2 \ne 0),\ \ z_2 = w_2 \text{ AND NOT } G && 5 \quad (\text{bit } 0)
\end{aligned}
$$

- The bridge $z_1 + (w_2 \text{ OR } G)$ gives the right sign and count in all three cases. The axis written back is $a_1 \text{ XOR } (\text{cross} \text{ AND } z_1 = 0)$: $a_1$, unless $w_1$ is the zero of the other axis.
- Same-axis overflow: OF of the bridge `add` is valid here too. The cross cases never overflow.

**With selects** (ARM64 `csel`, x86-64 `cmov`) *(hand)*: the 4-step ARM64 bridge add, then `tst w2,#~G; csel r,w1,r,eq; tst w1,#~G; csel r,w2,r,eq` (4), so 8. The flag adds about 5 (`eor`, `ubfx`, a `ccmp` chain, `cset`, `and`), about 13 in total. On x86-64, $\lnot G$ is not an imm32. The zero test is `rorx t,w,62; cmp t,1` (the rotated word is ≤ 1 exactly for a zero), then `cmovbe`: 3 per operand.

The superoptimizer found no full add (without the flag) of length ≤ 4.

## Multiply and exact division (reference)

**Sign extension.** $x = w \text{ XOR } \big((w \text{ XOR } (w \text{ shr } 1)) \text{ AND } G\big)$ copies $s$ into bit $n-2$. That turns the word into the $n$-bit two's complement of the axis value. It costs 4. ARM64 *(hand)*: `asr x,w,#1; bfxil x,w,#0,#62` = 2.

**Multiply: 18.** $i \cdot i = -1$.

$$
\begin{aligned}
m &= ((w_1 \text{ AND } w_2) \text{ shl } 1) \text{ sar } (n-1) && 3 \quad \text{(both imaginary)} \\
p &= \big((x_1 \text{ XOR } m) - m\big) \cdot x_2 && 3 \ (+8 \text{ for } x_1, x_2) \\
r &= p \text{ XOR } \big((p \text{ XOR } w_1 \text{ XOR } w_2) \text{ AND } G\big) && 4
\end{aligned}
$$

- Correct whenever the product fits. On overflow the result is not the $(n-1)$-bit wrap: bit $n-1$ is bit $n-1$ of the $n$-bit product.
- ARM64 *(hand)*: about 12.

**Exact division: 18.** $v / (i u) = -i\,v/u$.
- The same with $m = ((w_2 \text{ AND NOT } w_1) \text{ shl } 1) \text{ sar } (n-1)$ (3, or 2 with `andn`/`bic`) and $q = x_1 \div_s x_2$.
- The sign-extended values are $\ge -2^{n-2}$, so `idiv`/`sdiv` cannot trap.
- $-2^{n-2} / -1$ is out of range, as with int64.

**Against the packed layouts.** Prior art has top 7 / bottom 6 for multiply and 6 for division. The complement layout pays for converting to and from two's complement (4 + 4 + 3–4). It is the worst of the four layouts for multiply and divide, as split was. The gain is in add and subtract (5 and 4, with a free machine overflow flag), against 13 for the sign-magnitude packed forms.

## Superoptimizer

`superopt.c` is an exhaustive search over straight-line DAG programs.

**Operations, one instruction each:**
- `not`, `neg`;
- `shl`, `shr`, `sar`, `rol` by every constant $1 \ldots N-1$ (`ror` is `rol` by $N-k$);
- `add`, `sub`, `and`, `or`, `xor`, `andn` (both orders) on any two values;
- `add`, `sub`, `rsub`, `and`, `or`, `xor`, `candn` with a constant from $\{1, \lnot 1, G, \lnot G, S, \lnot S, K, C\}$.

**Test cases.** The cases are every same-axis pair: 128 on $N = 4$ and 2,048 on $N = 6$. Every case is a lane, so value equality is exact.

**Pruning (all safe, so "no hit" is a proof within the operation set):**
- drop a value equal to an input, an earlier value or a set constant;
- every value must be used later;
- independent adjacent instructions are kept in one canonical order;
- operands of commutative operators are ordered.

**Hit tags.** Each hit is tagged:
- `OF@vK`: the machine signed-overflow flag of add/sub instruction K equals the true overflow on every lane;
- `signtest-ok`: the branch-free sign test on $r$ is exact.

| Target | $N$ | Shortest | Hits at that length | Searched |
| --- | --- | --- | --- | --- |
| same-axis add (exact word) | 4, 6 | **5** (none ≤ 4 on $N = 4$ or $6$) | **160** at $N = 4$ (873M nodes, 12 min): 32 bridge `OF@v2`, 50 re-add/pre-carry `OF@v5`, 78 carry pair with no valid flag; all `signtest-ok`. All 160 re-checked on $n = 6, 8$ (exhaustive) and 64 (random): no failures. List: [`add-L5-hits.txt`](test/complement/add-L5-hits.txt) | ≤ 4 on $N = 6$ (5.9M nodes), ≤ 5 on $N = 4$ |
| same-axis add, either zero allowed | 4 | none ≤ 4 | | ≤ 4 |
| same-axis subtract | 4, 6 | **4** | 4: the formula above, with $w_1$ or $w_2$ as fix source, and the merge form; all `OF@v1 signtest-ok` | |
| negate | 4, 6 | **4** | 5: the formula above with OR/XOR/+, `OF@v2`; the $K$ variants have no valid flag | |
| full add (no flag) | 4 | none ≤ 4 | | ≤ 4 |

The 4-bit no-hit results also rule out any width-generic program of that length, provided its shift amounts and constants exist at 4 bits.

Reproduce: `gcc -O3 -o so superopt.c && ./so 6 add 4 && ./so 4 add 5 2000 && ./so 6 sub 4 && ./so 6 neg 4 && ./so 4 full 4`, then `python3 -I generalize.py add-L5-hits.txt`. None of the hits use shifts or rotations, so the 4-bit programs carry over to 64 bits symbol for symbol ($G, \lnot G, C, K$).

## Approaches tried that do not beat the bridge

- **Carry pair** (use the count's overflow into the axis as information): $\delta = $ bit $n-2$ of $s \text{ XOR } w_1$, then flip bits $n-1$ and $n-2$. It ties at 5, but it spoils the flag. Using the same $\delta$ to correct an *input* and add again (re-add) keeps the flag. That is the search's best idea.
- **Add as subtract.**
  - Bridge by `sub`, $w_1 - (w_2 \text{ XOR } \lnot G)$: this gives $v_1 + v_2 + 1$, and its OF is that of the wrong sum.
  - $\lnot(\lnot v_1 - v_2)$ (Warren §2-2): 6.
- **x86 BMI2 `pext`/`pdep`.** `pext` removes $G$, then add, `pdep`, and OR in $a$: 6. The 64-bit add's OF is then about bit 63, not about the $(n-1)$-bit value. **Flag invalid.**
- **Rotations** and **SWAR partitioned add**: impossible or inapplicable, as shown above.
- **FP adders.** A mantissa overflow that carries into the exponent is the opposite situation (wanted there). The two-path, swap and complement ideas belong to sign-magnitude and give nothing for a two's-complement field.
- **`adc`/`sbb` tricks.** x86 has no one-instruction "copy CF into bit 62". The fix is 3 there; ARM64 `bfi` makes it 2.

## Open points

- The "either zero" relaxation did not shorten add at length ≤ 4. Length 5 was not run for it.
- Length 6 was not searched, and length 5 only at $N = 4$ (the $N = 6$ run would take about 2 hours on this machine). The full add (16) and the multiply/divide (18) are hand-built, not searched.
- Nothing is timed. The x86-64 and ARM64 counts are by hand.

## Round 2: optimization

Nov 2, 2026. Cheaper arithmetic for the complement layout. Conventions as above: one instruction per operator, constants ($G$, $\lnot G$, $S$) free, `mov` copies not counted; *(hand)* means not assembled. Counts below are without loads and stores (add 3 for the harness's loop counts: round 1's mul 18 is `results-cpl.md`'s 21).

### M1: multiply and exact division, by reasoning

Script: [`test/complement/m1.py`](test/complement/m1.py), about 15 s. Every program is a list of instructions, so its count is its length; x86-64 and ARM64 instructions (`lea`, one-operand `imul`, `shrd`, `test`/`cmov`, `pext`/`pdep`, `bfxil`, `bfi`, `ubfx`, shifted-operand `eor`/`tst`/`bics`, `cneg`) are modelled at width $n$. It is checked exhaustively on $n = 4, 6, 8$ (every pair) and on 43,611 random and edge pairs at $n = 64$. **No in-range failures and no divide traps.** Division is checked on every nonzero divisor, exact or not (truncating).

**Three facts** (checked exhaustively in the script). Write $h = (w \text{ shl } 1) \text{ sar } 1$ (the word with the axis bit copied into bit $n-1$) and $e = a \text{ XOR } s$.

$$
\text{(I1)}\ \ w + h \equiv 2v \pmod{2^n} \qquad
\text{(I2)}\ \ w_1 w_2 \equiv v_1 v_2 + 2^{n-2}\,(e_1 c_{2,0} \oplus e_2 c_{1,0}) \pmod{2^{n-1}} \qquad
\text{(I3)}\ \ (w_1 \text{ shl } 1)(w_2 \text{ shl } 1) \equiv 4 v_1 v_2 \pmod{2^n}
$$

- (I1): $2v = 2c - 2^{n-1}s$ fits in $n$ bits, so **$2v$ costs 3** (`shl`, `sar`, `add`; x86 `lea`, `sar`, `lea`), one less than the sign extension $x$ (4). It also leaves $L = w \text{ shl } 1$, whose top bit is $a$.
- (I2) is the predicted-fix idea applied to the raw words. The count bits of $w_1 w_2$ are already right. The sign bit is off by a parity term $e_1 c_{2,0} \oplus e_2 c_{1,0}$, which needs both $e$'s and both low bits (about 9 operations). That is more than converting the operands, so **the raw product is not used.**
- (I3) is exact but loses bit $n-2$ of the product (the sign). Only a widening multiply gets the sign back.

**Multiply** (result $(-1)^{a_1 a_2} v_1 v_2$ on axis $a_1 \oplus a_2$):

| Form | Formula | Count | Overflow |
| --- | --- | --- | --- |
| round 1 | $x_1, x_2$ (4 each); $m = ((w_1 \text{ AND } w_2) \text{ shl } 1) \text{ sar } (n{-}1)$; $p = ((x_1 \oplus m) - m)\,x_2$; fix | 18 | not the wrap |
| **generic, scaled** | $L_i = w_i \text{ shl } 1,\ h_i = L_i \text{ sar } 1,\ D_i = w_i + h_i$ (3 each); $x_2 = D_2 \text{ sar } 1$; $m = (L_1 \text{ AND } L_2) \text{ sar } (n{-}1)$; $P = ((D_1 \oplus m) - m)\,x_2$ ($= \pm 2p$); $p = P \text{ sar } 1$; fix | **17** | **wrap** |
| generic | the same with $x_1 = D_1 \text{ sar } 1$ and no final `sar` | 17 | not the wrap |
| **x86-64** *(hand)* | $D_i$ by `lea`/`sar`/`lea` (3 each); `test L1,L2` (SF $= a_1 a_2$), `neg`, `cmovs` on $D_1$; one-operand `imul` → rdx:rax $= \pm 4p$; $A = (L_1 \oplus L_2) \text{ shr } (n{-}1)$; `lea hi,[A+2*rdx]`; `shrd rax,hi,2` | **14** (+3 `mov`) | **wrap** |
| x86-64 BMI2 *(hand)* | $y_i = \text{pext}(w_i, \lnot G)$; `and t,w1,w2; bt t,62` (CF $= a_1 a_2$), `neg`, `cmovc` on $y_1$; $P = y_1 y_2$; $r = \text{pdep}(P, \lnot G) \text{ OR } ((w_1 \oplus w_2) \text{ AND } G)$ | **11** (12 with a mask instead of `cmov`) | **wrap** |
| **ARM64** *(hand)* | `asr x,w,#1; bfxil x,w,#0,#62` (×2); `mul p,x1,x2`; `ubfx t,w1,#62,#1; tst t,w2,lsr #62; cneg p,p,ne`; `eor u,t,w2,lsr #62; bfi p,u,#62,#1` | **10** | not the wrap |
| generic `mul`+`mulh` (RV64) | the x86 route without a widening multiply or funnel shift | 19 | wrap |

The fix is round 1's $r = p \oplus ((p \oplus w_1 \oplus w_2) \text{ AND } G)$, 4 operations.

- **Where the saving is.**
  - $D = 2v$ costs 3 where $x$ costs 4.
  - The mask comes from $L_1 \text{ AND } L_2$, whose top bit is $a_1 a_2$, in 2 operations rather than 3.
  - **Scaled form.** $(\pm 2v_1) \cdot x_2 = \pm 2p$ exactly mod $2^n$, so $p = P \text{ sar } 1$ copies bit $n-2$ of the product into the sign. The overflow is then **the $(n-1)$-bit wrap on the right axis, as for add.** Round 1's form is not (its bit $n-1$ is bit $n-1$ of the $n$-bit product).
- **x86-64.** $\pm 4p$ fits in 128 bits. When $p$ fits, `rdx` is $0$ or $-1$ (the sign). `lea hi,[A+2*rdx]` puts $A$ in bit 0 and the sign in bit 1, and `shrd rax,hi,2` moves the count down and those two bits into bits 62–63. On overflow, bit 0 of `rdx` is bit $n-2$ of $p$, so it wraps. BMI2 `mulx` would do the same without flags.
- **BMI2.** `pext` removes the axis bit, which leaves the 63-bit two's-complement value (bit 63 is 0, so it is right mod $2^{63}$, which is all the product needs). `pdep` re-inserts the gap, and the OR writes $A$. `pext`/`pdep` are 1 µop and 3 cycles on Intel since Haswell and on AMD since Zen 3, but microcoded and slow on Zen 1/2 *(from memory)*.
- **ARM64.**
  - `ubfx` isolates $a_1$ in bit 0. A shifted-operand `tst` then sets Z $= \lnot(a_1 a_2)$ in one instruction, and the same `t` gives $A$ in bit 0 for `bfi` through a shifted `eor`. So the i·i sign and the axis fix together cost 5, not 6.
  - The ARM64 forms keep the round 1 overflow (not the wrap). Using $D_i$ (`and D,w,#S; bfi D,w,#1,#62`, 2) plus `asr` would give the wrap for +1.

**Exact (and truncating) division** (result $\operatorname{trunc}(v_1/v_2) \cdot (-1)^{a_2 \bar a_1}$ on axis $a_1 \oplus a_2$):

| Form | Formula | Count |
| --- | --- | --- |
| round 1 | $x_1 \div x_2$, mask $((w_2 \text{ AND NOT } w_1) \text{ shl } 1) \text{ sar } (n{-}1)$, condneg, fix | 18 with `andn` (19 without, as `results-cpl.md` found) |
| **generic** | $D_1 \div D_2$ ($= v_1 / v_2$: the quotient is unchanged by scaling both); $m = (L_2 \text{ AND NOT } L_1) \text{ sar } (n{-}1)$; $q' = (q \oplus m) - m$; fix | **15** (16 without `andn`) |
| x86-64 *(hand)* | $D_i$ (3 each); `cqo; idiv`; `andn k,L1,L2` (sets SF $= a_2 \bar a_1$), `neg`, `cmovs`; fix | **15** with BMI1 (16 with `not; test`) |
| **ARM64** *(hand)* | sign extensions (2 each); `sdiv`; `ubfx t,w2,#62,#1; bics xzr,t,w1,lsr #62; cneg q,q,ne`; `eor u,t,w1,lsr #62; bfi q,u,#62,#1` | **10** |

- **No trap.** $D_2$ is even, so $D_1 \div D_2$ never meets $\text{INT\_MIN} \div -1$; $D_1 = -2^{63}$ only with $D_2 = \pm 2$ or larger.
- **Overflow.** The only overflowing quotient is $+2^{n-2}$ (from $-2^{n-2}$ divided by $\pm 1$ with the sign rule). Every form, round 1's included, returns **0 on the right axis** for it, not the wrap $-2^{n-2}$.
- The scaled route works because a quotient does not change when both operands are scaled. The multiply's scaled route costs an extra `sar` at the end, so it only ties there.

**Summary of M1** (operation only; +3 for loads and the store):

| | Round 1 | Generic (RV64) | x86-64 *(hand)* | ARM64 *(hand)* |
| --- | --- | --- | --- | --- |
| multiply | 18 | **17**, wraps | **14** (BMI2: **11**), wraps | **10** |
| divide | 18 (19 no `andn`) | **15** (16) | **15** (16 no BMI1) | **10** |

Control (pair of int64) is 9 for multiply and 7 for divide on the same basis. The layout still pays about 6–8 for the conversions and the i·i sign on generic machines, and 1–3 on ARM64.

**Not used, and why.**
- **The raw product with a predicted fix (I2):** the parity correction costs more than the conversion.
- **(I3) with `mulh` to recover the sign:** 19 on generic machines. It only pays with x86's widening `imul` + `shrd` (14).
- **Folding the i·i negation into $D$:** $-2v = w - 3h$ holds, but a conditional form costs as much as `xor`/`sub` after it.
- **A cheaper sign extension:** none of length ≤ 3 is known; M2 searches for one.

### M2: superoptimizer searches, and the full add

**Tool.** [`test/complement/superopt2.c`](test/complement/superopt2.c): the same exhaustive DAG search as `superopt.c`, with the same safe pruning, so "none ≤ L" is a proof within the operation set. It adds:
- up to 3 inputs per lane (for example the machine's own product $p$ beside $w_1, w_2$);
- a care mask per lane (only some bits must match);
- optional operations: `mul`, `div`, `seqz`/`snez` (Z), RISC-V Zicond `czero.eqz`/`czero.nez` (Q), and Zbb `maxu`/`minu` with `sltu` (U).

Runs are resumable by first instruction (`SO_LMIN`, `SO_START`). Logs: [`test/complement/m2-logs/`](test/complement/m2-logs/). Checks of the new formulas: [`test/complement/m2.py`](test/complement/m2.py), exhaustive on $n = 4, 6, 8$ and 60,000 random/edge 64-bit pairs, **0 failures**.

**Multiply and divide: the pieces around the machine's `mul`/`div`.** A whole multiply (M1: 17) is far beyond exhaustive reach, so each piece was searched separately.

| Piece | $N$ | Shortest | Hits / note |
| --- | --- | --- | --- |
| sign extension $x$ | 6 | **4** (none ≤ 3) | 235, e.g. round 1's and M1's $(w + ((w \text{ shl } 1) \text{ sar } 1)) \text{ sar } 1$ |
| $v \bmod 2^{n-1}$ (top bit free) | 6 | **4** (none ≤ 3) | 937. Freeing the top bit does not help, so only BMI2 `pext` (1) beats 4. |
| $2v$ | 6 | **3** (none ≤ 2) | 10, including M1's $w + ((w \text{ shl } 1) \text{ sar } 1)$ and the shift-free $(w \text{ OR } G) + (w \text{ OR } K)$ and $z + (z \text{ AND } C)$, $z = w \text{ AND } \lnot G$ |
| $-2v$ | 6 | 4 | 232 |
| mask of $a_1 a_2$ / of $a_2 \bar a_1$ from $w_1, w_2$ | 6 | **3** | round 1's (and `rol 1` for `shl 1`). From M1's $L_i$ it is 2. |
| conditional negate of $x$ by that mask, from $x, w_1, w_2$ | 4 | **5** (none ≤ 4) | round 1's mask + `xor`/`sub` is optimal; M1 saves only through the shared $L_i$ |
| axis fix $p \to r$ (no negation) | 6 | **4** (none ≤ 3) | 4: round 1's fix and its merge form |
| i·i sign + fix, from $(p, w_1, w_2)$ | 4 | **none ≤ 5** (by hand: 9) | all 234 first instructions, 2.55·10⁹ nodes |
| divide sign + fix, from $(q, w_1, w_2)$ | 4 | **none ≤ 5** (by hand: 9) | all 234 first instructions, 2.56·10⁹ nodes |

So every piece of M1's multiply (17) and divide (15) is at its shortest in this operation set, given the inputs it takes. What is left to win is in sharing across pieces (as $L_i$ does), and that is beyond the search. No whole-program lower bound is claimed.

**Full add on the axes.** The search target `faxis` asks only for a word whose axis bit is the result's axis: $a_1$, except $a_2$ when $w_1$ is a zero on the other axis; zero results are free. It found sources shorter than round 1's mask-and-select (5, with $Z_1$):

| Ops | Shortest | Axis source |
| --- | --- | --- |
| base + `seqz` | **4** (none ≤ 3) | $\textit{src} = w_2 - \operatorname{seqz}\big(w_2 \text{ XOR } (G \text{ AND NOT } w_1)\big)$ |
| + Zicond | 4 | `czero` selects, e.g. $y = \operatorname{czero.eqz}(w_1, w_1 - G)$; $\textit{src} = y + \operatorname{czero.nez}(w_2, y)$ |
| + Zbb | **3** (none ≤ 2) | $v = \operatorname{maxu}(w_1, w_2)$; $\textit{src} = \operatorname{maxu}(v, K - v)$ |

- **Generic source.** $G \text{ AND NOT } w_1$ is the zero word on the axis other than $a_1$. So the XOR is 0 exactly when $w_2$ is that zero, the only case where the axis must come from $w_1$. Subtracting 1 from that zero ($G \to C$, or $0 \to$ all ones) puts $a_1$ into bit $n-2$. Otherwise $\textit{src} = w_2$, whose axis is right.
- **Zbb source.** A zero word is 0 or $G$, so the unsigned maximum is the nonzero operand. The one exception is a positive real below $G$ against the imaginary zero $G$. There $v = G$, and $K - v = S$ wins with axis 0.
- **Expression flag** $e$: none of length ≤ 4 even with Z, Q and U (`eflag`, $N = 4$). Round 1's 5 shares $Z_1$, and the rotated-key form below shares $\rho_i$.

**Results** (operation only; the fix is $r = b \text{ XOR } ((b \text{ XOR } \textit{src}) \text{ AND } G)$, 3):

| | Bridge $b$ (3) | Source | Total | Round 1 |
| --- | --- | --- | --- | --- |
| full add, no flag, generic | $(w_1 \text{ AND } \lnot G) + (w_2 \text{ OR } G)$ | `seqz` form, 4 | **10** | 11 |
| full add, no flag, Zbb | same | `maxu` form, 3 | **9** | 11 |
| full add + $e$, generic | round 1 (the flag reuses $Z_1$) | | 16 | 16 |
| full add + $e$, Zbb | same | $\rho_i = \operatorname{rol}(w_i, 2)$, $\textit{src} = \operatorname{ror}(\operatorname{maxu}(\rho_1, \rho_2), 2)$, 4; $e = \operatorname{sltu}(1, \operatorname{minu}(\rho_1, \rho_2)) \text{ AND } (\rho_1 \text{ XOR } \rho_2)$, 4 | **14** | 16 |
| full sub, no flag | $z_1 - z_2$ ($z_i = w_i \text{ AND } \lnot G$) | 4 / 3 | **10** / **9** (Zbb) | 15 (negate 4 + 11) |
| full sub + $e$, generic | $z_1 - z_2$; round 1's $Z_1$, src and $e$, with $z_2$ shared | | **15** | 20 |
| full sub + $e$, Zbb | $z_1 - z_2$ | rotated keys as for add | **14** | 20 |

- **Subtract needs no negate first.** In $z_1 - z_2$ both axis bits are cleared, so the axis column is $0 - 0 - \beta$. It outputs $\beta$ and borrows $\beta$ on every input, including the cross cases ($w_1$ zero gives $-v_2$). That is round 1's same-axis subtract argument, applied to all lanes. The source and the fix are the add's, because the result's axis rule is the same.
- **Rotated keys.** $\rho = \operatorname{rol}(w, 2) = 4c + 2s + a$. A zero word gives $\rho \le 1$, and every nonzero word gives $\rho \ge 2$. So $\operatorname{maxu}$ picks the nonzero operand, and its bit 0 is the axis. $\operatorname{minu} \ge 2$ means both operands are nonzero, and bit 0 of $\rho_1 \text{ XOR } \rho_2$ is "different axes". The flag $e$ is bit 0.
- Same-axis overflow is the $(n-1)$-bit wrap in every form, as in round 1: the bridge's sign and count are round 1's. The cross cases never overflow.
- **x86-64 / ARM64** *(hand)*:
  - `maxu` is `cmp` + `cmovb` or `csel` (2), so the Zbb source is 5 there.
  - The generic `seqz` source is 5 on ARM64 (`bic t,G,w1`, `eor`, `cmp`+`cset`, `sub`) and 6 on x86-64 (`andn`, `xor`, `test`+`sete`+`movzx`, `sub`).
  - So off RISC-V the gain is about 1, and round 1's ARM64 select form (8) is still the shortest there.

**Summary of round 2** (operation only, without loads and the store; the harness adds 3):

| Operation | Round 1 | Generic (RV64, no extensions) | RV64 + Zbb | x86-64 *(hand)* | ARM64 *(hand)* |
| --- | --- | --- | --- | --- | --- |
| multiply | 18 | **17** (wraps) | 17 | **14** (BMI2 **11**) | **10** |
| divide | 18 (19 no `andn`) | **15** (16 no `andn`) | 15 | **15** | **10** |
| full add + $e$ | 16 | 16 | **14** | | |
| full sub + $e$ | 20 | **15** | **14** | | |
| full add / sub, no flag | 11 / 15 | **10** / **10** | **9** / **9** | | |
| same-axis add / sub / negate | 5 / 4 / 4 | unchanged (round 1 searched) | | | |

Reproduce:
- `gcc -O3 -o so2 superopt2.c`;
- `./so2 6 sext 4`, `./so2 6 twov 3`, `./so2 6 mmul 3`, `./so2 6 fix 4`, `./so2 4 cnegm 4`, `./so2 4 faxis 4 40 Z`, `./so2 4 faxis 3 40 U`, `./so2 4 eflag 4 40 ZQU` (seconds each);
- `SO_LMIN=5 SO_START=0 ./so2 4 postmul 5` and `postdiv` (about 35 min each, resumable by chunk);
- `python3 -I m2.py` (about 1 min).

**Open.**
- Whole-program searches for multiply, divide and the full add (lengths 9–17) are out of reach. Only the pieces are shown optimal.
- The post-multiply and post-divide bounds are at $N = 4$ and stop at length 5, against 9 by hand.
- Nothing was timed or assembled.
