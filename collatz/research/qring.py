"""qring.py: generic arithmetic for the rings of rings-test.md.

A ring is Z[tau] with tau^2 = t*tau - n, elements (a, b) = a + b*tau, or Z (dim 1, elements (a, 0)).
  Z        dim 1, pi = 2
  Zi       t = 0,  n = 1   tau = i                  pi = 1+i
  Zw       t = -1, n = 1   tau = omega              pi = 2 (inert)
  Zs2      t = 0,  n = 2   tau = sqrt(-2)           pi = sqrt(-2) (ramified)
  Z7       t = 1,  n = 2   tau = (1+sqrt(-7))/2     pi = tau (split; Z7b: pi = 1 - tau, the other prime)
  Z11      t = 1,  n = 3   tau = (1+sqrt(-11))/2    pi = 2 (inert, q = 4)
  Zr2      t = 0,  n = -2  tau = sqrt(2)            pi = sqrt(2) (ramified, infinite units)
Norm N(a + b tau) = a^2 + t a b + n b^2 (signed for Zr2; |a| on Z for the window).
"""
import math, itertools, cmath
from functools import lru_cache

RINGS = {
    'Z':    dict(t=0, n=1, dim=1, pi=(2, 0)),
    'Zi':   dict(t=0, n=1, dim=2, pi=(1, 1)),
    'Zw':   dict(t=-1, n=1, dim=2, pi=(2, 0)),
    'Zs2':  dict(t=0, n=2, dim=2, pi=(0, 1)),
    'Z7':   dict(t=1, n=2, dim=2, pi=(0, 1)),
    'Z7b':  dict(t=1, n=2, dim=2, pi=(1, -1)),
    'Z11':  dict(t=1, n=3, dim=2, pi=(2, 0)),
    'Zr2':  dict(t=0, n=-2, dim=2, pi=(0, 1)),
}
EPS = (1, 1)        # fundamental unit 1 + sqrt 2 of Zr2
EPSINV = (-1, 1)    # sqrt 2 - 1


