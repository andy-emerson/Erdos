# Exact division methods against Nim's `div`, native uint64 (Oct 31, 2026)

`src/divbench.nim`; `build/bin/divbench check` checks every method (153,600 divisions, all exact). One core pinned,
idle check, reps of at least 50 ms, 15 reps, median per division. Control: Nim's `a div b` (two control rows per set;
they agree within 1%). Times are × control (lower is better). No method allocates.

| Method | small (a < 2^32) | medium (a < 2^53) | large (a < 2^64) | static data | loop code |
| --- | --- | --- | --- | --- | --- |
| Nim `div` (control) | 1.00 (2.61 ns) | 1.00 (2.61 ns) | 1.00 (2.60 ns) | 0 | 86 B |
| Newton inverse, 5-bit start | 1.00 | 1.00 | 1.00 | 0 | 207 B |
| Parallel inverse | 1.01 | 1.02 | 1.01 | 0 | 219 B |
| Table start (8 bits), 3 Newton steps | **0.82** | **0.82** | **0.82** | 128 B table | 217 B |
| 32-bit divide when a < 2^32 (special case) | **0.60** | 1.00 | 1.00 | 0 | 101 B |
| Double divide when a < 2^53 (special case) | **0.42** | **0.41** | 0.98 | 0 | 220 B |
| Inverse reused for a repeated divisor (per division) | **0.55** | **0.55** | **0.55** | 0 | 243 B |
