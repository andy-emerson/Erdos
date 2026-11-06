"""Verification bounds needed for larger cycle-length bounds, Gaussian maps
(cycle-bounds.md, Section 3.4).  Planning numbers in double precision; the
script then certifies the bound at a few B with the exact routine of
cyclebound_gauss.py.

For each n, r*(n) is the least r for which n passes both conditions (to first
order in r): max(|V - n th1| ln2 / (2n), |W - n beta| pi / (4n)) minimized over
V and W = V (mod m).  The bound at B is the least n with r*(n) <= r(B), so the
bound reaches N once r(B) < min_{n<N} r*(n), i.e. B > dmax2/(N(alpha) r*^2).

Usage: python3 cyclebound_thresholds.py > logs/cyclebound_thresholds.txt
"""
import numpy as np
import mpmath as mp
import cyclebound_gauss as G

mp.mp.dps = 40


def split(x):
    hi = float(x)
    lo = float(x - mp.mpf(hi))
    return hi, lo


def frac_dist(n, hi, lo, step=1):
    """n*theta - nearest multiple of step (signed), n an int64 array."""
    a = np.mod(n.astype(np.float64) * hi, step) + n.astype(np.float64) * lo
    a = np.mod(a, step)
    return np.where(a > step / 2, a - step, a)


def records(alpha, dmax2, m, nmax):
    a, b = alpha
    N = a * a + b * b
    th1 = mp.log(N) / mp.log(2)
    beta = 4 * mp.atan2(b, a) / mp.pi
    gam = (beta - th1) / m
    t1, g = split(th1), split(gam)
    out = []
    best = np.inf
    chunk = 10**6
    for s in range(1, nmax + 1, chunk):
        n = np.arange(s, min(s + chunk, nmax + 1), dtype=np.int64)
        e1 = frac_dist(n, *t1)              # V - n th1 = -e1
        e2 = frac_dist(n, *g)               # c - n gamma = -e2 ;  W - n beta = (V - n th1) + m (c - n gamma)
        ew = -e1 - m * e2
        rs = np.maximum(np.abs(e1) * 0.6931471805599453 / (2 * n), np.abs(ew) * np.pi / (4 * n))
        prev_min = np.minimum.accumulate(np.concatenate(([best], rs[:-1])))
        idx = np.nonzero(rs < prev_min * (1 - 1e-6))[0]  # multiples kn have the same r* and never become the bound
        for i in idx:
            out.append((int(n[i]), float(rs[i]), dmax2 / (N * float(rs[i]) ** 2)))
        best = min(best, float(rs.min()))
    return out


def main():
    print("# cyclebound_thresholds.py (double precision planning numbers, then exact checks)")
    for label, alpha, dmax2, mode, nmax in [("5+2i", (5, 2), 5, "units", 2 * 10**7),
                                            ("cone 2+i", (2, 1), 1, "units", 10**7)]:
        m = 2 if mode == "units" else 8
        rec = records(alpha, dmax2, m, nmax)
        print("\n%s: n with r*(n) below all earlier r*; the bound is >= the next listed n once B > B*" % label)
        prev = 0.0
        for n, rs, Bs in rec:
            if n > 50:
                print("  n = %-9d r* = %.4e   B* = %.4e   (bound >= this n needs B > %.4e)" % (n, rs, Bs, prev))
            prev = Bs
        print(" exact checks:")
        for B in [3 * 10**9, 4 * 10**9, 5 * 10**9, 10**10, 3 * 10**10, 10**11, 10**12, 10**13, 10**14, 10**15]:
            M = G.Map(label, alpha, B, dmax2, mode)
            nj = M.n_joint(3 * 10**7)
            print("  B = %.1e : box bound n >= %d" % (B, nj[0]))


if __name__ == "__main__":
    main()


def constants(alpha, m, nmin=10, nmax=2 * 10**7):
    """Diophantine constants on [nmin, nmax] (double precision):
    c_s = min n^2 r_s*(n), c_a = min n^2 r_a*(n), c_j = min n^(3/2) r*(n), where r_s*, r_a* are
    the size-only and angle-only thresholds.  Then for B with r(B) small enough the bounds are
    at least min(nmax, (c/r)^(1/2)) for single conditions and min(nmax, (c_j/r)^(2/3)) for both."""
    a, b = alpha
    N = a * a + b * b
    th1 = mp.log(N) / mp.log(2)
    beta = 4 * mp.atan2(b, a) / mp.pi
    gam = (beta - th1) / m
    t1, g, bb = split(th1), split(gam), split(beta)
    cs = ca = cj = np.inf
    for s in range(nmin, nmax + 1, 10**6):
        n = np.arange(s, min(s + 10**6, nmax + 1), dtype=np.int64)
        e1 = frac_dist(n, *t1)
        e2 = frac_dist(n, *g)
        ea = frac_dist(n, *bb)
        rs = np.abs(e1) * 0.6931471805599453 / (2 * n)
        ra = np.abs(ea) * np.pi / (4 * n)
        rj = np.maximum(rs, np.abs(-e1 - m * e2) * np.pi / (4 * n))
        nf = n.astype(np.float64)
        cs = min(cs, float((nf**2 * rs).min()))
        ca = min(ca, float((nf**2 * ra).min()))
        cj = min(cj, float((nf**1.5 * rj).min()))
    return cs, ca, cj


if __name__ == "__main__":
    print("\n# Diophantine constants on 10 <= n <= 2*10^7 (double precision)")
    for label, alpha, m in [("5+2i", (5, 2), 2), ("cone 2+i", (2, 1), 2), ("2-i eps=1", (2, -1), 8),
                            ("-12+3i", (-12, 3), 2), ("7+4i", (7, 4), 2)]:
        print("  %-10s c_size = %.4f  c_angle = %.4f  c_joint = %.4f" % ((label,) + constants(alpha, m)))
