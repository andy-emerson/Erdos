"""Escaping orbits of real Eisenstein multipliers (integer-census.md, Section 4).

About 200 random odd starts with |a|, |b| <= 300 are followed for up to 20000 odd steps under
the minimal symmetric rule (first minimal digit set) at level k with multiplier alpha = a + b w;
orbits passing norm 2^400 are counted, and their last point, rotated into the sector 0 <= b < a,
is described by its coordinates b and a - b (a value 1 means the point lies on the line
t + w or t(1 + w) + 1 type, at distance one from a unit line).
Usage: python3 zeisline.py k a b
"""
import sys, math, random, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import zeis

k, alpha = int(sys.argv[1]), (int(sys.argv[2]), int(sys.argv[3]))
best, orbits = zeis.min_digit_sets(k)
D = [zeis.mul(u, els[0]) for c, (n, els) in orbits for u in zeis.UNITS]
S = zeis.make_S(alpha, k, zeis.digit_table(k, D))
random.seed(1); tried = esc = 0; kinds = {}
for t in range(200):
    z = (random.randint(-300, 300), random.randint(-300, 300))
    if not zeis.odd(z): continue
    tried += 1
    for n in range(20000):
        z = S(z)
        if zeis.N(z) > 2**400: break
    if zeis.N(z) > 2**400:
        esc += 1; a, b = zeis.canon(z)
        kind = 'b = %d' % b if abs(b) < 100 else ('a - b = %d' % (a - b) if abs(a - b) < 100 else 'off the lines')
        kinds[kind] = kinds.get(kind, 0) + 1
print(f'k={k} alpha={alpha}: {esc} of {tried} starts passed norm 2^400; final points: {kinds}')
