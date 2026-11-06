"""realbal.py: multiplier classes with an associate in the per-embedding window (real-quadratic.md, Section 3.4).
For each ring and k: classes of associates with |N| in the norm window, the best associate (minimizing max_j D_j),
and whether max_j D_j < 0. log: logs/realq_balanced.txt"""
import realq as Q
L = []
for nm, kmax in (('Zr2', 8), ('Zr3', 8), ('Zg5', 4)):
    R = Q.RQ(nm)
    L.append(f"## {nm}")
    for k in range(1, kmax + 1):
        ms = Q.odd_multipliers(R, k); good = []
        for a0, n in ms:
            best = None
            for m in range(-12, 13):
                a = R.mul(a0, R.pw(R.eps, m)); D = R.window_drifts(a, k)
                if best is None or max(D) < best[0]: best = (max(D), a, D)
            if best[0] < 0: good.append(f"{R.fmt(best[1])} (|N|={n}, D=({best[2][0]:+.3f},{best[2][1]:+.3f}))")
        L.append(f"k={k}: {len(ms)} classes in the norm window, {len(good)} with an associate in the per-embedding window: " + "; ".join(good))
open('logs/realq_balanced.txt', 'w').write("\n".join(L) + "\n"); print("\n".join(L))
