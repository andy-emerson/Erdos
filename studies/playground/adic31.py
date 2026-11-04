# Study 31: the (1+i)-adic representation x = (1+i)^k u, u odd, and how to make addition cheap.
# u stored (a) in unit digits: words A, B; bit pair (A_j, B_j) picks u_j: 00 -> 1, 01 -> i, 10 -> -1, 11 -> -i,
#   u = sum u_j 2^j, n positions; covers exactly the odd values with |re| + |im| <= 2^n - 1.
#   or (b) as an ordinary pair (a, b), a + b odd.
# Zero: k = ZK (reserved code).
# Usage: python3 adic31.py [conv|pat|carry|alg|all]
import sys, random
from collections import Counter, defaultdict

ZK = -1
UNITS = [(1, 0), (0, 1), (-1, 0), (0, -1)]            # code 0..3 = 1, i, -1, -i

def gmul(p, q): return (p[0]*q[0] - p[1]*q[1], p[0]*q[1] + p[1]*q[0])
def gadd(p, q): return (p[0] + q[0], p[1] + q[1])
def odd(z): return (z[0] + z[1]) & 1
def pidiv(z): return ((z[0] + z[1]) >> 1, (z[1] - z[0]) >> 1)    # z / (1+i), z even
def pimul(z): return (z[0] - z[1], z[0] + z[1])                   # z (1+i)
def l1(z): return abs(z[0]) + abs(z[1])
def ipow(z, e):                                                  # z i^e
    for _ in range(e & 3): z = (-z[1], z[0])
    return z
def pipow(z, d):                                                 # z (1+i)^d
    for _ in range(d): z = pimul(z)
    return z

# ---------- converters ----------
def to_k(z):                       # ordinary -> (k, u) by repeated division (reference)
    if z == (0, 0): return ZK, (0, 0)
    k = 0
    while not odd(z): z = pidiv(z); k += 1
    return k, z
def from_k(k, u):
    return (0, 0) if k == ZK else pipow(u, k)

def ud_value(A, B, n):             # reference: sum of digits
    re = im = 0
    for j in range(n):
        c = ((A >> j) & 1) * 2 + ((B >> j) & 1)
        re += UNITS[c][0] << j; im += UNITS[c][1] << j
    return re, im
def ud_decode(A, B, n):            # word-level: 4 ops (xor, sub, sub, sub)
    M = (1 << n) - 1; X = A ^ B
    return M - X - A, X - A
def ud_encode(u, n):               # word-level: 8 ops; u odd with l1(u) <= 2^n - 1
    M = (1 << n) - 1
    R, S = u[0] - u[1], u[0] + u[1]                              # (1+i) u
    Rp, Sp = (R + M) >> 1, (S + M) >> 1
    return M ^ Sp, Rp ^ Sp                                       # A = NOT S', B = R' XOR S'
def ud_need(u):                    # smallest n with l1(u) <= 2^n - 1
    n = 1
    while l1(u) > (1 << n) - 1: n += 1
    return n

def check_conv():
    print("converters")
    for n in range(1, 9):
        M = (1 << n) - 1; seen = set()
        for A in range(1 << n):
            for B in range(1 << n):
                v = ud_value(A, B, n)
                assert v == ud_decode(A, B, n) and ud_encode(v, n) == (A, B)
                assert odd(v) and l1(v) <= M
                seen.add(v)
        diamond = {(a, b) for a in range(-M, M+1) for b in range(-M, M+1) if l1((a, b)) <= M and (a+b) & 1}
        assert seen == diamond and len(seen) == 4**n
    print("  unit digits n=1..8: 4^n codes distinct, = odd diamond, decode/encode exact")
    R = 40
    for a in range(-R, R+1):
        for b in range(-R, R+1):
            k, u = to_k((a, b))
            assert from_k(k, u) == (a, b)
            if k != ZK:
                assert odd(u) and k == fast_k((a, b))
                n = ud_need(u); assert ud_decode(*ud_encode(u, n), n) == u
    print("  ordinary <-> (k, u) <-> unit digits: box |a|,|b| <= %d exact; ctz formula for k exact" % R)
    for _ in range(20000):
        z = (random.randrange(-2**40, 2**40), random.randrange(-2**40, 2**40))
        if z == (0, 0): continue
        k, u = to_k(z); assert k == fast_k(z) and from_k(k, u) == z
        assert ud_decode(*ud_encode(u, 45), 45) == u
    print("  random 41-bit: exact")

