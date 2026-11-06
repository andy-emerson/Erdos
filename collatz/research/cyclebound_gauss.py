"""Exact lower bounds on the number of odd steps of a non-trivial cycle of a
Gaussian Collatz map, from a verification bound, by size alone, angle alone,
both (box) and the log-disc (cycle-bounds.md, Sections 3 and 4).

A cycle up to the unit eps with odd elements z_0, ..., z_{n-1} (z_n = eps z_0),
z_{j+1} pi^{v_j} = alpha z_j + d_j, V = sum v_j, gives
    alpha^n prod (1 + w_j) = eps pi^V,   w_j = d_j / (alpha z_j).
If N(z_j) >= B + 1 for all j, then |w_j| <= r, r^2 = dmax2 / (N(alpha)(B+1)).
Size:  V ln2/2 - n ln N(alpha)/2 in [n ln(1-r), n ln(1+r)]
Angle: W pi/4 - n arg(alpha) in [-n asin r, n asin r], W = V + 2c (eps any unit)
       or W = V + 8c (eps = 1 forced: maps that do not commute, on Z[i]).
Disc:  (V ln2/2 - n ln N/2)^2 + (W pi/4 - n arg alpha)^2 <= (n r/(1-r))^2.
All decisions use outer rational enclosures (certarith); "attained" re-checks
the found n with inner enclosures.

Usage: python3 cyclebound_gauss.py maps   > logs/cyclebound_gauss_maps.txt
       python3 cyclebound_gauss.py scale  > logs/cyclebound_gauss_scale.txt
"""
import sys
import time
from fractions import Fraction
import certarith as C

Q = 200  # dyadic precision for the scan endpoints
LN2 = C.iv_frac(C.ln_rat(2, 1))
PI = C.iv_frac(C.pi_iv())


def dy_floor(x):
    return C.floor_frac(x * (1 << Q))


def dy_ceil(x):
    return C.ceil_frac(x * (1 << Q))


