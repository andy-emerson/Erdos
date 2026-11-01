# Review of Spec 13 (Signed Integers with a Header) v0.1

Nov 1, 2026. Reviewed against `RULES.md`, the design the author agreed on Nov 1, 2026, spec 12 v0.1 and `spec/12-integers/src/growing.nim`. Nothing was built or run.

**Verdict: needs revision.** The representation and the direction rules follow the author's design closely, and the timing method is spec 12's. However, two rules do not work as written (the header byte add and signed compare), Header's routines are not yet specified well enough to build without questions, and the spec is about twice spec 12's length (1522 words against 794), mostly because of jobs and pairings the author did not ask for. None of this is hard to fix.

## 1. Rules check

### Algorithm design

| Rule | Status | Spec section | Reason |
| --- | --- | --- | --- |
| 1 Exact | holds | §1, §3 | Every path is exact. A result past class 31 is reported as an error, not approximated. |
| 2 General before special | violated | §3 Result class, Mixed classes | Holding a class-0 by class-0 result in registers is a special case (a condition under which a faster path applies), and it is built rather than named. Widening the smaller operand is not the textbook path either: schoolbook routines take two lengths, as Growing's do. See F4 and F5. |
| 3 Prefer shared solutions | unclear | §3 Mixed classes | Division and gcd share Growing's routines. For add, subtract, multiply and compare the spec says only "run at the full class width", so a builder cannot tell whether these are Growing's routines or new ones. See F6. |
| 4 One structure each | holds, with one question | §3 Structures | Stored: the slot. Temporary: the predicted slot. Local: one buffer. The pool table is global, like malloc's heap, and the spec says so. The parallel-lists question is answered in section 3 below. |
| 5 Self-contained | holds | §2 | Nim, no libraries. |
| 6 Portable pseudocode (draft) | holds | §1, §3 | The spec gives no machine-level pseudocode. What it reuses from Growing is portable: carries and borrows come from comparisons, not a carry flag; the 64 by 64 to 128 product is available on all three families (MUL, UMULH, MULHU); Hensel division needs no 128 by 64 divide; the header ops are byte adds and masks. The little-endian claim holds on the common current forms of all three. |

### Optimization approach

| Rule | Status | Spec section | Reason |
| --- | --- | --- | --- |
| 1 Schoolbook first, then measure | holds | §2, §3 | Header builds on Growing's schoolbook routines, and the special cases are listed for later. The register path in F4 is the exception. |
| 2 Solve the problem in front of us | mostly holds | §1, §3 | Class 31 overflow and cross-axis adds are reported, not solved, and pools never shrink. But the W = 32 check is outside the agreed scope (F8). |
| 3 Match the control | unclear | §4, §5 | The spec mentions "the check" but never says what it checks. See F7. |
| 4 Timed runs alone | holds | §5 | Spec 12's method: an idle machine and one pinned core. |

## 2. Findings, most serious first

**F1. Blocking. §1 Header, the "byte add" of directions.** With the class in bits 0 to 4, adding two whole headers carries the class sum (up to 62) into bit 5 and then into the direction bits. The direction wraps mod 4 only if the class bits are cleared first. *Fix:* state the header ops exactly, with `D = 0xC0`. Multiply: `(h1 and D) + (h2 and D)`, the carry falling off the byte. Divide: `(h1 and D) - (h2 and D)`, kept to 8 bits. Negate: `h xor 0x80`. Opposite add: `(h1 and D) + (b shl 7)`. Then OR in the result class, which comes from its own rule.

**F2. Blocking. §3 Direction arithmetic, compare.** "By direction, then by magnitude" gives the wrong order in two ways. First, the direction codes do not order the values (d = 0 is positive, d = 2 is negative, so the smaller code is the larger value). Second, when both values are negative, the larger magnitude is the smaller value. *Fix:* "If the directions differ, the value with d = 0 is larger (zero has d = 0, so 0 > −x holds, and 0 compared with a positive value falls to the magnitude rule). If both have d = 0, compare magnitudes. If both have d = 2, compare magnitudes and reverse the result. Directions 1 and 3 are reported as an error."

