"""Norm-5 multipliers at level 3 with the units as digits (paper draft 2, Section 3.8).

For alpha in {1+2i, 2+i, 1-2i, 2-i}: every odd start with N(z) <= 10^5 is followed under the
exact accelerated map (exact.make_S); checks that every odd step lowers the norm, counts odd steps
to a unit, and lists the cycles up to units.
Usage: python3 below_window.py   (from research/collatz)
"""
import rules as R, exact as E

B = 10**5
r = int(B ** 0.5) + 1
for alpha in [(1, 2), (2, 1), (1, -2), (2, -1)]:
    S = E.make_S(alpha, R.UNITS, 3)
    cyc, maxsteps, decreasing = {}, 0, True
    for a in range(-r, r + 1):
        for b in range(-r, r + 1):
            if a * a + b * b > B or (a + b) % 2 == 0: continue
            z, n = (a, b), 0
            while R.norm(z) > 1:
                w, _ = S(z)
                if R.norm(w) >= R.norm(z): decreasing = False
                z, n = w, n + 1
            maxsteps = max(maxsteps, n)
            c = tuple(sorted(E.proj_cycle(S, z)[1]))
            cyc[c] = cyc.get(c, 0) + 1
    print('alpha', alpha, 'S on units', [S(u)[0] for u in R.UNITS],
          'cycles up to units', list(cyc), 'max odd steps to a unit', maxsteps,
          'every odd step lowered the norm', decreasing)