class Map:
    def __init__(self, name, alpha, B, dmax2, mode):
        self.name, self.alpha, self.B, self.dmax2, self.mode = name, alpha, B, dmax2, mode
        a, b = alpha
        self.N = a * a + b * b
        self.m = 2 if mode == "units" else 8
        lnN = C.iv_frac(C.ln_rat(self.N, 1))
        arg = C.iv_frac(C.arg_iv(a, b))
        # theta1 = log2 N, beta = 4 arg / pi
        self.th1 = (lnN[0] / LN2[1], lnN[1] / LN2[0])
        if arg[0] >= 0:
            self.beta = (4 * arg[0] / PI[1], 4 * arg[1] / PI[0])
        elif arg[1] <= 0:
            self.beta = (4 * arg[0] / PI[0], 4 * arg[1] / PI[1])
        else:
            raise ValueError("argument straddles 0")
        rl, rh = C.iv_frac(C.sqrt_frac_iv(dmax2, self.N * (B + 1)))
        self.r = (rl, rh)
        # outer allowances (per odd step), in units of V and of W
        self.s_lo_out = self.th1[0] - 2 * rh / ((1 - rh) * LN2[0])
        self.s_hi_out = self.th1[1] + 2 * rh / LN2[0]
        self.a_half_out = 4 * rh / ((1 - rh * rh) * PI[0])
        self.a_lo_out = self.beta[0] - self.a_half_out
        self.a_hi_out = self.beta[1] + self.a_half_out
        # inner allowances
        self.s_lo_in = self.th1[1] - 2 * rl / LN2[1]
        self.s_hi_in = self.th1[0] + 2 * (rl - rl * rl / 2) / LN2[1]
        a_half_in = 4 * rl / PI[1]
        self.a_lo_in = self.beta[1] - a_half_in
        self.a_hi_in = self.beta[0] + a_half_in
        self.rho_out = rh / (1 - rh)
        self.rho_in = rl  # |log(1+w)| can equal ln(1+r) >= r - r^2/2; used only as a sanity flag

    # single conditions: smallest denominators
    def n_size(self):
        f = C.simplest_in(self.s_lo_out, self.s_hi_out)
        return f.denominator, f.numerator, self.s_lo_in <= f <= self.s_hi_in

    def n_angle(self):
        f = C.simplest_in(self.a_lo_out, self.a_hi_out)
        return f.denominator, f.numerator, self.a_lo_in <= f <= self.a_hi_in

    def n_joint(self, nmax, disc=False):
        """Least n <= nmax with V in n*[s_lo, s_hi], W in n*[a_lo, a_hi], W = V mod m
        (box), and for disc=True also the certified disc test."""
        m = self.m
        if disc:
            rho = self.rho_out
            slo = dy_floor(self.th1[0] - 2 * rho / LN2[0])
            shi = dy_ceil(self.th1[1] + 2 * rho / LN2[0])
            alo = dy_floor(self.beta[0] - 4 * rho / PI[0])
            ahi = dy_ceil(self.beta[1] + 4 * rho / PI[0])
        else:
            slo, shi = dy_floor(self.s_lo_out), dy_ceil(self.s_hi_out)
            alo, ahi = dy_floor(self.a_lo_out), dy_ceil(self.a_hi_out)
        for n in range(1, nmax + 1):
            v0 = -((-n * slo) >> Q)
            v1 = (n * shi) >> Q
            if v0 > v1:
                continue
            w0 = -((-n * alo) >> Q)
            w1 = (n * ahi) >> Q
            if w0 > w1:
                continue
            for V in range(v0, v1 + 1):
                for W in range(w0, w1 + 1):
                    if (W - V) % m:
                        continue
                    if not disc or self.disc_possible(n, V, W):
                        return n, V, W
        return None

    def disc_possible(self, n, V, W):
        """False only if the disc condition certainly fails (outer test)."""
        # |x| >= lower bound, x = (V - n th1) ln2 / 2
        def absmin(lo, hi):
            if lo <= 0 <= hi:
                return Fraction(0)
            return min(abs(lo), abs(hi))
        d1 = (V - n * self.th1[1], V - n * self.th1[0])
        x = absmin(*d1) * LN2[0] / 2
        d2 = (W - n * self.beta[1], W - n * self.beta[0])
        y = absmin(*d2) * PI[0] / 4
        return x * x + y * y <= (n * self.rho_out) ** 2

    def joint_attained(self, n, V, W):
        return (self.s_lo_in * n <= V <= self.s_hi_in * n) and (self.a_lo_in * n <= W <= self.a_hi_in * n)


def heuristic(mp_):
    """Expected first admissible n from the count of admissible n <= M
    (equidistribution): single ~ (1/a)^(1/2), box ~ (3 m / (4 a_s a_a))^(1/3),
    disc ~ (3 m ln2 / (8 rho^2))^(1/3)."""
    r = float(mp_.r[1])
    a_s = 2 * r / 0.6931471805599453
    a_a = 4 * r / 3.141592653589793
    return (1 / a_s) ** 0.5, (1 / a_a) ** 0.5, (3 * mp_.m / (4 * a_s * a_a)) ** (1 / 3), \
        (3 * mp_.m * 0.6931471805599453 / (8 * r * r)) ** (1 / 3)


MAPS = [
    Map("5+2i, (1+i)^4, commuting", (5, 2), 3 * 10**9, 5, "units"),
    Map("7+4i, (1+i)^6, commuting", (7, 4), 10**6, 25, "units"),
    Map("8+3i, (1+i)^6, commuting", (8, 3), 10**6, 25, "units"),
    Map("9+2i, (1+i)^6, commuting", (9, 2), 10**6, 25, "units"),
    Map("10+i, (1+i)^6, commuting", (10, 1), 10**6, 25, "units"),
    Map("10+3i, (1+i)^6, commuting", (10, 3), 10**6, 25, "units"),
    Map("11+2i, (1+i)^6, commuting", (11, 2), 10**6, 25, "units"),
    Map("-11+4i, (1+i)^7, commuting", (-11, 4), 10**9, 49, "units"),
    Map("-12+3i, (1+i)^7, commuting", (-12, 3), 10**9, 49, "units"),
    Map("-11+8i, (1+i)^7, commuting", (-11, 8), 10**9, 49, "units"),
    Map("-12+7i, (1+i)^7, commuting", (-12, 7), 10**9, 49, "units"),
    Map("2+i, (1+i)^2, cone C (on Z[i]/U)", (2, 1), 10**7, 1, "units"),
    Map("2-i, (1+i)^2, D={1,i}, eps=1", (2, -1), 10**7, 1, "one"),
    Map("-1-2i, (1+i)^2, D={1,i}, eps=1", (-1, -2), 10**7, 1, "one"),
    Map("-2-i, (1+i)^2, D={1,i}, eps=1", (-2, -1), 10**7, 1, "one"),
    Map("1+2i, (1+i)^2, D={1,i}, eps=1", (1, 2), 10**7, 1, "one"),
    Map("-2+i, (1+i)^2, D={1,i}, eps=1", (-2, 1), 10**7, 1, "one"),
]


