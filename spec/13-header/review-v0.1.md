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

## Second pass

Nov 1, 2026, on the revised v0.1 (905 words, against 794 for spec 12). Nothing was built or run.

**Verdict: ready after minor fixes.** All three blocking findings are fixed, and the header formulas are correct on 8 bits. What remains are three small should-fix items. Two of them came in with the revision: the claimed rounding cost no longer matches the real-length operands, and the refactor's "one extra word" does not fit a slot. The third, a result predicted to be class 0, was in the first draft and I missed it in the first pass.

### Findings F1 to F17

| Finding | Status | Note |
| --- | --- | --- |
| F1 header add | resolved | Checked on 8 bits with `D = 0xC0`. Multiply: 0x80 + 0x80 = 0x100, and the carry falls off to give d = 0. Divide: 0x40 - 0x80 = 0xC0 mod 256, so 1 - 2 = 3. Opposite add: `b shl 7` = 0x80 adds 2. The class bits stay 0 until the result class is ORed in. |
| F2 signed compare | resolved | Zero against a negative falls under "directions differ", and zero against a positive falls under "both 0". Both are correct. Nothing says what happens for d = 1 or 3, but integers never produce them (minor). |
| F3 gcd with zero | resolved | Real lengths, and length 0 for zero, so Growing's early return fires. |
| F4 register path | resolved | Moved to special case (a). |
| F5 widening | resolved | Nothing is widened, and special cases (a) and (c) of the first draft are gone. |
| F6 refactor | mostly resolved | The borrow, the destination and gcd's bound are stated. See N2 for the wording "room for one extra word". |
| F7 correctness check | resolved | §5 Correctness. |
| F8 W = 32 | resolved | Nothing runs at W = 32. |
| F9 job 1 uses pools | resolved | §5 Known costs says so, and comparisons are Header × Control only. |
| F10 jobs and length | resolved | Five jobs, add once, one pairing. The spec is now 14% longer than spec 12, close enough. |
| F11 growth job | resolved | It is now a workload, not a job. |
| F12 move-down target | resolved | "The smallest class that holds it (possibly 0)". |
| F13 class 31 | resolved | A predicted class above 31 is an error. |
| F14 first capacity | resolved | One slot at first use, then doubling, and the free list ends with all ones. |
| F15 Control's header | resolved | §2. |
| F16 zero and axes | resolved | "Zero has no axis". |
| F17 peak and doubling | resolved | Reported apart. |

### New or remaining findings

**N1. Should fix. §5 Known costs, rounding up.** "To 2× on add, 4× on multiply" was true while operands were widened to the class width. Now that the routines read operands at their real length, rounding adds no arithmetic work. Its costs are memory, up to 2× the bytes per value, plus the scan for the top nonzero word. Because the form is canonical, that word lies in the slot's upper half, so the scan covers at most half the slot. *Fix:* "Rounding up: up to 2× bytes per value, and the scan for an operand's top word. Read from bytes per value and callgrind." The sentence in §4 that rounding "shows against Control" is then about memory, which is still true.

**N2. Should fix. §2 Shared routines, "room for one extra word".** Slots have exactly their class width, so a Header destination cannot always have a spare word. No routine needs one, provided each writes only up to its bound. Each bound fits the predicted class:
- add writes at most max(n1, n2) + 1 words, which fits class L + 1;
- multiply writes n1 + n2 words, at most 2^(L+1);
- absolute difference writes max(n1, n2) words, within class L;
- division writes n1 - n2 + 1 words, at most n1, within class c1;
- gcd writes at most min(n1, n2) words.

*Fix:* "Each writes at most its bound (add max + 1, multiply n1 + n2, absolute difference max, division n1 - n2 + 1, gcd min) into a destination of at least that size, and returns the result's length." The returned length is also what the move-down needs.

**N3. Should fix. §3 Result class, a predicted class of 0.** Division with c1 = 0, gcd with a class-0 operand, and opposite add of two class-0 values all predict class 0, which has no slot. The spec says "allocate that slot". This was already in the first draft, and the first pass missed it. *Fix:* "A predicted class 0 writes into a one-word local, and the result goes into the inline word." Under rule 4, that word is the function's one local, or Growing's buffer is, in division and gcd.

