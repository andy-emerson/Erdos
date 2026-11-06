"""Averaged box and disc bounds for the 5+2i map at chosen B (cycle-bounds-refined.md, Section 4.3).
c = 935255468522/2^40 is the certified constant of cyclebound_avg.py (eta = 1/4096, U = 64; it was computed
at B = 3e9 and is valid for every B >= 3e9, see Section 3.4).
Usage: python3 cyclebound_avg_B.py > logs/cyclebound_avg_B.txt; python3 cyclebound_avg_B.py bisect >> logs/cyclebound_avg_B.txt"""
import sys
from fractions import Fraction
import cyclebound_gauss as G

cG = Fraction(935255468522, 1 << 40)


def averaged(B, c):
    M = G.Map("5+2i", (5, 2), B, 5, "units")
    rl, rh = M.r
    M.s_lo_out = M.th1[0] - 2 * c * rh / ((1 - rh) * G.LN2[0])
    M.s_hi_out = M.th1[1] + 2 * c * rh / G.LN2[0]
    M.a_half_out = 4 * c * rh / ((1 - rh * rh) * G.PI[0])
    M.a_lo_out = M.beta[0] - M.a_half_out
    M.a_hi_out = M.beta[1] + M.a_half_out
    M.rho_out = c * rh / (1 - rh)
    return M


def bisect_jumps():
    """B at which the bound leaves 8696 (basic disc, averaged disc, averaged box), to 0.1 percent."""
    def f(B, avg, disc):
        M = averaged(B, cG) if avg else G.Map("5+2i", (5, 2), B, 5, "units")
        return M.n_joint(3 * 10**5, disc=disc)[0]
    for avg, disc, lo, hi in [(False, True, 5 * 10**12, 8 * 10**12), (True, True, 5 * 10**12, 8 * 10**12),
                              (True, False, 9 * 10**12, 96 * 10**11)]:
        a = f(lo, avg, disc)
        while hi - lo > lo // 1000:
            mid = (lo + hi) // 2
            if f(mid, avg, disc) == a:
                lo = mid
            else:
                hi = mid
        print("avg", avg, "disc", disc, ": bound", a, "up to B=%.4e, then %d at B=%.4e" % (lo, f(hi, avg, disc), hi))


if __name__ == "__main__" and sys.argv[1:] == ["bisect"]:
    bisect_jumps()
elif __name__ == "__main__":
  for B in [13 * 10**10, 10**12, 5 * 10**12, 8 * 10**12, 9 * 10**12,
                                                        9.5e12, 9.6e12, 9.7e12, 10**13, 1.32e13]:
        B = int(B)
        base = G.Map("5+2i", (5, 2), B, 5, "units")
        M = averaged(B, cG)
        print("B = %.4e: basic box %d, basic disc %d; averaged box %d, averaged disc %d" % (
            B, base.n_joint(3 * 10**5)[0], base.n_joint(3 * 10**5, disc=True)[0],
            M.n_joint(3 * 10**5)[0], M.n_joint(3 * 10**5, disc=True)[0]))
        sys.stdout.flush()
