# 13-header v0.2: the builds

Nov 1, 2026. The builds of [spec v0.2](../v0.2.md) for §7's timed trials: Control, Design A, A's branch-free row and Design B, one word per magnitude, for integers and for complex numbers on the axes, with the driver, the correctness check and the timing harness. Nothing is committed. No timed trials have been run, only a smoke run of a few seconds.

## Files

| File | What |
| --- | --- |
| `pseudo.nim` | shared: `Word`, the error exit, and `opaque` (an empty asm barrier) |
| `control.nim` | **Control** (§2): `int64`; complex numbers as a pair `(re, im)` |
| `designa.nim` | **Design A** (§3): the notes' tree (`aAdd`), and **A's branch-free row** (`aAddBF`); negate, multiply and divide are shared by both |
| `designb.nim` | **Design B** (§4): integers `(P, N)`, complex numbers four slots; add, subtract, negate, multiply, collapse, exact division |
| `bench.nim` | the driver, one binary per build (`-d:bld=control\|a\|abf\|b`). Modes: `dump`, `sum`, `calib`, `time`, `bytes` |
| `build.sh` | builds the four binaries into `../build/bin/`, writes their loops' disassembly to `../build/asm/`, and runs the rule 6 gate |
| `../test/gen.py` | §7's 72 rows (operands and exact checksums) and the job file format |
| `../test/check.py` | the correctness check against exact answers (Python integers) |
| `../test/asmcheck.py` | the rule 6 gate and the branch inventory of each timed loop |
| `../test/run.py` | the timing harness, spec 12's method |

## Build, check, run

Run these from `spec/13-header`:

```sh
NIM=/path/to/nim-2.2.14/bin/nim sh src/build.sh   # Nim 2.2.14 and gcc 13.3.0, as in spec 12; ends "asmcheck: PASS", "built"
python3 test/check.py                             # about 3 s; prints PASS or FAILED
python3 test/asmcheck.py -v                       # each timed loop's element loop, instruction by instruction
python3 test/run.py --toy --rows int-add-rand-half,cx-div-rand   # a smoke run: not a measurement
python3 test/run.py [--ir]                        # the trials: a quiet machine only (RULES.md, optimization 4)
```

Outputs go to `build/`, which is in `.gitignore`:
- `build/work/`: the 72 row files;
- `build/check/`: the check's job files and dumps;
- `build/asm/<b>.s`: the loops' disassembly;
- `build/results/report.md` and `results.json`: the trials' results (`build/toy/` for a toy run).

Rows are named `<kind>-<op>-<sign>[-<order>]`:
- kind is `int` or `cx`;
- op is `add`, `sub`, `neg`, `mul`, `div` or `collapse` (B only);
- sign is `pos` (all positive), `most` (each operand negative at 1/16) or `rand` (at 1/2);
- order, for add, sub and collapse, is `ge` (|a| ≥ |b| always), `most` (in 15/16 of pairs) or `half`.

That gives 9 + 9 + 3 + 3 + 3 + 9 = 36 rows per kind, 72 in all. Each row times all four builds and the first build again (Control; for collapse, B), whose difference is the noise.

Driver modes (`build/bin/<b> <mode> ...`):
- `dump <job> <out>` runs one pass and writes each result's stored form: Control the integers, A the header and word in hex, B the parts in hex.
- `sum <job>` runs one pass and prints the checksum.
- `calib <job> [min_ns]` and `time <job> <r> <reps>` time the row as in spec 12. `time` ends with the checksum and the bytes per value.
- `bytes` prints the bytes per value.