**N4. Minor. §3 Result class.** "Then move the result ... and release the slot" reads as though it always happens. *Fix:* add "if it fits a smaller class".

**N5. Minor. §4 Jobs, k = 1.** Lengths from 33 to 64 bits replace spec 12's 1 to 63, so job 1 no longer has small values. This is consistent with the other jobs and needs no change unless the author wants small values.

### Open questions (§6)

The four entries match the first pass: no Inline build, pools never shrink, the parallel lists count as one stored structure, and the mixed job is dropped unless the author sets a rate. I agree with each recommendation.

## Third pass

Nov 1, 2026, on the revision with two designs, A and B, and two rounds (1289 words, against 794 for spec 12). Nothing was built or run.

**Verdict: needs revision, but small.** B's rules are correct: collapse, zero, negation and the L + 2 prediction all hold. There is one blocking item, which a one-line fix settles: the shared multiply-accumulate bound now lets A's multiply write one word past its slot. Four should-fix items remain. Three are design points:
- the timed builds are hand machine code, while Control is compiled Nim;
- round 1's four-slot B model is not specified;
- B's collapse adds temporaries that rule 4 counts.

The fourth is length: about 400 words can go.

### Second-pass items

N1 (rounding cost), N3 (predicted class 0) and N4 (move-down wording) are resolved. N2 (bounds) is resolved for every routine except multiply-accumulate (T1). N5 is unchanged and stays minor.

### B's rules checked

- **Value and negation.** P − N is preserved by add, (P1 + P2) − (N1 + N2). Negate is a swap. Multiply holds because (P1 − N1)(P2 − N2) = (P1P2 + N1N2) − (P1N2 + N1P2).
- **Collapse.** When P = N, |P − N| = 0 with borrow 0, so the result goes to P and N becomes 0, giving (0, 0). When P ≠ N, the nonzero part is the magnitude and the borrow picks its side. So collapse never produces a negative zero: the only collapsed zero is (0, 0), class 0 in both bytes. Uncollapsed zeros such as (5, 5) exist, but compare, division, gcd and output always collapse first, so none of them is ever seen.
- **Add prediction L + 1.** This is sound: two parts of class at most L sum to at most 2^L words plus one bit.
- **Multiply prediction L + 2.** This is sound and is the smallest class bound. Each product fills at most class L + 1 exactly (2^(L+1) words), and the sum of two such products can carry one more bit, so the next class is L + 2. Two class-0 inputs give at most 2 words plus a bit, which is class 2. The cost is that almost every B multiply allocates two slots four times too wide and moves both down. That is the honest general cost and is already listed among the known costs (T7 suggests a cheaper general prediction).
- **Collapse prediction max(c_P, c_N).** This is sound.
- **Compare on uncollapsed operands.** Collapsing both operands and then using A's rule is correct, because each collapsed operand has direction 0 (P) or 2 (N).
- **Division and gcd.** These collapse their operands, then follow A's direction rules: the quotient goes to P when d1 − d2 = 0 and to N when it is 2, and gcd goes to P. Correct. A divisor such as (5, 5) collapses to zero, so the rule "divisors are nonzero" must hold for the value, not just for the parts. The generator gives only collapsed divisors, so this cannot arise.

### Findings

**T1. Blocking. §1 Routines, the multiply-accumulate bound against A's multiply.** "Multiply-accumulate writes at most n1 + n2 + 1" is right for B, whose class L + 2 has room. A multiplies through the same routine but predicts L + 1, whose slot is exactly n1 + n2 words when both operands fill class L. The extra word is a carry that is always 0 when r starts at 0, but writing it overwrites the first word of the next slot in the pool. The bound is also incomplete for B's second accumulate, where r already holds up to n(P1) + n(P2) words. *Fix:* "multiply-accumulate writes at most max(n_r, n1 + n2) words, plus a final carry word only when it is nonzero, so r = 0 writes n1 + n2". A's L + 1 and B's L + 2 both fit then.

**T2. Should fix. §5 Round 2, machine code against compiled Control.** A and B are to be written as x86-64 machine code, while Control is Growing in Nim compiled by gcc. A comparison of A or B with Control would then mix the design with the change from compiler to hand-written code, so it would not measure one idea. *Fix:* build Control the same way, as machine code from the same portable pseudocode, or keep all three in Nim. This sentence also limits the timed builds to x86-64 only, although rule 6 is about portability. That is fine for timing on this machine, but the spec should say so.

