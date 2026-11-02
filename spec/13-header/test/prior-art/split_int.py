"""split_int.py: checks for prior-art.md's round 2 split-layout integer rows (axis bit always 0, d in {0, 2}).
Word w = s*2^(R-1) + 2m; F = bits 1..R-2; S = the sign bit.

  multiply: r = ((w1 * (w2 AND F)) shr 1) OR ((w1 XOR w2) AND S)          6   (w1's sign term s1*2^(R-1)*2m2 vanishes mod 2^R)
  divide:   r = ((w1 shl 1) /u (w2 AND F)) OR ((w1 XOR w2) AND S)         6   (4m1 / 2m2 = 2q)

Exhaustive on R = 8 and 10, random on R = 64. Also checks that the multiply form fails for complex operands
(it needs a = 0), so the restriction is real.

  python3 -I split_int.py
"""
import random
import sys


def run(R, cases):
    N = 1 << R; Fm = N - 1; S = 1 << (R - 1); F = S - 2
    enc = lambda m, s: (s << (R - 1)) | (m << 1)
    dec = lambda w: ((w >> 1) & ((1 << (R - 2)) - 1), w >> (R - 1))
    bad = 0
    for m1, s1, m2, s2 in cases:
        w1, w2 = enc(m1, s1), enc(m2, s2)
        sv = s1 ^ s2
        if m1 * m2 < (1 << (R - 2)):
            r = (((w1 * (w2 & F)) & Fm) >> 1) | ((w1 ^ w2) & S)
            bad += dec(r) != (m1 * m2, sv)
        if m2 and m1 % m2 == 0:
            r = (((w1 << 1) & Fm) // (w2 & F)) | ((w1 ^ w2) & S)
            m, s = dec(r)
            bad += m != m1 // m2 or (m1 and s != sv)
    return bad


def complex_counterexample(R):
    S = 1 << (R - 1); F = S - 2; Fm = (1 << R) - 1
    w1 = (1 << 1) | 1; w2 = (1 << 1) | 1                 # i * i, magnitudes 1
    r = (((w1 * (w2 & F)) & Fm) >> 1) | ((w1 ^ w2) & S)
    return r != (S | 2)                                     # want -1: sign set, magnitude 1, axis 0


if __name__ == "__main__":
    random.seed(5)
    ok = True
    for R in (8, 10):
        M = (1 << (R - 2)) - 1
        cases = [(m1, s1, m2, s2) for m1 in range(M + 1) for m2 in range(M + 1) for s1 in (0, 1) for s2 in (0, 1)]
        b = run(R, cases); ok &= b == 0
        print("R = %d, %d pairs: %s" % (R, len(cases), "ok" if b == 0 else "%d failures" % b))
    cases = []
    for _ in range(300000):
        m2 = random.getrandbits(random.randint(1, 31)) or 1; q = random.getrandbits(random.randint(1, 31))
        m1 = q * m2 if random.random() < 0.7 else random.getrandbits(31)
        cases.append((m1, random.randrange(2), m2, random.randrange(2)))
    b = run(64, cases); ok &= b == 0
    print("R = 64, %d random pairs: %s" % (len(cases), "ok" if b == 0 else "%d failures" % b))
    ce = complex_counterexample(64); ok &= ce
    print("complex operands break the multiply form (as expected):", ce)
    sys.exit(0 if ok else 1)
