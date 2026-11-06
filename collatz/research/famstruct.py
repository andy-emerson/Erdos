"""Structure tests on the cycle data written by families.py (families.md, Section 4).

usage: famstruct.py OUT.json
  1. classes (n, V, eps) with counts; the valuation vectors as necklaces (least rotation) and how many
     cycles share each necklace; Hamming distance between necklaces within a class.
  2. affine test: for every pair of cycles in the same class, is there c, e in Q(i) with
     w_j = c z_{j+r} + e for all j (some rotation r, after replacing the second cycle by any associate or
     by the conjugate cycle)?  Unit and conjugation maps are reported separately from other maps.
  3. differences: is the difference of canonical elements of consecutive cycles (by norm) constant
     on any run of three or more?
  4. angles of canonical elements and of all cycle elements (histogram mod 90 degrees).
  5. composite classes: for a cycle with n = m n0 (n0 the main class), whether every block of n0
     consecutive valuations sums to V0, and how close S^n0 z is to eps0 z along the cycle."""
import sys, json, math, cmath
from fractions import Fraction
from collections import Counter, defaultdict

d = json.load(open(sys.argv[1]))
C = d['cycles']
for c in C:
    c['els'] = [tuple(e) for e in c['els']]; c['unit'] = tuple(c['unit']); c['canon'] = tuple(c['canon'])
def neck(v):
    return min(tuple(v[i:] + v[:i]) for i in range(len(v)))
cls = defaultdict(list)
for c in C: cls[(c['odd'], c['V'], c['unit'])].append(c)
print(f"# {sys.argv[1]}: alpha={d['alpha']} k={d['k']} B={d['B']}, {len(C)} cycles")
print("## 1. classes")
for key, L in sorted(cls.items(), key=lambda t: -len(t[1])):
    nk = Counter(neck(c['vals']) for c in L)
    print(f"class n={key[0]} V={key[1]} eps={key[2]}: {len(L)} cycles, {len(nk)} distinct valuation necklaces, "
          f"multiplicities {sorted(Counter(nk.values()).items())}")
    if 1 < len(L) <= 200 and key[0] <= 40:
        ns = list(nk)
        def ham(a, b):
            return min(sum(x != y for x, y in zip(a, b[i:] + b[:i])) for i in range(len(b)))
        hs = Counter(ham(a, b) for i, a in enumerate(ns) for b in ns[i+1:])
        print(f"   Hamming distances between necklaces (min over rotations): {sorted(hs.items())}")

def cx(z): return complex(z[0], z[1])
def fq(z):
    for u in ((1,0),(0,1),(-1,0),(0,-1)):
        w = (u[0]*z[0]-u[1]*z[1], u[0]*z[1]+u[1]*z[0])
        if w[0] > 0 and w[1] >= 0: return w
print("## 2. affine maps between cycles of the same class")
def affine(z, w):
    """w_j = a z_j + b for all j, exact; z, w lists of Gaussian integers; returns (a, b) or None"""
    if len(z) < 2: return None
    # find two distinct z
    if z[0] == z[1]: return None
    zc = [complex(*p) for p in z[:3]]; wc = [complex(*p) for p in w[:3]]
    a_ = (wc[1]-wc[0])/(zc[1]-zc[0]); b_ = wc[0] - a_*zc[0]
    if len(z) > 2 and abs(a_*zc[2] + b_ - wc[2]) > 1e-6*(1+abs(wc[2])): return None
    def sub(p, q): return (p[0]-q[0], p[1]-q[1])
    dz = sub(z[1], z[0]); dw = sub(w[1], w[0])
    nz = dz[0]**2 + dz[1]**2
    a = (Fraction(dw[0]*dz[0] + dw[1]*dz[1], nz), Fraction(dw[1]*dz[0] - dw[0]*dz[1], nz))
    b = (w[0][0] - (a[0]*z[0][0] - a[1]*z[0][1]), w[0][1] - (a[0]*z[0][1] + a[1]*z[0][0]))
    for p, q in zip(z, w):
        if (a[0]*p[0] - a[1]*p[1] + b[0], a[0]*p[1] + a[1]*p[0] + b[1]) != q: return None
    return a, b
