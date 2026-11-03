# Spec 14: Gaussian integers in one word — decisions so far

Nov 2, 2026, from the author's conversation. Not a specification yet.

**Decided**
1. **Scope.** Exact integers on the complex plane (Gaussian integers a + bi, and Gaussian integers mod n). No rationals. Users on the real line use the machine's own integers.
2. **The type.** One 64-bit word holds a + bi as two 32-bit halves (playground: real part in the low half). Its shape is closed under add, subtract, multiply and exact division.
3. **Infinity.** −2^31 is never a value. A number is the single complex point at infinity (no sign) when either half is −2^31. Every finite half then lies in −(2^31 − 1) … 2^31 − 1: a symmetric range, every value negatable. Checking for the marker is still to be designed.
4. **Chip-agnostic.** Pseudocode is chip-agnostic when an equivalent exists on each of x86-64, ARM64 and RISC-V, vector instructions included. Vector-oriented pseudocode qualifies.
5. **No branches** where avoidable: a branch turns a deterministic problem into a probabilistic one.
6. **Self-contained.** People opt into this type and stay in it; it is not mixed with other number types.
7. **Values live in vector registers,** even one number at a time, so no moves between ordinary and vector registers.

**Open**
- **Overflow, the author's preferred direction (Nov 2):** not a family of widths but one type with more wheels: a stack of identical one-word numbers, each half carrying into the same half of the next word (real into real, imaginary into imaginary), like multi-word integers. The per-half overflow test supplies each carry.
- **Overflow, earlier direction:** keep a whole array at one width; per-half overflow masks (the packed form of ((x XOR r) AND (y XOR r))) ORed across the array decide, once per array, whether to promote it to the next width (two int32 in one word → two int64 → bigger). An add overflows by exactly one wrap; a multiply's overflow is its high half; so the exact answer is recoverable at the next width.
- **Mod n.** Lazy reduction (1-instruction add) needs n < 2^29; reducing after every add allows n < 2^31 at about 6 instructions. Too expensive either way, in the author's view; to be improved.
- **Multiply.** No packed trick yet: four products and repacking. Barrett reduction for mod n is in the playground.

