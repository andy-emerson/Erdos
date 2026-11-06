"""realwin_extra.py: supplements to realwin.py (real-quadratic.md, Section 3).

1. Unit multipliers alpha = eps^m (|N| = 1, far below the norm window) read literally: predicted and measured
   drift per embedding and fates.
2. Slopes do not depend on the digits: random tables (digits among the 3 smallest) for a few multipliers.
3. The exception alpha = 5 in Z[(1+sqrt5)/2] at modulus 4: the line b = 1 (z = a + g) is invariant under the
   smallest table and every step on it has v = k exactly, so |a| grows by 5/4 per step.
log: logs/realq_window_extra.txt
"""
import random, collections
import realq as Q, realwin as W

L = []
def out(s): print(s); L.append(s)


def main():
    rnd = random.Random(11)
    W.NS = 40; W.CAP = 3000
    out("# 1. unit multipliers alpha = eps^m, smallest tables, literal on R")
    for nm, ks in (('Zr2', (1, 2)), ('Zr3', (1, 2)), ('Zg5', (1, 2))):
        R = Q.RQ(nm)
        for k in ks:
            for m in range(-3, 4):
                a = R.pw(R.eps, m)
                if not R.odd(a): continue
                D = R.window_drifts(a, k)
                tab = W.smallest_table(R, a, k)
                sl, mv, fate, eb, st = W.run(R, a, tab, k, rnd)
                out(f"{nm} k={k} alpha=eps^{m}={R.fmt(a)} D=({D[0]:+.3f},{D[1]:+.3f}) slope=({sl[0]:+.3f},{sl[1]:+.3f}) "
                    f"v={mv:.3f} fates {dict(fate)} {dict(eb) if eb else ''}")
    out("\n# 2. random tables (digits among the 3 smallest per class): slopes against D")
    for nm, k, a in (('Zr2', 4, (5, 2)), ('Zr2', 4, (1, 3)), ('Zr3', 1, (3, 2)), ('Zr3', 3, (31, 18)), ('Zg5', 2, (4, 1)), ('Zg5', 3, (8, 1))):
        R = Q.RQ(nm); D = R.window_drifts(a, k)
        for t in range(3):
            tab = W.smallest_table(R, a, k, nsmall=3, rnd=rnd)
            sl, mv, fate, eb, st = W.run(R, a, tab, k, rnd)
            out(f"{nm} k={k} alpha={R.fmt(a)} table {t}: D=({D[0]:+.3f},{D[1]:+.3f}) slope=({sl[0]:+.3f},{sl[1]:+.3f}) "
                f"v={mv:.3f} fates {dict(fate)}")
    out("\n# 3. Zg5, alpha = 5, modulus 4, smallest table: the line b = 1")
    R = Q.RQ('Zg5'); k = 2; a = (5, 0)
    tab = W.smallest_table(R, a, k)
    cls_b1 = [i for i in R.odd_classes(k) if R.classrep(i, k)[1] % 4 == 1]
    out("digits on the classes with b = 1 mod 4: " + ", ".join(f"[{R.fmt(R.classrep(i, k))}]->{R.fmt(tab[i])}" for i in cls_b1))
    ok = True; vs = collections.Counter()
    for x in range(-2000, 2001):
        z = (x, 1)
        w, v = R.S(a, tab, k, z); vs[v] += 1
        if w[1] != 1: ok = False
    out(f"for z = a + g, |a| <= 2000: S(z) has b = 1 in every case: {ok}; valuations {dict(vs)}")
    z = (10 ** 6 + 1, 1); n = 0
    while n < 200: z, v = R.S(a, tab, k, z); n += 1
    out(f"orbit of 10^6+1+g after 200 steps: a = {z[0]:.4e}, b = {z[1]}, log2 growth per step {(R.lg(z)[0] - R.lg((10**6+1, 1))[0]) / 200:.4f} (log2(5/4) = 0.3219)")
    open('logs/realq_window_extra.txt', 'w').write("\n".join(L) + "\n")


if __name__ == '__main__': main()
