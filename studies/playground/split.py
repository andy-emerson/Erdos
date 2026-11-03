# Study 5: the mod p split.
# For a prime p = 1 (mod 4) there is s with s^2 = -1 (mod p), and Z[i]/p is isomorphic to F_p x F_p by
#   phi(a + bi) = (a + bs, a - bs) mod p,      inverse  a = (u + v)/2,  b = (u - v)/(2s)  mod p.
# In the split form add and multiply are componentwise: a product is 2 products of residues instead of 4.
# Words: a residue pair in one 64-bit word, first component (or real part) in the low 32 bits. p < 2^31.
# Reductions use Barrett as in study 4 (7 instructions, 2 multiplies). All operations are written with word operations.
# Usage: python3 split.py            (exhaustive for small p, random for p near 2^31)
import random, itertools
W = 64; MW = (1 << W) - 1; ML = (1 << 32) - 1
def pack(a, b): return (b << 32) | a
def lanes(w): return w & ML, w >> 32
mulhi = lambda x, y: (x * y) >> W
sarm = lambda v: MW if (v >> 63) & 1 else 0                 # v sar 63

class Mod:
    def __init__(s_, p):
        s_.p = p; s_.m = (1 << W) // p
        c = 2
        while pow(c, (p - 1) // 2, p) != p - 1: c += 1       # a non-residue; s = c^((p-1)/4) has s^2 = c^((p-1)/2) = -1
        s_.s = pow(c, (p - 1) // 4, p)
        s_.inv2s = pow(2 * s_.s, -1, p); s_.p2 = p * p
    def red(s_, t):                     # Barrett, t < 2^63: 7 (MULHI, MUL, SUB, SUB, SAR, AND, ADD)
        p = s_.p; r = (t - mulhi(t, s_.m) * p) & MW; r = (r - p) & MW
        return (r + (p & sarm(r))) & MW
    def addm(s_, x, y):                 # x + y mod p for residues: ADD, SUB, SAR, AND, ADD = 5
        r = (x + y - s_.p) & MW; return (r + (s_.p & sarm(r))) & MW
    def subm(s_, x, y):                 # x - y mod p: SUB, SAR, AND, ADD = 4
        r = (x - y) & MW; return (r + (s_.p & sarm(r))) & MW
    def half(s_, x):                    # x / 2 mod p: add p when x is odd, shift: AND, NEG, AND, ADD, SHR = 5
        return (x + (s_.p & ((0 - (x & 1)) & MW))) >> 1
    # forward map: extract 2, b s: MUL + reduce 8, a + bs: 5, a - bs: 4, pack 2 = 21
    def fwd(s_, z):
        a, b = lanes(z); bs = s_.red(b * s_.s)
        return pack(s_.addm(a, bs), s_.subm(a, bs))
    # inverse map: extract 2, (u + v)/2: 5 + 5, (u - v)/(2s): 4 + MUL + reduce 8, pack 2 = 26
    def inv(s_, w):
        u, v = lanes(w)
        return pack(s_.half(s_.addm(u, v)), s_.red(s_.subm(u, v) * s_.inv2s))
    # split form: componentwise multiply: extract 4, 2 MUL, 2 reductions 14, pack 2 = 22 (6 multiplies)
    def mul_split(s_, x, y):
        (u1, v1), (u2, v2) = lanes(x), lanes(y)
        return pack(s_.red(u1 * u2), s_.red(v1 * v2))
    # split form: componentwise add: extract 4, 2 x 5, pack 2 = 16 (the same as the direct add, lane for lane)
    def add_split(s_, x, y):
        (u1, v1), (u2, v2) = lanes(x), lanes(y)
        return pack(s_.addm(u1, u2), s_.addm(v1, v2))
    # direct Gaussian product mod p (study 4): extract 4, 4 MUL, 3 add/sub, 2 reductions 14, pack 2 = 27 (8 multiplies)
    def mul_direct(s_, x, y):
        (a, b), (c, d) = lanes(x), lanes(y)
        return pack(s_.red(a * c + (s_.p2 - b * d)), s_.red(a * d + b * c))
    def conj_split(s_, w):              # conjugation swaps the components: one rotate by 32
        return ((w >> 32) | (w << 32)) & MW
    def norm_split(s_, w):              # a^2 + b^2 = (a + bs)(a - bs) mod p: extract 2, MUL, reduce 7 = 10
        u, v = lanes(w); return s_.red(u * v)

def isprime(n):
    if n < 2: return False
    for q in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        if n % q == 0: return n == q
    d, r = n - 1, 0
    while d % 2 == 0: d //= 2; r += 1
    for a in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        x = pow(a, d, n)
        if x in (1, n - 1): continue
        for _ in range(r - 1):
            x = x * x % n
            if x == n - 1: break
        else: return False
    return True

st = {}
def chk(k, ok):
    st[k + ' cases'] = st.get(k + ' cases', 0) + 1
    if not ok: st[k + ' FAIL'] = st.get(k + ' FAIL', 0) + 1
def run(p, zs, pairs):
    M = Mod(p); g = lambda a, b: pack(a % p, b % p)
    assert M.s * M.s % p == p - 1
    for a, b in zs:
        z = g(a, b); w = M.fwd(z)
        chk('fwd matches (a+bs, a-bs)', lanes(w) == ((a + b * M.s) % p, (a - b * M.s) % p))
        chk('inverse(fwd(z)) = z', M.inv(w) == z)
        chk('conj = swap', M.fwd(g(a, -b)) == M.conj_split(w))
        chk('norm = u v', M.norm_split(w) == (a * a + b * b) % p)
    for (a, b), (c, d) in pairs:
        x, y = g(a, b), g(c, d)
        chk('phi(x+y) = phi(x)+phi(y)', M.fwd(g(a + c, b + d)) == M.add_split(M.fwd(x), M.fwd(y)))
        chk('phi(xy) = phi(x)phi(y)', M.fwd(g(a*c - b*d, a*d + b*c)) == M.mul_split(M.fwd(x), M.fwd(y)))
        chk('direct product correct', M.mul_direct(x, y) == g(a*c - b*d, a*d + b*c))
        chk('inv(split product) = direct product', M.inv(M.mul_split(M.fwd(x), M.fwd(y))) == M.mul_direct(x, y))
    chk('phi(1) = (1, 1)', M.fwd(pack(1, 0)) == pack(1, 1))
    return M

small = [p for p in range(3, 40) if isprime(p) and p % 4 == 1]        # 5, 13, 17, 29, 37
for p in small:
    zs = list(itertools.product(range(p), repeat=2))
    images = {Mod(p).fwd(pack(a, b)) for a, b in zs}
    chk('bijective (p^2 distinct images)', len(images) == p * p)
    run(p, zs, list(itertools.product(zs, repeat=2)))
print("small p (exhaustive: all p^2 elements, all p^4 pairs):", small)
random.seed(5)
big = [p for p in range((1 << 31) - 1, (1 << 31) - 2000, -1) if isprime(p) and p % 4 == 1][:4]
for p in big:
    edge = [0, 1, p - 1, p // 2]
    zs = [(random.randrange(p), random.randrange(p)) for _ in range(20000)] + [(a, b) for a in edge for b in edge]
    run(p, zs, [(random.choice(zs), random.choice(zs)) for _ in range(20000)])
print("large p (random):", big)
for k, v in sorted(st.items()): print(f"  {k}: {v}")
