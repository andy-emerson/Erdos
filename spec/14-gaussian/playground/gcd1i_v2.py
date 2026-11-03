# Study 11: study 9's (1+i)-ary gcd with the ideas of Damgard & Frandsen's Eisenstein algorithm (BRICS RS-03-8, 2003),
# each as a separate variant:
#   (a) stop when the two numbers are equal (both primary, so equal means a - b = 0): one step fewer than stopping at b = 0
#   (b) compare sizes by their approximate norm: each coordinate's absolute value with all but its 6 most significant bits
#       cleared, then squared and summed (their Lemma 2 gives (8/9) N <= N~ <= N for Z[zeta]; for Z[i] the factor is
#       (31/32)^2, about 0.94). They use it because it is linear time on long numbers.
#   (c) both
#   (d) not from the paper: a word-friendly size, |x| + |y| (the L1 size, N <= L1^2 <= 2N), no multiply.
#       Termination: replacing a with L1(a) >= L1(b) gives N(b) <= 2 N(a), so N(new a) <= (|a| + |b|)^2 / 8
#       <= (1 + sqrt 2)^2 / 8 N(a) < 0.73 N(a); the product of the norms falls by a factor > 1.37 per step.
#   (e) (d) with (a)
# Everything else (strip, primary, the gcd's power of (1+i), normalise) is study 9's, unchanged.
# Usage: python3 gcd1i_v2.py K   (K <= 5 exhaustive over all pairs; K = 32 random, the same pairs as study 9)
import sys, random
from fractions import Fraction
K = int(sys.argv[1]) if len(sys.argv) > 1 else 4
W = 2*K; MW = (1 << W) - 1
HI = (1 << (K-1)) - 1

def ctz(v):
    v &= MW; return W if v == 0 else (v & -v).bit_length() - 1
def clz(v):
    v &= MW; return W - v.bit_length()
def strip(x, y):                                   # study 9: 16
    e = ctz(x | y) % W; x >>= e; y >>= e
    m = -(x & y & 1); px, py = (x + y) >> 1, (y - x) >> 1
    return x ^ ((x ^ px) & m), y ^ ((y ^ py) & m)
def primary(x, y):                                 # study 9: 17
    m = (x & 1) - 1
    nx, ny = x ^ ((x ^ y) & m), y ^ ((y ^ -x) & m)
    m = -(((nx - ny) >> 1) & 1)
    return (nx ^ m) - m, (ny ^ m) - m
def val_pi(x, y): e = ctz(x | y); return 2*e + ((x >> e) & (y >> e) & 1)
def cswap(p, q, m): d = (p ^ q) & m; return p ^ d, q ^ d

# --- size measures
def size_exact(x, y): return x*x + y*y                          # 2 MUL, ADD = 3
def top6(d):                     # |d| with all but its 6 most significant bits cleared.
    m = -1 if d < 0 else 0; a = (d ^ m) - m                     # SAR, XOR, SUB (3)
    s = max((W - clz(a)) - 6, 0)                                # CLZ, SUB (from a constant), max: compare + choice (5)
    return a & (-1 << s)                                        # SHL of all-ones, AND (2)          10 per coordinate
def size_df(x, y): tx, ty = top6(x), top6(y); return tx*tx + ty*ty   # 2 x 10 + 2 MUL + ADD = 23
def size_l1(x, y): return abs(x) + abs(y)                       # 2 x ABS (3 each) + ADD = 7, no multiply
SIZES = {'exact': (size_exact, 3, 2), 'df': (size_df, 23, 2), 'l1': (size_l1, 7, 0)}   # (function, instructions, multiplies)

def gcd_pi(a, b, size, stop_equal):
    (a1, a2), (b1, b2) = a, b
    if (a1, a2) == (0, 0) or (b1, b2) == (0, 0):
        g = (b1, b2) if (a1, a2) == (0, 0) else (a1, a2); return normalise(*g), 0
    k = min(val_pi(a1, a2), val_pi(b1, b2))
    a1, a2 = primary(*strip(a1, a2)); b1, b2 = primary(*strip(b1, b2))
    sa, sb = size(a1, a2), size(b1, b2)
    m = -int(sa < sb); a1, b1 = cswap(a1, b1, m); a2, b2 = cswap(a2, b2, m); sa, sb = cswap(sa, sb, m)
    steps = 0
    while ((a1, a2) != (b1, b2)) if stop_equal else ((b1, b2) != (0, 0)):   # equal: XOR, XOR, OR, compare (4); zero: OR, compare (2)
        a1, a2 = primary(*strip(a1 - b1, a2 - b2))                          # 2 + 16 + 17
        sa = size(a1, a2)                                                   # size
        m = -int(sa < sb)                                                   # 2
        a1, b1 = cswap(a1, b1, m); a2, b2 = cswap(a2, b2, m); sa, sb = cswap(sa, sb, m)   # 12
        steps += 1
        assert max(abs(a1), abs(a2), abs(b1), abs(b2)) < (1 << K)
    g1, g2 = a1 << (k >> 1), a2 << (k >> 1)
    if k & 1: g1, g2 = g1 - g2, g1 + g2
    return normalise(g1, g2), steps