def ctz(x): return (x & -x).bit_length() - 1
def fast_k(z):                     # v_{1+i}(a + bi) = 2 ctz(a|b) + bit m of (a & b)
    a, b = z; m = ctz(a | b)
    return 2*m + (((a & b) >> m) & 1)

# ---------- patterns ----------
def low_digits(u, j):              # code of digit j of u in a wide enough encoding
    n = max(ud_need(u), j + 1) + 1
    A, B = ud_encode(u, n)
    return ((A >> j) & 1) * 2 + ((B >> j) & 1)

def patterns():
    print("patterns")
    R = 31
    box = [(a, b) for a in range(-R, R+1) for b in range(-R, R+1) if (a, b) != (0, 0)]
    random.seed(1)
    pairs = [(random.choice(box), random.choice(box)) for _ in range(400000)]
    # 1. result k against input k
    rel = Counter(); tdist = Counter(); tdist_d = defaultdict(Counter)
    for x, y in pairs:
        s = gadd(x, y); k1, u1 = to_k(x); k2, u2 = to_k(y); ks, us = to_k(s)
        d = abs(k1 - k2)
        if d: rel[('d>0', ks == min(k1, k2))] += 1
        elif ks == ZK: rel[('d=0', 'zero')] += 1
        else:
            t = ks - k1; tdist[t] += 1; rel[('d=0', 't>=1')] += 1
    print("  k relation:", dict(rel))
    tot = sum(tdist.values())
    print("  stripped t (d = 0, nonzero):", " ".join("%d:%.4f" % (t, tdist[t]/tot) for t in sorted(tdist)[:10]))
    kin = Counter(to_k(x)[0] for x, _ in pairs); tot = sum(kin.values())
    print("  input k distribution (box):", " ".join("%d:%.3f" % (k, kin[k]/tot) for k in sorted(kin)[:8]))
    # 2. t predicted by low digits: v(u1+u2) >= m is a function of digits 0..ceil(m/2)-1
    print("  t from low digits (all odd u in diamond n=6):")
    n = 6; M = (1 << n) - 1
    odds = [ud_value(A, B, n) for A in range(1 << n) for B in range(1 << n)]
    for m in range(2, 9):
        nd = (m + 1) // 2
        table = {}; ok = True
        for u1 in odds[::7]:
            for u2 in odds[::5]:
                s = gadd(u1, u2)
                if s == (0, 0): continue
                key = tuple(low_digits(u1, j) for j in range(nd)) + tuple(low_digits(u2, j) for j in range(nd))
                val = to_k(s)[0] >= m
                if table.setdefault(key, val) != val: ok = False
        print("    t >= %d determined by %d low digit(s) of each: %s" % (m, nd, ok))
    # bit rule for t = 1 and t = 2
    bad = Counter()
    for u1 in odds:
        for u2 in odds[::3]:
            s = gadd(u1, u2)
            if s == (0, 0): continue
            t = to_k(s)[0]
            A1, B1 = ud_encode(u1, n); A2, B2 = ud_encode(u2, n)
            if (t == 1) != (((B1 ^ B2) & 1) == 1): bad['t=1 rule'] += 1
            if t >= 2:   # digit0 equal up to sign; t = 2 iff ... look at digit 1
                same0 = (A1 & 1) == (A2 & 1)    # u2_0 == u1_0 (else u2_0 = -u1_0)
                # if same: s = 2(u_0 + ...) ; if opposite: s = 2(u1_1 + u2_1) + ...
                pass
    print("  rule t = 1 iff B1_0 xor B2_0 = 1: violations", bad['t=1 rule'])
    # full closed form: t = 1 + 2 ctz(P|Q) + bit, P = M - (X1 + X2), Q = M - (A1 + A2), X = A ^ B
    viol = 0
    for u1 in odds:
        for u2 in odds[::3]:
            s = gadd(u1, u2)
            if s == (0, 0): continue
            A1, B1 = ud_encode(u1, n); A2, B2 = ud_encode(u2, n)
            P = M - ((A1 ^ B1) + (A2 ^ B2)); Q = M - (A1 + A2)
            if gmul((0, -1), pimul((P, Q))) != s: viol += 1
            if to_k(s)[0] != 1 + fast_k((P, Q)): viol += 1
    print("  u1 + u2 = -i (1+i) (P + iQ), t = 1 + v(P + iQ): violations", viol)
    # 3. t distribution conditioned on low digit codes
    cond = defaultdict(Counter)
    for u1 in odds:
        for u2 in odds[::3]:
            s = gadd(u1, u2)
            if s == (0, 0): continue
            c1, c2 = low_digits(u1, 0), low_digits(u2, 0)
            rel0 = (c2 - c1) & 3          # u2_0 = u1_0 i^rel0
            cond[rel0][min(to_k(s)[0], 6)] += 1
    for r in range(4):
        tot = sum(cond[r].values())
        print("    u2_0 = u1_0 * i^%d: t dist" % r, " ".join("%d:%.3f" % (t, cond[r][t]/tot) for t in sorted(cond[r])))
    # 4. units and small numbers
    print("  adding a unit e to odd u (k = 0): t dist")
    tu = Counter()
    for u in odds:
        for e in UNITS:
            s = gadd(u, e)
            if s != (0, 0): tu[min(to_k(s)[0], 6)] += 1
    tot = sum(tu.values()); print("   ", " ".join("%d:%.3f" % (t, tu[t]/tot) for t in sorted(tu)))
    # 5. large k gap: low digits of the result equal u1's below position m - 1 (d = 2m), digit m-1 negated
    print("  gap d = 2m (even): result digits vs u1 digits")
    for m in (1, 2, 3, 4):
        okc = 0; tot = 0
        for u1 in odds[::11]:
            for u2 in odds[::13]:
                w = gadd(u1, pipow(u2, 2*m)); nn = ud_need(w) + 1
                Aw, Bw = ud_encode(w, nn); A1, B1 = ud_encode(u1, nn)
                mask = (1 << (m - 1)) - 1
                c_ok = (Aw & mask) == (A1 & mask) and (Bw & mask) == (B1 & mask)
                dm1 = ((Aw >> (m-1)) & 1) * 2 + ((Bw >> (m-1)) & 1); u1m1 = ((A1 >> (m-1)) & 1) * 2 + ((B1 >> (m-1)) & 1)
                okc += c_ok and dm1 == (u1m1 + 2) & 3; tot += 1
        print("    m=%d: digits < m-1 equal and digit m-1 negated in %d / %d" % (m, okc, tot))
    print("  gap d = 2m+1 (odd): digits < m equal u1's?")
    for m in (0, 1, 2, 3):
        okc = 0; tot = 0
        for u1 in odds[::11]:
            for u2 in odds[::13]:
                w = gadd(u1, pipow(u2, 2*m+1)); nn = ud_need(w) + 1
                Aw, Bw = ud_encode(w, nn); A1, B1 = ud_encode(u1, nn)
                mask = (1 << m) - 1
                okc += (Aw & mask) == (A1 & mask) and (Bw & mask) == (B1 & mask); tot += 1
        print("    m=%d: %d / %d" % (m, okc, tot))
    # 6. growth: digits needed by the result against inputs
    print("  digits needed: result minus max(input) (d = 0, after strip), box pairs")
    g = Counter()
    for x, y in pairs[:100000]:
        k1, u1 = to_k(x); k2, u2 = to_k(y); s = gadd(x, y)
        if k1 != k2 or s == (0, 0): continue
        g[ud_need(to_k(s)[1]) - max(ud_need(u1), ud_need(u2))] += 1
    tot = sum(g.values()); print("   ", " ".join("%+d:%.3f" % (t, g[t]/tot) for t in sorted(g)))
    # 7. subtraction
    sd = Counter()
    for x, y in pairs[:100000]:
        k1 = to_k(x)[0]; k2 = to_k(y)[0]; s = (x[0]-y[0], x[1]-y[1])
        if k1 == k2 and s != (0, 0): sd[min(to_k(s)[0] - k1, 6)] += 1
    tot = sum(sd.values()); print("  subtraction, d = 0: t dist", " ".join("%d:%.3f" % (t, sd[t]/tot) for t in sorted(sd)))

if __name__ == "__main__":
    what = sys.argv[1] if len(sys.argv) > 1 else "all"
    random.seed(31)
    if what in ("conv", "all"): check_conv()
    if what in ("pat", "all"): patterns()