class Ring:
    def __init__(self, name):
        self.name = name
        s = RINGS[name]
        self.t, self.n, self.dim, self.pi = s['t'], s['n'], s['dim'], s['pi']
        self.real = self.n < 0
        self.pibar = self.conj(self.pi)
        self.Npi = self.N(self.pi)              # signed; 4 for Z (as element of Z[i] with b = 0)
        if self.dim == 1: self.q = 2
        else: self.q = abs(self.Npi)
        if self.real:
            self.U = None
        else:
            self.U = self._units()

    # ---- arithmetic
    def mul(self, x, y):
        a, b = x; c, d = y
        return (a * c - self.n * b * d, a * d + b * c + self.t * b * d)
    def add(self, x, y): return (x[0] + y[0], x[1] + y[1])
    def sub(self, x, y): return (x[0] - y[0], x[1] - y[1])
    def neg(self, x): return (-x[0], -x[1])
    def conj(self, x): return (x[0] + self.t * x[1], -x[1])
    def N(self, x):
        a, b = x
        return a * a + self.t * a * b + self.n * b * b
    def absN(self, x):
        """the norm used for windows: |a| on Z, |N| otherwise"""
        return abs(x[0]) if self.dim == 1 else abs(self.N(x))
    def size(self, x):
        """size used to order digits: |a| on Z, N for imaginary rings, the Minkowski T2/2 = a^2 + 2b^2 on Zr2"""
        if self.dim == 1: return abs(x[0])
        if self.real: return x[0] ** 2 + 2 * x[1] ** 2
        return self.N(x)
    def pw(self, x, k):
        r = (1, 0)
        for _ in range(k): r = self.mul(r, x)
        return r
    def div_exact(self, x, y):
        """x / y if y divides x, else None"""
        Ny = self.N(y)
        w = self.mul(x, self.conj(y))
        if w[0] % Ny or w[1] % Ny: return None
        return (w[0] // Ny, w[1] // Ny)
    def is_unit(self, x): return abs(self.N(x)) == 1
    # ---- pi
    def div_pi(self, x):
        if self.dim == 1:
            return (x[0] // 2, 0) if x[0] % 2 == 0 else None
        return self.div_exact(x, self.pi)
    def odd(self, x): return x != (0, 0) and self.div_pi(x) is None
    def vpi(self, x):
        if x == (0, 0): return math.inf
        v = 0
        while True:
            y = self.div_pi(x)
            if y is None: return v
            x = y; v += 1
    def oddpart(self, x):
        v = 0
        while x != (0, 0):
            y = self.div_pi(x)
            if y is None: break
            x = y; v += 1
        return x, v
    # ---- units
    def _units(self):
        out = []
        for a in range(-2, 3):
            for b in range(-2, 3):
                if self.dim == 1 and b: continue
                if self.N((a, b)) == 1: out.append((a, b))
        # order: 1 first, then by generating
        out.sort(key=lambda u: (u != (1, 0), u))
        return out
    def unit_order(self, u):
        x = u; k = 1
        while x != (1, 0): x = self.mul(x, u); k += 1
        return k
    def kappa(self):
        if self.real:
            return None
        return max(self.vpi(self.sub(u, (1, 0))) for u in self.U if u != (1, 0))
    # ---- residue classes mod pi^K: HNF basis (A,0), (c,g)
    @lru_cache(maxsize=None)
    def hnf(self, K):
        if self.dim == 1: return (2 ** K, 0, 1)
        P = self.pw(self.pi, K)
        v1 = P; v2 = self.mul(P, (0, 1))
        det = abs(v1[0] * v2[1] - v1[1] * v2[0])
        g, x, y = egcd(v1[1], v2[1])
        w = (x * v1[0] + y * v2[0], x * v1[1] + y * v2[1])
        if g < 0: g = -g; w = (-w[0], -w[1])
        A = det // g
        return (A, w[0] % A, g)
    def idx(self, z, K):
        A, c, g = self.hnf(K)
        a, b = z
        y = b % g
        x = (a - ((b - y) // g) * c) % A
        return y * A + x
    def nclasses(self, K):
        A, c, g = self.hnf(K); return A * g
    def classrep(self, i, K):
        A, c, g = self.hnf(K)
        y, x = divmod(i, A)
        return (x + y * c, y) if self.dim == 2 else (x, 0)
    def odd_classes(self, K):
        return [i for i in range(self.nclasses(K)) if self.odd(self.classrep(i, K))]
    # ---- elements by size
    def elements(self, bound):
        """all nonzero elements of size <= bound (size as in self.size)"""
        out = []
        if self.dim == 1:
            for a in range(-bound, bound + 1):
                if a: out.append((a, 0))
            return out
        if self.real:
            R = int(math.isqrt(bound)) + 1
            for a in range(-R, R + 1):
                for b in range(-R, R + 1):
                    if (a, b) != (0, 0) and a * a + 2 * b * b <= bound: out.append((a, b))
            return out
        D = 4 * self.n - self.t ** 2
        Bb = int(math.isqrt(4 * bound // D)) + 1
        for b in range(-Bb, Bb + 1):
            Ra = int(math.isqrt(bound)) + abs(self.t * b) + 2
            for a in range(-Ra, Ra + 1):
                if (a, b) != (0, 0) and self.N((a, b)) <= bound: out.append((a, b))
        return out
    def by_class(self, K, bound):
        d = {}
        for z in self.elements(bound):
            if self.odd(z): d.setdefault(self.idx(z, K), []).append(z)
        for v in d.values(): v.sort(key=lambda z: (self.size(z), self.arg(z)))
        return d
    # ---- geometry
    def cx(self, z):
        """complex embedding (imaginary rings); for Zr2 the pair of real embeddings"""
        if self.real:
            r = math.sqrt(2)
            return (z[0] + z[1] * r, z[0] - z[1] * r)
        if self.dim == 1: return complex(z[0], 0)
        tau = complex(self.t / 2, math.sqrt(4 * self.n - self.t ** 2) / 2)
        return z[0] + z[1] * tau
    def arg(self, z):
        if self.real or self.dim == 1: return 0.0
        return cmath.phase(self.cx(z))
    # ---- unit action on classes
    def unit_orbits(self, K):
        """orbits of U on odd classes mod pi^K (finite U)"""
        seen = set(); orbs = []
        for i in self.odd_classes(K):
            if i in seen: continue
            c = self.classrep(i, K)
            orb = []
            for u in self.U:
                j = self.idx(self.mul(u, c), K)
                orb.append((u, j))
            seen |= {j for _, j in orb}
            orbs.append(orb)
        return orbs
    def acts_freely(self, K):
        return all(len({j for _, j in o}) == len(self.U) for o in self.unit_orbits(K))
    def canon(self, z, F='R'):
        """representative of the class of associates of z in the fundamental domain F (finite U)"""
        for u in self.U:
            w = self.mul(u, z)
            if inF(self, F, w): return w
        raise ValueError((z, F))


def egcd(a, b):
    if b == 0: return (a, 1, 0) if a >= 0 else (-a, -1, 0)
    g, x, y = egcd(b, a % b)
    return (g, y, x - (a // b) * y)


def inF(R, F, z):
    a, b = z
    if F == 'R':      # right half-plane, Re z > 0, or Re z = 0 and Im z > 0
        s = 2 * a + R.t * b
        return s > 0 or (s == 0 and b > 0)
    if F == 'H':      # upper half-plane, Im z > 0, or Im z = 0 and Re z > 0
        return b > 0 or (b == 0 and a > 0)
    if F == 'O':      # Z: positives
        return a > 0
    if F == 'Q': return a > 0 and b >= 0
    if F == 'C': return a > 0 and -a < b <= a
    if F == 'S': return 0 <= b < a                 # Zw, 0 <= arg < 60
    if F == 'T': return a + b > 0 and 2 * b <= a   # Zw, -30 < arg <= 30
    if F == 'E': return zr2_inE(z)
    raise ValueError(F)


# ---- Zr2: the cone E = {sigma1 > 0, 1 <= |sigma1/sigma2| < eps^2}, one element per class of U = <-1, eps>
def zr2_pos(z):
    a, b = z
    if a >= 0 and b >= 0: return (a, b) != (0, 0)
    if a <= 0 and b <= 0: return False
    if a > 0: return a * a > 2 * b * b
    return 2 * b * b > a * a

def zr2_mul(x, y): return (x[0] * y[0] + 2 * x[1] * y[1], x[0] * y[1] + x[1] * y[0])

def zr2_inE(z):
    if not zr2_pos(z): return False
    if z[0] * z[1] < 0: return False
    w = zr2_mul(z, EPSINV)
    return w[0] * w[1] < 0

def zr2_rep(z):
    """rep of z in E and the unit u with z = u * rep"""
    while True:
        if z[0] * z[1] < 0: z = zr2_mul(z, EPS); continue
        w = zr2_mul(z, EPSINV)
        if w[0] * w[1] >= 0: z = w; continue
        break
    if not zr2_pos(z): z = (-z[0], -z[1])
    return z


def rep(R, F, z):
    if F == 'E': return zr2_rep(z)
    if F == 'A': return z
    return R.canon(z, F)


def fmt(R, z):
    if R.dim == 1: return str(z[0])
    s = {'Zi': 'i', 'Zw': 'w', 'Zs2': 's', 'Z7': 't', 'Z7b': 't', 'Z11': 'f', 'Zr2': 'r'}[R.name]
    a, b = z
    if b == 0: return str(a)
    if a == 0: return f"{b}{s}" if b not in (1, -1) else ('' if b == 1 else '-') + s
    return f"{a}{b:+d}{s}" if b not in (1, -1) else f"{a}{'+' if b == 1 else '-'}{s}"
