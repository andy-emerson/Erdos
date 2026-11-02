# Study 6: exact division, division with remainder, and the Gaussian gcd by Euclid.
# Halves are signed K-bit lanes in the symmetric range -(2^(K-1)-1) .. 2^(K-1)-1, in a word of 2K bits (real part low).
# The arithmetic runs on extracted halves in 2K-bit signed words, as a machine would with 64-bit registers at K = 32:
#   a / b = a conj(b) / N(b):   x = a1 b1 + a2 b2,  y = a2 b1 - a1 b2,  N = b1^2 + b2^2   (|x|, |y|, N < 2^(2K-1))
# Division uses the machine's signed truncating divide of 2K-bit words (x86-64 idiv, ARM64 sdiv, RISC-V div);
# rounding is then fixed by masks. Exact division: q = (x / N, y / N). Division with remainder:
# q = nearest Gaussian integer to a / b (each part rounded to nearest, half up), r = a - q b, N(r) <= N(b)/2 < N(b).
# gcd: Euclid, (a, b) <- (b, a mod b) until b = 0, then the associate with real part > 0 and imaginary part >= 0.
# Division by 0: x86-64 traps on a zero divisor, so N is replaced by 1 when it is 0 and the quotient becomes the
# infinity marker (pack(MIN, MIN)); a / 0 = infinity, and 0 / 0 (undefined) also gives infinity.
# Usage: python3 gcd.py K   (K <= 5: exhaustive over all pairs; else random pairs)
import sys, random
from fractions import Fraction
K = int(sys.argv[1]) if len(sys.argv) > 1 else 4
W = 2*K; MW = (1 << W) - 1; ML = (1 << K) - 1
MIN = -(1 << (K-1)); HI = (1 << (K-1)) - 1
def sg(v, k): v &= (1 << k) - 1; return v - (1 << k) if v >> (k-1) else v
def pack(a, b): return ((b & ML) << K) | (a & ML)
def unpack(w): return sg(w, K), sg(w >> K, K)
INF = pack(MIN, MIN)
sar = lambda v: -1 if v < 0 else 0                         # v sar (2K-1): the sign's mask (values here are 2K-bit signed)
def tdiv(x, n): q = abs(x) // n; return q if x >= 0 else -q   # the machine's truncating divide (n > 0)

def round_div(x, n):            # nearest integer to x / n, n > 0, halves rounded up. 10 instructions, 1 divide, 1 multiply
    q = tdiv(x, n)              # DIV
    r = x - q * n               # MUL, SUB          r in (-n, n), same sign as x
    m = sar(r)                  # SAR
    q += m; r += n & m          # ADD; AND, ADD     floor: r in [0, n)
    k = int(r >= n - r)         # SUB, compare      2r >= n, without forming 2r (no overflow)
    return q + k                # ADD
def divmod_g(a, b):             # a, b packed words, b != 0.  Returns (q, r) packed.
    a1, a2 = unpack(a); b1, b2 = unpack(b)                  # extract 6
    n = b1*b1 + b2*b2                                       # 2 MUL, ADD
    x = a1*b1 + a2*b2; y = a2*b1 - a1*b2                    # 4 MUL, 2 add/sub
    q1, q2 = round_div(x, n), round_div(y, n)               # 2 x 10
    r1 = a1 - (q1*b1 - q2*b2); r2 = a2 - (q1*b2 + q2*b1)    # 4 MUL, 2 add/sub, 2 SUB
    return pack(q1, q2), pack(r1, r2)                       # pack 3 + 3
# divmod_g: 6 + 3 + 6 + 20 + 8 + 6 = 49 instructions; 12 multiplies, 2 divides.
def div_exact(a, b):            # b divides a, b may be 0 (then infinity)
    a1, a2 = unpack(a); b1, b2 = unpack(b)                  # 6
    n = b1*b1 + b2*b2                                       # 3
    z = int(n == 0); n |= z                                 # compare, OR: a zero divisor becomes 1 (no trap)
    x = a1*b1 + a2*b2; y = a2*b1 - a1*b2                    # 6
    q = pack(tdiv(x, n), tdiv(y, n))                        # 2 DIV, pack 3
    m = (0 - z) & MW
    return q ^ ((q ^ INF) & m)                              # mask 1, choice 3
