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
