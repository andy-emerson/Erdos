# Spec 13 v0.2, round 3: timed trials

Nov 1, 2026, 23:31–23:41 local, after an idle check, no other work running. `R3_N=32768 python3 test/run.py`: 32,768 operations per row, 15 reps per cell, median ns per operation; all checksums agree with the exact values. Builds: Control, A (without `and D`), A's narrow mask row (borrow by mask, other branches kept), A's branch-free row, B. C is not timed (author's call).

The first run used 1024 operations per row, replayed many times, and the branch predictor learned much of each "random" row: A's random-sign add took 0.89 ns there and 4.15 ns here. That run is kept below as superseded; its branch results do not hold. At 32,768 operations A's lists still fit this core's 2 MB L2, but not its 48 KB L1, which is why Control's add rises from 0.23 to 0.35 ns.

## Summary (ns per operation)

| | Control | A | A narrow mask | A branch-free |
| --- | --- | --- | --- | --- |
| Integer add, all positive | 0.36 | 0.65 | 0.65 | 2.04 |
| Integer add, random signs, borrow half | 0.35 | 4.15 | 3.38 | 2.06 |
| Integer subtract, all positive, borrow half | 0.36 | 3.84 | 1.11 | 2.14 |
| Integer subtract, random signs, borrow half | 0.35 | 4.35 | 3.47 | 2.17 |
| Integer multiply, random | 0.35 | 0.65 | 0.66 | 0.65 |
| Integer divide, random | 2.62 | 2.62 | 2.61 | 2.62 |
| Complex add, random | 0.72 | 5.43 | 4.68 | 2.03 |
| Complex multiply, random | 1.09 | 0.65 | 0.65 | 0.65 |
| Complex divide, random | 5.22 | 2.61 | 2.62 | 2.62 |

## Findings

1. Under unpredictable signs or axes, A's branches cost 3 to 5 ns per add, and the branch-free row, flat at about 2.1 ns, is the fastest A. With predictable signs A's tree is fastest (0.65 ns against 2.04).
2. The narrow mask row removes the borrow's cost and nothing else: integer subtract of positive operands with the borrow a coin toss takes 1.11 ns against A's 3.84. Under random signs the direction branch still mispredicts, and it saves under 1 ns.
3. Control's add and subtract do not depend on the data (0.35 ns integers, 0.73 complex); A is never faster at addition.
4. Multiply and divide have no data-dependent branch in A; A beats Control on complex multiply (0.65 against 1.09) and divide (2.61 against 5.22), and costs about twice Control on integer multiply and negate (header list traffic). Integer divide is the same in every build.
5. The short rows' finding that branches beat masks everywhere was an artifact of the predictor learning 1024-operation rows.

## Full report


N = 32768 pairs per row, 15 reps per cell; median time per operation. Bytes per value: Control 8 (16 complex), A 9, B 16 (32 complex).

| row | build | ns/op | spread | Ir/op | bytes/value | sums |
| --- | --- | --- | --- | --- | --- | --- |
| int-add-pos-ge | control | 0.3561 | 5.5% | - | 8 | agree |
| int-add-pos-ge | a | 0.6509 | 1.6% | - | 9 | agree |
| int-add-pos-ge | abf | 2.043 | 8.7% | - | 9 | agree |
| int-add-pos-ge | anm | 0.6469 | 2.3% | - | 9 | agree |
| int-add-pos-ge | b | 0.7418 | 1.5% | - | 16 | agree |
| int-add-pos-ge | control-2 | 0.35 | 1.0% | - | 8 | agree |
| int-collapse-pos-ge | b | 0.4309 | 8.9% | - | 16 | agree |
| int-collapse-pos-ge | b-2 | 0.4629 | 21.8% | - | 16 | agree |
| int-add-pos-most | control | 0.3533 | 14.6% | - | 8 | agree |
| int-add-pos-most | a | 0.649 | 0.6% | - | 9 | agree |
| int-add-pos-most | abf | 2.033 | 7.5% | - | 9 | agree |
| int-add-pos-most | anm | 0.6637 | 23.8% | - | 9 | agree |
| int-add-pos-most | b | 0.7623 | 19.7% | - | 16 | agree |
| int-add-pos-most | control-2 | 0.3507 | 3.1% | - | 8 | agree |
| int-collapse-pos-most | b | 0.4104 | 2.2% | - | 16 | agree |
| int-collapse-pos-most | b-2 | 0.4118 | 43.9% | - | 16 | agree |
| int-add-pos-half | control | 0.3565 | 7.5% | - | 8 | agree |
| int-add-pos-half | a | 0.6545 | 15.1% | - | 9 | agree |
| int-add-pos-half | abf | 2.053 | 7.5% | - | 9 | agree |
| int-add-pos-half | anm | 0.6546 | 3.5% | - | 9 | agree |
| int-add-pos-half | b | 0.7365 | 5.6% | - | 16 | agree |
| int-add-pos-half | control-2 | 0.3514 | 2.7% | - | 8 | agree |
| int-collapse-pos-half | b | 0.4082 | 3.5% | - | 16 | agree |
| int-collapse-pos-half | b-2 | 0.4132 | 90.6% | - | 16 | agree |
| int-add-most-ge | control | 0.3502 | 1.3% | - | 8 | agree |
| int-add-most-ge | a | 1 | 7.5% | - | 9 | agree |
| int-add-most-ge | abf | 2.056 | 40.6% | - | 9 | agree |
| int-add-most-ge | anm | 0.9755 | 2.9% | - | 9 | agree |
| int-add-most-ge | b | 0.7518 | 16.6% | - | 16 | agree |
| int-add-most-ge | control-2 | 0.3521 | 9.7% | - | 8 | agree |
| int-collapse-most-ge | b | 0.7637 | 7.3% | - | 16 | agree |
| int-collapse-most-ge | b-2 | 0.7752 | 11.7% | - | 16 | agree |
| int-add-most-most | control | 0.3513 | 1.4% | - | 8 | agree |
| int-add-most-most | a | 1.004 | 1.9% | - | 9 | agree |
| int-add-most-most | abf | 2.04 | 4.5% | - | 9 | agree |
| int-add-most-most | anm | 0.9951 | 11.1% | - | 9 | agree |
| int-add-most-most | b | 0.7372 | 5.0% | - | 16 | agree |
| int-add-most-most | control-2 | 0.355 | 23.1% | - | 8 | agree |
| int-collapse-most-most | b | 0.7566 | 6.4% | - | 16 | agree |
| int-collapse-most-most | b-2 | 0.7601 | 22.5% | - | 16 | agree |
| int-add-most-half | control | 0.3498 | 0.6% | - | 8 | agree |
| int-add-most-half | a | 0.9954 | 3.3% | - | 9 | agree |
| int-add-most-half | abf | 2.029 | 4.0% | - | 9 | agree |
| int-add-most-half | anm | 0.9836 | 3.8% | - | 9 | agree |
| int-add-most-half | b | 0.7544 | 4.6% | - | 16 | agree |
| int-add-most-half | control-2 | 0.3496 | 1.3% | - | 8 | agree |
| int-collapse-most-half | b | 0.7854 | 0.7% | - | 16 | agree |
| int-collapse-most-half | b-2 | 0.7621 | 9.4% | - | 16 | agree |
| int-add-rand-ge | control | 0.354 | 4.7% | - | 8 | agree |
| int-add-rand-ge | a | 3.413 | 12.7% | - | 9 | agree |
| int-add-rand-ge | abf | 2.027 | 1.6% | - | 9 | agree |
| int-add-rand-ge | anm | 3.418 | 9.0% | - | 9 | agree |
| int-add-rand-ge | b | 0.7433 | 7.4% | - | 16 | agree |
| int-add-rand-ge | control-2 | 0.3516 | 30.5% | - | 8 | agree |
| int-collapse-rand-ge | b | 3.127 | 8.9% | - | 16 | agree |
| int-collapse-rand-ge | b-2 | 3.147 | 6.1% | - | 16 | agree |
| int-add-rand-most | control | 0.3519 | 2.9% | - | 8 | agree |
| int-add-rand-most | a | 3.391 | 6.5% | - | 9 | agree |
| int-add-rand-most | abf | 2.05 | 3.6% | - | 9 | agree |
| int-add-rand-most | anm | 3.409 | 8.9% | - | 9 | agree |
| int-add-rand-most | b | 0.7413 | 24.1% | - | 16 | agree |
| int-add-rand-most | control-2 | 0.3502 | 1.4% | - | 8 | agree |
| int-collapse-rand-most | b | 3.11 | 18.3% | - | 16 | agree |
| int-collapse-rand-most | b-2 | 3.12 | 8.0% | - | 16 | agree |
| int-add-rand-half | control | 0.3484 | 0.5% | - | 8 | agree |
| int-add-rand-half | a | 4.148 | 9.3% | - | 9 | agree |
| int-add-rand-half | abf | 2.058 | 4.5% | - | 9 | agree |
| int-add-rand-half | anm | 3.385 | 24.3% | - | 9 | agree |
| int-add-rand-half | b | 0.7308 | 3.3% | - | 16 | agree |
| int-add-rand-half | control-2 | 0.3532 | 5.0% | - | 8 | agree |
| int-collapse-rand-half | b | 3.133 | 6.3% | - | 16 | agree |
| int-collapse-rand-half | b-2 | 3.172 | 8.0% | - | 16 | agree |
| int-sub-pos-ge | control | 0.3604 | 29.0% | - | 8 | agree |
| int-sub-pos-ge | a | 0.7245 | 3.7% | - | 9 | agree |
| int-sub-pos-ge | abf | 2.138 | 15.4% | - | 9 | agree |
| int-sub-pos-ge | anm | 1.12 | 6.6% | - | 9 | agree |
| int-sub-pos-ge | b | 0.7427 | 27.2% | - | 16 | agree |
| int-sub-pos-ge | control-2 | 0.354 | 22.9% | - | 8 | agree |
| int-sub-pos-most | control | 0.3496 | 5.9% | - | 8 | agree |
| int-sub-pos-most | a | 1.067 | 1.7% | - | 9 | agree |
| int-sub-pos-most | abf | 2.193 | 38.6% | - | 9 | agree |
| int-sub-pos-most | anm | 1.149 | 72.8% | - | 9 | agree |
| int-sub-pos-most | b | 0.7246 | 7.8% | - | 16 | agree |
| int-sub-pos-most | control-2 | 0.3492 | 0.8% | - | 8 | agree |
| int-sub-pos-half | control | 0.3562 | 10.8% | - | 8 | agree |
| int-sub-pos-half | a | 3.843 | 6.3% | - | 9 | agree |
| int-sub-pos-half | abf | 2.139 | 4.0% | - | 9 | agree |
| int-sub-pos-half | anm | 1.115 | 7.0% | - | 9 | agree |
| int-sub-pos-half | b | 0.773 | 8.2% | - | 16 | agree |
| int-sub-pos-half | control-2 | 0.3513 | 2.8% | - | 8 | agree |
| int-sub-most-ge | control | 0.353 | 17.1% | - | 8 | agree |
| int-sub-most-ge | a | 1.408 | 20.5% | - | 9 | agree |
| int-sub-most-ge | abf | 2.156 | 25.9% | - | 9 | agree |
| int-sub-most-ge | anm | 1.519 | 49.4% | - | 9 | agree |
| int-sub-most-ge | b | 0.743 | 5.4% | - | 16 | agree |
| int-sub-most-ge | control-2 | 0.3539 | 13.4% | - | 8 | agree |
| int-sub-most-most | control | 0.3506 | 110.6% | - | 8 | agree |
| int-sub-most-most | a | 0.9176 | 3.2% | - | 9 | agree |
| int-sub-most-most | abf | 2.143 | 5.4% | - | 9 | agree |
| int-sub-most-most | anm | 1.5 | 2.8% | - | 9 | agree |
| int-sub-most-most | b | 0.7361 | 7.5% | - | 16 | agree |
| int-sub-most-most | control-2 | 0.3521 | 26.9% | - | 8 | agree |
| int-sub-most-half | control | 0.3521 | 2.8% | - | 8 | agree |
| int-sub-most-half | a | 3.437 | 15.1% | - | 9 | agree |
| int-sub-most-half | abf | 2.164 | 12.5% | - | 9 | agree |
| int-sub-most-half | anm | 1.493 | 2.2% | - | 9 | agree |
| int-sub-most-half | b | 0.7444 | 14.9% | - | 16 | agree |
| int-sub-most-half | control-2 | 0.3546 | 5.9% | - | 8 | agree |
| int-sub-rand-ge | control | 0.3507 | 6.2% | - | 8 | agree |
| int-sub-rand-ge | a | 3.641 | 24.7% | - | 9 | agree |
| int-sub-rand-ge | abf | 2.189 | 9.9% | - | 9 | agree |
| int-sub-rand-ge | anm | 3.474 | 8.6% | - | 9 | agree |
| int-sub-rand-ge | b | 0.7463 | 23.3% | - | 16 | agree |
| int-sub-rand-ge | control-2 | 0.3618 | 6.8% | - | 8 | agree |
| int-sub-rand-most | control | 0.3509 | 1.1% | - | 8 | agree |
| int-sub-rand-most | a | 3.38 | 6.5% | - | 9 | agree |
| int-sub-rand-most | abf | 2.181 | 57.6% | - | 9 | agree |
| int-sub-rand-most | anm | 3.397 | 13.7% | - | 9 | agree |
| int-sub-rand-most | b | 0.7328 | 15.4% | - | 16 | agree |
| int-sub-rand-most | control-2 | 0.3503 | 1.5% | - | 8 | agree |
| int-sub-rand-half | control | 0.3507 | 22.3% | - | 8 | agree |
| int-sub-rand-half | a | 4.35 | 27.9% | - | 9 | agree |
| int-sub-rand-half | abf | 2.167 | 5.7% | - | 9 | agree |
| int-sub-rand-half | anm | 3.472 | 6.5% | - | 9 | agree |
| int-sub-rand-half | b | 0.7636 | 14.9% | - | 16 | agree |
| int-sub-rand-half | control-2 | 0.3495 | 0.5% | - | 8 | agree |
| int-neg-pos | control | 0.2412 | 3.0% | - | 8 | agree |
| int-neg-pos | a | 0.6089 | 1.4% | - | 9 | agree |
| int-neg-pos | abf | 0.6147 | 17.3% | - | 9 | agree |
| int-neg-pos | anm | 0.6056 | 4.7% | - | 9 | agree |
| int-neg-pos | b | 0.4308 | 2.7% | - | 16 | agree |
| int-neg-pos | control-2 | 0.2411 | 3.5% | - | 8 | agree |
| int-neg-most | control | 0.241 | 2.4% | - | 8 | agree |
| int-neg-most | a | 0.6043 | 2.4% | - | 9 | agree |
| int-neg-most | abf | 0.6108 | 3.2% | - | 9 | agree |
| int-neg-most | anm | 0.6062 | 1.1% | - | 9 | agree |
| int-neg-most | b | 0.3964 | 3.9% | - | 16 | agree |
| int-neg-most | control-2 | 0.2414 | 1.9% | - | 8 | agree |
| int-neg-rand | control | 0.245 | 18.7% | - | 8 | agree |
| int-neg-rand | a | 0.6071 | 7.9% | - | 9 | agree |
| int-neg-rand | abf | 0.6124 | 2.5% | - | 9 | agree |
| int-neg-rand | anm | 0.6067 | 2.8% | - | 9 | agree |
| int-neg-rand | b | 0.3984 | 4.9% | - | 16 | agree |
| int-neg-rand | control-2 | 0.2409 | 2.3% | - | 8 | agree |
| int-mul-pos | control | 0.3533 | 2.4% | - | 8 | agree |
| int-mul-pos | a | 0.6472 | 1.6% | - | 9 | agree |
| int-mul-pos | abf | 0.6535 | 6.7% | - | 9 | agree |
| int-mul-pos | anm | 0.6474 | 9.2% | - | 9 | agree |
| int-mul-pos | b | 1.07 | 5.0% | - | 16 | agree |
| int-mul-pos | control-2 | 0.354 | 1.1% | - | 8 | agree |
| int-mul-most | control | 0.3577 | 5.4% | - | 8 | agree |
| int-mul-most | a | 0.6462 | 18.8% | - | 9 | agree |
| int-mul-most | abf | 0.6503 | 2.7% | - | 9 | agree |
| int-mul-most | anm | 0.6494 | 1.4% | - | 9 | agree |
| int-mul-most | b | 1.085 | 22.7% | - | 16 | agree |
| int-mul-most | control-2 | 0.3553 | 11.3% | - | 8 | agree |
| int-mul-rand | control | 0.3535 | 2.6% | - | 8 | agree |
| int-mul-rand | a | 0.6486 | 3.0% | - | 9 | agree |
| int-mul-rand | abf | 0.6512 | 16.4% | - | 9 | agree |
| int-mul-rand | anm | 0.6588 | 14.6% | - | 9 | agree |
| int-mul-rand | b | 1.065 | 20.2% | - | 16 | agree |
| int-mul-rand | control-2 | 0.3564 | 4.9% | - | 8 | agree |
| int-div-pos | control | 2.611 | 1.1% | - | 8 | agree |
| int-div-pos | a | 2.615 | 1.3% | - | 9 | agree |
| int-div-pos | abf | 2.619 | 4.0% | - | 9 | agree |
| int-div-pos | anm | 2.641 | 5.1% | - | 9 | agree |
| int-div-pos | b | 2.631 | 4.2% | - | 16 | agree |
| int-div-pos | control-2 | 2.618 | 9.1% | - | 8 | agree |
| int-div-most | control | 2.616 | 2.2% | - | 8 | agree |
| int-div-most | a | 2.613 | 14.4% | - | 9 | agree |
| int-div-most | abf | 2.635 | 13.0% | - | 9 | agree |
| int-div-most | anm | 2.654 | 3.9% | - | 9 | agree |
| int-div-most | b | 2.625 | 9.6% | - | 16 | agree |
| int-div-most | control-2 | 2.615 | 5.1% | - | 8 | agree |
| int-div-rand | control | 2.621 | 14.4% | - | 8 | agree |
| int-div-rand | a | 2.617 | 4.9% | - | 9 | agree |
| int-div-rand | abf | 2.616 | 5.4% | - | 9 | agree |
| int-div-rand | anm | 2.611 | 1.4% | - | 9 | agree |
| int-div-rand | b | 7.709 | 3.6% | - | 16 | agree |
| int-div-rand | control-2 | 2.613 | 6.9% | - | 8 | agree |
| cx-add-pos-ge | control | 0.7478 | 4.3% | - | 16 | agree |
| cx-add-pos-ge | a | 3.672 | 3.4% | - | 9 | agree |
| cx-add-pos-ge | abf | 2.046 | 4.0% | - | 9 | agree |
| cx-add-pos-ge | anm | 3.531 | 19.1% | - | 9 | agree |
| cx-add-pos-ge | b | 2.837 | 4.5% | - | 32 | agree |
| cx-add-pos-ge | control-2 | 0.7528 | 8.2% | - | 16 | agree |
| cx-collapse-pos-ge | b | 1.223 | 11.6% | - | 32 | agree |
| cx-collapse-pos-ge | b-2 | 1.188 | 2.8% | - | 32 | agree |
| cx-add-pos-most | control | 0.7165 | 6.0% | - | 16 | agree |
| cx-add-pos-most | a | 3.606 | 5.9% | - | 9 | agree |
| cx-add-pos-most | abf | 2.028 | 4.1% | - | 9 | agree |
| cx-add-pos-most | anm | 3.591 | 3.3% | - | 9 | agree |
| cx-add-pos-most | b | 3.106 | 1.6% | - | 32 | agree |
| cx-add-pos-most | control-2 | 0.7197 | 1.0% | - | 16 | agree |
| cx-collapse-pos-most | b | 1.156 | 3.9% | - | 32 | agree |
| cx-collapse-pos-most | b-2 | 1.179 | 3.5% | - | 32 | agree |
| cx-add-pos-half | control | 0.762 | 4.0% | - | 16 | agree |
| cx-add-pos-half | a | 4.255 | 6.7% | - | 9 | agree |
| cx-add-pos-half | abf | 2.064 | 3.8% | - | 9 | agree |
| cx-add-pos-half | anm | 4.737 | 2.2% | - | 9 | agree |
| cx-add-pos-half | b | 3.085 | 12.8% | - | 32 | agree |
| cx-add-pos-half | control-2 | 0.7672 | 10.1% | - | 16 | agree |
| cx-collapse-pos-half | b | 1.202 | 21.1% | - | 32 | agree |
| cx-collapse-pos-half | b-2 | 1.183 | 1.3% | - | 32 | agree |
| cx-add-most-ge | control | 0.7681 | 6.6% | - | 16 | agree |
| cx-add-most-ge | a | 3.504 | 11.7% | - | 9 | agree |
| cx-add-most-ge | abf | 2.033 | 1.3% | - | 9 | agree |
| cx-add-most-ge | anm | 3.24 | 9.9% | - | 9 | agree |
| cx-add-most-ge | b | 2.81 | 7.2% | - | 32 | agree |
| cx-add-most-ge | control-2 | 0.7304 | 24.3% | - | 16 | agree |
| cx-collapse-most-ge | b | 1.233 | 7.3% | - | 32 | agree |
| cx-collapse-most-ge | b-2 | 1.31 | 24.5% | - | 32 | agree |
| cx-add-most-most | control | 0.7173 | 4.1% | - | 16 | agree |
| cx-add-most-most | a | 3.52 | 7.0% | - | 9 | agree |
| cx-add-most-most | abf | 2.035 | 4.5% | - | 9 | agree |
| cx-add-most-most | anm | 3.418 | 2.8% | - | 9 | agree |
| cx-add-most-most | b | 3.115 | 4.2% | - | 32 | agree |
| cx-add-most-most | control-2 | 0.7235 | 2.1% | - | 16 | agree |
| cx-collapse-most-most | b | 1.256 | 4.2% | - | 32 | agree |
| cx-collapse-most-most | b-2 | 1.266 | 2.9% | - | 32 | agree |
| cx-add-most-half | control | 0.7224 | 2.5% | - | 16 | agree |
| cx-add-most-half | a | 4.509 | 12.1% | - | 9 | agree |
| cx-add-most-half | abf | 2.041 | 2.3% | - | 9 | agree |
| cx-add-most-half | anm | 4.689 | 4.1% | - | 9 | agree |
| cx-add-most-half | b | 3.297 | 9.0% | - | 32 | agree |
| cx-add-most-half | control-2 | 0.7913 | 5.9% | - | 16 | agree |
| cx-collapse-most-half | b | 1.193 | 7.5% | - | 32 | agree |
| cx-collapse-most-half | b-2 | 1.266 | 33.9% | - | 32 | agree |
| cx-add-rand-ge | control | 0.7319 | 2.9% | - | 16 | agree |
| cx-add-rand-ge | a | 3.635 | 10.3% | - | 9 | agree |
| cx-add-rand-ge | abf | 2.031 | 18.3% | - | 9 | agree |
| cx-add-rand-ge | anm | 3.302 | 17.0% | - | 9 | agree |
| cx-add-rand-ge | b | 2.918 | 11.6% | - | 32 | agree |
| cx-add-rand-ge | control-2 | 0.723 | 7.4% | - | 16 | agree |
| cx-collapse-rand-ge | b | 3.136 | 29.5% | - | 32 | agree |
| cx-collapse-rand-ge | b-2 | 3.15 | 14.7% | - | 32 | agree |
| cx-add-rand-most | control | 0.7498 | 6.1% | - | 16 | agree |
| cx-add-rand-most | a | 3.877 | 21.9% | - | 9 | agree |
| cx-add-rand-most | abf | 2.03 | 2.4% | - | 9 | agree |
| cx-add-rand-most | anm | 3.846 | 18.3% | - | 9 | agree |
| cx-add-rand-most | b | 2.89 | 42.0% | - | 32 | agree |
| cx-add-rand-most | control-2 | 0.7439 | 33.8% | - | 16 | agree |
| cx-collapse-rand-most | b | 3.209 | 19.7% | - | 32 | agree |
| cx-collapse-rand-most | b-2 | 3.076 | 16.5% | - | 32 | agree |
| cx-add-rand-half | control | 0.7215 | 21.4% | - | 16 | agree |
| cx-add-rand-half | a | 5.428 | 8.9% | - | 9 | agree |
| cx-add-rand-half | abf | 2.029 | 2.2% | - | 9 | agree |
| cx-add-rand-half | anm | 4.677 | 10.1% | - | 9 | agree |
| cx-add-rand-half | b | 3.157 | 2.5% | - | 32 | agree |
| cx-add-rand-half | control-2 | 0.7353 | 5.5% | - | 16 | agree |
| cx-collapse-rand-half | b | 3.165 | 20.6% | - | 32 | agree |
| cx-collapse-rand-half | b-2 | 3.29 | 34.2% | - | 32 | agree |
| cx-sub-pos-ge | control | 0.7766 | 9.7% | - | 16 | agree |
| cx-sub-pos-ge | a | 3.949 | 4.3% | - | 9 | agree |
| cx-sub-pos-ge | abf | 2.143 | 1.9% | - | 9 | agree |
| cx-sub-pos-ge | anm | 4.083 | 5.1% | - | 9 | agree |
| cx-sub-pos-ge | b | 2.772 | 5.2% | - | 32 | agree |
| cx-sub-pos-ge | control-2 | 0.7168 | 2.7% | - | 16 | agree |
| cx-sub-pos-most | control | 0.7357 | 9.2% | - | 16 | agree |
| cx-sub-pos-most | a | 4.108 | 27.4% | - | 9 | agree |
| cx-sub-pos-most | abf | 2.151 | 51.8% | - | 9 | agree |
| cx-sub-pos-most | anm | 3.964 | 7.6% | - | 9 | agree |
| cx-sub-pos-most | b | 3.24 | 4.2% | - | 32 | agree |
| cx-sub-pos-most | control-2 | 0.7604 | 8.7% | - | 16 | agree |
| cx-sub-pos-half | control | 0.7239 | 20.3% | - | 16 | agree |
| cx-sub-pos-half | a | 5.965 | 5.9% | - | 9 | agree |
| cx-sub-pos-half | abf | 2.134 | 12.5% | - | 9 | agree |
| cx-sub-pos-half | anm | 4.69 | 7.6% | - | 9 | agree |
| cx-sub-pos-half | b | 3.471 | 3.2% | - | 32 | agree |
| cx-sub-pos-half | control-2 | 0.7367 | 38.4% | - | 16 | agree |
| cx-sub-most-ge | control | 0.7202 | 5.0% | - | 16 | agree |
| cx-sub-most-ge | a | 4.032 | 9.3% | - | 9 | agree |
| cx-sub-most-ge | abf | 2.147 | 17.0% | - | 9 | agree |
| cx-sub-most-ge | anm | 3.635 | 11.6% | - | 9 | agree |
| cx-sub-most-ge | b | 2.704 | 10.3% | - | 32 | agree |
| cx-sub-most-ge | control-2 | 0.7157 | 4.0% | - | 16 | agree |
| cx-sub-most-most | control | 0.7022 | 3.8% | - | 16 | agree |
| cx-sub-most-most | a | 4.206 | 9.6% | - | 9 | agree |
| cx-sub-most-most | abf | 2.133 | 55.6% | - | 9 | agree |
| cx-sub-most-most | anm | 3.63 | 5.1% | - | 9 | agree |
| cx-sub-most-most | b | 3.368 | 3.1% | - | 32 | agree |
| cx-sub-most-most | control-2 | 0.7829 | 2.1% | - | 16 | agree |
| cx-sub-most-half | control | 0.7555 | 2.7% | - | 16 | agree |
| cx-sub-most-half | a | 5.924 | 6.2% | - | 9 | agree |
| cx-sub-most-half | abf | 2.155 | 2.7% | - | 9 | agree |
| cx-sub-most-half | anm | 4.656 | 6.4% | - | 9 | agree |
| cx-sub-most-half | b | 3.223 | 2.3% | - | 32 | agree |
| cx-sub-most-half | control-2 | 0.7084 | 9.7% | - | 16 | agree |
| cx-sub-rand-ge | control | 0.7185 | 35.3% | - | 16 | agree |
| cx-sub-rand-ge | a | 3.932 | 3.0% | - | 9 | agree |
| cx-sub-rand-ge | abf | 2.138 | 5.2% | - | 9 | agree |
| cx-sub-rand-ge | anm | 3.376 | 35.6% | - | 9 | agree |
| cx-sub-rand-ge | b | 2.898 | 17.8% | - | 32 | agree |
| cx-sub-rand-ge | control-2 | 0.6925 | 6.3% | - | 16 | agree |
| cx-sub-rand-most | control | 0.7075 | 4.9% | - | 16 | agree |
| cx-sub-rand-most | a | 4.088 | 47.6% | - | 9 | agree |
| cx-sub-rand-most | abf | 2.145 | 5.5% | - | 9 | agree |
| cx-sub-rand-most | anm | 3.832 | 22.2% | - | 9 | agree |
| cx-sub-rand-most | b | 3.309 | 12.4% | - | 32 | agree |
| cx-sub-rand-most | control-2 | 0.7267 | 24.7% | - | 16 | agree |
| cx-sub-rand-half | control | 0.728 | 3.4% | - | 16 | agree |
| cx-sub-rand-half | a | 5.339 | 6.2% | - | 9 | agree |
| cx-sub-rand-half | abf | 2.169 | 55.8% | - | 9 | agree |
| cx-sub-rand-half | anm | 4.871 | 11.6% | - | 9 | agree |
| cx-sub-rand-half | b | 3.23 | 7.5% | - | 32 | agree |
| cx-sub-rand-half | control-2 | 0.7104 | 14.1% | - | 16 | agree |
| cx-neg-pos | control | 0.4239 | 1.9% | - | 16 | agree |
| cx-neg-pos | a | 0.6069 | 18.5% | - | 9 | agree |
| cx-neg-pos | abf | 0.6129 | 1.9% | - | 9 | agree |
| cx-neg-pos | anm | 0.6057 | 1.1% | - | 9 | agree |
| cx-neg-pos | b | 1.273 | 6.4% | - | 32 | agree |
| cx-neg-pos | control-2 | 0.4188 | 2.0% | - | 16 | agree |
| cx-neg-most | control | 0.4212 | 5.1% | - | 16 | agree |
| cx-neg-most | a | 0.6045 | 0.3% | - | 9 | agree |
| cx-neg-most | abf | 0.6115 | 1.0% | - | 9 | agree |
| cx-neg-most | anm | 0.605 | 3.1% | - | 9 | agree |
| cx-neg-most | b | 1.126 | 6.3% | - | 32 | agree |
| cx-neg-most | control-2 | 0.4195 | 5.9% | - | 16 | agree |
| cx-neg-rand | control | 0.4193 | 14.9% | - | 16 | agree |
| cx-neg-rand | a | 0.6042 | 3.5% | - | 9 | agree |
| cx-neg-rand | abf | 0.6087 | 0.8% | - | 9 | agree |
| cx-neg-rand | anm | 0.6036 | 1.5% | - | 9 | agree |
| cx-neg-rand | b | 1.2 | 2.0% | - | 32 | agree |
| cx-neg-rand | control-2 | 0.4208 | 4.1% | - | 16 | agree |
| cx-mul-pos | control | 1.079 | 5.2% | - | 16 | agree |
| cx-mul-pos | a | 0.6458 | 1.6% | - | 9 | agree |
| cx-mul-pos | abf | 0.6509 | 13.9% | - | 9 | agree |
| cx-mul-pos | anm | 0.6477 | 10.7% | - | 9 | agree |
| cx-mul-pos | b | 4.522 | 1.6% | - | 32 | agree |
| cx-mul-pos | control-2 | 1.064 | 1.9% | - | 16 | agree |
| cx-mul-most | control | 1.057 | 13.8% | - | 16 | agree |
| cx-mul-most | a | 0.6545 | 5.6% | - | 9 | agree |
| cx-mul-most | abf | 0.6534 | 8.0% | - | 9 | agree |
| cx-mul-most | anm | 0.6532 | 7.1% | - | 9 | agree |
| cx-mul-most | b | 4.487 | 2.5% | - | 32 | agree |
| cx-mul-most | control-2 | 1.057 | 3.6% | - | 16 | agree |
| cx-mul-rand | control | 1.093 | 1.4% | - | 16 | agree |
| cx-mul-rand | a | 0.647 | 7.9% | - | 9 | agree |
| cx-mul-rand | abf | 0.6489 | 8.1% | - | 9 | agree |
| cx-mul-rand | anm | 0.6522 | 14.0% | - | 9 | agree |
| cx-mul-rand | b | 4.507 | 21.0% | - | 32 | agree |
| cx-mul-rand | control-2 | 1.107 | 2.6% | - | 16 | agree |
| cx-div-pos | control | 5.335 | 32.6% | - | 16 | agree |
| cx-div-pos | a | 2.608 | 1.4% | - | 9 | agree |
| cx-div-pos | abf | 2.604 | 5.2% | - | 9 | agree |
| cx-div-pos | anm | 2.627 | 10.0% | - | 9 | agree |
| cx-div-pos | b | 3.444 | 8.8% | - | 32 | agree |
| cx-div-pos | control-2 | 5.255 | 3.5% | - | 16 | agree |
| cx-div-most | control | 5.343 | 30.6% | - | 16 | agree |
| cx-div-most | a | 2.613 | 3.1% | - | 9 | agree |
| cx-div-most | abf | 2.616 | 2.2% | - | 9 | agree |
| cx-div-most | anm | 2.614 | 9.6% | - | 9 | agree |
| cx-div-most | b | 3.282 | 4.6% | - | 32 | agree |
| cx-div-most | control-2 | 5.232 | 2.3% | - | 16 | agree |
| cx-div-rand | control | 5.224 | 1.4% | - | 16 | agree |
| cx-div-rand | a | 2.61 | 1.4% | - | 9 | agree |
| cx-div-rand | abf | 2.623 | 3.4% | - | 9 | agree |
| cx-div-rand | anm | 2.623 | 3.5% | - | 9 | agree |
| cx-div-rand | b | 9.565 | 5.3% | - | 32 | agree |
| cx-div-rand | control-2 | 5.217 | 1.5% | - | 16 | agree |

| row | build x Control | T(Control)/T(build) | band | time |
| --- | --- | --- | --- | --- |
| int-add-pos-ge | a | 0.547 | 0.05 | slower |
| int-add-pos-ge | abf | 0.174 | 0.05 | slower |
| int-add-pos-ge | anm | 0.55 | 0.05 | slower |
| int-add-pos-ge | b | 0.48 | 0.05 | slower |
| int-add-pos-most | a | 0.544 | 0.05 | slower |
| int-add-pos-most | abf | 0.174 | 0.05 | slower |
| int-add-pos-most | anm | 0.532 | 0.05 | slower |
| int-add-pos-most | b | 0.463 | 0.05 | slower |
| int-add-pos-half | a | 0.545 | 0.05 | slower |
| int-add-pos-half | abf | 0.174 | 0.05 | slower |
| int-add-pos-half | anm | 0.545 | 0.05 | slower |
| int-add-pos-half | b | 0.484 | 0.05 | slower |
| int-add-most-ge | a | 0.35 | 0.05 | slower |
| int-add-most-ge | abf | 0.17 | 0.05 | slower |
| int-add-most-ge | anm | 0.359 | 0.05 | slower |
| int-add-most-ge | b | 0.466 | 0.05 | slower |
| int-add-most-most | a | 0.35 | 0.05 | slower |
| int-add-most-most | abf | 0.172 | 0.05 | slower |
| int-add-most-most | anm | 0.353 | 0.05 | slower |
| int-add-most-most | b | 0.477 | 0.05 | slower |
| int-add-most-half | a | 0.351 | 0.05 | slower |
| int-add-most-half | abf | 0.172 | 0.05 | slower |
| int-add-most-half | anm | 0.356 | 0.05 | slower |
| int-add-most-half | b | 0.464 | 0.05 | slower |
| int-add-rand-ge | a | 0.104 | 0.05 | slower |
| int-add-rand-ge | abf | 0.175 | 0.05 | slower |
| int-add-rand-ge | anm | 0.104 | 0.05 | slower |
| int-add-rand-ge | b | 0.476 | 0.05 | slower |
| int-add-rand-most | a | 0.104 | 0.05 | slower |
| int-add-rand-most | abf | 0.172 | 0.05 | slower |
| int-add-rand-most | anm | 0.103 | 0.05 | slower |
| int-add-rand-most | b | 0.475 | 0.05 | slower |
| int-add-rand-half | a | 0.084 | 0.05 | slower |
| int-add-rand-half | abf | 0.169 | 0.05 | slower |
| int-add-rand-half | anm | 0.103 | 0.05 | slower |
| int-add-rand-half | b | 0.477 | 0.05 | slower |
| int-sub-pos-ge | a | 0.497 | 0.05 | slower |
| int-sub-pos-ge | abf | 0.169 | 0.05 | slower |
| int-sub-pos-ge | anm | 0.322 | 0.05 | slower |
| int-sub-pos-ge | b | 0.485 | 0.05 | slower |
| int-sub-pos-most | a | 0.328 | 0.05 | slower |
| int-sub-pos-most | abf | 0.159 | 0.05 | slower |
| int-sub-pos-most | anm | 0.304 | 0.05 | slower |
| int-sub-pos-most | b | 0.482 | 0.05 | slower |
| int-sub-pos-half | a | 0.093 | 0.05 | slower |
| int-sub-pos-half | abf | 0.167 | 0.05 | slower |
| int-sub-pos-half | anm | 0.32 | 0.05 | slower |
| int-sub-pos-half | b | 0.461 | 0.05 | slower |
| int-sub-most-ge | a | 0.251 | 0.05 | slower |
| int-sub-most-ge | abf | 0.164 | 0.05 | slower |
| int-sub-most-ge | anm | 0.232 | 0.05 | slower |
| int-sub-most-ge | b | 0.475 | 0.05 | slower |
| int-sub-most-most | a | 0.382 | 0.05 | slower |
| int-sub-most-most | abf | 0.164 | 0.05 | slower |
| int-sub-most-most | anm | 0.234 | 0.05 | slower |
| int-sub-most-most | b | 0.476 | 0.05 | slower |
| int-sub-most-half | a | 0.102 | 0.05 | slower |
| int-sub-most-half | abf | 0.163 | 0.05 | slower |
| int-sub-most-half | anm | 0.236 | 0.05 | slower |
| int-sub-most-half | b | 0.473 | 0.05 | slower |
| int-sub-rand-ge | a | 0.096 | 0.05 | slower |
| int-sub-rand-ge | abf | 0.16 | 0.05 | slower |
| int-sub-rand-ge | anm | 0.101 | 0.05 | slower |
| int-sub-rand-ge | b | 0.47 | 0.05 | slower |
| int-sub-rand-most | a | 0.104 | 0.05 | slower |
| int-sub-rand-most | abf | 0.161 | 0.05 | slower |
| int-sub-rand-most | anm | 0.103 | 0.05 | slower |
| int-sub-rand-most | b | 0.479 | 0.05 | slower |
| int-sub-rand-half | a | 0.081 | 0.05 | slower |
| int-sub-rand-half | abf | 0.162 | 0.05 | slower |
| int-sub-rand-half | anm | 0.101 | 0.05 | slower |
| int-sub-rand-half | b | 0.459 | 0.05 | slower |
| int-neg-pos | a | 0.396 | 0.05 | slower |
| int-neg-pos | abf | 0.392 | 0.05 | slower |
| int-neg-pos | anm | 0.398 | 0.05 | slower |
| int-neg-pos | b | 0.56 | 0.05 | slower |
| int-neg-most | a | 0.399 | 0.05 | slower |
| int-neg-most | abf | 0.395 | 0.05 | slower |
| int-neg-most | anm | 0.398 | 0.05 | slower |
| int-neg-most | b | 0.608 | 0.05 | slower |
| int-neg-rand | a | 0.404 | 0.05 | slower |
| int-neg-rand | abf | 0.4 | 0.05 | slower |
| int-neg-rand | anm | 0.404 | 0.05 | slower |
| int-neg-rand | b | 0.615 | 0.05 | slower |
| int-mul-pos | a | 0.546 | 0.05 | slower |
| int-mul-pos | abf | 0.541 | 0.05 | slower |
| int-mul-pos | anm | 0.546 | 0.05 | slower |
| int-mul-pos | b | 0.33 | 0.05 | slower |
| int-mul-most | a | 0.554 | 0.05 | slower |
| int-mul-most | abf | 0.55 | 0.05 | slower |
| int-mul-most | anm | 0.551 | 0.05 | slower |
| int-mul-most | b | 0.33 | 0.05 | slower |
| int-mul-rand | a | 0.545 | 0.05 | slower |
| int-mul-rand | abf | 0.543 | 0.05 | slower |
| int-mul-rand | anm | 0.537 | 0.05 | slower |
| int-mul-rand | b | 0.332 | 0.05 | slower |
| int-div-pos | a | 0.999 | 0.05 | within noise |
| int-div-pos | abf | 0.997 | 0.05 | within noise |
| int-div-pos | anm | 0.989 | 0.05 | within noise |
| int-div-pos | b | 0.993 | 0.05 | within noise |
| int-div-most | a | 1.001 | 0.05 | within noise |
| int-div-most | abf | 0.993 | 0.05 | within noise |
| int-div-most | anm | 0.986 | 0.05 | within noise |
| int-div-most | b | 0.997 | 0.05 | within noise |
| int-div-rand | a | 1.002 | 0.05 | within noise |
| int-div-rand | abf | 1.002 | 0.05 | within noise |
| int-div-rand | anm | 1.004 | 0.05 | within noise |
| int-div-rand | b | 0.34 | 0.05 | slower |
| cx-add-pos-ge | a | 0.204 | 0.05 | slower |
| cx-add-pos-ge | abf | 0.365 | 0.05 | slower |
| cx-add-pos-ge | anm | 0.212 | 0.05 | slower |
| cx-add-pos-ge | b | 0.264 | 0.05 | slower |
| cx-add-pos-most | a | 0.199 | 0.05 | slower |
| cx-add-pos-most | abf | 0.353 | 0.05 | slower |
| cx-add-pos-most | anm | 0.2 | 0.05 | slower |
| cx-add-pos-most | b | 0.231 | 0.05 | slower |
| cx-add-pos-half | a | 0.179 | 0.05 | slower |
| cx-add-pos-half | abf | 0.369 | 0.05 | slower |
| cx-add-pos-half | anm | 0.161 | 0.05 | slower |
| cx-add-pos-half | b | 0.247 | 0.05 | slower |
| cx-add-most-ge | a | 0.219 | 0.052 | slower |
| cx-add-most-ge | abf | 0.378 | 0.052 | slower |
| cx-add-most-ge | anm | 0.237 | 0.052 | slower |
| cx-add-most-ge | b | 0.273 | 0.052 | slower |
| cx-add-most-most | a | 0.204 | 0.05 | slower |
| cx-add-most-most | abf | 0.353 | 0.05 | slower |
| cx-add-most-most | anm | 0.21 | 0.05 | slower |
| cx-add-most-most | b | 0.23 | 0.05 | slower |
| cx-add-most-half | a | 0.16 | 0.095 | slower |
| cx-add-most-half | abf | 0.354 | 0.095 | slower |
| cx-add-most-half | anm | 0.154 | 0.095 | slower |
| cx-add-most-half | b | 0.219 | 0.095 | slower |
| cx-add-rand-ge | a | 0.201 | 0.05 | slower |
| cx-add-rand-ge | abf | 0.36 | 0.05 | slower |
| cx-add-rand-ge | anm | 0.222 | 0.05 | slower |
| cx-add-rand-ge | b | 0.251 | 0.05 | slower |
| cx-add-rand-most | a | 0.193 | 0.05 | slower |
| cx-add-rand-most | abf | 0.369 | 0.05 | slower |
| cx-add-rand-most | anm | 0.195 | 0.05 | slower |
| cx-add-rand-most | b | 0.259 | 0.05 | slower |
| cx-add-rand-half | a | 0.133 | 0.05 | slower |
| cx-add-rand-half | abf | 0.356 | 0.05 | slower |
| cx-add-rand-half | anm | 0.154 | 0.05 | slower |
| cx-add-rand-half | b | 0.229 | 0.05 | slower |
| cx-sub-pos-ge | a | 0.197 | 0.084 | slower |
| cx-sub-pos-ge | abf | 0.362 | 0.084 | slower |
| cx-sub-pos-ge | anm | 0.19 | 0.084 | slower |
| cx-sub-pos-ge | b | 0.28 | 0.084 | slower |
| cx-sub-pos-most | a | 0.179 | 0.05 | slower |
| cx-sub-pos-most | abf | 0.342 | 0.05 | slower |
| cx-sub-pos-most | anm | 0.186 | 0.05 | slower |
| cx-sub-pos-most | b | 0.227 | 0.05 | slower |
| cx-sub-pos-half | a | 0.121 | 0.05 | slower |
| cx-sub-pos-half | abf | 0.339 | 0.05 | slower |
| cx-sub-pos-half | anm | 0.154 | 0.05 | slower |
| cx-sub-pos-half | b | 0.209 | 0.05 | slower |
| cx-sub-most-ge | a | 0.179 | 0.05 | slower |
| cx-sub-most-ge | abf | 0.335 | 0.05 | slower |
| cx-sub-most-ge | anm | 0.198 | 0.05 | slower |
| cx-sub-most-ge | b | 0.266 | 0.05 | slower |
| cx-sub-most-most | a | 0.167 | 0.115 | slower |
| cx-sub-most-most | abf | 0.329 | 0.115 | slower |
| cx-sub-most-most | anm | 0.193 | 0.115 | slower |
| cx-sub-most-most | b | 0.208 | 0.115 | slower |
| cx-sub-most-half | a | 0.128 | 0.066 | slower |
| cx-sub-most-half | abf | 0.351 | 0.066 | slower |
| cx-sub-most-half | anm | 0.162 | 0.066 | slower |
| cx-sub-most-half | b | 0.234 | 0.066 | slower |
| cx-sub-rand-ge | a | 0.183 | 0.05 | slower |
| cx-sub-rand-ge | abf | 0.336 | 0.05 | slower |
| cx-sub-rand-ge | anm | 0.213 | 0.05 | slower |
| cx-sub-rand-ge | b | 0.248 | 0.05 | slower |
| cx-sub-rand-most | a | 0.173 | 0.05 | slower |
| cx-sub-rand-most | abf | 0.33 | 0.05 | slower |
| cx-sub-rand-most | anm | 0.185 | 0.05 | slower |
| cx-sub-rand-most | b | 0.214 | 0.05 | slower |
| cx-sub-rand-half | a | 0.136 | 0.05 | slower |
| cx-sub-rand-half | abf | 0.336 | 0.05 | slower |
| cx-sub-rand-half | anm | 0.149 | 0.05 | slower |
| cx-sub-rand-half | b | 0.225 | 0.05 | slower |
| cx-neg-pos | a | 0.698 | 0.05 | slower |
| cx-neg-pos | abf | 0.692 | 0.05 | slower |
| cx-neg-pos | anm | 0.7 | 0.05 | slower |
| cx-neg-pos | b | 0.333 | 0.05 | slower |
| cx-neg-most | a | 0.697 | 0.05 | slower |
| cx-neg-most | abf | 0.689 | 0.05 | slower |
| cx-neg-most | anm | 0.696 | 0.05 | slower |
| cx-neg-most | b | 0.374 | 0.05 | slower |
| cx-neg-rand | a | 0.694 | 0.05 | slower |
| cx-neg-rand | abf | 0.689 | 0.05 | slower |
| cx-neg-rand | anm | 0.695 | 0.05 | slower |
| cx-neg-rand | b | 0.349 | 0.05 | slower |
| cx-mul-pos | a | 1.672 | 0.05 | faster |
| cx-mul-pos | abf | 1.659 | 0.05 | faster |
| cx-mul-pos | anm | 1.667 | 0.05 | faster |
| cx-mul-pos | b | 0.239 | 0.05 | slower |
| cx-mul-most | a | 1.615 | 0.05 | faster |
| cx-mul-most | abf | 1.618 | 0.05 | faster |
| cx-mul-most | anm | 1.618 | 0.05 | faster |
| cx-mul-most | b | 0.236 | 0.05 | slower |
| cx-mul-rand | a | 1.689 | 0.05 | faster |
| cx-mul-rand | abf | 1.684 | 0.05 | faster |
| cx-mul-rand | anm | 1.675 | 0.05 | faster |
| cx-mul-rand | b | 0.242 | 0.05 | slower |
| cx-div-pos | a | 2.046 | 0.05 | faster |
| cx-div-pos | abf | 2.049 | 0.05 | faster |
| cx-div-pos | anm | 2.031 | 0.05 | faster |
| cx-div-pos | b | 1.549 | 0.05 | faster |
| cx-div-most | a | 2.045 | 0.05 | faster |
| cx-div-most | abf | 2.043 | 0.05 | faster |
| cx-div-most | anm | 2.044 | 0.05 | faster |
| cx-div-most | b | 1.628 | 0.05 | faster |
| cx-div-rand | a | 2.002 | 0.05 | faster |
| cx-div-rand | abf | 1.991 | 0.05 | faster |
| cx-div-rand | anm | 1.991 | 0.05 | faster |
| cx-div-rand | b | 0.546 | 0.05 | slower |

## Superseded: the first run, 1024 operations per row

Nov 1, 2026, 22:33–22:40 local, after an idle check, no other work running. `python3 test/run.py`, spec 12's method: 1024 operations per row, 15 reps per cell, median ns per operation. All checksums agree with the exact values. Builds: Control, A (without `and D`, the author's change after round 2), A's branch-free row, B. C is not timed (author's call). Control was timed twice per row (`control`, `control-2`) as a noise check; the two medians agree within a few percent everywhere, but the spread (max − min over reps, divided by the median) is large in many cells, up to 111%, so single cells should be read with care.

