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

def carry_structure():
    print("digit-serial carries")
    def step(t, need_odd):
        for d in UNITS:
            r = (t[0]-d[0], t[1]-d[1])
            if r[0] % 2 == 0 and r[1] % 2 == 0:
                c = (r[0]//2, r[1]//2)
                if odd(c) == need_odd: return d, c
    maps = {}
    for a in range(4):
        for b in range(4):
            m = []
            for c in range(4):
                t = gadd(gadd(UNITS[a], UNITS[b]), UNITS[c]); d, cn = step(t, 1)
                assert cn in UNITS; m.append(UNITS.index(cn))
            maps[(a, b)] = tuple(m)
    kind = Counter()
    for (a, b), m in maps.items():
        r = (b - a) & 3
        kind[(r, len(set(m)))] += 1
    print("  even gap: carries stay units; carry map image size by u2_j = u1_j i^r:", dict(kind))
    # in rotated coordinates (1+i)c = (+-1) + (+-1)i: the map acts on the two sign bits independently
    def sb(c): z = pimul(UNITS[c]); return (z[0] < 0, z[1] < 0)
    indep = True
    for m in maps.values():
        for c1 in range(4):
            for c2 in range(4):
                s1, s2 = sb(c1), sb(c2); o1, o2 = sb(m[c1]), sb(m[c2])
                if s1[0] == s2[0] and o1[0] != o2[0]: indep = False
                if s1[1] == s2[1] and o1[1] != o2[1]: indep = False
    print("  real sign bit of (1+i)carry depends only on its own previous value, imag likewise:", indep)
    # verify generic digit-serial addition with one-position parity lookahead, even and odd gaps
    def serial(summ):           # summ: list of Gaussian digit sums s_j (s_0 odd); returns unit digit codes
        out = []; c = (0, 0); j = 0
        while True:
            s = summ[j] if j < len(summ) else (0, 0)
            t = gadd(s, c)
            if j >= len(summ) and c in UNITS: out.append(UNITS.index(c)); return out
            nxt = summ[j+1] if j + 1 < len(summ) else (0, 0)
            d, c = step(t, 1 - odd(nxt)); out.append(UNITS.index(d)); j += 1
    n = 4; cnt = 0; cset = set()
    odds = [(ud_value(A, B, n), A, B) for A in range(1 << n) for B in range(1 << n)]
    for u1, A1, B1 in odds:
        for u2, A2, B2 in odds:
            for dd in range(1, 6):
                h = dd // 2
                d1 = [UNITS[((A1 >> j) & 1)*2 + ((B1 >> j) & 1)] for j in range(n)]
                d2 = [UNITS[((A2 >> j) & 1)*2 + ((B2 >> j) & 1)] for j in range(n)]
                if dd & 1: d2 = [pimul(x) for x in d2]
                d2 = [ipow(x, h) for x in d2]
                L = max(n, n + h)
                summ = [gadd(d1[j] if j < n else (0, 0), d2[j-h] if 0 <= j-h < n else (0, 0)) for j in range(L)]
                digs = serial(summ)
                v = (sum(UNITS[c][0] << j for j, c in enumerate(digs)), sum(UNITS[c][1] << j for j, c in enumerate(digs)))
                assert v == gadd(u1, pipow(u2, dd)); cnt += 1
    print("  digit-serial with one-digit parity lookahead: exact on %d cases (n = 4, d = 1..5)" % cnt)

# ---------- addition algorithms with operation counts ----------
# Model: alu (add, sub, and, or, xor, not, neg, compare-to-mask), sh (shift), ctz (tzcnt, ctz(0) = W), br (compare and branch,
# a jump table counts 1). A select by mask is written out in alu ops. Constants (M, INF) live in registers.
# Pair state: (k, a, b). Canonical: x = (1+i)^k (a + bi). Split: x = 2^(k>>1) (1+i)^(k&1) (a + bi), i.e. w = i^(k>>1) u.
# Zero: k = INF, a = b = 0 (INF is larger than any k, so zero is always the aligned operand and contributes 0).
W = 64; INF = 1 << 20
class Ops(Counter):
    def __call__(self, kind, n=1): self[kind] += n
def ctzw(x): return W if x == 0 else ctz(x)
ZERO = (INF, 0, 0)

def p0(x, y, o):                       # ordinary pair add
    o('alu', 2); return (x[0] + y[0], x[1] + y[1])

def rot_branchy(a, b, r, o):           # (a + bi) i^r, jump table
    o('br'); r &= 3
    if r == 1: o('alu'); return -b, a
    if r == 2: o('alu', 2); return -a, -b
    if r == 3: o('alu'); return b, -a
    return a, b
def rot_bf(a, b, r, o):                # (a + bi) i^r, masks
    o('alu', 3); o('sh', 1); m0 = -(r & 1); m1 = -((r >> 1) & 1)
    o('alu', 4); t = (a ^ b) & m0; a ^= t; b ^= t          # swap if r odd
    o('alu', 2); a = (a ^ m0) - m0                          # then negate the new real part: (-b, a)
    o('alu', 4); a = (a ^ m1) - m1; b = (b ^ m1) - m1
    return a, b

def pk_branchy(x, y, o, split=False):
    (k1, a1, b1), (k2, a2, b2) = x, y
    o('alu'); d = k2 - k1
    o('br')
    if d < 0: (k1, a1, b1), (k2, a2, b2) = (k2, a2, b2), (k1, a1, b1); o('alu'); d = -d
    o('br')
    if d == 0:
        o('alu', 3); a = a1 + a2; b = b1 + b2; oo = a | b
        o('br')
        if oo == 0: return ZERO
        o('ctz'); m = ctz(oo)
        o('alu', 2); o('sh'); e = ((a & b) >> m) & 1
        o('sh', 2); a >>= m; b >>= m
        o('br')
        if e: o('alu', 2); o('sh', 2); a, b = (a + b) >> 1, (b - a) >> 1
        if split:                      # (1+i)^(e1 + e) = 2i when both odd: multiply w by i
            o('alu'); o('br')
            if k1 & e: o('alu'); a, b = -b, a
        else:
            o('alu'); a, b = rot_branchy(a, b, -m, o)
        o('alu', 2); o('sh'); k = k1 + (m << 1) + e
        return (k, a, b)
    o('sh'); h = d >> 1
    if not split:
        a2, b2 = rot_branchy(a2, b2, h, o)
    o('alu'); o('br')
    if d & 1:
        o('alu', 2); p, q = a2 - b2, a2 + b2
        if split:
            o('alu'); o('br')
            if k1 & 1: o('alu'); p, q = q, -p       # (1-i) w2 when e1 = 1
        a2, b2 = p, q
    o('sh', 2); a2 <<= h; b2 <<= h
    o('alu', 2)
    return (k1, a1 + a2, b1 + b2)

def pk_bf(x, y, o, split=False):
    (k1, a1, b1), (k2, a2, b2) = x, y
    o('alu'); o('sh'); d = k2 - k1; s = -1 if d < 0 else 0                # sign mask (sar 63)
    o('alu', 2); o('alu', 2); k1 = k1 + (d & s); d = (d ^ s) - s            # k1 = min, d = |d|
    o('alu', 8)                                                             # conditional swap of (a, b) pairs
    if s: a1, b1, a2, b2 = a2, b2, a1, b1
    o('sh'); o('alu', 2); h = d >> 1; om = -(d & 1)
    o('alu', 2); p, q = a2 - b2, a2 + b2                                    # (1+i) w2
    if split:
        o('alu', 7); em = -(k1 & 1)                                          # (1-i) w2 = (q, -p) when e1 = 1
        if em: p, q = q, -p
    o('alu', 6)                                                             # select by d odd
    if om: a2, b2 = p, q
    if not split:
        a2, b2 = rot_bf(a2, b2, h, o)
    o('sh', 2); a2 <<= h; b2 <<= h
    o('alu', 2); a = a1 + a2; b = b1 + b2
    # strip: a no-op (m = e = 0) when d != 0, since the sum is then odd
    o('alu'); o('ctz'); oo = a | b; m = ctzw(oo)
    o('alu', 2); o('sh'); e = ((a & b) >> m) & 1
    o('sh', 2); a >>= m; b >>= m
    o('alu', 7); o('sh', 2); em = -e; a, b = (a + (b & em)) >> e, (b - (a & em)) >> e
    if split:
        o('alu', 2); im = -(k1 & e & 1)                                      # times i when e1 = e = 1
        o('alu', 6)
        if im: a, b = -b, a
    else:
        o('alu'); a, b = rot_bf(a, b, -m, o)
    o('alu', 2); o('sh'); k = k1 + (m << 1) + e
    o('alu', 4)                                                             # zero result: k = INF
    if oo == 0: return ZERO
    return (k, a, b)

# unit-digit state: (k, A, B) with n digits; zero = (INF, ., .) with a flag; output encoded with n_out digits
def ud_dec(k, A, B, n, o):
    if k >= INF: return ZERO
    o('alu', 4); r, i_ = ud_decode(A, B, n); return (k, r, i_)
def ud_enc(z, n, o):
    if z[0] >= INF: return (INF, 0, 0)
    o('alu', 6); o('sh', 2); A, B = ud_encode((z[1], z[2]), n); return (z[0], A, B)
def ud_alg(inner):
    def f(x, y, n, nout, o):
        return ud_enc(inner(ud_dec(*x, n, o), ud_dec(*y, n, o), o), nout, o)
    return f

def ud_d0(x, y, n, nout, o):           # d = 0 directly on digits, split convention; else via decode
    (k1, A1, B1), (k2, A2, B2) = x, y
    if k1 != k2 or k1 >= INF: return ud_alg(lambda p, q, oo: pk_branchy(p, q, oo, True))(x, y, n, nout, o)
    o('alu'); o('br'); o('br')
    M = (1 << n) - 1; Mo = (1 << nout) - 1
    o('alu', 6); X1 = A1 ^ B1; X2 = A2 ^ B2; P = M - (X1 + X2); Q = M - (A1 + A2)  # u1 + u2 = -i(1+i)(P + iQ)
    o('alu'); o('br'); oo = P | Q
    if oo == 0: return (INF, 0, 0)
    o('ctz'); o('alu', 2); o('sh'); m = ctz(oo); e = ((P & Q) >> m) & 1     # t = 1 + 2m + e
    o('sh', 2); P >>= m; Q >>= m
    # sum of split parts = -i(1+i) 2^m z', z' = (P + iQ) >> m. e = 1: encode D = z'/(1+i), sum = 2^(m+1) D.
    # e = 0: encode D = -i z' (so (1+i) D = (1-i) z'), sum = (1+i) 2^m D; with k1 odd, (1+i)^2 = 2i gives w = i D.
    o('br')
    if e: R, S = P, Q
    else: o('alu', 2); R, S = P + Q, Q - P
    o('alu', 2); o('sh', 2); Rp = (R + Mo) >> 1; Sp = (S + Mo) >> 1
    o('alu', 2); A = Mo ^ Sp; B = Rp ^ Sp
    o('alu', 2); o('sh'); k = k1 + 1 + (m << 1) + e
    o('alu', 1); o('br')
    if (k1 & 1) and not e: o('alu', 2); A, B = A ^ B, Mo ^ B             # times i
    return (k, A, B)

def digit_rot(A, B, r, Mo, o):        # multiply digits by i^r: code + r mod 4 per digit
    r &= 3
    if r == 0: return A, B
    o('alu', 2 if r != 2 else 1)
    if r == 1: return A ^ B, Mo ^ B
    if r == 2: return Mo ^ A, B
    return Mo ^ A ^ B, Mo ^ B

def to_split(x):
    k, u = to_k(x)
    if k == ZK: return ZERO
    a, b = ipow(u, k >> 1); return (k, a, b)
def from_split(s):
    k, a, b = s
    return (0, 0) if k >= INF else from_k(k, ipow((a, b), -(k >> 1)))
def to_canon(x):
    k, u = to_k(x); return ZERO if k == ZK else (k, u[0], u[1])
def from_canon(s): return (0, 0) if s[0] >= INF else from_k(s[0], (s[1], s[2]))

def algs():
    sb = lambda x, y, o: pk_branchy(x, y, o, True)
    sf = lambda x, y, o: pk_bf(x, y, o, True)
    return [
        ("K pair, canonical, branchy", 'canon', lambda x, y, n, no, o: pk_branchy(x, y, o)),
        ("K pair, canonical, branch-free", 'canon', lambda x, y, n, no, o: pk_bf(x, y, o)),
        ("S pair, split, branchy", 'split', lambda x, y, n, no, o: sb(x, y, o)),
        ("S pair, split, branch-free", 'split', lambda x, y, n, no, o: sf(x, y, o)),
        ("U digits, decode + S branchy + encode", 'ud', ud_alg(sb)),
        ("U digits, decode + S branch-free + encode", 'ud', ud_alg(sf)),
        ("U digits, direct d = 0, else as above", 'ud', ud_d0),
    ]
def state(kind, x, n):
    if kind == 'canon': return to_canon(x)
    s = to_split(x)
    if kind == 'split': return s
    if s[0] >= INF: return (INF, 0, 0)
    A, B = ud_encode((s[1], s[2]), n); return (s[0], A, B)
def value(kind, s, n):
    if kind == 'canon': return from_canon(s)
    if kind == 'split': return from_split(s)
    if s[0] >= INF: return (0, 0)
    r, i_ = ud_decode(s[1], s[2], n); return from_split((s[0], r, i_))

def run_algs():
    print("addition algorithms: verification")
    AL = algs()
    def verify(n, xs, ys, label, pairs=None):
        nout = n + 6; cnt = 0
        it = pairs if pairs is not None else ((x, y) for x in xs for y in ys)
        cache = {}
        for x, y in it:
            want = gadd(x, y)
            nout = n + 2 + abs(to_k(x)[0] - to_k(y)[0]) // 2 if x != (0, 0) and y != (0, 0) else n + 2
            for name, kind, f in AL:
                key = (kind, x)
                sx = cache.get(key) or cache.setdefault(key, state(kind, x, n))
                key = (kind, y)
                sy = cache.get(key) or cache.setdefault(key, state(kind, y, n))
                got = f(sx, sy, n, nout, Ops())
                assert value(kind, got, nout) == want, (name, x, y, got)
                if kind != 'ud' and got[0] < INF: assert odd((got[1], got[2]))
            cnt += 1
        print("  %s: %d sums, all 7 algorithms exact" % (label, cnt))
    for n in (3, 4, 5, 6):
        us = [ud_value(A, B, n) for A in range(1 << n) for B in range(1 << n)]
        if n == 3:
            vals = [pipow(u, k) for u in us for k in range(9)] + [(0, 0)]
            verify(n, vals, vals, "n=3 exhaustive, k1, k2 in 0..8 and zero")
        elif n == 4:
            xs = [pipow(u, k) for u in us for k in (0, 1)] + [(0, 0)]
            ys = [pipow(u, k) for u in us for k in range(9)] + [(0, 0)]
            verify(n, xs, ys, "n=4 exhaustive, k1 in {0,1} (+zero), k2 in 0..8 (+zero)")
        else:
            xs = [pipow(u, k) for u in us for k in range(9)] + [(0, 0)]
            random.seed(n)
            pairs = [(random.choice(xs), random.choice(xs)) for _ in range(150000)]
            for u in us[::max(1, len(us)//64)]:
                pairs += [(u, gmul(u, (-1, 0))), (u, u), (pipow(u, 3), pipow(gmul(u, (0, 1)), 3))]
            verify(n, None, None, "n=%d sampled, k in 0..8, plus cancelling and equal pairs" % n, pairs)
    # n = 32 random, k geometric (as in a box of ordinary values) and k uniform 0..8
    n = 32; random.seed(32)
    def rnd_u():
        while True:
            u = (random.randrange(-2**31, 2**31), random.randrange(-2**31, 2**31))
            if odd(u) and l1(u) <= 2**32 - 1: return u
    def rnd_k(geo):
        if geo:
            k = 0
            while random.random() < 0.5: k += 1
            return k
        return random.randrange(9)
    pairs = []
    for geo in (True, False):
        for _ in range(30000):
            pairs.append((pipow(rnd_u(), rnd_k(geo)), pipow(rnd_u(), rnd_k(geo))))
    for _ in range(3000):
        u = rnd_u(); k = rnd_k(True); v = gadd(gmul(u, (-1, 0)), (1 << random.randrange(31), 0))
        if l1(v) <= 2**32 - 1: pairs.append((pipow(u, k), pipow(v, k)))
    verify(n, None, None, "n=32 random (geometric and uniform k, near-cancelling pairs)", pairs)
    # operation counts
    print("operation counts, n=32, k geometric (P(k) = 2^-(k+1), as for random ordinary values)")
    random.seed(33)
    pairs = [(pipow(rnd_u(), rnd_k(True)), pipow(rnd_u(), rnd_k(True))) for _ in range(40000)]
    rows = []
    for name, kind, f in AL:
        bycase = defaultdict(Counter); ncase = Counter(); worst = 0
        for x, y in pairs:
            sx, sy = state(kind, x, n), state(kind, y, n)
            o = Ops(); f(sx, sy, n, n + 6, o)
            d = abs(to_k(x)[0] - to_k(y)[0]); c = 'd=0' if d == 0 else ('d odd' if d & 1 else 'd even')
            bycase[c].update(o); ncase[c] += 1; worst = max(worst, sum(o.values()))
        tot = Counter()
        for c in bycase: tot.update(bycase[c])
        N = len(pairs)
        avg = sum(tot.values()) / N
        cells = ["%.1f" % (sum(bycase[c].values()) / ncase[c]) for c in ('d=0', 'd odd', 'd even')]
        mix = "alu %.1f sh %.1f ctz %.2f br %.1f" % tuple(tot[k_]/N for k_ in ('alu', 'sh', 'ctz', 'br'))
        rows.append((name, avg, cells, worst, mix))
    share = Counter()
    for x, y in pairs:
        d = abs(to_k(x)[0] - to_k(y)[0]); share['d=0' if d == 0 else ('d odd' if d & 1 else 'd even')] += 1
    print("  case shares: " + ", ".join("%s %.3f" % (c, share[c]/len(pairs)) for c in ('d=0', 'd odd', 'd even')))
    print("  %-44s %6s  %6s %6s %6s %6s  %s" % ("algorithm", "avg", "d=0", "d odd", "d even", "worst", "mix (avg)"))
    print("  %-44s %6.1f" % ("ordinary pair add", 2.0))
    for name, avg, cells, worst, mix in rows:
        print("  %-44s %6.1f  %6s %6s %6s %6d  %s" % (name, avg, *cells, worst, mix))

# ---------- lazy forms: no stripping on add (normalise on demand) ----------
# LS: x = 2^(k>>1) (1+i)^(k&1) w, w any Gaussian integer (not nec. odd). LB: x = 2^m w, w any.
def ls_branchy(x, y, o):
    (k1, a1, b1), (k2, a2, b2) = x, y
    o('alu'); o('br'); d = k2 - k1
    if d < 0: (k1, a1, b1), (k2, a2, b2) = (k2, a2, b2), (k1, a1, b1); o('alu'); d = -d
    o('br')
    if d == 0: o('alu', 2); return (k1, a1 + a2, b1 + b2)
    o('sh'); h = d >> 1
    o('alu'); o('br')
    if d & 1:
        o('alu', 2); p, q = a2 - b2, a2 + b2
        o('alu'); o('br')
        if k1 & 1: o('alu'); p, q = q, -p
        a2, b2 = p, q
    o('sh', 2); o('alu', 2)
    return (k1, a1 + (a2 << h), b1 + (b2 << h))
def ls_bf(x, y, o):
    (k1, a1, b1), (k2, a2, b2) = x, y
    o('alu', 6); o('sh'); d = k2 - k1; s = -1 if d < 0 else 0; k1 = k1 + (d & s); d = (d ^ s) - s
    o('alu', 8)
    if s: a1, b1, a2, b2 = a2, b2, a1, b1
    o('sh'); o('alu', 2); h = d >> 1; om = -(d & 1)
    o('alu', 4); a2, b2 = a2 - (b2 & om), b2 + (a2 & om)                  # times (1+i) when d odd
    o('alu', 3); rm = om & -(k1 & 1)                                      # then times -i when also e1 = 1
    o('alu', 6)
    if rm: a2, b2 = b2, -a2
    o('sh', 2); o('alu', 2)
    return (k1, a1 + (a2 << h), b1 + (b2 << h))
def lb_branchy(x, y, o):
    (m1, a1, b1), (m2, a2, b2) = x, y
    o('alu'); o('br'); d = m2 - m1
    if d < 0: (m1, a1, b1), (m2, a2, b2) = (m2, a2, b2), (m1, a1, b1); o('alu'); d = -d
    o('sh', 2); o('alu', 2)
    return (m1, a1 + (a2 << d), b1 + (b2 << d))
def lb_bf(x, y, o):
    (m1, a1, b1), (m2, a2, b2) = x, y
    o('alu', 6); o('sh'); d = m2 - m1; s = -1 if d < 0 else 0; m1 = m1 + (d & s); d = (d ^ s) - s
    o('alu', 8)
    if s: a1, b1, a2, b2 = a2, b2, a1, b1
    o('sh', 2); o('alu', 2)
    return (m1, a1 + (a2 << d), b1 + (b2 << d))
def ls_normalise(z, o):                 # LS -> split canonical (the strip of pk_branchy, split form)
    k1, a, b = z
    o('alu'); o('br'); oo = a | b
    if oo == 0: return ZERO
    o('ctz'); m = ctz(oo)
    o('alu', 2); o('sh'); e = ((a & b) >> m) & 1
    o('sh', 2); a >>= m; b >>= m
    o('br')
    if e: o('alu', 2); o('sh', 2); a, b = (a + b) >> 1, (b - a) >> 1
    o('alu'); o('br')
    if k1 & e: o('alu'); a, b = -b, a
    o('alu', 2); o('sh')
    return (k1 + (m << 1) + e, a, b)

def run_lazy():
    print("lazy forms")
    random.seed(35)
    def val_ls(s): k, a, b = s; return from_k(k, ipow((a, b), -(k >> 1)))
    def val_lb(s): m, a, b = s; return (a << m, b << m)
    cnt = 0
    for trial in range(60000):
        R = 1 << random.choice((3, 8, 31))
        x = (random.randrange(10), random.randrange(-R, R), random.randrange(-R, R))
        y = (random.randrange(10), random.randrange(-R, R), random.randrange(-R, R))
        for f, v in ((ls_branchy, val_ls), (ls_bf, val_ls), (lb_branchy, val_lb), (lb_bf, val_lb)):
            assert v(f(x, y, Ops())) == gadd(v(x), v(y))
        z = ls_branchy(x, y, Ops()); nz = ls_normalise(z, Ops())
        assert from_split(nz) == val_ls(z) and (nz[0] >= INF or odd((nz[1], nz[2])))
        cnt += 1
    print("  %d random sums (any w, k 0..9): LS, LB both forms exact; LS normalise gives the split canonical form" % cnt)
    random.seed(36)
    rows = []
    for name, f, kind in (("LS lazy split, branchy", ls_branchy, 'ls'), ("LS lazy split, branch-free", ls_bf, 'ls'),
                          ("LB lazy 2^m, branchy", lb_branchy, 'lb'), ("LB lazy 2^m, branch-free", lb_bf, 'lb')):
        tot = Ops(); N = 20000; eq = 0
        for _ in range(N):
            k1 = k2 = 0
            while random.random() < 0.5: k1 += 1
            while random.random() < 0.5: k2 += 1
            if kind == 'lb': k1 >>= 1; k2 >>= 1
            f((k1, 1, 2), (k2, 3, 4), tot)
        rows.append((name, sum(tot.values()) / N))
    tot = Ops(); N = 20000
    for _ in range(N):
        a, b = random.randrange(-2**31, 2**31), random.randrange(-2**31, 2**31); ls_normalise((0, a, b), tot)
    rows.append(("LS normalise (random w)", sum(tot.values()) / N))
    for name, c in rows: print("  %-36s %5.1f ops avg (k geometric)" % (name, c))

# ---------- odd representatives modulo an odd modulus nu ----------
# Cell(g): r with coordinates z = r conj(g) / N(g) in [-1/2, 1/2)^2 (half-open). Plain residues: Cell(nu).
# Odd representatives: the odd points of Cell((1+i) nu); every class mod nu has exactly one (checked below).
def conj(z): return (z[0], -z[1])
def norm(z): return z[0]*z[0] + z[1]*z[1]
def reduce_cell(x, g):
    n1, n2 = gmul(x, conj(g)); N = norm(g)
    q = ((2*n1 + N) // (2*N), (2*n2 + N) // (2*N))
    return gadd(x, gmul(g, (-q[0], -q[1])))
def in_cell(r, g):
    n1, n2 = gmul(r, conj(g)); N = norm(g)
    return -N <= 2*n1 < N and -N <= 2*n2 < N
def same_class(x, y, nu):
    n1, n2 = gmul((x[0]-y[0], x[1]-y[1]), conj(nu)); N = norm(nu)
    return n1 % N == 0 and n2 % N == 0
def odd_canon(x, nu):              # parity fix (masked add of nu), then reduce mod (1+i) nu
    if not odd(x): x = gadd(x, nu)
    return reduce_cell(x, pimul(nu))
def odd_add(x, y, nu):             # x, y odd reps: s even; subtract exactly one of +-nu, +-i nu, by two sign tests
    g = pimul(nu); s = gadd(x, y); n1, n2 = gmul(s, conj(g))
    s1 = 1 if n1 >= 0 else -1; s2 = 1 if n2 >= 0 else -1
    c = gmul(g, (s1, s2)); c = (c[0] // 2, c[1] // 2)          # g (s1 + i s2) / 2, one of +-nu, +-i nu
    return (s[0] - c[0], s[1] - c[1])
def odd_sub(x, y, nu): return odd_add(x, (-y[0], -y[1]), nu)  # same rule: coordinates of x - y lie in (-1, 1)
def plain_add(x, y, nu):           # plain: coordinates in [-1, 1), subtract nu (q1 + i q2), q_j in {-1, 0, 1}
    s = gadd(x, y); n1, n2 = gmul(s, conj(nu)); N = norm(nu)
    q1 = 1 if 2*n1 >= N else (-1 if 2*n1 < -N else 0); q2 = 1 if 2*n2 >= N else (-1 if 2*n2 < -N else 0)
    return gadd(s, gmul(nu, (-q1, -q2)))

def gaussian_primes(maxnorm):
    out = []
    for q in range(5, maxnorm + 1, 4):
        if all(q % d for d in range(2, int(q**0.5) + 1)):
            for a in range(1, 30):
                b2 = q - a*a
                if b2 > 0 and int(round(b2**0.5))**2 == b2: out.append((a, int(round(b2**0.5)))); break
    return out

def run_mod():
    print("odd representatives modulo odd nu")
    mods = [(p, 0) for p in (3, 7, 11, 19)] + [(p, 0) for p in (5, 13, 17)] + [(9, 0), (15, 0)]
    mods += gaussian_primes(300) + [gmul((2, 1), (3, 2)), gmul((2, 1), (2, -1)), gmul((1, 2), (1, 2)), (3, 6)]
    mods = [m for m in mods if odd(m)]
    stats = []; total = 0
    for nu in mods:
        N = norm(nu); g = pimul(nu)
        R = int(norm(g) ** 0.5) + 2
        cellg = [(a, b) for a in range(-R, R+1) for b in range(-R, R+1) if in_cell((a, b), g)]
        celln = [(a, b) for a in range(-R, R+1) for b in range(-R, R+1) if in_cell((a, b), nu)]
        assert len(cellg) == 2*N and len(celln) == N
        reps = [r for r in cellg if odd(r)]
        assert len(reps) == N
        for i_ in range(len(reps)):                     # distinct classes: one odd rep per class
            for j_ in range(i_):
                assert not same_class(reps[i_], reps[j_], nu)
        odd_in_plain = sum(odd(r) for r in celln)
        ties = sum(1 for r in cellg if gmul(r, conj(g))[0] * 2 in (-norm(g),) or gmul(r, conj(g))[1] * 2 in (-norm(g),))
        nplain_ties = sum(1 for (a, b) in [(a, b) for a in range(-R, R+1) for b in range(-R, R+1)]
                          if any(abs(2*c) == N for c in gmul((a, b), conj(nu))))
        S = set(reps)
        for x in reps:
            assert odd_canon(x, nu) == x
            nx = (-x[0], -x[1])
            if nx not in S: assert odd_canon(nx, nu) in S
            for y in reps:
                s = odd_add(x, y, nu); assert s in S and s == odd_canon(gadd(x, y), nu)
                d = odd_sub(x, y, nu); assert d in S and d == odd_canon((x[0]-y[0], x[1]-y[1]), nu)
                pr = gmul(x, y); assert odd(pr)
                m = reduce_cell(pr, g); assert m in S and m == odd_canon(pr, nu)
                total += 1
        for x in celln:
            for y in celln[::3]:
                assert plain_add(x, y, nu) == reduce_cell(gadd(x, y), nu)
        negfix = sum(1 for x in reps if (-x[0], -x[1]) not in S)
        stats.append((nu, N, odd_in_plain, ties, negfix, nplain_ties))
    print("  %d moduli (rational 3..19 incl. 9, 15; Gaussian primes of norm <= 300; products), %d (x, y) pairs each way" % (len(mods), total))
    print("  checked: one odd rep per class in Cell((1+i)nu); odd_add/odd_sub (one forced +-nu/+-i nu by two signs) land in it;")
    print("  odd * odd is odd and reducing it mod (1+i)nu keeps it odd (no parity fix); plain_add exact")
    print("  %-10s %5s %18s %18s %8s" % ("nu", "N", "odd in plain cell", "boundary pts (odd)", "neg fix"))
    for nu, N, oip, ties, nf, npt in stats[:6] + stats[9:12] + stats[-3:]:
        print("  %-10s %5d %18s %18d %8d" % ("%d%+di" % nu, N, "%d (%.2f)" % (oip, oip / N), ties, nf))
    print("  plain cells of odd nu have no boundary points (ties): %s" % all(s[5] == 0 for s in stats))
    # rational p: odd reps in rotated offset coordinates are plain residues of x (1+i)/2 in [1, p]
    for p in (3, 7, 11, 13, 101):
        nu = (p, 0); reps = [] ; R = p
        for a in range(-R, R+1):
            for b in range(-R, R+1):
                if odd((a, b)) and in_cell((a, b), pimul(nu)): reps.append((a, b))
        inv2 = pow(2, -1, p)
        for x in reps:
            Rr, Ss = x[0] - x[1], x[0] + x[1]
            assert -p < Rr <= p and -p <= Ss < p          # the cell's half-open convention
            R2, S2 = (Rr + p) // 2, (Ss + p) // 2
            assert 1 <= R2 <= p and 0 <= S2 < p
            tw = gmul(x, (1, 1)); tw = (tw[0] * inv2 % p, tw[1] * inv2 % p)
            assert (R2 % p, S2 % p) == tw
    print("  rational p (3, 7, 11, 13, 101): odd rep x <-> (R, S) = (1+i)x, R odd in (-p, p], S odd in [-p, p)")
    print("    <-> ((R+p)/2, (S+p)/2) in [1, p] x [0, p-1],")
    print("    which is the plain residue of x (1+i)/2: the odd form is a relabelling (twist by the unit (1+i)/2)")
    # random large
    random.seed(37); cnt = 0
    for _ in range(3000):
        while True:
            nu = (random.randrange(-2**30, 2**30), random.randrange(-2**30, 2**30))
            if odd(nu): break
        g = pimul(nu)
        x = odd_canon((random.randrange(-2**62, 2**62), random.randrange(-2**62, 2**62)), nu)
        y = odd_canon((random.randrange(-2**62, 2**62), random.randrange(-2**62, 2**62)), nu)
        for v in (x, y): assert odd(v) and in_cell(v, g)
        s = odd_add(x, y, nu); assert s == odd_canon(gadd(x, y), nu) and same_class(s, gadd(x, y), nu)
        d = odd_sub(x, y, nu); assert d == odd_canon((x[0]-y[0], x[1]-y[1]), nu)
        m = reduce_cell(gmul(x, y), g); assert odd(m) and same_class(m, gmul(x, y), nu)
        assert odd_canon(gadd(x, nu), nu) == x            # x and x + nu (even) map to the same rep
        assert odd_canon(gadd(x, g), nu) == x
        cnt += 1
    print("  random nu with |re|, |im| < 2^30: %d cases exact (add, sub, mul, canonical form)" % cnt)

# counted add kernels for a rational odd p (word ops; sar = arithmetic shift by 63, giving 0 or -1)
def sar(x): return -1 if x < 0 else 0
def k_plain01(x, y, p, o):         # plain residues in [0, p) per part: add, sub, sar, and, add
    o('alu', 8); o('sh', 2)
    out = []
    for a, b in zip(x, y):
        s = a + b - p; out.append(s + (p & sar(s)))
    return tuple(out)
def k_plainsym(x, y, p, o):        # plain symmetric residues in [-(p-1)/2, (p-1)/2] per part
    h = (p - 1) // 2; out = []
    o('alu', 14); o('sh', 4)
    for a, b in zip(x, y):
        s = a + b; s -= p & sar(h - s); s += p & sar(s + h); out.append(s)
    return tuple(out)
def k_odd_rot(x, y, p, o):         # odd reps kept as (R, S) = (1+i)x; R odd in (-p, p], S odd in [-p, p)
    p2 = 2 * p
    o('alu', 5); o('sh'); R = x[0] + y[0]; R += (p2 & sar(R - 1)) - p     # R <= 0: +p, else -p
    o('alu', 4); o('sh'); S = x[1] + y[1]; S += (p2 & sar(S)) - p         # S < 0: +p, else -p
    return (R, S)
def k_odd_pair(x, y, p, o):        # odd reps kept as the pair x itself
    p2 = 2 * p
    o('alu', 4); sa, sb = x[0] + y[0], x[1] + y[1]; R, S = sa - sb, sa + sb
    o('alu', 5); o('sh', 2); dR = (p2 & sar(R - 1)) - p; dS = (p2 & sar(S)) - p
    o('alu', 4); o('sh', 2)
    return (sa + ((dR + dS) >> 1), sb + ((dS - dR) >> 1))                   # (dR + i dS) / (1+i)

def run_modcost():
    print("add kernels, rational p: verification and counts")
    def reps_odd(p):
        return [(a, b) for a in range(-p, p+1) for b in range(-p, p+1) if odd((a, b)) and in_cell((a, b), pimul((p, 0)))]
    for p in (3, 5, 7, 11, 13, 17, 19, 23):
        ro = reps_odd(p); rot = [(x[0]-x[1], x[0]+x[1]) for x in ro]
        for x in ro:
            for y in ro:
                w = odd_add(x, y, (p, 0)); o = Ops()
                assert k_odd_pair(x, y, p, o) == w
                assert k_odd_rot((x[0]-x[1], x[0]+x[1]), (y[0]-y[1], y[0]+y[1]), p, o) == (w[0]-w[1], w[0]+w[1])
        r01 = [(a, b) for a in range(p) for b in range(p)]
        for x in r01[::3]:
            for y in r01:
                assert k_plain01(x, y, p, Ops()) == ((x[0]+y[0]) % p, (x[1]+y[1]) % p)
                xs = tuple(((v + p//2) % p) - p//2 for v in x); ys = tuple(((v + p//2) % p) - p//2 for v in y)
                assert k_plainsym(xs, ys, p, Ops()) == tuple(((xs[j]+ys[j] + p//2) % p) - p//2 for j in (0, 1))
    random.seed(38)
    for _ in range(20000):
        p = random.randrange(3, 2**61, 2)
        x = odd_canon((random.randrange(-2**62, 2**62), random.randrange(-2**62, 2**62)), (p, 0))
        y = odd_canon((random.randrange(-2**62, 2**62), random.randrange(-2**62, 2**62)), (p, 0))
        assert k_odd_pair(x, y, p, Ops()) == odd_add(x, y, (p, 0))
    print("  exhaustive p = 3..23 and random p < 2^61: all four kernels exact")
    for name, f in (("plain [0, p)", k_plain01), ("plain symmetric", k_plainsym), ("odd, rotated (R, S)", k_odd_rot), ("odd, pair x", k_odd_pair)):
        o = Ops(); f((1, 1), (1, 1), 7, o); print("  %-22s %2d ops (%s)" % (name, sum(o.values()), ", ".join("%s %d" % kv for kv in sorted(o.items()))))

if __name__ == "__main__":
    what = sys.argv[1] if len(sys.argv) > 1 else "all"
    random.seed(31)
    if what in ("conv", "all"): check_conv()
    if what in ("pat", "all"): patterns()
    if what in ("carry", "all"): carry_structure()
    if what in ("alg", "all"): run_algs()
    if what in ("lazy", "all"): run_lazy()
    if what in ("mod", "all"): run_mod(); run_modcost()
