# 12-integers timing

N = 1024, 15 reps per cell. Word builds: 8 bytes per value and 8 peak, no allocation (trivial).

| build | job | op | ns/op | spread | Ir/op | inexact | bytes/value acct (heap) | peak acct |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| control | 1 | add | 0.231 | 1.5% | 6.0 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | add | 0.536 | 1.0% | 14.0 | 2 | 8.0 (0.0) | 8.0 |
| even | 1 | add | 0.577 | 1.3% | 14.0 | 2 | 8.0 (0.0) | 8.0 |
| growing | 1 | add | 8.87 | 2.0% | 209.0 | 0 | 18.7 (24.0) | 24.0 |
| bytes | 1 | add | 43.5 | 3.1% | 663.1 | 0 | 6.0 (24.0) | 62.8 |
| hybrid | 1 | add | 1.35 | 1.0% | 33.4 | 0 | 8.0 (0.0) | 0.0 |
| control-2 | 1 | add | 0.232 | 1.9% | 6.0 | 0 | 8.0 (0.0) | 8.0 |
| control | 1 | sub | 0.231 | 2.5% | 6.0 | 546 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | sub | 0.535 | 1.1% | 14.0 | 546 | 8.0 (0.0) | 8.0 |
| even | 1 | sub | 0.575 | 2.7% | 14.0 | 546 | 8.0 (0.0) | 8.0 |
| growing | 1 | sub | 8.53 | 1.5% | 204.0 | 546 | 16.0 (24.0) | 16.0 |
| bytes | 1 | sub | 44.6 | 1.8% | 671.7 | 546 | 6.3 (24.0) | 55.9 |
| hybrid | 1 | sub | 7.7 | 4.0% | 133.2 | 546 | 9.4 (4.3) | 8.5 |
| control-2 | 1 | sub | 0.232 | 1.3% | 6.0 | 546 | 8.0 (0.0) | 8.0 |
| control | 1 | absdiff | 0.56 | 8.3% | 12.0 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | absdiff | 0.859 | 4.1% | 21.5 | 0 | 8.0 (0.0) | 8.0 |
| even | 1 | absdiff | 0.76 | 1.2% | 16.5 | 0 | 8.0 (0.0) | 8.0 |
| growing | 1 | absdiff | 14.3 | 1.4% | 225.1 | 0 | 16.0 (24.0) | 16.0 |
| bytes | 1 | absdiff | 44.5 | 3.0% | 678.8 | 0 | 5.9 (24.0) | 54.8 |
| hybrid | 1 | absdiff | 1.44 | 71.8% | 37.0 | 0 | 8.0 (0.0) | 0.0 |
| control-2 | 1 | absdiff | 0.549 | 7.9% | 12.0 | 0 | 8.0 (0.0) | 8.0 |
| control | 1 | mul | 0.342 | 26.9% | 6.0 | 497 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | mul | 0.693 | 1.1% | 17.0 | 513 | 8.0 (0.0) | 8.0 |
| even | 1 | mul | 0.572 | 1.1% | 14.0 | 513 | 8.0 (0.0) | 8.0 |
| growing | 1 | mul | 19.4 | 1.2% | 233.6 | 0 | 18.7 (24.0) | 24.0 |
| bytes | 1 | mul | 53.7 | 35.1% | 715.1 | 0 | 6.7 (24.0) | 65.4 |
| hybrid | 1 | mul | 6.67 | 4.5% | 132.8 | 0 | 10.7 (4.0) | 12.0 |
| control-2 | 1 | mul | 0.272 | 30.6% | 6.0 | 497 | 8.0 (0.0) | 8.0 |
| control | 1 | divexact | 2.6 | 0.9% | 7.0 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | divexact | 2.24 | 2.0% | 21.1 | 0 | 8.0 (0.0) | 8.0 |
| even | 1 | divexact | 2.59 | 1.0% | 14.0 | 0 | 8.0 (0.0) | 8.0 |
| growing | 1 | divexact | 25 | 1.3% | 398.4 | 0 | 16.0 (24.0) | 32.0 |
| bytes | 1 | divexact | 59.5 | 1.4% | 790.3 | 0 | 4.1 (24.0) | 64.0 |
| hybrid | 1 | divexact | 2.6 | 20.1% | 34.0 | 0 | 8.0 (0.0) | 0.0 |
| control-2 | 1 | divexact | 2.61 | 0.8% | 7.0 | 0 | 8.0 (0.0) | 8.0 |
| control | 1 | gcd | 9.34 | 21.8% | 122.1 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | gcd | 8.38 | 23.4% | 121.9 | 0 | 8.0 (0.0) | 8.0 |
| even | 1 | gcd | 9.17 | 31.4% | 129.9 | 0 | 8.0 (0.0) | 8.0 |
| growing | 1 | gcd | 42.9 | 11.3% | 506.4 | 0 | 18.7 (24.0) | 40.0 |
| bytes | 1 | gcd | 87.2 | 31.5% | 919.6 | 0 | 4.7 (24.0) | 72.0 |
| hybrid | 1 | gcd | 9.88 | 43.9% | 148.1 | 0 | 8.0 (0.0) | 0.0 |
| control-2 | 1 | gcd | 9.09 | 11.6% | 122.1 | 0 | 8.0 (0.0) | 8.0 |
| control | 1 | absdiff2 | 0.449 | 0.8% | 12.0 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | absdiff2 | 0.971 | 1.5% | 24.0 | 0 | 8.0 (0.0) | 8.0 |
| even | 1 | absdiff2 | 0.861 | 1.6% | 21.0 | 0 | 8.0 (0.0) | 8.0 |
| growing | 1 | absdiff2 | 9.4 | 2.4% | 227.0 | 0 | 16.0 (24.0) | 16.0 |
| bytes | 1 | absdiff2 | 46.1 | 4.7% | 680.7 | 0 | 5.9 (24.0) | 54.8 |
| hybrid | 1 | absdiff2 | 1.43 | 4.9% | 38.0 | 0 | 8.0 (0.0) | 0.0 |
| control-2 | 1 | absdiff2 | 0.45 | 116.7% | 12.0 | 0 | 8.0 (0.0) | 8.0 |
| control | 2 | add | 0.23 | 2.4% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | add | 0.575 | 68.8% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | add | 0.587 | 1.1% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | add | 9.23 | 1.2% | 219.0 | 0 | 26.7 (29.4) | 32.0 |
| bytes | 2 | add | 63 | 1.6% | 935.1 | 0 | 13.9 (24.0) | 94.8 |
| hybrid | 2 | add | 10.4 | 3.0% | 245.0 | 0 | 26.7 (29.4) | 32.0 |
| control-2 | 2 | add | 0.23 | 1.1% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 2 | sub | 0.231 | 1.2% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | sub | 0.568 | 2.4% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | sub | 0.579 | 3.6% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | sub | 9.04 | 4.9% | 219.0 | 535 | 24.0 (24.0) | 24.0 |
| bytes | 2 | sub | 64.5 | 2.1% | 948.5 | 535 | 14.3 (24.0) | 88.0 |
| hybrid | 2 | sub | 10 | 2.4% | 245.0 | 535 | 24.0 (24.0) | 24.0 |
| control-2 | 2 | sub | 0.229 | 2.5% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 2 | absdiff | 0.504 | 1.4% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | absdiff | 0.868 | 2.2% | 21.5 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | absdiff | 0.78 | 1.6% | 16.5 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | absdiff | 15.6 | 1.0% | 240.1 | 0 | 24.0 (24.0) | 24.0 |
| bytes | 2 | absdiff | 64.4 | 2.7% | 955.9 | 0 | 13.9 (24.0) | 86.8 |
| hybrid | 2 | absdiff | 16.9 | 2.3% | 269.1 | 0 | 24.0 (24.0) | 24.0 |
| control-2 | 2 | absdiff | 0.491 | 3.4% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 2 | mul | 0.265 | 2.6% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | mul | 0.838 | 2.5% | 16.3 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | mul | 0.58 | 1.4% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | mul | 23.1 | 1.9% | 290.3 | 0 | 29.3 (29.4) | 40.0 |
| bytes | 2 | mul | 87.2 | 1.5% | 1129.5 | 0 | 17.7 (27.5) | 113.8 |
| hybrid | 2 | mul | 24.8 | 1.3% | 320.3 | 0 | 29.3 (29.4) | 40.0 |
| control-2 | 2 | mul | 0.265 | 2.7% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 2 | divexact | 2.61 | 1.8% | 7.0 | 1016 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| even | 2 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| growing | 2 | divexact | 30 | 2.1% | 471.6 | 0 | 21.1 (24.0) | 47.2 |
| bytes | 2 | divexact | 82.3 | 1.4% | 1039.6 | 0 | 9.5 (24.0) | 87.2 |
| hybrid | 2 | divexact | 29.9 | 3.0% | 511.4 | 0 | 13.5 (8.4) | 47.2 |
| control-2 | 2 | divexact | 2.61 | 4.3% | 7.0 | 1016 | 8.0 (0.0) | 8.0 |
| control | 2 | gcd | 135 | 7.8% | 384.7 | 981 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | gcd | 132 | 8.2% | 362.5 | 987 | 8.0 (0.0) | 8.0 |
| even | 2 | gcd | 124 | 8.7% | 385.9 | 987 | 8.0 (0.0) | 8.0 |
| growing | 2 | gcd | 286 | 9.3% | 2764.5 | 0 | 24.0 (24.1) | 55.9 |
| bytes | 2 | gcd | 337 | 3.6% | 3396.7 | 0 | 11.5 (24.0) | 103.7 |
| hybrid | 2 | gcd | 286 | 4.8% | 2796.5 | 0 | 18.9 (16.5) | 55.9 |
| control-2 | 2 | gcd | 138 | 8.5% | 384.7 | 981 | 8.0 (0.0) | 8.0 |
| control | 2 | absdiff2 | 0.447 | 3.4% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | absdiff2 | 0.991 | 3.2% | 24.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | absdiff2 | 0.859 | 6.3% | 21.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | absdiff2 | 10.4 | 27.7% | 251.0 | 0 | 24.0 (24.0) | 24.0 |
| bytes | 2 | absdiff2 | 65.9 | 1.7% | 966.9 | 0 | 13.9 (24.0) | 86.8 |
| hybrid | 2 | absdiff2 | 11.6 | 1.5% | 280.0 | 0 | 24.0 (24.0) | 24.0 |
| control-2 | 2 | absdiff2 | 0.449 | 1.0% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | add | 0.231 | 1.7% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | add | 0.565 | 9.8% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | add | 0.59 | 1.3% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | add | 10.3 | 3.2% | 239.0 | 0 | 42.7 (45.4) | 48.0 |
| bytes | 4 | add | 109 | 3.1% | 1485.8 | 0 | 30.0 (40.0) | 158.9 |
| hybrid | 4 | add | 11.5 | 17.8% | 265.0 | 0 | 42.7 (45.4) | 48.0 |
| control-2 | 4 | add | 0.23 | 1.1% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | sub | 0.232 | 4.1% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | sub | 0.56 | 2.5% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | sub | 0.59 | 2.6% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | sub | 10.1 | 48.7% | 249.0 | 510 | 40.0 (40.0) | 40.0 |
| bytes | 4 | sub | 99.9 | 2.0% | 1507.8 | 510 | 30.3 (40.0) | 151.9 |
| hybrid | 4 | sub | 11.2 | 32.3% | 275.0 | 510 | 40.0 (40.0) | 40.0 |
| control-2 | 4 | sub | 0.229 | 1.6% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | absdiff | 0.491 | 6.3% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | absdiff | 0.854 | 6.6% | 21.5 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | absdiff | 0.792 | 4.2% | 16.5 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | absdiff | 16.8 | 47.9% | 270.0 | 0 | 40.0 (40.0) | 40.0 |
| bytes | 4 | absdiff | 104 | 3.9% | 1516.5 | 0 | 30.0 (40.0) | 150.8 |
| hybrid | 4 | absdiff | 18.3 | 2.6% | 299.0 | 0 | 40.0 (40.0) | 40.0 |
| control-2 | 4 | absdiff | 0.491 | 1.2% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | mul | 0.263 | 2.2% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | mul | 0.799 | 7.6% | 16.2 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | mul | 0.58 | 4.5% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | mul | 28.1 | 8.6% | 492.5 | 0 | 50.7 (50.7) | 72.0 |
| bytes | 4 | mul | 146 | 9.1% | 2033.5 | 0 | 38.9 (48.6) | 209.6 |
| hybrid | 4 | mul | 29.2 | 1.6% | 522.5 | 0 | 50.7 (50.7) | 72.0 |
| control-2 | 4 | mul | 0.262 | 2.0% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| pow2 | 4 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| even | 4 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| growing | 4 | divexact | 42.2 | 2.1% | 635.9 | 0 | 31.7 (34.2) | 79.2 |
| bytes | 4 | divexact | 116 | 3.4% | 1558.6 | 0 | 20.2 (29.4) | 143.2 |
| hybrid | 4 | divexact | 42.7 | 11.8% | 663.9 | 0 | 31.7 (34.2) | 79.2 |
| control-2 | 4 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| control | 4 | gcd | 127 | 7.4% | 357.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | gcd | 122 | 12.0% | 340.8 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | gcd | 118 | 15.3% | 358.8 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | gcd | 924 | 1.0% | 11401.8 | 0 | 37.3 (40.0) | 96.0 |
| bytes | 4 | gcd | 995 | 4.4% | 12477.3 | 0 | 24.8 (34.7) | 175.9 |
| hybrid | 4 | gcd | 927 | 3.8% | 11430.8 | 0 | 37.3 (40.0) | 96.0 |
| control-2 | 4 | gcd | 127 | 10.5% | 357.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | absdiff2 | 0.448 | 2.0% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | absdiff2 | 0.995 | 2.4% | 24.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | absdiff2 | 0.861 | 5.5% | 21.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | absdiff2 | 12.5 | 9.5% | 299.0 | 0 | 40.0 (40.0) | 40.0 |
| bytes | 4 | absdiff2 | 103 | 1.0% | 1545.5 | 0 | 30.0 (40.0) | 150.8 |
| hybrid | 4 | absdiff2 | 13.9 | 2.3% | 328.0 | 0 | 40.0 (40.0) | 40.0 |
| control-2 | 4 | absdiff2 | 0.448 | 0.8% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | add | 0.23 | 0.9% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | add | 0.555 | 0.4% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | add | 0.591 | 3.8% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | add | 12.2 | 84.3% | 279.0 | 0 | 74.7 (77.3) | 80.0 |
| bytes | 8 | add | 196 | 13.1% | 2583.5 | 0 | 62.0 (72.0) | 286.9 |
| hybrid | 8 | add | 13.4 | 0.9% | 305.0 | 0 | 74.7 (77.3) | 80.0 |
| control-2 | 8 | add | 0.23 | 14.0% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | sub | 1.29 | 2.4% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | sub | 0.557 | 1.2% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | sub | 0.577 | 0.6% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | sub | 12.4 | 1.6% | 309.0 | 523 | 72.0 (72.0) | 72.0 |
| bytes | 8 | sub | 196 | 0.8% | 2624.9 | 523 | 62.4 (72.0) | 280.0 |
| hybrid | 8 | sub | 13.5 | 1.7% | 335.0 | 523 | 72.0 (72.0) | 72.0 |
| control-2 | 8 | sub | 0.243 | 1.4% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | absdiff | 0.488 | 1.0% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | absdiff | 0.852 | 66.3% | 21.5 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | absdiff | 0.797 | 2.5% | 16.5 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | absdiff | 19.5 | 1.4% | 330.0 | 0 | 72.0 (72.0) | 72.0 |
| bytes | 8 | absdiff | 202 | 1.4% | 2634.2 | 0 | 62.0 (72.0) | 278.9 |
| hybrid | 8 | absdiff | 20.7 | 2.7% | 359.0 | 0 | 72.0 (72.0) | 72.0 |
| control-2 | 8 | absdiff | 0.49 | 27.2% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | mul | 0.268 | 6.8% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | mul | 1.35 | 2.4% | 16.2 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | mul | 0.577 | 2.7% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | mul | 64.7 | 1.7% | 1263.5 | 0 | 93.3 (93.3) | 136.0 |
| bytes | 8 | mul | 302 | 1.3% | 4211.9 | 0 | 81.5 (91.3) | 401.5 |
| hybrid | 8 | mul | 67 | 25.6% | 1293.5 | 0 | 93.3 (93.3) | 136.0 |
| control-2 | 8 | mul | 0.269 | 18.1% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| pow2 | 8 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| even | 8 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| growing | 8 | divexact | 66.6 | 3.8% | 1110.7 | 0 | 53.1 (55.6) | 143.3 |
| bytes | 8 | divexact | 199 | 1.0% | 2736.8 | 0 | 41.5 (50.7) | 255.2 |
| hybrid | 8 | divexact | 66.3 | 1.5% | 1138.7 | 0 | 53.1 (55.6) | 143.3 |
| control-2 | 8 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| control | 8 | gcd | 119 | 6.0% | 338.1 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | gcd | 115 | 13.7% | 324.9 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | gcd | 112 | 6.1% | 342.1 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | gcd | 2.46e+03 | 1.0% | 38739.8 | 0 | 64.0 (66.7) | 176.0 |
| bytes | 8 | gcd | 2.72e+03 | 2.2% | 40696.8 | 0 | 51.5 (61.4) | 319.9 |
| hybrid | 8 | gcd | 2.46e+03 | 5.0% | 38768.8 | 0 | 64.0 (66.7) | 176.0 |
| control-2 | 8 | gcd | 121 | 7.3% | 338.1 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | absdiff2 | 0.449 | 3.4% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | absdiff2 | 0.973 | 70.0% | 24.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | absdiff2 | 0.873 | 5.8% | 21.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | absdiff2 | 17.4 | 3.7% | 395.0 | 0 | 72.0 (72.0) | 72.0 |
| bytes | 8 | absdiff2 | 202 | 20.5% | 2699.2 | 0 | 62.0 (72.0) | 278.9 |
| hybrid | 8 | absdiff2 | 18.3 | 2.8% | 424.0 | 0 | 72.0 (72.0) | 72.0 |
| control-2 | 8 | absdiff2 | 0.446 | 1.0% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 16 | add | 0.232 | 73.9% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | add | 0.555 | 1.2% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 16 | add | 0.591 | 7.0% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 16 | add | 16.1 | 13.3% | 359.0 | 0 | 138.7 (141.3) | 144.0 |
| bytes | 16 | add | 358 | 3.3% | 4794.8 | 0 | 126.0 (136.0) | 543.0 |
| hybrid | 16 | add | 17.4 | 1.1% | 385.0 | 0 | 138.7 (141.3) | 144.0 |
| control-2 | 16 | add | 0.231 | 0.8% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 16 | sub | 0.229 | 1.0% | 6.0 | 1023 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | sub | 0.556 | 0.6% | 14.0 | 1023 | 8.0 (0.0) | 8.0 |
| even | 16 | sub | 0.575 | 0.9% | 14.0 | 1023 | 8.0 (0.0) | 8.0 |
| growing | 16 | sub | 17 | 10.0% | 429.1 | 511 | 136.0 (136.0) | 136.0 |
| bytes | 16 | sub | 365 | 1.4% | 4887.0 | 511 | 126.5 (136.0) | 536.3 |
| hybrid | 16 | sub | 17.8 | 2.0% | 455.1 | 511 | 136.0 (136.0) | 136.0 |
| control-2 | 16 | sub | 0.232 | 8.8% | 6.0 | 1023 | 8.0 (0.0) | 8.0 |
| control | 16 | absdiff | 0.489 | 0.5% | 12.0 | 1023 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | absdiff | 0.857 | 2.9% | 21.5 | 1023 | 8.0 (0.0) | 8.0 |
| even | 16 | absdiff | 0.793 | 1.7% | 16.5 | 1023 | 8.0 (0.0) | 8.0 |
| growing | 16 | absdiff | 24.7 | 1.1% | 450.2 | 0 | 136.0 (136.0) | 136.0 |
| bytes | 16 | absdiff | 363 | 0.8% | 4884.4 | 0 | 126.0 (136.0) | 534.8 |
| hybrid | 16 | absdiff | 25.4 | 0.9% | 479.2 | 0 | 136.0 (136.0) | 136.0 |
| control-2 | 16 | absdiff | 0.488 | 0.8% | 12.0 | 1023 | 8.0 (0.0) | 8.0 |
| control | 16 | mul | 0.265 | 56.1% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | mul | 0.837 | 42.2% | 16.3 | 1024 | 8.0 (0.0) | 8.0 |
| even | 16 | mul | 0.577 | 1.4% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 16 | mul | 193 | 0.9% | 4238.4 | 0 | 178.7 (178.7) | 264.0 |
| bytes | 16 | mul | 643 | 1.5% | 10051.6 | 0 | 167.3 (177.2) | 786.6 |
| hybrid | 16 | mul | 194 | 0.9% | 4268.4 | 0 | 178.7 (178.7) | 264.0 |
| control-2 | 16 | mul | 0.263 | 1.0% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 16 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| pow2 | 16 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| even | 16 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| growing | 16 | divexact | 124 | 4.3% | 2609.8 | 0 | 95.7 (98.2) | 271.1 |
| bytes | 16 | divexact | 371 | 2.7% | 5650.2 | 0 | 84.1 (93.4) | 479.1 |
| hybrid | 16 | divexact | 126 | 5.6% | 2637.8 | 0 | 95.7 (98.2) | 271.1 |
| control-2 | 16 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| control | 16 | gcd | 120 | 8.3% | 346.3 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | gcd | 117 | 12.1% | 330.6 | 1024 | 8.0 (0.0) | 8.0 |
| even | 16 | gcd | 113 | 9.6% | 348.2 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 16 | gcd | 7.57e+03 | 5.7% | 141854.2 | 0 | 117.4 (120.1) | 336.3 |
| bytes | 16 | gcd | 8.28e+03 | 3.5% | 145592.4 | 0 | 105.0 (114.8) | 608.2 |
| hybrid | 16 | gcd | 7.59e+03 | 1.0% | 141883.2 | 0 | 117.4 (120.1) | 336.3 |
| control-2 | 16 | gcd | 120 | 8.6% | 346.3 | 1024 | 8.0 (0.0) | 8.0 |
| control | 16 | absdiff2 | 0.447 | 3.4% | 12.0 | 1023 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | absdiff2 | 0.987 | 3.0% | 24.0 | 1023 | 8.0 (0.0) | 8.0 |
| even | 16 | absdiff2 | 0.857 | 0.9% | 21.0 | 1023 | 8.0 (0.0) | 8.0 |
| growing | 16 | absdiff2 | 26.6 | 0.8% | 587.1 | 0 | 136.0 (136.0) | 136.0 |
| bytes | 16 | absdiff2 | 367 | 3.3% | 5021.3 | 0 | 126.0 (136.0) | 534.8 |
| hybrid | 16 | absdiff2 | 27.6 | 0.7% | 616.1 | 0 | 136.0 (136.0) | 136.0 |
| control-2 | 16 | absdiff2 | 0.447 | 0.7% | 12.0 | 1023 | 8.0 (0.0) | 8.0 |