### Summary (ns per operation, random signs; add and subtract with magnitude order random half the time)

| | Control | A | A branch-free | B |
| --- | --- | --- | --- | --- |
| Integer add | 0.23 | 0.86 | 2.02 | 0.44 |
| Integer subtract | 0.23 | 0.95 | 2.13 | 0.43 |
| Integer negate | 0.23 | 0.52 | 0.56 | 0.33 |
| Integer multiply | 0.26 | 0.52 | 0.54 | 1.06 |
| Integer divide | 2.67 | 2.61 | 2.61 | 2.64 |
| Complex add | 0.44 | 1.13 | 2.03 | 1.68 |
| Complex subtract | 0.45 | 1.19 | 2.17 | 1.67 |
| Complex negate | 0.40 | 0.52 | 0.52 | 0.80 |
| Complex multiply | 1.05 | 0.53 | 0.52 | 4.17 |
| Complex divide | 5.23 | 2.61 | 2.62 | 3.51 |

### Findings

1. A's branches beat A's branch-free row on every row. The branch-free row is flat at 2.0–2.2 ns, about 45 instructions per element in its loop; A's tree is 0.52 ns when every sign is positive and at most 0.96 ns in the worst mix (subtract, all positive, magnitude order half: the borrow branch is a coin toss).
2. Branch cost in A's integer add: 0.52 ns with predictable branches, 0.78–0.86 ns under random signs.
3. A's floor is about 0.52 ns, against Control's 0.23, on operations with no branch (negate, multiply): A reads and writes a header list beside the word list.
4. On complex numbers A beats Control at multiply (0.53 against 1.05) and divide (2.61 against 5.23, Control's two divides and its axis branch), and loses at add, subtract and negate.
5. Integer divide is the same in every build, about 2.6 ns: the divider sets the time.

### Full report


N = 1024 pairs per row, 15 reps per cell; median time per operation. Bytes per value: Control 8 (16 complex), A 9, B 16 (32 complex).

| row | build | ns/op | spread | Ir/op | bytes/value | sums |
| --- | --- | --- | --- | --- | --- | --- |
| int-add-pos-ge | control | 0.2297 | 15.1% | - | 8 | agree |
| int-add-pos-ge | a | 0.5225 | 46.1% | - | 9 | agree |
| int-add-pos-ge | abf | 2.023 | 7.2% | - | 9 | agree |
| int-add-pos-ge | b | 0.4426 | 5.6% | - | 16 | agree |
| int-add-pos-ge | control-2 | 0.2305 | 71.3% | - | 8 | agree |
| int-collapse-pos-ge | b | 0.4025 | 7.8% | - | 16 | agree |
| int-collapse-pos-ge | b-2 | 0.4015 | 32.9% | - | 16 | agree |
| int-add-pos-most | control | 0.2299 | 2.6% | - | 8 | agree |
| int-add-pos-most | a | 0.5242 | 104.8% | - | 9 | agree |
| int-add-pos-most | abf | 2.028 | 3.2% | - | 9 | agree |
| int-add-pos-most | b | 0.4524 | 2.8% | - | 16 | agree |
| int-add-pos-most | control-2 | 0.2343 | 7.0% | - | 8 | agree |
| int-collapse-pos-most | b | 0.404 | 2.8% | - | 16 | agree |
| int-collapse-pos-most | b-2 | 0.4029 | 2.7% | - | 16 | agree |
| int-add-pos-half | control | 0.2358 | 15.1% | - | 8 | agree |
| int-add-pos-half | a | 0.5212 | 79.5% | - | 9 | agree |
| int-add-pos-half | abf | 2.023 | 0.8% | - | 9 | agree |
| int-add-pos-half | b | 0.4401 | 2.9% | - | 16 | agree |
| int-add-pos-half | control-2 | 0.2288 | 1.7% | - | 8 | agree |
| int-collapse-pos-half | b | 0.401 | 0.8% | - | 16 | agree |
| int-collapse-pos-half | b-2 | 0.4017 | 4.4% | - | 16 | agree |
| int-add-most-ge | control | 0.2335 | 85.3% | - | 8 | agree |
| int-add-most-ge | a | 0.5604 | 6.7% | - | 9 | agree |
| int-add-most-ge | abf | 2.021 | 2.0% | - | 9 | agree |
| int-add-most-ge | b | 0.4386 | 2.0% | - | 16 | agree |
| int-add-most-ge | control-2 | 0.2294 | 2.2% | - | 8 | agree |
| int-collapse-most-ge | b | 0.5466 | 26.5% | - | 16 | agree |
| int-collapse-most-ge | b-2 | 0.5327 | 5.5% | - | 16 | agree |
| int-add-most-most | control | 0.2308 | 2.6% | - | 8 | agree |
| int-add-most-most | a | 0.5727 | 1.2% | - | 9 | agree |
| int-add-most-most | abf | 2.023 | 66.0% | - | 9 | agree |
| int-add-most-most | b | 0.4402 | 5.5% | - | 16 | agree |
| int-add-most-most | control-2 | 0.2309 | 4.2% | - | 8 | agree |
| int-collapse-most-most | b | 0.5214 | 1.8% | - | 16 | agree |
| int-collapse-most-most | b-2 | 0.5257 | 2.6% | - | 16 | agree |
| int-add-most-half | control | 0.2331 | 8.3% | - | 8 | agree |
| int-add-most-half | a | 0.5765 | 3.4% | - | 9 | agree |
| int-add-most-half | abf | 2.05 | 64.1% | - | 9 | agree |
| int-add-most-half | b | 0.4393 | 4.8% | - | 16 | agree |
| int-add-most-half | control-2 | 0.2292 | 5.6% | - | 8 | agree |
| int-collapse-most-half | b | 0.5319 | 9.4% | - | 16 | agree |
| int-collapse-most-half | b-2 | 0.5214 | 3.9% | - | 16 | agree |
| int-add-rand-ge | control | 0.2274 | 2.9% | - | 8 | agree |
| int-add-rand-ge | a | 0.7835 | 4.4% | - | 9 | agree |
| int-add-rand-ge | abf | 2.012 | 2.9% | - | 9 | agree |
| int-add-rand-ge | b | 0.4384 | 2.3% | - | 16 | agree |
| int-add-rand-ge | control-2 | 0.229 | 1.0% | - | 8 | agree |
| int-collapse-rand-ge | b | 0.5815 | 1.7% | - | 16 | agree |
| int-collapse-rand-ge | b-2 | 0.5836 | 3.6% | - | 16 | agree |
| int-add-rand-most | control | 0.241 | 20.0% | - | 8 | agree |
| int-add-rand-most | a | 0.8104 | 2.2% | - | 9 | agree |
| int-add-rand-most | abf | 2.051 | 21.9% | - | 9 | agree |
| int-add-rand-most | b | 0.4468 | 4.2% | - | 16 | agree |
| int-add-rand-most | control-2 | 0.232 | 59.5% | - | 8 | agree |
| int-collapse-rand-most | b | 0.5926 | 1.7% | - | 16 | agree |
| int-collapse-rand-most | b-2 | 0.5891 | 2.8% | - | 16 | agree |
| int-add-rand-half | control | 0.2326 | 3.7% | - | 8 | agree |
| int-add-rand-half | a | 0.8639 | 9.0% | - | 9 | agree |
| int-add-rand-half | abf | 2.019 | 2.0% | - | 9 | agree |
| int-add-rand-half | b | 0.4436 | 44.6% | - | 16 | agree |
| int-add-rand-half | control-2 | 0.2307 | 6.8% | - | 8 | agree |
| int-collapse-rand-half | b | 0.5921 | 4.8% | - | 16 | agree |
| int-collapse-rand-half | b-2 | 0.5916 | 2.2% | - | 16 | agree |
| int-sub-pos-ge | control | 0.2308 | 11.2% | - | 8 | agree |
| int-sub-pos-ge | a | 0.712 | 2.8% | - | 9 | agree |
| int-sub-pos-ge | abf | 2.131 | 1.4% | - | 9 | agree |
| int-sub-pos-ge | b | 0.4352 | 9.2% | - | 16 | agree |
| int-sub-pos-ge | control-2 | 0.2322 | 35.0% | - | 8 | agree |
| int-sub-pos-most | control | 0.2368 | 9.5% | - | 8 | agree |
| int-sub-pos-most | a | 0.7941 | 4.5% | - | 9 | agree |
| int-sub-pos-most | abf | 2.139 | 20.5% | - | 9 | agree |
| int-sub-pos-most | b | 0.4342 | 2.3% | - | 16 | agree |
| int-sub-pos-most | control-2 | 0.2288 | 3.3% | - | 8 | agree |
| int-sub-pos-half | control | 0.2306 | 4.1% | - | 8 | agree |
| int-sub-pos-half | a | 0.9479 | 2.7% | - | 9 | agree |
| int-sub-pos-half | abf | 2.136 | 19.0% | - | 9 | agree |
| int-sub-pos-half | b | 0.4359 | 4.5% | - | 16 | agree |
| int-sub-pos-half | control-2 | 0.2294 | 20.8% | - | 8 | agree |
| int-sub-most-ge | control | 0.2318 | 2.6% | - | 8 | agree |
| int-sub-most-ge | a | 0.6994 | 42.7% | - | 9 | agree |
| int-sub-most-ge | abf | 2.132 | 3.8% | - | 9 | agree |
| int-sub-most-ge | b | 0.4316 | 1.8% | - | 16 | agree |
| int-sub-most-ge | control-2 | 0.2288 | 2.2% | - | 8 | agree |
| int-sub-most-most | control | 0.2301 | 2.2% | - | 8 | agree |
| int-sub-most-most | a | 0.6865 | 1.5% | - | 9 | agree |
| int-sub-most-most | abf | 2.129 | 1.3% | - | 9 | agree |
| int-sub-most-most | b | 0.4346 | 12.0% | - | 16 | agree |
| int-sub-most-most | control-2 | 0.2285 | 2.2% | - | 8 | agree |
| int-sub-most-half | control | 0.2327 | 3.5% | - | 8 | agree |
| int-sub-most-half | a | 0.9591 | 4.2% | - | 9 | agree |
| int-sub-most-half | abf | 2.156 | 26.7% | - | 9 | agree |
| int-sub-most-half | b | 0.4362 | 4.4% | - | 16 | agree |
| int-sub-most-half | control-2 | 0.2288 | 5.0% | - | 8 | agree |
| int-sub-rand-ge | control | 0.2294 | 1.6% | - | 8 | agree |
| int-sub-rand-ge | a | 0.7677 | 3.0% | - | 9 | agree |
| int-sub-rand-ge | abf | 2.136 | 28.1% | - | 9 | agree |
| int-sub-rand-ge | b | 0.4378 | 3.5% | - | 16 | agree |
| int-sub-rand-ge | control-2 | 0.2288 | 3.3% | - | 8 | agree |
| int-sub-rand-most | control | 0.2316 | 2.5% | - | 8 | agree |
| int-sub-rand-most | a | 0.7946 | 16.4% | - | 9 | agree |
| int-sub-rand-most | abf | 2.137 | 4.0% | - | 9 | agree |
| int-sub-rand-most | b | 0.4346 | 3.4% | - | 16 | agree |
| int-sub-rand-most | control-2 | 0.2289 | 1.1% | - | 8 | agree |
| int-sub-rand-half | control | 0.2301 | 7.8% | - | 8 | agree |
| int-sub-rand-half | a | 0.948 | 8.1% | - | 9 | agree |
| int-sub-rand-half | abf | 2.126 | 1.2% | - | 9 | agree |
| int-sub-rand-half | b | 0.4327 | 5.5% | - | 16 | agree |
| int-sub-rand-half | control-2 | 0.2286 | 12.9% | - | 8 | agree |
| int-neg-pos | control | 0.2283 | 0.8% | - | 8 | agree |
| int-neg-pos | a | 0.5213 | 3.7% | - | 9 | agree |
| int-neg-pos | abf | 0.5235 | 3.2% | - | 9 | agree |
| int-neg-pos | b | 0.3314 | 50.1% | - | 16 | agree |
| int-neg-pos | control-2 | 0.2289 | 3.3% | - | 8 | agree |
| int-neg-most | control | 0.23 | 2.5% | - | 8 | agree |
| int-neg-most | a | 0.5249 | 12.9% | - | 9 | agree |
| int-neg-most | abf | 0.5215 | 1.9% | - | 9 | agree |
| int-neg-most | b | 0.3312 | 1.5% | - | 16 | agree |
| int-neg-most | control-2 | 0.2285 | 2.3% | - | 8 | agree |
| int-neg-rand | control | 0.2289 | 2.2% | - | 8 | agree |
| int-neg-rand | a | 0.5242 | 4.8% | - | 9 | agree |
| int-neg-rand | abf | 0.5636 | 31.9% | - | 9 | agree |
| int-neg-rand | b | 0.3332 | 9.8% | - | 16 | agree |
| int-neg-rand | control-2 | 0.2293 | 34.7% | - | 8 | agree |
| int-mul-pos | control | 0.2626 | 57.8% | - | 8 | agree |
| int-mul-pos | a | 0.5336 | 7.8% | - | 9 | agree |
| int-mul-pos | abf | 0.5228 | 3.1% | - | 9 | agree |
| int-mul-pos | b | 1.048 | 2.8% | - | 16 | agree |
| int-mul-pos | control-2 | 0.2632 | 21.2% | - | 8 | agree |
| int-mul-most | control | 0.2628 | 2.4% | - | 8 | agree |
| int-mul-most | a | 0.5244 | 4.2% | - | 9 | agree |
| int-mul-most | abf | 0.5243 | 5.7% | - | 9 | agree |
| int-mul-most | b | 1.045 | 3.8% | - | 16 | agree |
| int-mul-most | control-2 | 0.2635 | 10.6% | - | 8 | agree |
| int-mul-rand | control | 0.2639 | 13.5% | - | 8 | agree |
| int-mul-rand | a | 0.5234 | 0.8% | - | 9 | agree |
| int-mul-rand | abf | 0.5354 | 4.6% | - | 9 | agree |
| int-mul-rand | b | 1.056 | 34.7% | - | 16 | agree |
| int-mul-rand | control-2 | 0.2623 | 31.2% | - | 8 | agree |
| int-div-pos | control | 2.62 | 2.1% | - | 8 | agree |
| int-div-pos | a | 2.615 | 3.0% | - | 9 | agree |
| int-div-pos | abf | 2.605 | 3.9% | - | 9 | agree |
| int-div-pos | b | 2.607 | 1.7% | - | 16 | agree |
| int-div-pos | control-2 | 2.631 | 8.5% | - | 8 | agree |
| int-div-most | control | 2.613 | 4.7% | - | 8 | agree |
| int-div-most | a | 2.609 | 2.0% | - | 9 | agree |
| int-div-most | abf | 2.625 | 2.5% | - | 9 | agree |
| int-div-most | b | 2.626 | 6.3% | - | 16 | agree |
| int-div-most | control-2 | 2.607 | 2.7% | - | 8 | agree |
| int-div-rand | control | 2.665 | 7.8% | - | 8 | agree |
| int-div-rand | a | 2.613 | 1.4% | - | 9 | agree |
| int-div-rand | abf | 2.614 | 3.5% | - | 9 | agree |
| int-div-rand | b | 2.636 | 15.1% | - | 16 | agree |
| int-div-rand | control-2 | 2.605 | 3.3% | - | 8 | agree |
| cx-add-pos-ge | control | 0.4443 | 3.8% | - | 16 | agree |
| cx-add-pos-ge | a | 1.082 | 5.0% | - | 9 | agree |
| cx-add-pos-ge | abf | 2.044 | 10.2% | - | 9 | agree |
| cx-add-pos-ge | b | 1.386 | 4.1% | - | 32 | agree |
| cx-add-pos-ge | control-2 | 0.4449 | 4.9% | - | 16 | agree |
| cx-collapse-pos-ge | b | 0.8247 | 36.3% | - | 32 | agree |
| cx-collapse-pos-ge | b-2 | 0.8209 | 30.7% | - | 32 | agree |
| cx-add-pos-most | control | 0.4407 | 3.9% | - | 16 | agree |
| cx-add-pos-most | a | 1.143 | 13.5% | - | 9 | agree |
| cx-add-pos-most | abf | 2.028 | 23.4% | - | 9 | agree |
| cx-add-pos-most | b | 1.673 | 1.2% | - | 32 | agree |
| cx-add-pos-most | control-2 | 0.437 | 12.4% | - | 16 | agree |
| cx-collapse-pos-most | b | 0.8184 | 3.9% | - | 32 | agree |
| cx-collapse-pos-most | b-2 | 0.8172 | 3.6% | - | 32 | agree |
| cx-add-pos-half | control | 0.449 | 34.8% | - | 16 | agree |
| cx-add-pos-half | a | 1.046 | 7.0% | - | 9 | agree |
| cx-add-pos-half | abf | 2.018 | 1.7% | - | 9 | agree |
| cx-add-pos-half | b | 1.675 | 4.5% | - | 32 | agree |
| cx-add-pos-half | control-2 | 0.4372 | 2.2% | - | 16 | agree |
| cx-collapse-pos-half | b | 0.8278 | 3.1% | - | 32 | agree |
| cx-collapse-pos-half | b-2 | 0.835 | 4.0% | - | 32 | agree |
| cx-add-most-ge | control | 0.4411 | 92.8% | - | 16 | agree |
| cx-add-most-ge | a | 1.085 | 4.2% | - | 9 | agree |
| cx-add-most-ge | abf | 2.041 | 6.6% | - | 9 | agree |
| cx-add-most-ge | b | 1.379 | 2.1% | - | 32 | agree |
| cx-add-most-ge | control-2 | 0.4368 | 6.9% | - | 16 | agree |
| cx-collapse-most-ge | b | 0.9747 | 58.3% | - | 32 | agree |
| cx-collapse-most-ge | b-2 | 0.9633 | 3.7% | - | 32 | agree |
| cx-add-most-most | control | 0.4429 | 7.8% | - | 16 | agree |
| cx-add-most-most | a | 1.097 | 10.6% | - | 9 | agree |
| cx-add-most-most | abf | 2.026 | 23.7% | - | 9 | agree |
| cx-add-most-most | b | 1.649 | 1.3% | - | 32 | agree |
| cx-add-most-most | control-2 | 0.4521 | 35.9% | - | 16 | agree |
| cx-collapse-most-most | b | 0.9449 | 18.4% | - | 32 | agree |
| cx-collapse-most-most | b-2 | 0.9414 | 68.7% | - | 32 | agree |
| cx-add-most-half | control | 0.4541 | 5.9% | - | 16 | agree |
| cx-add-most-half | a | 1.078 | 17.2% | - | 9 | agree |
| cx-add-most-half | abf | 2.043 | 11.8% | - | 9 | agree |
| cx-add-most-half | b | 1.649 | 38.0% | - | 32 | agree |
| cx-add-most-half | control-2 | 0.4498 | 19.3% | - | 16 | agree |
| cx-collapse-most-half | b | 1.002 | 4.4% | - | 32 | agree |
| cx-collapse-most-half | b-2 | 1.006 | 43.4% | - | 32 | agree |
| cx-add-rand-ge | control | 0.4474 | 71.2% | - | 16 | agree |
| cx-add-rand-ge | a | 1.053 | 9.1% | - | 9 | agree |
| cx-add-rand-ge | abf | 2.021 | 32.3% | - | 9 | agree |
| cx-add-rand-ge | b | 1.379 | 3.8% | - | 32 | agree |
| cx-add-rand-ge | control-2 | 0.4672 | 12.8% | - | 16 | agree |
| cx-collapse-rand-ge | b | 0.9966 | 58.4% | - | 32 | agree |
| cx-collapse-rand-ge | b-2 | 0.9907 | 2.6% | - | 32 | agree |
| cx-add-rand-most | control | 0.44 | 18.7% | - | 16 | agree |
| cx-add-rand-most | a | 1.122 | 6.8% | - | 9 | agree |
| cx-add-rand-most | abf | 2.055 | 70.4% | - | 9 | agree |
| cx-add-rand-most | b | 1.672 | 3.1% | - | 32 | agree |
| cx-add-rand-most | control-2 | 0.4468 | 4.3% | - | 16 | agree |
| cx-collapse-rand-most | b | 0.9916 | 3.9% | - | 32 | agree |
| cx-collapse-rand-most | b-2 | 0.9743 | 25.7% | - | 32 | agree |
| cx-add-rand-half | control | 0.4403 | 5.2% | - | 16 | agree |
| cx-add-rand-half | a | 1.129 | 26.5% | - | 9 | agree |
| cx-add-rand-half | abf | 2.031 | 6.3% | - | 9 | agree |
| cx-add-rand-half | b | 1.675 | 5.8% | - | 32 | agree |
| cx-add-rand-half | control-2 | 0.4404 | 2.2% | - | 16 | agree |
| cx-collapse-rand-half | b | 0.9931 | 8.3% | - | 32 | agree |
| cx-collapse-rand-half | b-2 | 0.9958 | 69.4% | - | 32 | agree |
| cx-sub-pos-ge | control | 0.4504 | 101.3% | - | 16 | agree |
| cx-sub-pos-ge | a | 1.076 | 5.7% | - | 9 | agree |
| cx-sub-pos-ge | abf | 2.129 | 6.0% | - | 9 | agree |
| cx-sub-pos-ge | b | 1.347 | 3.1% | - | 32 | agree |
| cx-sub-pos-ge | control-2 | 0.4494 | 111.8% | - | 16 | agree |
| cx-sub-pos-most | control | 0.4496 | 27.2% | - | 16 | agree |
| cx-sub-pos-most | a | 1.134 | 4.9% | - | 9 | agree |
| cx-sub-pos-most | abf | 2.137 | 3.1% | - | 9 | agree |
| cx-sub-pos-most | b | 1.691 | 2.8% | - | 32 | agree |
| cx-sub-pos-most | control-2 | 0.4498 | 102.9% | - | 16 | agree |
| cx-sub-pos-half | control | 0.4462 | 13.8% | - | 16 | agree |
| cx-sub-pos-half | a | 1.249 | 2.3% | - | 9 | agree |
| cx-sub-pos-half | abf | 2.132 | 3.2% | - | 9 | agree |
| cx-sub-pos-half | b | 1.699 | 5.2% | - | 32 | agree |
| cx-sub-pos-half | control-2 | 0.4414 | 38.1% | - | 16 | agree |
| cx-sub-most-ge | control | 0.446 | 17.7% | - | 16 | agree |
| cx-sub-most-ge | a | 1.099 | 3.9% | - | 9 | agree |
| cx-sub-most-ge | abf | 2.169 | 6.7% | - | 9 | agree |
| cx-sub-most-ge | b | 1.377 | 38.6% | - | 32 | agree |
| cx-sub-most-ge | control-2 | 0.4363 | 2.8% | - | 16 | agree |
| cx-sub-most-most | control | 0.4348 | 23.7% | - | 16 | agree |
| cx-sub-most-most | a | 1.134 | 3.8% | - | 9 | agree |
| cx-sub-most-most | abf | 2.136 | 2.7% | - | 9 | agree |
| cx-sub-most-most | b | 1.664 | 1.5% | - | 32 | agree |
| cx-sub-most-most | control-2 | 0.434 | 9.3% | - | 16 | agree |
| cx-sub-most-half | control | 0.4365 | 7.6% | - | 16 | agree |
| cx-sub-most-half | a | 1.258 | 1.8% | - | 9 | agree |
| cx-sub-most-half | abf | 2.129 | 19.0% | - | 9 | agree |
| cx-sub-most-half | b | 1.676 | 5.5% | - | 32 | agree |
| cx-sub-most-half | control-2 | 0.4416 | 11.6% | - | 16 | agree |
| cx-sub-rand-ge | control | 0.438 | 10.5% | - | 16 | agree |
| cx-sub-rand-ge | a | 1.107 | 33.2% | - | 9 | agree |
| cx-sub-rand-ge | abf | 2.149 | 18.1% | - | 9 | agree |
| cx-sub-rand-ge | b | 1.355 | 8.6% | - | 32 | agree |
| cx-sub-rand-ge | control-2 | 0.4505 | 70.5% | - | 16 | agree |
| cx-sub-rand-most | control | 0.4504 | 7.1% | - | 16 | agree |
| cx-sub-rand-most | a | 1.14 | 5.1% | - | 9 | agree |
| cx-sub-rand-most | abf | 2.152 | 4.1% | - | 9 | agree |
| cx-sub-rand-most | b | 1.7 | 10.1% | - | 32 | agree |
| cx-sub-rand-most | control-2 | 0.4386 | 4.8% | - | 16 | agree |
| cx-sub-rand-half | control | 0.4504 | 80.1% | - | 16 | agree |
| cx-sub-rand-half | a | 1.19 | 1.3% | - | 9 | agree |
| cx-sub-rand-half | abf | 2.169 | 40.5% | - | 9 | agree |
| cx-sub-rand-half | b | 1.672 | 3.4% | - | 32 | agree |
| cx-sub-rand-half | control-2 | 0.4373 | 4.2% | - | 16 | agree |
| cx-neg-pos | control | 0.4473 | 20.5% | - | 16 | agree |
| cx-neg-pos | a | 0.5229 | 1.8% | - | 9 | agree |
| cx-neg-pos | abf | 0.5223 | 2.8% | - | 9 | agree |
| cx-neg-pos | b | 0.7984 | 1.3% | - | 32 | agree |
| cx-neg-pos | control-2 | 0.4377 | 35.9% | - | 16 | agree |
| cx-neg-most | control | 0.3777 | 4.6% | - | 16 | agree |
| cx-neg-most | a | 0.5206 | 0.8% | - | 9 | agree |
| cx-neg-most | abf | 0.5207 | 0.7% | - | 9 | agree |
| cx-neg-most | b | 0.7965 | 0.9% | - | 32 | agree |
| cx-neg-most | control-2 | 0.4051 | 66.3% | - | 16 | agree |
| cx-neg-rand | control | 0.4003 | 5.1% | - | 16 | agree |
| cx-neg-rand | a | 0.5231 | 40.5% | - | 9 | agree |
| cx-neg-rand | abf | 0.5229 | 3.4% | - | 9 | agree |
| cx-neg-rand | b | 0.8014 | 9.7% | - | 32 | agree |
| cx-neg-rand | control-2 | 0.4308 | 12.4% | - | 16 | agree |
| cx-mul-pos | control | 1.05 | 2.9% | - | 16 | agree |
| cx-mul-pos | a | 0.5323 | 6.3% | - | 9 | agree |
| cx-mul-pos | abf | 0.5215 | 2.2% | - | 9 | agree |
| cx-mul-pos | b | 4.148 | 0.9% | - | 32 | agree |
| cx-mul-pos | control-2 | 1.049 | 31.1% | - | 16 | agree |
| cx-mul-most | control | 1.045 | 0.8% | - | 16 | agree |
| cx-mul-most | a | 0.5221 | 0.9% | - | 9 | agree |
| cx-mul-most | abf | 0.5216 | 1.3% | - | 9 | agree |
| cx-mul-most | b | 4.169 | 2.1% | - | 32 | agree |
| cx-mul-most | control-2 | 1.048 | 22.0% | - | 16 | agree |
| cx-mul-rand | control | 1.046 | 3.6% | - | 16 | agree |
| cx-mul-rand | a | 0.5252 | 2.7% | - | 9 | agree |
| cx-mul-rand | abf | 0.522 | 1.5% | - | 9 | agree |
| cx-mul-rand | b | 4.173 | 1.2% | - | 32 | agree |
| cx-mul-rand | control-2 | 1.046 | 4.9% | - | 16 | agree |
| cx-div-pos | control | 5.235 | 7.2% | - | 16 | agree |
| cx-div-pos | a | 2.61 | 3.6% | - | 9 | agree |
| cx-div-pos | abf | 2.614 | 2.2% | - | 9 | agree |
| cx-div-pos | b | 3.402 | 1.6% | - | 32 | agree |
| cx-div-pos | control-2 | 5.229 | 1.1% | - | 16 | agree |
| cx-div-most | control | 5.216 | 0.4% | - | 16 | agree |
| cx-div-most | a | 2.606 | 1.5% | - | 9 | agree |
| cx-div-most | abf | 2.598 | 1.3% | - | 9 | agree |
| cx-div-most | b | 3.43 | 4.0% | - | 32 | agree |
| cx-div-most | control-2 | 5.245 | 11.1% | - | 16 | agree |
| cx-div-rand | control | 5.231 | 3.0% | - | 16 | agree |
| cx-div-rand | a | 2.613 | 1.6% | - | 9 | agree |
| cx-div-rand | abf | 2.621 | 1.7% | - | 9 | agree |
| cx-div-rand | b | 3.508 | 6.2% | - | 32 | agree |
| cx-div-rand | control-2 | 5.232 | 1.0% | - | 16 | agree |

| row | build x Control | T(Control)/T(build) | band | time |
| --- | --- | --- | --- | --- |
| int-add-pos-ge | a | 0.44 | 0.05 | slower |
| int-add-pos-ge | abf | 0.114 | 0.05 | slower |
| int-add-pos-ge | b | 0.519 | 0.05 | slower |
| int-add-pos-most | a | 0.439 | 0.05 | slower |
| int-add-pos-most | abf | 0.113 | 0.05 | slower |
| int-add-pos-most | b | 0.508 | 0.05 | slower |
| int-add-pos-half | a | 0.452 | 0.05 | slower |
| int-add-pos-half | abf | 0.117 | 0.05 | slower |
| int-add-pos-half | b | 0.536 | 0.05 | slower |
| int-add-most-ge | a | 0.417 | 0.05 | slower |
| int-add-most-ge | abf | 0.116 | 0.05 | slower |
| int-add-most-ge | b | 0.532 | 0.05 | slower |
| int-add-most-most | a | 0.403 | 0.05 | slower |
| int-add-most-most | abf | 0.114 | 0.05 | slower |
| int-add-most-most | b | 0.524 | 0.05 | slower |
| int-add-most-half | a | 0.404 | 0.05 | slower |
| int-add-most-half | abf | 0.114 | 0.05 | slower |
| int-add-most-half | b | 0.531 | 0.05 | slower |
| int-add-rand-ge | a | 0.29 | 0.05 | slower |
| int-add-rand-ge | abf | 0.113 | 0.05 | slower |
| int-add-rand-ge | b | 0.519 | 0.05 | slower |
| int-add-rand-most | a | 0.297 | 0.05 | slower |
| int-add-rand-most | abf | 0.117 | 0.05 | slower |
| int-add-rand-most | b | 0.539 | 0.05 | slower |
| int-add-rand-half | a | 0.269 | 0.05 | slower |
| int-add-rand-half | abf | 0.115 | 0.05 | slower |
| int-add-rand-half | b | 0.524 | 0.05 | slower |
| int-sub-pos-ge | a | 0.324 | 0.05 | slower |
| int-sub-pos-ge | abf | 0.108 | 0.05 | slower |
| int-sub-pos-ge | b | 0.53 | 0.05 | slower |
| int-sub-pos-most | a | 0.298 | 0.05 | slower |
| int-sub-pos-most | abf | 0.111 | 0.05 | slower |
| int-sub-pos-most | b | 0.545 | 0.05 | slower |
| int-sub-pos-half | a | 0.243 | 0.05 | slower |
| int-sub-pos-half | abf | 0.108 | 0.05 | slower |
| int-sub-pos-half | b | 0.529 | 0.05 | slower |
| int-sub-most-ge | a | 0.331 | 0.05 | slower |
| int-sub-most-ge | abf | 0.109 | 0.05 | slower |
| int-sub-most-ge | b | 0.537 | 0.05 | slower |
| int-sub-most-most | a | 0.335 | 0.05 | slower |
| int-sub-most-most | abf | 0.108 | 0.05 | slower |
| int-sub-most-most | b | 0.529 | 0.05 | slower |
| int-sub-most-half | a | 0.243 | 0.05 | slower |
| int-sub-most-half | abf | 0.108 | 0.05 | slower |
| int-sub-most-half | b | 0.533 | 0.05 | slower |
| int-sub-rand-ge | a | 0.299 | 0.05 | slower |
| int-sub-rand-ge | abf | 0.107 | 0.05 | slower |
| int-sub-rand-ge | b | 0.524 | 0.05 | slower |
| int-sub-rand-most | a | 0.291 | 0.05 | slower |
| int-sub-rand-most | abf | 0.108 | 0.05 | slower |
| int-sub-rand-most | b | 0.533 | 0.05 | slower |
| int-sub-rand-half | a | 0.243 | 0.05 | slower |
| int-sub-rand-half | abf | 0.108 | 0.05 | slower |
| int-sub-rand-half | b | 0.532 | 0.05 | slower |
| int-neg-pos | a | 0.438 | 0.05 | slower |
| int-neg-pos | abf | 0.436 | 0.05 | slower |
| int-neg-pos | b | 0.689 | 0.05 | slower |
| int-neg-most | a | 0.438 | 0.05 | slower |
| int-neg-most | abf | 0.441 | 0.05 | slower |
| int-neg-most | b | 0.694 | 0.05 | slower |
| int-neg-rand | a | 0.437 | 0.05 | slower |
| int-neg-rand | abf | 0.406 | 0.05 | slower |
| int-neg-rand | b | 0.687 | 0.05 | slower |
| int-mul-pos | a | 0.492 | 0.05 | slower |
| int-mul-pos | abf | 0.502 | 0.05 | slower |
| int-mul-pos | b | 0.251 | 0.05 | slower |
| int-mul-most | a | 0.501 | 0.05 | slower |
| int-mul-most | abf | 0.501 | 0.05 | slower |
| int-mul-most | b | 0.252 | 0.05 | slower |
| int-mul-rand | a | 0.504 | 0.05 | slower |
| int-mul-rand | abf | 0.493 | 0.05 | slower |
| int-mul-rand | b | 0.25 | 0.05 | slower |
| int-div-pos | a | 1.002 | 0.05 | within noise |
| int-div-pos | abf | 1.006 | 0.05 | within noise |
| int-div-pos | b | 1.005 | 0.05 | within noise |
| int-div-most | a | 1.001 | 0.05 | within noise |
| int-div-most | abf | 0.995 | 0.05 | within noise |
| int-div-most | b | 0.995 | 0.05 | within noise |
| int-div-rand | a | 1.02 | 0.05 | within noise |
| int-div-rand | abf | 1.019 | 0.05 | within noise |
| int-div-rand | b | 1.011 | 0.05 | within noise |
| cx-add-pos-ge | a | 0.411 | 0.05 | slower |
| cx-add-pos-ge | abf | 0.217 | 0.05 | slower |
| cx-add-pos-ge | b | 0.321 | 0.05 | slower |
| cx-add-pos-most | a | 0.386 | 0.05 | slower |
| cx-add-pos-most | abf | 0.217 | 0.05 | slower |
| cx-add-pos-most | b | 0.263 | 0.05 | slower |
| cx-add-pos-half | a | 0.429 | 0.05 | slower |
| cx-add-pos-half | abf | 0.223 | 0.05 | slower |
| cx-add-pos-half | b | 0.268 | 0.05 | slower |
| cx-add-most-ge | a | 0.406 | 0.05 | slower |
| cx-add-most-ge | abf | 0.216 | 0.05 | slower |
| cx-add-most-ge | b | 0.32 | 0.05 | slower |
| cx-add-most-most | a | 0.404 | 0.05 | slower |
| cx-add-most-most | abf | 0.219 | 0.05 | slower |
| cx-add-most-most | b | 0.269 | 0.05 | slower |
| cx-add-most-half | a | 0.421 | 0.05 | slower |
| cx-add-most-half | abf | 0.222 | 0.05 | slower |
| cx-add-most-half | b | 0.275 | 0.05 | slower |
| cx-add-rand-ge | a | 0.425 | 0.05 | slower |
| cx-add-rand-ge | abf | 0.221 | 0.05 | slower |
| cx-add-rand-ge | b | 0.325 | 0.05 | slower |
| cx-add-rand-most | a | 0.392 | 0.05 | slower |
| cx-add-rand-most | abf | 0.214 | 0.05 | slower |
| cx-add-rand-most | b | 0.263 | 0.05 | slower |
| cx-add-rand-half | a | 0.39 | 0.05 | slower |
| cx-add-rand-half | abf | 0.217 | 0.05 | slower |
| cx-add-rand-half | b | 0.263 | 0.05 | slower |
| cx-sub-pos-ge | a | 0.419 | 0.05 | slower |
| cx-sub-pos-ge | abf | 0.212 | 0.05 | slower |
| cx-sub-pos-ge | b | 0.334 | 0.05 | slower |
| cx-sub-pos-most | a | 0.396 | 0.05 | slower |
| cx-sub-pos-most | abf | 0.21 | 0.05 | slower |
| cx-sub-pos-most | b | 0.266 | 0.05 | slower |
| cx-sub-pos-half | a | 0.357 | 0.05 | slower |
| cx-sub-pos-half | abf | 0.209 | 0.05 | slower |
| cx-sub-pos-half | b | 0.263 | 0.05 | slower |
| cx-sub-most-ge | a | 0.406 | 0.05 | slower |
| cx-sub-most-ge | abf | 0.206 | 0.05 | slower |
| cx-sub-most-ge | b | 0.324 | 0.05 | slower |
| cx-sub-most-most | a | 0.383 | 0.05 | slower |
| cx-sub-most-most | abf | 0.204 | 0.05 | slower |
| cx-sub-most-most | b | 0.261 | 0.05 | slower |
| cx-sub-most-half | a | 0.347 | 0.05 | slower |
| cx-sub-most-half | abf | 0.205 | 0.05 | slower |
| cx-sub-most-half | b | 0.26 | 0.05 | slower |
| cx-sub-rand-ge | a | 0.396 | 0.05 | slower |
| cx-sub-rand-ge | abf | 0.204 | 0.05 | slower |
| cx-sub-rand-ge | b | 0.323 | 0.05 | slower |
| cx-sub-rand-most | a | 0.395 | 0.05 | slower |
| cx-sub-rand-most | abf | 0.209 | 0.05 | slower |
| cx-sub-rand-most | b | 0.265 | 0.05 | slower |
| cx-sub-rand-half | a | 0.379 | 0.05 | slower |
| cx-sub-rand-half | abf | 0.208 | 0.05 | slower |
| cx-sub-rand-half | b | 0.269 | 0.05 | slower |
| cx-neg-pos | a | 0.855 | 0.05 | slower |
| cx-neg-pos | abf | 0.856 | 0.05 | slower |
| cx-neg-pos | b | 0.56 | 0.05 | slower |
| cx-neg-most | a | 0.726 | 0.072 | slower |
| cx-neg-most | abf | 0.725 | 0.072 | slower |
| cx-neg-most | b | 0.474 | 0.072 | slower |
| cx-neg-rand | a | 0.765 | 0.076 | slower |
| cx-neg-rand | abf | 0.766 | 0.076 | slower |
| cx-neg-rand | b | 0.5 | 0.076 | slower |
| cx-mul-pos | a | 1.973 | 0.05 | faster |
| cx-mul-pos | abf | 2.014 | 0.05 | faster |
| cx-mul-pos | b | 0.253 | 0.05 | slower |
| cx-mul-most | a | 2.002 | 0.05 | faster |
| cx-mul-most | abf | 2.004 | 0.05 | faster |
| cx-mul-most | b | 0.251 | 0.05 | slower |
| cx-mul-rand | a | 1.993 | 0.05 | faster |
| cx-mul-rand | abf | 2.005 | 0.05 | faster |
| cx-mul-rand | b | 0.251 | 0.05 | slower |
| cx-div-pos | a | 2.006 | 0.05 | faster |
| cx-div-pos | abf | 2.003 | 0.05 | faster |
| cx-div-pos | b | 1.539 | 0.05 | faster |
| cx-div-most | a | 2.002 | 0.05 | faster |
| cx-div-most | abf | 2.008 | 0.05 | faster |
| cx-div-most | b | 1.521 | 0.05 | faster |
| cx-div-rand | a | 2.002 | 0.05 | faster |
| cx-div-rand | abf | 1.996 | 0.05 | faster |
| cx-div-rand | b | 1.491 | 0.05 | faster |

### Addendum: the narrow mask row (Nov 1, 2026, 23:16–23:21)

Added at the author's request: A's tree with its direction and axis branches, but no borrow branch; on opposite directions the difference is negated by the mask `(d xor (0 − b)) + b` and the direction turned by `h1 + (b shl 7)` (`aAddNM`, build `anm`). The check passes (491,229 results, 96 error cases, 288 checksums); its loop has no `jb`, the borrow is a `setb`. Add and subtract rows only, rerun for every build, after an idle check. ns per operation:

| row | Control | A | A narrow mask | A branch-free |
| --- | --- | --- | --- | --- |
| int-add-pos-ge | 0.23 | 0.52 | 0.52 | 2.04 |
| int-add-pos-most | 0.24 | 0.52 | 0.52 | 2.02 |
| int-add-pos-half | 0.24 | 0.52 | 0.52 | 2.02 |
| int-add-most-ge | 0.23 | 0.57 | 0.59 | 2.04 |
| int-add-most-most | 0.24 | 0.58 | 0.57 | 2.03 |
| int-add-most-half | 0.23 | 0.59 | 0.58 | 2.02 |
| int-add-rand-ge | 0.24 | 0.79 | 0.75 | 2.04 |
| int-add-rand-most | 0.23 | 0.80 | 0.76 | 2.03 |
| int-add-rand-half | 0.23 | 0.89 | 0.73 | 2.03 |
| int-sub-pos-ge | 0.23 | 0.71 | 1.12 | 2.14 |
| int-sub-pos-most | 0.23 | 0.79 | 1.13 | 2.17 |
| int-sub-pos-half | 0.23 | 0.95 | 1.12 | 2.13 |
| int-sub-most-ge | 0.24 | 0.75 | 1.04 | 2.13 |
| int-sub-most-most | 0.23 | 0.70 | 1.06 | 2.14 |
| int-sub-most-half | 0.23 | 0.96 | 1.08 | 2.14 |
| int-sub-rand-ge | 0.23 | 0.76 | 0.81 | 2.11 |
| int-sub-rand-most | 0.23 | 0.79 | 0.78 | 2.13 |
| int-sub-rand-half | 0.23 | 0.95 | 0.78 | 2.13 |
| cx-add-pos-ge | 0.44 | 1.07 | 0.78 | 2.03 |
| cx-add-pos-most | 0.43 | 1.13 | 0.79 | 2.02 |
| cx-add-pos-half | 0.45 | 1.09 | 0.94 | 2.03 |
| cx-add-most-ge | 0.45 | 1.09 | 0.79 | 2.04 |
| cx-add-most-most | 0.43 | 1.10 | 0.82 | 2.03 |
| cx-add-most-half | 0.44 | 1.08 | 0.95 | 2.05 |
| cx-add-rand-ge | 0.43 | 1.06 | 0.90 | 2.04 |
| cx-add-rand-most | 0.45 | 1.09 | 0.92 | 2.03 |
| cx-add-rand-half | 0.44 | 1.12 | 1.00 | 2.03 |
| cx-sub-pos-ge | 0.44 | 1.07 | 0.99 | 2.15 |
| cx-sub-pos-most | 0.44 | 1.13 | 0.97 | 2.13 |
| cx-sub-pos-half | 0.44 | 1.25 | 1.03 | 2.13 |
| cx-sub-most-ge | 0.44 | 1.10 | 0.94 | 2.14 |
| cx-sub-most-most | 0.45 | 1.13 | 0.96 | 2.15 |
| cx-sub-most-half | 0.43 | 1.25 | 1.02 | 2.14 |
| cx-sub-rand-ge | 0.43 | 1.07 | 0.89 | 2.14 |
| cx-sub-rand-most | 0.44 | 1.14 | 0.90 | 2.15 |
| cx-sub-rand-half | 0.44 | 1.19 | 1.05 | 2.20 |

6. The narrow mask row is faster than A's tree on every complex row (0.78–1.05 ns against 1.06–1.25) and on integer rows with random signs and a random borrow (add 0.73 against 0.89, subtract 0.78 against 0.95). It is slower when every operation takes the opposite path (integer subtract of positive operands: 1.04–1.13 ns against 0.70–0.96), even with the borrow a coin toss.
7. That last case suggests the branch predictor learns part of a "random" row: each row is the same 1024 operations replayed many times, so A's coin-toss borrow costs less than a true misprediction rate would. Rows with more operations, or fresh operands per pass, would test this.

## Addendum: the multiplication row (Nov 2, 2026, 00:37–00:48)

Added at the author's request: A's add with the notes' exponent rule as code. A sign is applied by multiplying by 1 − 2k (1 or −1 on the clock), and the cross-axis choice of direction by multiplying by a 0/1 flag: `r = m1 + m2·(1 − 2o)`, magnitude `r·(1 − 2b)`, direction `h1 + k·(h2 − h1) + (b shl 7)` (`aAddMul`, build `amul`). No branch, no cmov; the factors pass through `opaque` because gcc otherwise turned one multiply into a cmov. 52 instructions per add in the loop, against the branch-free row's 45. The check passes (587,131 results, 128 error cases, 342 checksums). Every row rerun for every build, 32,768 operations per row, after an idle check. Add and subtract, ns per operation:

| row | Control | A | narrow mask | branch-free (masks) | multiplication |
| --- | --- | --- | --- | --- | --- |
| int-add-pos-ge | 0.36 | 0.65 | 0.66 | 2.06 | 2.72 |
| int-add-pos-most | 0.35 | 0.65 | 0.69 | 2.04 | 2.72 |
| int-add-pos-half | 0.35 | 0.65 | 0.65 | 2.05 | 2.58 |
| int-add-most-ge | 0.35 | 1.00 | 0.97 | 2.05 | 2.64 |
| int-add-most-most | 0.35 | 1.00 | 1.00 | 2.05 | 2.78 |
| int-add-most-half | 0.35 | 0.99 | 0.98 | 2.04 | 2.76 |
| int-add-rand-ge | 0.36 | 3.51 | 3.40 | 2.04 | 2.80 |
| int-add-rand-most | 0.35 | 3.32 | 3.44 | 2.07 | 2.78 |
| int-add-rand-half | 0.35 | 4.17 | 3.40 | 2.04 | 2.48 |
| int-sub-pos-ge | 0.35 | 0.72 | 1.12 | 2.15 | 2.34 |
| int-sub-pos-most | 0.35 | 1.09 | 1.12 | 2.13 | 2.34 |
| int-sub-pos-half | 0.36 | 3.85 | 1.15 | 2.13 | 2.35 |
| int-sub-most-ge | 0.35 | 1.41 | 1.52 | 2.12 | 2.50 |
| int-sub-most-most | 0.35 | 1.00 | 1.50 | 2.16 | 2.33 |
| int-sub-most-half | 0.35 | 3.52 | 1.49 | 2.16 | 2.37 |
| int-sub-rand-ge | 0.35 | 3.74 | 3.50 | 2.12 | 2.35 |
| int-sub-rand-most | 0.35 | 3.39 | 3.45 | 2.14 | 2.35 |
| int-sub-rand-half | 0.36 | 4.46 | 3.47 | 2.15 | 2.44 |
| cx-add-pos-ge | 0.75 | 3.66 | 3.45 | 2.03 | 2.78 |
| cx-add-pos-most | 0.75 | 3.71 | 3.62 | 2.04 | 2.57 |
| cx-add-pos-half | 0.77 | 4.22 | 4.69 | 2.02 | 2.73 |
| cx-add-most-ge | 0.73 | 3.54 | 3.23 | 2.03 | 2.35 |
| cx-add-most-most | 0.75 | 3.60 | 3.46 | 2.03 | 2.66 |
| cx-add-most-half | 0.71 | 4.63 | 4.58 | 2.06 | 2.77 |
| cx-add-rand-ge | 0.73 | 3.44 | 3.45 | 2.06 | 2.42 |
| cx-add-rand-most | 0.73 | 3.82 | 3.63 | 2.07 | 2.74 |
| cx-add-rand-half | 0.73 | 5.42 | 4.44 | 2.02 | 2.77 |
| cx-sub-pos-ge | 0.73 | 3.94 | 4.14 | 2.14 | 2.32 |
| cx-sub-pos-most | 0.72 | 4.11 | 3.91 | 2.14 | 2.34 |
| cx-sub-pos-half | 0.75 | 6.19 | 4.71 | 2.14 | 2.34 |
| cx-sub-most-ge | 0.72 | 4.04 | 3.60 | 2.12 | 2.38 |
| cx-sub-most-most | 0.73 | 4.19 | 3.63 | 2.14 | 2.31 |
| cx-sub-most-half | 0.74 | 5.95 | 4.66 | 2.12 | 2.40 |
| cx-sub-rand-ge | 0.72 | 3.84 | 3.19 | 2.13 | 2.34 |
| cx-sub-rand-most | 0.73 | 4.05 | 3.82 | 2.18 | 2.35 |
| cx-sub-rand-half | 0.73 | 5.28 | 4.77 | 2.12 | 2.32 |

8. The multiplication row is flat, as the mask row is: 2.3–2.8 ns whatever the signs, axes or borrow. It is slower than the mask row (2.0–2.2 ns) everywhere: more instructions as compiled (52 against 45), and three multiplies per add. Negate, multiply and divide are A's shared routines, unchanged.