**F3. Blocking. §3 Mixed classes, gcd (and the zero edge generally).** Growing's `opGcd` returns at once only when an operand's length is 0. A class-0 zero passed as one word `[0]`, or any operand zero-padded to a class width, gets past that test, and `ctzw` then scans past the end of the buffer looking for a nonzero word. Growing's division is safe, because `shrw` normalizes both lengths. *Fix:* pass Growing's routines each operand's normalized length (its top nonzero word, 0 for zero), not the class width. This also makes the widening step unnecessary (F5).

**F4. Should fix. §3 Result class, the register path.** "When the predicted class is 1 and both operands are class 0, the two-word result is held in registers" is a special case under rule 2, and it is built into Header as a coordinator's call. *Fix:* either move it to the special-case list as (d), with its own explanation, or get the author's explicit approval to build it. Without it, every same-direction add and every multiply in job 1 takes a class-1 slot, and the move-down then releases it. That is the honest general cost.

**F5. Should fix. §3 Mixed classes, widening.** Copying the smaller operand, zero-extended, to the larger class is an extra step, not the textbook one. On multiply it is costly: 1 word by 16 words becomes 16 by 16, which is 16 times the work, and the mixed job makes about one multiply in nine of this kind. Growing's add, subtract, multiply and compare already take two lengths, so the general solution is to call them on each operand's own words. That removes a local structure and special cases (a) and (c), and shortens the spec. *Fix:* "Each routine reads each operand's own words: the inline word as one word, or the slot up to its top nonzero word. Growing's routines handle unequal lengths."

**F6. Should fix. §2 and §3, which routines, and Control "as it stands".** Three points need stating.
- Control's opposite-direction add needs the final borrow to set the direction, but `opAbsdiff2` returns only the block.
- Header writes into a slot, but Growing's routines `malloc` their results.
- `opGcd` builds its result with `shlInto`, which writes n + q + 1 words. Its top word is zero, but it can still lie one word past a slot of class min(c1, c2).

*Fix:* "Growing's routines are refactored once into a form that writes into a given destination, has room for one extra word, and returns the final borrow where there is one. Control wraps them with `malloc`, and Header with a slot. Control must still give spec 12's values." This keeps one routine per operation (rule 3), and Control is re-timed in the same run anyway.

**F7. Should fix. §4/§5, the correctness check is not specified.** *Fix:* add one line: "Every result is checked against Python's exact value and against Control, value for value. Each Header result is also checked for canonical form: the smallest class, zero as header 0 and word 0, and the spare bit 0."

**F8. Should fix. §1 Word size, W = 32.** The agreed scope is W = 64 only, and W = 32 is ambiguous for Header. A class-0 word holds "up to 64 bits", so it is unclear whether the inline word becomes 32 bits and the class widths shift. *Fix:* drop the W = 32 check, and say that W = 64 is fixed for this spec.

**F9. Should fix. §2 coordinator's call, "job 1 here touches no pool".** This is false. In job 1, a multiply of two operands of up to 64 bits needs more than 64 bits about half the time, and a same-direction add carries out sometimes, so both take class-1 slots. Known cost 3 ("the gap between job 1 and job 2") is also a cross-job comparison rather than a comparison with Control. *Fix:* correct the claim, and read pool costs from Header × Control and callgrind's share of instructions in the pool routines (known cost 2), not from comparisons across jobs.

