"""check.py: checks every formula in complement-research.md.

Complement layout on an n-bit word: bit n-1 = sign start s, bit n-2 = axis a (G), bits 0..n-3 = count.
Value (count - 2^(n-2) s) * i^a. Each axis is an (n-1)-bit two's-complement integer with the axis bit
inserted between its sign bit and the rest.

Exhaustive on n = 4, 6, 8 (every word or pair), then random n = 64 (skewed to 0, the extremes, and carries).
Each formula is checked against the decoded value. Wrapping formulas must equal the (n-1)-bit wrapped sum
exactly (axis kept); overflow tests must be exact (no false alarms, nothing missed); flags "OF of the add"
are checked by computing the machine's signed-overflow flag of the named add/sub instruction.

  python3 -I check.py          # n = 4, 6, 8 exhaustive, then 200,000 random n = 64 cases
"""
import random
import sys


class L:
    def __init__(self, n):
        self.n = n; self.F = (1 << n) - 1
        self.S = 1 << (n - 1); self.G = 1 << (n - 2); self.K = self.S | self.G; self.C = self.G - 1
        self.H = 1 << (n - 2)                     # 2^(n-2): the axis range is -H .. H-1

    # ---- layout ----
    def val(self, w): return (w & self.C) - self.H * (w >> (self.n - 1))
    def ax(self, w): return (w >> (self.n - 2)) & 1
    def enc(self, v, a):                          # wraps like an (n-1)-bit signed integer
        v &= (1 << (self.n - 1)) - 1
        return ((v >> (self.n - 2)) << (self.n - 1)) | (a << (self.n - 2)) | (v & self.C)
    def iszero(self, w): return (w & ~self.G & self.F) == 0
    def inrange(self, v): return -self.H <= v < self.H
    def sar(self, x, k): return ((x - (1 << self.n) if x >> (self.n - 1) else x) >> k) & self.F
    def of_add(self, x, y):                       # machine signed-overflow flag of x + y (x86 OF, ARM64 V)
        r = (x + y) & self.F; return ((x ^ r) & (y ^ r)) >> (self.n - 1)
    def of_sub(self, x, y):
        r = (x - y) & self.F; return ((x ^ y) & (x ^ r)) >> (self.n - 1)

    # ---- same-axis add, wrapping ----
    def add_bridge(self, w1, w2):                 # 5: xor, add | xor, and, xor
        b = ((w1 ^ self.G) + w2) & self.F
        return b ^ ((b ^ w2) & self.G)
    def add_bridge_b(self, w1, w2):               # 5: the merge form of the fix
        b = ((w1 ^ self.G) + w2) & self.F
        return w1 ^ ((w1 ^ b) & ~self.G & self.F)
    def add_bridge_flag(self, w1, w2):            # OF of the bridge add, the add instruction itself
        return self.of_add(w1 ^ self.G, w2)
    def add_carry3(self, w1, w2):                 # 5 with x86 lea / 6: add, xor, and, (lea 3e | shl, xor), xor
        s = (w1 + w2) & self.F
        e = (s ^ w1) & self.G                     # bit G = axis XOR carry out of the count
        return s ^ e ^ ((e << 1) & self.F)
    def add_carry3_flag(self, w1, w2): return self.of_add(w1, w2)   # expected WRONG
    def add_orand(self, w1, w2):                  # 6: or, and, add | xor, and, xor  (also the cross-axis bridge)
        b = ((w1 | self.G) + (w2 & ~self.G & self.F)) & self.F
        return b ^ ((b ^ w1) & self.G)
    def add_pdep(self, w1, w2):                   # x86 BMI2 reference: pext, pext, add, pdep, and, or
        m = ~self.G & self.F
        def pext(x): return (x & self.C) | ((x >> 1) & self.G)
        def pdep(x): return (x & self.C) | ((x << 1) & self.S)
        return pdep((pext(w1) + pext(w2)) & self.F) | (w1 & self.G)

    # ---- overflow tests (same axis) ----
    def ovf_add(self, w1, w2, r): return ((w1 ^ r) & (w2 ^ r)) >> (self.n - 1)          # 3 (+1 to a mask)
    def ovf_add_andn(self, w1, w2, r): return (~(w1 ^ w2) & (w1 ^ r) & self.F) >> (self.n - 1)
    def ovf_sub(self, w1, w2, r): return ((w1 ^ w2) & (w1 ^ r)) >> (self.n - 1)

    # ---- same-axis subtract ----
    def sub(self, w1, w2):                        # 4: sub | xor, and, xor
        d = (w1 - w2) & self.F
        return d ^ ((d ^ w1) & self.G)
    def sub_b(self, w1, w2):
        d = (w1 - w2) & self.F
        return w1 ^ ((w1 ^ d) & ~self.G & self.F)
    def sub_flag(self, w1, w2): return self.of_sub(w1, w2)

    # ---- negate ----
    def neg(self, w):                             # 4: and, sub, and, or
        z = w & self.G
        return (((z - w) & self.F) & ~self.G) | z
    def neg_k(self, w):                           # 4: and K, sub, and, xor (its sub's OF is NOT the overflow)
        z = w & self.K
        return (((z - w) & self.F) & ~self.G) ^ z
    def neg_flag(self, w): return self.of_sub(w & self.G, w)         # OF of (w AND G) - w: w = -H exactly
    def neg_sub(self, w): return self.sub(w & self.G, w)   # 0 on w's axis, minus w: 5

    # ---- full add on the axes ----
    def full(self, w1, w2):
        """11 + flag 5. Same axis: the sum. Different axes, one operand zero: the other. e = expression."""
        F, G = self.F, self.G
        t = w1 ^ w2                               # 1
        z1 = w1 & ~G & F                          # 1
        b = (z1 + (w2 | G)) & F                   # 2  bridge on every input (axis column 0 + 1)
        Z1 = (0 - int(z1 == 0)) & F               # 2  seqz, neg
        src = w1 ^ (t & Z1)                       # 2  w2 if w1 is zero, else w1
        r = b ^ ((b ^ src) & G)                   # 3
        z2 = w2 & ~G & F                          # 1
        e = ((t & ~Z1 & F) >> (self.n - 2)) & int(z2 != 0)   # 4: andn, shr, snez, and  (bit 0)
        return r, e & 1
    def full_sel(self, w1, w2):
        """With selects (ARM64 csel, x86 cmov): same-axis bridge, then w2 if w1 is zero, w1 if w2 is zero."""
        r = self.add_bridge(w1, w2)
        if self.iszero(w2): r = w1
        if self.iszero(w1): r = w2
        e = ((w1 ^ w2) >> (self.n - 2)) & 1 & int(not self.iszero(w1)) & int(not self.iszero(w2))
        return r, e

    # ---- multiply and exact division (reference) ----
    def sext(self, w):                            # 4: the (n-1)-bit value sign-extended: bit G := s
        return w ^ ((w ^ (w >> 1)) & self.G)
    def mul(self, w1, w2):
        """18. Product mod 2^(n-1) when it fits; i*i = -1."""
        F, G = self.F, self.G
        x1, x2 = self.sext(w1), self.sext(w2)     # 8
        m = self.sar(((w1 & w2) << 1) & F, self.n - 1)   # 3  both imaginary
        x1 = ((x1 ^ m) - m) & F                   # 2
        p = (x1 * x2) & F                         # 1
        t = w1 ^ w2                               # 1
        return p ^ ((p ^ t) & G)                  # 3
    def div(self, w1, w2):
        """18. Exact quotient; v/(i u) = -i v/u. Signed divide on sign-extended values: no trap."""
        F, G = self.F, self.G
        x1, x2 = self.sext(w1), self.sext(w2)
        sx = lambda x: x - (1 << self.n) if x >> (self.n - 1) else x
        m = self.sar(((w2 & ~w1) << 1) & F, self.n - 1)  # 3 (2 with andn): a2 = 1, a1 = 0
        q = sx(x1) // sx(x2)                      # exact
        q = q & F
        q = ((q ^ m) - m) & F                     # 2
        t = w1 ^ w2
        return q ^ ((q ^ t) & G)


