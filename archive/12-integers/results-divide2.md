# Exact division: three methods and their combinations, against control

Program: `src/divbench2.nim`. Raw output: `results-divide2-raw.txt` (row, set, ns per division, spread, checksum).

Every row divides two dividends by the same divisor, as a fraction's numerator and denominator are divided by their
gcd. Time is per division. Each row runs over the same 1024 operand triples: reps of at least 50 ms, 15 reps, the
median, on one pinned core of a quiet machine. Every row gives the same checksum per set, and `divbench2 check`
compares every row against the exact quotient on 1,474,560 divisions (3 sets × 30 seeds × 8 rows × 2048).

Control is the type's own division: uint64 `div` for dividends below 2^32 and 2^64, and the compiler's
`unsigned __int128` division for dividends below 2^128. Divisor and quotient are each below half the size, so a
quotient always fits 64 bits. Some divisors are even.

- **table**: inverse from the 8-bit start table (128 bytes, a constant) and 3 Newton steps, once per dividend.
- **reuse**: inverse from the 5-bit start and 4 Newton steps, once per divisor, used for both dividends.
- **32-bit**: the 32-bit hardware divide when both dividends are below 2^32; otherwise control (or, in the
  combinations, the other method).

| Method | below 2^32 | below 2^64 | below 2^128 |
| --- | --- | --- | --- |
| control | 1.00 | 1.00 | 1.00 |
| table | 0.82 | 0.82 | 1.01 |
| reuse | 0.56 | 0.55 | 0.60 |
| 32-bit | 0.60 | 1.00 | 1.00 |
| table + reuse | **0.46** | **0.46** | **0.55** |
| table + 32-bit | 0.60 | 0.94 | 1.37 |
| reuse + 32-bit | 0.60 | 0.56 | 0.96 |
| all three | 0.60 | 0.55 | 0.77 |

Control was run twice per set; the two runs agree within 0.5%. Control per division: 2.62 ns, 2.61 ns, 2.61 ns.

Notes:

- Table + reuse is fastest at every size. Below 2^32 it beats the 32-bit divide: one inverse and two multiplies cost
  less than two 32-bit divides. So adding the 32-bit divide to it (all three) is slower.
- Below 2^64 and 2^128 the 32-bit divide never applies; its rows cost control, or the other method plus the check.
- Below 2^128, the rows with the 32-bit check run about 1 ns per division slower than the same method without it,
  though the check never passes and compiles to one compare and a jump. Not explained; no counters on this machine.
- Two fixes to the benchmark itself before these figures: the compiler merged the table row's two inverses of the
  same divisor (secretly reusing), so the second call now hides the divisor from it (no instructions); and the
  two-word shift had a branch on the shift being 0, which mispredicted, so it is now branch-free.
- A fix after these figures (Oct 31, 2026): the 32-bit rows tested only the two dividends, not the divisor. When both
  dividends are 0, the divisor can be any size, and one whose low 32 bits are 0 made the 32-bit divide divide by zero
  and crash. The test now includes the divisor (one more OR), in `divbench2.nim` and in `bits.div32`, and both check
  modes now test zero dividends with such divisors (the old code crashes on them). The operands here never hit the
  case, so the results are unchanged; the timings were taken before the extra OR.

## Repeat: 10 rounds

The same test 10 more times, each round running all 24 cells in a shuffled order. Each cell is the median across the
rounds of (row ÷ that round's control), with the lowest and highest round in brackets. Raw output:
`results-divide2-repeat-raw.txt` (round, then the usual line). The checksums were the same in every round.

| Method | below 2^32 | below 2^64 | below 2^128 |
| --- | --- | --- | --- |
| control | 1.00 | 1.00 | 1.00 |
| table | 0.83 (0.82–0.83) | 0.82 (0.81–0.83) | 1.00 (0.99–1.00) |
| reuse | 0.55 (0.55–0.57) | 0.55 (0.54–0.55) | 0.62 (0.61–0.67) |
| 32-bit | 0.60 (0.60–0.60) | 1.00 (0.99–1.00) | 1.00 (0.99–1.00) |
| table + reuse | **0.46** (0.46–0.47) | **0.46** (0.46–0.47) | **0.56** (0.55–0.56) |
| table + 32-bit | 0.60 (0.60–0.60) | 0.94 (0.94–0.95) | 1.37 (1.37–1.38) |
| reuse + 32-bit | 0.60 (0.60–0.61) | 0.56 (0.55–0.56) | 0.96 (0.95–0.97) |
| all three | 0.60 (0.60–0.61) | 0.55 (0.54–0.55) | 0.77 (0.77–0.78) |

Control per division across the rounds: 2.59–2.61 ns, 2.60–2.64 ns, 2.60–2.63 ns. Every figure is within 0.02 of the
first run except reuse below 2^128 (0.60 then, 0.62 now), the noisiest cell.

## Control in other languages: C (clang) and Rust

The language's own division, over the same operands (`divbench2 dump <set>` writes them; `src/divctl.c`, built with
clang 18 at -O2, and `src/divctl.rs`, rustc 1.97 at opt-level 2, read them), with the same loop, timing and checksum.
C built with gcc is not a row: it emits exactly Nim's control, since Nim compiles to C and we build with gcc.
10 shuffled rounds, each figure the median of (row ÷ that round's Nim control), lowest and highest round in brackets.
Raw output: `results-divctl-raw.txt`. All checksums match Nim's in every round.

| Control | below 2^32 | below 2^64 | below 2^128 |
| --- | --- | --- | --- |
| Nim (gcc) | 1.00 | 1.00 | 1.00 |
| C (clang) | 0.60 (0.60–0.60) | 1.00 (0.99–1.02) | 1.00 (1.00–1.01) |
| Rust | 0.60 (0.60–0.61) | 1.00 (0.99–1.01) | 1.00 (1.00–1.01) |

Clang and Rust both check whether the numbers fit 32 bits and use the 32-bit divide when they do (our 32-bit row,
built into their compiler, LLVM); Rust also checks for division by zero. At 128 bits all three call a library routine
(gcc's and clang's `__udivti3` from libgcc, Rust's its own), and they cost the same.
