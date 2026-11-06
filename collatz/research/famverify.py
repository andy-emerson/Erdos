"""Exact check of every cycle in a families.py JSON file (families.md, Section 5).

For each cycle z_0, ..., z_{n-1} with valuations v_j and unit eps (S^n z_0 = eps z_0) it recovers the digit
d_j = (1+i)^{v_j} z_{j+1} - alpha z_j and checks, in exact Gaussian-integer arithmetic:
  (a) d_j is in the digit set D and (1+i)^k divides alpha z_j + d_j (the congruence that selects the digit;
      D has one element per odd class mod (1+i)^k, so this is the digit the rule uses);
  (b) z_{j+1} is odd, so v_j is the full valuation;
  (c) the closed cycle equation z_0 (eps (1+i)^V - alpha^n) = sum_j d_j alpha^(n-1-j) (1+i)^(V - s_j),
      s_j = v_j + ... + v_{n-1}, which is S^n z_0 = eps z_0 written out;
  (d) the n points are pairwise non-associate (so n is the exact length up to units).
It also prints N(den) for each class exactly, and N(den)/2^V, the squared relative denominator.
usage: famverify.py OUT.json [more.json ...]"""
import sys, json
from fractions import Fraction

def mul(a, b): return (a[0]*b[0]-a[1]*b[1], a[0]*b[1]+a[1]*b[0])
def add(a, b): return (a[0]+b[0], a[1]+b[1])
def sub(a, b): return (a[0]-b[0], a[1]-b[1])
def pw(a, n):
    r = (1, 0)
    while n:
        if n & 1: r = mul(r, a)
        a = mul(a, a); n >>= 1
    return r
def val(z):
    x, y = z; v = 0
    while (x + y) % 2 == 0:
        if x == 0 and y == 0: return 10**9
        x, y = (x + y)//2, (y - x)//2; v += 1
    return v
UNITS = [(1,0),(0,1),(-1,0),(0,-1)]
def fq(z):
    for u in UNITS:
        w = mul(u, z)
        if w[0] > 0 and w[1] >= 0: return w

for f in sys.argv[1:]:
    d = json.load(open(f)); k = d['k']; al = tuple(d['alpha'])
    D = set(mul(u, tuple(g)) for g in d['gens'] for u in UNITS)
    P = (1, 1); ok = 0; bad = 0; dens = {}
    for c in d['cycles']:
        z = [tuple(e) for e in c['els']]; v = c['vals']; n = len(z); eps = tuple(c['unit']); V = sum(v)
        good = len(set(fq(e) for e in z)) == n and V == c['V'] and n == c['odd']
        digs = []
        for j in range(n):
            nxt = z[j+1] if j+1 < n else mul(eps, z[0])
            dj = sub(mul(pw(P, v[j]), nxt), mul(al, z[j]))
            digs.append(dj)
            good &= dj in D and val(add(mul(al, z[j]), dj)) >= k and val(nxt) == 0
        # closed form
        s = [sum(v[j:]) for j in range(n)]
        rhs = (0, 0)
        for j in range(n):
            rhs = add(rhs, mul(digs[j], mul(pw(al, n-1-j), pw(P, V - s[j]))))
        den = sub(mul(eps, pw(P, V)), pw(al, n))
        good &= mul(z[0], den) == rhs
        key = (n, V, eps)
        if key not in dens:
            Nd = den[0]**2 + den[1]**2; dens[key] = (Nd, float(Fraction(Nd, 2**V)))
        ok += good; bad += not good
    print(f"{f}: alpha={al} k={k}: {ok} cycles verified, {bad} failed")
    for key, (Nd, r) in sorted(dens.items(), key=lambda t: t[1][1])[:6]:
        print(f"   class n={key[0]} V={key[1]} eps={key[2]}: N(den) = {Nd if Nd < 10**30 else 'about 10^%d' % (len(str(Nd))-1)}, N(den)/2^V = {r:.4e}")