def check(n, pairs, words):
    Lw = L(n); fails = []
    def bad(name, *a): fails.append((name,) + a)
    for w in words:
        v, a = Lw.val(w), Lw.ax(w)
        want = Lw.enc(-v, a)
        for f in ('neg', 'neg_k', 'neg_sub'):
            if getattr(Lw, f)(w) != want: bad(f, w)
        if Lw.neg_flag(w) != int(not Lw.inrange(-v)): bad('neg_flag', w)
    for w1, w2 in pairs:
        v1, v2, a1, a2 = Lw.val(w1), Lw.val(w2), Lw.ax(w1), Lw.ax(w2)
        if a1 == a2:
            s, d = v1 + v2, v1 - v2
            ws, wd = Lw.enc(s, a1), Lw.enc(d, a1)
            os_, od = int(not Lw.inrange(s)), int(not Lw.inrange(d))
            for f in ('add_bridge', 'add_bridge_b', 'add_carry3', 'add_orand', 'add_pdep'):
                r = getattr(Lw, f)(w1, w2)
                if r != ws: bad(f, w1, w2)
                if Lw.ovf_add(w1, w2, r) != os_: bad('ovf_add/' + f, w1, w2)
                if Lw.ovf_add_andn(w1, w2, r) != os_: bad('ovf_add_andn/' + f, w1, w2)
            if Lw.add_bridge_flag(w1, w2) != os_: bad('add_bridge_flag', w1, w2)
            if Lw.add_carry3_flag(w1, w2) != os_: fails.append(('EXPECTED add_carry3_flag', w1, w2))
            for f in ('sub', 'sub_b'):
                r = getattr(Lw, f)(w1, w2)
                if r != wd: bad(f, w1, w2)
                if Lw.ovf_sub(w1, w2, r) != od: bad('ovf_sub/' + f, w1, w2)
            if Lw.sub_flag(w1, w2) != od: bad('sub_flag', w1, w2)
            p = v1 * v2 * (-1 if a1 and a2 else 1)
            if Lw.inrange(p) and Lw.mul(w1, w2) != Lw.enc(p, a1 ^ a2): bad('mul', w1, w2)
        else:
            p = v1 * v2 * (-1 if a1 and a2 else 1)
            if Lw.inrange(p) and Lw.mul(w1, w2) != Lw.enc(p, a1 ^ a2): bad('mul', w1, w2)
        # exact division
        if v2 != 0 and v1 % v2 == 0:
            q = v1 // v2 * (-1 if (a2 and not a1) else 1)
            if Lw.inrange(q) and Lw.div(w1, w2) != Lw.enc(q, a1 ^ a2): bad('div', w1, w2)
        # full add
        z1, z2 = Lw.iszero(w1), Lw.iszero(w2)
        for f in ('full', 'full_sel'):
            r, e = getattr(Lw, f)(w1, w2)
            if a1 == a2:
                ok = r == Lw.enc(v1 + v2, a1) and e == 0
            elif z1 or z2:
                vv, aa = (v2, a2) if z1 else (v1, a1)
                ok = e == 0 and Lw.val(r) == vv and (Lw.ax(r) == aa or vv == 0)
            else:
                ok = e == 1
            if not ok: bad(f, w1, w2)
    real = [f for f in fails if not f[0].startswith('EXPECTED')]
    exp = len(fails) - len(real)
    return real, exp


