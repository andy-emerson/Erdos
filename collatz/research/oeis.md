# OEIS searches (run by the author, Nov 5, 2026)

1. Calibration. `8, 1, 20, 2, 36, 3, 48, 4` found A317640, "The 7x+-1 function" (David Barina, Aug 2 2018), which links Barina, arXiv:1807.00908, and K. Matthews, "David Barina's 7x+1 conjecture". The search method works.
2. The 5x ± 1 map (n even: n/2; n ≡ 1 mod 4: 5n − 1; n ≡ 3 mod 4: 5n + 1), values for n = 1, 2, 3, ...: `4, 1, 16, 2, 24, 3, 36, 4, 44, 5, 56`. No match.
3. Its accelerated form on odd n = 1, 3, 5, ...: `1, 1, 3, 9, 11, 7, 1, 19, 21`. No match.
4. The cycle through 7 under the full map, unsigned search (so it also covers every signed sequence): `7, 36, 18, 9, 44, 22, 11, 56, 28, 14, 7`. One match, A305057, "-5x + 1 sequence starting at 5" (Alonso del Arte, May 26 2018; cross-reference A259207), whose terms −7, 36, 18, 9, −44, −22, −11, 56, 28, 14, 7 agree with the cycle up to sign; the −5x+1 map then sends 7 to −34, so the cycle itself is not recorded there.
5. Conclusion: 5x ± 1 and its cycle through 7 are not in the OEIS. A text search with the word "sign" (not the `sign:` prefix) also found nothing; it adds no information given item 4.
