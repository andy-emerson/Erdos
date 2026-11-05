"""Independent check of the integer census (integer-census.md, Proposition 6).

For every rule in the given sym census logs with more than one cycle, take two of the reported
cycle keys (least |element|), iterate S in Python big-integer arithmetic from each, and confirm
that each key lies on a cycle of the reported odd length and that the two cycles are not equal
or negatives of each other. Also confirms S(1) = 1 for every rule.
Usage: python3 zverify.py logfile [logfile ...]
"""
import sys

def make_S(k, m):
    q, h = 2**k, 2**(k - 1)
    def S(x):
        r = (-m * x) % q
        if r > h: r -= q
        assert r % 2 and abs(r) < h
        y = m * x + r
        assert y % q == 0
        return y >> ((y & -y).bit_length() - 1)
    return S

ok = bad = 0
for fn in sys.argv[1:]:
    for line in open(fn):
        if line.startswith('#'): continue
        h, c = line.split('|'); h = h.split(); k, m = int(h[0]), int(h[1])
        S = make_S(k, m)
        assert S(1) == 1
        cyc = [t.split(':') for t in c.split()]
        if len(cyc) < 2: continue
        sets = []
        for key, L, _basin in cyc[:2]:
            x, orbit = int(key), []
            for _ in range(int(L)):
                orbit.append(x); x = S(x)
            good = x == int(key) and len(set(orbit)) == int(L) and min(abs(t) for t in orbit) == int(key)
            sets.append(frozenset(orbit))
            if not good: bad += 1; print('FAIL', k, m, key, L)
        if sets[0] == sets[1] or sets[0] == frozenset(-t for t in sets[1]):
            bad += 1; print('SAME', k, m)
        else: ok += 1
print(f'{ok} rules with two distinct cycles up to sign confirmed, {bad} failures')
