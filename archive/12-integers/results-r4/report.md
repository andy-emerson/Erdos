# 12-integers timing

N = 1024, 15 reps per cell. Word builds: 8 bytes per value and 8 peak, no allocation (trivial).

| build | job | op | ns/op | spread | Ir/op | inexact | bytes/value acct (heap) | peak acct |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| control | 1 | add | 0.231 | 2.9% | 6.0 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | add | 0.537 | 1.8% | 14.0 | 2 | 8.0 (0.0) | 8.0 |
| even | 1 | add | 0.576 | 1.8% | 14.0 | 2 | 8.0 (0.0) | 8.0 |
| growing | 1 | add | 8.82 | 6.8% | 209.0 | 0 | 18.7 (24.0) | 24.0 |
| bytes | 1 | add | 43.4 | 1.3% | 663.1 | 0 | 6.0 (24.0) | 62.8 |
| hybrid | 1 | add | 1.36 | 8.2% | 33.4 | 0 | 8.0 (0.0) | 0.0 |
| hyb2 | 1 | add | 1.06 | 2.2% | 25.4 | 0 | 8.0 (0.0) | 0.0 |
| hyb2p2 | 1 | add | 1.13 | 79.3% | 25.4 | 0 | 8.0 (0.0) | 0.0 |
| hyb2ad | 1 | add | 1.02 | 5.1% | 25.4 | 0 | 8.0 (0.0) | 0.0 |
| control-2 | 1 | add | 0.234 | 4.4% | 6.0 | 0 | 8.0 (0.0) | 8.0 |
| control | 1 | sub | 0.234 | 36.5% | 6.0 | 546 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | sub | 0.535 | 1.4% | 14.0 | 546 | 8.0 (0.0) | 8.0 |
| even | 1 | sub | 0.575 | 8.7% | 14.0 | 546 | 8.0 (0.0) | 8.0 |
| growing | 1 | sub | 8.66 | 3.2% | 204.0 | 546 | 16.0 (24.0) | 16.0 |
| bytes | 1 | sub | 44.8 | 18.0% | 671.7 | 546 | 6.3 (24.0) | 55.9 |
| hybrid | 1 | sub | 7.67 | 3.9% | 133.2 | 546 | 9.4 (4.3) | 8.5 |
| hyb2 | 1 | sub | 7.63 | 53.7% | 126.3 | 546 | 9.4 (4.3) | 8.5 |
| hyb2p2 | 1 | sub | 7.37 | 14.5% | 126.3 | 546 | 9.4 (4.3) | 8.5 |
| hyb2ad | 1 | sub | 1.14 | 1.0% | 28.0 | 546 | 8.0 (0.0) | 0.0 |
| control-2 | 1 | sub | 0.231 | 0.4% | 6.0 | 546 | 8.0 (0.0) | 8.0 |
| control | 1 | absdiff | 0.543 | 18.4% | 12.0 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | absdiff | 0.859 | 57.8% | 21.5 | 0 | 8.0 (0.0) | 8.0 |
| even | 1 | absdiff | 0.756 | 1.2% | 16.5 | 0 | 8.0 (0.0) | 8.0 |
| growing | 1 | absdiff | 14.3 | 1.1% | 225.1 | 0 | 16.0 (24.0) | 16.0 |
| bytes | 1 | absdiff | 44.4 | 3.1% | 678.8 | 0 | 5.9 (24.0) | 54.8 |
| hybrid | 1 | absdiff | 1.44 | 1.0% | 37.0 | 0 | 8.0 (0.0) | 0.0 |
| hyb2 | 1 | absdiff | 1.14 | 1.6% | 28.0 | 0 | 8.0 (0.0) | 0.0 |
| hyb2p2 | 1 | absdiff | 1.14 | 1.8% | 28.0 | 0 | 8.0 (0.0) | 0.0 |
| hyb2ad | 1 | absdiff | 1.15 | 30.3% | 28.0 | 0 | 8.0 (0.0) | 0.0 |
| control-2 | 1 | absdiff | 0.532 | 77.6% | 12.0 | 0 | 8.0 (0.0) | 8.0 |
| control | 1 | mul | 0.271 | 10.3% | 6.0 | 497 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | mul | 0.72 | 4.8% | 17.0 | 513 | 8.0 (0.0) | 8.0 |
| even | 1 | mul | 0.577 | 2.6% | 14.0 | 513 | 8.0 (0.0) | 8.0 |
| growing | 1 | mul | 19 | 4.3% | 233.6 | 0 | 18.7 (24.0) | 24.0 |
| bytes | 1 | mul | 53.5 | 2.7% | 715.1 | 0 | 6.7 (24.0) | 65.4 |
| hybrid | 1 | mul | 6.53 | 7.1% | 132.8 | 0 | 10.7 (4.0) | 12.0 |
| hyb2 | 1 | mul | 6.82 | 6.4% | 127.3 | 0 | 10.7 (4.0) | 12.0 |
| hyb2p2 | 1 | mul | 6.79 | 8.8% | 127.3 | 0 | 10.7 (4.0) | 12.0 |
| hyb2ad | 1 | mul | 6.81 | 54.5% | 127.3 | 0 | 10.7 (4.0) | 12.0 |
| control-2 | 1 | mul | 0.302 | 60.8% | 6.0 | 497 | 8.0 (0.0) | 8.0 |
| control | 1 | divexact | 3.34 | 4.0% | 7.0 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | divexact | 2.25 | 3.8% | 21.1 | 0 | 8.0 (0.0) | 8.0 |
| even | 1 | divexact | 2.61 | 2.6% | 14.0 | 0 | 8.0 (0.0) | 8.0 |
| growing | 1 | divexact | 25.5 | 20.8% | 398.4 | 0 | 16.0 (24.0) | 32.0 |
| bytes | 1 | divexact | 59.4 | 1.8% | 790.3 | 0 | 4.1 (24.0) | 64.0 |
| hybrid | 1 | divexact | 2.61 | 1.8% | 34.0 | 0 | 8.0 (0.0) | 0.0 |
| hyb2 | 1 | divexact | 2.61 | 0.9% | 26.0 | 0 | 8.0 (0.0) | 0.0 |
| hyb2p2 | 1 | divexact | 2.38 | 1.3% | 34.5 | 0 | 8.0 (0.0) | 0.0 |
| hyb2ad | 1 | divexact | 2.61 | 2.3% | 26.0 | 0 | 8.0 (0.0) | 0.0 |
| control-2 | 1 | divexact | 2.6 | 0.9% | 7.0 | 0 | 8.0 (0.0) | 8.0 |
| control | 1 | gcd | 9.08 | 52.1% | 122.1 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | gcd | 8.4 | 6.3% | 121.9 | 0 | 8.0 (0.0) | 8.0 |
| even | 1 | gcd | 9.14 | 1.4% | 129.9 | 0 | 8.0 (0.0) | 8.0 |
| growing | 1 | gcd | 42.7 | 13.3% | 506.4 | 0 | 18.7 (24.0) | 40.0 |
| bytes | 1 | gcd | 87.2 | 4.2% | 919.6 | 0 | 4.7 (24.0) | 72.0 |
| hybrid | 1 | gcd | 9.9 | 51.9% | 148.1 | 0 | 8.0 (0.0) | 0.0 |
| hyb2 | 1 | gcd | 10.1 | 20.7% | 138.1 | 0 | 8.0 (0.0) | 0.0 |
| hyb2p2 | 1 | gcd | 9.84 | 8.9% | 168.9 | 0 | 8.0 (0.0) | 0.0 |
| hyb2ad | 1 | gcd | 9.54 | 53.0% | 138.1 | 0 | 8.0 (0.0) | 0.0 |
| control-2 | 1 | gcd | 9.06 | 11.8% | 122.1 | 0 | 8.0 (0.0) | 8.0 |
| control | 1 | absdiff2 | 0.456 | 1.5% | 12.0 | 0 | 8.0 (0.0) | 8.0 |
| pow2 | 1 | absdiff2 | 0.969 | 0.7% | 24.0 | 0 | 8.0 (0.0) | 8.0 |
| even | 1 | absdiff2 | 0.859 | 1.3% | 21.0 | 0 | 8.0 (0.0) | 8.0 |
| growing | 1 | absdiff2 | 9.39 | 1.7% | 227.0 | 0 | 16.0 (24.0) | 16.0 |
| bytes | 1 | absdiff2 | 45.6 | 1.1% | 680.7 | 0 | 5.9 (24.0) | 54.8 |
| hybrid | 1 | absdiff2 | 1.43 | 0.7% | 38.0 | 0 | 8.0 (0.0) | 0.0 |
| hyb2 | 1 | absdiff2 | 1.14 | 0.6% | 29.0 | 0 | 8.0 (0.0) | 0.0 |
| hyb2p2 | 1 | absdiff2 | 1.14 | 0.9% | 29.0 | 0 | 8.0 (0.0) | 0.0 |
| hyb2ad | 1 | absdiff2 | 1.14 | 2.9% | 29.0 | 0 | 8.0 (0.0) | 0.0 |
| control-2 | 1 | absdiff2 | 0.449 | 0.8% | 12.0 | 0 | 8.0 (0.0) | 8.0 |
| control | 2 | add | 0.229 | 2.5% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | add | 0.574 | 0.8% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | add | 0.591 | 27.9% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | add | 9.27 | 1.6% | 219.0 | 0 | 26.7 (29.4) | 32.0 |
| bytes | 2 | add | 63.2 | 1.0% | 935.1 | 0 | 13.9 (24.0) | 94.8 |
| hybrid | 2 | add | 10.5 | 0.9% | 245.0 | 0 | 26.7 (29.4) | 32.0 |
| hyb2 | 2 | add | 11 | 1.0% | 253.0 | 0 | 26.7 (29.4) | 32.0 |
| hyb2p2 | 2 | add | 11.7 | 2.0% | 253.0 | 0 | 26.7 (29.4) | 32.0 |
| hyb2ad | 2 | add | 10.8 | 2.8% | 253.0 | 0 | 26.7 (29.4) | 32.0 |
| control-2 | 2 | add | 0.228 | 0.9% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 2 | sub | 0.228 | 1.3% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | sub | 0.565 | 0.5% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | sub | 0.576 | 0.7% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | sub | 8.95 | 0.8% | 219.0 | 535 | 24.0 (24.0) | 24.0 |
| bytes | 2 | sub | 63.7 | 0.7% | 948.5 | 535 | 14.3 (24.0) | 88.0 |
| hybrid | 2 | sub | 9.96 | 1.2% | 245.0 | 535 | 24.0 (24.0) | 24.0 |
| hyb2 | 2 | sub | 11.9 | 1.2% | 253.0 | 535 | 24.0 (24.0) | 24.0 |
| hyb2p2 | 2 | sub | 10.3 | 3.2% | 253.0 | 535 | 24.0 (24.0) | 24.0 |
| hyb2ad | 2 | sub | 17.1 | 1.0% | 273.1 | 535 | 24.0 (24.0) | 24.0 |
| control-2 | 2 | sub | 0.228 | 1.3% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 2 | absdiff | 0.49 | 0.5% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | absdiff | 0.868 | 1.9% | 21.5 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | absdiff | 0.778 | 1.7% | 16.5 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | absdiff | 15.7 | 1.1% | 240.1 | 0 | 24.0 (24.0) | 24.0 |
| bytes | 2 | absdiff | 63.9 | 2.6% | 955.9 | 0 | 13.9 (24.0) | 86.8 |
| hybrid | 2 | absdiff | 16.9 | 0.8% | 269.1 | 0 | 24.0 (24.0) | 24.0 |
| hyb2 | 2 | absdiff | 17.4 | 1.1% | 274.1 | 0 | 24.0 (24.0) | 24.0 |
| hyb2p2 | 2 | absdiff | 16.9 | 1.3% | 274.1 | 0 | 24.0 (24.0) | 24.0 |
| hyb2ad | 2 | absdiff | 17.5 | 1.0% | 273.1 | 0 | 24.0 (24.0) | 24.0 |
| control-2 | 2 | absdiff | 0.489 | 0.6% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 2 | mul | 0.265 | 0.8% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | mul | 0.837 | 1.0% | 16.3 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | mul | 0.582 | 0.6% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | mul | 23.2 | 11.9% | 290.3 | 0 | 29.3 (29.4) | 40.0 |
| bytes | 2 | mul | 84.1 | 0.8% | 1129.5 | 0 | 17.7 (27.5) | 113.8 |
| hybrid | 2 | mul | 24.5 | 0.6% | 320.3 | 0 | 29.3 (29.4) | 40.0 |
| hyb2 | 2 | mul | 25.2 | 1.0% | 325.4 | 0 | 29.3 (29.4) | 40.0 |
| hyb2p2 | 2 | mul | 25.3 | 0.8% | 325.4 | 0 | 29.3 (29.4) | 40.0 |
| hyb2ad | 2 | mul | 25.4 | 0.6% | 325.4 | 0 | 29.3 (29.4) | 40.0 |
| control-2 | 2 | mul | 0.265 | 1.0% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 2 | divexact | 2.6 | 0.5% | 7.0 | 1016 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| even | 2 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| growing | 2 | divexact | 29.8 | 0.6% | 471.6 | 0 | 21.1 (24.0) | 47.2 |
| bytes | 2 | divexact | 82.3 | 0.7% | 1039.6 | 0 | 9.5 (24.0) | 87.2 |
| hybrid | 2 | divexact | 30.8 | 0.8% | 511.4 | 0 | 13.5 (8.4) | 47.2 |
| hyb2 | 2 | divexact | 30.7 | 1.5% | 510.6 | 0 | 13.5 (8.4) | 47.2 |
| hyb2p2 | 2 | divexact | 30.1 | 1.8% | 510.6 | 0 | 13.5 (8.4) | 47.2 |
| hyb2ad | 2 | divexact | 30.9 | 7.9% | 510.6 | 0 | 13.5 (8.4) | 47.2 |
| control-2 | 2 | divexact | 2.61 | 5.3% | 7.0 | 1016 | 8.0 (0.0) | 8.0 |
| control | 2 | gcd | 136 | 5.7% | 384.7 | 981 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | gcd | 134 | 4.9% | 362.5 | 987 | 8.0 (0.0) | 8.0 |
| even | 2 | gcd | 126 | 6.1% | 385.9 | 987 | 8.0 (0.0) | 8.0 |
| growing | 2 | gcd | 287 | 21.6% | 2764.5 | 0 | 24.0 (24.1) | 55.9 |
| bytes | 2 | gcd | 337 | 2.7% | 3396.7 | 0 | 11.5 (24.0) | 103.7 |
| hybrid | 2 | gcd | 285 | 2.3% | 2796.5 | 0 | 18.9 (16.5) | 55.9 |
| hyb2 | 2 | gcd | 286 | 21.1% | 2801.5 | 0 | 18.9 (16.5) | 55.9 |
| hyb2p2 | 2 | gcd | 283 | 2.0% | 2840.5 | 0 | 18.9 (16.5) | 55.4 |
| hyb2ad | 2 | gcd | 286 | 5.3% | 2801.5 | 0 | 18.9 (16.5) | 55.9 |
| control-2 | 2 | gcd | 136 | 6.2% | 384.7 | 981 | 8.0 (0.0) | 8.0 |
| control | 2 | absdiff2 | 0.446 | 39.4% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 2 | absdiff2 | 0.987 | 2.4% | 24.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 2 | absdiff2 | 0.857 | 0.9% | 21.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 2 | absdiff2 | 10.5 | 1.0% | 251.0 | 0 | 24.0 (24.0) | 24.0 |
| bytes | 2 | absdiff2 | 66 | 0.7% | 966.9 | 0 | 13.9 (24.0) | 86.8 |
| hybrid | 2 | absdiff2 | 11.6 | 0.9% | 280.0 | 0 | 24.0 (24.0) | 24.0 |
| hyb2 | 2 | absdiff2 | 11.6 | 1.5% | 285.0 | 0 | 24.0 (24.0) | 24.0 |
| hyb2p2 | 2 | absdiff2 | 11.4 | 1.2% | 285.0 | 0 | 24.0 (24.0) | 24.0 |
| hyb2ad | 2 | absdiff2 | 11.9 | 4.1% | 285.0 | 0 | 24.0 (24.0) | 24.0 |
| control-2 | 2 | absdiff2 | 0.444 | 2.4% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | add | 0.232 | 1.2% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | add | 0.565 | 14.8% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | add | 0.595 | 7.8% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | add | 10.5 | 5.9% | 239.0 | 0 | 42.7 (45.4) | 48.0 |
| bytes | 4 | add | 109 | 6.0% | 1485.8 | 0 | 30.0 (40.0) | 158.9 |
| hybrid | 4 | add | 11.4 | 1.0% | 265.0 | 0 | 42.7 (45.4) | 48.0 |
| hyb2 | 4 | add | 11.9 | 4.5% | 273.0 | 0 | 42.7 (45.4) | 48.0 |
| hyb2p2 | 4 | add | 12.4 | 2.8% | 273.0 | 0 | 42.7 (45.4) | 48.0 |
| hyb2ad | 4 | add | 11.7 | 1.9% | 273.0 | 0 | 42.7 (45.4) | 48.0 |
| control-2 | 4 | add | 0.232 | 0.4% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | sub | 0.232 | 3.0% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | sub | 0.558 | 1.6% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | sub | 0.577 | 4.7% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | sub | 11 | 5.5% | 249.0 | 510 | 40.0 (40.0) | 40.0 |
| bytes | 4 | sub | 99.6 | 0.5% | 1507.8 | 510 | 30.3 (40.0) | 151.9 |
| hybrid | 4 | sub | 11 | 5.2% | 275.0 | 510 | 40.0 (40.0) | 40.0 |
| hyb2 | 4 | sub | 13.2 | 1.3% | 283.0 | 510 | 40.0 (40.0) | 40.0 |
| hyb2p2 | 4 | sub | 11.9 | 3.6% | 283.0 | 510 | 40.0 (40.0) | 40.0 |
| hyb2ad | 4 | sub | 18.1 | 1.1% | 303.0 | 510 | 40.0 (40.0) | 40.0 |
| control-2 | 4 | sub | 0.232 | 1.6% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | absdiff | 0.489 | 2.7% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | absdiff | 0.852 | 4.8% | 21.5 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | absdiff | 0.786 | 2.3% | 16.5 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | absdiff | 16.7 | 1.5% | 270.0 | 0 | 40.0 (40.0) | 40.0 |
| bytes | 4 | absdiff | 104 | 5.9% | 1516.5 | 0 | 30.0 (40.0) | 150.8 |
| hybrid | 4 | absdiff | 17.9 | 2.9% | 299.0 | 0 | 40.0 (40.0) | 40.0 |
| hyb2 | 4 | absdiff | 18.6 | 1.1% | 304.0 | 0 | 40.0 (40.0) | 40.0 |
| hyb2p2 | 4 | absdiff | 18.2 | 1.0% | 304.0 | 0 | 40.0 (40.0) | 40.0 |
| hyb2ad | 4 | absdiff | 18.5 | 0.8% | 303.0 | 0 | 40.0 (40.0) | 40.0 |
| control-2 | 4 | absdiff | 0.494 | 43.3% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | mul | 0.261 | 33.8% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | mul | 0.792 | 2.5% | 16.2 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | mul | 0.576 | 2.5% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | mul | 27.8 | 0.7% | 492.5 | 0 | 50.7 (50.7) | 72.0 |
| bytes | 4 | mul | 143 | 2.0% | 2033.5 | 0 | 38.9 (48.6) | 209.6 |
| hybrid | 4 | mul | 29 | 0.9% | 522.5 | 0 | 50.7 (50.7) | 72.0 |
| hyb2 | 4 | mul | 29.6 | 31.9% | 527.5 | 0 | 50.7 (50.7) | 72.0 |
| hyb2p2 | 4 | mul | 30.3 | 1.3% | 527.5 | 0 | 50.7 (50.7) | 72.0 |
| hyb2ad | 4 | mul | 29.5 | 1.5% | 527.5 | 0 | 50.7 (50.7) | 72.0 |
| control-2 | 4 | mul | 0.262 | 4.1% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| pow2 | 4 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| even | 4 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| growing | 4 | divexact | 42 | 5.0% | 635.9 | 0 | 31.7 (34.2) | 79.2 |
| bytes | 4 | divexact | 116 | 3.9% | 1558.6 | 0 | 20.2 (29.4) | 143.2 |
| hybrid | 4 | divexact | 42.1 | 1.0% | 663.9 | 0 | 31.7 (34.2) | 79.2 |
| hyb2 | 4 | divexact | 43.3 | 1.6% | 669.9 | 0 | 31.7 (34.2) | 79.2 |
| hyb2p2 | 4 | divexact | 43.7 | 1.6% | 669.9 | 0 | 31.7 (34.2) | 79.2 |
| hyb2ad | 4 | divexact | 43.1 | 2.8% | 669.9 | 0 | 31.7 (34.2) | 79.2 |
| control-2 | 4 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| control | 4 | gcd | 126 | 11.5% | 357.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | gcd | 124 | 8.4% | 340.8 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | gcd | 120 | 8.1% | 358.8 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | gcd | 921 | 1.1% | 11401.8 | 0 | 37.3 (40.0) | 96.0 |
| bytes | 4 | gcd | 993 | 3.4% | 12477.3 | 0 | 24.8 (34.7) | 175.9 |
| hybrid | 4 | gcd | 921 | 2.3% | 11430.8 | 0 | 37.3 (40.0) | 96.0 |
| hyb2 | 4 | gcd | 921 | 1.5% | 11436.8 | 0 | 37.3 (40.0) | 96.0 |
| hyb2p2 | 4 | gcd | 915 | 1.0% | 11467.4 | 0 | 37.3 (40.0) | 95.6 |
| hyb2ad | 4 | gcd | 919 | 1.1% | 11436.8 | 0 | 37.3 (40.0) | 96.0 |
| control-2 | 4 | gcd | 127 | 7.4% | 357.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 4 | absdiff2 | 0.448 | 1.1% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 4 | absdiff2 | 1 | 2.5% | 24.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 4 | absdiff2 | 0.857 | 2.8% | 21.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 4 | absdiff2 | 12.2 | 2.6% | 299.0 | 0 | 40.0 (40.0) | 40.0 |
| bytes | 4 | absdiff2 | 102 | 2.4% | 1545.5 | 0 | 30.0 (40.0) | 150.8 |
| hybrid | 4 | absdiff2 | 14.6 | 2.9% | 328.0 | 0 | 40.0 (40.0) | 40.0 |
| hyb2 | 4 | absdiff2 | 13.9 | 2.3% | 333.0 | 0 | 40.0 (40.0) | 40.0 |
| hyb2p2 | 4 | absdiff2 | 14.1 | 1.9% | 333.0 | 0 | 40.0 (40.0) | 40.0 |
| hyb2ad | 4 | absdiff2 | 14.5 | 3.7% | 333.0 | 0 | 40.0 (40.0) | 40.0 |
| control-2 | 4 | absdiff2 | 0.447 | 5.6% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | add | 0.231 | 43.6% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | add | 0.556 | 39.4% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | add | 0.594 | 60.1% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | add | 12.1 | 1.2% | 279.0 | 0 | 74.7 (77.3) | 80.0 |
| bytes | 8 | add | 200 | 1.0% | 2583.5 | 0 | 62.0 (72.0) | 286.9 |
| hybrid | 8 | add | 13.4 | 1.5% | 305.0 | 0 | 74.7 (77.3) | 80.0 |
| hyb2 | 8 | add | 13.9 | 1.2% | 313.0 | 0 | 74.7 (77.3) | 80.0 |
| hyb2p2 | 8 | add | 14.2 | 1.8% | 313.0 | 0 | 74.7 (77.3) | 80.0 |
| hyb2ad | 8 | add | 13.8 | 0.7% | 313.0 | 0 | 74.7 (77.3) | 80.0 |
| control-2 | 8 | add | 0.229 | 1.5% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | sub | 0.243 | 0.7% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | sub | 0.559 | 1.7% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | sub | 0.577 | 0.7% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | sub | 12.4 | 1.8% | 309.0 | 523 | 72.0 (72.0) | 72.0 |
| bytes | 8 | sub | 197 | 1.0% | 2624.9 | 523 | 62.4 (72.0) | 280.0 |
| hybrid | 8 | sub | 13.5 | 1.1% | 335.0 | 523 | 72.0 (72.0) | 72.0 |
| hyb2 | 8 | sub | 15.6 | 2.3% | 343.0 | 523 | 72.0 (72.0) | 72.0 |
| hyb2p2 | 8 | sub | 14.1 | 5.4% | 343.0 | 523 | 72.0 (72.0) | 72.0 |
| hyb2ad | 8 | sub | 20.8 | 1.3% | 363.0 | 523 | 72.0 (72.0) | 72.0 |
| control-2 | 8 | sub | 0.242 | 1.3% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | absdiff | 0.489 | 0.5% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | absdiff | 0.855 | 3.1% | 21.5 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | absdiff | 0.788 | 1.3% | 16.5 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | absdiff | 19.6 | 3.7% | 330.0 | 0 | 72.0 (72.0) | 72.0 |
| bytes | 8 | absdiff | 203 | 0.9% | 2634.2 | 0 | 62.0 (72.0) | 278.9 |
| hybrid | 8 | absdiff | 20.6 | 1.8% | 359.0 | 0 | 72.0 (72.0) | 72.0 |
| hyb2 | 8 | absdiff | 21.2 | 1.2% | 364.0 | 0 | 72.0 (72.0) | 72.0 |
| hyb2p2 | 8 | absdiff | 21 | 13.6% | 364.0 | 0 | 72.0 (72.0) | 72.0 |
| hyb2ad | 8 | absdiff | 20.8 | 38.4% | 363.0 | 0 | 72.0 (72.0) | 72.0 |
| control-2 | 8 | absdiff | 0.489 | 6.6% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | mul | 0.272 | 8.7% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | mul | 0.83 | 18.6% | 16.2 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | mul | 0.578 | 0.9% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | mul | 65 | 0.9% | 1263.5 | 0 | 93.3 (93.3) | 136.0 |
| bytes | 8 | mul | 295 | 2.1% | 4211.9 | 0 | 81.5 (91.3) | 401.5 |
| hybrid | 8 | mul | 67 | 6.8% | 1293.5 | 0 | 93.3 (93.3) | 136.0 |
| hyb2 | 8 | mul | 67.9 | 1.5% | 1298.5 | 0 | 93.3 (93.3) | 136.0 |
| hyb2p2 | 8 | mul | 71.9 | 3.7% | 1298.5 | 0 | 93.3 (93.3) | 136.0 |
| hyb2ad | 8 | mul | 67.6 | 2.5% | 1298.5 | 0 | 93.3 (93.3) | 136.0 |
| control-2 | 8 | mul | 0.268 | 48.7% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| pow2 | 8 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| even | 8 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| growing | 8 | divexact | 66.5 | 1.1% | 1110.7 | 0 | 53.1 (55.6) | 143.3 |
| bytes | 8 | divexact | 199 | 1.9% | 2736.8 | 0 | 41.5 (50.7) | 255.2 |
| hybrid | 8 | divexact | 65.8 | 2.6% | 1138.7 | 0 | 53.1 (55.6) | 143.3 |
| hyb2 | 8 | divexact | 66.7 | 1.1% | 1144.7 | 0 | 53.1 (55.6) | 143.3 |
| hyb2p2 | 8 | divexact | 66.7 | 1.1% | 1144.7 | 0 | 53.1 (55.6) | 143.3 |
| hyb2ad | 8 | divexact | 66.7 | 1.6% | 1144.7 | 0 | 53.1 (55.6) | 143.3 |
| control-2 | 8 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| control | 8 | gcd | 118 | 5.5% | 338.1 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | gcd | 117 | 8.2% | 324.9 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | gcd | 110 | 11.1% | 342.1 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | gcd | 2.45e+03 | 2.7% | 38739.8 | 0 | 64.0 (66.7) | 176.0 |
| bytes | 8 | gcd | 2.71e+03 | 1.0% | 40696.8 | 0 | 51.5 (61.4) | 319.9 |
| hybrid | 8 | gcd | 2.44e+03 | 1.0% | 38768.8 | 0 | 64.0 (66.7) | 176.0 |
| hyb2 | 8 | gcd | 2.45e+03 | 1.5% | 38774.8 | 0 | 64.0 (66.7) | 176.0 |
| hyb2p2 | 8 | gcd | 2.51e+03 | 1.2% | 38748.0 | 0 | 64.0 (66.7) | 175.1 |
| hyb2ad | 8 | gcd | 2.44e+03 | 3.2% | 38774.8 | 0 | 64.0 (66.7) | 176.0 |
| control-2 | 8 | gcd | 117 | 13.0% | 338.1 | 1024 | 8.0 (0.0) | 8.0 |
| control | 8 | absdiff2 | 0.448 | 1.9% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 8 | absdiff2 | 0.982 | 24.4% | 24.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 8 | absdiff2 | 0.859 | 3.4% | 21.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 8 | absdiff2 | 16.8 | 1.7% | 395.0 | 0 | 72.0 (72.0) | 72.0 |
| bytes | 8 | absdiff2 | 203 | 1.5% | 2699.2 | 0 | 62.0 (72.0) | 278.9 |
| hybrid | 8 | absdiff2 | 18.2 | 2.7% | 424.0 | 0 | 72.0 (72.0) | 72.0 |
| hyb2 | 8 | absdiff2 | 18.1 | 2.6% | 429.0 | 0 | 72.0 (72.0) | 72.0 |
| hyb2p2 | 8 | absdiff2 | 18.6 | 2.4% | 429.0 | 0 | 72.0 (72.0) | 72.0 |
| hyb2ad | 8 | absdiff2 | 18.3 | 3.3% | 429.0 | 0 | 72.0 (72.0) | 72.0 |
| control-2 | 8 | absdiff2 | 0.448 | 2.5% | 12.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 16 | add | 0.231 | 2.3% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | add | 0.555 | 2.5% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| even | 16 | add | 0.589 | 1.1% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 16 | add | 16 | 8.2% | 359.0 | 0 | 138.7 (141.3) | 144.0 |
| bytes | 16 | add | 355 | 8.0% | 4794.8 | 0 | 126.0 (136.0) | 543.0 |
| hybrid | 16 | add | 17.4 | 4.7% | 385.0 | 0 | 138.7 (141.3) | 144.0 |
| hyb2 | 16 | add | 17.7 | 15.9% | 393.0 | 0 | 138.7 (141.3) | 144.0 |
| hyb2p2 | 16 | add | 18.6 | 17.4% | 393.0 | 0 | 138.7 (141.3) | 144.0 |
| hyb2ad | 16 | add | 17.6 | 1.9% | 393.0 | 0 | 138.7 (141.3) | 144.0 |
| control-2 | 16 | add | 0.231 | 1.3% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 16 | sub | 0.231 | 1.0% | 6.0 | 1023 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | sub | 0.557 | 10.3% | 14.0 | 1023 | 8.0 (0.0) | 8.0 |
| even | 16 | sub | 0.576 | 0.8% | 14.0 | 1023 | 8.0 (0.0) | 8.0 |
| growing | 16 | sub | 16.9 | 0.7% | 429.1 | 511 | 136.0 (136.0) | 136.0 |
| bytes | 16 | sub | 364 | 1.2% | 4887.0 | 511 | 126.5 (136.0) | 536.3 |
| hybrid | 16 | sub | 17.9 | 1.9% | 455.1 | 511 | 136.0 (136.0) | 136.0 |
| hyb2 | 16 | sub | 19.8 | 4.6% | 463.1 | 511 | 136.0 (136.0) | 136.0 |
| hyb2p2 | 16 | sub | 22.3 | 55.0% | 463.1 | 511 | 136.0 (136.0) | 136.0 |
| hyb2ad | 16 | sub | 26.8 | 5.8% | 483.2 | 511 | 136.0 (136.0) | 136.0 |
| control-2 | 16 | sub | 0.231 | 1.1% | 6.0 | 1023 | 8.0 (0.0) | 8.0 |
| control | 16 | absdiff | 0.489 | 6.8% | 12.0 | 1023 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | absdiff | 0.859 | 2.8% | 21.5 | 1023 | 8.0 (0.0) | 8.0 |
| even | 16 | absdiff | 0.794 | 1.2% | 16.5 | 1023 | 8.0 (0.0) | 8.0 |
| growing | 16 | absdiff | 24.8 | 1.6% | 450.2 | 0 | 136.0 (136.0) | 136.0 |
| bytes | 16 | absdiff | 364 | 0.7% | 4884.4 | 0 | 126.0 (136.0) | 534.8 |
| hybrid | 16 | absdiff | 25.3 | 1.1% | 479.2 | 0 | 136.0 (136.0) | 136.0 |
| hyb2 | 16 | absdiff | 25.8 | 1.0% | 484.2 | 0 | 136.0 (136.0) | 136.0 |
| hyb2p2 | 16 | absdiff | 26.4 | 10.8% | 484.2 | 0 | 136.0 (136.0) | 136.0 |
| hyb2ad | 16 | absdiff | 26.5 | 5.9% | 483.2 | 0 | 136.0 (136.0) | 136.0 |
| control-2 | 16 | absdiff | 0.489 | 4.4% | 12.0 | 1023 | 8.0 (0.0) | 8.0 |
| control | 16 | mul | 0.263 | 1.4% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | mul | 0.834 | 1.1% | 16.3 | 1024 | 8.0 (0.0) | 8.0 |
| even | 16 | mul | 0.586 | 4.8% | 14.0 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 16 | mul | 195 | 7.8% | 4238.4 | 0 | 178.7 (178.7) | 264.0 |
| bytes | 16 | mul | 667 | 25.2% | 10051.6 | 0 | 167.3 (177.2) | 786.6 |
| hybrid | 16 | mul | 196 | 5.1% | 4268.4 | 0 | 178.7 (178.7) | 264.0 |
| hyb2 | 16 | mul | 198 | 80.5% | 4273.4 | 0 | 178.7 (178.7) | 264.0 |
| hyb2p2 | 16 | mul | 211 | 5.4% | 4273.4 | 0 | 178.7 (178.7) | 264.0 |
| hyb2ad | 16 | mul | 196 | 2.3% | 4273.4 | 0 | 178.7 (178.7) | 264.0 |
| control-2 | 16 | mul | 0.264 | 0.6% | 6.0 | 1024 | 8.0 (0.0) | 8.0 |
| control | 16 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| pow2 | 16 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| even | 16 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| growing | 16 | divexact | 124 | 1.0% | 2609.8 | 0 | 95.7 (98.2) | 271.1 |
| bytes | 16 | divexact | 362 | 0.5% | 5650.2 | 0 | 84.1 (93.4) | 479.1 |
| hybrid | 16 | divexact | 125 | 2.0% | 2637.8 | 0 | 95.7 (98.2) | 271.1 |
| hyb2 | 16 | divexact | 126 | 3.6% | 2643.8 | 0 | 95.7 (98.2) | 271.1 |
| hyb2p2 | 16 | divexact | 126 | 1.3% | 2643.8 | 0 | 95.7 (98.2) | 271.1 |
| hyb2ad | 16 | divexact | 126 | 1.8% | 2643.8 | 0 | 95.7 (98.2) | 271.1 |
| control-2 | 16 | divexact | trap | - | None | trap | 8.0 (0.0) | 8.0 |
| control | 16 | gcd | 125 | 9.3% | 346.3 | 1024 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | gcd | 122 | 5.6% | 330.6 | 1024 | 8.0 (0.0) | 8.0 |
| even | 16 | gcd | 116 | 3.1% | 348.2 | 1024 | 8.0 (0.0) | 8.0 |
| growing | 16 | gcd | 7.56e+03 | 1.2% | 141854.2 | 0 | 117.4 (120.1) | 336.3 |
| bytes | 16 | gcd | 8.27e+03 | 1.6% | 145592.4 | 0 | 105.0 (114.8) | 608.2 |
| hybrid | 16 | gcd | 7.57e+03 | 0.9% | 141883.2 | 0 | 117.4 (120.1) | 336.3 |
| hyb2 | 16 | gcd | 7.58e+03 | 0.9% | 141889.2 | 0 | 117.4 (120.1) | 336.3 |
| hyb2p2 | 16 | gcd | 7.86e+03 | 1.4% | 141621.7 | 0 | 117.4 (120.1) | 334.0 |
| hyb2ad | 16 | gcd | 7.59e+03 | 16.3% | 141889.2 | 0 | 117.4 (120.1) | 336.3 |
| control-2 | 16 | gcd | 125 | 27.3% | 346.3 | 1024 | 8.0 (0.0) | 8.0 |
| control | 16 | absdiff2 | 0.447 | 0.5% | 12.0 | 1023 | 8.0 (0.0) | 8.0 |
| pow2 | 16 | absdiff2 | 0.988 | 5.2% | 24.0 | 1023 | 8.0 (0.0) | 8.0 |
| even | 16 | absdiff2 | 0.862 | 48.6% | 21.0 | 1023 | 8.0 (0.0) | 8.0 |
| growing | 16 | absdiff2 | 26.9 | 8.3% | 587.1 | 0 | 136.0 (136.0) | 136.0 |
| bytes | 16 | absdiff2 | 369 | 13.4% | 5021.3 | 0 | 126.0 (136.0) | 534.8 |
| hybrid | 16 | absdiff2 | 27.8 | 7.2% | 616.1 | 0 | 136.0 (136.0) | 136.0 |
| hyb2 | 16 | absdiff2 | 28.6 | 4.0% | 621.1 | 0 | 136.0 (136.0) | 136.0 |
| hyb2p2 | 16 | absdiff2 | 27.5 | 1.2% | 621.1 | 0 | 136.0 (136.0) | 136.0 |
| hyb2ad | 16 | absdiff2 | 27.9 | 2.6% | 621.1 | 0 | 136.0 (136.0) | 136.0 |
| control-2 | 16 | absdiff2 | 0.449 | 1.8% | 12.0 | 1023 | 8.0 (0.0) | 8.0 |