**T3. Should fix. §4, B's four-slot model.** This model is a third representation: four magnitudes for 1, i, −1 and −i, checked against Gaussian integers. The spec never defines its header, its classes or its collapse result. It is outside the agreed scope (integers, with complex values left to a later spec), and only round 1 uses it. A builder would have to invent it. *Fix:* drop it, and A's four-axis tests too. Integers use only d = 0 and 2, and A's cross-axis error needs only one test line. If the author wants a complex check, that belongs in the later complex spec. This removes about 130 words.

**T4. Should fix. §3 Structures, temporaries from collapse (rule 4).** Values are immutable, so collapsing both operands of a compare, a division or a gcd creates up to two new values, each possibly holding a slot. Together with the predicted result slot, that is up to three temporary structures in one operation, and rule 4 allows one. Open question (e) covers only the stored side. *Fix:* add this to (e) for the author. Two simpler options follow the rules more closely:
- compare without collapsing, as P1 + N2 against P2 + N1, which also keeps B's rule of never subtracting outside collapse (two adds, but each sum can live in the one local buffer, used in two halves);
- collapse into the operation's one local buffer, as division and gcd already copy their operands into Growing's buffer.

The second option needs no new slot at all and is the general one I recommend.

**T5. Should fix. Length.** At 1289 words the spec is 62% longer than spec 12. The cuts, which together come to about 400 words:
- T3 (about 130 words);
- the details of §5 Round 2: the spec says "details wait for round 1", so round 2 needs only Control, the builds and "A × Control, B × Control", about 100 words;
- open questions (a) and (d), already answered or dropped (the mixed job), about 40 words;
- the parentheticals in the round 1 counts (pair counts, coordinator's-call markers) folded into one line;
- the spec's version history in the opening line.

**T6. Minor. §4 Round 1 coverage.** Round 1 covers every operation of both designs, including collapse, negate and release, with pool checks. Two gaps remain:
- the exhaustive uncollapsed B set keeps both parts below 2^4, which is class 0 at W = 4, so uncollapsed parts above class 0 are covered only by the random set. Say that random B operands are uncollapsed, with each part drawn on its own.
- the pass test checks canonical form only for collapsed B results. It should also check that each part of every B result is in its smallest class, since move-down applies to every part.

**T7. Minor. §1 Result class, predicting from lengths.** The routines already know each operand's real length, so predicting the class from lengths rather than from classes is equally general. For example, a B product part gets the smallest class holding max(n(P1) + n(P2), n(N1) + n(N2)) + 1 words. That would avoid most of B's four-times-too-wide slots and the move-downs that follow, without adding a special case. This is for the author to weigh, and it could wait for round 2.

**T8. Minor. §3 B result class wording.** "L the largest class among the inputs" should say "among that part's inputs": both parts of an operand for multiply, and only the matching parts for add.

### Rules check (algorithm design), for the revision

| Rule | Status | Reason |
| --- | --- | --- |
| 1 Exact | holds | Both designs are exact, and a prediction above class 31 is an error. |
| 2 General before special | holds | Special cases (a) and (b) are named and not built. |
| 3 Shared solutions | holds | One routine set in §1 serves A, B and Control. |
| 4 One structure each | needs discussion | Stored: B holds two slots per number (open question e). Temporary: collapse of two operands adds up to two more (T4). A's parallel lists are open question (c). |
| 5 Self-contained | holds | Nothing outside the project is used. |
| 6 Portable pseudocode | holds, with a note | Borrows are values (a < b), and the 64 × 64 → 128 product is common to all three families. The timed builds are x86-64 only (T2). |

### Open questions

- **(a) Inline build and (b) pools shrinking.** I agree with no and never.
- **(c) A's parallel lists.** I agree that they are one structure.
- **(d) Mixed job.** Drop it unless the author sets a rate.
- **(e) Two slots per B number.** I agree that it should be allowed, since B cannot exist without it. The author should also be asked about collapse temporaries (T4).
- **(f) Collapsing beyond the required points.** I agree: never in this round. Parts growing until collapse is the thing being measured.
