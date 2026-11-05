"""The eight lines L: x = +-1, y = +-1, x - y = +-1, x + y = +-1 (families.md, Section 6).
For a rule, check S(L) in L for every odd point of L with parameter m in a window [M, M + W), and give the
mean valuation of S on L over the window (one full period of residues mod 2^K for K up to log2 W), to compare
with log2 N(alpha): on L, log|z| changes by (1/2) log2 N(alpha) - v/2 bits per odd step, plus O(1/|z|).
usage: famline1d.py k ax,ay g1x,g1y;... [M] [W]"""
import sys, math
sys.path.insert(0, '.')
import rules as R, exact as E
from collections import Counter
k = int(sys.argv[1]); al = tuple(map(int, sys.argv[2].split(','))); G = [tuple(map(int, g.split(','))) for g in sys.argv[3].split(';')]
M = int(sys.argv[4]) if len(sys.argv) > 4 else 10**12; W = int(sys.argv[5]) if len(sys.argv) > 5 else 2**18
D = [R.mul(u, g) for g in G for u in R.UNITS]; S = E.make_S(al, D, k)
def online(z): x, y = z; return min(abs(x), abs(y), abs(x - y), abs(x + y)) == 1
pts = 0; stay = 0; vs = Counter()
for m in range(M, M + W):
    for z in ((m, 1), (m, -1), (m, m + 1), (m, m - 1)):     # y = +-1 and x - y = -+1; other six lines are unit images
        if (z[0] + z[1]) % 2 == 0: continue
        w, v = S(z); pts += 1; vs[v] += 1; stay += online(w)
mv = sum(v*c for v, c in vs.items()) / pts
print(f"k={k} alpha={al}: {stay}/{pts} odd points of L map into L (m in [{M}, {M+W})); mean valuation {mv:.5f}; "
      f"log2 N(alpha) = {math.log2(al[0]**2 + al[1]**2):.5f}; drift on L {(math.log2(al[0]**2+al[1]**2) - mv)/2:+.5f} bits of |z| per odd step;"
      f" valuation counts {sorted(vs.items())[:8]}")

# split by line type: A = axis-parallel (x or y = +-1), Dg = diagonal (x -+ y = +-1).  An odd step multiplies the
# direction of z by alpha/(1+i)^v up to O(1/|z|), so it changes the type exactly when v is odd (alpha real).
def typ(z): x, y = z; return 'A' if min(abs(x), abs(y)) == 1 else 'Dg'
st = {t: Counter() for t in ('A', 'Dg')}; tr = Counter()
for m in range(M, M + W):
    for z in ((m, 1), (m, -1), (m, m + 1), (m, m - 1)):
        if (z[0] + z[1]) % 2 == 0: continue
        w, v = S(z); st[typ(z)][v] += 1; tr[(typ(z), typ(w))] += 1
mean = {t: sum(v*c for v, c in st[t].items()) / sum(st[t].values()) for t in st}
pA = {t: tr[(t, 'A')] / (tr[(t, 'A')] + tr[(t, 'Dg')]) for t in st}
# stationary distribution of the two-state chain
piA = pA['Dg'] / (1 - pA['A'] + pA['Dg'])
mv2 = piA * mean['A'] + (1 - piA) * mean['Dg']
print(f"   by type: mean v on A {mean['A']:.5f}, on Dg {mean['Dg']:.5f}; P(A->A) {pA['A']:.4f}, P(Dg->A) {pA['Dg']:.4f};"
      f" stationary share of A {piA:.4f}; mean v {mv2:.5f}; drift {(math.log2(al[0]**2+al[1]**2) - mv2)/2:+.5f} bits per odd step")
