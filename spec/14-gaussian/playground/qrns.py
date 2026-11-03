# Study 16: exact Gaussian integers in a quadratic residue number system (QRNS).
# Primes p = 1 (mod 4), t^2 = -1 (mod p). Per prime, a + bi -> (u, v) = (a + bt, a - bt) mod p (study 5's split), so
# add and multiply are lane by lane: 2 residues per prime per Gaussian integer, m primes, M = p_1 ... p_m.
# Conversion out: per prime a = (u + v)/2, b = (u - v)/(2t); then Garner's mixed radix over the primes, Horner to binary,
# and the centered (signed) value in (-M/2, M/2). Exact while |re|, |im| < M/2.
# Scalar residue arithmetic: Barrett (study 4). Vector lanes: Montgomery with R = 2^h in h-bit lanes (no 64-bit mulhi needed).
# Overflow: (1) bound tracking, (2) a redundant extra prime checked at conversion out, (3) sizing m in advance.
# Usage: python3 qrns.py toy | random
import sys, random, math

def isprime(n):
    if n < 2: return False
    for q in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        if n % q == 0: return n == q
    d, s = n - 1, 0
    while d % 2 == 0: d //= 2; s += 1
    for a in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        x = pow(a, d, n)
        if x in (1, n - 1): continue
        for _ in range(s - 1):
            x = x * x % n
            if x == n - 1: break
        else: return False
    return True
