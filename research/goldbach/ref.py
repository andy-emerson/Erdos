"""Independent brute-force reference for gg.c (numpy, small norms).

For each even z = a+bi in the fundamental domain (0 <= b <= a) with N(z) <= NMAX:
  E  : min N(p1) over z = p1+p2, Gaussian primes, angle(pj, z) < 45 deg  (via 2 dot^2 > |p|^2 |z|^2)
  F  : same with < 30 deg                                               (via 4 dot^2 > 3 |p|^2 |z|^2)
  Q  : min (x+y) over p1 = x+yi, p2 = z-p1 both in the open first quadrant (b >= 2)
Writes lines "Z a b E F Q" (-9 = not applicable, -1 = no representation), plus representation
counts for small z. Uses a full box search (no early exit), so it is independent of gg.c's search order.
usage: python3 ref.py NMAX [counts_max_norm]
"""
import sys, numpy as np

NMAX = int(sys.argv[1]); CMAX = int(sys.argv[2]) if len(sys.argv) > 2 else 0
R = int((2 * NMAX) ** 0.5) + 3          # |p| < sqrt(2)|z| for E; F, Q are inside that
L = 2 * R * R + 10
sv = np.ones(L + 1, bool); sv[:2] = False
for p in range(2, int(L ** 0.5) + 1):
    if sv[p]: sv[p * p::p] = False

g = np.arange(-2 * R, 2 * R + 1)
X, Y = np.meshgrid(g, g, indexing='ij')
NN = X * X + Y * Y
def gprime(x, y, n):
    ax, ay = np.abs(x), np.abs(y)
    axis = ((x == 0) & (ay % 4 == 3) & sv[np.minimum(ay, L)]) | ((y == 0) & (ax % 4 == 3) & sv[np.minimum(ax, L)])
    return np.where((x != 0) & (y != 0), sv[np.minimum(n, L)], axis)
GPR = gprime(X, Y, NN)
off = 2 * R

out = []; counts = []
for a in range(0, int(NMAX ** 0.5) + 1):
    for b in range(0, a + 1):
        if (a + b) % 2 or a * a + b * b > NMAX: continue
        nz = a * a + b * b
        rz = int((2 * nz) ** 0.5) + 2
        sl = slice(off - rz, off + rz + 1)
        x, y, n, pr = X[sl, sl], Y[sl, sl], NN[sl, sl], GPR[sl, sl]
        qx, qy = a - x, b - y; qn = qx * qx + qy * qy
        qpr = gprime(qx, qy, qn)
        dot = x * a + y * b; dot2 = nz - dot
        both = pr & qpr
        mE = both & (dot > 0) & (dot2 > 0) & (2 * dot * dot > n * nz) & (2 * dot2 * dot2 > qn * nz)
        mEn = both & (dot > 0) & (dot2 > 0) & (2 * dot * dot >= n * nz) & (2 * dot2 * dot2 >= qn * nz)
        mF = both & (dot > 0) & (dot2 > 0) & (4 * dot * dot > 3 * n * nz) & (4 * dot2 * dot2 > 3 * qn * nz)
        mQ = both & (x > 0) & (y > 0) & (qx > 0) & (qy > 0)
        E = int(n[mE].min()) if mE.any() else -1
        F = int(n[mF].min()) if mF.any() else -1
        Q = int((x + y)[mQ].min()) if mQ.any() else -1
        if nz <= 2: E = -9
        if nz <= 10: F = -9
        if b < 2: Q = -9
        if E == -1 and mEn.any(): E = -2   # only the non-strict 45 deg form works
        out.append(f"Z {a} {b} {E} {F} {Q}")
        if nz <= CMAX:
            counts.append((nz, a, b, int(mE.sum()), int(mEn.sum()), int(mF.sum()), int(mQ.sum()) if b >= 2 else None))

with open(f"dump_py_{NMAX}.txt", "w") as f:
    f.write("\n".join(sorted(out)) + "\n")
if counts:
    print("N a b  E(<45) E(<=45) F(<30) Q   (ordered pairs (p1,p2))")
    for c in sorted(counts): print(*c)
