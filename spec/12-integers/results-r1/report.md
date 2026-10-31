# 12-integers timing

N = 1024, 15 reps per cell. Word builds: 8 bytes per value and 8 peak, no allocation (trivial).

| build | job | op | ns/op | spread | Ir/op | inexact | bytes/value acct (heap) | peak acct |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| control | 1 | add | 0.245 | 42.4% | 6.0 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | add | 0.536 | 57.5% | 14.0 | 2 | 8.0 (0.0) | 8.0 |
| even | 1 | add | 0.578 | 0.6% | 14.0 | 2 | 8.0 (0.0) | 8.0 |
| growing | 1 | add | 8.8 | 2.1% | 210.0 | 0 | 18.7 (24.0) | 24.0 |
| bytes | 1 | add | 54.1 | 1.1% | 925.1 | 0 | 6.0 (24.0) | 62.8 |
| control-2 | 1 | add | 0.244 | 9.0% | 6.0 | 0 | 8.0 (0.0) | 8.0 |
| control | 1 | sub | 0.232 | 2.0% | 6.0 | 546 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | sub | 0.536 | 2.6% | 14.0 | 546 | 8.0 (0.0) | 8.0 |
| even | 1 | sub | 0.572 | 3.1% | 14.0 | 546 | 8.0 (0.0) | 8.0 |
| growing | 1 | sub | 8.76 | 1.0% | 202.0 | 546 | 16.0 (24.1) | 16.0 |
| bytes | 1 | sub | 55.3 | 0.6% | 930.7 | 546 | 6.3 (24.0) | 55.9 |
| control-2 | 1 | sub | 0.234 | 1.4% | 6.0 | 546 | 8.0 (0.0) | 8.0 |
| control | 1 | absdiff | 0.562 | 51.1% | 12.0 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | absdiff | 0.856 | 3.4% | 21.5 | 0 | 8.0 (0.0) | 8.0 |
| even | 1 | absdiff | 0.785 | 5.4% | 16.5 | 0 | 8.0 (0.0) | 8.0 |
| growing | 1 | absdiff | 13.9 | 2.6% | 223.1 | 0 | 16.0 (24.0) | 16.0 |
| bytes | 1 | absdiff | 54.1 | 62.3% | 937.8 | 0 | 5.9 (24.0) | 54.8 |
| control-2 | 1 | absdiff | 0.56 | 48.4% | 12.0 | 0 | 8.0 (0.0) | 8.0 |
| control | 1 | mul | 0.272 | 16.2% | 6.0 | 497 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | mul | 0.825 | 0.9% | 19.5 | 513 | 8.0 (0.0) | 8.0 |
| even | 1 | mul | 0.576 | 1.9% | 14.0 | 513 | 8.0 (0.0) | 8.0 |
| growing | 1 | mul | 19.4 | 1.1% | 233.6 | 0 | 18.7 (24.0) | 24.0 |
| bytes | 1 | mul | 67 | 1.8% | 976.1 | 0 | 6.7 (24.0) | 65.4 |
| control-2 | 1 | mul | 0.308 | 6.5% | 6.0 | 497 | 8.0 (0.0) | 8.0 |
| control | 1 | divexact | 2.61 | 0.7% | 7.0 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | divexact | 2.25 | 1.4% | 21.1 | 0 | 8.0 (0.0) | 8.0 |
| even | 1 | divexact | 2.6 | 2.5% | 14.0 | 0 | 8.0 (0.0) | 8.0 |
| growing | 1 | divexact | 31.2 | 34.5% | 529.9 | 0 | 16.0 (24.0) | 32.0 |
| bytes | 1 | divexact | 77.2 | 7.1% | 1182.8 | 0 | 4.1 (24.0) | 64.0 |
| control-2 | 1 | divexact | 2.61 | 3.3% | 7.0 | 0 | 8.0 (0.0) | 8.0 |
| control | 1 | gcd | 9.29 | 4.9% | 122.1 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | gcd | 8.49 | 3.3% | 121.9 | 0 | 8.0 (0.0) | 8.0 |
| even | 1 | gcd | 9.28 | 1.8% | 129.9 | 0 | 8.0 (0.0) | 8.0 |
| growing | 1 | gcd | 140 | 1.4% | 1291.0 | 0 | 18.7 (24.0) | 40.0 |
| bytes | 1 | gcd | 193 | 3.5% | 1965.2 | 0 | 4.7 (24.0) | 72.0 |
| control-2 | 1 | gcd | 9.43 | 22.8% | 122.1 | 0 | 8.0 (0.0) | 8.0 |
| control | 2 | add | 1.29 | 6.4% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | add | 0.586 | 2.8% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | add | 0.592 | 2.1% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | add | 10.1 | 5.7% | 226.0 | 0 | 26.7 (29.4) | 32.0 |
| bytes | 2 | add | 73.3 | 12.5% | 1203.1 | 0 | 13.9 (24.0) | 94.8 |
| control-2 | 2 | add | 0.245 | 8.5% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 2 | sub | 0.231 | 49.5% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | sub | 0.566 | 1.3% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | sub | 0.575 | 0.8% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | sub | 9.07 | 1.5% | 223.0 | 535 | 24.0 (24.0) | 24.0 |
| bytes | 2 | sub | 74.2 | 1.1% | 1213.5 | 535 | 14.3 (24.0) | 88.0 |
| control-2 | 2 | sub | 0.232 | 85.0% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 2 | absdiff | 0.492 | 8.2% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | absdiff | 0.871 | 3.5% | 21.5 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | absdiff | 0.78 | 4.6% | 16.5 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | absdiff | 15.4 | 3.1% | 244.1 | 0 | 24.0 (24.0) | 24.0 |
| bytes | 2 | absdiff | 74.4 | 1.0% | 1220.9 | 0 | 13.9 (24.0) | 86.8 |
| control-2 | 2 | absdiff | 0.487 | 0.8% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 2 | mul | 0.267 | 1.4% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | mul | 0.826 | 2.6% | 18.3 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | mul | 0.576 | 1.2% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | mul | 22.8 | 2.4% | 290.3 | 0 | 29.3 (29.4) | 40.0 |
| bytes | 2 | mul | 93.9 | 1.2% | 1390.5 | 0 | 17.7 (27.5) | 113.8 |
| control-2 | 2 | mul | 0.267 | 0.5% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 2 | divexact | 2.61 | 2.3% | 7.0 | 1016 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| even | 2 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| growing | 2 | divexact | 37.2 | 41.4% | 603.1 | 0 | 21.1 (24.0) | 47.2 |
| bytes | 2 | divexact | 96 | 4.6% | 1432.0 | 0 | 9.5 (24.0) | 87.2 |
| control-2 | 2 | divexact | 2.64 | 6.3% | 7.0 | 1016 | 8.0 (0.0) | 8.0 |
| control | 2 | gcd | 133 | 7.0% | 384.7 | 981 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | gcd | 132 | 7.7% | 362.5 | 987 | 8.0 (0.0) | 8.0 |
| even | 2 | gcd | 125 | 6.7% | 385.9 | 987 | 8.0 (0.0) | 8.0 |
| growing | 2 | gcd | 378 | 1.1% | 3493.7 | 0 | 24.0 (24.1) | 55.9 |
| bytes | 2 | gcd | 452 | 7.0% | 4386.9 | 0 | 11.5 (24.0) | 103.7 |
| control-2 | 2 | gcd | 134 | 5.7% | 384.7 | 981 | 8.0 (0.0) | 8.0 |
| control | 4 | add | 0.246 | 0.9% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | add | 0.576 | 1.0% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | add | 0.585 | 50.4% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | add | 12.2 | 34.1% | 258.0 | 0 | 42.7 (45.4) | 48.0 |
| bytes | 4 | add | 114 | 34.9% | 1765.8 | 0 | 30.0 (40.0) | 158.9 |
| control-2 | 4 | add | 0.246 | 37.3% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | sub | 0.245 | 1.4% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | sub | 0.561 | 2.0% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | sub | 0.582 | 1.1% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | sub | 10.6 | 1.3% | 265.0 | 510 | 40.0 (40.0) | 40.0 |
| bytes | 4 | sub | 127 | 33.3% | 1784.8 | 510 | 30.3 (40.0) | 151.9 |
| control-2 | 4 | sub | 0.246 | 28.1% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | absdiff | 0.486 | 8.4% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | absdiff | 0.87 | 55.3% | 21.5 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | absdiff | 0.785 | 18.1% | 16.5 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | absdiff | 16 | 1.0% | 286.0 | 0 | 40.0 (40.0) | 40.0 |
| bytes | 4 | absdiff | 128 | 1.4% | 1793.5 | 0 | 30.0 (40.0) | 150.8 |
| control-2 | 4 | absdiff | 0.486 | 1.0% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | mul | 0.27 | 6.1% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | mul | 0.748 | 2.8% | 18.2 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | mul | 0.578 | 7.5% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | mul | 28.4 | 29.9% | 492.5 | 0 | 50.7 (50.7) | 72.0 |
| bytes | 4 | mul | 158 | 5.3% | 2294.5 | 0 | 38.9 (48.6) | 209.6 |
| control-2 | 4 | mul | 0.268 | 1.3% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| pow2 | 4 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| even | 4 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| growing | 4 | divexact | 48.6 | 20.0% | 767.4 | 0 | 31.7 (34.2) | 79.2 |
| bytes | 4 | divexact | 128 | 1.9% | 1951.1 | 0 | 20.2 (29.4) | 143.2 |
| control-2 | 4 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| control | 4 | gcd | 131 | 14.8% | 357.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | gcd | 124 | 7.8% | 340.8 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | gcd | 118 | 9.3% | 358.8 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | gcd | 922 | 1.2% | 10990.3 | 0 | 37.3 (40.0) | 96.0 |
| bytes | 4 | gcd | 1.03e+03 | 4.0% | 12326.8 | 0 | 24.8 (34.7) | 175.9 |
| control-2 | 4 | gcd | 127 | 5.8% | 357.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | add | 0.245 | 26.7% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | add | 0.555 | 2.6% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | add | 0.59 | 67.5% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | add | 13.8 | 2.0% | 322.0 | 0 | 74.7 (77.3) | 80.0 |
| bytes | 8 | add | 207 | 0.6% | 2887.5 | 0 | 62.0 (72.0) | 286.9 |
| control-2 | 8 | add | 0.245 | 1.3% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | sub | 0.245 | 1.3% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | sub | 0.563 | 4.1% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | sub | 0.577 | 3.0% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | sub | 13.7 | 40.8% | 349.0 | 523 | 72.0 (72.0) | 72.0 |
| bytes | 8 | sub | 211 | 11.6% | 2925.9 | 523 | 62.4 (72.0) | 280.0 |
| control-2 | 8 | sub | 0.245 | 1.1% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | absdiff | 0.486 | 1.2% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | absdiff | 0.848 | 2.5% | 21.5 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | absdiff | 0.785 | 1.1% | 16.5 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | absdiff | 20.2 | 2.2% | 370.0 | 0 | 72.0 (72.0) | 72.0 |
| bytes | 8 | absdiff | 214 | 1.6% | 2935.2 | 0 | 62.0 (72.0) | 278.9 |
| control-2 | 8 | absdiff | 0.489 | 1.3% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | mul | 0.266 | 2.4% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | mul | 0.802 | 2.2% | 18.2 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | mul | 0.576 | 0.9% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | mul | 68.2 | 3.1% | 1263.5 | 0 | 93.3 (93.3) | 136.0 |
| bytes | 8 | mul | 310 | 0.5% | 4472.9 | 0 | 81.5 (91.3) | 401.5 |
| control-2 | 8 | mul | 0.267 | 1.5% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| pow2 | 8 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| even | 8 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| growing | 8 | divexact | 72.2 | 1.2% | 1242.1 | 0 | 53.1 (55.6) | 143.3 |
| bytes | 8 | divexact | 205 | 37.2% | 3129.2 | 0 | 41.5 (50.7) | 255.2 |
| control-2 | 8 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| control | 8 | gcd | 119 | 13.0% | 338.1 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | gcd | 116 | 8.5% | 324.9 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | gcd | 115 | 9.0% | 342.1 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | gcd | 2.47e+03 | 0.6% | 37604.4 | 0 | 64.0 (66.7) | 176.0 |
| bytes | 8 | gcd | 2.68e+03 | 1.3% | 39822.4 | 0 | 51.5 (61.4) | 319.9 |
| control-2 | 8 | gcd | 120 | 7.8% | 338.1 | 1024 | 8.0 (0.0) | 8.0 |
| control | 16 | add | 0.245 | 46.4% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | add | 0.56 | 0.9% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 16 | add | 0.589 | 0.9% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 16 | add | 17.6 | 61.2% | 450.0 | 0 | 138.7 (141.3) | 144.0 |
| bytes | 16 | add | 372 | 1.0% | 5147.1 | 0 | 126.0 (136.0) | 543.0 |
| control-2 | 16 | add | 0.244 | 0.3% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 16 | sub | 0.231 | 1.6% | 6.0 | 1023 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | sub | 0.554 | 0.8% | 14.0 | 1023 | 8.0 (0.0) | 8.0 |
| even | 16 | sub | 0.576 | 0.8% | 14.0 | 1023 | 8.0 (0.0) | 8.0 |
| growing | 16 | sub | 19.6 | 0.8% | 517.1 | 511 | 136.0 (136.0) | 136.0 |
| bytes | 16 | sub | 377 | 0.7% | 5236.6 | 511 | 126.5 (136.0) | 536.3 |
| control-2 | 16 | sub | 0.231 | 0.9% | 6.0 | 1023 | 8.0 (0.0) | 8.0 |
| control | 16 | absdiff | 0.488 | 0.7% | 12.0 | 1023 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | absdiff | 0.865 | 2.7% | 21.5 | 1023 | 8.0 (0.0) | 8.0 |
| even | 16 | absdiff | 0.791 | 2.5% | 16.5 | 1023 | 8.0 (0.0) | 8.0 |
| growing | 16 | absdiff | 26.7 | 1.2% | 538.2 | 0 | 136.0 (136.0) | 136.0 |
| bytes | 16 | absdiff | 374 | 0.9% | 5233.6 | 0 | 126.0 (136.0) | 534.8 |
| control-2 | 16 | absdiff | 0.488 | 0.9% | 12.0 | 1023 | 8.0 (0.0) | 8.0 |
| control | 16 | mul | 0.266 | 0.9% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | mul | 0.818 | 1.7% | 18.3 | 1024 | 8.0 (0.0) | 8.0 |
| even | 16 | mul | 0.577 | 0.6% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 16 | mul | 205 | 8.7% | 4238.4 | 0 | 178.7 (178.7) | 264.0 |
| bytes | 16 | mul | 667 | 4.6% | 10313.6 | 0 | 167.3 (177.2) | 786.6 |
| control-2 | 16 | mul | 0.271 | 19.7% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 16 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| pow2 | 16 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| even | 16 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| growing | 16 | divexact | 132 | 59.8% | 2741.3 | 0 | 95.7 (98.2) | 271.1 |
| bytes | 16 | divexact | 379 | 0.8% | 6042.6 | 0 | 84.1 (93.4) | 479.1 |
| control-2 | 16 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| control | 16 | gcd | 122 | 6.3% | 346.3 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | gcd | 121 | 9.4% | 330.6 | 1024 | 8.0 (0.0) | 8.0 |
| even | 16 | gcd | 116 | 11.1% | 348.2 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 16 | gcd | 7.73e+03 | 1.7% | 139150.9 | 0 | 117.4 (120.1) | 336.3 |
| bytes | 16 | gcd | 8.29e+03 | 2.8% | 143150.1 | 0 | 105.0 (114.8) | 608.2 |
| control-2 | 16 | gcd | 122 | 7.6% | 346.3 | 1024 | 8.0 (0.0) | 8.0 |

