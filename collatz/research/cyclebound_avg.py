"""Averaged refinement of the cycle-length bounds (cycle-bounds-refined.md, Sections 2 to 4).

The basic method bounds every per-step error by its worst value, 1/(7X) for
7x +- 1 and r for the 5+2i map.  The averaged method bounds the SUM of the
per-step errors over the cycle, sum_j X/|x_j| <= c n (resp. sum_j sqrt(B+1)/|z_j|
<= c n), with a certified constant c < 1, obtained as follows.

Heights.  u_j = |x_j|/X >= 1 (resp. |z_j|/sqrt(B+1) >= 1) for every odd element.
One odd step with valuation v moves u into
    7x+-1 : [(7u - 1/X)/2^v, (7u + 1/X)/2^v],                     v >= 2
    3x+1  : [3u/2^v, (3u + 1/X)/2^v],                              v >= 1 (positive cycles)
    5+2i  : [(sqrt29 u - sqrt5/sqrt(B+1))/2^(v/2), (... + ...)/2^(v/2)], v >= 4.
Bin graph.  [1, U) is cut into bins [e_k, e_{k+1}) (geometric, ratio about 1+eta),
plus a top bin [U, oo).  There is an edge k -> j if some u in bin k can step
into bin j for some v (computed with outward integer roundings, so every real
step is an edge).  The weight of bin k is an upper bound for 1/u on it.
The odd elements of a cycle give a closed walk, so the mean of 1/u_j over the
cycle is at most the maximum cycle mean of the graph.
Certificate.  For any potential g on the bins, lam(g) = max_k (w_k + max_{k->j} g_j - g_k)
bounds every cycle mean: along an edge k -> j, w_k - lam <= g_k - g_j, and the
right side telescopes to 0 around a closed walk.  g is found by (damped) value
iteration in floating point, then rounded to integers at scale 2^SC and lam is
recomputed in exact integer arithmetic.  Only that last step is the proof.

Usage: python3 cyclebound_avg.py > logs/cyclebound_avg.txt
"""
import sys
import time
from fractions import Fraction
from math import isqrt, log
import numpy as np
import certarith as C

S = 1 << 40      # scale of bin endpoints (u = E / S)
SC = 40          # scale of the integer potential


def sqrt_lo_hi(num, den, q):
    """floor and ceil of sqrt(num/den) * 2^q, as integers."""
    x = (num << (2 * q)) // den
    s = isqrt(x)
    return s, s + 2


def make_bins(eta, U):
    E = [S]
    while E[-1] < U * S:
        nxt = int(E[-1] * (1 + eta))
        if nxt <= E[-1]:
            nxt = E[-1] + 1
        E.append(nxt)
    return E  # bins 0..K-1 are [E_k, E_{k+1}); bin K = [E_K, oo)


def bin_of(E, y):
    """index of the bin containing the integer y (y >= E[0]); np.searchsorted on the list."""
    return int(np.searchsorted(E, y, side="right")) - 1