| pairing | job | op | ref | row | T(ref)/T(row) | band | time | bytes ratio | peak ratio |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| build x Control | 1 | add | control | pow2 | 0.431 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | add | control | even | 0.402 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | add | control | growing | 0.026 | 0.05 | slower | 2.333 | 3.000 |
| build x Control | 1 | add | control | bytes | 0.005 | 0.05 | slower | 0.744 | 7.852 |
| build x Control | 1 | add | control | hybrid | 0.17 | 0.05 | slower | 1.001 | 0.004 |
| build x Control | 1 | add | control | hyb2 | 0.219 | 0.05 | slower | 1.001 | 0.004 |
| build x Control | 1 | add | control | hyb2p2 | 0.205 | 0.05 | slower | 1.001 | 0.004 |
| build x Control | 1 | add | control | hyb2ad | 0.226 | 0.05 | slower | 1.001 | 0.004 |
| build x Control | 1 | sub | control | pow2 | 0.437 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | sub | control | even | 0.407 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | sub | control | growing | 0.027 | 0.05 | slower | 2.000 | 2.000 |
| build x Control | 1 | sub | control | bytes | 0.005 | 0.05 | slower | 0.791 | 6.993 |
| build x Control | 1 | sub | control | hybrid | 0.03 | 0.05 | slower | 1.178 | 1.066 |
| build x Control | 1 | sub | control | hyb2 | 0.031 | 0.05 | slower | 1.178 | 1.066 |
| build x Control | 1 | sub | control | hyb2p2 | 0.032 | 0.05 | slower | 1.178 | 1.066 |
| build x Control | 1 | sub | control | hyb2ad | 0.206 | 0.05 | slower | 1.000 | 0.000 |
| build x Control | 1 | absdiff | control | pow2 | 0.632 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | absdiff | control | even | 0.717 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | absdiff | control | growing | 0.038 | 0.05 | slower | 2.000 | 2.000 |
| build x Control | 1 | absdiff | control | bytes | 0.012 | 0.05 | slower | 0.743 | 6.849 |
| build x Control | 1 | absdiff | control | hybrid | 0.377 | 0.05 | slower | 1.000 | 0.000 |
| build x Control | 1 | absdiff | control | hyb2 | 0.475 | 0.05 | slower | 1.000 | 0.000 |
| build x Control | 1 | absdiff | control | hyb2p2 | 0.476 | 0.05 | slower | 1.000 | 0.000 |
| build x Control | 1 | absdiff | control | hyb2ad | 0.472 | 0.05 | slower | 1.000 | 0.000 |
| build x Control | 1 | mul | control | pow2 | 0.377 | 0.113 | slower | 1.000 | 1.000 |
| build x Control | 1 | mul | control | even | 0.47 | 0.113 | slower | 1.000 | 1.000 |
| build x Control | 1 | mul | control | growing | 0.014 | 0.113 | slower | 2.333 | 3.000 |
| build x Control | 1 | mul | control | bytes | 0.005 | 0.113 | slower | 0.844 | 8.172 |
| build x Control | 1 | mul | control | hybrid | 0.042 | 0.113 | slower | 1.334 | 1.503 |
| build x Control | 1 | mul | control | hyb2 | 0.04 | 0.113 | slower | 1.334 | 1.503 |
| build x Control | 1 | mul | control | hyb2p2 | 0.04 | 0.113 | slower | 1.334 | 1.503 |
| build x Control | 1 | mul | control | hyb2ad | 0.04 | 0.113 | slower | 1.334 | 1.503 |
| build x Control | 1 | divexact | control | pow2 | 1.484 | 0.288 | faster | 1.000 | 1.000 |
| build x Control | 1 | divexact | control | even | 1.282 | 0.288 | within noise | 1.000 | 1.000 |
| build x Control | 1 | divexact | control | growing | 0.131 | 0.288 | slower | 2.000 | 4.000 |
| build x Control | 1 | divexact | control | bytes | 0.056 | 0.288 | slower | 0.511 | 8.000 |
| build x Control | 1 | divexact | control | hybrid | 1.282 | 0.288 | within noise | 1.000 | 0.000 |
| build x Control | 1 | divexact | control | hyb2 | 1.283 | 0.288 | within noise | 1.000 | 0.000 |
| build x Control | 1 | divexact | control | hyb2p2 | 1.402 | 0.288 | faster | 1.000 | 0.000 |
| build x Control | 1 | divexact | control | hyb2ad | 1.28 | 0.288 | within noise | 1.000 | 0.000 |
| build x Control | 1 | gcd | control | pow2 | 1.082 | 0.05 | faster | 1.000 | 1.000 |
| build x Control | 1 | gcd | control | even | 0.994 | 0.05 | within noise | 1.000 | 1.000 |
| build x Control | 1 | gcd | control | growing | 0.213 | 0.05 | slower | 2.333 | 5.000 |
| build x Control | 1 | gcd | control | bytes | 0.104 | 0.05 | slower | 0.592 | 9.000 |
| build x Control | 1 | gcd | control | hybrid | 0.918 | 0.05 | slower | 1.000 | 0.000 |
| build x Control | 1 | gcd | control | hyb2 | 0.896 | 0.05 | slower | 1.000 | 0.000 |
| build x Control | 1 | gcd | control | hyb2p2 | 0.923 | 0.05 | slower | 1.000 | 0.000 |
| build x Control | 1 | gcd | control | hyb2ad | 0.953 | 0.05 | within noise | 1.000 | 0.000 |
| build x Control | 1 | absdiff2 | control | pow2 | 0.47 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | absdiff2 | control | even | 0.53 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 1 | absdiff2 | control | growing | 0.049 | 0.05 | slower | 2.000 | 2.000 |
| build x Control | 1 | absdiff2 | control | bytes | 0.01 | 0.05 | slower | 0.743 | 6.849 |
| build x Control | 1 | absdiff2 | control | hybrid | 0.319 | 0.05 | slower | 1.000 | 0.000 |
| build x Control | 1 | absdiff2 | control | hyb2 | 0.399 | 0.05 | slower | 1.000 | 0.000 |
| build x Control | 1 | absdiff2 | control | hyb2p2 | 0.399 | 0.05 | slower | 1.000 | 0.000 |
| build x Control | 1 | absdiff2 | control | hyb2ad | 0.399 | 0.05 | slower | 1.000 | 0.000 |
| build x Control | 2 | add | control | pow2 | 0.398 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | add | control | even | 0.387 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | add | control | growing | 0.025 | 0.05 | slower | 3.333 | 4.000 |
| build x Control | 2 | add | control | bytes | 0.004 | 0.05 | slower | 1.743 | 11.849 |
| build x Control | 2 | add | control | hybrid | 0.022 | 0.05 | slower | 3.333 | 4.000 |
| build x Control | 2 | add | control | hyb2 | 0.021 | 0.05 | slower | 3.333 | 4.000 |
| build x Control | 2 | add | control | hyb2p2 | 0.02 | 0.05 | slower | 3.333 | 4.000 |
| build x Control | 2 | add | control | hyb2ad | 0.021 | 0.05 | slower | 3.333 | 4.000 |
| build x Control | 2 | sub | control | pow2 | 0.403 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | sub | control | even | 0.396 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | sub | control | growing | 0.025 | 0.05 | slower | 3.000 | 3.000 |
| build x Control | 2 | sub | control | bytes | 0.004 | 0.05 | slower | 1.794 | 11.001 |
| build x Control | 2 | sub | control | hybrid | 0.023 | 0.05 | slower | 3.000 | 3.000 |
| build x Control | 2 | sub | control | hyb2 | 0.019 | 0.05 | slower | 3.000 | 3.000 |
| build x Control | 2 | sub | control | hyb2p2 | 0.022 | 0.05 | slower | 3.000 | 3.000 |
| build x Control | 2 | sub | control | hyb2ad | 0.013 | 0.05 | slower | 2.999 | 3.000 |
| build x Control | 2 | absdiff | control | pow2 | 0.565 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | absdiff | control | even | 0.63 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | absdiff | control | growing | 0.031 | 0.05 | slower | 3.000 | 3.000 |
| build x Control | 2 | absdiff | control | bytes | 0.008 | 0.05 | slower | 1.742 | 10.846 |
| build x Control | 2 | absdiff | control | hybrid | 0.029 | 0.05 | slower | 2.999 | 3.000 |
| build x Control | 2 | absdiff | control | hyb2 | 0.028 | 0.05 | slower | 2.999 | 3.000 |
| build x Control | 2 | absdiff | control | hyb2p2 | 0.029 | 0.05 | slower | 2.999 | 3.000 |
| build x Control | 2 | absdiff | control | hyb2ad | 0.028 | 0.05 | slower | 2.999 | 3.000 |
| build x Control | 2 | mul | control | pow2 | 0.316 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | mul | control | even | 0.454 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | mul | control | growing | 0.011 | 0.05 | slower | 3.667 | 5.000 |
| build x Control | 2 | mul | control | bytes | 0.003 | 0.05 | slower | 2.209 | 14.219 |
| build x Control | 2 | mul | control | hybrid | 0.011 | 0.05 | slower | 3.667 | 5.000 |
| build x Control | 2 | mul | control | hyb2 | 0.01 | 0.05 | slower | 3.667 | 5.000 |
| build x Control | 2 | mul | control | hyb2p2 | 0.01 | 0.05 | slower | 3.667 | 5.000 |
| build x Control | 2 | mul | control | hyb2ad | 0.01 | 0.05 | slower | 3.667 | 5.000 |
| build x Control | 2 | divexact | control | pow2 | - | - | trap | 1.000 | 1.000 |
| build x Control | 2 | divexact | control | even | - | - | trap | 1.000 | 1.000 |
| build x Control | 2 | divexact | control | growing | 0.087 | 0.05 | slower | 2.635 | 5.903 |
| build x Control | 2 | divexact | control | bytes | 0.032 | 0.05 | slower | 1.182 | 10.896 |
| build x Control | 2 | divexact | control | hybrid | 0.084 | 0.05 | slower | 1.693 | 5.903 |
| build x Control | 2 | divexact | control | hyb2 | 0.085 | 0.05 | slower | 1.693 | 5.903 |
| build x Control | 2 | divexact | control | hyb2p2 | 0.086 | 0.05 | slower | 1.693 | 5.903 |
| build x Control | 2 | divexact | control | hyb2ad | 0.084 | 0.05 | slower | 1.693 | 5.903 |
| build x Control | 2 | gcd | control | pow2 | 1.01 | 0.05 | within noise | 1.000 | 1.000 |
| build x Control | 2 | gcd | control | even | 1.078 | 0.05 | faster | 1.000 | 1.000 |
| build x Control | 2 | gcd | control | growing | 0.471 | 0.05 | slower | 2.996 | 6.987 |
| build x Control | 2 | gcd | control | bytes | 0.403 | 0.05 | slower | 1.433 | 12.968 |
| build x Control | 2 | gcd | control | hybrid | 0.476 | 0.05 | slower | 2.362 | 6.987 |
| build x Control | 2 | gcd | control | hyb2 | 0.474 | 0.05 | slower | 2.362 | 6.987 |
| build x Control | 2 | gcd | control | hyb2p2 | 0.479 | 0.05 | slower | 2.361 | 6.921 |
| build x Control | 2 | gcd | control | hyb2ad | 0.474 | 0.05 | slower | 2.362 | 6.987 |
| build x Control | 2 | absdiff2 | control | pow2 | 0.452 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | absdiff2 | control | even | 0.52 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 2 | absdiff2 | control | growing | 0.043 | 0.05 | slower | 3.000 | 3.000 |
| build x Control | 2 | absdiff2 | control | bytes | 0.007 | 0.05 | slower | 1.742 | 10.846 |
| build x Control | 2 | absdiff2 | control | hybrid | 0.038 | 0.05 | slower | 2.999 | 3.000 |
| build x Control | 2 | absdiff2 | control | hyb2 | 0.038 | 0.05 | slower | 2.999 | 3.000 |
| build x Control | 2 | absdiff2 | control | hyb2p2 | 0.039 | 0.05 | slower | 2.999 | 3.000 |
| build x Control | 2 | absdiff2 | control | hyb2ad | 0.037 | 0.05 | slower | 2.999 | 3.000 |
| build x Control | 4 | add | control | pow2 | 0.41 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | add | control | even | 0.389 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | add | control | growing | 0.022 | 0.05 | slower | 5.333 | 6.000 |
| build x Control | 4 | add | control | bytes | 0.002 | 0.05 | slower | 3.745 | 19.858 |
| build x Control | 4 | add | control | hybrid | 0.02 | 0.05 | slower | 5.333 | 6.000 |
| build x Control | 4 | add | control | hyb2 | 0.02 | 0.05 | slower | 5.333 | 6.000 |
| build x Control | 4 | add | control | hyb2p2 | 0.019 | 0.05 | slower | 5.333 | 6.000 |
| build x Control | 4 | add | control | hyb2ad | 0.02 | 0.05 | slower | 5.333 | 6.000 |
| build x Control | 4 | sub | control | pow2 | 0.416 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | sub | control | even | 0.402 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | sub | control | growing | 0.021 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | sub | control | bytes | 0.002 | 0.05 | slower | 3.791 | 18.994 |
| build x Control | 4 | sub | control | hybrid | 0.021 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | sub | control | hyb2 | 0.018 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | sub | control | hyb2p2 | 0.019 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | sub | control | hyb2ad | 0.013 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | absdiff | control | pow2 | 0.574 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | absdiff | control | even | 0.623 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | absdiff | control | growing | 0.029 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | absdiff | control | bytes | 0.005 | 0.05 | slower | 3.744 | 18.854 |
| build x Control | 4 | absdiff | control | hybrid | 0.027 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | absdiff | control | hyb2 | 0.026 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | absdiff | control | hyb2p2 | 0.027 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | absdiff | control | hyb2ad | 0.026 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | mul | control | pow2 | 0.33 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | mul | control | even | 0.454 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | mul | control | growing | 0.009 | 0.05 | slower | 6.333 | 9.000 |
| build x Control | 4 | mul | control | bytes | 0.002 | 0.05 | slower | 4.860 | 26.195 |
| build x Control | 4 | mul | control | hybrid | 0.009 | 0.05 | slower | 6.333 | 9.000 |
| build x Control | 4 | mul | control | hyb2 | 0.009 | 0.05 | slower | 6.333 | 9.000 |
| build x Control | 4 | mul | control | hyb2p2 | 0.009 | 0.05 | slower | 6.333 | 9.000 |
| build x Control | 4 | mul | control | hyb2ad | 0.009 | 0.05 | slower | 6.333 | 9.000 |
| build x Control | 4 | divexact | control | pow2 | - | - | trap | 1.000 | 1.000 |
| build x Control | 4 | divexact | control | even | - | - | trap | 1.000 | 1.000 |
| build x Control | 4 | divexact | control | growing | - | - | trap | 3.968 | 9.905 |
| build x Control | 4 | divexact | control | bytes | - | - | trap | 2.522 | 17.899 |
| build x Control | 4 | divexact | control | hybrid | - | - | trap | 3.968 | 9.905 |
| build x Control | 4 | divexact | control | hyb2 | - | - | trap | 3.968 | 9.905 |
| build x Control | 4 | divexact | control | hyb2p2 | - | - | trap | 3.968 | 9.905 |
| build x Control | 4 | divexact | control | hyb2ad | - | - | trap | 3.968 | 9.905 |
| build x Control | 4 | gcd | control | pow2 | 1.016 | 0.05 | within noise | 1.000 | 1.000 |
| build x Control | 4 | gcd | control | even | 1.05 | 0.05 | faster | 1.000 | 1.000 |
| build x Control | 4 | gcd | control | growing | 0.137 | 0.05 | slower | 4.667 | 12.001 |
| build x Control | 4 | gcd | control | bytes | 0.127 | 0.05 | slower | 3.097 | 21.987 |
| build x Control | 4 | gcd | control | hybrid | 0.137 | 0.05 | slower | 4.667 | 12.001 |
| build x Control | 4 | gcd | control | hyb2 | 0.137 | 0.05 | slower | 4.667 | 12.001 |
| build x Control | 4 | gcd | control | hyb2p2 | 0.138 | 0.05 | slower | 4.665 | 11.948 |
| build x Control | 4 | gcd | control | hyb2ad | 0.137 | 0.05 | slower | 4.667 | 12.001 |
| build x Control | 4 | absdiff2 | control | pow2 | 0.447 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | absdiff2 | control | even | 0.523 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 4 | absdiff2 | control | growing | 0.037 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | absdiff2 | control | bytes | 0.004 | 0.05 | slower | 3.744 | 18.854 |
| build x Control | 4 | absdiff2 | control | hybrid | 0.031 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | absdiff2 | control | hyb2 | 0.032 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | absdiff2 | control | hyb2p2 | 0.032 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 4 | absdiff2 | control | hyb2ad | 0.031 | 0.05 | slower | 5.000 | 5.000 |
| build x Control | 8 | add | control | pow2 | 0.416 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 8 | add | control | even | 0.39 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 8 | add | control | growing | 0.019 | 0.05 | slower | 9.333 | 10.000 |
| build x Control | 8 | add | control | bytes | 0.001 | 0.05 | slower | 7.752 | 35.866 |
| build x Control | 8 | add | control | hybrid | 0.017 | 0.05 | slower | 9.333 | 10.000 |
| build x Control | 8 | add | control | hyb2 | 0.017 | 0.05 | slower | 9.333 | 10.000 |
| build x Control | 8 | add | control | hyb2p2 | 0.016 | 0.05 | slower | 9.333 | 10.000 |
| build x Control | 8 | add | control | hyb2ad | 0.017 | 0.05 | slower | 9.333 | 10.000 |
| build x Control | 8 | sub | control | pow2 | 0.435 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 8 | sub | control | even | 0.422 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 8 | sub | control | growing | 0.02 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 8 | sub | control | bytes | 0.001 | 0.05 | slower | 7.795 | 34.996 |
| build x Control | 8 | sub | control | hybrid | 0.018 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 8 | sub | control | hyb2 | 0.016 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 8 | sub | control | hyb2p2 | 0.017 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 8 | sub | control | hyb2ad | 0.012 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 8 | absdiff | control | pow2 | 0.571 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 8 | absdiff | control | even | 0.62 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 8 | absdiff | control | growing | 0.025 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 8 | absdiff | control | bytes | 0.002 | 0.05 | slower | 7.751 | 34.862 |
| build x Control | 8 | absdiff | control | hybrid | 0.024 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 8 | absdiff | control | hyb2 | 0.023 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 8 | absdiff | control | hyb2p2 | 0.023 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 8 | absdiff | control | hyb2ad | 0.023 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 8 | mul | control | pow2 | 0.328 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 8 | mul | control | even | 0.471 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 8 | mul | control | growing | 0.004 | 0.05 | slower | 11.667 | 17.000 |
| build x Control | 8 | mul | control | bytes | 0.001 | 0.05 | slower | 10.191 | 50.191 |
| build x Control | 8 | mul | control | hybrid | 0.004 | 0.05 | slower | 11.667 | 17.000 |
| build x Control | 8 | mul | control | hyb2 | 0.004 | 0.05 | slower | 11.667 | 17.000 |
| build x Control | 8 | mul | control | hyb2p2 | 0.004 | 0.05 | slower | 11.667 | 17.000 |
| build x Control | 8 | mul | control | hyb2ad | 0.004 | 0.05 | slower | 11.667 | 17.000 |
| build x Control | 8 | divexact | control | pow2 | - | - | trap | 1.000 | 1.000 |
| build x Control | 8 | divexact | control | even | - | - | trap | 1.000 | 1.000 |
| build x Control | 8 | divexact | control | growing | - | - | trap | 6.637 | 17.911 |
| build x Control | 8 | divexact | control | bytes | - | - | trap | 5.182 | 31.902 |
| build x Control | 8 | divexact | control | hybrid | - | - | trap | 6.637 | 17.911 |
| build x Control | 8 | divexact | control | hyb2 | - | - | trap | 6.637 | 17.911 |
| build x Control | 8 | divexact | control | hyb2p2 | - | - | trap | 6.637 | 17.911 |
| build x Control | 8 | divexact | control | hyb2ad | - | - | trap | 6.637 | 17.911 |
| build x Control | 8 | gcd | control | pow2 | 1.008 | 0.05 | within noise | 1.000 | 1.000 |
| build x Control | 8 | gcd | control | even | 1.074 | 0.05 | faster | 1.000 | 1.000 |
| build x Control | 8 | gcd | control | growing | 0.048 | 0.05 | slower | 8.002 | 22.005 |
| build x Control | 8 | gcd | control | bytes | 0.044 | 0.05 | slower | 6.436 | 39.993 |
| build x Control | 8 | gcd | control | hybrid | 0.048 | 0.05 | slower | 8.002 | 22.005 |
| build x Control | 8 | gcd | control | hyb2 | 0.048 | 0.05 | slower | 8.002 | 22.005 |
| build x Control | 8 | gcd | control | hyb2p2 | 0.047 | 0.05 | slower | 7.999 | 21.889 |
| build x Control | 8 | gcd | control | hyb2ad | 0.048 | 0.05 | slower | 8.002 | 22.005 |
| build x Control | 8 | absdiff2 | control | pow2 | 0.456 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 8 | absdiff2 | control | even | 0.522 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 8 | absdiff2 | control | growing | 0.027 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 8 | absdiff2 | control | bytes | 0.002 | 0.05 | slower | 7.751 | 34.862 |
| build x Control | 8 | absdiff2 | control | hybrid | 0.025 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 8 | absdiff2 | control | hyb2 | 0.025 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 8 | absdiff2 | control | hyb2p2 | 0.024 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 8 | absdiff2 | control | hyb2ad | 0.025 | 0.05 | slower | 9.000 | 9.000 |
| build x Control | 16 | add | control | pow2 | 0.416 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | add | control | even | 0.392 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | add | control | growing | 0.014 | 0.05 | slower | 17.333 | 18.000 |
| build x Control | 16 | add | control | bytes | 0.001 | 0.05 | slower | 15.756 | 67.873 |
| build x Control | 16 | add | control | hybrid | 0.013 | 0.05 | slower | 17.333 | 18.000 |
| build x Control | 16 | add | control | hyb2 | 0.013 | 0.05 | slower | 17.333 | 18.000 |
| build x Control | 16 | add | control | hyb2p2 | 0.012 | 0.05 | slower | 17.333 | 18.000 |
| build x Control | 16 | add | control | hyb2ad | 0.013 | 0.05 | slower | 17.333 | 18.000 |
| build x Control | 16 | sub | control | pow2 | 0.415 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | sub | control | even | 0.401 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | sub | control | growing | 0.014 | 0.05 | slower | 17.000 | 17.000 |
| build x Control | 16 | sub | control | bytes | 0.001 | 0.05 | slower | 15.811 | 67.037 |
| build x Control | 16 | sub | control | hybrid | 0.013 | 0.05 | slower | 16.995 | 17.000 |
| build x Control | 16 | sub | control | hyb2 | 0.012 | 0.05 | slower | 16.995 | 17.000 |
| build x Control | 16 | sub | control | hyb2p2 | 0.01 | 0.05 | slower | 16.995 | 17.000 |
| build x Control | 16 | sub | control | hyb2ad | 0.009 | 0.05 | slower | 16.995 | 17.000 |
| build x Control | 16 | absdiff | control | pow2 | 0.57 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | absdiff | control | even | 0.616 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | absdiff | control | growing | 0.02 | 0.05 | slower | 17.000 | 17.000 |
| build x Control | 16 | absdiff | control | bytes | 0.001 | 0.05 | slower | 15.750 | 66.855 |
| build x Control | 16 | absdiff | control | hybrid | 0.019 | 0.05 | slower | 16.995 | 17.000 |
| build x Control | 16 | absdiff | control | hyb2 | 0.019 | 0.05 | slower | 16.995 | 17.000 |
| build x Control | 16 | absdiff | control | hyb2p2 | 0.019 | 0.05 | slower | 16.995 | 17.000 |
| build x Control | 16 | absdiff | control | hyb2ad | 0.018 | 0.05 | slower | 16.995 | 17.000 |
| build x Control | 16 | mul | control | pow2 | 0.315 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | mul | control | even | 0.449 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | mul | control | growing | 0.001 | 0.05 | slower | 22.333 | 33.000 |
| build x Control | 16 | mul | control | bytes | 0.0 | 0.05 | slower | 20.915 | 98.325 |
| build x Control | 16 | mul | control | hybrid | 0.001 | 0.05 | slower | 22.333 | 33.000 |
| build x Control | 16 | mul | control | hyb2 | 0.001 | 0.05 | slower | 22.333 | 33.000 |
| build x Control | 16 | mul | control | hyb2p2 | 0.001 | 0.05 | slower | 22.333 | 33.000 |
| build x Control | 16 | mul | control | hyb2ad | 0.001 | 0.05 | slower | 22.333 | 33.000 |
| build x Control | 16 | divexact | control | pow2 | - | - | trap | 1.000 | 1.000 |
| build x Control | 16 | divexact | control | even | - | - | trap | 1.000 | 1.000 |
| build x Control | 16 | divexact | control | growing | - | - | trap | 11.964 | 33.892 |
| build x Control | 16 | divexact | control | bytes | - | - | trap | 10.513 | 59.882 |
| build x Control | 16 | divexact | control | hybrid | - | - | trap | 11.964 | 33.892 |
| build x Control | 16 | divexact | control | hyb2 | - | - | trap | 11.964 | 33.892 |
| build x Control | 16 | divexact | control | hyb2p2 | - | - | trap | 11.964 | 33.892 |
| build x Control | 16 | divexact | control | hyb2ad | - | - | trap | 11.964 | 33.892 |
| build x Control | 16 | gcd | control | pow2 | 1.024 | 0.05 | within noise | 1.000 | 1.000 |
| build x Control | 16 | gcd | control | even | 1.078 | 0.05 | faster | 1.000 | 1.000 |
| build x Control | 16 | gcd | control | growing | 0.017 | 0.05 | slower | 14.681 | 42.043 |
| build x Control | 16 | gcd | control | bytes | 0.015 | 0.05 | slower | 13.130 | 76.027 |
| build x Control | 16 | gcd | control | hybrid | 0.017 | 0.05 | slower | 14.681 | 42.043 |
| build x Control | 16 | gcd | control | hyb2 | 0.017 | 0.05 | slower | 14.681 | 42.043 |
| build x Control | 16 | gcd | control | hyb2p2 | 0.016 | 0.05 | slower | 14.678 | 41.753 |
| build x Control | 16 | gcd | control | hyb2ad | 0.017 | 0.05 | slower | 14.681 | 42.043 |
| build x Control | 16 | absdiff2 | control | pow2 | 0.452 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | absdiff2 | control | even | 0.519 | 0.05 | slower | 1.000 | 1.000 |
| build x Control | 16 | absdiff2 | control | growing | 0.017 | 0.05 | slower | 17.000 | 17.000 |
| build x Control | 16 | absdiff2 | control | bytes | 0.001 | 0.05 | slower | 15.750 | 66.855 |
| build x Control | 16 | absdiff2 | control | hybrid | 0.016 | 0.05 | slower | 16.995 | 17.000 |
| build x Control | 16 | absdiff2 | control | hyb2 | 0.016 | 0.05 | slower | 16.995 | 17.000 |
| build x Control | 16 | absdiff2 | control | hyb2p2 | 0.016 | 0.05 | slower | 16.995 | 17.000 |
| build x Control | 16 | absdiff2 | control | hyb2ad | 0.016 | 0.05 | slower | 16.995 | 17.000 |
| absdiff x Control sub | 1 | - | control sub | control absdiff | 0.431 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 1 | - | control sub | control absdiff2 | 0.513 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 2 | - | control sub | control absdiff | 0.465 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 2 | - | control sub | control absdiff2 | 0.511 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 4 | - | control sub | control absdiff | 0.474 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 4 | - | control sub | control absdiff2 | 0.518 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 8 | - | control sub | control absdiff | 0.498 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 8 | - | control sub | control absdiff2 | 0.543 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 16 | - | control sub | control absdiff | 0.473 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 16 | - | control sub | control absdiff2 | 0.517 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 1 | - | control sub | pow2 absdiff | 0.272 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 1 | - | control sub | pow2 absdiff2 | 0.241 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 2 | - | control sub | pow2 absdiff | 0.262 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 2 | - | control sub | pow2 absdiff2 | 0.231 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 4 | - | control sub | pow2 absdiff | 0.272 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 4 | - | control sub | pow2 absdiff2 | 0.232 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 8 | - | control sub | pow2 absdiff | 0.284 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 8 | - | control sub | pow2 absdiff2 | 0.248 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 16 | - | control sub | pow2 absdiff | 0.269 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 16 | - | control sub | pow2 absdiff2 | 0.234 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 1 | - | control sub | even absdiff | 0.309 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 1 | - | control sub | even absdiff2 | 0.272 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 2 | - | control sub | even absdiff | 0.293 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 2 | - | control sub | even absdiff2 | 0.266 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 4 | - | control sub | even absdiff | 0.295 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 4 | - | control sub | even absdiff2 | 0.271 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 8 | - | control sub | even absdiff | 0.309 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 8 | - | control sub | even absdiff2 | 0.283 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 16 | - | control sub | even absdiff | 0.291 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 16 | - | control sub | even absdiff2 | 0.268 | 0.05 | slower | 1.000 | 1.000 |
| absdiff x Control sub | 1 | - | control sub | growing absdiff | 0.016 | 0.05 | slower | 2.000 | 2.000 |
| absdiff x Control sub | 1 | - | control sub | growing absdiff2 | 0.025 | 0.05 | slower | 2.000 | 2.000 |
| absdiff x Control sub | 2 | - | control sub | growing absdiff | 0.015 | 0.05 | slower | 3.000 | 3.000 |
| absdiff x Control sub | 2 | - | control sub | growing absdiff2 | 0.022 | 0.05 | slower | 3.000 | 3.000 |
| absdiff x Control sub | 4 | - | control sub | growing absdiff | 0.014 | 0.05 | slower | 5.000 | 5.000 |
| absdiff x Control sub | 4 | - | control sub | growing absdiff2 | 0.019 | 0.05 | slower | 5.000 | 5.000 |
| absdiff x Control sub | 8 | - | control sub | growing absdiff | 0.012 | 0.05 | slower | 9.000 | 9.000 |
| absdiff x Control sub | 8 | - | control sub | growing absdiff2 | 0.014 | 0.05 | slower | 9.000 | 9.000 |
| absdiff x Control sub | 16 | - | control sub | growing absdiff | 0.009 | 0.05 | slower | 17.000 | 17.000 |
| absdiff x Control sub | 16 | - | control sub | growing absdiff2 | 0.009 | 0.05 | slower | 17.000 | 17.000 |
| absdiff x Control sub | 1 | - | control sub | bytes absdiff | 0.005 | 0.05 | slower | 0.743 | 6.849 |
| absdiff x Control sub | 1 | - | control sub | bytes absdiff2 | 0.005 | 0.05 | slower | 0.743 | 6.849 |
| absdiff x Control sub | 2 | - | control sub | bytes absdiff | 0.004 | 0.05 | slower | 1.742 | 10.846 |
| absdiff x Control sub | 2 | - | control sub | bytes absdiff2 | 0.003 | 0.05 | slower | 1.742 | 10.846 |
| absdiff x Control sub | 4 | - | control sub | bytes absdiff | 0.002 | 0.05 | slower | 3.744 | 18.854 |
| absdiff x Control sub | 4 | - | control sub | bytes absdiff2 | 0.002 | 0.05 | slower | 3.744 | 18.854 |
| absdiff x Control sub | 8 | - | control sub | bytes absdiff | 0.001 | 0.05 | slower | 7.751 | 34.862 |
| absdiff x Control sub | 8 | - | control sub | bytes absdiff2 | 0.001 | 0.05 | slower | 7.751 | 34.862 |
| absdiff x Control sub | 16 | - | control sub | bytes absdiff | 0.001 | 0.05 | slower | 15.750 | 66.855 |
| absdiff x Control sub | 16 | - | control sub | bytes absdiff2 | 0.001 | 0.05 | slower | 15.750 | 66.855 |
| absdiff x Control sub | 1 | - | control sub | hybrid absdiff | 0.163 | 0.05 | slower | 1.000 | 0.000 |
| absdiff x Control sub | 1 | - | control sub | hybrid absdiff2 | 0.164 | 0.05 | slower | 1.000 | 0.000 |
| absdiff x Control sub | 2 | - | control sub | hybrid absdiff | 0.013 | 0.05 | slower | 2.999 | 3.000 |
| absdiff x Control sub | 2 | - | control sub | hybrid absdiff2 | 0.02 | 0.05 | slower | 2.999 | 3.000 |
| absdiff x Control sub | 4 | - | control sub | hybrid absdiff | 0.013 | 0.05 | slower | 5.000 | 5.000 |
| absdiff x Control sub | 4 | - | control sub | hybrid absdiff2 | 0.016 | 0.05 | slower | 5.000 | 5.000 |
| absdiff x Control sub | 8 | - | control sub | hybrid absdiff | 0.012 | 0.05 | slower | 9.000 | 9.000 |
| absdiff x Control sub | 8 | - | control sub | hybrid absdiff2 | 0.013 | 0.05 | slower | 9.000 | 9.000 |
| absdiff x Control sub | 16 | - | control sub | hybrid absdiff | 0.009 | 0.05 | slower | 16.995 | 17.000 |
| absdiff x Control sub | 16 | - | control sub | hybrid absdiff2 | 0.008 | 0.05 | slower | 16.995 | 17.000 |
| absdiff x Control sub | 1 | - | control sub | hyb2 absdiff | 0.205 | 0.05 | slower | 1.000 | 0.000 |
| absdiff x Control sub | 1 | - | control sub | hyb2 absdiff2 | 0.205 | 0.05 | slower | 1.000 | 0.000 |
| absdiff x Control sub | 2 | - | control sub | hyb2 absdiff | 0.013 | 0.05 | slower | 2.999 | 3.000 |
| absdiff x Control sub | 2 | - | control sub | hyb2 absdiff2 | 0.02 | 0.05 | slower | 2.999 | 3.000 |
| absdiff x Control sub | 4 | - | control sub | hyb2 absdiff | 0.012 | 0.05 | slower | 5.000 | 5.000 |
| absdiff x Control sub | 4 | - | control sub | hyb2 absdiff2 | 0.017 | 0.05 | slower | 5.000 | 5.000 |
| absdiff x Control sub | 8 | - | control sub | hyb2 absdiff | 0.011 | 0.05 | slower | 9.000 | 9.000 |
| absdiff x Control sub | 8 | - | control sub | hyb2 absdiff2 | 0.013 | 0.05 | slower | 9.000 | 9.000 |
| absdiff x Control sub | 16 | - | control sub | hyb2 absdiff | 0.009 | 0.05 | slower | 16.995 | 17.000 |
| absdiff x Control sub | 16 | - | control sub | hyb2 absdiff2 | 0.008 | 0.05 | slower | 16.995 | 17.000 |
| absdiff x Control sub | 1 | - | control sub | hyb2p2 absdiff | 0.205 | 0.05 | slower | 1.000 | 0.000 |
| absdiff x Control sub | 1 | - | control sub | hyb2p2 absdiff2 | 0.205 | 0.05 | slower | 1.000 | 0.000 |
| absdiff x Control sub | 2 | - | control sub | hyb2p2 absdiff | 0.013 | 0.05 | slower | 2.999 | 3.000 |
| absdiff x Control sub | 2 | - | control sub | hyb2p2 absdiff2 | 0.02 | 0.05 | slower | 2.999 | 3.000 |
| absdiff x Control sub | 4 | - | control sub | hyb2p2 absdiff | 0.013 | 0.05 | slower | 5.000 | 5.000 |
| absdiff x Control sub | 4 | - | control sub | hyb2p2 absdiff2 | 0.016 | 0.05 | slower | 5.000 | 5.000 |
| absdiff x Control sub | 8 | - | control sub | hyb2p2 absdiff | 0.012 | 0.05 | slower | 9.000 | 9.000 |
| absdiff x Control sub | 8 | - | control sub | hyb2p2 absdiff2 | 0.013 | 0.05 | slower | 9.000 | 9.000 |
| absdiff x Control sub | 16 | - | control sub | hyb2p2 absdiff | 0.009 | 0.05 | slower | 16.995 | 17.000 |
| absdiff x Control sub | 16 | - | control sub | hyb2p2 absdiff2 | 0.008 | 0.05 | slower | 16.995 | 17.000 |
| absdiff x Control sub | 1 | - | control sub | hyb2ad absdiff | 0.203 | 0.05 | slower | 1.000 | 0.000 |
| absdiff x Control sub | 1 | - | control sub | hyb2ad absdiff2 | 0.205 | 0.05 | slower | 1.000 | 0.000 |
| absdiff x Control sub | 2 | - | control sub | hyb2ad absdiff | 0.013 | 0.05 | slower | 2.999 | 3.000 |
| absdiff x Control sub | 2 | - | control sub | hyb2ad absdiff2 | 0.019 | 0.05 | slower | 2.999 | 3.000 |
| absdiff x Control sub | 4 | - | control sub | hyb2ad absdiff | 0.013 | 0.05 | slower | 5.000 | 5.000 |
| absdiff x Control sub | 4 | - | control sub | hyb2ad absdiff2 | 0.016 | 0.05 | slower | 5.000 | 5.000 |
| absdiff x Control sub | 8 | - | control sub | hyb2ad absdiff | 0.012 | 0.05 | slower | 9.000 | 9.000 |
| absdiff x Control sub | 8 | - | control sub | hyb2ad absdiff2 | 0.013 | 0.05 | slower | 9.000 | 9.000 |
| absdiff x Control sub | 16 | - | control sub | hyb2ad absdiff | 0.009 | 0.05 | slower | 16.995 | 17.000 |
| absdiff x Control sub | 16 | - | control sub | hyb2ad absdiff2 | 0.008 | 0.05 | slower | 16.995 | 17.000 |
