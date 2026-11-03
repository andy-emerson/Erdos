# Study 14: the extended (1+i)-ary gcd, against an extended Euclid (study 6 with coefficients).
# pi = 1 + i. p(z) = (re + im) mod 2 is z mod pi (Z[i]/pi = F_2). z / pi = ((x + y) + (y - x) i) / 2 when p(z) = 0.
# Inputs A, B. Strip the common pi^k exactly: A = pi^k A', B = pi^k B', so one of A', B' is prime to pi.
# Keep a = ua A' + va B',  b = ub A' + vb B'. Dividing a by pi: add m (B', -A') to (ua, va) first (the sum ua A' + va B'
# is unchanged), with m in {0, 1} chosen so both coefficients become divisible by pi:
#   m = p(ua) if B' is prime to pi, else m = p(va)        (since p(a) = p(ua) p(A') + p(va) p(B') = 0).
# Making a primary multiplies a by a unit; the coefficients are multiplied by the same unit.
# Loop version (study 11's best: L1 size |x| + |y|, stop when equal):
#   strip a, make a and b primary, order by size; while a != b: a -= b (coefficients too), divide out pi while p(a) = 0,
#   make a primary, swap if size(a) < size(b). Then g = pi^k a, Bezout (ua, va); normalise g, coefficients by the same unit.
# Branch-free version: a fixed number T of micro-steps, each selecting by a mask between
#   even: a <- a / pi (coefficient fix)       odd: a <- primary(a); swap if size(a) < size(b); a <- a - b
# Once a = 0, every micro-step is "even" on 0: a stays 0 and b, ub, vb never change, so no freeze mask is needed.
# T from the bound: per odd micro-step N(a)N(b) grows by at most (1 + sqrt 2)^2 (L1 ordering gives N(b) <= 2 N(a)), and
# the next >= 3 even micro-steps divide it by 8: a factor <= 0.729 per 4 micro-steps, from N(A')N(B') < 2^(4K-2).
# Usage: python3 xgcd1i.py K   (K <= 5 exhaustive (branch-free exhaustive for K <= 4, sampled at K = 5); K = 32 random)
import sys, random, math
from fractions import Fraction
K = int(sys.argv[1]) if len(sys.argv) > 1 else 4
HI = (1 << (K-1)) - 1

def add(p, q): return (p[0] + q[0], p[1] + q[1])
def sub(p, q): return (p[0] - q[0], p[1] - q[1])
def gmul(p, q): return (p[0]*q[0] - p[1]*q[1], p[0]*q[1] + p[1]*q[0])
def par(z): return (z[0] + z[1]) & 1
def pidiv(z): return ((z[0] + z[1]) >> 1, (z[1] - z[0]) >> 1)
def scale(z, m): return (z[0] & -m, z[1] & -m)
def l1(z): return abs(z[0]) + abs(z[1])
def primary_unit(z):                 # the unit e (as a function) with e z primary; z prime to pi
    x, y = z; m1 = (x & 1) == 0                       # x even: times -i
    if m1: x, y = y, -x
    m2 = ((x - y) >> 1) & 1 == 1                      # x - y = 3 (mod 4): negate
    return lambda w: (lambda w1: (-w1[0], -w1[1]) if m2 else w1)((w[1], -w[0]) if m1 else w)
def normalise_unit(z):               # unit e with e z: re > 0, im >= 0 (z = 0: identity)
    def f(e): return lambda w: gmul(e, w)
    for e in ((1, 0), (0, -1), (-1, 0), (0, 1)):
        w = gmul(e, z)
        if z == (0, 0) or (w[0] > 0 and w[1] >= 0): return f(e)

def setup(A, B):                     # strip the common pi^k exactly (prologue, once per gcd)
    k = 0
    while par(A) == 0 and par(B) == 0: A, B, k = pidiv(A), pidiv(B), k + 1
    return A, B, k