| pairing | job | op | ref | row | T(ref)/T(row) | band | time | bytes ratio | peak ratio |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| build x Control | 1 | add | control | pow2 | 0.43 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | add | control | even | 0.4 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | add | control | growing | 0.026 | 0.05 | slower | 2.333 | 3.000 |
| build x Control | 1 | add | control | bytes | 0.005 | 0.05 | slower | 0.744 | 7.852 |
| build x Control | 1 | add | control | hybrid | 0.17 | 0.05 | slower | 1.001 | 0.004 |
| build x Control | 1 | sub | control | pow2 | 0.432 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | sub | control | even | 0.401 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | sub | control | growing | 0.027 | 0.05 | slower | 2.000 | 2.000 |
| build x Control | 1 | sub | control | bytes | 0.005 | 0.05 | slower | 0.791 | 6.993 |
| build x Control | 1 | sub | control | hybrid | 0.03 | 0.05 | slower | 1.178 | 1.066 |
| build x Control | 1 | absdiff | control | pow2 | 0.652 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | absdiff | control | even | 0.737 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | absdiff | control | growing | 0.039 | 0.05 | slower | 2.000 | 2.000 |
| build x Control | 1 | absdiff | control | bytes | 0.013 | 0.05 | slower | 0.743 | 6.849 |
| build x Control | 1 | absdiff | control | hybrid | 0.39 | 0.05 | slower | 1.000 | 0.000 |
| build x Control | 1 | mul | control | pow2 | 0.494 | 0.255 | slower | 1.000 | 1.000 |
| build x Control | 1 | mul | control | even | 0.598 | 0.255 | slower | 1.000 | 1.000 |
| build x Control | 1 | mul | control | growing | 0.018 | 0.255 | slower | 2.333 | 3.000 |
| build x Control | 1 | mul | control | bytes | 0.006 | 0.255 | slower | 0.844 | 8.172 |
| build x Control | 1 | mul | control | hybrid | 0.051 | 0.255 | slower | 1.334 | 1.503 |
| build x Control | 1 | divexact | control | pow2 | 1.158 | 0.05 | faster | 1.000 | 1.000 |
| build x Control | 1 | divexact | control | even | 1.003 | 0.05 | within noise | 1.000 | 1.000 |
| build x Control | 1 | divexact | control | growing | 0.104 | 0.05 | slower | 2.000 | 4.000 |
| build x Control | 1 | divexact | control | bytes | 0.044 | 0.05 | slower | 0.511 | 8.000 |
| build x Control | 1 | divexact | control | hybrid | 0.997 | 0.05 | within noise | 1.000 | 0.000 |
| build x Control | 1 | gcd | control | pow2 | 1.114 | 0.05 | faster | 1.000 | 1.000 |
| build x Control | 1 | gcd | control | even | 1.018 | 0.05 | within noise | 1.000 | 1.000 |
| build x Control | 1 | gcd | control | growing | 0.218 | 0.05 | slower | 2.333 | 5.000 |
| build x Control | 1 | gcd | control | bytes | 0.107 | 0.05 | slower | 0.592 | 9.000 |
| build x Control | 1 | gcd | control | hybrid | 0.945 | 0.05 | slower | 1.000 | 0.000 |
| build x Control | 1 | absdiff2 | control | pow2 | 0.463 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | absdiff2 | control | even | 0.522 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | absdiff2 | control | growing | 0.048 | 0.05 | slower | 2.000 | 2.000 |
| build x Control | 1 | absdiff2 | control | bytes | 0.01 | 0.05 | slower | 0.743 | 6.849 |
| build x Control | 1 | absdiff2 | control | hybrid | 0.314 | 0.05 | slower | 1.000 | 0.000 |
| build x Control | 2 | add | control | pow2 | 0.401 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | add | control | even | 0.392 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | add | control | growing | 0.025 | 0.05 | slower | 3.333 | 4.000 |
| build x Control | 2 | add | control | bytes | 0.004 | 0.05 | slower | 1.743 | 11.849 |
| build x Control | 2 | add | control | hybrid | 0.022 | 0.05 | slower | 3.333 | 4.000 |
| build x Control | 2 | sub | control | pow2 | 0.406 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | sub | control | even | 0.399 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | sub | control | growing | 0.026 | 0.05 | slower | 3.000 | 3.000 |
| build x Control | 2 | sub | control | bytes | 0.004 | 0.05 | slower | 1.794 | 11.001 |
| build x Control | 2 | sub | control | hybrid | 0.023 | 0.05 | slower | 3.000 | 3.000 |
| build x Control | 2 | absdiff | control | pow2 | 0.581 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | absdiff | control | even | 0.647 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | absdiff | control | growing | 0.032 | 0.05 | slower | 3.000 | 3.000 |
| build x Control | 2 | absdiff | control | bytes | 0.008 | 0.05 | slower | 1.742 | 10.846 |
| build x Control | 2 | absdiff | control | hybrid | 0.03 | 0.05 | slower | 2.999 | 3.000 |
| build x Control | 2 | mul | control | pow2 | 0.316 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | mul | control | even | 0.457 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | mul | control | growing | 0.011 | 0.05 | slower | 3.667 | 5.000 |
| build x Control | 2 | mul | control | bytes | 0.003 | 0.05 | slower | 2.209 | 14.219 |
| build x Control | 2 | mul | control | hybrid | 0.011 | 0.05 | slower | 3.667 | 5.000 |
| build x Control | 2 | divexact | control | pow2 | - | - | trap | 1.000 | 1.000 |
| build x Control | 2 | divexact | control | even | - | - | trap | 1.000 | 1.000 |
| build x Control | 2 | divexact | control | growing | 0.087 | 0.05 | slower | 2.635 | 5.903 |
| build x Control | 2 | divexact | control | bytes | 0.032 | 0.05 | slower | 1.182 | 10.896 |
| build x Control | 2 | divexact | control | hybrid | 0.087 | 0.05 | slower | 1.693 | 5.903 |
| build x Control | 2 | gcd | control | pow2 | 1.019 | 0.05 | within noise | 1.000 | 1.000 |
| build x Control | 2 | gcd | control | even | 1.09 | 0.05 | faster | 1.000 | 1.000 |
| build x Control | 2 | gcd | control | growing | 0.471 | 0.05 | slower | 2.996 | 6.987 |
| build x Control | 2 | gcd | control | bytes | 0.4 | 0.05 | slower | 1.433 | 12.968 |
| build x Control | 2 | gcd | control | hybrid | 0.472 | 0.05 | slower | 2.362 | 6.987 |
| build x Control | 2 | absdiff2 | control | pow2 | 0.451 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | absdiff2 | control | even | 0.52 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | absdiff2 | control | growing | 0.043 | 0.05 | slower | 3.000 | 3.000 |
| build x Control | 2 | absdiff2 | control | bytes | 0.007 | 0.05 | slower | 1.742 | 10.846 |
| build x Control | 2 | absdiff2 | control | hybrid | 0.039 | 0.05 | slower | 2.999 | 3.000 |
| build x Control | 4 | add | control | pow2 | 0.409 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | add | control | even | 0.392 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | add | control | growing | 0.022 | 0.05 | slower | 5.333 | 6.000 |
| build x Control | 4 | add | control | bytes | 0.002 | 0.05 | slower | 3.745 | 19.858 |
| build x Control | 4 | add | control | hybrid | 0.02 | 0.05 | slower | 5.333 | 6.000 |
| build x Control | 4 | sub | control | pow2 | 0.414 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | sub | control | even | 0.393 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | sub | control | growing | 0.023 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | sub | control | bytes | 0.002 | 0.05 | slower | 3.791 | 18.994 |
| build x Control | 4 | sub | control | hybrid | 0.021 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | absdiff | control | pow2 | 0.575 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | absdiff | control | even | 0.619 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | absdiff | control | growing | 0.029 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | absdiff | control | bytes | 0.005 | 0.05 | slower | 3.744 | 18.854 |
| build x Control | 4 | absdiff | control | hybrid | 0.027 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | mul | control | pow2 | 0.329 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | mul | control | even | 0.454 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | mul | control | growing | 0.009 | 0.05 | slower | 6.333 | 9.000 |
| build x Control | 4 | mul | control | bytes | 0.002 | 0.05 | slower | 4.860 | 26.195 |
| build x Control | 4 | mul | control | hybrid | 0.009 | 0.05 | slower | 6.333 | 9.000 |
| build x Control | 4 | divexact | control | pow2 | - | - | trap | 1.000 | 1.000 |
| build x Control | 4 | divexact | control | even | - | - | trap | 1.000 | 1.000 |
| build x Control | 4 | divexact | control | growing | - | - | trap | 3.968 | 9.905 |
| build x Control | 4 | divexact | control | bytes | - | - | trap | 2.522 | 17.899 |
| build x Control | 4 | divexact | control | hybrid | - | - | trap | 3.968 | 9.905 |
| build x Control | 4 | gcd | control | pow2 | 1.043 | 0.05 | within noise | 1.000 | 1.000 |
| build x Control | 4 | gcd | control | even | 1.079 | 0.05 | faster | 1.000 | 1.000 |
| build x Control | 4 | gcd | control | growing | 0.138 | 0.05 | slower | 4.667 | 12.001 |
| build x Control | 4 | gcd | control | bytes | 0.128 | 0.05 | slower | 3.097 | 21.987 |
| build x Control | 4 | gcd | control | hybrid | 0.137 | 0.05 | slower | 4.667 | 12.001 |
| build x Control | 4 | absdiff2 | control | pow2 | 0.45 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | absdiff2 | control | even | 0.521 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | absdiff2 | control | growing | 0.036 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | absdiff2 | control | bytes | 0.004 | 0.05 | slower | 3.744 | 18.854 |
| build x Control | 4 | absdiff2 | control | hybrid | 0.032 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 8 | add | control | pow2 | 0.414 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 8 | add | control | even | 0.389 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 8 | add | control | growing | 0.019 | 0.05 | slower | 9.333 | 10.000 |
| build x Control | 8 | add | control | bytes | 0.001 | 0.05 | slower | 7.752 | 35.866 |
| build x Control | 8 | add | control | hybrid | 0.017 | 0.05 | slower | 9.333 | 10.000 |
| build x Control | 8 | sub | control | pow2 | 2.311 | 4.299 | within noise | 1.000 | 1.000 |
| build x Control | 8 | sub | control | even | 2.232 | 4.299 | within noise | 1.000 | 1.000 |
| build x Control | 8 | sub | control | growing | 0.103 | 4.299 | within noise | 9.000 | 9.000 |
| build x Control | 8 | sub | control | bytes | 0.007 | 4.299 | within noise | 7.795 | 34.996 |
| build x Control | 8 | sub | control | hybrid | 0.096 | 4.299 | within noise | 9.000 | 9.000 |
| build x Control | 8 | absdiff | control | pow2 | 0.572 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 8 | absdiff | control | even | 0.612 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 8 | absdiff | control | growing | 0.025 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 8 | absdiff | control | bytes | 0.002 | 0.05 | slower | 7.751 | 34.862 |
| build x Control | 8 | absdiff | control | hybrid | 0.024 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 8 | mul | control | pow2 | 0.199 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 8 | mul | control | even | 0.464 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 8 | mul | control | growing | 0.004 | 0.05 | slower | 11.667 | 17.000 |
| build x Control | 8 | mul | control | bytes | 0.001 | 0.05 | slower | 10.191 | 50.191 |
| build x Control | 8 | mul | control | hybrid | 0.004 | 0.05 | slower | 11.667 | 17.000 |
| build x Control | 8 | divexact | control | pow2 | - | - | trap | 1.000 | 1.000 |
| build x Control | 8 | divexact | control | even | - | - | trap | 1.000 | 1.000 |
| build x Control | 8 | divexact | control | growing | - | - | trap | 6.637 | 17.911 |
| build x Control | 8 | divexact | control | bytes | - | - | trap | 5.182 | 31.902 |
| build x Control | 8 | divexact | control | hybrid | - | - | trap | 6.637 | 17.911 |
| build x Control | 8 | gcd | control | pow2 | 1.029 | 0.05 | within noise | 1.000 | 1.000 |
| build x Control | 8 | gcd | control | even | 1.059 | 0.05 | faster | 1.000 | 1.000 |
| build x Control | 8 | gcd | control | growing | 0.048 | 0.05 | slower | 8.002 | 22.005 |
| build x Control | 8 | gcd | control | bytes | 0.044 | 0.05 | slower | 6.436 | 39.993 |
| build x Control | 8 | gcd | control | hybrid | 0.048 | 0.05 | slower | 8.002 | 22.005 |
| build x Control | 8 | absdiff2 | control | pow2 | 0.462 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 8 | absdiff2 | control | even | 0.514 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 8 | absdiff2 | control | growing | 0.026 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 8 | absdiff2 | control | bytes | 0.002 | 0.05 | slower | 7.751 | 34.862 |
| build x Control | 8 | absdiff2 | control | hybrid | 0.025 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 16 | add | control | pow2 | 0.417 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | add | control | even | 0.392 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | add | control | growing | 0.014 | 0.05 | slower | 17.333 | 18.000 |
| build x Control | 16 | add | control | bytes | 0.001 | 0.05 | slower | 15.756 | 67.873 |
| build x Control | 16 | add | control | hybrid | 0.013 | 0.05 | slower | 17.333 | 18.000 |
| build x Control | 16 | sub | control | pow2 | 0.412 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | sub | control | even | 0.398 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | sub | control | growing | 0.013 | 0.05 | slower | 17.000 | 17.000 |
| build x Control | 16 | sub | control | bytes | 0.001 | 0.05 | slower | 15.811 | 67.037 |
| build x Control | 16 | sub | control | hybrid | 0.013 | 0.05 | slower | 16.995 | 17.000 |
| build x Control | 16 | absdiff | control | pow2 | 0.57 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | absdiff | control | even | 0.616 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | absdiff | control | growing | 0.02 | 0.05 | slower | 17.000 | 17.000 |
| build x Control | 16 | absdiff | control | bytes | 0.001 | 0.05 | slower | 15.750 | 66.855 |
| build x Control | 16 | absdiff | control | hybrid | 0.019 | 0.05 | slower | 16.995 | 17.000 |
| build x Control | 16 | mul | control | pow2 | 0.316 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | mul | control | even | 0.458 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | mul | control | growing | 0.001 | 0.05 | slower | 22.333 | 33.000 |
| build x Control | 16 | mul | control | bytes | 0.0 | 0.05 | slower | 20.915 | 98.325 |
| build x Control | 16 | mul | control | hybrid | 0.001 | 0.05 | slower | 22.333 | 33.000 |
| build x Control | 16 | divexact | control | pow2 | - | - | trap | 1.000 | 1.000 |
| build x Control | 16 | divexact | control | even | - | - | trap | 1.000 | 1.000 |
| build x Control | 16 | divexact | control | growing | - | - | trap | 11.964 | 33.892 |
| build x Control | 16 | divexact | control | bytes | - | - | trap | 10.513 | 59.882 |
| build x Control | 16 | divexact | control | hybrid | - | - | trap | 11.964 | 33.892 |
| build x Control | 16 | gcd | control | pow2 | 1.029 | 0.05 | within noise | 1.000 | 1.000 |
| build x Control | 16 | gcd | control | even | 1.061 | 0.05 | faster | 1.000 | 1.000 |
| build x Control | 16 | gcd | control | growing | 0.016 | 0.05 | slower | 14.681 | 42.043 |
| build x Control | 16 | gcd | control | bytes | 0.014 | 0.05 | slower | 13.130 | 76.027 |
| build x Control | 16 | gcd | control | hybrid | 0.016 | 0.05 | slower | 14.681 | 42.043 |
| build x Control | 16 | absdiff2 | control | pow2 | 0.453 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | absdiff2 | control | even | 0.521 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | absdiff2 | control | growing | 0.017 | 0.05 | slower | 17.000 | 17.000 |
| build x Control | 16 | absdiff2 | control | bytes | 0.001 | 0.05 | slower | 15.750 | 66.855 |
| build x Control | 16 | absdiff2 | control | hybrid | 0.016 | 0.05 | slower | 16.995 | 17.000 |
| absdiff x Control sub | 1 | - | control sub | control absdiff | 0.412 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 1 | - | control sub | control absdiff2 | 0.514 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 2 | - | control sub | control absdiff | 0.458 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 2 | - | control sub | control absdiff2 | 0.516 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 4 | - | control sub | control absdiff | 0.473 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 4 | - | control sub | control absdiff2 | 0.518 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 8 | - | control sub | control absdiff | 2.64 | 4.299 | within noise | 1.000 | 1.000 |
| absdiff x Control sub | 8 | - | control sub | control absdiff2 | 2.866 | 4.299 | within noise | 1.000 | 1.000 |
| absdiff x Control sub | 16 | - | control sub | control absdiff | 0.469 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 16 | - | control sub | control absdiff2 | 0.512 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 1 | - | control sub | pow2 absdiff | 0.269 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 1 | - | control sub | pow2 absdiff2 | 0.238 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 2 | - | control sub | pow2 absdiff | 0.266 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 2 | - | control sub | pow2 absdiff2 | 0.233 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 4 | - | control sub | pow2 absdiff | 0.272 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 4 | - | control sub | pow2 absdiff2 | 0.233 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 8 | - | control sub | pow2 absdiff | 1.51 | 4.299 | within noise | 1.000 | 1.000 |
| absdiff x Control sub | 8 | - | control sub | pow2 absdiff2 | 1.323 | 4.299 | within noise | 1.000 | 1.000 |
| absdiff x Control sub | 16 | - | control sub | pow2 absdiff | 0.267 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 16 | - | control sub | pow2 absdiff2 | 0.232 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 1 | - | control sub | even absdiff | 0.304 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 1 | - | control sub | even absdiff2 | 0.268 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 2 | - | control sub | even absdiff | 0.296 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 2 | - | control sub | even absdiff2 | 0.269 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 4 | - | control sub | even absdiff | 0.293 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 4 | - | control sub | even absdiff2 | 0.27 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 8 | - | control sub | even absdiff | 1.615 | 4.299 | within noise | 1.000 | 1.000 |
| absdiff x Control sub | 8 | - | control sub | even absdiff2 | 1.475 | 4.299 | within noise | 1.000 | 1.000 |
| absdiff x Control sub | 16 | - | control sub | even absdiff | 0.289 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 16 | - | control sub | even absdiff2 | 0.267 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 1 | - | control sub | growing absdiff | 0.016 | 0.05 | slower | 2.000 | 2.000 |
| absdiff x Control sub | 1 | - | control sub | growing absdiff2 | 0.025 | 0.05 | slower | 2.000 | 2.000 |
| absdiff x Control sub | 2 | - | control sub | growing absdiff | 0.015 | 0.05 | slower | 3.000 | 3.000 |
| absdiff x Control sub | 2 | - | control sub | growing absdiff2 | 0.022 | 0.05 | slower | 3.000 | 3.000 |
| absdiff x Control sub | 4 | - | control sub | growing absdiff | 0.014 | 0.05 | slower | 5.000 | 5.000 |
| absdiff x Control sub | 4 | - | control sub | growing absdiff2 | 0.019 | 0.05 | slower | 5.000 | 5.000 |
| absdiff x Control sub | 8 | - | control sub | growing absdiff | 0.066 | 4.299 | within noise | 9.000 | 9.000 |
| absdiff x Control sub | 8 | - | control sub | growing absdiff2 | 0.074 | 4.299 | within noise | 9.000 | 9.000 |
| absdiff x Control sub | 16 | - | control sub | growing absdiff | 0.009 | 0.05 | slower | 17.000 | 17.000 |
| absdiff x Control sub | 16 | - | control sub | growing absdiff2 | 0.009 | 0.05 | slower | 17.000 | 17.000 |
| absdiff x Control sub | 1 | - | control sub | bytes absdiff | 0.005 | 0.05 | slower | 0.743 | 6.849 |
| absdiff x Control sub | 1 | - | control sub | bytes absdiff2 | 0.005 | 0.05 | slower | 0.743 | 6.849 |
| absdiff x Control sub | 2 | - | control sub | bytes absdiff | 0.004 | 0.05 | slower | 1.742 | 10.846 |
| absdiff x Control sub | 2 | - | control sub | bytes absdiff2 | 0.004 | 0.05 | slower | 1.742 | 10.846 |
| absdiff x Control sub | 4 | - | control sub | bytes absdiff | 0.002 | 0.05 | slower | 3.744 | 18.854 |
| absdiff x Control sub | 4 | - | control sub | bytes absdiff2 | 0.002 | 0.05 | slower | 3.744 | 18.854 |
| absdiff x Control sub | 8 | - | control sub | bytes absdiff | 0.006 | 4.299 | within noise | 7.751 | 34.862 |
| absdiff x Control sub | 8 | - | control sub | bytes absdiff2 | 0.006 | 4.299 | within noise | 7.751 | 34.862 |
| absdiff x Control sub | 16 | - | control sub | bytes absdiff | 0.001 | 0.05 | slower | 15.750 | 66.855 |
| absdiff x Control sub | 16 | - | control sub | bytes absdiff2 | 0.001 | 0.05 | slower | 15.750 | 66.855 |
| absdiff x Control sub | 1 | - | control sub | hybrid absdiff | 0.161 | 0.05 | slower | 1.000 | 0.000 |
| absdiff x Control sub | 1 | - | control sub | hybrid absdiff2 | 0.161 | 0.05 | slower | 1.000 | 0.000 |
| absdiff x Control sub | 2 | - | control sub | hybrid absdiff | 0.014 | 0.05 | slower | 2.999 | 3.000 |
| absdiff x Control sub | 2 | - | control sub | hybrid absdiff2 | 0.02 | 0.05 | slower | 2.999 | 3.000 |
| absdiff x Control sub | 4 | - | control sub | hybrid absdiff | 0.013 | 0.05 | slower | 5.000 | 5.000 |
| absdiff x Control sub | 4 | - | control sub | hybrid absdiff2 | 0.017 | 0.05 | slower | 5.000 | 5.000 |
| absdiff x Control sub | 8 | - | control sub | hybrid absdiff | 0.062 | 4.299 | within noise | 9.000 | 9.000 |
| absdiff x Control sub | 8 | - | control sub | hybrid absdiff2 | 0.07 | 4.299 | within noise | 9.000 | 9.000 |
| absdiff x Control sub | 16 | - | control sub | hybrid absdiff | 0.009 | 0.05 | slower | 16.995 | 17.000 |
| absdiff x Control sub | 16 | - | control sub | hybrid absdiff2 | 0.008 | 0.05 | slower | 16.995 | 17.000 |
