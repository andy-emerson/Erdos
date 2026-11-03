# 13-header round 2: the Nim builds

Nov 1, 2026. The builds of [spec v0.1](../v0.1.md) §5: Control, Design A, A's branch-free row and Design B, in Nim, integers only, with the benchmark driver, the correctness check and the timing harness. Nothing is committed. No timed round has been run, only a smoke run of a few seconds.

## Files

| File | What |
| --- | --- |
| `routines.nim` | §1 Routines, shared by all builds: add, multiply-accumulate, absolute difference with its borrow, compare, Hensel exact division, binary gcd. They are Growing's (spec 12 `growing.nim`), refactored to write into a given destination at most their bound and to return the length. Also the one-word `imax`/`imin` and the working buffer |
| `pools.nim` | §1 classes, pools, free lists, result slots, the move-down and the zeroing of the rest of a slot. Shared by A and B |
| `control.nim` | **Control**: Growing's malloc'd blocks (a length word, then the words), with A's header list. The class bits are 0, the direction bits are A's, and the word points to the block |
| `designa.nim` | **Design A** (§2), and with `-d:branchfree` **A's branch-free row** (§5 c) |
| `designb.nim` | **Design B** (§3) |
| `bench.nim` | the driver, one binary per build: modes `dump`, `sum`, `calib`, `time`, `mem`, and the growth workload as operation `growth` |
| `alloc.nim` | spec 12's malloc/free with byte accounting in the memory build (a copy) |
| `build.sh` | builds 8 binaries into `../build/bin/` (`control a abf b`, each also as `-mem`) and counts conditional moves |
| `../test/r2gen.py` | the operands: 45 timing jobs and the growth job |
| `../test/r2check.py` | the correctness check, against round 1's models at W = 64 |
| `../test/r2run.py` | the timing harness, spec 12's method |

## Build, check, run

```sh
NIM=/path/to/nim-2.2.14/bin/nim sh src/build.sh    # Nim 2.2.14 and gcc 13.3.0, as in spec 12
python3 test/r2check.py                            # the check: about 20 s, prints PASS or FAILED
python3 test/r2run.py --toy --jobs k1-srand-orand --ops add   # a smoke run of the harness: not a measurement
python3 test/r2run.py [--ir]                       # the real round: a quiet machine and the author's go-ahead only
```

Run these from `spec/13-header`. Outputs go to `build/` (now in `.gitignore`):
- `build/r2/work`: the job files;
- `build/r2/check`: the check's dumps;
- `build/r2/results/report.md` and `results.json`: the round's results (`build/r2/toy/` for a toy run).

Driver modes, with `<op>` one of `add sub neg mul div gcd cmp`, or `growth` with `work/growth.bin`:
- `bin/<b> dump <job> <op> <out>` runs two passes, so results are released and their slots reused. It writes the second pass's results raw, then checks that every slot is free after everything is released.
- `bin/<b> sum <job> <op>` runs one pass and prints the checksum of the values.
- `bin/<b> calib|time ...` time as in spec 12.
- `bin/<b>-mem mem <job>` prints bytes per value.

The check had these results on Nov 1, 2026:
- **Edge cases:** 6,798 results per build. They cover zeros in directions 0 and 2, equal magnitudes, lengths from 1 to 32 words, all-ones words whose carries leave the top word, and B's parts uncollapsed with all-ones bases.
- **Random:** 28,672 results per build, up to 8 words. Zeros and equal magnitudes each come at 1/16, and half of B's operands are uncollapsed.
- **Four directions:** 2,768 results each for Control, A and A's branch-free row.
- **Errors:** 35 error cases. gcd(0, 0) is tested in all four direction pairs and with B's uncollapsed zeros; sums and compares are tested across axes.
- **Model B's stale pointer:** the case in Finding 1 below, checked against exact values.
- **Growth workload:** replayed in the models.
- **Timing jobs:** 1,260 checksums (45 jobs × 7 operations × 4 builds) against Python's exact values.

