# Study 9: a (1+i)-ary binary gcd for Gaussian integers, against study 6's Euclid.
# pi = 1 + i, N(pi) = 2, pi^2 = 2i, pi^3 = -2 + 2i.
#   pi | x + yi  exactly when x = y (mod 2);   (x + yi) / pi = ((x + y) + (y - x) i) / 2   (ADD, SUB, two shifts).
#   pi^v(z) for z = x + yi != 0:  e = ctz(x OR y);  v = 2e + 1 if x shr e and y shr e are both odd, else 2e.
#   Up to a unit, z / pi^v = (x sar e, y sar e), then one division by pi if both parts are odd.
# Every element prime to pi has exactly one associate that is "primary": = 1 mod pi^3, i.e. x odd, y even, x - y = 1 (mod 4).
# Two primary elements a, b have a - b = 0 (mod pi^3), so N((a - b)/pi^3) <= (|a| + |b|)^2 / 8 <= N(a)/2 when N(a) >= N(b).
# The algorithm (a variant in the spirit of Weilert 2000; not checked against his paper):
#   k = min(v(a), v(b)); strip pi from a and b; make both primary; order so N(a) >= N(b).
#   step: a <- primary(strip(a - b)); swap if N(a) < N(b).   Stop when b = 0.   gcd = a pi^k, normalised (re > 0, im >= 0).
# Once b = 0 the step leaves (a, b) unchanged, so a branch-free version just runs the step a fixed number of times.
# Values are kept as two extracted halves in 2K-bit registers (a - b can exceed a half's range); no multiply in the step
# except the norm (2 MUL); no divide.
# Usage: python3 gcd1i.py K   (K <= 5 exhaustive over all pairs; K = 32 random)
import sys, random
from fractions import Fraction
K = int(sys.argv[1]) if len(sys.argv) > 1 else 4
W = 2*K; MW = (1 << W) - 1
HI = (1 << (K-1)) - 1

def ctz(v):                       # count trailing zeros of a 2K-bit word (x86 tzcnt, ARM64 rbit + clz, RISC-V Zbb ctz); 0 -> 2K
    v &= MW; return W if v == 0 else (v & -v).bit_length() - 1
def shr_count(c): return c % W    # the machines take a shift count mod 2K, so a count of 2K shifts by 0

def strip(x, y):                  # z / pi^v(z) up to a unit, z != 0 (z = 0 stays 0).  16 instructions
    e = shr_count(ctz(x | y))                               # OR, CTZ (2)
    x >>= e; y >>= e                                        # SAR, SAR (2)
    m = -(x & y & 1)                                        # AND, AND, NEG (3): both odd
    px, py = (x + y) >> 1, (y - x) >> 1                     # ADD, SUB, SAR, SAR (4)
    return x ^ ((x ^ px) & m), y ^ ((y ^ py) & m)           # 2 x conditional choice (6)
def primary(x, y):                # the associate = 1 mod pi^3 of an element prime to pi (0 stays 0).  17 instructions
    m = (x & 1) - 1                                         # AND, SUB (2): all ones where x is even (then y is odd)
    nx, ny = x ^ ((x ^ y) & m), y ^ ((y ^ -x) & m)          # times -i: (x, y) -> (y, -x); choice 3, NEG + choice 4 (7)
    m = -(((nx - ny) >> 1) & 1)                             # SUB, SHR, AND, NEG (4): x - y = 3 (mod 4)
    return (nx ^ m) - m, (ny ^ m) - m                       # conditional negation 2 + 2 (4)
def val_pi(x, y):                 # v(z), z != 0: OR, CTZ, SAR, SAR, AND, AND, ADD of 2e (7)
    e = ctz(x | y); return 2*e + ((x >> e) & (y >> e) & 1)
def norm(x, y): return x*x + y*y                            # 2 MUL, ADD (3)
def cswap(p, q, m):               # conditional swap of one word pair: XOR, AND, XOR, XOR (4)
    d = (p ^ q) & m; return p ^ d, q ^ d