def verdict(ns, na, nj):
    if nj == ns and nj == na:
        return "joint = size = angle"
    if nj == ns:
        return "joint = size (size binding)"
    if nj == na:
        return "joint = angle (angle binding)"
    return "joint exceeds both"


def run_maps():
    print("# cyclebound_gauss.py maps; certarith P = %d bits, scan endpoints dyadic 2^-%d" % (C.P, Q))
    for M in MAPS:
        t0 = time.time()
        ns = M.n_size()
        na = M.n_angle()
        nj = M.n_joint(10**7)
        nd = M.n_joint(10**7, disc=True)
        h = heuristic(M)
        print("\n%s: alpha = %s, N = %d, B = %d, dmax^2 = %d, eps %s" % (
            M.name, M.alpha, M.N, M.B, M.dmax2, "any unit" if M.mode == "units" else "= 1"))
        print("  r = %.6e, theta1 = log2 N = %.12f, beta = 4 arg/pi = %.12f" % (
            float(M.r[1]), float(M.th1[0]), float(M.beta[0])))
        print("  size alone : n >= %d (V = %d), attained %s" % ns)
        print("  angle alone: n >= %d (W = %d), attained %s" % na)
        print("  both (box) : n >= %d (V = %d, W = %d), attained %s" % (nj + (M.joint_attained(*nj),)))
        print("  disc       : n >= %d (V = %d, W = %d)" % nd)
        print("  verdict: %s; joint/max(single) = %.2f" % (verdict(ns[0], na[0], nj[0]), nj[0] / max(ns[0], na[0])))
        print("  heuristic first n: size %.0f, angle %.0f, box %.0f, disc %.0f" % h)
        print("  divisions V >= %d at the box bound; steps of T >= n + V = %d   [%.1f s]" % (
            C.ceil_frac(nj[0] * M.s_lo_out), nj[0] + C.ceil_frac(nj[0] * M.s_lo_out), time.time() - t0))
        sys.stdout.flush()


def run_scale():
    print("# cyclebound_gauss.py scale: 5+2i map and cone map as functions of B")
    for alpha, dmax2, mode, label, ks in [((5, 2), 5, "units", "5+2i", range(3, 21)),
                                           ((2, 1), 1, "units", "cone 2+i", range(3, 19)),
                                           ((2, -1), 1, "one", "2-i eps=1", range(3, 19))]:
        print("\n%s" % label)
        print("  %-6s %-10s %-10s %-10s %-10s %-10s  %s" % ("B", "size", "angle", "box", "disc", "pred box", "verdict"))
        for k in ks:
            M = Map(label, alpha, 10**k, dmax2, mode)
            ns, na = M.n_size()[0], M.n_angle()[0]
            nj = M.n_joint(3 * 10**7)[0]
            nd = M.n_joint(3 * 10**7, disc=True)[0]
            h = heuristic(M)
            print("  1e%-4d %-10d %-10d %-10d %-10d %-10.0f  %s" % (k, ns, na, nj, nd, h[2], verdict(ns, na, nj)))
            sys.stdout.flush()


if __name__ == "__main__":
    {"maps": run_maps, "scale": run_scale}[sys.argv[1]]()