def finish(g, u, v, k):              # g = pi^k g', then normalise g and the coefficients by one unit
    for _ in range(k): g = (g[0] - g[1], g[0] + g[1])
    e = normalise_unit(g); return e(g), e(u), e(v)

stats = {}; cmax = [0]
def note(k, v): stats.setdefault(k, []).append(v)
def track(*zs):
    for z in zs: cmax[0] = max(cmax[0], abs(z[0]), abs(z[1]))

def xgcd_loop(A0, B0):
    if A0 == (0, 0) or B0 == (0, 0):
        g, u, v = (B0, (0, 0), (1, 0)) if A0 == (0, 0) else (A0, (1, 0), (0, 0))
        e = normalise_unit(g); return e(g), e(u), e(v)
    Ap, Bp, k = setup(A0, B0); Bodd = par(Bp)
    a, ua, va, b, ub, vb = Ap, (1, 0), (0, 0), Bp, (0, 0), (1, 0)
    if not Bodd: a, ua, va, b, ub, vb = b, ub, vb, a, ua, va          # b prime to pi
    def divpi(a, u, v):
        m = par(u) if Bodd else par(v)
        return pidiv(a), pidiv(add(u, scale(Bp, m))), pidiv(sub(v, scale(Ap, m)))
    e = primary_unit(b); b, ub, vb = e(b), e(ub), e(vb)
    divs = 0
    while par(a) == 0: a, ua, va = divpi(a, ua, va); divs += 1
    e = primary_unit(a); a, ua, va = e(a), e(ua), e(va)
    if l1(a) < l1(b): a, ua, va, b, ub, vb = b, ub, vb, a, ua, va
    steps = 0
    while a != b:
        a, ua, va = sub(a, b), sub(ua, ub), sub(va, vb); steps += 1
        while par(a) == 0: a, ua, va = divpi(a, ua, va); divs += 1
        e = primary_unit(a); a, ua, va = e(a), e(ua), e(va)
        if l1(a) < l1(b): a, ua, va, b, ub, vb = b, ub, vb, a, ua, va
        track(ua, va, ub, vb)
    note('loop: outer steps', steps); note('loop: divisions by 1+i', divs)
    return finish(a, ua, va, k)

def T_bound(K):                      # micro-steps from the bound (L1 ordering), plus the setup's own halvings of a
    return 4 * math.ceil((4 * K - 2) / math.log2(8 / (1 + math.sqrt(2)) ** 2)) + 4 + 2 * K
def xgcd_branchfree(A0, B0, T):
    if A0 == (0, 0) or B0 == (0, 0): return xgcd_loop(A0, B0)          # (prologue case, masks not modelled)
    Ap, Bp, k = setup(A0, B0); Bodd = par(Bp)
    a, ua, va, b, ub, vb = Ap, (1, 0), (0, 0), Bp, (0, 0), (1, 0)
    if not Bodd: a, ua, va, b, ub, vb = b, ub, vb, a, ua, va
    e = primary_unit(b); b, ub, vb = e(b), e(ub), e(vb); sb = l1(b)
    first_zero = None
    for t in range(T):
        # even branch (computed always)
        m = par(ua) if Bodd else par(va)
        ev = (pidiv(a), pidiv(add(ua, scale(Bp, m))), pidiv(sub(va, scale(Ap, m))))
        # odd branch (computed always; primary of an even a is garbage and is discarded by the select)
        if par(a):
            e = primary_unit(a); a1, ua1, va1 = e(a), e(ua), e(va); s1 = l1(a1)
            if s1 < sb: a1, ua1, va1, b1, ub1, vb1, sb1 = b, ub, vb, a1, ua1, va1, s1
            else: b1, ub1, vb1, sb1 = b, ub, vb, sb
            od = (sub(a1, b1), sub(ua1, ub1), sub(va1, vb1), b1, ub1, vb1, sb1)
            a, ua, va, b, ub, vb, sb = od                                  # select by the mask p(a)
        else:
            a, ua, va = ev
        track(ua, va, ub, vb)
        if a == (0, 0) and first_zero is None: first_zero = t + 1
    assert a == (0, 0), "the fixed count was not enough"
    note('branch-free: micro-steps actually needed', first_zero)
    return finish(b, ub, vb, k)