# div_exact: 6 + 3 + 2 + 6 + 2 + 3 + 4 = 26 instructions; 6 multiplies, 2 divides.
def normalise(g):               # the associate with re > 0, im >= 0 (0 stays 0). 22 instructions.
    x, y = unpack(g)                                        # extract 3
    m = sar(y * (1 << K) + x)   # SHL, ADD, SAR (3): negative exactly when y < 0, or y = 0 and x < 0 (|x| < 2^K)
    x, y = (x ^ m) - m, (y ^ m) - m                         # conditional negation, 2 each (4): now y > 0, or y = 0 and x >= 0
    k = int(x <= 0)                                         # compare (1): x <= 0 here means y > 0 (or g = 0)
    m = 0 - k                                               # mask 1
    x, y = (x ^ ((x ^ y) & m)), (y ^ ((y ^ -x) & m))        # times -i: (x, y) -> (y, -x); choice 3, NEG + choice 4
    return pack(x, y)                                       # pack 3.  total 3 + 3 + 4 + 1 + 1 + 3 + 4 + 3 = 22
def gcd_g(a, b):
    steps = 0
    while unpack(b) != (0, 0):                              # data-dependent loop (the step count is bounded, see below)
        a, b = b, divmod_g(a, b)[1]; steps += 1
    return normalise(a), steps

# ---------------- reference (Python rationals and big integers, independent of the word code)
def ref_round(f): return (f.numerator * 2 + f.denominator) // (2 * f.denominator)   # nearest, halves up
def ref_divmod(a, b):
    (a1, a2), (b1, b2) = a, b; n = b1*b1 + b2*b2
    q = (ref_round(Fraction(a1*b1 + a2*b2, n)), ref_round(Fraction(a2*b1 - a1*b2, n)))
    return q, (a1 - (q[0]*b1 - q[1]*b2), a2 - (q[0]*b2 + q[1]*b1))
def ref_xgcd(a, b):             # extended Euclid: g = u a + v b
    r0, r1, u0, u1, v0, v1 = a, b, (1, 0), (0, 0), (0, 0), (1, 0)
    mul = lambda p, q: (p[0]*q[0] - p[1]*q[1], p[0]*q[1] + p[1]*q[0])
    subt = lambda p, q: (p[0] - q[0], p[1] - q[1])
    while r1 != (0, 0):
        q, r = ref_divmod(r0, r1)
        r0, r1 = r1, r; u0, u1 = u1, subt(u0, mul(q, u1)); v0, v1 = v1, subt(v0, mul(q, v1))
    return r0, u0, v0
def ref_norm_assoc(g):
    for u in ((1, 0), (0, 1), (-1, 0), (0, -1)):
        h = (g[0]*u[0] - g[1]*u[1], g[0]*u[1] + g[1]*u[0])
        if h == (0, 0) or (h[0] > 0 and h[1] >= 0): return h
def divides(d, a):
    n = d[0]**2 + d[1]**2
    if n == 0: return a == (0, 0)
    return (a[0]*d[0] + a[1]*d[1]) % n == 0 and (a[1]*d[0] - a[0]*d[1]) % n == 0

st = {}; steps_hist = {}
def chk(k, ok):
    st[k + ' cases'] = st.get(k + ' cases', 0) + 1
    if not ok: st[k + ' FAIL'] = st.get(k + ' FAIL', 0) + 1
