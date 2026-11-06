# Score and drift of Collatz maps

1. Definitions. For a map with multiplier α and modulus π^k, let N be the size of the multiplier and M the size of the guaranteed division, measured the same way: in Z, N = |m| and M = 2^k; in Z[i], N = N(α) and M = N((1+i)^k) = 2^k. The score is (N − M)/(NM) = 1/M − 1/N. The drift is log2(N/M) − 1, the mean gain per odd step in bits of size (Z) or of norm (Z[i]). Inside the window M < N < 2M, so the score lies in (0, 1/(2M)) and the drift in (−1, 0). Below the window the score is negative.

2. Behaviour. Within one modulus the score and the drift order maps the same way, since both increase with N. Across moduli the score favours the smaller modulus through the term 1/M; the drift ignores size. Comparisons are made within one ring and one class (for example, maps on Z[i] that commute with the units).

3. Values. Collatz maps that commute with the units, one row per family of rotated and mirrored maps.

| Map | Modulus | Score | Drift |
|---|---|---|---|
| 5+2i | (1+i)^4 | 0.0280 | −0.142 |
| 11+2i | (1+i)^6 | 0.0076 | −0.034 |
| 10+3i | (1+i)^6 | 0.0065 | −0.232 |
| 10+i | (1+i)^6 | 0.0057 | −0.342 |
| 9+2i | (1+i)^6 | 0.0039 | −0.591 |
| 12+7i | (1+i)^7 | 0.0026 | −0.408 |
| 11+8i | (1+i)^7 | 0.0024 | −0.469 |
| 8+3i | (1+i)^6 | 0.0019 | −0.810 |
| 12+3i | (1+i)^7 | 0.0013 | −0.743 |
| 11+4i | (1+i)^7 | 0.0005 | −0.902 |
| 7+4i | (1+i)^6 | 0.0002 | −0.978 |

| Map | Modulus | Score | Drift |
|---|---|---|---|
| 7x ± 1 | 4 | 0.1071 | −0.193 |
| 25x ± 1 | 16 | 0.0225 | −0.356 |
| 21x ± 1 | 16 | 0.0149 | −0.608 |
| 53x ± 1 | 32 | 0.0124 | −0.272 |
| 49x ± 1 | 32 | 0.0108 | −0.385 |
| 43x ± 1 | 32 | 0.0080 | −0.574 |
| 41x ± 1 | 32 | 0.0069 | −0.642 |

For comparison, 3x+1 on N scores 1/2 − 1/3 = 0.167, and the modulus-(1+i)^2 map 2 − i scores 1/4 − 1/5 = 0.05. Prior art for such a score has not been searched.
