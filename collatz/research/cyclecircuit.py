"""Steiner-type 1-cycles for Barina's 7x +- 1 and for the 5+2i map
(cycle-bounds-refined.md, Section 5).

A 1-cycle is a cycle (up to units for 5+2i) whose odd elements, read from a
suitable element x_0, take n - 1 steps with the least valuation (v = 2, resp.
v = 4) and one and the same digit d, followed by one step with a larger
valuation and an arbitrary digit e.  The cycle equation then reads
    7x+-1:  h (2^V - 7^n)          = d (2^(l+2) - 7) + 3e,          l = V - 2n >= 1
    5+2i :  h (eps pi^V - alpha^n) = d eps pi^u - d alpha + (alpha - pi^4) e,  u = V - 4n + 4 >= 5
with h a nonzero integer (Gaussian integer), so the linear form in two logarithms
is exponentially small in n.  This script
  1. certifies the continued fractions of log2 7 and log2 29 (certarith);
  2. evaluates the Laurent and Matveev upper bounds N0 for n (constants as quoted
     in the write-up, Section 6);
  3. reduces N0 to a small N1 with the best-approximation property of the
     convergents (exact Fractions);
  4. finds every 1-cycle with n <= N2 (N2 >= N1) by exact integer arithmetic and
     checks each by running the map.
Usage: python3 cyclecircuit.py > logs/cyclecircuit.txt
"""
import sys
from fractions import Fraction
from math import log, ceil
import certarith as C

# ---------------------------------------------------------------- arithmetic in Z[i]
def gmul(a, b):
    return (a[0] * b[0] - a[1] * b[1], a[0] * b[1] + a[1] * b[0])


def gadd(a, b):
    return (a[0] + b[0], a[1] + b[1])


def gsub(a, b):
    return (a[0] - b[0], a[1] - b[1])


def gpow(a, k):
    r, b = (1, 0), a
    while k:
        if k & 1:
            r = gmul(r, b)
        b = gmul(b, b)
        k >>= 1
    return r


def gnorm(a):
    return a[0] * a[0] + a[1] * a[1]