def normalise(x, y):
    for _ in range(4):
        if (x > 0 and y >= 0) or (x, y) == (0, 0): return (x, y)
        x, y = -y, x

def ref_round(f): return (f.numerator * 2 + f.denominator) // (2 * f.denominator)
def euclid(a, b):                                  # study 6's Euclid (its reference)
    steps = 0
    while b != (0, 0):
        n = b[0]**2 + b[1]**2
        q = (ref_round(Fraction(a[0]*b[0] + a[1]*b[1], n)), ref_round(Fraction(a[1]*b[0] - a[0]*b[1], n)))
        a, b = b, (a[0] - (q[0]*b[0] - q[1]*b[1]), a[1] - (q[0]*b[1] + q[1]*b[0])); steps += 1
    return normalise(*a), steps

VARIANTS = {'study 9 (exact norm, stop at 0)': ('exact', False), '(a) stop when equal': ('exact', True),
            '(b) D&F approximate norm': ('df', False), '(c) (a) + (b)': ('df', True),
            '(d) L1 size |x|+|y|': ('l1', False), '(e) (d) + (a)': ('l1', True)}
st = {}; hist = {v: {} for v in VARIANTS}; ehist = {}
def chk(k, ok):
    st[k + ' cases'] = st.get(k + ' cases', 0) + 1
    if not ok: st[k + ' FAIL'] = st.get(k + ' FAIL', 0) + 1
def test(a, b):
    r, es = euclid(a, b); ehist[es] = ehist.get(es, 0) + 1
    for name, (sz, eq) in VARIANTS.items():
        g, steps = gcd_pi(a, b, SIZES[sz][0], eq)
        chk(name + ' = Euclid', g == r); hist[name][steps] = hist[name].get(steps, 0) + 1

if K <= 5:
    vals = range(-HI, HI + 1); elems = [(x, y) for x in vals for y in vals]
    for a in elems:
        for b in elems: test(a, b)
    print(f"K={K}: exhaustive, {len(elems)**2} pairs")
else:
    random.seed(9); r = lambda lim: random.randint(-lim, lim)          # the same pairs as study 9
    for i in range(100000):
        kind = i % 4
        if kind == 0: a, b = (r(HI), r(HI)), (r(HI), r(HI))
        elif kind == 1: a, b = (r(HI), r(HI)), (r(1 << 12), r(1 << 12))
        else:
            e = random.randint(4, 15); g = (r(1 << e), r(1 << e)); lim = HI // (2 << e)
            u, v = (r(lim), r(lim)), (r(lim), r(lim))
            a = (g[0]*u[0] - g[1]*u[1], g[0]*u[1] + g[1]*u[0]); b = (g[0]*v[0] - g[1]*v[1], g[0]*v[1] + g[1]*v[0])
        test(a, b)
    for a, b in [((HI, HI), (HI, HI)), ((HI, -HI), (1, 1)), ((-HI, 0), (0, HI)), ((HI, HI), (0, 0)), ((0, 0), (0, 0)),
                 ((HI, 1), (HI - 1, 1)), ((1 << 30, 1 << 30), (1 << 29, 0)), ((HI, 0), (1, 0))]:
        test(a, b)
    print(f"K={K}: 100008 random pairs")
for k, v in sorted(st.items()): print(f"  {k}: {v}")
mean = lambda h: sum(k*v for k, v in h.items()) / sum(h.values())
print(f"  {'Euclid (study 6)':34s} steps mean {mean(ehist):6.2f} max {max(ehist):3d}   per step 50, 12 MUL, 2 DIV")
for name, (sz, eq) in VARIANTS.items():
    per = 49 + SIZES[sz][1] + (4 if eq else 2)
    print(f"  {name:34s} steps mean {mean(hist[name]):6.2f} max {max(hist[name]):3d}   per step {per}, {SIZES[sz][2]} MUL, 0 DIV"
          f"   ~ {per * mean(hist[name]):7.1f} in steps")
