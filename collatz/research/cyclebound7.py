"""Exact lower bound on the number of odd steps in a non-trivial cycle of
Barina's 7x +- 1 map (cycle-bounds.md, Section 2).

Map: f(x) = 7x+1 (x = 1 mod 4), 7x-1 (x = 3 mod 4), x/2 (x even).
A cycle with n odd elements x_j and V halvings satisfies prod (7 + d_j/x_j) = 2^V,
d_j = +-1.  If every odd element has |x_j| >= X then
    n log2(7 - 1/X) <= V <= n log2(7 + 1/X),
so V/n lies in the fixed interval I(X) = [log2(7-1/X), log2(7+1/X)] and n is at
least the smallest denominator of a fraction in I(X).  All decisions below use
certarith (integer intervals, exact Fractions).

Usage: python3 cyclebound7.py > logs/cyclebound7.txt
"""
import sys
from fractions import Fraction
import certarith as C
import mpmath as mp

ln2 = C.iv_frac(C.ln_rat(2, 1))
ln7 = C.iv_frac(C.ln_rat(7, 1))


def interval(X):
    """Outer and inner rational enclosures of I(X)."""
    lm = C.iv_frac(C.ln_rat(7 * X - 1, X))
    lp = C.iv_frac(C.ln_rat(7 * X + 1, X))
    outer = (lm[0] / ln2[1], lp[1] / ln2[0])
    inner = (lm[1] / ln2[0], lp[0] / ln2[1])
    return outer, inner


def bound(X):
    outer, inner = interval(X)
    f = C.simplest_in(*outer)
    attained = inner[0] <= f <= inner[1]
    return f, attained, outer, inner


def enumerate_in(lo, hi, M):
    """All reduced fractions p/q in [lo, hi] with q <= M (Stern-Brocot descent)."""
    out = []
    a, b = C.floor_frac(lo), 1
    stack = [(a, 1, a + 1, 1)]  # Farey neighbours a/b < c/d
    if Fraction(a) >= lo:
        out.append(Fraction(a))
    while stack:
        a, b, c, d = stack.pop()
        if b + d > M:
            continue
        m = Fraction(a + c, b + d)
        if lo <= m <= hi:
            out.append(m)
        if m > lo:
            stack.append((a, b, a + c, b + d))
        if m < hi:
            stack.append((a + c, b + d, c, d))
    return sorted(set(out), key=lambda f: f.denominator)


def main():
    out = sys.stdout
    print("# cyclebound7.py: exact bound for 7x +- 1, certarith precision P =", C.P, "bits", file=out)
    th = (ln7[0] / ln2[1], ln7[1] / ln2[0])
    print("log2 7 enclosure width:", float(th[1] - th[0]), file=out)
    cf = C.certified_cf(*th, maxterms=45)
    print("certified partial quotients of log2 7 (%d):" % len(cf), cf, file=out)
    conv = C.convergents(cf)
    print("convergents p/q:", [(p, q) for p, q in conv[:25]], file=out)

    X = 10**15 + 1
    f, att, outer, inner = bound(X)
    n0, V0 = f.denominator, f.numerator
    print("\nX = 10^15 + 1 (least odd |x| above Barina's bound 10^15)", file=out)
    print("outer interval width", float(outer[1] - outer[0]), " inner width", float(inner[1] - inner[0]), file=out)
    print("smallest denominator in outer interval: n0 =", n0, " V0 =", V0, file=out)
    print("V0/n0 in inner interval (bound attained by the method):", att, file=out)
    print("n0 is a convergent denominator:", any(q == n0 for p, q in conv), file=out)
    # admissible V for n = n0
    Vlo, Vhi = C.ceil_frac(n0 * outer[0]), C.floor_frac(n0 * outer[1])
    print("admissible V for n0 (outer):", list(range(Vlo, Vhi + 1)), file=out)
    print("steps of f in such a cycle >= n0 + V0 =", n0 + V0, file=out)
    # which side of log2 7
    print("V0/n0 - log2 7 sign:", "+" if f > th[1] else ("-" if f < th[0] else "?"), file=out)
    print("position of V0/n0 in I: (f - lo)/(hi - lo) =", float((f - outer[0]) / (outer[1] - outer[0])), file=out)

    M = 10**9
    adm = enumerate_in(outer[0], outer[1], M)
    print("\nall fractions V/n in the outer I(X) with n <= 10^9 (reduced):", file=out)
    print("  count:", len(adm), "; those with n <= 3*10^8:", file=out)
    for g in adm:
        if g.denominator <= 3 * 10**8:
            print("  n = %d  V = %d" % (g.denominator, g.numerator), file=out)
    print("(every multiple and every sum of admissible (n, V) is admissible)", file=out)

    # thresholds: for a fraction p/q, the largest X with p/q in I(X) is 1/|2^(p/q) - 7|
    mp.mp.dps = 80
    t = mp.log(7) / mp.log(2)
    cands = set()
    for k in range(1, len(conv)):
        p0, q0 = conv[k - 1]
        p1, q1 = conv[k]
        a_next = cf[k + 1] if k + 1 < len(cf) else 1
        for j in range(0, a_next + 1):
            cands.add((p0 + j * p1, q0 + j * q1))
    cands = sorted((q, p) for p, q in cands if q > 0)
    rows = []
    best = mp.mpf(0)
    for q, p in cands:
        Xs = 1 / abs(mp.power(2, mp.mpf(p) / q) - 7)
        if Xs > best:
            rows.append((q, p, Xs, best))
            best = Xs
    print("\nrecord thresholds X*(V/n) = 1/|2^(V/n) - 7| among convergents and intermediate fractions", file=out)
    print("(the bound is >= n exactly when X exceeds every X* of smaller n; mpmath 80 digits, planning numbers)", file=out)
    prev_q = None
    table = []
    for i, (q, p, Xs, _) in enumerate(rows):
        table.append((q, p, Xs))
    # bound as a function of X: n(X) = least q with X*(q) >= X
    for i, (q, p, Xs) in enumerate(table):
        need = max([x for (qq, pp, x) in table[:i]], default=mp.mpf(0))
        if Xs > 10**26:
            break
        print("  n = %-16d V = %-17d X* = %.4e   bound becomes n once X > %.4e" % (q, p, Xs, need), file=out)
    # certify a few jumps exactly
    print("\nexact checks of the bound at chosen X:", file=out)
    for Xc in [10**15 + 1, 5086 * 10**12, 5087 * 10**12, 5 * 10**15, 6 * 10**15, 1728 * 10**15, 1729 * 10**15, 2**60, 3 * 10**15, 10**16, 10**17, 10**18, 10**19, 2**68, 10**21, 2**71]:
        g, a2, _, _ = bound(Xc)
        print("  X = %.4e : n >= %d (V = %d), attained %s" % (Xc, g.denominator, g.numerator, a2), file=out)


if __name__ == "__main__":
    main()
