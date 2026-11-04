# Contraction per unit of valuation for step S (recursive.md, section 4).
# Potential P_m = max(x_m, x_{m+1} theta^(-g_m)), x = absolute value of the normalised remainders.
# P_{m+1} <= theta^(g_m) P_m holds whenever, for all gaps g, g' >= delta,
#     F(g, g') = u^-(g+g') + c(g) u^-g' <= 1,   u = sqrt(2) theta,  c(g) = rho(g+delta) 2^(-g/2).
# (a) with the covering radius, c(g) <= 2^((delta-1)/2), worst case g = g' = delta, closed form;
# (b) with the exact rho(m), m <= 11, from rho.py (floating point);
# (c) the pair dynamic programme of the draft, rescaled by theta^(-v) (floating point).
from math import sqrt, log2
from rho import rho
EX = {m: rho(m)[0] for m in range(1, 12)}
def R(m): return EX[m] if m in EX else 2 ** ((m - 1) / 2)

def theta_cover(d):
    c = 2 ** ((d - 1) / 2)
    w = (-c + sqrt(c * c + 4)) / 2          # w = u^-d
    u = w ** (-1 / d)
    return u / sqrt(2)

def F(th, d, g, g2):
    u = sqrt(2) * th
    return u ** (-(g + g2)) + R(g + d) * 2 ** (-g / 2) * u ** (-g2)
def theta_exact(d, gmax=80):
    lo, hi = 1 / sqrt(2) + 1e-9, 1.0
    for _ in range(60):
        th = (lo + hi) / 2
        ok = all(F(th, d, g, g2) <= 1 for g in range(d, gmax) for g2 in range(d, gmax))
        lo, hi = (lo, th) if ok else (th, hi)
    return hi

def growth_dp(th, d, steps=400, gmax=40):
    u = sqrt(2) * th
    S = {d: (1.0, 1.0)}
    m = 1
    for _ in range(steps):
        T = {}
        for g, (yp, yc) in S.items():
            for g2 in range(d, gmax + 1):
                yn = u ** (-(g + g2)) * (yp + R(g + d) * th ** g * yc)
                o = T.get(g2, (0, 0)); T[g2] = (max(o[0], yc), max(o[1], yn))
        m = max(max(v) for v in T.values())
        S = {g: (v[0] / m, v[1] / m) for g, v in T.items()}
    return m
def theta_dp(d):
    lo, hi = 1 / sqrt(2) + 1e-6, 1.0
    for _ in range(40):
        th = (lo + hi) / 2
        lo, hi = (lo, th) if growth_dp(th, d) <= 1 else (th, hi)
    return hi

if __name__ == "__main__":
    for d in (2, 3, 4, 6):
        tc, te, td = theta_cover(d), theta_exact(d), theta_dp(d)
        print("delta %d: theta cover %.4f (bits of norm lost per unit valuation %.4f), "
              "exact-rho potential %.4f (%.4f), pair DP %.4f (%.4f)"
              % (d, tc, -2 * log2(tc), te, -2 * log2(te), td, -2 * log2(td)))
    # first step (gap 1) factor for the covering bound at delta = 2
    th = theta_cover(2); u = sqrt(2) * th
    print("delta 2, first step factor F(1, 2) at theta_cover: %.4f" % (u ** -3 + sqrt(2) * u ** -2))
