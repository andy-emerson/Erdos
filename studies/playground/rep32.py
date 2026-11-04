# Study 32: three representations of an exact Gaussian integer, compared in one word-operation model.
#   A: ordinary pair (a, b), two's complement, m bits each (m = 32 in the tables).
#   B: x = (1+i)^k u, u odd in unit digits (adic31.py), split convention x = 2^(k>>1) (1+i)^(k&1) w, w = i^(k>>1) u.
#   C: base beta = -1+i, digits 0/1, one word of n = 2m digits (Penney; Knuth 4.1).
# Op model (Ops counter): alu (add sub and or xor not neg, compare to mask), sh (shift), mul, ctz, br (compare and branch,
# a jump table counts 1), ld (table load), div (integer divide, kept apart). Constants are free (in registers).
# Totals count every op as 1. These are estimates in the model, not timings.
# Usage: python3 rep32.py [c|table|conv|gcd|hyb|all]
import sys, random
from collections import Counter
sys.path.insert(0, __file__.rsplit('/', 1)[0] if '/' in __file__ else '.')
import adic31 as S31

class Ops(Counter):
    def __call__(self, kind, n=1): self[kind] += n
    def tot(self): return sum(self.values())

# ---------------- exact reference (Python big ints) ----------------
def gmul(p, q): return (p[0]*q[0] - p[1]*q[1], p[0]*q[1] + p[1]*q[0])
def norm(z): return z[0]*z[0] + z[1]*z[1]

# ---------------- C: base -1+i ----------------
def c_value(w, n):                     # reference: sum of d_j beta^j
    re = im = 0; pr, pi = 1, 0
    for j in range(n):
        if (w >> j) & 1: re += pr; im += pi
        pr, pi = -pr - pi, pr - pi
    return re, im
def c_encode_ref(a, b):                # reference digit-serial expansion (exact, any size): returns (w, length)
    w = j = 0
    while (a, b) != (0, 0):
        if (a + b) & 1: w |= 1 << j; a -= 1
        a, b = (b - a) // 2, (-a - b) // 2       # divide by beta exactly
        j += 1
    return w, j
def rep(p, period, n):                 # repeat pattern p of 'period' bits over n bits
    r = 0
    for t in range(0, n, period): r |= p << t
    return r & ((1 << n) - 1)
class CM:                              # masks for width m (word n = 2m digits)
    def __init__(self, m):
        self.m = m; self.n = n = 2*m; self.M = (1 << m) - 1; self.N = (1 << n) - 1
        self.q = [rep(1 << r, 4, m) for r in range(4)]          # positions t = r mod 4 in an m-bit word
        self.stages = []; s = 1
        while 4*s <= n:                                         # delta-swap masks for the outer unshuffle
            self.stages.append((s, rep(((1 << s) - 1) << s, 4*s, n))); s *= 2
        q = self.q
        self.nreE = q[2]; self.nreO = q[0] | q[3]; self.nimE = q[1]; self.nimO = q[2] | q[3]
        self.mE_re = q[0] | q[2]; self.mE_im = q[1] | q[3]
def unshuffle(w, cm, o):               # even digits -> low m bits (E), odd digits -> high m bits (O)
    for s, mk in cm.stages:
        o('sh', 2); o('alu', 4); t = (w ^ (w >> s)) & mk; w = w ^ t ^ (t << s)
    return w
def shuffle(w, cm, o):
    for s, mk in reversed(cm.stages):
        o('sh', 2); o('alu', 4); t = (w ^ (w >> s)) & mk; w = w ^ t ^ (t << s)
    return w
def c2a(w, cm, o):                     # C -> A, exact (results need m+1 bits signed): 30 + 2 + 12 at m = 32
    w = unshuffle(w, cm, o)
    o('alu'); o('sh'); E = w & cm.M; O = w >> cm.m
    # twisted base 2: position t of E/O has weight +-2^t; signs by t mod 4 (offset trick (x ^ neg) - neg)
    o('alu', 3); vE = ((E & cm.mE_re) ^ cm.nreE) - cm.nreE
    o('alu', 2); vO = (O ^ cm.nreO) - cm.nreO
    o('alu', 3); uE = ((E & cm.mE_im) ^ cm.nimE) - cm.nimE
    o('alu', 2); uO = (O ^ cm.nimO) - cm.nimO
    o('alu', 2); return vE + vO, uE + uO
def a2c(a, b, cm, o, check=False):     # A -> C, mod 2^m (wraps); Gauss-Seidel fixpoint on the two coordinates
    q = cm.q; M = cm.M
    o('alu'); Ba = b & M; rounds = 0
    while True:
        o('alu', 4); Na = q[3] | (Ba & q[0]) | (~Ba & q[2])      # a's negative positions: t=3, t=0 if gamma, t=2 if not
        o('alu', 3); Aa = ((a + Na) ^ Na) & M
        o('alu', 4); Nb = q[2] | (~Aa & q[1]) | (Aa & q[3])      # b's negative positions: t=2, t=1 if not alpha, t=3 if alpha
        o('alu', 3); nB = ((b + Nb) ^ Nb) & M
        o('br'); rounds += 1
        if nB == Ba: break
        Ba = nB
    o('alu'); E = Aa ^ Ba
    o('alu', 3); O = (Ba & (q[0] | q[2])) | (Aa & (q[1] | q[3]))
    o('alu'); o('sh'); w = shuffle(E | (O << cm.m), cm, o)
    ok = True
    if check:                          # exact iff no bits above m: (a + Na) >> m == 0 and (b + Nb) >> m == 0
        o('alu', 3); o('sh', 2); ok = ((a + Na) >> cm.m) == 0 and ((b + Nb) >> cm.m) == 0
    return (w, ok) if check else w

