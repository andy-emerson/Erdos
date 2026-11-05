"""Small denominators of the cycle equation, heuristic counts and a priori norm bounds (families.md).

A cycle up to units with n odd steps, valuations v_0..v_{n-1} (total V) and S^n(z) = eps z satisfies
    z (eps (1+i)^V - alpha^n) = sum_j d_j alpha^(n-1-j) (1+i)^(V - s_j),   s_j = v_j + ... + v_{n-1},
so z = c / den with den = eps (1+i)^V - alpha^n.  Every element of such a cycle obeys
    |z| <= dmax * sum_{j} |alpha|^(n-1-j) 2^(-s_j/2) / |eps - alpha^n/(1+i)^V|,
and since s_j >= k (n-j) and s_j <= V - k j, the worst case over valuation vectors is computed exactly
by a small dynamic programme (max of sum_j |alpha|^(n-1-j) 2^(-s_j/2) over admissible s).
Heuristic count of cycles up to units with data (n, V, eps):
    E = 2^((k-1) n) * C(V - k n + n - 1, n - 1) / (4 n N(den)).
usage: famtheory.py k ax,ay dmax2 [nmax] [Emin]
  lists every (n, V, eps) with n <= nmax and E >= Emin (default nmax 300, Emin 0.01), and the bound."""
import sys, math, cmath
from fractions import Fraction

def gmul(a, b): return (a[0]*b[0]-a[1]*b[1], a[0]*b[1]+a[1]*b[0])
def gpow(a, n):
    r = (1, 0)
    while n:
        if n & 1: r = gmul(r, a)
        a = gmul(a, a); n >>= 1
    return r
UNITS = [(1,0),(0,1),(-1,0),(0,-1)]

def den(alpha, n, V, eps):
    p = gmul(eps, gpow((1, 1), V)); q = gpow(alpha, n)
    return (p[0]-q[0], p[1]-q[1])

def logC(a, b):
    return math.lgamma(a+1) - math.lgamma(b+1) - math.lgamma(a-b+1)

def heuristic(k, alpha, n, V, eps):
    d = den(alpha, n, V, eps); Nd = d[0]**2 + d[1]**2
    e = V - k*n
    if e < 0 or Nd == 0: return None, Nd
    lg = (k-1)*n*math.log(2) + logC(e+n-1, n-1) - math.log(4*n) - math.log(Nd)
    return math.exp(lg), Nd

def bound(k, alpha, n, V, eps, dmax):
    """max |z| over all cycles with data (n, V, eps): worst case of sum_j |alpha|^(n-1-j) 2^(-s_j/2),
    s_j = v_j+...+v_{n-1}, v >= k, sum = V.  Terms with larger j have smaller exponent of |alpha| and
    s_j smaller; s_j >= k(n-j) and s_0 = V.  Maximising: put all excess into v_0 (s_j = k(n-j) for j >= 1)."""
    a = math.sqrt(alpha[0]**2 + alpha[1]**2)
    # |eps - alpha^n/(1+i)^V| computed in logs to avoid overflow
    ang = n*math.atan2(alpha[1], alpha[0]) - V*math.pi/4 - math.atan2(eps[1], eps[0])
    r = math.exp(n*math.log(a) - V*math.log(2)/2)
    rel = abs(1 - r*cmath.exp(1j*ang))
    S = math.exp((n-1)*math.log(a) - V*math.log(2)/2)
    for j in range(1, n):
        S += math.exp((n-1-j)*math.log(a) - k*(n-j)*math.log(2)/2)
    return dmax * S / rel, rel

if __name__ == '__main__':
    k = int(sys.argv[1]); alpha = tuple(map(int, sys.argv[2].split(','))); dmax = math.sqrt(int(sys.argv[3]))
    nmax = int(sys.argv[4]) if len(sys.argv) > 4 else 300
    Emin = float(sys.argv[5]) if len(sys.argv) > 5 else 0.01
    a2 = alpha[0]**2 + alpha[1]**2
    rows = []
    for n in range(1, nmax+1):
        Vc = n*math.log2(a2)
        for V in range(max(k*n, int(Vc) - 3), int(Vc) + 4):
            for eps in UNITS:
                E, Nd = heuristic(k, alpha, n, V, eps)
                if E is None or E < Emin: continue
                b, rel = bound(k, alpha, n, V, eps, dmax)
                rows.append((n, V, eps, E, rel, b))
    print(f"# k={k} alpha={alpha}: (n, V, eps) with heuristic count >= {Emin}, n <= {nmax}")
    print("# n V eps E rel=|1-alpha^n/(eps(1+i)^V)| bound_on_|z| bound_on_N")
    for n, V, eps, E, rel, b in rows:
        print(f"{n} {V} {eps[0]},{eps[1]} {E:.3g} {rel:.4g} {b:.4g} {b*b:.4g}")
    print(f"# total heuristic {sum(r[3] for r in rows):.4g}")

def maxbound(k, alpha, n, dmax):
    """max over V >= k n and units eps of the a-priori bound on N(z) for cycles with n odd steps."""
    a2 = alpha[0]**2 + alpha[1]**2
    best = (0, None, None)
    for V in range(k*n, int(2*n*math.log2(a2)) + 60):
        for eps in UNITS:
            b, rel = bound(k, alpha, n, V, eps, dmax)
            if b*b > best[0]: best = (b*b, V, eps)
    return best
