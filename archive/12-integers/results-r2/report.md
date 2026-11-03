# 12-integers timing

N = 1024, 15 reps per cell. Word builds: 8 bytes per value and 8 peak, no allocation (trivial).

| build | job | op | ns/op | spread | Ir/op | inexact | bytes/value acct (heap) | peak acct |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| control | 1 | add | 0.23 | 3.1% | 6.0 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | add | 0.537 | 1.1% | 14.0 | 2 | 8.0 (0.0) | 8.0 |
| even | 1 | add | 0.578 | 1.3% | 14.0 | 2 | 8.0 (0.0) | 8.0 |
| growing | 1 | add | 8.87 | 2.1% | 209.0 | 0 | 18.7 (24.0) | 24.0 |
| bytes | 1 | add | 43.7 | 41.0% | 663.1 | 0 | 6.0 (24.0) | 62.8 |
| control-2 | 1 | add | 0.233 | 1.7% | 6.0 | 0 | 8.0 (0.0) | 8.0 |
| control | 1 | sub | 0.232 | 1.8% | 6.0 | 546 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | sub | 0.532 | 44.7% | 14.0 | 546 | 8.0 (0.0) | 8.0 |
| even | 1 | sub | 0.576 | 1.0% | 14.0 | 546 | 8.0 (0.0) | 8.0 |
| growing | 1 | sub | 8.4 | 1.2% | 204.0 | 546 | 16.0 (24.0) | 16.0 |
| bytes | 1 | sub | 36.3 | 1.1% | 671.7 | 546 | 6.3 (24.0) | 55.9 |
| control-2 | 1 | sub | 0.231 | 2.3% | 6.0 | 546 | 8.0 (0.0) | 8.0 |
| control | 1 | absdiff | 0.505 | 3.6% | 12.0 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | absdiff | 0.858 | 22.9% | 21.5 | 0 | 8.0 (0.0) | 8.0 |
| even | 1 | absdiff | 0.757 | 1.8% | 16.5 | 0 | 8.0 (0.0) | 8.0 |
| growing | 1 | absdiff | 14.5 | 1.6% | 225.1 | 0 | 16.0 (24.0) | 16.0 |
| bytes | 1 | absdiff | 44.6 | 1.4% | 678.8 | 0 | 5.9 (24.0) | 54.8 |
| control-2 | 1 | absdiff | 0.533 | 12.1% | 12.0 | 0 | 8.0 (0.0) | 8.0 |
| control | 1 | mul | 0.275 | 12.5% | 6.0 | 497 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | mul | 0.697 | 5.5% | 17.0 | 513 | 8.0 (0.0) | 8.0 |
| even | 1 | mul | 0.574 | 2.0% | 14.0 | 513 | 8.0 (0.0) | 8.0 |
| growing | 1 | mul | 19.6 | 12.4% | 233.6 | 0 | 18.7 (24.0) | 24.0 |
| bytes | 1 | mul | 53.4 | 3.4% | 715.1 | 0 | 6.7 (24.0) | 65.4 |
| control-2 | 1 | mul | 0.309 | 9.8% | 6.0 | 497 | 8.0 (0.0) | 8.0 |
| control | 1 | divexact | 2.61 | 3.5% | 7.0 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | divexact | 2.25 | 1.1% | 21.1 | 0 | 8.0 (0.0) | 8.0 |
| even | 1 | divexact | 2.6 | 1.0% | 14.0 | 0 | 8.0 (0.0) | 8.0 |
| growing | 1 | divexact | 24.7 | 1.6% | 398.4 | 0 | 16.0 (24.0) | 32.0 |
| bytes | 1 | divexact | 59.3 | 20.1% | 790.3 | 0 | 4.1 (24.0) | 64.0 |
| control-2 | 1 | divexact | 2.58 | 2.8% | 7.0 | 0 | 8.0 (0.0) | 8.0 |
| control | 1 | gcd | 9.07 | 7.5% | 122.1 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | gcd | 8.41 | 1.2% | 121.9 | 0 | 8.0 (0.0) | 8.0 |
| even | 1 | gcd | 9.2 | 5.0% | 129.9 | 0 | 8.0 (0.0) | 8.0 |
| growing | 1 | gcd | 43 | 14.3% | 506.4 | 0 | 18.7 (24.0) | 40.0 |
| bytes | 1 | gcd | 87.8 | 3.9% | 919.6 | 0 | 4.7 (24.0) | 72.0 |
| control-2 | 1 | gcd | 9.09 | 5.3% | 122.1 | 0 | 8.0 (0.0) | 8.0 |
| control | 1 | absdiff2 | 0.448 | 0.6% | 12.0 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | absdiff2 | 0.968 | 31.8% | 24.0 | 0 | 8.0 (0.0) | 8.0 |
| even | 1 | absdiff2 | 0.867 | 7.8% | 21.0 | 0 | 8.0 (0.0) | 8.0 |
| growing | 1 | absdiff2 | 9.49 | 5.4% | 227.0 | 0 | 16.0 (24.0) | 16.0 |
| bytes | 1 | absdiff2 | 45.3 | 32.7% | 680.7 | 0 | 5.9 (24.0) | 54.8 |
| control-2 | 1 | absdiff2 | 0.448 | 1.4% | 12.0 | 0 | 8.0 (0.0) | 8.0 |
| control | 2 | add | 0.232 | 60.6% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | add | 0.571 | 31.1% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | add | 0.587 | 2.2% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | add | 9.45 | 7.3% | 219.0 | 0 | 26.7 (29.4) | 32.0 |
| bytes | 2 | add | 62.7 | 41.1% | 935.1 | 0 | 13.9 (24.0) | 94.8 |
| control-2 | 2 | add | 0.235 | 44.9% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 2 | sub | 0.232 | 0.6% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | sub | 0.564 | 1.7% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | sub | 0.578 | 2.5% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | sub | 9.04 | 72.1% | 219.0 | 535 | 24.0 (24.0) | 24.0 |
| bytes | 2 | sub | 63.5 | 1.5% | 948.5 | 535 | 14.3 (24.0) | 88.0 |
| control-2 | 2 | sub | 0.229 | 1.6% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 2 | absdiff | 0.49 | 1.2% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | absdiff | 0.867 | 2.7% | 21.5 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | absdiff | 0.779 | 2.8% | 16.5 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | absdiff | 15.8 | 1.6% | 240.1 | 0 | 24.0 (24.0) | 24.0 |
| bytes | 2 | absdiff | 63.9 | 3.0% | 955.9 | 0 | 13.9 (24.0) | 86.8 |
| control-2 | 2 | absdiff | 0.491 | 2.0% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 2 | mul | 0.267 | 4.8% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | mul | 0.84 | 2.2% | 16.3 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | mul | 0.578 | 19.7% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | mul | 23.2 | 23.9% | 290.3 | 0 | 29.3 (29.4) | 40.0 |
| bytes | 2 | mul | 84.3 | 1.1% | 1129.5 | 0 | 17.7 (27.5) | 113.8 |
| control-2 | 2 | mul | 0.269 | 1.6% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 2 | divexact | 2.59 | 1.1% | 7.0 | 1016 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| even | 2 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| growing | 2 | divexact | 29.7 | 24.5% | 471.6 | 0 | 21.1 (24.0) | 47.2 |
| bytes | 2 | divexact | 81.4 | 0.6% | 1039.6 | 0 | 9.5 (24.0) | 87.2 |
| control-2 | 2 | divexact | 2.6 | 1.4% | 7.0 | 1016 | 8.0 (0.0) | 8.0 |
| control | 2 | gcd | 139 | 15.9% | 384.7 | 981 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | gcd | 133 | 7.6% | 362.5 | 987 | 8.0 (0.0) | 8.0 |
| even | 2 | gcd | 127 | 32.5% | 385.9 | 987 | 8.0 (0.0) | 8.0 |
| growing | 2 | gcd | 286 | 2.6% | 2764.5 | 0 | 24.0 (24.1) | 55.9 |
| bytes | 2 | gcd | 339 | 3.3% | 3396.7 | 0 | 11.5 (24.0) | 103.7 |
| control-2 | 2 | gcd | 136 | 5.1% | 384.7 | 981 | 8.0 (0.0) | 8.0 |
| control | 2 | absdiff2 | 0.448 | 15.0% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | absdiff2 | 0.99 | 5.4% | 24.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | absdiff2 | 0.857 | 1.1% | 21.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | absdiff2 | 10.2 | 1.9% | 251.0 | 0 | 24.0 (24.0) | 24.0 |
| bytes | 2 | absdiff2 | 66.1 | 0.5% | 966.9 | 0 | 13.9 (24.0) | 86.8 |
| control-2 | 2 | absdiff2 | 0.446 | 0.4% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | add | 0.23 | 29.9% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | add | 0.563 | 1.0% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | add | 0.592 | 2.7% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | add | 10.3 | 2.5% | 239.0 | 0 | 42.7 (45.4) | 48.0 |
| bytes | 4 | add | 108 | 1.4% | 1485.8 | 0 | 30.0 (40.0) | 158.9 |
| control-2 | 4 | add | 0.228 | 0.9% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | sub | 0.23 | 20.4% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | sub | 0.558 | 1.5% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | sub | 0.576 | 1.3% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | sub | 10.1 | 1.8% | 249.0 | 510 | 40.0 (40.0) | 40.0 |
| bytes | 4 | sub | 99.9 | 1.2% | 1507.8 | 510 | 30.3 (40.0) | 151.9 |
| control-2 | 4 | sub | 0.229 | 1.3% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | absdiff | 0.492 | 1.5% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | absdiff | 0.848 | 2.0% | 21.5 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | absdiff | 0.798 | 1.6% | 16.5 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | absdiff | 16.7 | 23.9% | 270.0 | 0 | 40.0 (40.0) | 40.0 |
| bytes | 4 | absdiff | 106 | 37.0% | 1516.5 | 0 | 30.0 (40.0) | 150.8 |
| control-2 | 4 | absdiff | 0.49 | 0.9% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | mul | 0.261 | 2.0% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | mul | 0.788 | 0.9% | 16.2 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | mul | 0.576 | 1.1% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | mul | 28 | 1.5% | 492.5 | 0 | 50.7 (50.7) | 72.0 |
| bytes | 4 | mul | 145 | 5.3% | 2033.5 | 0 | 38.9 (48.6) | 209.6 |
| control-2 | 4 | mul | 0.262 | 1.0% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| pow2 | 4 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| even | 4 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| growing | 4 | divexact | 41.9 | 2.0% | 635.9 | 0 | 31.7 (34.2) | 79.2 |
| bytes | 4 | divexact | 116 | 0.8% | 1558.6 | 0 | 20.2 (29.4) | 143.2 |
| control-2 | 4 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| control | 4 | gcd | 130 | 2.7% | 357.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | gcd | 125 | 10.6% | 340.8 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | gcd | 121 | 9.5% | 358.8 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | gcd | 908 | 0.7% | 11401.8 | 0 | 37.3 (40.0) | 96.0 |
| bytes | 4 | gcd | 983 | 1.3% | 12477.3 | 0 | 24.8 (34.7) | 175.9 |
| control-2 | 4 | gcd | 130 | 8.2% | 357.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | absdiff2 | 0.447 | 1.7% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | absdiff2 | 0.992 | 0.6% | 24.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | absdiff2 | 0.858 | 2.7% | 21.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | absdiff2 | 12.2 | 3.1% | 299.0 | 0 | 40.0 (40.0) | 40.0 |
| bytes | 4 | absdiff2 | 102 | 1.8% | 1545.5 | 0 | 30.0 (40.0) | 150.8 |
| control-2 | 4 | absdiff2 | 0.448 | 3.4% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | add | 0.229 | 1.5% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | add | 0.554 | 44.3% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | add | 0.589 | 1.2% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | add | 12.1 | 1.4% | 279.0 | 0 | 74.7 (77.3) | 80.0 |
| bytes | 8 | add | 196 | 0.7% | 2583.5 | 0 | 62.0 (72.0) | 286.9 |
| control-2 | 8 | add | 0.23 | 1.6% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | sub | 0.243 | 1.6% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | sub | 0.558 | 73.8% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | sub | 1.15 | 3.2% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | sub | 12.5 | 1.8% | 309.0 | 523 | 72.0 (72.0) | 72.0 |
| bytes | 8 | sub | 197 | 1.0% | 2624.9 | 523 | 62.4 (72.0) | 280.0 |
| control-2 | 8 | sub | 0.243 | 0.8% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | absdiff | 0.489 | 1.0% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | absdiff | 0.856 | 4.6% | 21.5 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | absdiff | 0.797 | 20.9% | 16.5 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | absdiff | 19.6 | 1.0% | 330.0 | 0 | 72.0 (72.0) | 72.0 |
| bytes | 8 | absdiff | 208 | 1.2% | 2634.2 | 0 | 62.0 (72.0) | 278.9 |
| control-2 | 8 | absdiff | 0.489 | 0.6% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | mul | 0.269 | 41.6% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | mul | 0.824 | 5.4% | 16.2 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | mul | 0.577 | 1.0% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | mul | 64.6 | 1.5% | 1263.5 | 0 | 93.3 (93.3) | 136.0 |
| bytes | 8 | mul | 296 | 0.8% | 4211.9 | 0 | 81.5 (91.3) | 401.5 |
| control-2 | 8 | mul | 0.267 | 9.5% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| pow2 | 8 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| even | 8 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| growing | 8 | divexact | 66.6 | 1.3% | 1110.7 | 0 | 53.1 (55.6) | 143.3 |
| bytes | 8 | divexact | 200 | 6.6% | 2736.8 | 0 | 41.5 (50.7) | 255.2 |
| control-2 | 8 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| control | 8 | gcd | 118 | 9.7% | 338.1 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | gcd | 118 | 4.7% | 324.9 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | gcd | 113 | 11.8% | 342.1 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | gcd | 2.46e+03 | 54.9% | 38739.8 | 0 | 64.0 (66.7) | 176.0 |
| bytes | 8 | gcd | 2.72e+03 | 10.0% | 40696.8 | 0 | 51.5 (61.4) | 319.9 |
| control-2 | 8 | gcd | 119 | 29.9% | 338.1 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | absdiff2 | 0.446 | 4.2% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | absdiff2 | 1 | 45.5% | 24.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | absdiff2 | 0.854 | 2.6% | 21.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | absdiff2 | 16.8 | 21.4% | 395.0 | 0 | 72.0 (72.0) | 72.0 |
| bytes | 8 | absdiff2 | 201 | 1.8% | 2699.2 | 0 | 62.0 (72.0) | 278.9 |
| control-2 | 8 | absdiff2 | 0.447 | 3.0% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 16 | add | 0.232 | 2.1% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | add | 0.551 | 1.0% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 16 | add | 0.587 | 8.6% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 16 | add | 16 | 1.6% | 359.0 | 0 | 138.7 (141.3) | 144.0 |
| bytes | 16 | add | 357 | 18.6% | 4794.8 | 0 | 126.0 (136.0) | 543.0 |
| control-2 | 16 | add | 0.236 | 4.0% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 16 | sub | 0.234 | 71.3% | 6.0 | 1023 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | sub | 0.554 | 1.2% | 14.0 | 1023 | 8.0 (0.0) | 8.0 |
| even | 16 | sub | 0.576 | 1.9% | 14.0 | 1023 | 8.0 (0.0) | 8.0 |
| growing | 16 | sub | 17.1 | 66.7% | 429.1 | 511 | 136.0 (136.0) | 136.0 |
| bytes | 16 | sub | 363 | 1.7% | 4887.0 | 511 | 126.5 (136.0) | 536.3 |
| control-2 | 16 | sub | 0.232 | 0.7% | 6.0 | 1023 | 8.0 (0.0) | 8.0 |
| control | 16 | absdiff | 0.489 | 0.7% | 12.0 | 1023 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | absdiff | 0.869 | 13.7% | 21.5 | 1023 | 8.0 (0.0) | 8.0 |
| even | 16 | absdiff | 0.788 | 2.4% | 16.5 | 1023 | 8.0 (0.0) | 8.0 |
| growing | 16 | absdiff | 24.8 | 2.3% | 450.2 | 0 | 136.0 (136.0) | 136.0 |
| bytes | 16 | absdiff | 364 | 0.6% | 4884.4 | 0 | 126.0 (136.0) | 534.8 |
| control-2 | 16 | absdiff | 0.49 | 1.6% | 12.0 | 1023 | 8.0 (0.0) | 8.0 |
| control | 16 | mul | 0.264 | 0.7% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | mul | 0.756 | 2.0% | 16.3 | 1024 | 8.0 (0.0) | 8.0 |
| even | 16 | mul | 0.578 | 0.6% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 16 | mul | 193 | 1.7% | 4238.4 | 0 | 178.7 (178.7) | 264.0 |
| bytes | 16 | mul | 650 | 1.1% | 10051.6 | 0 | 167.3 (177.2) | 786.6 |
| control-2 | 16 | mul | 0.263 | 0.9% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 16 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| pow2 | 16 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| even | 16 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| growing | 16 | divexact | 124 | 1.8% | 2609.8 | 0 | 95.7 (98.2) | 271.1 |
| bytes | 16 | divexact | 362 | 6.6% | 5650.2 | 0 | 84.1 (93.4) | 479.1 |
| control-2 | 16 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| control | 16 | gcd | 123 | 4.9% | 346.3 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | gcd | 121 | 11.7% | 330.6 | 1024 | 8.0 (0.0) | 8.0 |
| even | 16 | gcd | 114 | 6.8% | 348.2 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 16 | gcd | 7.53e+03 | 4.2% | 141854.2 | 0 | 117.4 (120.1) | 336.3 |
| bytes | 16 | gcd | 8.32e+03 | 3.7% | 145592.4 | 0 | 105.0 (114.8) | 608.2 |
| control-2 | 16 | gcd | 127 | 6.1% | 346.3 | 1024 | 8.0 (0.0) | 8.0 |
| control | 16 | absdiff2 | 0.448 | 64.8% | 12.0 | 1023 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | absdiff2 | 0.985 | 1.3% | 24.0 | 1023 | 8.0 (0.0) | 8.0 |
| even | 16 | absdiff2 | 0.858 | 1.4% | 21.0 | 1023 | 8.0 (0.0) | 8.0 |
| growing | 16 | absdiff2 | 26.5 | 1.0% | 587.1 | 0 | 136.0 (136.0) | 136.0 |
| bytes | 16 | absdiff2 | 366 | 0.9% | 5021.3 | 0 | 126.0 (136.0) | 534.8 |
| control-2 | 16 | absdiff2 | 0.446 | 0.8% | 12.0 | 1023 | 8.0 (0.0) | 8.0 |