**Notes (Nov 2, later)**
- **Conversion.** Every integer type converts in (an integer n becomes (n, 0); a standard complex integer pair becomes (re, im)), through the overflow path when too big. Conversion is its own routine, run once on entry; the type owns its operators so arithmetic never falls back to the machine's plain operations. Non-integer types are the open case (refuse, or accept only exact integers that fit).
- **Infinity.** Correct (study 1), but costly as tested: add 5 → 21, multiply 15 → 26 (study 2 with exact overflow marking: 26–32). To try: vector compares; deferring the check (sticky, once per array); one shared path for overflow and infinity. Undefined cases (∞ − ∞, 0·∞): follow prior practice (IEEE NaN; C's complex infinity rules). When one half is −2^31 the other half is free, so one reserved value gives a family of markers (∞, undefined, overflow information) at no extra range.

**Tricks to try later** (after correctness and speed of the textbook version)
- A packed multiply: multiply the whole word without splitting it into halves first.
- Lazy reduction for mod n (shelved, not archived): add 1 instruction, subtract 2, but n < 2^29 (3 bits of headroom per half), which matrix multiplication would feel quickly.
- Complex multiply with three products instead of four; real and imaginary partial products formed together in one word (from study 3).
- Vector compares for the infinity check (first test when the vector build exists).
- Batch inversion (Montgomery's trick, study 13): many inverses mod one n for one real inverse plus 3 products each, about 81 instructions per element against 136 alone. For element-by-element division of arrays mod n, turning fractions a/b into residues, interpolation and Chinese-remainder weights. One non-invertible element fails the batch.
- Mod-p split form for p ≡ 1 (mod 4) (study 5): convert once, then cheaper products; for multiply-heavy work mod such primes.

**Open questions**
- Which special values the type wants: ∞, undefined (NaN-like), null (missing, as in q), an overflow pointer. One reserved value with the other half free gives room for all.
- Whether stacks of words have a fixed maximum height (then the top word's −2^31 can give symmetry and an overflow marker) or always grow.
- Multiply in a stack wraps (the low words of the product, like the machine's multiply); overflow detection is separate.

**Decided (Nov 2, evening)**
8. **The reserved value is the overflow marker,** not infinity, undefined or null (the host system has those). It still gives the symmetric range.
9. **The gcd may branch:** every practical gcd's length depends on the numbers; branch-free versions cost 4–5 times as much.
10. **Overflow, leading design:** the author's growing chain (each half carries into the same half of the next word, halves independently) with one shared length per array: when any number overflows, every number in the array gains a word (unused upper halves hold the sign). Uniform width, width always known, no pointers, no count limit. The deferred check (study 7) detects when to grow.

**Decided (Nov 2, night)**
11. **Overflow: the family of word sizes,** replacing item 10. On overflow the whole array is copied into words twice the size (64 → 128 → …), sign on top, split in halves, same rules at every size; past the largest native size each half becomes a multi-word integer (ordinary arbitrary precision). The user opts into the 64-bit type and never sees the bookkeeping. Arrays shrink back when every element fits in half the width; the shrink rule and its margin are under test (study 12). What becomes of the reserved value is open.

12. **Full two's-complement range; −2^31 is no longer reserved** (replacing items 3 and 8). Negating or conjugating −2^31 overflows like any other operation and promotes to the next size. Cheaper add, subtract, multiply and shrink test (study 15).

13. **Overflow detection, default: the sticky flag.** Each element's mask test is ORed into one register during an operation's sweep; checked once at the end; if set, the operation is redone at the next size from the untouched inputs. Shrink with one bit of margin (rule B, study 12). A survey of other strategies is running before this is final.

14. **The residue form (quadratic residue number system) is dropped** (studies 16 and 18). Each multiply-add costs more than the direct one below 64-bit halves, and conversions are paid per entry; square matrix products pay off only from n ≈ 60 at 32-bit halves. Powers of a matrix (study 22): wins only for n ≥ 8, not enough to bring it back. Closed.
15. **Layout: separate arrays of real and imaginary parts** for whole-array work (study 17); pairs for single values and for moving data in and out. 32-bit lanes are the common unit on CPU vectors and GPUs.

16. **Documentation stays chip-agnostic; the build targets x86-64** (the largest audience, and the only machine available for timing). AVX2 (256-bit) as the baseline, AVX-512 optional.
17. **The sticky flag is checked once per array,** not per chunk: the copy to wider words is needed anyway, so chunks would save only arithmetic, at the cost of a branch per chunk.

18. **Shrinking:** the shrink test is fused into the sweep of operations that can make values smaller (add, subtract, divide, mod), never multiply, and only for arrays above the base size (a 64-bit-word array has nothing smaller to shrink to). Rule B's one-bit margin. At the end of the sweep, if every element fits, the result is written at the smaller size.

19. **Mod n reduction: Montgomery in vector lanes** (study 19: 11.4 times faster than the hardware divide for the Gaussian product mod n, in cache; conversions repaid after about three multiplies). Reopen trigger: if a plain-register version of the build is ever needed, Barrett is the choice there (it ties Montgomery, needs no conversions, accepts even n).

20. **The sticky flag is final** (replacing the "survey running" note in item 13). The overflow survey found nothing better, and among the variants in use (Gubner–Boncz, CedarDB, Materialize) ours is at least as good: test inside the loop, check once per array.
21. **Bound tracking is dropped** (a stored structure; pays only for chains of adds).
22. **Inverses mod n: the norm route with Euclid** (x⁻¹ = conj(x) · N(x)⁻¹ mod n, study 13). Batch inversion stays on the tricks list.
23. **Equality only.** Gaussian integers have no order; the operation is called equality.

24. **Even moduli:** Montgomery for odd n, Barrett in vector lanes for even n. The parity check is once per modulus (n is fixed for an array), so it costs nothing per element.
25. **Conversion** (replacing the "not mixed" part of item 6: Gaussian integers are not closed under division, so conversion is needed). Another type converts to ours when it can; if not, and ours can convert to it, ours converts; otherwise the operation is refused. Open caveat: converting to an inexact type (floats) gives up exactness.
26. **Shrinking: the refined test is confirmed** by study 21 (pays for itself within 2 to 4 later operations; costs nothing measurable from memory when it never fires). Rule B for now, pending data on how often values hover near the line.
27. **Language: Nim, calling C's vector instructions (intrinsics) directly.** Vector instructions are written explicitly, not left to the compiler.

28. **Division with remainder rounds to nearest, ties toward zero** (study 25): q = sign(u)·((2|u| + N − 1) div 2N) per part, u a part of x·conj(y), N = N(y), computed on the magnitude with one sign mask. It satisfies the author's magnitude rule (sign changes, rotation by i and conjugation change only the direction of the answer), keeps N(r) ≤ N(y)/2, and was among the cheapest rules measured. Rationale: add, subtract and multiply never choose and favour no axis; division is the only basic operation that must choose, so its choice should keep the same symmetries; what the fixed-box convention offers can be had by other means.
29. **Exact division: multiply by the inverse mod 2^32** (study 25: 1.6 ns per element against 5.2 for the hardware divide; 0.21 ns with a shared divisor in cache).
30. **Division with remainder by one divisor shared across an array: the precomputed reciprocal** (study 25: 1.74 ns, three times faster than the divide, pays from 4 elements).
31. **Gcd: the (1+i)-ary binary gcd in vector lanes, at machine level** (studies 24 and 26: 42.7 ns per gcd against 109 for Euclid in lanes and 270 in plain registers). Pairs with a −2^31 half are set aside and recomputed exactly once per array.

32. **Division with remainder by divisors that vary per element: the per-lane reciprocal** (study 25: a steady 4.45 ns with two registers interleaved). Shift-and-subtract is dropped: fastest for short quotients, but its time varies with the data and it loses past about 8 quotient bits. The shared and per-lane reciprocals stay separate cases (the shared one is 2.5 times faster), chosen by the shape of the call (array by number, array by array), once per operation, never per element.

33. **Bit counting in the gcd (trailing zeros in vector lanes): branch to a table lookup, falling back to the multiply** (study 27, nbrd). One test per register per step: when every lane has k < 4 (about 95% of steps), k is one vpshufb lookup; otherwise the 16-bit de Bruijn multiply (vpmullw by 0x09AF, shift, lookup, moved down by 16 or 0). Integer only, no floating point. 36.3 to 36.9 ns per gcd with 3 registers, tying the same branch with a float fallback; 2.5% behind it with 1 register. The branch's outcome follows the data; the gcd's arithmetic makes small k common in every application, so the predictor is right about 95% of the time.

34. **Build target: ARM64 (NEON baseline, SVE2 when available); the test machine is x86 with AVX-512 used as a NEON stand-in** (replacing the build part of item 16). Test builds use 128-bit registers (AVX-512VL) and only operations NEON also has (a written portable instruction list): no AVX-512 mask registers, no 64-bit lane multiply, no three-input logic. Timings on x86 are read for ranking, not absolute speed: a variant faster than another on x86 should be faster on ARM too, and where an operation has no direct x86 equivalent (NEON's upper-half widening, 32-bit high-half multiply, larger table lookup) the ranking is flagged as uncertain. Correctness is tested in full on x86. AVX2 is no longer a target; the AVX2 studies remain the path for x86 chips without AVX-512.
35. **Consequences of item 34:** the gcd counts bits with the lane count-leading-zeros instruction (replacing item 33's branch); the grown-size multiply and the gcd's interleaving use the 32 registers NEON and AVX-512 have (a tie or a win, not retested); the layout stays separate arrays (better on NEON, ranking kept). Montgomery against Barrett is retested in NEON shape.

36. **Compiled-code check:** each kernel's disassembly is compared automatically with the list of instructions intended (study 28 began this: a scan for forbidden instructions, and empty asm barriers where gcc fused logic into vpternlog). Deviations are noted, not necessarily retested: a deviation may be an improvement, or a regression that forces inline assembly for that kernel.
37. **ARM build, candidate (study 28, verified by emulation only):** the overflow check uses NEON's saturating add and saturating narrowing, whose hardware sticky QC flag replaces the mask test: checked add costs the same as bare add (0.5 instructions per element against 2.5), checked multiply 3.0 against 4.5; one flag read per array. To confirm on ARM hardware.

**Research list**
- Barrett and Montgomery: Lean proofs still to do.
- Gcd: Weilert's (1+i)-ary algorithm (paper pending), the extended gcd by the norm route and by the extended binary gcd (studies 13–14).
- A Gaussian version of Bernstein–Yang's fixed-count divsteps: no known bound on the step count; a separate research project.

**For the documentation (Nov 3)**
- Division's rounding: explain the box (a fundamental domain; a unique quotient is a choice of box; the box inside the unit circle gives the small remainder; no box is symmetric because +1/2 and −1/2 differ by an integer), and the author's magnitude rule (if the operands' magnitudes don't change, the answer's magnitudes don't change).
- List the gains of a symmetric rule (sign changes, rotation by i and conjugation pass through division; magnitudes independent of signs; no favoured direction), and for each thing the fixed-box convention gives (one remainder per residue class: comparing, grouping, tables, the Chinese remainder theorem), show the clean equivalent in our library. Ties occur only when 1+i divides the divisor; for odd integer moduli every rule agrees.
- Tabled: offering both a symmetric division and a canonical mod (as Haskell and Ada do with rem and mod).
- Worked examples for both tie rules (ties toward zero and away from zero), including negative divisors and divisors turned by i. Facts worth stating: turning the divisor by a unit changes only the quotient's direction, never the remainder (both rules); under ties toward zero the remainder at a tie points the same way as x, under away from zero against it; outside ties both rules round to nearest, so a negative remainder can appear for all-positive inputs (7 ÷ 4 = 2 remainder −1).
