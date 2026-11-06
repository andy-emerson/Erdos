"""ringwin.py: attribution 2 (window exponent follows q) in Z[(1+sqrt-11)/2] at modulus 4: multipliers with
25.4 < N < 64 (inside the integer-style window (16, 64), above the q = 4 window) against those inside (16, 25.4).
Smallest commuting table of each, started from 200 random odd z with |a|, |b| <= 300; count starts that pass |N| = 2^200
within 20000 odd steps, and the mean of log2 N(S z) - log2 N(z) per odd step over the first 2000 steps.
usage: python3 ringwin.py"""
import random, math
import qring as Q, ringrun as RR, ringtest as RT
R = Q.Ring('Z11'); rnd = random.Random(3); k = 2
L = ["# Z11 modulus 4: multipliers inside and above the window 4^2 < N < 4^(7/3) = 25.4; smallest commuting table"]
cands = {}
for z in R.elements(70):
    if R.odd(z) and 16 < R.N(z) < 64: cands.setdefault(R.N(z), Q.rep(R, 'R', z))
for n in sorted(cands):
    a = cands[n]
    tab = RT.commuting_tables(R, a, k, 1)[0][0]
    pred = math.log2(n) - (k + 1 / 3) * 2
    esc = 0; drift = []
    for _ in range(200):
        z = (rnd.randint(-300, 300), rnd.randint(-300, 300))
        if not R.odd(z): continue
        x = z; s = 0
        for i in range(20000):
            y = RR.apply_S(R, a, tab, k, x, 'R')[0]
            if y == (0, 0): break
            if i < 2000: drift.append(math.log2(R.N(y)) - math.log2(R.N(x)))
            x = y
            if R.N(x) > 2 ** 200: esc += 1; break
    L.append(f"N={n} alpha={Q.fmt(R, a)}: predicted drift {pred:+.3f} bits, measured {sum(drift)/len(drift):+.3f}; starts past 2^200: {esc}")
RT.log(R, 'window_test', L)