def sqrt_m1(p):
    c = 2
    while pow(c, (p - 1) // 2, p) != p - 1: c += 1
    return pow(c, (p - 1) // 4, p)

# ---------- scalar residue operations (64-bit registers, p < 2^31): Barrett as study 4
W = 64; MW = (1 << W) - 1
def barrett(t, p, m):                   # t < 2^63: MULHI, MUL, SUB, SUB, SAR, AND, ADD = 7
    r = (t - ((t * m) >> W) * p) & MW; r = (r - p) & MW; return (r + (p & (MW if r >> 63 else 0))) & MW
def addm(x, y, p): r = (x + y - p) & MW; return (r + (p & (MW if r >> 63 else 0))) & MW    # 5
def subm(x, y, p): r = (x - y) & MW; return (r + (p & (MW if r >> 63 else 0))) & MW        # 4
def halfm(x, p): return (x + (p & -(x & 1))) >> 1                                           # 5

class Prime:
    def __init__(s, p):
        s.p = p; s.m = (1 << W) // p; s.t = sqrt_m1(p); s.i2t = pow(2 * s.t, -1, p)
    def to(s, a, b):                    # signed a, b with |a|, |b| < 2^31 (full range: a may be -2^31)
        p, m = s.p, s.m
        ar = barrett(a + 2 * p, p, m); br = barrett(b + 2 * p, p, m)       # ADD + Barrett: 8 each (a + 2p > 0 since p > 2^30)
        bt = barrett(br * s.t, p, m)                                     # MUL + Barrett 8
        return addm(ar, bt, p), subm(ar, bt, p)                          # 5 + 4.     per prime 33
    def back(s, u, v):                  # residues of a and b: (u + v)/2 10, (u - v)(2t)^-1 12: 22
        p, m = s.p, s.m
        return halfm(addm(u, v, p), p), barrett(subm(u, v, p) * s.i2t, p, m)

class RNS:
    def __init__(s, primes, redundant=None):
        s.P = [Prime(p) for p in primes]; s.M = math.prod(primes); s.R = Prime(redundant) if redundant else None
        # Garner constants: c[j][i] = p_i^-1 mod p_j (i < j)
        s.c = [[pow(primes[i], -1, primes[j]) for i in range(j)] for j in range(len(primes))]
    def all(s): return s.P + ([s.R] if s.R else [])
    def to(s, a, b): return [P.to(a, b) for P in s.all()]                 # 33 per prime (+ extract 3 per element)
    def add(s, X, Y): return [(addm(x[0], y[0], P.p), addm(x[1], y[1], P.p)) for P, x, y in zip(s.all(), X, Y)]  # 10 per prime
    def mul(s, X, Y): return [(barrett(x[0] * y[0], P.p, P.m), barrett(x[1] * y[1], P.p, P.m)) for P, x, y in zip(s.all(), X, Y)]  # 16 per prime
    def garner(s, res):                 # mixed-radix digits x_1..x_m: value = x_1 + x_2 p_1 + x_3 p_1 p_2 + ...
        xs = []
        for j, P in enumerate(s.P):
            v = res[j]
            for i in range(j): v = barrett(subm(v, xs[i] % P.p, P.p) * s.c[j][i], P.p, P.m)   # submod 4 + mulmod 8 = 12
            xs.append(v)
        return xs
    def value(s, xs):                   # Horner to binary (multiword in a real build), then centered
        v = 0
        for j in range(len(s.P) - 1, -1, -1): v = v * s.P[j].p + xs[j] if j < len(s.P) - 1 else xs[j]
        return v - s.M if v > s.M // 2 else v
    def back(s, X):                     # returns (a, b, consistent with the redundant prime?)
        pairs = [P.back(*x) for P, x in zip(s.P, X)]
        a = s.value(s.garner([q[0] for q in pairs])); b = s.value(s.garner([q[1] for q in pairs]))
        ok = True
        if s.R:
            ra, rb = s.R.back(*X[-1]); ok = (a % s.R.p, b % s.R.p) == (ra, rb)
        return a, b, ok

# ---------- vector lanes: Montgomery in h-bit lanes (R = 2^h), p < 2^(h-1)
def mont_consts(p, h): R = 1 << h; return R, (-pow(p, -1, R)) % R
def montmul(x, y, p, h):                # REDC(x y): t = x y (widening), m = (t mod R) n' mod R, u = (t + m p) / R < 2p, u - p if u >= p
    R, ninv = mont_consts(p, h); t = x * y; mm = ((t & (R - 1)) * ninv) & (R - 1); u = (t + mm * p) >> h
    v = (u - p) & (R - 1); return min(u, v)                              # SUB, unsigned MIN (min trick: u < 2p < 2^h)
def lane_add(x, y, p, h): R = 1 << h; r = x + y; return min(r, (r - p) & (R - 1))   # ADD, SUB, MINU = 3 (x + y < 2p < 2^h)
def lane_sub(x, y, p, h): R = 1 << h; r = (x - y) & (R - 1); return min(r, (r + p) & (R - 1))  # SUB, ADD, MINU = 3

# ---------- instruction counts (scalar, per Gaussian element)
def counts(m):
    conv_in = 3 + 33 * m
    add, sub, mul = 10 * m, 8 * m, 16 * m
    words = lambda bits: -(-bits // 64)
    horner = 0                          # Horner steps: v = v p_j + x_j; v grows by 31 bits per step
    for s in range(2, m + 1):           # step multiplying a value of 31(s-1) bits by p: MUL (+ MULHI, carry) per word
        horner += 2 if s == 2 else 5 * words(31 * (s - 1))
    center = 5 * words(31 * m)          # compare with M/2 and conditional subtract of M, per word
    conv_out = 22 * m + 2 * (12 * m * (m - 1) // 2) + 2 * (horner + center)
    return conv_in, add, sub, mul, conv_out

st = {}
def chk(k, ok):
    st[k + ' cases'] = st.get(k + ' cases', 0) + 1
    if not ok: st[k + ' FAIL'] = st.get(k + ' FAIL', 0) + 1
def gm(x, y): return (x[0]*y[0] - x[1]*y[1], x[0]*y[1] + x[1]*y[0])
def fitsM(z, M): return all(abs(v) <= (M - 1) // 2 for v in z)

def run_chain(rns, zs, ops):            # zs: Gaussian inputs, ops: 'add' / 'mul' applied left to right; returns exact, rns result
    acc = zs[0]; X = rns.to(*zs[0]); bound = max(abs(zs[0][0]), abs(zs[0][1])) * math.sqrt(2)
    tracked_ok = bound < rns.M / 2                                       # the inputs themselves must fit
    for op, z in zip(ops, zs[1:]):
        Y = rns.to(*z); bz = math.hypot(*z)
        if op == 'add': acc = (acc[0] + z[0], acc[1] + z[1]); X = rns.add(X, Y); bound = bound + bz
        else: acc = gm(acc, z); X = rns.mul(X, Y); bound = bound * bz
        tracked_ok = tracked_ok and bound < rns.M / 2                    # bound tracking: |re|, |im| <= |z| <= bound
    return acc, rns.back(X), tracked_ok

mode = sys.argv[1] if len(sys.argv) > 1 else 'toy'
if mode == 'toy':
    # Montgomery lanes and Barrett, every residue pair, h = 8, every prime p = 1 mod 4 below 128
    h = 8; R = 1 << h
    for p in [q for q in range(5, 128) if isprime(q) and q % 4 == 1]:
        for x in range(p):
            for y in range(p):
                chk('lane Montgomery: REDC(x y) = x y R^-1 mod p (h = 8)', montmul(x, y, p, h) == x * y * pow(R, -1, p) % p)
                chk('lane add/sub mod p (h = 8)', lane_add(x, y, p, h) == (x + y) % p and lane_sub(x, y, p, h) == (x - y) % p)
                chk('scalar Barrett product mod p', barrett(x * y, p, (1 << W) // p) == x * y % p)
    # RNS over (5, 13, 17), redundant prime 29: conversion round trip for every a, b with |a|, |b| <= (M-1)/2
    rns = RNS([5, 13, 17], redundant=29); M = rns.M; H = (M - 1) // 2
    for a in range(-H, H + 1):
        for b in range(-H, H + 1):
            A, B, ok = rns.back(rns.to(a, b)); chk('round trip to RNS and back (all |a|, |b| <= (M-1)/2, M = 1105)', (A, B, ok) == (a, b, True))
    # every pair with |parts| <= 15 in the smaller system (5, 13), M = 65, redundant 17: add and multiply; exact when the
    # result fits, else the redundant prime flags it (when |result| < 65 17 / 2)
    rns = RNS([5, 13], redundant=17); M = rns.M; RP = 17
    vals = range(-15, 16); el = [(x, y) for x in vals for y in vals]
    for z1 in el:
        X = rns.to(*z1)
        for z2 in el:
            Y = rns.to(*z2)
            for name, f, ex in (('add', rns.add, (z1[0] + z2[0], z1[1] + z2[1])), ('mul', rns.mul, gm(z1, z2))):
                A, B, ok = rns.back(f(X, Y))
                if fitsM(ex, M): chk(f'{name}: exact when the result fits', (A, B) == ex and ok)
                elif all(abs(v) <= (M * RP - 1) // 2 for v in ex): chk(f'{name}: redundant prime flags a result that does not fit', not ok)
                else: st['beyond the redundant range (not checked)'] = st.get('beyond the redundant range (not checked)', 0) + 1
    random.seed(16)
    for i in range(20000):
        k = random.randint(1, 4); zs = [(random.randint(-4, 4), random.randint(-4, 4)) for _ in range(k + 1)]
        ops = [random.choice(['add', 'mul']) for _ in range(k)]
        ex, (A, B, ok), tracked = run_chain(rns, zs, ops)
        if fitsM(ex, M): chk('chains: exact when the result fits', (A, B) == ex and ok)
        elif all(abs(v) <= (M * RP - 1) // 2 for v in ex): chk('chains: redundant prime flags a result that does not fit', not ok)
        if tracked: chk('bound tracking: "safe" implies the result fits', fitsM(ex, M))
    print("toy: lanes h = 8, all p = 1 mod 4 below 128; RNS (5, 13, 17) + redundant 29")
else:
    random.seed(16)
    big = [p for p in range((1 << 31) - 1, (1 << 31) - 4000, -1) if isprime(p) and p % 4 == 1][:7]
    print("primes:", big)
    h = 32; R = 1 << h
    for p in big[:3]:
        for i in range(30000):
            x, y = random.randrange(p), random.randrange(p)
            chk('lane Montgomery (h = 32)', montmul(x, y, p, h) == x * y * pow(R, -1, p) % p)
            chk('lane add/sub (h = 32)', lane_add(x, y, p, h) == (x + y) % p and lane_sub(x, y, p, h) == (x - y) % p)
    HI = (1 << 31) - 1
    rnd = lambda: random.choice([random.randint(-HI - 1, HI), random.choice([HI, -HI - 1, 0, 1, -1])])
    for k in range(0, 5):
        need = 31.5 * (k + 1) + 1 + (k + 1)                              # bits of M: |z|^(k+1) with |z| < sqrt 2 2^31, + sign
        m = next(j for j in range(1, 8) if math.prod(big[:j]) > 2 ** need)
        rns = RNS(big[:m], redundant=big[m]); tr = 0
        for i in range(3000):
            zs = [(rnd(), rnd()) for _ in range(k + 1)]; ops = ['mul'] * k
            ex, (A, B, ok), tracked = run_chain(rns, zs, ops)
            chk(f'random: product of {k + 1} inputs with 32-bit halves exact (m = {m} primes)', (A, B) == ex and ok)
            tr += tracked
        # one prime fewer: results may not fit; the redundant prime must flag every one that does not
        if m > 1:
            small = RNS(big[:m - 1], redundant=big[m - 1])
            for i in range(3000):
                zs = [(rnd(), rnd()) for _ in range(k + 1)]; ex, (A, B, ok), tracked = run_chain(small, zs, ['mul'] * k)
                if fitsM(ex, small.M): chk('undersized: exact when the result fits', (A, B) == ex and ok)
                else: chk('undersized: redundant prime flags a result that does not fit', not ok)
                if tracked: chk('undersized: bound tracking "safe" implies the result fits', fitsM(ex, small.M))
        print(f"  k = {k} multiplies: m = {m} primes ({math.log2(rns.M):.1f} bits), bound tracking said safe in {tr}/3000")
for k, v in sorted(st.items()): print(f"  {k}: {v}")
print("  scalar counts per element (conv in, add, sub, mul, conv out):")
for m in range(1, 7): print(f"    m = {m}: {counts(m)}")
