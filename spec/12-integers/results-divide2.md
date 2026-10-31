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
