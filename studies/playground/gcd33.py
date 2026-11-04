# Study 33: gcd algorithms native to B (x = (1+i)^k u, u odd in unit digits), counted in study 32's op model.
# Key fact used: B's digit words are offset binary of (R, S) = (1+i)u (study 31), so the loop can run in
# "half coordinates" H: p = (R-1)/2, q = (S-1)/2 (signed). In H the unit class of u mod (1+i)^3 is the low bit of p and q
# separately, times i is (p, q) -> (~q, p), negation is (~p, ~q), and a - b is two subtractions that come out pre-divided.
# Usage: python3 gcd33.py [verify|count|choice|hensel|all]
import sys, random
from collections import Counter
sys.path.insert(0, __file__.rsplit('/', 1)[0] if '/' in __file__ else '.')
import adic31 as S31
import rep32 as R32
from rep32 import Ops, INF, gmul, norm

def assoc_norm(z):                     # canonical associate: re > 0, im >= 0 (or 0)
    if z == (0, 0): return z
    for _ in range(4):
        if z[0] > 0 and z[1] >= 0: return z
        z = (-z[1], z[0])
def ctz(x): return (x & -x).bit_length() - 1

# ---------------- H coordinates: building blocks (counted) ----------------
def strip_to_half(x, y, o):
    """(x, y) nonzero pair of Z = x + iy. Returns H coords (p, q) of the odd part of Z (up to a unit),
    the valuation of Z, and 2N(odd part) = R^2 + S^2 (the norm in H scale). 13 ops + norm 3 (+2 for v if wanted)."""
    o('alu'); o('ctz'); oo = x | y; m = ctz(oo)
    o('sh', 2); x >>= m; y >>= m
    o('alu', 3); e = x & y & 1; nm = e - 1                    # nm = -1 when Z/2^m is odd (multiply by 1+i), 0 when it is pi*odd
    o('alu', 4); Rr = x - (y & nm); Ss = y + (x & nm)            # (R, S) = pair coords of (1+i) * odd part
    o('mul', 2); o('alu'); N2 = Rr * Rr + Ss * Ss
    o('sh', 2); return Rr >> 1, Ss >> 1, 2 * m + e, N2
def primary_H(p, q, o):
    """rotate to the class p, q even (one fixed unit class mod (1+i)^3): 11 ops."""
    o('alu', 4); mp = -(p & 1); mq = -(q & 1)
    o('alu', 2); p ^= mp; q ^= mq
    o('alu'); ms = mp ^ mq
    o('alu', 4); t = (p ^ q) & ms; p ^= t; q ^= t
    return p, q
