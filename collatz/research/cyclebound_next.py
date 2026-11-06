"""Next jumps of the exact cycle-length bound for the Gaussian maps of cycle-bounds.md,
Section 4 (and the 5+2i map), with the verification needed for each (Section 7 of the note).

For each map: the exact box bound n0 at its present verification bound B (cyclebound_gauss), the
double-precision threshold B*(n0) = dmax2/(N(alpha) r*(n0)^2) above which n0 is no longer
admissible (cyclebound_thresholds.records), and the exact bound at B = 1.02 B*(n0) (rounded up),
then the same for the following jump.  The joint bound n(B) is constant on [B*(previous), B*(n0)).

Usage: python3 cyclebound_next.py > logs/cyclebound_next.txt
"""
import math
import cyclebound_gauss as G
import cyclebound_thresholds as T


def round_up(x):
    e = 10 ** max(0, int(math.floor(math.log10(x))) - 1)
    return int(math.ceil(x / e) * e)


def main():
    print("# cyclebound_next.py: next jumps of the box bound (thresholds double precision, bounds exact)")
    print("# map | B now | bound now | B* (bound now stops) | B used | exact bound | B* (that one stops) | B used | exact bound")
    for M in G.MAPS:
        rec = T.records(M.alpha, M.dmax2, M.m, 2 * 10**6)
        recd = {n: Bs for n, rs, Bs in rec}
        n0 = M.n_joint(3 * 10**7)[0]
        row = [M.name, "%.3g" % M.B, str(n0)]
        n = n0
        for _ in range(2):
            Bs = recd.get(n)
            if Bs is None:
                row.append("n=%d not a record?" % n)
                break
            Bu = round_up(Bs * 1.02)
            M2 = G.Map(M.name, M.alpha, Bu, M.dmax2, M.mode)
            n = M2.n_joint(3 * 10**7)[0]
            row += ["%.4g" % Bs, "%.3g" % Bu, str(n)]
        print(" | ".join(row), flush=True)


if __name__ == "__main__":
    main()
