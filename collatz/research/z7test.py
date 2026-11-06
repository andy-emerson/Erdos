"""Independent check of the z7stop sieve in Python integers.  For random odd n in (lo, 2 lo], run
Barina's 7x +- 1 on n itself through the prefix whose steps are fixed by n mod 2^k (a halving
uses one bit, an odd step needs two bits left); if no iterate in that prefix is below n, then
n mod 2^k must be listed as a survivor.  Also checks the mod-7 predecessor claim: for n = 2 or 5
(mod 7), m = (4n -+ 1)/7 is an odd integer with f^3(m) = n and m < n.
usage: python3 z7test.py lo k survivor-file samples"""
import random
import sys

lo, k, path, S = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3], int(sys.argv[4])
surv = set(int(x) for x in open(path))


def f(n):
    return n // 2 if n % 2 == 0 else 7 * n + (1 if n % 4 == 1 else -1)


bad = dropped = 0
for _ in range(S):
    n = random.randrange(lo + 1, 2 * lo) | 1
    x, bits, d = n, k, False
    while True:
        if x % 2 == 0:
            if bits < 1:
                break
            bits -= 1
        elif bits < 2:
            break
        x = f(x)
        if x < n:
            d = True
            break
    if d:
        dropped += 1
    elif n % (1 << k) not in surv:
        bad += 1
    for m7, dd in ((2, -1), (5, 1)):
        if n % 7 == m7:
            m = (4 * n + dd) // 7
            assert (4 * n + dd) % 7 == 0 and m % 2 == 1 and f(f(f(m))) == n and m < n, n
print("samples %d, dropped within the prefix %d, survivors needed %d, non-dropping non-survivors %d (must be 0)"
      % (S, dropped, S - dropped, bad))