| pairing | job | op | ref | row | T(ref)/T(row) | band | time | bytes ratio | peak ratio |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| build x Control | 1 | add | control | pow2 | 0.457 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | add | control | even | 0.424 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | add | control | growing | 0.028 | 0.05 | slower | 2.333 | 3.000 |
| build x Control | 1 | add | control | bytes | 0.005 | 0.05 | slower | 0.744 | 7.852 |
| build x Control | 1 | sub | control | pow2 | 0.434 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | sub | control | even | 0.406 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | sub | control | growing | 0.027 | 0.05 | slower | 2.000 | 2.000 |
| build x Control | 1 | sub | control | bytes | 0.004 | 0.05 | slower | 0.791 | 6.993 |
| build x Control | 1 | absdiff | control | pow2 | 0.657 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | absdiff | control | even | 0.716 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | absdiff | control | growing | 0.041 | 0.05 | slower | 2.000 | 2.000 |
| build x Control | 1 | absdiff | control | bytes | 0.01 | 0.05 | slower | 0.743 | 6.849 |
| build x Control | 1 | mul | control | pow2 | 0.33 | 0.131 | slower | 1.000 | 1.000 |
| build x Control | 1 | mul | control | even | 0.472 | 0.131 | slower | 1.000 | 1.000 |
| build x Control | 1 | mul | control | growing | 0.014 | 0.131 | slower | 2.333 | 3.000 |
| build x Control | 1 | mul | control | bytes | 0.004 | 0.131 | slower | 0.844 | 8.172 |
| build x Control | 1 | divexact | control | pow2 | 1.162 | 0.05 | faster | 1.000 | 1.000 |
| build x Control | 1 | divexact | control | even | 1.004 | 0.05 | within noise | 1.000 | 1.000 |
| build x Control | 1 | divexact | control | growing | 0.084 | 0.05 | slower | 2.000 | 4.000 |
| build x Control | 1 | divexact | control | bytes | 0.034 | 0.05 | slower | 0.511 | 8.000 |
| build x Control | 1 | gcd | control | pow2 | 1.095 | 0.05 | faster | 1.000 | 1.000 |
| build x Control | 1 | gcd | control | even | 1.002 | 0.05 | within noise | 1.000 | 1.000 |
| build x Control | 1 | gcd | control | growing | 0.066 | 0.05 | slower | 2.333 | 5.000 |
| build x Control | 1 | gcd | control | bytes | 0.048 | 0.05 | slower | 0.592 | 9.000 |
| Byte encoding x Growing | 2 | add | growing | bytes | 0.138 | 4.254 | within noise | 0.523 | 2.962 |
| Byte encoding x Growing | 2 | sub | growing | bytes | 0.122 | 0.05 | slower | 0.598 | 3.667 |
| Byte encoding x Growing | 2 | absdiff | growing | bytes | 0.207 | 0.05 | slower | 0.581 | 3.615 |
| Byte encoding x Growing | 2 | mul | growing | bytes | 0.243 | 0.05 | slower | 0.602 | 2.844 |
| Byte encoding x Growing | 2 | divexact | growing | bytes | 0.388 | 0.05 | slower | 0.449 | 1.846 |
| Byte encoding x Growing | 2 | gcd | growing | bytes | 0.837 | 0.05 | slower | 0.478 | 1.856 |
| Byte encoding x Growing | 4 | add | growing | bytes | 0.106 | 0.05 | slower | 0.702 | 3.310 |
| Byte encoding x Growing | 4 | sub | growing | bytes | 0.083 | 0.05 | slower | 0.758 | 3.799 |
| Byte encoding x Growing | 4 | absdiff | growing | bytes | 0.125 | 0.05 | slower | 0.749 | 3.771 |
| Byte encoding x Growing | 4 | mul | growing | bytes | 0.18 | 0.05 | slower | 0.767 | 2.911 |
| Byte encoding x Growing | 4 | divexact | growing | bytes | 0.378 | 0.05 | slower | 0.635 | 1.807 |
| Byte encoding x Growing | 4 | gcd | growing | bytes | 0.898 | 0.05 | slower | 0.664 | 1.832 |
| Byte encoding x Growing | 8 | add | growing | bytes | 0.067 | 0.05 | slower | 0.831 | 3.587 |
| Byte encoding x Growing | 8 | sub | growing | bytes | 0.065 | 0.05 | slower | 0.866 | 3.888 |
| Byte encoding x Growing | 8 | absdiff | growing | bytes | 0.094 | 0.05 | slower | 0.861 | 3.874 |
| Byte encoding x Growing | 8 | mul | growing | bytes | 0.22 | 0.05 | slower | 0.873 | 2.952 |
| Byte encoding x Growing | 8 | divexact | growing | bytes | 0.353 | 0.05 | slower | 0.781 | 1.781 |
| Byte encoding x Growing | 8 | gcd | growing | bytes | 0.919 | 0.05 | slower | 0.804 | 1.817 |
| Byte encoding x Growing | 16 | add | growing | bytes | 0.047 | 0.05 | slower | 0.909 | 3.771 |
| Byte encoding x Growing | 16 | sub | growing | bytes | 0.052 | 0.05 | slower | 0.930 | 3.943 |
| Byte encoding x Growing | 16 | absdiff | growing | bytes | 0.071 | 0.05 | slower | 0.926 | 3.933 |
| Byte encoding x Growing | 16 | mul | growing | bytes | 0.307 | 0.05 | slower | 0.936 | 2.980 |
| Byte encoding x Growing | 16 | divexact | growing | bytes | 0.348 | 0.05 | slower | 0.879 | 1.767 |
| Byte encoding x Growing | 16 | gcd | growing | bytes | 0.932 | 0.05 | slower | 0.894 | 1.808 |
| absdiff x sub | 1 | - | sub | control absdiff | 0.414 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 2 | - | sub | control absdiff | 0.47 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 4 | - | sub | control absdiff | 0.505 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 8 | - | sub | control absdiff | 0.503 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 16 | - | sub | control absdiff | 0.472 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 1 | - | sub | pow2 absdiff | 0.626 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 2 | - | sub | pow2 absdiff | 0.649 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 4 | - | sub | pow2 absdiff | 0.645 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 8 | - | sub | pow2 absdiff | 0.664 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 16 | - | sub | pow2 absdiff | 0.64 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 1 | - | sub | even absdiff | 0.729 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 2 | - | sub | even absdiff | 0.737 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 4 | - | sub | even absdiff | 0.741 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 8 | - | sub | even absdiff | 0.735 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 16 | - | sub | even absdiff | 0.728 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 1 | - | sub | growing absdiff | 0.631 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 2 | - | sub | growing absdiff | 0.589 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 4 | - | sub | growing absdiff | 0.663 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 8 | - | sub | growing absdiff | 0.68 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 16 | - | sub | growing absdiff | 0.732 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 1 | - | sub | bytes absdiff | 1.023 | 0.05 | within noise | 0.939 | 0.979 |
| absdiff x sub | 2 | - | sub | bytes absdiff | 0.997 | 0.05 | within noise | 0.971 | 0.986 |
| absdiff x sub | 4 | - | sub | bytes absdiff | 1.0 | 0.05 | within noise | 0.988 | 0.993 |
| absdiff x sub | 8 | - | sub | bytes absdiff | 0.988 | 0.05 | within noise | 0.994 | 0.996 |
| absdiff x sub | 16 | - | sub | bytes absdiff | 1.006 | 0.05 | within noise | 0.996 | 0.997 |