def gcd_pi(a, b, count=None):
    (a1, a2), (b1, b2) = a, b
    if (a1, a2) == (0, 0) or (b1, b2) == (0, 0):            # gcd(0, b) = b: prologue case, outside the step loop
        g = (b1, b2) if (a1, a2) == (0, 0) else (a1, a2); return normalise(*g), 0
    k = min(val_pi(a1, a2), val_pi(b1, b2))                 # 2 x 7 + min 3
    a1, a2 = primary(*strip(a1, a2)); b1, b2 = primary(*strip(b1, b2))
    na, nb = norm(a1, a2), norm(b1, b2)
    m = -int(na < nb); a1, b1 = cswap(a1, b1, m); a2, b2 = cswap(a2, b2, m); na, nb = cswap(na, nb, m)
    steps = 0
    while (b1, b2) != (0, 0) and (count is None or steps < count):
        a1, a2 = primary(*strip(a1 - b1, a2 - b2))          # SUB, SUB (2) + 16 + 17
        na = norm(a1, a2)                                   # 3
        m = -int(na < nb)                                   # compare, NEG (2)
        a1, b1 = cswap(a1, b1, m); a2, b2 = cswap(a2, b2, m); na, nb = cswap(na, nb, m)   # 12
        steps += 1                                          # step: 2 + 16 + 17 + 3 + 2 + 12 = 52 (+ loop test OR, compare: 2)
        assert max(abs(a1), abs(a2), abs(b1), abs(b2)) < (1 << K), "a value left 2K-bit headroom"
    # g = a pi^k up to a unit: pi^k = 2^(k div 2) times a unit, times pi when k is odd: (x - y, x + y)
    g1, g2 = a1 << (k >> 1), a2 << (k >> 1)
    if k & 1: g1, g2 = g1 - g2, g1 + g2                      # branch-free: mask from k AND 1 and two choices
    return normalise(g1, g2), steps
def normalise(x, y):              # study 6: the associate with re > 0, im >= 0
    for _ in range(4):
        if (x > 0 and y >= 0) or (x, y) == (0, 0): return (x, y)
        x, y = -y, x

# ---- reference: study 6's Euclid (rounded division by rationals, as its reference, which its word code matched)
def ref_round(f): return (f.numerator * 2 + f.denominator) // (2 * f.denominator)
def euclid(a, b):
    steps = 0
    while b != (0, 0):
        n = b[0]**2 + b[1]**2
        q = (ref_round(Fraction(a[0]*b[0] + a[1]*b[1], n)), ref_round(Fraction(a[1]*b[0] - a[0]*b[1], n)))
        a, b = b, (a[0] - (q[0]*b[0] - q[1]*b[1]), a[1] - (q[0]*b[1] + q[1]*b[0])); steps += 1
    return normalise(*a), steps

st = {}; hist = {}; ehist = {}
def chk(k, ok):
    st[k + ' cases'] = st.get(k + ' cases', 0) + 1
    if not ok: st[k + ' FAIL'] = st.get(k + ' FAIL', 0) + 1
def test(a, b):
    g, steps = gcd_pi(a, b); r, es = euclid(a, b)
    chk('(1+i)-ary gcd = Euclid gcd (normalised)', g == r)
    hist[steps] = hist.get(steps, 0) + 1; ehist[es] = ehist.get(es, 0) + 1
    return steps

if K <= 5:
    vals = range(-HI, HI + 1)
    elems = [(x, y) for x in vals for y in vals]
    for a in elems:
        for b in elems: test(a, b)
    print(f"K={K}: exhaustive, {len(elems)**2} pairs")
else:
    random.seed(9); r = lambda lim: random.randint(-lim, lim)
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
print(f"  (1+i)-ary steps: max {max(hist)}  mean {mean(hist):.2f}")
print(f"  Euclid steps:    max {max(ehist)}  mean {mean(ehist):.2f}")