All passed. A, the branch-free row and B match the models exactly: the header (a zero's direction too), the word or slot index, and the whole slot. Control matches model A's direction and every value.

## Choices

Each choice below is the builder's, made where the spec is silent or open. Each is for the author to confirm.

1. **ctz is gcc's builtin, as in Growing.** It is used only in division and gcd, to shift out trailing zeros. It is not one step on every family:
   - x86-64 has TZCNT or BSF, one instruction;
   - ARM64 needs RBIT and CLZ, two instructions;
   - RISC-V has CTZ only in Zbb (RVA22 and RVA23), not in RV64GC.

   This is a rule 6 question for the author, like conditional moves. The class of a length is computed by a short loop, so no CLZ is needed. The 64 × 64 → 128 product (x86-64 MUL, ARM64 MUL and UMULH, RISC-V MUL and MULHU) needs RISC-V's M extension, which RV64I, the base, also lacks.
2. **Keeping gcc to the pseudocode.** Every build has the same flags. They are spec 12's flags plus these:
   - `-fno-tree-vectorize` everywhere, not only for job 1;
   - `-fno-if-conversion -fno-if-conversion2 -fno-ssa-phiopt`;
   - `max` and `min` of lengths written as branches (`imax`, `imin`), because gcc turns them into conditional moves otherwise;
   - one mask in the branch-free compare built from a shift, not from `0 - (a == b)`, which gcc also turned into a conditional move.

   `build.sh` counts conditional moves in the builds' functions, and there are none. gcc may still render a carry computed as a value (`t < s`) as x86-64's `setb` or `adc`. The disassembly table of §5(a) should show which it does.
3. **A's borrow is a branch, as in the notes' tree.** Control and B's collapse use the same routine, which negates the difference only when there is a borrow. Growing's `opAbsdiff2` always ran its mask pass. The values are the same either way.
4. **A's branch-free row.** Add and subtract run in one loop: r = m1 + (m2 XOR s) + o, with o = 1 for opposite directions and s = 0 − o. The steps after the loop:
   - The carry out is the top word in the same direction, and 1 − b in the opposite direction.
   - The top word's store goes to a sink through an address mask when the directions are opposite, because the slot then holds only max words.
   - The negation pass always runs, with m = 0 − b: (r XOR m) + b.

   Compare turns its zero and sign tests into values (sign in −1, 0 or 1) and combines them with AND and XOR masks.

   These branches are kept:
   - the axis check, which integers never take;
   - the loops over lengths, and the magnitude compare's loop;
   - the class structure: an inline word or a slot, and the move-down.

   Multiply, divide, negate and gcd have no sign branch, so they are A's.
5. **The rest of a slot is zeroed before the move-down decision.** §1 and the model say so. When the result moves down, that zeroing is wasted. Zeroing only when the result stays would be a special case, so it is named here and not built.
6. **Control is Growing refactored as the review's F6 asks.**
   - Its routines are the shared ones, each writing into a block malloc'd at the §1 bound.
   - Division writes exactly n1 − n2 + 1 words.
   - gcd's one-word special case from spec 12's round 2 is not built in any build (rule 2), as in round 1.
   - A zero still has a block, of length 0.
   - Negate copies the block.
7. **The timing jobs.**
   - **Magnitudes:** k words have bit lengths uniform in 64(k−1)+1 .. 64k, and job 1 runs from 1 to 64 bits. Division and gcd operands follow spec 12 §3. Spec 12's powers of 2 at 1/16 were for its flag builds and are dropped.
   - **Sign mixes:** each operand is negative with probability 0 (all positive), 1/16 (mostly one sign) or 1/2 (random).
   - **Magnitude-order mixes:** |a| ≥ |b| in all pairs, in 15/16 of them, or in 1/2, swapping pairs to fit. Division keeps a = bq, and negate has one operand, so the order mixes leave them as drawn.
   - **The cross:** every sign mix with every order mix, 9 jobs per k and 45 in all. The magnitudes are the same in all nine jobs of a k, so the mixes alone differ.
   - **B's operands** are collapsed in the timing jobs, as the integers are given. The check uses uncollapsed ones too.
8. **The growth workload.** x0, y and z are one word each, with bit length uniform from 1 to 64 and each sign at 1/2. x grows to about 33 words. Time is reported per step, which is one multiply and one add. B's parts stay uncollapsed through the chain, since no collapse is required there.
9. **Bytes per value** are the stored lists' share (A: 1 + 8 bytes; B: 2 + 16) plus the slots. Control's is 1 + 8 plus its block, accounted, with malloc's usable size beside. The pools' total capacity is printed beside as `poolbytes`. No peak is measured, since §5 asks only for bytes per value.
10. **Errors stop the binary (exit 3).** These are a sum or compare across axes, a class above 31, a full pool and gcd(0, 0). Nim exceptions would add a test after every call in the timed code. Division by zero is not tested for: the operands exclude it, as in Growing.
11. **The check runs round 1's models at W = 64.** It sets `words.py`'s globals at import. Round 1's files are unchanged.

## Findings

1. **Round 1's model B can read a stale pointer.** B's add, subtract and multiply take both parts' slot pointers before finishing either part. Finishing the first part can move it down into the second part's class and grow that pool. The model then zeroes the rest of the second slot in the old, discarded memory, and the new slot keeps an unwritten word. At W = 4, (0xFFF0, 0xFF) + (0x1, 0xFF) gives 3571, not 65011. Round 1's sets never reached this case.

   The Nim build takes pointers afresh after each allocation, the move-down's included. `r2check.py` runs the same case at W = 64, checks B against the exact value, and reports how far the model is off. The spec's sentence "Result slots are allocated first, since growth moves pointers but not indices" may want a clause saying that a move-down's allocation moves pointers too.
2. **The spec leaves some rates and sizes open.** These were filled in by choices 7 and 8:
   - how much "mostly one sign" means;
   - what the magnitude-order mixes are, and whether they cross the sign mixes;
   - the growth workload's operand sizes.