found = Counter(); examples = []
for key, L in cls.items():
    if key[0] < 2: continue
    for i in range(len(L)):
        z = L[i]['els']
        for j in range(i+1, len(L)):
            w0 = L[j]['els']
            for conj, norm_ in ((False, False), (True, False), (False, True), (True, True)):
                # norm_: compare first-quadrant associates (conjugation multiplies each step by a unit power)
                ws = [(p[0], -p[1]) for p in w0] if conj else w0
                if norm_: ws = [fq(p) for p in ws]
                zz = [fq(p) for p in L[i]['els']] if norm_ else L[i]['els']
                for r in range(len(ws)):
                    w = ws[r:] + ws[:r]
                    m = affine(zz, w)
                    if m:
                        a, b = m
                        triv = (a[0]**2 + a[1]**2 == 1 and b == (0, 0))
                        found[(key, 'conj' if conj else 'lin', 'fq' if norm_ else 'raw', 'unit' if triv else 'other')] += 1
                        if not triv and len(examples) < 10: examples.append((L[i]['canon'], L[j]['canon'], a, b))
for k_, v in sorted(found.items(), key=str): print(f"  {k_}: {v} pairs")
if not found: print("  none")
for e in examples: print("  example", e)

print("## 3. constant differences between consecutive canonical elements (sorted by norm)")
for key, L in cls.items():
    if len(L) < 3: continue
    s = sorted(L, key=lambda c: (c['minN'], c['canon']))
    diffs = [(s[i+1]['canon'][0] - s[i]['canon'][0], s[i+1]['canon'][1] - s[i]['canon'][1]) for i in range(len(s)-1)]
    runs = 0; best = 1; cur = 1
    for i in range(1, len(diffs)):
        cur = cur + 1 if diffs[i] == diffs[i-1] else 1; best = max(best, cur)
    print(f"  class {key}: longest run of equal consecutive differences: {best}")
    # any difference vector shared by many pairs of cycles in the class (canonical elements)?
    allc = [c['canon'] for c in L]
    dd = Counter((b[0]-a[0], b[1]-a[1]) for a in allc for b in allc if a != b)
    top = dd.most_common(3)
    print(f"   most common pairwise difference of canonical elements: {top}")

print("## 4. angles (degrees mod 90) and log-norm of cycle elements")
for key, L in sorted(cls.items(), key=lambda t: -len(t[1]))[:3]:
    h = Counter(); hc = Counter()
    for c in L:
        hc[int((math.degrees(math.atan2(c['canon'][1], c['canon'][0])) % 90) // 10)] += 1
        for e in c['els']: h[int((math.degrees(math.atan2(e[1], e[0])) % 90) // 10)] += 1
    print(f"  class {key}: canonical elements by 10-degree bin mod 90: {[hc[b] for b in range(9)]}")
    print(f"               all elements by 10-degree bin mod 90:       {[h[b] for b in range(9)]}")
    lmin = sorted(round(math.log10(c['minN']), 1) for c in L)
    print(f"   log10 least norm: quartiles {lmin[0]}, {lmin[len(lmin)//4]}, {lmin[len(lmin)//2]}, {lmin[3*len(lmin)//4]}, {lmin[-1]}")

print("## 5. composite classes")
main = max(cls.items(), key=lambda t: len(t[1]))[0]
n0, V0, e0 = main
for key, L in cls.items():
    n, V, eps = key
    if n <= n0: continue
    for c in L:
        v = c['vals']; blocks = []
        # best rotation: number of block boundaries with block sum V0
        best = None
        for r in range(n):
            w = v[r:] + v[:r]
            sums = [sum(w[i:i+n0]) for i in range(0, n - n0 + 1, n0)]
            ok = sum(s == V0 for s in sums)
            if best is None or ok > best[0]: best = (ok, sums, r)
        # how close is z_{j+n0} to e0 z_j (relative distance), along the cycle
        els = c['els']; m = len(els)
        e0c = complex(*e0)
        dev = [abs(cx(els[(j+n0) % m]) - e0c*cx(els[j]))/abs(cx(els[j])) for j in range(m)] if n % n0 == 0 else []
        print(f"  {c['canon']} n={n} V={V}: n/n0={n/n0:.3f}; blocks of {n0} with sum {V0}: {best[0]} (block sums {best[1][:12]})"
              + (f"; |z_(j+n0) - eps0 z_j|/|z_j|: min {min(dev):.3g} median {sorted(dev)[m//2]:.3g} max {max(dev):.3g}" if dev else ""))