class Steps:
    """Outward-rounded step maps, as integer affine bounds on scaled heights.
    For valuation v: lower(E) = floor(E * flo / 2^Q) - off, upper(E) = ceil(E * fhi / 2^Q) + off."""
    Q = 80

    def __init__(self, kind, bound):
        Q = self.Q
        self.kind = kind
        self.items = []  # (v, flo, fhi, off)
        if kind == "7x+-1":
            X = bound
            for v in range(2, 64):
                flo = (7 << Q) >> v
                fhi = -((-(7 << Q)) >> v)
                off = -((-S) // (X << v)) + 1       # S/(X 2^v) rounded up, plus 1
                self.items.append((v, flo, fhi, off))
        elif kind == "3x+1":
            X = bound
            for v in range(1, 64):
                flo = (3 << Q) >> v
                fhi = -((-(3 << Q)) >> v)
                off = -((-S) // (X << v)) + 1
                self.items.append((v, flo, fhi, off))
        elif kind == "5+2i":
            B = bound
            s29lo, s29hi = sqrt_lo_hi(29, 1, Q)
            for v in range(4, 128):
                # 2^(v/2) = 2^(v//2) * (sqrt2 if v odd)
                if v % 2 == 0:
                    tlo = thi = 1 << (Q + v // 2)
                else:
                    a, b = sqrt_lo_hi(2, 1, Q)
                    tlo, thi = a << (v // 2), b << (v // 2)
                flo = (s29lo << Q) // thi           # <= sqrt29/2^(v/2) * 2^Q
                fhi = -((-(s29hi << Q)) // tlo)
                # offset S * sqrt(5/(B+1)) / 2^(v/2), rounded up
                o_lo, o_hi = sqrt_lo_hi(5 * S * S, B + 1, Q)   # >= S sqrt(5/(B+1)) 2^Q
                off = -((-o_hi) // tlo) + 1                     # >= S sqrt(5/(B+1)) / 2^(v/2), plus 1
                self.items.append((v, flo, fhi, off))
        else:
            raise ValueError(kind)

    def ranges(self, E, k):
        """List of (lo_bin, hi_bin) target ranges from bin k."""
        Q = self.Q
        K = len(E) - 1
        out = []
        top = (k == K)
        lo_e = E[k]
        hi_e = None if top else E[k + 1]
        for v, flo, fhi, off in self.items:
            L = ((lo_e * flo) >> Q) - off - 1
            if top:
                H = None
            else:
                H = -((-(hi_e * fhi)) >> Q) + off + 1
                if H < E[0]:
                    break           # larger v only go lower
            jl = 0 if L < E[0] else bin_of(E, L)
            jh = K if H is None else bin_of(E, H)
            out.append((jl, jh))
            if top and L < E[0]:
                break               # from the top bin every bin is reachable once L < 1
        return out


def build(kind, bound, eta, U):
    E = make_bins(eta, U)
    K = len(E) - 1
    st = Steps(kind, bound)
    src, lo, hi = [], [], []
    for k in range(K + 1):
        for jl, jh in st.ranges(E, k):
            src.append(k)
            lo.append(jl)
            hi.append(jh)
    # integer weights: ceil(S * 2^SC / E_k) >= 2^SC / u for u >= E_k / S
    Wint = np.array([-((-(S << SC)) // e) for e in E], dtype=np.int64)
    return E, np.array(src), np.array(lo), np.array(hi), Wint


class RangeMax:
    def __init__(self, n):
        self.n = n
        self.levels = int(np.floor(np.log2(max(n, 1)))) + 1

    def table(self, g):
        T = [g]
        for l in range(1, self.levels):
            prev = T[-1]
            w = 1 << (l - 1)
            T.append(np.maximum(prev[:-w], prev[w:]))
        return T

    @staticmethod
    def query(T, lo, hi):
        ln = hi - lo + 1
        l = np.floor(np.log2(ln)).astype(np.int64)
        res = np.empty(len(lo), dtype=T[0].dtype)
        for lev in np.unique(l):
            idx = np.nonzero(l == lev)[0]
            tab = T[lev]
            res[idx] = np.maximum(tab[lo[idx]], tab[hi[idx] - (1 << lev) + 1])
        return res


def bellman(g, src, lo, hi, W, rm, K):
    """(T g)(k) = W_k + max over edges k -> j of g_j."""
    T = rm.table(g)
    vals = RangeMax.query(T, lo, hi)
    best = np.full(K + 1, np.iinfo(np.int64).min if g.dtype == np.int64 else -np.inf, dtype=g.dtype)
    np.maximum.at(best, src, vals)
    return W + best


def maxmean(kind, bound, eta=1 / 2048, U=32, iters=6000):
    t0 = time.time()
    E, src, lo, hi, Wint = build(kind, bound, eta, U)
    K = len(E) - 1
    rm = RangeMax(K + 1)
    W = Wint.astype(np.float64) / 2.0**SC
    g = np.zeros(K + 1)
    lam_hist = []
    for it in range(iters):
        Tg = bellman(g, src, lo, hi, W, rm, K)
        d = Tg - g
        lam_hist.append((d.max(), d.min()))
        g = 0.5 * g + 0.5 * (Tg - d.max())
        g -= g.max()
        if d.max() - d.min() < 1e-12:
            break
    # certification in exact integers
    G = np.round(g * 2.0**SC).astype(np.int64)
    TG = bellman(G, src, lo, hi, Wint, rm, K)
    lam_num = int((TG - G).max())
    lam = Fraction(lam_num, 1 << SC)
    # a lower estimate (float): the min of T g - g bounds the max cycle mean from below
    lower = lam_hist[-1][1]
    # a near-optimal closed walk: follow the best successor (float) until a bin repeats
    T = rm.table(g)
    vals = RangeMax.query(T, lo, hi)
    succ = {}
    for e in range(len(src)):
        k = int(src[e])
        if k not in succ or vals[e] > succ[k][0]:
            # best bin inside the range
            j = int(lo[e] + np.argmax(g[lo[e]:hi[e] + 1]))
            succ[k] = (vals[e], j)
    k = int(np.argmax(Tg - g))
    seen, path = {}, []
    while k not in seen:
        seen[k] = len(path)
        path.append(k)
        k = succ[k][1]
    cyc = path[seen[k]:]
    us = [E[b] / S for b in cyc]
    walk = dict(length=len(cyc), mean=float(np.mean([1 / u for u in us])), umin=min(us), umax=max(us))
    return dict(lam=lam, lower=lower, K=K, edges=len(src), iters=it + 1, secs=time.time() - t0, E=E, walk=walk)


def averaged_7(X, c):
    """Outer enclosure of the averaged interval for V/n, 7x +- 1:
    V ln2 - n ln7 in [-c n/(7X - 1), c n/(7X)]."""
    ln2 = C.iv_frac(C.ln_rat(2, 1))
    ln7 = C.iv_frac(C.ln_rat(7, 1))
    th = (ln7[0] / ln2[1], ln7[1] / ln2[0])
    lo = th[0] - c / ((7 * X - 1) * ln2[0])
    hi = th[1] + c / (7 * X * ln2[0])
    return lo, hi


def main():
    out = sys.stdout
    print("# cyclebound_avg.py: certified maximum cycle means of 1/u (averaged refinement)", file=out)
    res = {}
    for kind, bound in [("3x+1", 2**71), ("7x+-1", 10**15 + 1), ("5+2i", 3 * 10**9)]:
        for eta, U in [(1 / 512, 32), (1 / 2048, 32), (1 / 4096, 64)]:
            r = maxmean(kind, bound, eta, U)
            print("%-6s bound %.3e  eta = 1/%d, U = %d: bins %d, edge ranges %d, iterations %d, %.1f s" % (
                kind, bound, round(1 / eta), U, r["K"] + 1, r["edges"], r["iters"], r["secs"]), file=out)
            print("        certified: mean of 1/u over any cycle <= c = %d/2^%d = %.6f   (graph value >= %.6f, float)" % (
                int(r["lam"] * (1 << SC)), SC, float(r["lam"]), r["lower"]), file=out)
            w = r["walk"]
            print("        best closed walk found: %d bins, mean weight %.6f, u from %.4f to %.4f" % (
                w["length"], w["mean"], w["umin"], w["umax"]), file=out)
            out.flush()
            res[kind] = r["lam"]
    c7, cG = res["7x+-1"], res["5+2i"]

    # 7x +- 1: averaged bound
    print("\n7x +- 1 with c = %.6f" % float(c7), file=out)
    for X in [10**15 + 1, 2 * 10**15, 3 * 10**15, 4 * 10**15, 4 * 10**15 + 5 * 10**14, 5 * 10**15, 10**16]:
        lo, hi = averaged_7(X, c7)
        f = C.simplest_in(lo, hi)
        lo1, hi1 = averaged_7(X, Fraction(1))
        f1 = C.simplest_in(lo1, hi1)
        print("  X = %.4e: averaged n >= %d (V = %d);  linearised basic (c = 1) n >= %d" % (
            X, f.denominator, f.numerator, f1.denominator), file=out)
    # thresholds: with c, the fraction p/q is admissible at X iff |p/q - log2 7| <= c/(7X ln2) (to first order)
    import mpmath as mp
    mp.mp.dps = 60
    t = mp.log(7) / mp.log(2)
    print("  first-order thresholds X_c(p/q) = c / (7 ln2 |p/q - log2 7|) for the records of the basic method:", file=out)
    for q in [27855251, 465645424, 604921679, 855618938, 883474189, 5328700385, 8862597141]:
        p = int(mp.nint(q * t))
        X0 = 1 / (7 * mp.log(2) * abs(mp.mpf(p) / q - t))
        print("    n = %-11d basic X* = %.4e   averaged X* = %.4e" % (q, X0, X0 * mp.mpf(float(c7))), file=out)

    # 5+2i averaged: box and disc with r replaced by c r in the numerators
    import cyclebound_gauss as G
    print("\n5+2i with c = %.6f" % float(cG), file=out)
    for B in [3 * 10**9, 4 * 10**9, 5 * 10**9, 5 * 10**9 + 5 * 10**8, 6 * 10**9, 7 * 10**9, 10**10, 10**11]:
        M = G.Map("5+2i", (5, 2), B, 5, "units")
        base = M.n_joint(10**6)
        rl, rh = M.r
        LN2, PI = G.LN2, G.PI
        M.s_lo_out = M.th1[0] - 2 * cG * rh / ((1 - rh) * LN2[0])
        M.s_hi_out = M.th1[1] + 2 * cG * rh / LN2[0]
        M.a_half_out = 4 * cG * rh / ((1 - rh * rh) * PI[0])
        M.a_lo_out = M.beta[0] - M.a_half_out
        M.a_hi_out = M.beta[1] + M.a_half_out
        M.rho_out = cG * rh / (1 - rh)
        ns, na = M.n_size(), M.n_angle()
        nj = M.n_joint(10**6)
        nd = M.n_joint(10**6, disc=True)
        print("  B = %.3e: basic box %d; averaged size %d, angle %d, box %d (V = %d, W = %d), disc %d" % (
            B, base[0], ns[0], na[0], nj[0], nj[1], nj[2], nd[0]), file=out)
        out.flush()


if __name__ == "__main__":
    main()
