# Spec 13 v0.2, round 3: timed trials

Nov 1, 2026, 22:33–22:40 local, after an idle check, no other work running. `python3 test/run.py`, spec 12's method: 1024 operations per row, 15 reps per cell, median ns per operation. All checksums agree with the exact values. Builds: Control, A (without `and D`, the author's change after round 2), A's branch-free row, B. C is not timed (author's call). Control was timed twice per row (`control`, `control-2`) as a noise check; the two medians agree within a few percent everywhere, but the spread (max − min over reps, divided by the median) is large in many cells, up to 111%, so single cells should be read with care.

## Summary (ns per operation, random signs; add and subtract with magnitude order random half the time)

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

## Findings

1. A's branches beat A's branch-free row on every row. The branch-free row is flat at 2.0–2.2 ns, about 45 instructions per element in its loop; A's tree is 0.52 ns when every sign is positive and at most 0.96 ns in the worst mix (subtract, all positive, magnitude order half: the borrow branch is a coin toss).
2. Branch cost in A's integer add: 0.52 ns with predictable branches, 0.78–0.86 ns under random signs.
3. A's floor is about 0.52 ns, against Control's 0.23, on operations with no branch (negate, multiply): A reads and writes a header list beside the word list.
4. On complex numbers A beats Control at multiply (0.53 against 1.05) and divide (2.61 against 5.23, Control's two divides and its axis branch), and loses at add, subtract and negate.
5. Integer divide is the same in every build, about 2.6 ns: the divider sets the time.

## Full report


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

## Addendum: the narrow mask row (Nov 1, 2026, 23:16–23:21)

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
