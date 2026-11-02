# Exhaustive checks of the prior-art candidates (spec 13, header and packed forms).
# Registers are R bits wide; arithmetic wraps mod 2^R; sar is arithmetic shift right.
import sys

I = [(1, 0), (0, 1), (-1, 0), (0, -1)]
def val(m, d): return (m * I[d][0], m * I[d][1])

def run(R):
    N = 1 << R; F = N - 1
    sar = lambda x, k: ((x - N if x >> (R - 1) else x) >> k) & F
    sx8 = lambda h: (h - 256) & F if h & 0x80 else h          # sign-extending byte load (lb/ldrsb/movsx)
    bad = {}
    def fail(name, *a):
        bad[name] = bad.get(name, 0) + 1
        if bad[name] == 1: print("FAIL", R, name, a)

    # ---------------- header form (magnitude = whole register; header byte, d in bits 6-7) -----------
    def h_add(h1, m1, h2, m2, lb):
        if lb:
            t = sx8(h1) ^ sx8(h2); s = sar(t, R - 1)             # 1 op for s
        else:
            t = h1 ^ h2; s = (0 - (t >> 7)) & F                  # 2 ops for s
        y = m2 ^ s; r = (m1 + y - s) & F
        lt = int(m1 < m2); L = (0 - lt) & F
        mb = s & L; r = ((r ^ mb) - mb) & F
        hd = (h1 ^ (t & L)) & 0xFF
        err = (t >> 6) & int(m1 != 0) & int(m2 != 0) & 1
        return hd, r, err
    def h_add_q(h1, m1, h2, m2):                                 # variant: (m1^mb) + (m2^q) - s
        t = h1 ^ h2; s = (0 - (t >> 7)) & F
        L = (0 - int(m1 < m2)) & F; mb = s & L; q = s & ~L & F
        r = ((m1 ^ mb) + (m2 ^ q) - s) & F
        return (h1 ^ (t & L)) & 0xFF, r, (t >> 6) & int(m1 != 0) & int(m2 != 0) & 1
    def check_add(name, f, dec1, dec2, enc_ok, sub=False):
        pass

    if R == 8:
        for d1 in range(4):
            for d2 in range(4):
                h1, h2 = d1 << 6, d2 << 6
                for m1 in range(N):
                    for m2 in range(N):
                        for sub in (0, 1):
                            v1 = val(m1, d1); v2 = val(m2, d2)
                            if sub: v2 = (-v2[0], -v2[1])
                            v = (v1[0] + v2[0], v1[1] + v2[1])
                            expr = v[0] != 0 and v[1] != 0
                            mag = abs(v[0]) + abs(v[1])
                            hh2 = h2 ^ 0x80 if sub else h2
                            for name, res in (("H-add lbu", h_add(h1, m1, hh2, m2, False)),
                                              ("H-add lb", h_add(sx8(h1) & 0xFF if False else h1, m1, hh2, m2, True)),
                                              ("H-add q", h_add_q(h1, m1, hh2, m2))):
                                hd, r, err = res
                                if expr:
                                    if err != 1: fail(name, d1, m1, d2, m2, sub)
                                    continue
                                if mag >= N: continue                       # overflow: outside precondition
                                if err != 0 or val(r, hd >> 6) != v or hd & 0x3F: fail(name, d1, m1, d2, m2, sub, res)
                        # multiply / divide / negate, header
                        p = m1 * m2
                        if p < N:
                            hd = (h1 + h2) & 0xFF
                            a, b = val(m1, d1), val(m2, d2)
                            if val(p, hd >> 6) != (a[0]*b[0]-a[1]*b[1], a[0]*b[1]+a[1]*b[0]): fail("H-mul", d1, m1, d2, m2)
                        if m2 and m1 % m2 == 0:
                            hd = (h1 - h2) & 0xFF; q = m1 // m2
                            # check q*i^hd times m2*i^d2 = m1*i^d1
                            if (q * m2, ((hd >> 6) + d2) % 4) != (m1, d1) and not (m1 == 0): fail("H-div", d1, m1, d2, m2)
                if val(m1, ((h1 ^ 0x80) >> 6)) != tuple(-x for x in val(m1, d1)): fail("H-neg", d1)

    # ---------------- packed form: R-bit word, (R-2)-bit magnitude ------------------------------------
    n = R - 2; M = (1 << n) - 1; NOTM = F & ~M; SB_TOP = 1 << (R - 1)
    enc_top = lambda m, d: (d << n) | m
    dec_top = lambda w: (w & M, w >> n)
    enc_bot = lambda m, d: (m << 2) | d
    dec_bot = lambda w: (w >> 2, w & 3)

    def p_add_top(w1, w2, borrowtrick=False):
        t = w1 ^ w2; s = sar(t, R - 1)
        if borrowtrick:
            L = sar((((w1 - w2) ^ t) << 1) & F, R - 1)          # bit n of (w1-w2) xor t is the borrow out of the magnitudes
        else:
            L = sar(((w1 & M) - (w2 & M)) & F, R - 1)
        tL = t & L; W = w1 ^ tL; V = W ^ t; mn = V & M
        Rw = (W + (mn ^ s) - s) & F
        err = (t >> n) & int(mn != 0) & 1
        return Rw, err
    def p_add_bot(w1, w2):
        t = w1 ^ w2; s = sar((t << (R - 2)) & F, R - 1)
        L = (0 - int(w1 < (w2 & ~3 & F))) & F
        tL = t & L; W = w1 ^ tL; V = W ^ t; mn = V & ~3 & F
        Rw = (W + (mn ^ s) - s) & F
        err = t & 1 & int(mn != 0)
        return Rw, err
    def p_mul_top(w1, w2):
        a = w1 & M; b = w2 & M; p = (a * b) & F
        return p | (((w1 ^ a) + (w2 ^ b)) & F)
    def p_mul_top_lea(w1, w2):                                  # D = (w1 + w2 - 1) and not M; wrong direction only on zero products
        a = w1 & M; b = w2 & M; p = (a * b) & F
        return p | ((w1 + w2 - 1) & NOTM)
    def p_mul_bot(w1, w2):
        return (((w1 >> 2) * (w2 & ~3 & F)) & F) | ((w1 + w2) & 3)
    def p_div_top(w1, w2):
        a = w1 & M; b = w2 & M
        return (a // b) | ((w1 - w2) & NOTM)
    def p_div_bot(w1, w2):
        return (((w1 & ~3 & F) // (w2 >> 2)) & F) | ((w1 - w2) & 3)

    for name, enc, dec, sb, add in (("P-top", enc_top, dec_top, SB_TOP, p_add_top),
                                    ("P-top-bt", enc_top, dec_top, SB_TOP, lambda a, b: p_add_top(a, b, True)),
                                    ("P-bot", enc_bot, dec_bot, 2, p_add_bot)):
        for d1 in range(4):
            for d2 in range(4):
                for m1 in range(M + 1):
                    for m2 in range(M + 1):
                        w1, w2 = enc(m1, d1), enc(m2, d2)
                        for sub in (0, 1):
                            v1 = val(m1, d1); v2 = val(m2, d2)
                            if sub: v2 = (-v2[0], -v2[1])
                            v = (v1[0] + v2[0], v1[1] + v2[1])
                            expr = v[0] != 0 and v[1] != 0
                            Rw, err = add(w1, w2 ^ sb if sub else w2)
                            if expr:
                                if err != 1: fail(name + " add", d1, m1, d2, m2, sub)
                                continue
                            if abs(v[0]) + abs(v[1]) > M: continue
                            m, d = dec(Rw)
                            if err or val(m, d) != v: fail(name + " add", d1, m1, d2, m2, sub, Rw)
                        if name == "P-top-bt": continue
                        # neg
                        m, d = dec(w1 ^ sb)
                        if val(m, d) != tuple(-x for x in val(m1, d1)): fail(name + " neg", d1, m1)
                        # mul
                        a, b = val(m1, d1), val(m2, d2)
                        true = (a[0]*b[0]-a[1]*b[1], a[0]*b[1]+a[1]*b[0])
                        if m1 * m2 <= M:
                            fs = [(p_mul_top, "mul"), (p_mul_top_lea, "mul-lea")] if name == "P-top" else [(p_mul_bot, "mul")]
                            for f, nm in fs:
                                m, d = dec(f(w1, w2))
                                if val(m, d) != true: fail(name + " " + nm, d1, m1, d2, m2)
                        if m2 and m1 % m2 == 0:
                            f = p_div_top if name == "P-top" else p_div_bot
                            m, d = dec(f(w1, w2))
                            if m != m1 // m2 or (m1 and (m * m2, (d + d2) % 4) != (m1, d1)): fail(name + " div", d1, m1, d2, m2)
    print("R =", R, "failures:", bad if bad else "none")

for R in map(int, sys.argv[1:]): run(R)