def gdiv_exact(a, b):
    """a/b if b divides a in Z[i], else None."""
    n = gnorm(b)
    t = gmul(a, (b[0], -b[1]))
    if t[0] % n or t[1] % n:
        return None
    return (t[0] // n, t[1] // n)


ALPHA = (5, 2)
PI = (1, 1)
PI4 = gpow(PI, 4)          # -4
UNITS = [(1, 0), (0, 1), (-1, 0), (0, -1)]
DIGITS = [(1, 0), (0, 1), (-1, 0), (0, -1), (2, 1), (-1, 2), (-2, -1), (1, -2)]


def digit_H(z):
    """The digit of the 5+2i map (catalog H1): the unique d in D with 4 | alpha z + d."""
    az = gmul(ALPHA, z)
    ds = [d for d in DIGITS if (az[0] + d[0]) % 4 == 0 and (az[1] + d[1]) % 4 == 0]
    assert len(ds) == 1, (z, ds)
    return ds[0]


def step_H(z):
    d = digit_H(z)
    w = gadd(gmul(ALPHA, z), d)
    v = 0
    while (w[0] + w[1]) % 2 == 0:
        w = ((w[0] + w[1]) // 2, (w[1] - w[0]) // 2)
        v += 1
    return w, v, d


def step_7(x):
    d = 1 if x % 4 == 1 else -1
    w = 7 * x + d
    v = 0
    while w % 2 == 0:
        w //= 2
        v += 1
    return w, v, d


# ---------------------------------------------------------------- certified constants
LN2 = C.iv_frac(C.ln_rat(2, 1))


def log2_iv(a):
    la = C.iv_frac(C.ln_rat(a, 1))
    return la[0] / LN2[1], la[1] / LN2[0]


def best_approx_floor(th, cf, N):
    """Certified lower bound for min_{0<n<=N} min_V |n theta - V|, by the
    best-approximation property: for 0 < n < q_{k+1}, |n theta - V| >= |q_k theta - p_k|.
    Returns (bound, k, q_k, q_{k+1})."""
    conv = C.convergents(cf)
    for k in range(len(conv) - 1):
        p, q = conv[k]
        p1, q1 = conv[k + 1]
        if q1 > N:
            lo, hi = th
            d = min(abs(q * lo - p), abs(q * hi - p))
            assert (q * lo - p) * (q * hi - p) > 0
            return d, k, q, q1
    raise ValueError("continued fraction too short")


# ---------------------------------------------------------------- Baker bounds
def laurent_N0(h_big, coef_n_over_logs, c_rhs_log):
    """Largest n with n ln4 - c_rhs_log <= 17.9 (max(log b' + 0.38, 30, 1/2))^2 * 1 * h_big,
    b' = coef * n (Laurent 2008, Cor. 2, as restated in arXiv:1510.05579, Lemma 8; D = 1,
    h(2) replaced by 1/D = 1)."""
    def ok(n):
        bp = coef_n_over_logs * n
        m = max(log(bp) + 0.38, 30.0, 0.5)
        return n * log(4) - c_rhs_log <= 17.9 * m * m * h_big
    n = 1
    while ok(2 * n):
        n *= 2
    lo, hi = n, 2 * n
    while hi - lo > 1:
        mid = (lo + hi) // 2
        if ok(mid):
            lo = mid
        else:
            hi = mid
    # check monotone failure beyond (ok is eventually false and stays false)
    assert not any(ok(lo * k) for k in [2, 4, 10, 100, 10**4, 10**8])
    return lo


def matveev_N0(A1A2, Bcoef, c_rhs_log):
    """Largest n with n ln4 - c_rhs_log <= 1.4 * 30^5 * 2^4.5 * (1 + log B) * A1 A2, B = Bcoef * n
    (Matveev 2000, Cor. 2.3, as restated in arXiv:2208.03068, Prop. 14; t = 2, D = 1)."""
    K = 1.4 * 30**5 * 2**4.5 * A1A2

    def ok(n):
        return n * log(4) - c_rhs_log <= K * (1 + log(Bcoef * n))
    n = 1
    while ok(2 * n):
        n *= 2
    lo, hi = n, 2 * n
    while hi - lo > 1:
        mid = (lo + hi) // 2
        if ok(mid):
            lo = mid
        else:
            hi = mid
    return lo, K


# ---------------------------------------------------------------- exhaustive small-n search
def search_7(N2):
    found = []
    tests = 0
    for n in range(1, N2 + 1):
        p7 = 7**n
        Vmax = p7.bit_length() + 2
        for V in range(2 * n + 1, Vmax + 1):
            D = (1 << V) - p7
            l = V - 2 * n
            if abs(D) > (1 << (l + 2)):
                continue
            for d in (1, -1):
                for e in (1, -1):
                    tests += 1
                    Y = d * ((1 << (l + 2)) - 7) + 3 * e
                    if Y % D:
                        continue
                    h = Y // D
                    num = 4**(n - 1) * h - d
                    if num % 3:
                        continue
                    x0 = num // 3
                    # run the map and check the shape
                    x, okshape = x0, True
                    for j in range(n):
                        x, v, dd = step_7(x)
                        want_v = 2 if j < n - 1 else l + 2
                        want_d = d if j < n - 1 else e
                        if v != want_v or dd != want_d:
                            okshape = False
                            break
                    found.append((n, V, d, e, x0, okshape and x == x0))
    return found, tests


def search_H(N2):
    found = []
    tests = 0
    am = ALPHA[0] - PI4[0], ALPHA[1] - PI4[1]          # alpha - pi^4 = 9 + 2i
    for n in range(1, N2 + 1):
        an = gpow(ALPHA, n)
        Vmax = (29**n).bit_length() + 4
        for V in range(4 * n + 1, Vmax + 1):
            pv = gpow(PI, V)
            u = V - 4 * n + 4
            pu = gpow(PI, u)
            for eps in UNITS:
                D = gsub(gmul(eps, pv), an)
                for d in DIGITS:
                    base = gsub(gmul(gmul(d, eps), pu), gmul(d, ALPHA))
                    for e in DIGITS:
                        tests += 1
                        Y = gadd(base, gmul(am, e))
                        if gnorm(Y) < gnorm(D):
                            continue
                        h = gdiv_exact(Y, D)
                        if h is None or h == (0, 0):
                            continue
                        z0 = gdiv_exact(gsub(gmul(gpow(PI, 4 * (n - 1)), h), d), am)
                        if z0 is None:
                            continue
                        z, okshape = z0, True
                        for j in range(n):
                            if (z[0] + z[1]) % 2 == 0:
                                okshape = False
                                break
                            z, v, dd = step_H(z)
                            want_v = 4 if j < n - 1 else u
                            want_d = d if j < n - 1 else e
                            if v != want_v or dd != want_d:
                                okshape = False
                                break
                        closes = okshape and z == gmul(eps, z0)
                        found.append((n, V, eps, d, e, z0, closes))
    return found, tests


def main():
    out = sys.stdout
    print("# cyclecircuit.py: Steiner-type 1-cycles, certarith P = %d bits" % C.P, file=out)
    # digit table check
    for a in range(4):
        for b in range(4):
            if (a + b) % 2:
                digit_H((a, b))
    print("digit table of the 5+2i map: unique digit for each odd class mod 4 (checked)", file=out)

    for label, base, h_big, cf_terms, rhs_const in [("7x+-1", 7, log(7), 45, Fraction(16, 3)), ("5+2i (size)", 29, log(29), 45, Fraction(74))]:
        th = log2_iv(base)
        cf = C.certified_cf(*th, maxterms=cf_terms)
        conv = C.convergents(cf)
        print("\n%s: theta = log2 %d, %d certified partial quotients, last convergent denominator %.3e" % (
            label, base, len(cf), conv[-1][1]), file=out)
        print("  |V - n theta| <= %s * 4^(-n) / ln 2 for a 1-cycle with n >= %d (Section 5)" % (
            rhs_const, 2 if base == 7 else 4), file=out)
        N0L = laurent_N0(h_big, (1 + 1 / log(2)) * 1.001, log(float(rhs_const)))
        N0M, KM = matveev_N0(log(2) * h_big, log(base) / log(2) + 0.01, log(float(rhs_const)))
        print("  Laurent (17.9, 30/D): n <= N0 = %d" % N0L, file=out)
        print("  Matveev (Cor. 2.3, t = 2, D = 1): constant %.4e, n <= N0 = %d" % (KM, N0M), file=out)
        for N0 in (N0L, N0M):
            dmin, k, q, q1 = best_approx_floor(th, cf, N0)
            # smallest N1 such that for n > N1: rhs_const 4^-n / ln2 < dmin
            # (exact: compare Fractions with a rational upper bound of 1/ln2)
            inv_ln2 = 1 / LN2[0]
            N1 = 1
            while rhs_const * inv_ln2 / Fraction(4)**N1 >= dmin:
                N1 += 1
            print("  N0 = %d < q_%d = %d: |n theta - V| >= |q_%d theta - p_%d| >= %.4e for 0 < n <= N0, so n <= N1 = %d" % (
                N0, k + 1, q1, k, k, float(dmin), N1), file=out)
        out.flush()

    N2 = 60
    f7, t7 = search_7(N2)
    print("\n7x+-1: exhaustive search of 1-cycles with n <= %d (%d (n, V, d, e) tests)" % (N2, t7), file=out)
    for r in f7:
        print("  n = %d, V = %d, d = %+d, e = %+d, x0 = %d, genuine 1-cycle: %s" % r, file=out)
    N2H = 40
    fH, tH = search_H(N2H)
    print("\n5+2i: exhaustive search of 1-cycles up to units with n <= %d (%d (n, V, eps, d, e) tests)" % (N2H, tH), file=out)
    seen = set()
    for r in fH:
        key = (r[0], r[5], r[6])
        if key in seen:
            continue
        seen.add(key)
        print("  n = %d, V = %d, eps = %s, d = %s, e = %s, z0 = %s, genuine 1-cycle: %s" % r, file=out)


if __name__ == "__main__":
    main()