def main():
    for n in (4, 6, 8):
        W = range(1 << n)
        real, exp = check(n, [(a, b) for a in W for b in W], W)
        print(f"n={n}: {len(real)} failures; OF of the plain add wrong on {exp} same-axis pairs (expected)")
        for f in real[:10]: print("  ", f)
    n = 64; Lw = L(n); rnd = random.Random(2026)
    special = [0, 1, 2, Lw.C, Lw.C - 1, Lw.G, Lw.S, Lw.S | Lw.C, Lw.S | 1, Lw.F]
    def rw():
        k = rnd.random()
        if k < 0.3: return rnd.choice(special) ^ (rnd.getrandbits(1) << (n - 2))
        if k < 0.5: return (rnd.getrandbits(1) << (n - 1)) | (rnd.getrandbits(1) << (n - 2)) | (Lw.C - rnd.getrandbits(4))
        return rnd.getrandbits(n)
    pairs = []
    for _ in range(200000):
        w1, w2 = rw(), rw()
        if rnd.random() < 0.6: w2 = (w2 & ~Lw.G) | (w1 & Lw.G)        # mostly same axis
        if rnd.random() < 0.02: w2 = (w1 & Lw.S) | (Lw.G ^ (w1 & Lw.G)) & 0 | (rnd.getrandbits(1) << (n - 2))
        pairs.append((w1, w2))
    # exact division needs divisible pairs: build some
    for _ in range(20000):
        a = rnd.getrandbits(rnd.randint(1, 30)) * rnd.choice([1, -1]); b = rnd.getrandbits(rnd.randint(1, 30)) * rnd.choice([1, -1])
        if a == 0: continue
        pairs.append((Lw.enc(a * b, rnd.getrandbits(1)), Lw.enc(a, rnd.getrandbits(1))))
    real, exp = check(n, pairs, [p[0] for p in pairs[:50000]] + special + [s | Lw.G for s in special])
    print(f"n=64 random ({len(pairs)} pairs): {len(real)} failures; plain-add OF wrong on {exp}")
    for f in real[:10]: print("  ", f)
    sys.exit(1 if real else 0)


if __name__ == '__main__':
    main()
