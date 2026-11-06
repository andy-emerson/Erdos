"""Lattice reduction for the joint size and angle condition of the 5+2i map
(cycle-bounds-refined.md, Section 8).  Planning tool, not a proof.

With W = V + 2c and y = (beta - theta1)/2, the two conditions read, to first order,
    |n theta1 - V| ln2/2 <= n r,   |(n theta1 - V) + 2(n y - c)| pi/4 <= n r,
so good n are those for which (n theta1, n y) is close to Z^2: a simultaneous
approximation problem in dimension 2.  For a weight C, the integer lattice with basis
    b1 = (1, [C theta1], [C y]),  b2 = (0, C, 0),  b3 = (0, 0, C)
contains (n, ~C(n theta1 - V), ~C(n y - c)); LLL finds its short vectors, i.e. n with
both errors of order n/C.  Sweeping C over 10^3 .. 10^27 gives candidate n; for each
we compute r*(n) (cycle-bounds.md, Section 3.4) at 60 digits and keep the records.
The records up to 2*10^7 are compared with the exhaustive scan of cyclebound_thresholds.py.

Usage: python3 cyclebound_lll.py > logs/cyclebound_lll.txt
"""
import sys
from fractions import Fraction
import mpmath as mp

mp.mp.dps = 80
TH1 = mp.log(29) / mp.log(2)
BETA = 4 * mp.atan2(2, 5) / mp.pi
Y = (BETA - TH1) / 2
LN2, PI = mp.log(2), mp.pi


def lll(B, delta=Fraction(3, 4)):
    """Textbook LLL on integer row vectors (exact rational Gram-Schmidt)."""
    B = [list(b) for b in B]
    n = len(B)

    def dot(u, v):
        return sum(a * b for a, b in zip(u, v))

    def gs():
        Bs, mu = [], [[Fraction(0)] * n for _ in range(n)]
        for i in range(n):
            v = [Fraction(x) for x in B[i]]
            for j in range(i):
                mu[i][j] = Fraction(dot(B[i], Bs[j])) / dot(Bs[j], Bs[j])
                v = [a - mu[i][j] * b for a, b in zip(v, Bs[j])]
            Bs.append(v)
        return Bs, mu
    Bs, mu = gs()
    k = 1
    while k < n:
        for j in range(k - 1, -1, -1):
            q = round(mu[k][j])
            if q:
                B[k] = [a - q * b for a, b in zip(B[k], B[j])]
                Bs, mu = gs()
        if dot(Bs[k], Bs[k]) >= (delta - mu[k][k - 1] ** 2) * dot(Bs[k - 1], Bs[k - 1]):
            k += 1
        else:
            B[k], B[k - 1] = B[k - 1], B[k]
            Bs, mu = gs()
            k = max(k - 1, 1)
    return B


def rstar(n):
    """least r at which n passes both conditions (first order), eps any unit (W = V mod 2)."""
    best = None
    v0 = int(mp.nint(n * TH1))
    for V in (v0 - 1, v0, v0 + 1):
        es = abs(V - n * TH1) * LN2 / (2 * n)
        w0 = int(mp.nint(n * BETA))
        for W in range(w0 - 3, w0 + 4):
            if (W - V) % 2:
                continue
            ea = abs(W - n * BETA) * PI / (4 * n)
            m = max(es, ea)
            if best is None or m < best:
                best = m
    return best


def candidates():
    cands = set()
    k = 3.0
    while k <= 27.0:
        Cw = int(mp.nint(mp.mpf(10) ** k))
        basis = [[1, int(mp.nint(Cw * TH1)), int(mp.nint(Cw * Y))], [0, Cw, 0], [0, 0, Cw]]
        R = lll(basis)
        for a in range(-2, 3):
            for b in range(-2, 3):
                for c in range(-2, 3):
                    v = [a * R[0][i] + b * R[1][i] + c * R[2][i] for i in range(3)]
                    if v[0] != 0:
                        cands.add(abs(v[0]))
        k += 0.125
    return sorted(cands)


def main():
    out = sys.stdout
    print("# cyclebound_lll.py: LLL candidates for the joint condition of the 5+2i map (planning numbers, 80 digits)", file=out)
    cands = candidates()
    print("candidate n from LLL: %d values, largest %.3e" % (len(cands), cands[-1]), file=out)
    recs, best = [], None
    for n in cands:
        if n < 50:
            continue
        r = rstar(n)
        if best is None or r < best * (1 - mp.mpf(10) ** -9):
            recs.append((n, r))
            best = r
    scan = [58, 64, 134, 161, 225, 289, 514, 739, 1162, 1387, 1451, 1612, 1676, 1901, 2190, 4091, 6281, 8696,
            38875, 41065, 49761, 58457, 67153, 222942, 231638, 240334, 298791, 307487, 374640, 606278, 664735,
            673431, 2327780, 2702420, 3375851, 4049282, 8771995, 10426344, 12821277, 14475626, 15149057,
            18524908, 19198339]
    lllrec = [n for n, r in recs if n <= 2 * 10**7]
    print("records among LLL candidates with n <= 2e7: %d; scan records (cyclebound_thresholds.txt): %d" % (
        len(lllrec), len(scan)), file=out)
    print("  scan records missed by LLL:", [n for n in scan if n not in lllrec], file=out)
    print("  LLL 'records' that are not scan records (artefacts of missing candidates):",
          [n for n in lllrec if n not in scan], file=out)
    print("\nrecords among LLL candidates beyond 2e7, with B*(n) = 5/(29 r*^2) (the bound would reach the next", file=out)
    print("record once B exceeds this B*; valid only if no record was missed):", file=out)
    for n, r in recs:
        if n > 2 * 10**7:
            print("  n = %-22d r* = %.4e   B* = %.4e" % (n, r, 5 / (29 * r * r)), file=out)


if __name__ == "__main__":
    main()
