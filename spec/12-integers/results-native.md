# Machine-level operations against Nim's own, on native uint64 (Oct 31, 2026)

Same method as the builds: one core pinned, idle check, reps of at least 50 ms, 15 reps, median per operation.
`src/native.nim`; `build/bin/native check` runs the correctness checks.

| Operation | Nim (control) | Ours | Ours / Nim |
| --- | --- | --- | --- |
| exact division, b and q below 2^32 | `a div b`, 2.62 ns | inverse by Newton from a 5-bit start, 4 steps, then multiply: 2.64 ns | **1.00** |
| exact division, first version (3-bit start, 5 steps) | 2.63 ns | 3.14 ns | 1.20 |
| subtraction | `a - b` (wraps), 0.231 ns | \|a - b\| from the borrow (sub, sbb, xor, sub): 0.429 ns | 1.86 |

Correctness: 1 million random subtraction pairs, 2 million exact division pairs (odd and even divisors): all agree.
Directions mod 4: a byte that wraps (low 2 bits) and a word's top 2-bit field (carry falls off) agree with
(s + t) mod 4 on all 16 pairs.
