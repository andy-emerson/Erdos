"""split.py: checks for prior-art.md's "Packed, split layout" section and the same-direction fast paths of all three
packed layouts. An R-bit word w = s*2^(R-1) + 2m + a: sign s at the top bit, axis a at bit 0, magnitude m in bits
1..R-2; direction d = 2s + a; value m*i^d. (R = 64 is the real layout: m in bits 1-62.)

Exhaustive on R = 8 and 10 (every direction and magnitude pair, add and subtract), random on R = 64. Each result is
decoded and compared with the Gaussian-integer value; expressions must set the flag; inputs whose true result
overflows are skipped, except in the overflow-detection checks, which use exactly those inputs. A zero may have any
direction (spec v0.2 §3).

  python3 -I split.py            # R = 8, 10 exhaustive, then 300,000 random R = 64 cases
"""
import random
import sys

I = [(1, 0), (0, 1), (-1, 0), (0, -1)]
def val(m, d): return (m * I[d][0], m * I[d][1])
def gmul(x, y): return (x[0]*y[0] - x[1]*y[1], x[0]*y[1] + x[1]*y[0])


class Word:
    def __init__(self, R):
        self.R = R; self.N = 1 << R; self.F = self.N - 1
        self.n = R - 2; self.MAX = (1 << self.n) - 1            # magnitude bits, largest magnitude
        self.SB = 1 << (R - 1)                                   # the sign bit (bit 63)
        self.Mm = self.SB - 2                                    # bits 1..R-2 (0x7FFF_FFFF_FFFF_FFFE)
        self.K = self.SB + 1                                     # bits R-1 and 0  (0x8000_0000_0000_0001)
    def sar(self, x, k): return ((x - self.N if x >> (self.R - 1) else x) >> k) & self.F
    # layouts: (encode, decode)
    def enc_split(self, m, d): return ((d >> 1) << (self.R - 1)) | (m << 1) | (d & 1)
    def dec_split(self, w): return ((w >> 1) & self.MAX, 2 * (w >> (self.R - 1)) + (w & 1))
    def enc_top(self, m, d): return (d << self.n) | m
    def dec_top(self, w): return (w & self.MAX, w >> self.n)
    def enc_bot(self, m, d): return (m << 2) | d
    def dec_bot(self, w): return (w >> 2, w & 3)

    # ---------------- split layout: the formulas of prior-art.md, exactly --------------------------------------
    def add(self, w1, w2, lvariant=0):
        F, R = self.F, self.R
        t = w1 ^ w2                                              # 1
        s = self.sar(t, R - 1)                                   # 1   signs differ
        if lvariant == 0:
            L = (0 - int(((w1 << 1) & F) < ((w2 << 1) & F))) & F         # 4   shl, shl, sltu, neg
        else:
            L = (0 - int((w1 ^ (t & self.SB)) < w2)) & F                 # 4   and, xor, sltu, neg
        W = w1 ^ (t & L)                                         # 2   the word with the larger magnitude
        v = (W ^ t) & self.Mm                                    # 2   2 * the smaller magnitude
        r = (W + (v ^ s) - s) & F                                # 3
        e = t & 1 & int(v != 0)                                  # 3   expression flag
        return r, e, W
    def sub(self, w1, w2, lvariant=0): return self.add(w1, w2 ^ self.SB, lvariant)
    def neg(self, w): return w ^ self.SB
    def mul(self, w1, w2, variant=0):
        F, Mm, K = self.F, self.Mm, self.K
        x1 = w1 & Mm; x2 = w2 & Mm                               # 2   2*m1, 2*m2
        p = ((x1 * x2) & F) >> 1                                 # 2   2*m1*m2 (4*m1*m2 < 2^R by no overflow)
        if variant == 0:
            D = ((w1 | Mm) + (w2 ^ x2)) & F & K                  # 4   or, xor, add, and: d1 + d2 mod 4 at bits R-1, 0
        else:
            D = ((w1 ^ w2) ^ (((w1 & w2) << (self.R - 1)) & F)) & K      # 5   xor, and, shl, xor, and
        return p | D                                             # 1
    def div(self, w1, w2, variant=0):
        F, Mm, K = self.F, self.Mm, self.K
        x1 = w1 & Mm; x2 = w2 & Mm                               # 2
        if variant == 0:
            q = ((x1 // x2) << 1) & F                            # 2   m1/m2, doubled
            D = ((w1 ^ x1) - (w2 ^ x2)) & F & K                  # 4   xor, xor, sub, and: d1 - d2 mod 4
        else:
            q = x1 // (x2 >> 1)                                  # 2   2*m1 / m2 = 2q
            D = ((w1 ^ w2) ^ (((w2 & ~w1) << (self.R - 1)) & F)) & K     # 5 (4 with andn/bic)
        return q | D                                             # 1
    def is_zero(self, w): return int(((w << 1) & self.F) < 4)    # 2
    def overflow(self, r, W): return self.sar(r ^ W, self.R - 1) # 2   all ones when the magnitude carried into the sign


def check_split(R, pairs):
    w = Word(R); bad = {}
    def fail(name, *a):
        bad[name] = bad.get(name, 0) + 1
        if bad[name] == 1: print("FAIL", R, name, a)
    for d1, m1, d2, m2 in pairs:
        w1, w2 = w.enc_split(m1, d1), w.enc_split(m2, d2)
        v1, v2 = val(m1, d1), val(m2, d2)
        for sub in (0, 1):
            vv2 = (-v2[0], -v2[1]) if sub else v2
            v = (v1[0] + vv2[0], v1[1] + vv2[1])
            expr = v[0] != 0 and v[1] != 0
            for lv in (0, 1):
                r, e, W = (w.sub if sub else w.add)(w1, w2, lv)
                nm = "add L%d" % lv
                if expr:
                    if e != 1: fail(nm, d1, m1, d2, m2, sub)
                    continue
                if abs(v[0]) + abs(v[1]) > w.MAX:
                    if lv == 0 and w.overflow(r, W) != w.F: fail("overflow detect", d1, m1, d2, m2, sub)
                    continue
                m, d = w.dec_split(r)
                if e or val(m, d) != v: fail(nm, d1, m1, d2, m2, sub, r)
                if lv == 0 and w.overflow(r, W) != 0: fail("overflow false alarm", d1, m1, d2, m2, sub)
        m, d = w.dec_split(w.neg(w1))
        if val(m, d) != (-v1[0], -v1[1]): fail("neg", d1, m1)
        if w.is_zero(w1) != int(m1 == 0): fail("zero test", d1, m1)
        if m1 * m2 <= w.MAX:
            for var in (0, 1):
                m, d = w.dec_split(w.mul(w1, w2, var))
                if val(m, d) != gmul(v1, v2): fail("mul v%d" % var, d1, m1, d2, m2)
        if m2 and m1 % m2 == 0:
            for var in (0, 1):
                m, d = w.dec_split(w.div(w1, w2, var))
                if m != m1 // m2 or (m1 and gmul(val(m, d), v2) != v1): fail("div v%d" % var, d1, m1, d2, m2)
    return bad


def check_fastpaths(R, pairs):
    """Branching same-direction fast path, all three packed layouts: when the direction bits of t = w1 xor w2 are 0,
    r = w1 + (w2 AND magnitude field). Also each layout's overflow test on that path."""
    w = Word(R); F = w.F; bad = {}
    lay = {
        "top":   (w.enc_top,   w.dec_top,   lambda t: t >> w.n,     w.MAX,     lambda r, w1: (r ^ w1) >> w.n),
        "bot":   (w.enc_bot,   w.dec_bot,   lambda t: t & 3,        F & ~3,    lambda r, w1: int(r < w1)),
        "split": (w.enc_split, w.dec_split, lambda t: t & w.K,      w.Mm,      lambda r, w1: w.sar(r ^ w1, R - 1)),
    }
    for name, (enc, dec, dirbits, field, ovf) in lay.items():
        for d1, m1, d2, m2 in pairs:
            w1, w2 = enc(m1, d1), enc(m2, d2)
            same = dirbits(w1 ^ w2) == 0
            if same != (d1 == d2): bad[name + " test"] = bad.get(name + " test", 0) + 1; continue
            if not same: continue
            r = (w1 + (w2 & field)) & F
            if m1 + m2 > w.MAX:
                if ovf(r, w1) == 0: bad[name + " ovf"] = bad.get(name + " ovf", 0) + 1
                continue
            m, d = dec(r)
            if ovf(r, w1) != 0 or val(m, d) != val(m1 + m2, d1): bad[name + " fast"] = bad.get(name + " fast", 0) + 1
    return bad


def exhaustive(R):
    MAX = (1 << (R - 2)) - 1
    return [(d1, m1, d2, m2) for d1 in range(4) for d2 in range(4) for m1 in range(MAX + 1) for m2 in range(MAX + 1)]


def rnd64(k):
    MAX = (1 << 62) - 1
    def rm():
        if random.random() < 0.2: return random.choice([0, 1, 2, MAX, MAX - 1, 1 << 61])
        return random.getrandbits(random.randint(1, 62))
    out = []
    for _ in range(k):
        d1 = random.randrange(4); d2 = random.randrange(4)
        if random.random() < 0.5: d2 = random.choice([d1, (d1 + 2) % 4])
        m1 = rm(); m2 = rm()
        r = random.random()
        if r < 0.2: m2 = max(0, min(MAX, m1 + random.randint(-2, 2)))
        elif r < 0.4:                                            # small enough to multiply, and exact quotients
            m1 = random.getrandbits(31); m2 = random.getrandbits(31) or 1
            if random.random() < 0.5: m1 = m1 * m2
        out.append((d1, m1, d2, m2))
    return out


if __name__ == "__main__":
    random.seed(13)
    ok = True
    for R in (8, 10):
        P = exhaustive(R)
        b1 = check_split(R, P); b2 = check_fastpaths(R, P)
        print("R = %d, %d pairs: split %s, fast paths %s" % (R, len(P), b1 or "ok", b2 or "ok"))
        ok &= not b1 and not b2
    P = rnd64(300000)
    b1 = check_split(64, P); b2 = check_fastpaths(64, P)
    print("R = 64, %d random pairs: split %s, fast paths %s" % (len(P), b1 or "ok", b2 or "ok"))
    ok &= not b1 and not b2
    sys.exit(0 if ok else 1)
