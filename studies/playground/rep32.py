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

if __name__ == "__main__":
    what = sys.argv[1] if len(sys.argv) > 1 else "all"
    if what in ("c", "all"): check_c()