The checksum is over values, `(re < 0, |re|, im < 0, |im|)`, so every build that computes the same values gives the same checksum, whatever the form (a zero's direction, B's uncollapsed parts). Exit 3 is an error of the spec. Exit 4 means the build has no such operation (collapse is B's).

## Check results (Nov 1, 2026)

`python3 test/check.py` passes:
- **Results:** 395,327, every one equal to the exact value. There are three sets for each kind and each operation:
  - *ctl:* every magnitude and result part below 2^63, run in all four builds;
  - *wide:* magnitudes up to 2^64 − 1 with no result overflowing a word, run in A, the branch-free row and B;
  - *bun:* B with uncollapsed operands, both parts nonzero.

  Each set has edge cases and 4,000 random cases. The edge cases are every pair from {0, 1, 2, 3, 2^32 − 1, 2^32, 2^32 + 1, 2^62 ± 1, 2^62, 2^63 − 2, 2^63 − 1}, plus {2^63, 2^63 + 1, 2^64 − 2^32 − 1, 2^64 − 2, 2^64 − 1} in the wide set. They are taken in every direction pair: zeros in every direction, equal magnitudes, and sums landing exactly on 2^63 − 1 or 2^64 − 1.
- **Form, beyond the value:**
  - A's header has bits 0–5 at 0 and is exactly the header §3 gives, so a zero's direction is checked too;
  - B's parts are exactly §4's: add, subtract, negate and multiply part by part, collapse with one part of each axis 0, and division with the quotient in slot (d₁ − d₂) mod 4.
- **Errors:** 64 cases, all exit 3 in A and in the branch-free row. They are a complex add or subtract of two nonzero numbers on different axes, in every such direction pair, alone and after five valid pairs.
- **Timing rows:** 234 checksums (72 rows; collapse in B only) all equal the checksum of Python's exact values. Control and A exit 4 on collapse.
- **Bytes per value:** Control 8 (16 complex), A 9, B 16 (32 complex).

The check was itself checked by building two broken copies, then deleted:
- the branch-free row without its negation;
- A with a zero across axes keeping its own direction.

The check caught both: 4,098 failures. The second showed only through the header check, since a zero's value does not change.

## Disassembly (gcc 13.3.0, `build/asm/`, `test/asmcheck.py`)

The gate at the end of `build.sh` checks every timed loop's element loop (one operation per pass). It passes:
- no conditional moves and no vector instructions in any build;
- no conditional branch of the operation's own in the branch-free row.

| build | operation | conditional branches in the element loop (besides the loop's own) |
| --- | --- | --- |
| Control | int: all; cx: add, sub, neg, mul | 0 |
| Control | cx div | 1, which axis the divisor is on (`test; jne`) |
| A | add, sub | 5 on the static paths: `t ≠ 0` (`and $0xc0; jne`), `t = 0x80` (`cmp $0x80; je`), the borrow (`cmp; jb`), and on different axes `m1 = 0` and `m2 ≠ 0`, before the error call. An integer runs 1 (same direction) or 3 (opposite) |
| A, abf | neg, mul, div | 0 |
| abf | add, sub | **0**; four `setcc` (`setne`, `setne`, `setb`, `sete`) |
| B | add, sub, neg, mul (int, cx) | 0 |
| B | collapse | 1 per axis, the borrow (`cmp; jae`); gcc duplicated the imaginary axis's test onto both real paths, so cx shows 3 static and runs 2 |
| B | div | 1 per collapse, the borrow (`setb` and `jb`/`jae` on the same compare): int 2, cx 4 |

**A's borrow.** The source computes `d = m1 - m2` and `b = Word(m1 < m2)`, then negates `d` when `b = 1`. gcc emitted `cmp m1, m2; jb`: a branch on the carry flag of the compare. Each arm then does its own subtraction: `m1 - m2`, or `m2 - m1` in place of negating, so there is no `neg`. The borrow is never a value. The opposite-direction path is `cmp, jb, mov, sub`, plus the header's `and` and `add`, not the pseudocode's `sub, borrow, branch, neg`. B's collapse is compiled the same way. B's division keeps the borrow as a value (`setb`), since its direction rule uses it, and branches on the same compare. §6's counts should be checked against these paths.

**A's branch-free row.** gcc kept it branch-free with no barrier needed:
- the borrow is `cmp; setb`;
- the zero tests of the different-axes case are `test; setne/sete`;
- the masks are `neg`, `and` and `xor`;
- there is no `jb`, no `cmov` and no `adc`.

The element loop is 50 instructions for add and 51 for subtract, which turns `h2` with one more `add $0x80`. gcc spilled `n` to the stack, so one loop compare reads memory. It also loads `h1` twice, once more for the direction-select `xor`. Both are register pressure, not branches.

**What had to be fixed to get there:**
1. **Array and struct results.** At first B's operations returned arrays, and Nim's copy of a 16- or 32-byte array became SSE `movdqa/movups`. Complex negate even became two 16-byte loads. Every operation in every build now writes its result's words one by one into the result list (`r: var ...`), and the loops have no vector instructions.
2. **A's error call.** A returned a tuple, and its error call could raise. Nim then zeroed a 16-byte stack temporary per element (`pxor/movaps`), and checked the exception flag through `%fs` after the call. A now writes its outputs directly, and the error exit uses C's `write` and `exit`, so it raises nothing.
3. **B's complex division.** gcc turned `x and (0 - Word(i != 0))`, the mask that picks the imaginary axis's direction, into `test; cmove`. The fix is `opaque` (spec 12 divbench2's empty `asm("" : "+r"(y))`) on the compare's value. gcc then cannot see that the value is 0 or 1, and emits `setne; neg; and`. This is the only barrier in the builds.

Flags: spec 12's (`-d:danger --mm:arc -d:useMalloc --panics:on`, gcc `-O2 -fno-strict-aliasing`) plus `-fno-tree-vectorize -fno-if-conversion -fno-if-conversion2 -fno-ssa-phiopt`, as in v0.1.

## Choices

Each choice below was made where the spec is silent, or minimally where it is open. Each is for the author or coordinator to confirm.

1. **Only A tests for an expression.**
   - §3 says two nonzero operands across axes are an error in A's add, and A exits 3.
   - Control's pair and B's four slots hold such a sum without a test, as the notes say B does, and §2 and §4 give them no error.
   - §7's operands never form one: a nonzero pair across axes has one operand replaced by a zero.

   The check gives Control and B no such case.
2. **The branch-free row also masks the different-axes case.** "The direction and borrow branches replaced by masks" is read to include the axis branch, since complex rows take it at random. A zero first operand takes the second's direction through a mask, and the add runs as same-direction.

   Its error, two nonzero operands, is ORed into a sticky word. The driver tests that word after each pass, outside the operation, and exits 3, so the operation has no branch at all. The result written for the faulty pair is meaningless, but the run stops.
3. **A's code is the same for integers and complex numbers**, as the header does not distinguish them. So A's complex rows run the same loop functions (`loop_int_*`) on complex operands.
4. **B's division finds a direction from four slots.** After collapse, the imaginary axis is taken when its magnitude is nonzero. That is a compare written as a value, then a mask, not a branch. The magnitude is the OR of the two axes' magnitudes (one is 0).

   The quotient goes to slot (d₁ − d₂) mod 4 by an indexed store, after all slots are zeroed: two stores for an integer, four for a complex number, then one more. §4 does not say how the quotient is placed. Integers use part d/2 of (P, N).
5. **B's collapse is a branch on the borrow**, "as in A's add". gcc compiled it as A's, a compare and a branch with one subtraction per arm.
6. **Control's complex division** tests which axis the divisor is on (`b.im == 0`), a branch. It then divides both parts by the divisor's nonzero part, two signed divides: `(ar/br, ai/br)` or `(ai/bi, −ar/bi)`. The two products in each part of the complex multiply are the machine's, in full.
7. **The timing magnitudes.** v0.2 sets no sizes, and Control must not overflow:
   - add, subtract and negate draw bit lengths uniform in 1..62, so a sum stays below 2^63;
   - multiply and divide take a factor of L₁ bits (uniform 1..62) and one of L₂ bits (uniform 1..63 − L₁);
   - division's dividend is the product, so every product and quotient is below 2^63.

   The timing rows have no zero operands except the complex replacements. Magnitudes near 2^64 are in the check only, since Control cannot hold them.
8. **The mixes.**
   - **Sign:** each operand is negative with probability 0, 1/16 or 1/2 (v0.1's reading).
   - **Order:** the pair is swapped to fit. It applies to add and subtract, and to collapse, whose rows collapse the B sums of the add rows' pairs with the same mixes.
   - **Complex:** each operand's axis is uniform. For add and subtract a nonzero pair across axes has one operand, chosen at random, set to zero (keeping its direction), before the order swap. Multiply and divide across axes are valid and kept.

   Negate, multiply and divide have three rows each, one per sign mix. B's operands enter collapsed (§7). Collapse's own operands are B sums, which are uncollapsed in general.
9. **Each timed operation is inlined into its loop** (spec 12's word builds did the same). The disassembly of a loop is then the operation's machine code, plus 3 to 6 loop instructions: loads, stores, index, compare and branch. A's lists are parallel: `uint8` headers and `uint64` words, 9 bytes per value. Control and B keep one list of records.
10. **Noise row.** The harness repeats Control as a fifth row, as in spec 12. Collapse has only B, so it repeats B.
11. **Placement.** The Python tests and the harness are in `spec/13-header/test/` (spec 12's and v0.1's layout), and the builds are in `src/`.

## Problems and open points

- **§6's counts and gcc.** For A's opposite-direction add and B's collapse, gcc's machine code (compare, branch, one subtraction per arm) is shorter than the pseudocode as written (subtract, borrow, branch, negate). If the counts in §6 are for the pseudocode, the table that maps steps to instructions will not match the x86-64 disassembly on that path. Either gcc has to be kept to the pseudocode there too, for example with `opaque` on `d`, or the pseudocode can take gcc's form, which RV64 and ARM64 also have (`sltu` or `cmp` then branch). This is for the coordinator. Nothing was changed for it.
- **Setcc is a value step.** The branch-free row and B's division use a compare written as a value: `setb`/`setne`/`sete` on x86-64, `cset` on ARM64, `sltu`/`snez`/`seqz` on RISC-V. Rule 6 allows it, since all three families have it. On x86-64 it takes two instructions, an `xor` to clear and the `setcc`, or a `movzx`.