| pairing | job | op | ref | row | T(ref)/T(row) | band | time | bytes ratio | peak ratio |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| build x Control | 1 | add | control | pow2 | 0.429 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | add | control | even | 0.398 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | add | control | growing | 0.026 | 0.05 | slower | 2.333 | 3.000 |
| build x Control | 1 | add | control | bytes | 0.005 | 0.05 | slower | 0.744 | 7.852 |
| build x Control | 1 | sub | control | pow2 | 0.436 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | sub | control | even | 0.402 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | sub | control | growing | 0.028 | 0.05 | slower | 2.000 | 2.000 |
| build x Control | 1 | sub | control | bytes | 0.006 | 0.05 | slower | 0.791 | 6.993 |
| build x Control | 1 | absdiff | control | pow2 | 0.588 | 0.056 | slower | 1.000 | 1.000 |
| build x Control | 1 | absdiff | control | even | 0.667 | 0.056 | slower | 1.000 | 1.000 |
| build x Control | 1 | absdiff | control | growing | 0.035 | 0.056 | slower | 2.000 | 2.000 |
| build x Control | 1 | absdiff | control | bytes | 0.011 | 0.056 | slower | 0.743 | 6.849 |
| build x Control | 1 | mul | control | pow2 | 0.395 | 0.123 | slower | 1.000 | 1.000 |
| build x Control | 1 | mul | control | even | 0.479 | 0.123 | slower | 1.000 | 1.000 |
| build x Control | 1 | mul | control | growing | 0.014 | 0.123 | slower | 2.333 | 3.000 |
| build x Control | 1 | mul | control | bytes | 0.005 | 0.123 | slower | 0.844 | 8.172 |
| build x Control | 1 | divexact | control | pow2 | 1.16 | 0.05 | faster | 1.000 | 1.000 |
| build x Control | 1 | divexact | control | even | 1.005 | 0.05 | within noise | 1.000 | 1.000 |
| build x Control | 1 | divexact | control | growing | 0.105 | 0.05 | slower | 2.000 | 4.000 |
| build x Control | 1 | divexact | control | bytes | 0.044 | 0.05 | slower | 0.511 | 8.000 |
| build x Control | 1 | gcd | control | pow2 | 1.078 | 0.05 | faster | 1.000 | 1.000 |
| build x Control | 1 | gcd | control | even | 0.986 | 0.05 | within noise | 1.000 | 1.000 |
| build x Control | 1 | gcd | control | growing | 0.211 | 0.05 | slower | 2.333 | 5.000 |
| build x Control | 1 | gcd | control | bytes | 0.103 | 0.05 | slower | 0.592 | 9.000 |
| build x Control | 1 | absdiff2 | control | pow2 | 0.463 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | absdiff2 | control | even | 0.517 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | absdiff2 | control | growing | 0.047 | 0.05 | slower | 2.000 | 2.000 |
| build x Control | 1 | absdiff2 | control | bytes | 0.01 | 0.05 | slower | 0.743 | 6.849 |
| Byte encoding x Growing | 2 | add | growing | bytes | 0.151 | 0.05 | slower | 0.523 | 2.962 |
| Byte encoding x Growing | 2 | sub | growing | bytes | 0.142 | 0.05 | slower | 0.598 | 3.667 |
| Byte encoding x Growing | 2 | absdiff | growing | bytes | 0.247 | 0.05 | slower | 0.581 | 3.615 |
| Byte encoding x Growing | 2 | mul | growing | bytes | 0.275 | 0.05 | slower | 0.602 | 2.844 |
| Byte encoding x Growing | 2 | divexact | growing | bytes | 0.365 | 0.05 | slower | 0.449 | 1.846 |
| Byte encoding x Growing | 2 | gcd | growing | bytes | 0.844 | 0.05 | slower | 0.478 | 1.856 |
| Byte encoding x Growing | 2 | absdiff2 | growing | bytes | 0.155 | 0.05 | slower | 0.581 | 3.615 |
| Byte encoding x Growing | 4 | add | growing | bytes | 0.096 | 0.05 | slower | 0.702 | 3.310 |
| Byte encoding x Growing | 4 | sub | growing | bytes | 0.101 | 0.05 | slower | 0.758 | 3.799 |
| Byte encoding x Growing | 4 | absdiff | growing | bytes | 0.159 | 0.05 | slower | 0.749 | 3.771 |
| Byte encoding x Growing | 4 | mul | growing | bytes | 0.194 | 0.05 | slower | 0.767 | 2.911 |
| Byte encoding x Growing | 4 | divexact | growing | bytes | 0.363 | 0.05 | slower | 0.635 | 1.807 |
| Byte encoding x Growing | 4 | gcd | growing | bytes | 0.924 | 0.05 | slower | 0.664 | 1.832 |
| Byte encoding x Growing | 4 | absdiff2 | growing | bytes | 0.119 | 0.05 | slower | 0.749 | 3.771 |
| Byte encoding x Growing | 8 | add | growing | bytes | 0.061 | 0.05 | slower | 0.831 | 3.587 |
| Byte encoding x Growing | 8 | sub | growing | bytes | 0.064 | 0.05 | slower | 0.866 | 3.888 |
| Byte encoding x Growing | 8 | absdiff | growing | bytes | 0.094 | 0.05 | slower | 0.861 | 3.874 |
| Byte encoding x Growing | 8 | mul | growing | bytes | 0.218 | 0.05 | slower | 0.873 | 2.952 |
| Byte encoding x Growing | 8 | divexact | growing | bytes | 0.332 | 0.05 | slower | 0.781 | 1.781 |
| Byte encoding x Growing | 8 | gcd | growing | bytes | 0.905 | 0.05 | slower | 0.804 | 1.817 |
| Byte encoding x Growing | 8 | absdiff2 | growing | bytes | 0.084 | 0.05 | slower | 0.861 | 3.874 |
| Byte encoding x Growing | 16 | add | growing | bytes | 0.045 | 0.05 | slower | 0.909 | 3.771 |
| Byte encoding x Growing | 16 | sub | growing | bytes | 0.047 | 0.05 | slower | 0.930 | 3.943 |
| Byte encoding x Growing | 16 | absdiff | growing | bytes | 0.068 | 0.05 | slower | 0.926 | 3.933 |
| Byte encoding x Growing | 16 | mul | growing | bytes | 0.297 | 0.05 | slower | 0.936 | 2.980 |
| Byte encoding x Growing | 16 | divexact | growing | bytes | 0.341 | 0.05 | slower | 0.879 | 1.767 |
| Byte encoding x Growing | 16 | gcd | growing | bytes | 0.904 | 0.05 | slower | 0.894 | 1.808 |
| Byte encoding x Growing | 16 | absdiff2 | growing | bytes | 0.072 | 0.05 | slower | 0.926 | 3.933 |
| absdiff x sub | 1 | - | sub | control absdiff | 0.459 | 0.056 | slower | 1.000 | 1.000 |
| absdiff x sub | 1 | - | sub | control absdiff2 | 0.517 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 2 | - | sub | control absdiff | 0.474 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 2 | - | sub | control absdiff2 | 0.518 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 4 | - | sub | control absdiff | 0.467 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 4 | - | sub | control absdiff2 | 0.514 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 8 | - | sub | control absdiff | 0.497 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 8 | - | sub | control absdiff2 | 0.545 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 16 | - | sub | control absdiff | 0.478 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 16 | - | sub | control absdiff2 | 0.522 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 1 | - | sub | pow2 absdiff | 0.62 | 0.056 | slower | 1.000 | 1.000 |
| absdiff x sub | 1 | - | sub | pow2 absdiff2 | 0.549 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 2 | - | sub | pow2 absdiff | 0.65 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 2 | - | sub | pow2 absdiff2 | 0.569 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 4 | - | sub | pow2 absdiff | 0.657 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 4 | - | sub | pow2 absdiff2 | 0.562 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 8 | - | sub | pow2 absdiff | 0.651 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 8 | - | sub | pow2 absdiff2 | 0.558 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 16 | - | sub | pow2 absdiff | 0.637 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 16 | - | sub | pow2 absdiff2 | 0.562 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 1 | - | sub | even absdiff | 0.761 | 0.056 | slower | 1.000 | 1.000 |
| absdiff x sub | 1 | - | sub | even absdiff2 | 0.664 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 2 | - | sub | even absdiff | 0.743 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 2 | - | sub | even absdiff2 | 0.675 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 4 | - | sub | even absdiff | 0.723 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 4 | - | sub | even absdiff2 | 0.672 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 8 | - | sub | even absdiff | 1.441 | 0.05 | faster | 1.000 | 1.000 |
| absdiff x sub | 8 | - | sub | even absdiff2 | 1.345 | 0.05 | faster | 1.000 | 1.000 |
| absdiff x sub | 16 | - | sub | even absdiff | 0.731 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 16 | - | sub | even absdiff2 | 0.672 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 1 | - | sub | growing absdiff | 0.58 | 0.056 | slower | 1.000 | 1.000 |
| absdiff x sub | 1 | - | sub | growing absdiff2 | 0.885 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 2 | - | sub | growing absdiff | 0.573 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 2 | - | sub | growing absdiff2 | 0.882 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 4 | - | sub | growing absdiff | 0.605 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 4 | - | sub | growing absdiff2 | 0.831 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 8 | - | sub | growing absdiff | 0.639 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 8 | - | sub | growing absdiff2 | 0.744 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 16 | - | sub | growing absdiff | 0.689 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 16 | - | sub | growing absdiff2 | 0.645 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x sub | 1 | - | sub | bytes absdiff | 0.813 | 0.056 | slower | 0.939 | 0.979 |
| absdiff x sub | 1 | - | sub | bytes absdiff2 | 0.8 | 0.05 | slower | 0.939 | 0.979 |
| absdiff x sub | 2 | - | sub | bytes absdiff | 0.994 | 0.05 | within noise | 0.971 | 0.986 |
| absdiff x sub | 2 | - | sub | bytes absdiff2 | 0.961 | 0.05 | within noise | 0.971 | 0.986 |
| absdiff x sub | 4 | - | sub | bytes absdiff | 0.946 | 0.05 | slower | 0.988 | 0.993 |
| absdiff x sub | 4 | - | sub | bytes absdiff2 | 0.977 | 0.05 | within noise | 0.988 | 0.993 |
| absdiff x sub | 8 | - | sub | bytes absdiff | 0.946 | 0.05 | slower | 0.994 | 0.996 |
| absdiff x sub | 8 | - | sub | bytes absdiff2 | 0.981 | 0.05 | within noise | 0.994 | 0.996 |
| absdiff x sub | 16 | - | sub | bytes absdiff | 0.997 | 0.05 | within noise | 0.996 | 0.997 |
| absdiff x sub | 16 | - | sub | bytes absdiff2 | 0.99 | 0.05 | within noise | 0.996 | 0.997 |