def c2a_table_ops(cm):                 # byte-table C -> A: per byte shift, and, 2 loads, 2 shifts, 2 adds
    return Ops(sh=3*(cm.n//8), alu=3*(cm.n//8), ld=2*(cm.n//8))
BT = {}
def a2c_table(a, b, cm, o):            # A -> C by base 16 = beta^8: 8 rounds, 256-entry table on (a mod 16, b mod 16)
    if not BT:
        for byte in range(256):
            re, im = c_value(byte, 8); BT[(re & 15) | ((im & 15) << 4)] = (byte, re, im)
    w = 0
    for s in range(cm.n // 8):
        o('alu', 3); o('sh'); idx = (a & 15) | ((b & 15) << 4)
        o('ld', 3); byte, re, im = BT[idx]
        o('sh'); o('alu'); w |= byte << (8*s)
        o('alu', 2); o('sh', 2); a = (a - re) >> 4; b = (b - im) >> 4
    return w, (a, b) == (0, 0)         # remainder (0, 0) iff exact (the check is free: one or + test)

def c_add(x, y, cm, o):                # convert, add, convert back
    a1, b1 = c2a(x, cm, o); a2, b2 = c2a(y, cm, o)
    o('alu', 2); return a2c(a1 + a2, b1 + b2, cm, o)
def c_add_cs(x, y, cm, o):             # direct carry rule 1 + 1 = 1100 in three words, mod beta^n
    z = 0
    while True:
        o('alu', 5); t = x ^ y; s = t ^ z; c = (x & y) | (z & t)
        o('br')
        if c == 0: return s
        o('sh', 2); x, y, z = s, (c << 2) & cm.N, (c << 3) & cm.N

def check_c():
    print("C: base -1+i")
    for m in (2, 4, 8):
        cm = CM(m); n = cm.n; mod = 1 << m
        vals = {}
        for w in range(1 << n):
            v = c_value(w, n); o = Ops()
            assert c2a(w, cm, o) == v
            assert c_encode_ref(*v)[0] == w and c_encode_ref(*v)[1] <= n
            ww, ok = a2c(v[0], v[1], cm, Ops(), True); assert ww == w and ok
            assert a2c_table(v[0], v[1], cm, Ops()) == (w, True) if n % 8 == 0 else True
            vals[v] = w
        assert len(vals) == 1 << n
        # every residue mod 2^m is hit exactly once: C_n is a complete residue system mod beta^n = 2^m (up to unit)
        res = {(v[0] % mod, v[1] % mod) for v in vals}; assert len(res) == 1 << n
        # out-of-range points wrap and are flagged
        R = 3 * (1 << m) if m <= 4 else 40
        for a in range(-R, R + 1):
            for b in range(-R, R + 1):
                ww, ok = a2c(a, b, cm, Ops(), True)
                assert ok == ((a, b) in vals)
                v = c_value(ww, n); assert (v[0] - a) % mod == 0 and (v[1] - b) % mod == 0
        print("  m=%d (n=%d digits): %d words, values distinct, c2a/a2c exact, a2c wraps mod 2^m and flags overflow" % (m, n, 1 << n))
    # adders, exhaustive at m = 4: mod beta^8 = 16
    cm = CM(4); n = 8
    for x in range(256):
        for y in range(256):
            X, Y = c_value(x, n), c_value(y, n)
            s = c_add(x, y, cm, Ops()); s2 = c_add_cs(x, y, cm, Ops())
            S = c_value(s, n)
            assert s == s2 and ((S[0] - X[0] - Y[0]) % 16, (S[1] - X[1] - Y[1]) % 16) == (0, 0)
    print("  m=4: both adders exhaustive (65,536 pairs) agree and are exact mod 16")
    cm = CM(32); random.seed(7)
    stats = Counter(); rr = Counter(); nn = 20000
    for _ in range(nn):
        a, b, c, d = (random.randrange(-2**28, 2**28) for _ in range(4))
        x = c_encode_ref(a, b)[0]; y = c_encode_ref(c, d)[0]
        o1 = Ops(); assert c2a(x, cm, o1) == (a, b)
        o2 = Ops(); w, ok = a2c(a, b, cm, o2, True); assert w == x and ok
        o3 = Ops(); w3, ok3 = a2c_table(a, b, cm, o3); assert w3 == x and ok3
        o4 = Ops(); s = c_add(x, y, cm, o4); assert c_value(s, 64) == (a + c, b + d)
        o5 = Ops(); s5 = c_add_cs(x, y, cm, o5); assert s5 == s
        stats['c2a'] += o1.tot(); stats['a2c'] += o2.tot(); stats['a2c_tab'] += o3.tot()
        stats['add'] += o4.tot(); stats['add_cs'] += o5.tot(); rr[o2['br']] += 1
    print("  m=32 random (|a|,|b| < 2^28, %d cases): exact" % nn)
    for k in stats: print("    %-8s %6.1f ops avg" % (k, stats[k] / nn))
    print("    a2c rounds:", " ".join("%d:%.3f" % (k, rr[k] / nn) for k in sorted(rr)))
    # worst-case rounds, exhaustive at small m
    for m in (4, 6, 8):
        cm = CM(m); mx = 0
        for a in range(1 << m):
            for b in range(1 << m):
                o = Ops(); a2c(a, b, cm, o); mx = max(mx, o['br'])
        print("    a2c worst rounds, all residues, m=%d: %d" % (m, mx))


# ---------------- representations as states; reference converters (uncounted) ----------------
INF = S31.INF
def mkA(x): return x
def valA(s): return s
def mkB(x, n): return S31.state('ud', x, n)
def valB(s, n): return S31.value('ud', s, n)
def mkC(x, cm):
    w, L = c_encode_ref(*x); assert L <= cm.n, "value outside the C range"; return w
def valC(w, cm): return c_value(w, cm.n)

# ---------------- A: ordinary pair ----------------
def rotA(a, b, r, o): return S31.rot_branchy(a, b, r, o)          # (a + bi) i^r, jump table: br + <= 2 alu
def A_add(x, y, o): o('alu', 2); return (x[0] + y[0], x[1] + y[1])
def A_sub(x, y, o): o('alu', 2); return (x[0] - y[0], x[1] - y[1])
def A_neg(x, o): o('alu', 2); return (-x[0], -x[1])
def A_conj(x, o): o('alu'); return (x[0], -x[1])
def A_muli(x, o): o('alu'); return (-x[1], x[0])                  # the swap is free (register renaming)
def A_mul1i(x, o): o('alu', 2); return (x[0] - x[1], x[0] + x[1])
def A_mul(x, y, o): o('mul', 4); o('alu', 2); return gmul(x, y)
def A_norm(x, o): o('mul', 2); o('alu'); return norm(x)
def A_eq(x, y, o): o('alu', 3); return x == y
def A_iszero(x, o): o('alu', 2); return x == (0, 0)
def A_div1i(x, o): o('alu', 2); return ((x[0] ^ x[1]) & 1) == 0
def A_div2(x, o): o('alu', 2); return ((x[0] | x[1]) & 1) == 0
def A_exdiv_pij(x, j, o):                                         # x / (1+i)^j, exact; (1+i)^2 = 2i
    a, b = x; o('sh'); h = j >> 1
    o('alu'); o('br')
    if j & 1: o('alu', 2); o('sh', 2); a, b = (a + b) >> 1, (b - a) >> 1
    o('sh', 2); a >>= h; b >>= h
    o('alu'); return rotA(a, b, -h, o)
M64 = (1 << 64) - 1
def s64(v): v &= M64; return v - (1 << 64) if v >> 63 else v
def inv2adic(n, o):                                               # n odd: inverse mod 2^64 by Newton (3 -> 6 -> ... -> 96 bits)
    inv = n
    for _ in range(5): o('mul', 2); o('alu'); inv = (inv * (2 - n * inv)) & M64
    return inv
def A_exdiv(x, y, o):                                             # x / y known exact: x conj(y) / N(y), Hensel division
    o('mul', 4); o('alu', 2); p = (x[0]*y[0] + x[1]*y[1], x[1]*y[0] - x[0]*y[1])
    N = A_norm(y, o)
    o('ctz'); o('sh'); s = (N & -N).bit_length() - 1; n = N >> s
    inv = inv2adic(n, o)
    o('sh', 2); o('mul', 2); return (s64((p[0] >> s) * inv), s64((p[1] >> s) * inv))
def rdiv(n, N, o):                                                # nearest n / N (N > 0), ties toward zero; 2N, N - 1 given
    o('sh'); o('alu', 2); sg = -1 if n < 0 else 0; ab = (n ^ sg) - sg
    o('sh'); o('alu'); o('div'); q = (2*ab + N - 1) // (2*N)
    o('alu', 2); return (q ^ sg) - sg
def A_divrem(x, y, o):
    o('mul', 4); o('alu', 2); p = (x[0]*y[0] + x[1]*y[1], x[1]*y[0] - x[0]*y[1])
    N = A_norm(y, o); o('alu'); o('sh')                           # N - 1 and 2N, once
    q = (rdiv(p[0], N, o), rdiv(p[1], N, o))
    o('mul', 4); o('alu', 4); r = (x[0] - (q[0]*y[0] - q[1]*y[1]), x[1] - (q[0]*y[1] + q[1]*y[0]))
    return q, r

# ---------------- conversions A <-> B (split unit digits, n digits) ----------------
def b2a(s, n, o):
    k, A, B = s
    o('br')
    if k >= INF: return (0, 0)
    o('alu', 4); a, b = S31.ud_decode(A, B, n)
    o('sh'); o('alu'); h = k >> 1; e = k & 1
    o('br')
    if e: o('alu', 2); a, b = a - b, a + b
    o('sh', 2); return (a << h, b << h)
def a2b(x, n, o):
    a, b = x
    o('alu'); o('br'); oo = a | b
    if oo == 0: return (INF, 0, 0)
    o('ctz'); m = (oo & -oo).bit_length() - 1
    o('alu', 2); o('sh'); e = ((a & b) >> m) & 1
    o('sh', 2); a >>= m; b >>= m
    o('br')
    if e: o('alu', 2); o('sh', 2); a, b = (a + b) >> 1, (b - a) >> 1
    o('sh'); o('alu'); k = (m << 1) + e
    o('alu', 6); o('sh', 2); A, B = S31.ud_encode((a, b), n)
    return (k, A, B)
def drot(A, B, r, Mo, o):             # unit digits times i^r, jump table
    o('br'); return S31.digit_rot(A, B, r, Mo, o)

# ---------------- B operations ----------------
def B_add(x, y, n, o): return S31.ud_d0(x, y, n, n, o)
def B_neg(x, n, o): o('alu'); return (x[0], x[1] ^ ((1 << n) - 1), x[2])
def B_sub(x, y, n, o): return B_add(x, B_neg(y, n, o), n, o)
def B_conj(x, n, o):                  # conj(2^h (1+i)^e w) = 2^h (1+i)^e (-i)^e conj(w)
    k, A, B = x; Mo = (1 << n) - 1
    o('alu'); A ^= B
    o('alu'); o('br')
    if k & 1 and k < INF: A, B = S31.digit_rot(A, B, 3, Mo, o)
    return (k, A, B)
def B_muli(x, n, o): Mo = (1 << n) - 1; A, B = S31.digit_rot(x[1], x[2], 1, Mo, o); return (x[0], A, B)
def B_mul1i(x, n, o):                 # k + 1; when k is odd, (1+i)^2 = 2i: w -> i w
    k, A, B = x
    o('alu', 2); o('br')
    if k & 1 and k < INF: A, B = S31.digit_rot(A, B, 1, (1 << n) - 1, o)
    return (k + 1, A, B)
def B_mul(x, y, n, o):
    (k1, A1, B1), (k2, A2, B2) = x, y
    o('alu'); o('br')
    if max(k1, k2) >= INF: return (INF, 0, 0)
    o('alu', 8); w1 = S31.ud_decode(A1, B1, n); w2 = S31.ud_decode(A2, B2, n)
    w = A_mul(w1, w2, o)
    o('alu'); k = k1 + k2
    o('alu'); o('br')
    if k1 & k2 & 1: o('alu'); w = (-w[1], w[0])
    o('alu', 6); o('sh', 2); A, B = S31.ud_encode(w, n)
    return (k, A, B)
def B_norm(x, n, o):
    o('br')
    if x[0] >= INF: return 0
    o('alu', 4); w = S31.ud_decode(x[1], x[2], n)
    N = A_norm(w, o); o('sh'); return N << x[0]
def B_eq(x, y, n, o): o('alu', 5); return x == y
def B_iszero(x, n, o): o('alu'); return x[0] >= INF
def B_div1i(x, n, o): o('alu'); return x[0] >= 1
def B_div2(x, n, o): o('alu'); return x[0] >= 2
def B_exdiv_pij(x, j, n, o):          # k -> k - j; w' = i^((k'>>1) - (k>>1)) w
    k, A, B = x
    if k >= INF: o('br'); return x
    o('alu'); k2 = k - j
    o('sh', 2); o('alu'); r = (k2 >> 1) - (k >> 1)
    A, B = drot(A, B, r, (1 << n) - 1, o)
    return (k2, A, B)
def B_exdiv(x, y, n, o):              # x / y exact: k = kx - ky, w = wx / wy (odd / odd, Hensel, no strip)
    (k1, A1, B1), (k2, A2, B2) = x, y
    o('br')
    if k1 >= INF: return x
    o('alu'); k = k1 - k2
    o('alu', 8); w1 = S31.ud_decode(A1, B1, n); w2 = S31.ud_decode(A2, B2, n)
    o('mul', 4); o('alu', 2); p = (w1[0]*w2[0] + w1[1]*w2[1], w1[1]*w2[0] - w1[0]*w2[1])
    N = A_norm(w2, o); inv = inv2adic(N, o)
    o('mul', 2); q = (s64(p[0] * inv), s64(p[1] * inv))
    o('alu', 2); o('br')
    if (k2 & 1) and not (k1 & 1): o('alu'); q = (q[1], -q[0])     # (1+i)^-1 = 2^-1 (1+i) (-i)
    o('alu', 6); o('sh', 2); A, B = S31.ud_encode(q, n)
    return (k, A, B)
def B_divrem(x, y, n, o):
    q, r = A_divrem(b2a(x, n, o), b2a(y, n, o), o)
    return a2b(q, n, o), a2b(r, n, o)

# ---------------- C operations (cm: masks for m bits per coordinate, n = 2m digits) ----------------
def C_add(x, y, cm, o): return c_add(x, y, cm, o)
def C_sub(x, y, cm, o):
    a1, b1 = c2a(x, cm, o); a2, b2 = c2a(y, cm, o); o('alu', 2); return a2c(a1 - a2, b1 - b2, cm, o)
def C_un(f):                          # unary op through A
    def g(x, cm, o): return a2c(*f(c2a(x, cm, o), o), cm, o)
    return g
C_neg = C_un(A_neg); C_conj = C_un(A_conj); C_muli = C_un(A_muli); C_mul1i = C_un(A_mul1i)
def C_mulbeta(x, cm, o): o('sh'); return (x << 1) & cm.N          # times -1+i: a shift (mod beta^n)
def C_mul(x, y, cm, o): return a2c(*A_mul(c2a(x, cm, o), c2a(y, cm, o), o), cm, o)
def C_norm(x, cm, o): return A_norm(c2a(x, cm, o), o)
def C_eq(x, y, cm, o): o('alu'); return x == y
def C_iszero(x, cm, o): o('alu'); return x == 0
def C_div1i(x, cm, o): o('alu'); return (x & 1) == 0
def C_div2(x, cm, o): o('alu'); return (x & 3) == 0
def C_exdiv_pij(x, j, cm, o):         # (1+i) = -i beta: x / (1+i)^j = i^j (x >> j); the unit goes through A
    o('sh'); w = x >> j
    o('alu'); o('br')
    if j & 3 == 0: return w
    return a2c(*rotA(*c2a(w, cm, o), j, o), cm, o)
def C_exdiv(x, y, cm, o): return a2c(*A_exdiv(c2a(x, cm, o), c2a(y, cm, o), o), cm, o)
def C_divrem(x, y, cm, o):
    q, r = A_divrem(c2a(x, cm, o), c2a(y, cm, o), o)
    return a2c(*q, cm, o), a2c(*r, cm, o)

# ---------------- conversions involving C ----------------
def b2c(s, n, cm, o):                 # x = (1+i)^k u = (-i)^k beta^k u; split w = i^(k>>1) u, so C(x) = C(i^(-k-(k>>1)) w) << k
    k, A, B = s
    o('br')
    if k >= INF: return 0
    o('alu', 4); a, b = S31.ud_decode(A, B, n)
    o('sh'); o('alu', 2); r = -k - (k >> 1)
    a, b = rotA(a, b, r, o)
    w = a2c(a, b, cm, o)
    o('sh'); return (w << k) & cm.N
def c2b(w, n, cm, o):                 # strip is free: k = ctz(w); x = beta^k w' = (1+i)^k i^k w'; split w = i^(k + (k>>1)) C(w')
    o('br')
    if w == 0: return (INF, 0, 0)
    o('ctz'); o('sh'); k = (w & -w).bit_length() - 1; w >>= k
    a, b = c2a(w, cm, o)
    o('sh'); o('alu'); a, b = rotA(a, b, k + (k >> 1), o)
    o('alu', 6); o('sh', 2); A, B = S31.ud_encode((a, b), n)
    return (k, A, B)

def A_val0(x, o):
    o('alu'); o('ctz'); oo = x[0] | x[1]; e = (oo & -oo).bit_length() - 1 if oo else 64
    o('sh', 2); o('alu', 3); return 2*e + ((x[0] >> e) & (x[1] >> e) & 1) if oo else INF
def B_val(x, n, o): return x[0]                              # stored: 0 ops
def C_val(x, cm, o): o('ctz'); return (x & -x).bit_length() - 1 if x else INF
# ---------------- reference operations on values ----------------
def ref_round(n, N):                  # nearest integer to n/N, ties toward zero
    q, r = divmod(abs(n), N)
    if 2*r > N: q += 1
    return q if n >= 0 else -q
def ref_divrem(x, y):
    p = (x[0]*y[0] + x[1]*y[1], x[1]*y[0] - x[0]*y[1]); N = norm(y)
    q = (ref_round(p[0], N), ref_round(p[1], N)); qy = gmul(q, y)
    return q, (x[0] - qy[0], x[1] - qy[1])
def ref_pipow(x, j):                  # x / (1+i)^j
    for _ in range(j): x = ((x[0] + x[1]) // 2, (x[1] - x[0]) // 2)
    return x

# op table: name, kind ('bin', 'un', 'pred1', 'pred2', 'pij', 'exdiv', 'divrem'), reference, A, B, C
OPS = [
    ("add", 'bin', lambda x, y: (x[0]+y[0], x[1]+y[1]), A_add, B_add, C_add),
    ("subtract", 'bin', lambda x, y: (x[0]-y[0], x[1]-y[1]), A_sub, B_sub, C_sub),
    ("negate", 'un', lambda x: (-x[0], -x[1]), A_neg, B_neg, C_neg),
    ("conjugate", 'un', lambda x: (x[0], -x[1]), A_conj, B_conj, C_conj),
    ("times i", 'un', lambda x: (-x[1], x[0]), A_muli, B_muli, C_muli),
    ("times 1+i", 'un', lambda x: (x[0]-x[1], x[0]+x[1]), A_mul1i, B_mul1i, C_mul1i),
    ("multiply", 'mul', gmul, A_mul, B_mul, C_mul),
    ("norm", 'val', norm, A_norm, B_norm, C_norm),
    ("valuation", 'valk', None, A_val0, B_val, C_val),
    ("equality", 'pred2', lambda x, y: x == y, A_eq, B_eq, C_eq),
    ("is zero", 'pred1', lambda x: x == (0, 0), A_iszero, B_iszero, C_iszero),
    ("divisible by 1+i", 'pred1', lambda x: (x[0] - x[1]) % 2 == 0, A_div1i, B_div1i, C_div1i),
    ("divisible by 2", 'pred1', lambda x: x[0] % 2 == 0 and x[1] % 2 == 0, A_div2, B_div2, C_div2),
    ("exact / (1+i)^j", 'pij', ref_pipow, A_exdiv_pij, B_exdiv_pij, C_exdiv_pij),
    ("exact / general", 'exdiv', None, A_exdiv, B_exdiv, C_exdiv),
    ("div with remainder", 'divrem', ref_divrem, A_divrem, B_divrem, C_divrem),
]

def vstate(rep, x, n, cm):
    return mkA(x) if rep == 0 else mkB(x, n) if rep == 1 else mkC(x, cm)
def vval(rep, s, n, cm):
    return valA(s) if rep == 0 else valB(s, n) if rep == 1 else valC(s, cm)
def call(rep, f, args, n, cm, o):
    return f(*args, o) if rep == 0 else f(*args, n, o) if rep == 1 else f(*args, cm, o)

def pij_val(x):                        # (1+i)-adic valuation
    if x == (0, 0): return 0
    j = 0
    while (x[0] - x[1]) % 2 == 0: x = ((x[0] + x[1]) // 2, (x[1] - x[0]) // 2); j += 1
    return j

def run_case(op, x, y, n, cm, stats=None, j=None):
    name, kind, ref, *fs = op
    for rep in range(3):
        f = fs[rep]; o = Ops(); sx = vstate(rep, x, n, cm); sy = vstate(rep, y, n, cm)
        if kind == 'bin' or kind == 'mul':
            got = vval(rep, call(rep, f, (sx, sy), n, cm, o), n, cm); exp = ref(x, y)
        elif kind == 'un':
            got = vval(rep, call(rep, f, (sx,), n, cm, o), n, cm); exp = ref(x)
        elif kind == 'val':
            got = call(rep, f, (sx,), n, cm, o); exp = ref(x)
        elif kind == 'valk':
            got = call(rep, f, (sx,), n, cm, o); exp = pij_val(x) if x != (0, 0) else INF
            if exp == INF: got = INF if got >= 64 else got
        elif kind == 'pred1':
            got = call(rep, f, (sx,), n, cm, o); exp = ref(x)
        elif kind == 'pred2':
            got = call(rep, f, (sx, sy), n, cm, o); exp = ref(x, y)
        elif kind == 'pij':
            got = vval(rep, call(rep, f, (sx, j), n, cm, o), n, cm); exp = ref_pipow(x, j)
        elif kind == 'exdiv':                       # x is the quotient, y the divisor: divide x*y by y
            sxy = vstate(rep, gmul(x, y), n, cm)
            got = vval(rep, call(rep, f, (sxy, sy), n, cm, o), n, cm); exp = x
        elif kind == 'divrem':
            q, r = call(rep, f, (sx, sy), n, cm, o)
            got = (vval(rep, q, n, cm), vval(rep, r, n, cm)); exp = ref(x, y)
        assert got == exp, (name, rep, x, y, got, exp)
        if stats is not None: stats[(name, rep)].update(o); stats[(name, rep, 'cnt')]['n'] += 1

def pick_args(op, rnd, R):
    kind = op[1]
    if kind in ('mul', 'exdiv'): R = int(R ** 0.5) // 2
    x = (rnd.randrange(-R, R), rnd.randrange(-R, R)); y = (rnd.randrange(-R, R), rnd.randrange(-R, R))
    if kind in ('exdiv', 'divrem') and y == (0, 0): y = (1, 0)
    if kind == 'pred2' and rnd.random() < 0.5: y = x
    return x, y

def check_table():
    print("operation table: verification")
    # small widths, exhaustive over a box: B with 8 digits, C with 16 digits (inscribed box 34)
    n, cm = 8, CM(8); R = 8; box = [(a, b) for a in range(-R, R+1) for b in range(-R, R+1)]
    for op in OPS:
        kind = op[1]; cnt = 0
        for x in box:
            for y in box:
                if kind in ('mul', 'exdiv') and (max(map(abs, x + y)) > 4): continue
                if kind in ('exdiv', 'divrem') and y == (0, 0): continue
                if kind == 'pij':
                    if y != (0, 0): continue
                    for j in range(pij_val(x) + 1 if x != (0, 0) else 6): run_case(op, x, y, n, cm, j=j); cnt += 1
                    continue
                run_case(op, x, y, n, cm); cnt += 1
        print("  %-20s exhaustive, box |a|,|b| <= %d (B 8 digits, C 16 digits): %d cases ok" % (op[0], R, cnt))
    # full width, random: A int32, B 32 digits, C 64 digits; operands |a|,|b| < 2^28 (multiply, exact division: 2^13)
    n, cm = 32, CM(32); rnd = random.Random(11); stats = {}
    from collections import defaultdict
    stats = defaultdict(Counter); NR = 3000
    for op in OPS:
        for _ in range(NR):
            x, y = pick_args(op, rnd, 1 << 28)
            if op[1] == 'pij':
                j = pij_val(x) if x != (0, 0) else 0
                run_case(op, x, y, n, cm, stats, j=rnd.randint(0, j)); continue
            run_case(op, x, y, n, cm, stats)
    print("  full width (A int32, B 32 digits, C 64 digits), %d random cases per op: exact" % NR)
    # C exact division by (1+i)^j split by j mod 4
    for jm in (0, 1):
        o = Ops(); cnt = 0
        for _ in range(300):
            x = (rnd.randrange(-2**20, 2**20) << 4, rnd.randrange(-2**20, 2**20) << 4); j = 4 if jm == 0 else rnd.choice((1, 2, 3, 5))
            run_case(OPS[[op[0] for op in OPS].index("exact / (1+i)^j")], x, (0, 0), n, cm, None, j=j)
            C_exdiv_pij(mkC(x, cm), j, cm, o); cnt += 1
        print("  C exact / (1+i)^j with j %s: %.1f ops" % ("= 0 mod 4" if jm == 0 else "!= 0 mod 4", o.tot() / cnt))
    return stats

def fmt(c, cnt):
    t = sum(c.values()) / cnt
    extra = []
    if c['mul']: extra.append("%.0f mul" % (c['mul'] / cnt))
    if c['div']: extra.append("%.0f div" % (c['div'] / cnt))
    if c['ld']: extra.append("%.0f ld" % (c['ld'] / cnt))
    return ("%.1f" % t) + (" (" + ", ".join(extra) + ")" if extra else "")
def print_table(stats):
    print("\n  ops per operation (average over the random cases; every op counts 1):")
    print("  | operation | A pair | B unit digits | C base -1+i |")
    print("  |---|---|---|---|")
    for op in OPS:
        cells = [fmt(stats[(op[0], r)], stats[(op[0], r, 'cnt')]['n']) for r in range(3)]
        print("  | %s | %s |" % (op[0], " | ".join(cells)))


# ---------------- conversions between representations ----------------
CONV = [
    ("A -> B", 0, 1, lambda s, n, cm, o: a2b(s, n, o)),
    ("B -> A", 1, 0, lambda s, n, cm, o: b2a(s, n, o)),
    ("A -> C (fixpoint)", 0, 2, lambda s, n, cm, o: a2c(s[0], s[1], cm, o)),
    ("A -> C (byte table)", 0, 2, lambda s, n, cm, o: a2c_table(s[0], s[1], cm, o)[0]),
    ("C -> A", 2, 0, lambda s, n, cm, o: c2a(s, cm, o)),
    ("B -> C", 1, 2, b2c),
    ("C -> B", 2, 1, c2b),
]
def check_conv():
    print("conversions: verification")
    n, cm = 8, CM(8); R = 34
    box = [(a, b) for a in range(-R, R+1) for b in range(-R, R+1)]
    for name, r1, r2, f in CONV:
        for x in box:
            assert vval(r2, f(vstate(r1, x, n, cm), n, cm, Ops()), n, cm) == x, (name, x)
    print("  all 7 converters exact on the box |a|,|b| <= %d (the whole inscribed box of 16-digit C), B 8 digits" % R)
    n, cm = 32, CM(32); rnd = random.Random(5); st = {name: Ops() for name, *_ in CONV}; NR = 20000
    for _ in range(NR):
        x = (rnd.randrange(-2**28, 2**28), rnd.randrange(-2**28, 2**28))
        if rnd.random() < 0.3: x = ((x[0] >> 9) << 9, (x[1] >> 9) << 9) if rnd.random() < 0.5 else (x[0] >> 20, x[1] >> 20)
        for name, r1, r2, f in CONV:
            o = Ops(); assert vval(r2, f(vstate(r1, x, n, cm), n, cm, o), n, cm) == x; st[name].update(o)
    print("  full width, %d random values (some with large or small (1+i)-valuation shifts): exact" % NR)
    print("  | conversion | ops avg |")
    print("  |---|---|")
    for name, *_ in CONV: print("  | %s | %s |" % (name, fmt(st[name], NR)))
    return {name: st[name].tot() / NR for name, *_ in CONV}


# ---------------- binary (1+i)-ary gcd in each representation ----------------
# A: study 9 (gcd1i.py): strip 16, primary 17, step 54. B and C: replace the larger of a, b by a - eps b, eps the unit
# with a = eps b mod (1+i)^3, read from the low digits (B: digit 0 codes; C: digits 1, 2, since odd w = i^((w>>1)&3) mod beta^3).
# Both give the same norms as study 9 (a - eps b is a unit times primary(a) - primary(b)), so the same step count.
def A_strip(x, y, o):
    o('alu'); o('ctz'); oo = x | y; e = (oo & -oo).bit_length() - 1 if oo else 0   # ctz(0) = 64, shift count mod 64
    o('sh', 2); x >>= e; y >>= e
    o('alu', 3); m = -(x & y & 1)
    o('alu', 2); o('sh', 2); px, py = (x + y) >> 1, (y - x) >> 1
    o('alu', 6); return x ^ ((x ^ px) & m), y ^ ((y ^ py) & m)
def A_primary(x, y, o):
    o('alu', 2); m = (x & 1) - 1
    o('alu', 7); nx, ny = x ^ ((x ^ y) & m), y ^ ((y ^ -x) & m)
    o('alu', 3); o('sh'); m = -(((nx - ny) >> 1) & 1)
    o('alu', 4); return (nx ^ m) - m, (ny ^ m) - m
def A_val(x, o):
    o('alu'); o('ctz'); oo = x[0] | x[1]; e = (oo & -oo).bit_length() - 1
    o('sh', 2); o('alu', 3); return 2*e + ((x[0] >> e) & (x[1] >> e) & 1)
def cswap_ops(o, words): o('alu', 4 * words)
def gcd_A(x, y, o):
    o('alu', 2); o('br')
    if x == (0, 0) or y == (0, 0): return x if y == (0, 0) else y, 0
    k = min(A_val(x, o), A_val(y, o)); o('alu', 3)
    a = A_primary(*A_strip(*x, o), o); b = A_primary(*A_strip(*y, o), o)
    na, nb = A_norm(a, o), A_norm(b, o)
    o('alu', 2); cswap_ops(o, 3)
    if na < nb: a, b, na, nb = b, a, nb, na
    steps = 0
    while True:
        o('alu'); o('br')
        if b == (0, 0): break
        o('alu', 2); a = A_primary(*A_strip(a[0] - b[0], a[1] - b[1], o), o)
        na = A_norm(a, o)
        o('alu', 2); cswap_ops(o, 3)
        if na < nb: a, b, na, nb = b, a, nb, na
        steps += 1
    o('sh', 3); o('alu'); o('br'); g = (a[0] << (k >> 1), a[1] << (k >> 1))
    if k & 1: o('alu', 2); g = (g[0] - g[1], g[0] + g[1])
    return g, steps
def gcd_B(x, y, n, o):
    Mo = (1 << n) - 1
    o('alu'); o('br')
    if x[0] >= INF or y[0] >= INF: return (x if y[0] >= INF else y), 0
    o('alu', 3); k = min(x[0], y[0])                     # valuation: free; odd parts: free (split w is a unit times u)
    a, b = (x[1], x[2]), (y[1], y[2])
    o('alu', 8); na = A_norm(S31.ud_decode(*a, n), o); nb = A_norm(S31.ud_decode(*b, n), o)
    o('alu', 2); cswap_ops(o, 3)
    if na < nb: a, b, na, nb = b, a, nb, na
    steps = 0
    while True:
        (A1, B1), (A2, B2) = a, b
        o('sh', 2); o('alu', 6); r = (((A1 << 1) + (B1 & 1)) - ((A2 << 1) + (B2 & 1))) & 3   # digit-0 codes: eps = i^r
        o('alu', 6); A2, B2 = S31.digit_rot(A2, B2, r, Mo, Ops())                      # masked digit rotation
        o('alu'); A2 ^= Mo                                                           # negate eps b
        o('alu', 6); P = Mo - ((A1 ^ B1) + (A2 ^ B2)); Q = Mo - (A1 + A2)            # a - eps b = -i(1+i)(P + iQ)
        o('alu'); o('br'); steps += 1
        if P | Q == 0: break                                                          # a = eps b: gcd is b
        z = A_strip(P, Q, o); nz = A_norm(z, o)
        o('alu', 6); o('sh', 2); a = S31.ud_encode(z, n); na = nz
        o('alu', 2); cswap_ops(o, 3)
        if na < nb: a, b, na, nb = b, a, nb, na
    o('alu'); return (k, b[0], b[1]), steps
def gcd_C(x, y, cm, o):
    o('alu'); o('br')
    if x == 0 or y == 0: return (x if y == 0 else y), 0
    o('ctz', 2); o('alu', 3); kx = (x & -x).bit_length() - 1; ky = (y & -y).bit_length() - 1; k = min(kx, ky)
    o('sh', 2); a, b = x >> kx, y >> ky                                                  # strip: free
    na = A_norm(c2a(a, cm, o), o); nb = A_norm(c2a(b, cm, o), o)
    o('alu', 2); cswap_ops(o, 2)
    if na < nb: a, b, na, nb = b, a, nb, na
    steps = 0
    while True:
        xa = c2a(a, cm, o); xb = c2a(b, cm, o)
        o('sh', 2); o('alu', 2); r = ((a >> 1) - (b >> 1)) & 3
        xb = rotA(*xb, r, o)
        o('alu', 2); d = (xa[0] - xb[0], xa[1] - xb[1])
        o('alu'); o('br'); steps += 1
        if d == (0, 0): break
        nd = A_norm(d, o)
        w = a2c(*d, cm, o)
        o('ctz'); o('sh', 2); v = (w & -w).bit_length() - 1; a = w >> v; na = nd >> v      # strip: free
        o('alu', 2); cswap_ops(o, 2)
        if na < nb: a, b, na, nb = b, a, nb, na
    o('sh'); return (b << k) & cm.N, steps

def ref_gcd(x, y):
    while y != (0, 0): x, y = y, ref_divrem(x, y)[1]
    return x
def assoc(x, y): return any(gmul(x, u) == y for u in ((1, 0), (0, 1), (-1, 0), (0, -1)))

GCD = {}
def check_gcd():
    print("gcd: verification")
    n, cm = 8, CM(8); R = 6; box = [(a, b) for a in range(-R, R+1) for b in range(-R, R+1)]
    for x in box:
        for y in box:
            g = ref_gcd(x, y)
            gA, sA = gcd_A(x, y, Ops()); assert assoc(gA, g), (x, y, gA, g)
            gB, sB = gcd_B(mkB(x, n), mkB(y, n), n, Ops()); assert assoc(valB(gB, n), g)
            gC, sC = gcd_C(mkC(x, cm), mkC(y, cm), cm, Ops()); assert assoc(valC(gC, cm), g)
            assert sB == sC and (sA == sB or x == (0, 0) or y == (0, 0) or sA == sB - 1 or sA == sB)
    print("  exhaustive, box |a|,|b| <= %d (%d pairs): A, B, C all give an associate of the reference gcd" % (R, len(box)**2))
    n, cm = 32, CM(32); rnd = random.Random(9)
    for bits, reps in ((31, (0, 1)), (28, (0, 1, 2))):
        tot = [Ops() for _ in range(3)]; steps = [0, 0, 0]; NR = 1500; mx = 0
        for _ in range(NR):
            h = 1 << (bits - 1)
            x = (rnd.randrange(-h, h), rnd.randrange(-h, h)); y = (rnd.randrange(-h, h), rnd.randrange(-h, h))
            g = ref_gcd(x, y)
            o = Ops(); gA, s = gcd_A(x, y, o); assert assoc(gA, g); tot[0].update(o); steps[0] += s
            o = Ops(); gB, s = gcd_B(mkB(x, n), mkB(y, n), n, o); assert assoc(valB(gB, n), g); tot[1].update(o); steps[1] += s
            if 2 in reps:
                o = Ops(); gC, s = gcd_C(mkC(x, cm), mkC(y, cm), cm, o); assert assoc(valC(gC, cm), g); tot[2].update(o); steps[2] += s
        print("  random %d-bit pairs, %d cases: exact (up to a unit)" % (bits, NR))
        GCD[bits] = [tot[r].tot() / NR for r in range(3)]
        for r in reps:
            print("    %s: %.2f steps, %.0f ops per gcd, %.1f ops per step overall (%s)" % ("ABC"[r], steps[r] / NR,
                  tot[r].tot() / NR, tot[r].tot() / steps[r], fmt(tot[r], NR)))


# ---------------- hybrid analysis ----------------
def hybrid(stats, conv):
    names = [op[0] for op in OPS]
    cost = {(nm, r): sum(stats[(nm, r)].values()) / stats[(nm, r, 'cnt')]['n'] for nm in names for r in range(3)}
    for r in range(3): cost[('gcd', r)] = GCD[28][r] if r == 2 else GCD[31][r]
    cost[('gcd', 2)] = GCD[28][2] * GCD[31][0] / GCD[28][0]          # scale C's 28-bit gcd to 31-bit step counts
    to = {(0, 1): conv["A -> B"], (1, 0): conv["B -> A"], (0, 2): conv["A -> C (fixpoint)"], (2, 0): conv["C -> A"],
          (1, 2): conv["B -> C"], (2, 1): conv["C -> B"], (0, 0): 0, (1, 1): 0, (2, 2): 0}
    arity = {nm: (2 if op[1] in ('bin', 'mul', 'pred2', 'exdiv', 'divrem') else 1) for op in OPS for nm in [op[0]]}
    outs = {nm: (0 if op[1] in ('pred1', 'pred2', 'val', 'valk') else 2 if op[1] == 'divrem' else 1) for op in OPS for nm in [op[0]]}
    arity['gcd'] = 2; outs['gcd'] = 1
    def via_a(nm, S):                 # operands S -> A, compute in A, results A -> S
        return arity[nm] * to[(S, 0)] + cost[(nm, 0)] + outs[nm] * to[(0, S)]
    loads = {"sum of 1000 terms": ({"add": 999}, 1000, 1),
             "product chain of 100 terms": ({"multiply": 99}, 100, 1),
             "gcd batch, 100 pairs": ({"gcd": 100}, 200, 100),
             "mixed, 100 items": ({"add": 300, "subtract": 100, "multiply": 200, "norm": 100, "valuation": 100,
                                    "divisible by 1+i": 200, "exact / (1+i)^j": 100, "equality": 100, "is zero": 100,
                                    "div with remainder": 50, "gcd": 10}, 200, 100)}
    print("\nhybrid analysis (ops in the model; inputs and outputs in the storage format)")
    print("  | workload | A native | B native | B per-op best | B convert once | C native | C convert once |")
    print("  |---|---|---|---|---|---|---|")
    for wl, (mix, nin, nout) in loads.items():
        row = []
        for S in range(3):
            nat = sum(c * cost[(nm, S)] for nm, c in mix.items())
            best = sum(c * min(cost[(nm, S)], via_a(nm, S)) for nm, c in mix.items())
            once = nin * to[(S, 0)] + sum(c * cost[(nm, 0)] for nm, c in mix.items()) + nout * to[(0, S)]
            row.append((nat, best, once))
        print("  | %s | %.0f | %.0f | %.0f | %.0f | %.0f | %.0f |" % (wl, row[0][0], row[1][0], row[1][1], row[1][2], row[2][0], row[2][2]))
    print("\n  break-even: data in A, converted to F and back, to run k ops of one kind (k = round trip / saving per op)")
    print("  | op | A | B | k for B | C | k for C |")
    print("  |---|---|---|---|---|---|")
    rtB = to[(0, 1)] + to[(1, 0)]; rtC = to[(0, 2)] + to[(2, 0)]
    for nm in names:
        a, b, c = cost[(nm, 0)], cost[(nm, 1)], cost[(nm, 2)]
        kb = "%.0f" % (rtB / (a - b)) if a > b else "never"
        kc = "%.0f" % (rtC / (a - c)) if a > c else "never"
        print("  | %s | %.1f | %.1f | %s | %.1f | %s |" % (nm, a, b, kb, c, kc))
    print("  round trips: A->B->A %.1f, A->C->A %.1f; for tests (no result to convert back) only the forward conversion counts:"
          % (rtB, rtC))
    for nm in ("valuation", "is zero", "divisible by 1+i", "divisible by 2", "equality"):
        a, b, c = cost[(nm, 0)], cost[(nm, 1)], cost[(nm, 2)]
        print("    %s: B after %.1f, C after %.1f uses" % (nm, to[(0, 1)] / (a - b) if a > b else float('inf'),
                                                        to[(0, 2)] / (a - c) if a > c else float('inf')))

if __name__ == "__main__":
    what = sys.argv[1] if len(sys.argv) > 1 else "all"
    if what in ("c", "all"): check_c()
    if what in ("table", "all", "hyb"): st = check_table(); print_table(st)
    if what in ("conv", "all", "hyb"): cv = check_conv()
    if what in ("gcd", "all", "hyb"): check_gcd()
    if what in ("all", "hyb"): hybrid(st, cv)