def half_value(p, q):                   # reference: the odd u with (1+i)u = (2p+1) + i(2q+1)
    Rr, Ss = 2 * p + 1, 2 * q + 1
    return ((Rr + Ss) // 2, (Ss - Rr) // 2)

def h_loop(a, b, na, nb, o, check=False):
    """a, b primary in H, na, nb their H-norms. Replace the larger by primary(strip(a - b)) until a = b. Returns (b, steps)."""
    o('alu', 2); R32.cswap_ops(o, 3)
    if na < nb: a, b, na, nb = b, a, nb, na
    steps = 0
    while True:
        o('alu', 2); dx, dy = a[0] - b[0], a[1] - b[1]
        o('alu'); o('br'); steps += 1
        if dx | dy == 0: return b, steps
        if check: assert dx & 1 == 0 and dy & 1 == 0
        p, q, _, nz = strip_to_half(dx, dy, o)
        a = primary_H(p, q, o); na = nz
        o('alu', 2); R32.cswap_ops(o, 3)
        if na < nb: a, b, na, nb = b, a, nb, na

# ---------------- B-native gcd in H (input and output in B) ----------------
def b_to_half(A, B, n, o):             # Sp = ~A, Rp = B ^ Sp (n-bit), then remove the offset 2^(n-1): 4 ops
    Mo = (1 << n) - 1; h = 1 << (n - 1)
    o('alu', 2); Sp = Mo ^ A; Rp = B ^ Sp
    o('alu', 2); return Rp - h, Sp - h
def half_to_b(p, q, n, o):             # 4 ops
    Mo = (1 << n) - 1; h = 1 << (n - 1)
    o('alu', 2); Rp = p + h; Sp = q + h
    o('alu', 2); return Mo ^ Sp, Rp ^ Sp
def h_norm(p, q, o):                   # (2p+1)^2 + (2q+1)^2: 2 sh, 2 or, 2 mul, add
    o('sh', 2); o('alu', 3); o('mul', 2); Rr, Ss = 2 * p + 1, 2 * q + 1; return Rr * Rr + Ss * Ss
def gcd_BH(x, y, n, o, check=False):
    o('alu'); o('br')
    if x[0] >= INF or y[0] >= INF: return (x if y[0] >= INF else y), 0
    o('alu', 3); k = min(x[0], y[0])
    a = primary_H(*b_to_half(x[1], x[2], n, o), o); b = primary_H(*b_to_half(y[1], y[2], n, o), o)
    na, nb = h_norm(*a, o), h_norm(*b, o)
    b, steps = h_loop(a, b, na, nb, o, check)
    A, B = half_to_b(*b, n, o)
    return (k, A, B), steps

# ---------------- A converted into H at the start (the carry-back of the same loop) ----------------
def gcd_AH(x, y, o, check=False):
    o('alu', 4); o('br')
    if x == (0, 0) or y == (0, 0): return x if y == (0, 0) else y, 0
    p1, q1, k1, na = strip_to_half(*x, o); p2, q2, k2, nb = strip_to_half(*y, o)
    o('sh', 2); o('alu', 2); o('alu', 3); k = min(k1, k2)        # k = 2m + e for each, then min
    a = primary_H(p1, q1, o); b = primary_H(p2, q2, o)
    b, steps = h_loop(a, b, na, nb, o, check)
    # back to a pair: (R, S) = (1+i) u; gcd = (1+i)^(k-1) (R + iS); k = 0: one division by (1+i)
    o('sh', 2); o('alu', 2); Rr, Ss = 2 * b[0] + 1, 2 * b[1] + 1
    o('alu'); o('br')
    if k == 0: o('alu', 2); o('sh', 2); return ((Rr + Ss) >> 1, (Ss - Rr) >> 1), steps
    k -= 1
    o('sh', 3); o('alu'); o('br'); g = (Rr << (k >> 1), Ss << (k >> 1))
    if k & 1: o('alu', 2); g = (g[0] - g[1], g[0] + g[1])
    return g, steps

# ---------------- verification ----------------
def run_variant(name, x, y, n, o, check=False):
    if name == 'A': return R32.gcd_A(x, y, o)
    if name == 'B32': g, s = R32.gcd_B(R32.mkB(x, n), R32.mkB(y, n), n, o); return R32.valB(g, n), s
    if name == 'BH': g, s = gcd_BH(R32.mkB(x, n), R32.mkB(y, n), n, o, check); return R32.valB(g, n), s
    if name == 'AH': return gcd_AH(x, y, o, check)
VARS = ['A', 'B32', 'BH', 'AH']

def sample(rnd, bits):
    h = 1 << (bits - 1); r = rnd.random()
    x = (rnd.randrange(-h, h), rnd.randrange(-h, h)); y = (rnd.randrange(-h, h), rnd.randrange(-h, h))
    if r < 0.15:                          # built-in common factor
        c = (rnd.randrange(-2**12, 2**12), rnd.randrange(-2**12, 2**12)); s = 1 << (bits - 15)
        x = gmul(c, (rnd.randrange(-s, s), rnd.randrange(-s, s))); y = gmul(c, (rnd.randrange(-s, s), rnd.randrange(-s, s)))
    elif r < 0.25:                        # common power of (1+i)
        j = rnd.randrange(1, 12); s = 1 << (bits - 1 - (j + 1) // 2)
        x = S31.pipow((rnd.randrange(-s, s), rnd.randrange(-s, s)), j); y = S31.pipow((rnd.randrange(-s, s), rnd.randrange(-s, s)), j)
    return x, y

def verify(variants=VARS, extra=()):
    print("verification against the big-integer Euclid gcd, normalised to the associate with re > 0, im >= 0")
    n = 8; R = 7; box = [(a, b) for a in range(-R, R + 1) for b in range(-R, R + 1)]
    for v in list(variants) + list(extra):
        f = v if callable(v) else None
        for x in box:
            for y in box:
                g = assoc_norm(R32.ref_gcd(x, y))
                got = f(x, y, n, Ops())[0] if f else run_variant(v, x, y, n, Ops(), True)[0]
                assert assoc_norm(got) == g, (v, x, y, got, g)
    print("  exhaustive box |a|,|b| <= %d (%d pairs, B with %d digits): %s exact" % (R, len(box) ** 2, n,
          ", ".join(v if isinstance(v, str) else v.__name__ for v in list(variants) + list(extra))))
    rnd = random.Random(33); n = 32; NR = 4000
    for _ in range(NR):
        x, y = sample(rnd, 31)
        g = assoc_norm(R32.ref_gcd(x, y))
        for v in list(variants) + list(extra):
            got = v(x, y, n, Ops())[0] if callable(v) else run_variant(v, x, y, n, Ops(), True)[0]
            assert assoc_norm(got) == g, (v, x, y)
    print("  %d random 31-bit pairs (15%% with a common factor, 10%% with a common power of 1+i): exact" % NR)

def count(variants=VARS, NR=3000, seed=9, extra=()):
    rnd = random.Random(seed); n = 32; h = 1 << 30
    tot = {v: Ops() for v in variants}; st = {v: [] for v in variants}
    for _ in range(NR):
        x = (rnd.randrange(-h, h), rnd.randrange(-h, h)); y = (rnd.randrange(-h, h), rnd.randrange(-h, h))
        for v in variants:
            o = Ops(); _, s = run_variant(v, x, y, n, o); tot[v].update(o); st[v].append(s)
    print("ops per gcd, %d random 31-bit pairs (same pairs for every variant)" % NR)
    print("| variant | steps mean | steps max | ops per gcd | mul | ops per step (loop) |")
    print("|---|---|---|---|---|---|")
    res = {}
    for v in variants:
        T = tot[v]; ms = sum(st[v]) / NR
        print("| %s | %.2f | %d | %.0f | %.0f | %.1f |" % (v, ms, max(st[v]), T.tot() / NR, T['mul'] / NR, T.tot() / NR / ms))
        res[v] = (ms, T.tot() / NR)
    return res

def loop_breakdown():
    # one step of the H loop, by part
    parts = [("a - b", Ops(alu=2)), ("zero test (folded loop test)", Ops(alu=1, br=1))]
    o = Ops(); strip_to_half(12, 40, o); parts.append(("strip to H, with norm", o))
    o = Ops(); primary_H(3, 5, o); parts.append(("primary in H", o))
    parts.append(("compare and conditional swap (3 words)", Ops(alu=14)))
    print("H loop step: " + ", ".join("%s %d" % (nm, sum(c.values())) for nm, c in parts) +
          "; total %d" % sum(sum(c.values()) for _, c in parts))

if __name__ == "__main__":
    what = sys.argv[1] if len(sys.argv) > 1 else "all"
    if what in ("verify", "all"): verify()
    if what in ("count", "all"): loop_breakdown(); count()

# ---------------- choice rules: exact simulation of step counts (uncounted) ----------------
UNITS = [(1, 0), (0, 1), (-1, 0), (0, -1)]
def oddpart(z):
    t = 0
    while ((z[0] ^ z[1]) & 1) == 0: z = ((z[0] + z[1]) >> 1, (z[1] - z[0]) >> 1); t += 1
    return z, t
def sub_qb(a, q, b): qb = gmul(q, b); return (a[0] - qb[0], a[1] - qb[1])
def eps_of(a, b):                       # the unit with a = eps b mod (1+i)^3
    for e in UNITS:
        d = sub_qb(a, e, b)
        if d == (0, 0) or oddpart(d)[1] >= 3: return e
def round_div(z, mu):                   # nearest Gaussian integer to z / mu (ties down)
    p = gmul(z, (mu[0], -mu[1])); N = norm(mu)
    return ((2 * p[0] + N) // (2 * N), (2 * p[1] + N) // (2 * N))
PI = (1, 1)
def pipow_(m): return S31.pipow((1, 0), m)
def hensel_q(a, b, m):                  # q = a / b mod (1+i)^m, smallest representative (nearest-rounding reduction)
    mu = pipow_(m); Nb = norm(b); mod = 1 << (m // 2 + 2)
    inv = pow(Nb, -1, mod)
    q = gmul(a, (b[0] * inv, -b[1] * inv))
    r = round_div(q, mu); qm = gmul(r, mu)
    return (q[0] - qm[0], q[1] - qm[1])

def sim(x, y, rule, cap=400):
    """steps of the (1+i)-ary gcd on odd parts, with a choice rule; returns (odd gcd, steps, shed list)."""
    a, _ = oddpart(x); b, _ = oddpart(y)
    if norm(a) < norm(b): a, b = b, a
    steps = 0; shed = []
    while True:
        steps += 1
        if steps > cap: raise RuntimeError("no termination")
        cands = rule(a, b)
        best = None
        for q in cands:
            d = sub_qb(a, q, b)
            if d == (0, 0): return b, steps, shed
            z, t = oddpart(d)
            if best is None or norm(z) < best[0]: best = (norm(z), z, t)
        _, z, t = best; shed.append(t)
        a = z
        if norm(a) < norm(b): a, b = b, a

def r_base(a, b): return [eps_of(a, b)]
def r_best2(a, b): e = eps_of(a, b); return [e, (-e[0], -e[1])]
def r_best4(a, b): return UNITS
def r_hensel(c):
    def f(a, b):
        L = norm(a).bit_length() - norm(b).bit_length()
        m = max(3, L + c)
        return [hensel_q(a, b, m)]
    f.__name__ = "hensel%+d" % c; return f
def r_hensel_best(a, b):                 # oracle over a few m
    L = norm(a).bit_length() - norm(b).bit_length()
    return [eps_of(a, b)] + [hensel_q(a, b, m) for m in range(max(4, L - 2), L + 4)]

def choice(NR=2000, seed=9):
    rnd = random.Random(seed); h = 1 << 30
    pairs = [((rnd.randrange(-h, h), rnd.randrange(-h, h)), (rnd.randrange(-h, h), rnd.randrange(-h, h))) for _ in range(NR)]
    # idea 1 as stated: the most-shedding unit combination is always eps (t >= 3; a + eps b sheds exactly 2, a +- i eps b exactly 1)
    tcount = Counter()
    for x, y in pairs[:300]:
        a, _ = oddpart(x); b, _ = oddpart(y)
        e = eps_of(a, b)
        for u in UNITS:
            t = oddpart(sub_qb(a, u, b))[1]
            rel = gmul(u, (e[0], -e[1]))          # u / eps
            tcount[(rel, min(t, 3))] += 1
    print("shed t by combination a - u b, u = eps * (1, i, -1, -i), 300 random pairs:")
    print("  " + ", ".join("u/eps=%s: t%s" % (r, sorted(set(('>=3' if t == 3 else str(t)) for (rr, t) in tcount if rr == r))) for r in UNITS))
    rules = [("eps (study 9 / 32)", r_base), ("best of a - eps b, a + eps b (by result norm)", r_best2),
             ("best of all four units (oracle)", r_best4)] + \
            [("Hensel quotient, m = max(3, L %+d)" % c, r_hensel(c)) for c in (-1, 0, 1, 2)] + \
            [("Hensel, best m (oracle)", r_hensel_best)]
    print("\nsteps per gcd (odd parts), %d random 31-bit pairs; L = bit length of N(a) minus that of N(b)" % NR)
    print("| rule | steps mean | steps max | mean shed t |")
    print("|---|---|---|---|")
    out = {}
    for name, rule in rules:
        S = []; T = []
        for x, y in pairs:
            g, s, sh = sim(x, y, rule); S.append(s); T += sh
            assert assoc_norm(g) == assoc_norm(oddpart(R32.ref_gcd(x, y))[0])
        print("| %s | %.2f | %d | %.2f |" % (name, sum(S) / NR, max(S), sum(T) / max(1, len(T))))
        out[name] = sum(S) / NR
    return out

# ---------------- Hensel-quotient gcd (pi-adic quotient from the low bits), counted ----------------
# Values are kept as Z = (R, S) = pair coords of (1+i)u, u odd (R, S odd); nz = R^2 + S^2 = 2N(u).
# Step (N(a) >= N(b)): j = max(2, (L + c) >> 1), L = bitlen(na) - bitlen(nb);
#   q = a / b mod 2^j, centred coords in [-2^(j-1), 2^(j-1)): q = ((Za conj(Zb)) >> 1) * N(b)^-1 mod 2^j, sign-extended;
#   Za - q Zb = (1+i)(a - q b), divisible by (1+i)^(2j+1); strip it back to the (R, S) form; replace a.
# No primary is needed: q carries the unit. Everything mod 2^j is wrapping 64-bit arithmetic.
M64 = (1 << 64) - 1
def s64(v): v &= M64; return v - (1 << 64) if v >> 63 else v
INV8 = {n: pow(n, -1, 256) for n in range(1, 256, 2)}
def inv_mod64(N, o):                   # N odd: byte table (128 entries) + 3 Newton rounds (8 -> 16 -> 32 -> 64 bits): 12 ops
    o('alu'); o('ld'); x = INV8[N & 255]
    for _ in range(3): o('mul', 2); o('alu'); x = (x * (2 - N * x)) & M64
    return x
def strip_to_Z(x, y, o):               # nonzero pair -> (R, S) of (1+i) * odd part, and R^2 + S^2: 11 + 3 ops
    o('alu'); o('ctz'); oo = x | y; m = ctz(oo)
    o('sh', 2); x >>= m; y >>= m
    o('alu', 3); e = x & y & 1; nm = e - 1
    o('alu', 4); Rr = x - (y & nm); Ss = y + (x & nm)
    o('mul', 2); o('alu'); return Rr, Ss, 2 * m + e, Rr * Rr + Ss * Ss
def bitlen(v, o): o('ctz'); return v.bit_length()        # 64 - lzcnt (lzcnt counted like ctz; the constant folds)
HC = 6
def z_loop(a, b, na, nb, o, c=HC, check=False):
    o('alu', 2); R32.cswap_ops(o, 3)
    if na < nb: a, b, na, nb = b, a, nb, na
    steps = 0
    while True:
        o('alu'); L = bitlen(na, o) - bitlen(nb, o)
        o('alu', 3); o('sh'); j = max(2, (L + c) >> 1)                 # add, shift, max (cmp + cmov counted 2); j >= 2 guarantees descent
        o('sh'); inv = inv_mod64(nb >> 1, o)                           # N(b) = nb / 2, odd
        o('mul', 4); o('alu', 2); w = (a[0] * b[0] + a[1] * b[1], a[1] * b[0] - a[0] * b[1])   # Za conj(Zb) = 2 a conj(b)
        o('sh', 2); o('mul', 2); w0 = s64((w[0] >> 1) * inv); w1 = s64((w[1] >> 1) * inv)
        o('alu'); o('sh', 4); sft = 64 - j; q = (s64(w0 << sft) >> sft, s64(w1 << sft) >> sft)   # centred mod 2^j
        o('mul', 4); o('alu', 2); qb = (q[0] * b[0] - q[1] * b[1], q[0] * b[1] + q[1] * b[0])
        o('alu', 2); dx, dy = a[0] - qb[0], a[1] - qb[1]
        if check: assert max(abs(dx), abs(dy)) < 1 << 62 and (dx | dy) & ((1 << j) - 1) == 0
        o('alu'); o('br'); steps += 1
        if dx | dy == 0: return b, steps
        Rr, Ss, _, nz = strip_to_Z(dx, dy, o)
        a, na = (Rr, Ss), nz
        o('alu', 2); R32.cswap_ops(o, 3)
        if na < nb: a, b, na, nb = b, a, nb, na
        if steps > 200: raise RuntimeError("no termination")

def b_to_Z(A, B, n, o):                # (R, S) = (2 Rp - M, 2 Sp - M), Sp = ~A, Rp = B ^ Sp: 6 ops
    Mo = (1 << n) - 1
    o('alu', 2); Sp = Mo ^ A; Rp = B ^ Sp
    o('sh', 2); o('alu', 2); return 2 * Rp - Mo, 2 * Sp - Mo
def Z_to_b(Rr, Ss, n, o):              # 6 ops
    Mo = (1 << n) - 1
    o('alu', 2); o('sh', 2); Rp = (Rr + Mo) >> 1; Sp = (Ss + Mo) >> 1
    o('alu', 2); return Mo ^ Sp, Rp ^ Sp
def gcd_BZ(x, y, n, o, c=HC, check=False):
    o('alu'); o('br')
    if x[0] >= INF or y[0] >= INF: return (x if y[0] >= INF else y), 0
    o('alu', 3); k = min(x[0], y[0])
    a = b_to_Z(x[1], x[2], n, o); b = b_to_Z(y[1], y[2], n, o)
    o('mul', 4); o('alu', 2); na = a[0] ** 2 + a[1] ** 2; nb = b[0] ** 2 + b[1] ** 2
    b, steps = z_loop(a, b, na, nb, o, c, check)
    A, B = Z_to_b(*b, n, o)
    return (k, A, B), steps
def gcd_AZ(x, y, o, c=HC, check=False):
    o('alu', 4); o('br')
    if x == (0, 0) or y == (0, 0): return x if y == (0, 0) else y, 0
    R1, S1, k1, na = strip_to_Z(*x, o); R2, S2, k2, nb = strip_to_Z(*y, o)
    o('alu', 3); k = min(k1, k2)          # k = 2m + e: the 2m is a shift-add folded into the strip's ops (counted here)
    b, steps = z_loop((R1, S1), (R2, S2), na, nb, o, c, check)
    Rr, Ss = b
    o('alu'); o('br')
    if k == 0: o('alu', 2); o('sh', 2); return ((Rr + Ss) >> 1, (Ss - Rr) >> 1), steps
    k -= 1
    o('sh', 3); o('alu'); o('br'); g = (Rr << (k >> 1), Ss << (k >> 1))
    if k & 1: o('alu', 2); g = (g[0] - g[1], g[0] + g[1])
    return g, steps

def run_variant2(name, x, y, n, o, check=False):
    if name == 'BZ': g, s = gcd_BZ(R32.mkB(x, n), R32.mkB(y, n), n, o, check=check); return R32.valB(g, n), s
    if name == 'AZ': return gcd_AZ(x, y, o, check=check)
    return run_variant(name, x, y, n, o, check)
_rv = run_variant
def run_variant(name, x, y, n, o, check=False):
    if name in ('BZ', 'AZ'): return run_variant2(name, x, y, n, o, check)
    return _rv(name, x, y, n, o, check)
VARS += ['BZ', 'AZ']

if __name__ == "__main__":
    what = sys.argv[1] if len(sys.argv) > 1 else "all"
    if what in ("choice", "all"): choice()
    if what in ("hensel", "all"):
        verify(['BZ', 'AZ'])
        count(['A', 'BH', 'BZ', 'AZ'])
