"""header_gray.py: checks for prior-art.md's "Round 2 research: header add" (the Gray-and-spread header).

The header byte is G(d) = 0xFF*s XOR 0x40*a (d = 2s + a): d=0 -> 0x00, 1 -> 0x40, 2 -> 0xFF, 3 -> 0xBF.
Bit 7 is the sign; bit 6 is s XOR a (bits 7-6 are the Gray code of d); bits 0-5 copy the sign. So G(d+2) = NOT G(d),
and two headers loaded sign-extended (lb, ldrsb, movsx) XOR to an exact mask (0 or all ones) whenever the axes agree.
The magnitude is the whole register (R bits; 64 in the design).

Exhaustive on R = 8 (8-bit magnitudes, the full register, every direction and magnitude pair, add and subtract),
random on R = 64. Expressions must set the flag; overflowing inputs are skipped; a zero may have any direction.

  python3 -I header_gray.py
"""
import random
import sys

I = [(1, 0), (0, 1), (-1, 0), (0, -1)]
def val(m, d): return (m * I[d][0], m * I[d][1])
def gmul(x, y): return (x[0]*y[0] - x[1]*y[1], x[0]*y[1] + x[1]*y[0])
G = [0x00, 0x40, 0xFF, 0xBF]
UNG = {g: d for d, g in enumerate(G)}


class Reg:
    def __init__(self, R):
        self.R = R; self.N = 1 << R; self.F = self.N - 1
    def sar(self, x, k): return ((x - self.N if x >> (self.R - 1) else x) >> k) & self.F
    def lb(self, byte): return (byte | (self.F ^ 0xFF)) if byte & 0x80 else byte     # sign-extending byte load
    def spread7(self, x): return self.sar((x << (self.R - 8)) & self.F, self.R - 1)  # bit 7 -> every bit

    # ---------------- the formulas, exactly (e1, e2 = lb(header); m1, m2 = magnitude words) ------------------------
    def add(self, e1, m1, e2, m2):
        F = self.F
        t = e1 ^ e2                                              # 1
        r = (m1 + (m2 ^ t) - t) & F                              # 3   m1 + m2, or m1 - m2 mod 2^R
        L = (0 - int(m1 < m2)) & F                               # 2
        b = t & L                                                # 1
        r = ((r + b) ^ b) & F                                    # 2   undoes a wrap; on different axes undoes the first step
        h = (e1 ^ b) & 0xFF                                      # 1   stored byte: G of the larger operand's direction
        return h, r, b
    def flag(self, e1, m1, e2, m2):                              # expression flag, complex only: 6
        t = e1 ^ e2
        return ((t >> 6) ^ t) & int(m1 != 0) & int(m2 != 0) & 1  # bit 6 of t XOR bit 0 of t = axes differ
    def sub(self, e1, m1, e2, m2): return self.add(e1, m1, e2 ^ self.F, m2)   # NOT e2 (Zbb xnor / ARM eon fold it into t)
    def neg(self, e): return (e ^ 0xFF) & 0xFF                   # 1   NOT
    def mul_h(self, e1, e2):                                     # 7
        F = self.F
        u1 = (e1 + 0x40) & F; u2 = (e2 + 0x40) & F              # 2   bit 7 of u = the axis a
        return ((e1 ^ e2) ^ self.spread7(u1 & u2)) & 0xFF       # 5   t XOR mask(a1 AND a2): times -1 when both imaginary
    def div_h(self, e1, e2):                                     # 8 (7 with andn / bic)
        F = self.F
        u1 = (e1 + 0x40) & F; u2 = (e2 + 0x40) & F
        return ((e1 ^ e2) ^ self.spread7(u2 & ~u1 & F)) & 0xFF  # t XOR mask(a2 AND NOT a1)


def check(R, cases):
    g = Reg(R); bad = {}
    def fail(nm, *a):
        bad[nm] = bad.get(nm, 0) + 1
        if bad[nm] == 1: print("FAIL", R, nm, a)
    for d1, m1, d2, m2 in cases:
        e1, e2 = g.lb(G[d1]), g.lb(G[d2])
        v1, v2 = val(m1, d1), val(m2, d2)
        for sub in (0, 1):
            vv2 = (-v2[0], -v2[1]) if sub else v2
            v = (v1[0] + vv2[0], v1[1] + vv2[1])
            h, r, _ = (g.sub if sub else g.add)(e1, m1, e2, m2)
            fl = g.flag(e1, m1, e2 ^ g.F if sub else e2, m2)
            if v[0] and v[1]:
                if fl != 1: fail("flag", d1, m1, d2, m2, sub)
                continue
            if abs(v[0]) + abs(v[1]) >= g.N: continue
            if fl != 0 or h not in UNG or val(r, UNG[h]) != v: fail("add/sub", d1, m1, d2, m2, sub, h, r)
        if g.neg(G[d1]) != G[(d1 + 2) % 4]: fail("neg", d1)
        if g.mul_h(e1, e2) != G[(d1 + d2) % 4]: fail("mul header", d1, d2)
        if g.div_h(e1, e2) != G[(d1 - d2) % 4]: fail("div header", d1, d2)
        if m1 * m2 < g.N and val((m1 * m2), UNG[g.mul_h(e1, e2)]) != gmul(v1, v2): fail("mul", d1, m1, d2, m2)
        if m2 and m1 % m2 == 0:
            q = m1 // m2
            if m1 and gmul(val(q, UNG[g.div_h(e1, e2)]), v2) != v1: fail("div", d1, m1, d2, m2)
    return bad


if __name__ == "__main__":
    random.seed(21)
    ex = [(d1, m1, d2, m2) for d1 in range(4) for d2 in range(4) for m1 in range(256) for m2 in range(256)]
    b8 = check(8, ex)
    print("R = 8, %d pairs: %s" % (len(ex), b8 or "ok"))
    M = (1 << 64) - 1
    def rm():
        if random.random() < 0.2: return random.choice([0, 1, 2, M, M - 1, 1 << 63])
        return random.getrandbits(random.randint(1, 64))
    rc = []
    for _ in range(300000):
        d1 = random.randrange(4); d2 = random.randrange(4)
        if random.random() < 0.5: d2 = random.choice([d1, (d1 + 2) % 4])
        m1 = rm(); m2 = rm()
        x = random.random()
        if x < 0.2: m2 = max(0, min(M, m1 + random.randint(-2, 2)))
        elif x < 0.4:
            m1 = random.getrandbits(32); m2 = random.getrandbits(32) or 1
            if random.random() < 0.5: m1 *= m2
        rc.append((d1, m1, d2, m2))
    b64 = check(64, rc)
    print("R = 64, %d random pairs: %s" % (len(rc), b64 or "ok"))
    sys.exit(0 if not b8 and not b64 else 1)