def test(a, b, brute=None):
    A, B = pack(*a), pack(*b); Na, Nb = a[0]**2 + a[1]**2, b[0]**2 + b[1]**2
    if b != (0, 0):
        Q, R = divmod_g(A, B); q, r = ref_divmod(a, b)
        chk('divmod q, r match reference', (unpack(Q), unpack(R)) == (q, r))
        chk('divmod a = q b + r', (q[0]*b[0] - q[1]*b[1] + r[0], q[0]*b[1] + q[1]*b[0] + r[1]) == a)
        chk('divmod N(r) <= N(b)/2', 2 * (r[0]**2 + r[1]**2) <= Nb)
        chk('divmod q, r in range', all(-HI <= v <= HI for v in q + r))
        # exact division: on the product c = a b when it is in range, c / b must give a
        c = (a[0]*b[0] - a[1]*b[1], a[0]*b[1] + a[1]*b[0])
        if all(-HI <= v <= HI for v in c): chk('exact division (a b) / b = a', unpack(div_exact(pack(*c), B)) == a)
    else:
        chk('a / 0 gives infinity', div_exact(A, B) == INF)
    G, steps = gcd_g(A, B); g = unpack(G)
    steps_hist[steps] = steps_hist.get(steps, 0) + 1
    rg, u, v = ref_xgcd(a, b); rg = ref_norm_assoc(rg)
    chk('gcd equals reference (normalised)', g == rg)
    chk('gcd certificate: g | a, g | b, g = u a + v b', divides(rg, a) and divides(rg, b) and
        ref_norm_assoc((u[0]*a[0] - u[1]*a[1] + v[0]*b[0] - v[1]*b[1], u[0]*a[1] + u[1]*a[0] + v[0]*b[1] + v[1]*b[0])) == rg)
    if brute is not None: chk('gcd norm = largest norm of a common divisor (brute force)', g[0]**2 + g[1]**2 == brute(a, b))

vals = range(-HI, HI + 1)
if K <= 5:
    elems = [(x, y) for x in vals for y in vals]
    R = 2 * HI                                            # every divisor d of a nonzero a has |d| <= |a| <= sqrt(2) HI
    cands = [(x, y) for x in range(-R, R + 1) for y in range(-R, R + 1) if (x, y) != (0, 0)]
    divs = {e: [d for d in cands if divides(d, e)] for e in elems if e != (0, 0)} if K <= 4 else None
    def brute(a, b):
        if a == (0, 0) and b == (0, 0): return 0
        if a == (0, 0): return b[0]**2 + b[1]**2
        if b == (0, 0): return a[0]**2 + a[1]**2
        return max(d[0]**2 + d[1]**2 for d in divs[a] if divides(d, b))
    for a in elems:
        for b in elems: test(a, b, brute if K <= 4 else None)
    print(f"K={K}: exhaustive, {len(elems)**2} pairs" + (" (with brute-force gcd)" if K <= 4 else ""))
else:
    random.seed(6); r = lambda lim: random.randint(-lim, lim)
    for i in range(100000):
        kind = i % 4
        if kind == 0: a, b = (r(HI), r(HI)), (r(HI), r(HI))                         # random
        elif kind == 1: a, b = (r(HI), r(HI)), (r(1 << 12), r(1 << 12))             # small divisor
        else:                                                                     # a common factor g of size 2^(4..15)
            e = random.randint(4, 15); g = (r(1 << e), r(1 << e)); lim = HI // (2 << e)
            u, v = (r(lim), r(lim)), (r(lim), r(lim))
            a = (g[0]*u[0] - g[1]*u[1], g[0]*u[1] + g[1]*u[0]); b = (g[0]*v[0] - g[1]*v[1], g[0]*v[1] + g[1]*v[0])
        test(a, b)
    for a, b in [((HI, HI), (HI, HI)), ((HI, -HI), (1, 1)), ((-HI, 0), (0, HI)), ((HI, HI), (0, 0)), ((0, 0), (0, 0)), ((HI, 1), (HI - 1, 1))]:
        test(a, b)
    for i in range(100000):                                    # exact division at K = 32: c = q b built in range
        e = random.randint(0, 30); b = (r(1 << e), r(1 << e))
        if b == (0, 0): b = (1, 0)
        lim = HI // (2 << e); q = (r(lim), r(lim)); c = (q[0]*b[0] - q[1]*b[1], q[0]*b[1] + q[1]*b[0])
        chk('exact division (q b) / b = q', unpack(div_exact(pack(*c), pack(*b))) == q)
    for i in range(1000): chk('a / 0 gives infinity', div_exact(pack(r(HI), r(HI)), 0) == INF)
    print(f"K={K}: 100006 random pairs for divmod and gcd, 100000 exact divisions, 1000 divisions by 0")
for k, v in sorted(st.items()): print(f"  {k}: {v}")
tot = sum(steps_hist.values())
print("  Euclid steps: max", max(steps_hist), " mean %.2f" % (sum(k*v for k, v in steps_hist.items()) / tot),
      " histogram", dict(sorted(steps_hist.items())))