**F10. Should fix. §4 and §5, length and extra jobs.** There are 11 jobs (against spec 12's 5), add runs twice, and there are three within-build pairings. The author dislikes extra jobs and long specs and wants comparisons with Control. *Fix:*
- Replace the eight k-full and k-half jobs with one job per class, k ∈ {2, 4, 8, 16}, with bit lengths uniform from 32k + 1 to 64k. That covers the whole class, from little rounding to the most, and Header × Control shows the average rounding cost directly.
- Drop the k-half × k-full pairing.
- Drop the second add run, or keep it only if the author wants known cost 4. Subtract with random directions already exercises both branches.

These cuts and F5 should bring the spec close to spec 12's length.

**F11. Should fix. §4 Growth job against "every operation on every job, no exceptions".** The growth job runs only multiply and add, which contradicts §3's "every build and every job". *Fix:* call it a workload, not a job, in one sentence ("the growth workload is a chain of multiply and add, timed per step on both builds"). Alternatively, drop it. I recommend keeping it, because it is the only part of the spec that exercises release, reuse and growth of the pools.

**F12. Minor. §3 Result class, move-down target.** "Moves it there" should say "to the smallest class that holds it, which may be class 0 (inline)", because opposite adds and divisions can drop several classes.

**F13. Minor. §3 Result class, class 31.** An error on a predicted class of 32 can reject a result that would fit class 31 (class 31 × class 0). *Fix:* say that the error is raised when the predicted class exceeds 31. That is acceptable under optimization rule 2, and no job comes near it.

**F14. Minor. §3 Pools.** The first capacity, 16 slots, is absurd for high classes: at class 31 it is 256 GiB. Pools are also created on first use. *Fix:* "A pool is created at its first allocation." Also name the free list's end marker (for example, all ones).

**F15. Minor. §1 and §2, Control's header.** §1 says class 0 means "the word is the magnitude", but Control puts a pointer in that word. *Fix:* "In Control the class bits are unused and the word is a pointer to Growing's block."

**F16. Minor. §1 Directions, adding zero to ±i.** Zero has d = 0, so 0 + i would be reported as a cross-axis error. This is unreachable with integers, but the axis test should treat a zero operand as having no axis. One clause fixes it.

**F17. Minor. §5 peak bytes.** The spec should say whether a pool's doubling (old and new blocks both live during the copy) counts in the accounted peak, or only in the separately reported pool capacity. I suggest the latter.

## 3. Checks that pass

- **Direction rules.** The opposite-direction rule d1 + 2b is correct in all four sign cases. Equal magnitudes give borrow 0, a zero magnitude, and then header 0. Adding zero works through the opposite-direction branch. Division gives only 0 or 2 on integers.
- **Class predictions.** All are sound bounds: same-direction add L + 1, opposite add L, multiply L + 1 (at most 2^(L+7) bits), division c1, and gcd min(c1, c2), or the other operand's class when one operand is zero.
- **Indices and pointers.** "Allocate the result slot before taking operand addresses" correctly handles reallocation. A move-down allocates in a different pool, so the predicted slot's pointer stays valid.
- **Fidelity to the agreed design.** The 8-bit header and 64-bit word in parallel lists, unsigned words only, the 5/2/1 header split, slot indices into equal-width pools, all the direction rules, zero with direction 0, the CPU only, and Control as Growing all match. Only W = 32 departs from it (F8).
- **Match with spec 12.** The timing method, flags, 1024 operations, Control run twice, the max(5%, noise) threshold, the scalar job 1 and callgrind all match spec 12 §4 and its README. Hybrid was measured in spec 12's rounds 3 to 5, as claimed.

## 4. The spec's open questions

- **A third build, Inline.** Recommend no. The author dislikes extra builds, and the agreed design names two. Header × Control in job 1 already shows mostly the inline word (F9 corrects the "no pool" claim).
- **The mixed job's rate (15/16).** This is the author's call. If the author does not set it, recommend dropping the mixed job. Mixed classes already arise in division and gcd (the factors are at half size), and after F5 nothing in the code is specific to mixed classes. If the job is kept, 15/16 is a reasonable placeholder.
- **Whether pools shrink.** Recommend never, as the spec says. Shrinking would solve a problem that no job has (optimization rule 2), and it would make indices harder to keep valid.
- **Two parallel lists and rule 4.** The draft does not list this question in §6, although it was raised. Recommend counting them as one stored structure. They hold one record per number (a header and its word), stored as two lists so that the byte is not padded, which is a layout of one list and not a second structure. Rule 4 says that more than one structure needs discussion, so the spec should say this in one line under Structures and leave the final word to the author.
