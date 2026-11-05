"""Orbits started on the lines L (families.md, Section 6): iterate S exactly from points m + i, m + (m+1)i
with m random in [m0, 2 m0), until the orbit repeats an associate class, or |z| > 2^E, or n_max odd steps.
usage: famlinerun.py k ax,ay g1x,g1y;... m0 trials [E=400] [nmax=200000]"""
import sys, random, math
sys.path.insert(0, '.')
import rules as R, exact as E_
from collections import Counter
k = int(sys.argv[1]); al = tuple(map(int, sys.argv[2].split(','))); G = [tuple(map(int, g.split(','))) for g in sys.argv[3].split(';')]
m0 = int(sys.argv[4]); T = int(sys.argv[5]); Eb = int(sys.argv[6]) if len(sys.argv) > 6 else 400; nmax = int(sys.argv[7]) if len(sys.argv) > 7 else 200000
D = [R.mul(u, g) for g in G for u in R.UNITS]; S = E_.make_S(al, D, k)
random.seed(7); out = Counter(); cyc = Counter(); off = 0
for t in range(T):
    m = random.randrange(m0, 2*m0)
    z = random.choice([(m, 1), (m, m + 1)])
    if (z[0] + z[1]) % 2 == 0: z = (z[0] + 1, z[1]) if z[1] == 1 else (z[0] + 1, z[1] + 1)
    seen = set(); n = 0; res = None
    while n < nmax:
        a = R.assoc_min(z)
        if a in seen: res = 'cycle'; break
        if n < 5000: seen.add(a)
        z, v = S(z); n += 1
        if min(abs(z[0]), abs(z[1]), abs(z[0]-z[1]), abs(z[0]+z[1])) != 1: off += 1
        if z[0]*z[0] + z[1]*z[1] > 1 << (2*Eb): res = 'escape'; break
    if res is None: res = 'cap'
    out[res] += 1
    if res == 'cycle': cyc[R.assoc_min(z) if R.norm(z) < 10**6 else 'large'] += 1
print(f"k={k} alpha={al} starts on L with m in [{m0},{2*m0}): {dict(out)}; steps leaving L: {off}; cycles (assoc_min of a point): {cyc.most_common(5)}")