def xeuclid(A0, B0):                 # study 6's Euclid (nearest rounding, halves up) with both coefficients
    r0, r1, s0, s1, t0, t1, steps = A0, B0, (1, 0), (0, 0), (0, 0), (1, 0), 0
    while r1 != (0, 0):
        n = r1[0]**2 + r1[1]**2; x = r0[0]*r1[0] + r0[1]*r1[1]; y = r0[1]*r1[0] - r0[0]*r1[1]
        q = ((2*x + n) // (2*n), (2*y + n) // (2*n))
        r0, r1 = r1, sub(r0, gmul(q, r1)); s0, s1 = s1, sub(s0, gmul(q, s1)); t0, t1 = t1, sub(t0, gmul(q, t1)); steps += 1
    note('extended Euclid: steps', steps)
    e = normalise_unit(r0); return e(r0), e(s0), e(t0)

def ref_gcd(a, b):                   # study 6's reference (rationals)
    rnd = lambda f: (f.numerator * 2 + f.denominator) // (2 * f.denominator)
    while b != (0, 0):
        n = b[0]**2 + b[1]**2; q = (rnd(Fraction(a[0]*b[0] + a[1]*b[1], n)), rnd(Fraction(a[1]*b[0] - a[0]*b[1], n)))
        a, b = b, sub(a, gmul(q, b))
    return normalise_unit(a)(a)

res = {}
def chk(k, ok):
    res[k + ' cases'] = res.get(k + ' cases', 0) + 1
    if not ok: res[k + ' FAIL'] = res.get(k + ' FAIL', 0) + 1
def test(A, B, do_bf, T):
    r = ref_gcd(A, B)
    algs = [('loop', xgcd_loop), ('extended Euclid', xeuclid)] + ([('branch-free', lambda p, q: xgcd_branchfree(p, q, T))] if do_bf else [])
    for name, f in algs:
        g, u, v = f(A, B)
        chk(f'{name}: g = study 6 gcd', g == r)
        chk(f'{name}: g = u A + v B', add(gmul(u, A), gmul(v, B)) == g)

T = T_bound(K)
if K <= 5:
    vals = range(-HI, HI + 1); elems = [(x, y) for x in vals for y in vals]
    random.seed(14)
    for A in elems:
        for B in elems: test(A, B, K <= 4 or random.random() < 0.02, T)
    print(f"K={K}: exhaustive, {len(elems)**2} pairs (branch-free: {'all' if K <= 4 else 'a 2% sample'}), T = {T}")
else:
    random.seed(14); r = lambda lim: random.randint(-lim, lim)
    for i in range(20000):
        kind = i % 4
        if kind == 0: A, B = (r(HI), r(HI)), (r(HI), r(HI))
        elif kind == 1: A, B = (r(HI), r(HI)), (r(1 << 12), r(1 << 12))
        else:
            e = random.randint(4, 15); g = (r(1 << e), r(1 << e)); lim = HI // (2 << e)
            A, B = gmul(g, (r(lim), r(lim))), gmul(g, (r(lim), r(lim)))
        test(A, B, i < 2000, T)
    for A, B in [((HI, HI), (HI, HI)), ((HI, -HI), (1, 1)), ((1 << 30, 1 << 30), (1 << 29, 0)), ((HI, 0), (1, 0)), ((0, 0), (5, 3))]:
        test(A, B, True, T)
    print(f"K={K}: 20005 random pairs (branch-free on the first 2005), T = {T}")
for k, v in sorted(res.items()): print(f"  {k}: {v}")
for k, v in stats.items(): print(f"  {k}: mean {sum(v) / len(v):.2f}, max {max(v)}")
print(f"  largest coefficient part seen (loop and branch-free): {cmax[0]}  = 2^{math.log2(max(cmax[0], 1)):.2f}; inputs below 2^{K-1}")
