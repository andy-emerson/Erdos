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

**Open questions**
- Which special values the type wants: ∞, undefined (NaN-like), null (missing, as in q), an overflow pointer. One reserved value with the other half free gives room for all.
- Whether stacks of words have a fixed maximum height (then the top word's −2^31 can give symmetry and an overflow marker) or always grow.
- Multiply in a stack wraps (the low words of the product, like the machine's multiply); overflow detection is separate.
