"""realq.py: arithmetic for real quadratic rings (real-quadratic.md).

Elements (a, b) = a + b*tau with tau^2 = t*tau - n (n < 0), as in qring.py.
  Zr2  t = 0, n = -2   tau = sqrt2          pi = sqrt2       (ramified, q = 2)  eps = 1 + sqrt2   (N = -1)
  Zr3  t = 0, n = -3   tau = sqrt3          pi = 1 + sqrt3   (ramified, q = 2)  eps = 2 + sqrt3   (N = +1)
  Zg5  t = 1, n = -1   tau = (1+sqrt5)/2    pi = 2           (inert,    q = 4)  eps = tau         (N = -1)
Embeddings: sigma1(tau) = (t + sqrt(disc))/2, sigma2(tau) = (t - sqrt(disc))/2, disc = t^2 - 4n.
log2 |sigma_j| is computed exactly enough for huge elements: the larger embedding has no cancellation,
the smaller is |N| / larger.
"""
import math, itertools, random
from functools import lru_cache

RINGS = {
    'Zr2': dict(t=0, n=-2, pi=(0, 1), eps=(1, 1), sym='r'),
    'Zr3': dict(t=0, n=-3, pi=(1, 1), eps=(2, 1), sym='s'),
    'Zg5': dict(t=1, n=-1, pi=(2, 0), eps=(0, 1), sym='g'),
}


def egcd(a, b):
    if b == 0: return (a, 1, 0) if a >= 0 else (-a, -1, 0)
    g, x, y = egcd(b, a % b)
    return (g, y, x - (a // b) * y)


class RQ:
    def __init__(self, name):
        s = RINGS[name]
        self.name = name; self.t = s['t']; self.n = s['n']; self.pi = s['pi']; self.eps = s['eps']; self.sym = s['sym']
        self.dim = 2; self.real = True
        self.disc = self.t ** 2 - 4 * self.n
        self.rd = math.sqrt(self.disc)
        self.Npi = self.N(self.pi); self.q = abs(self.Npi)
        self.epsinv = self.inv_unit(self.eps)
        self.U = [(1, 0)]        # for ringrun headers (literal mode needs no units)
        self.lpi = self.lg(self.pi)
        self.leps = math.log2(self.s1(self.eps))

    # arithmetic
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
    def inv_unit(self, u):
        c = self.conj(u); s = self.N(u)
        return (c[0] * s, c[1] * s)
    def pw(self, x, k):
        r = (1, 0)
        if k < 0: x = self.inv_unit(x); k = -k
        for _ in range(k): r = self.mul(r, x)
        return r
    def div_exact(self, x, y):
        Ny = self.N(y); w = self.mul(x, self.conj(y))
        if w[0] % Ny or w[1] % Ny: return None
        return (w[0] // Ny, w[1] // Ny)
    def is_unit(self, x): return abs(self.N(x)) == 1
    def div_pi(self, x): return self.div_exact(x, self.pi)
    def odd(self, x): return x != (0, 0) and self.div_pi(x) is None
    def oddpart(self, x):
        v = 0
        while x != (0, 0):
            y = self.div_pi(x)
            if y is None: break
            x = y; v += 1
        return x, v

    # embeddings
    def s1(self, z): return z[0] + z[1] * (self.t + self.rd) / 2
    def s2(self, z): return z[0] + z[1] * (self.t - self.rd) / 2
    def sign1(self, z):
        """exact sign of sigma1(z): sigma1 = (x + y sqrt disc)/2"""
        x = 2 * z[0] + self.t * z[1]; y = z[1]
        if x >= 0 and y >= 0: return 1 if (x or y) else 0
        if x <= 0 and y <= 0: return -1
        if x > 0: return 1 if x * x > y * y * self.disc else -1
        return 1 if y * y * self.disc > x * x else -1
    def big1(self, z):
        """|sigma1(z)| >= |sigma2(z)|"""
        return z[1] * (2 * z[0] + self.t * z[1]) >= 0
    def lg(self, z):
        """(log2|sigma1 z|, log2|sigma2 z|)"""
        x = 2 * z[0] + self.t * z[1]; y = z[1]
        Nz = abs(self.N(z))
        S = 64
        big = abs(x) * (1 << S) + math.isqrt(y * y * self.disc * (1 << (2 * S)))   # (|x| + |y| sqrt disc) 2^S
        lbig = math.log2(big) - S - 1
        lsmall = math.log2(Nz) - lbig if Nz else -999.0
        return (lbig, lsmall) if x * y >= 0 else (lsmall, lbig)
    def size(self, z):
        """Minkowski size sigma1^2 + sigma2^2 (an integer: Tr(z^2))"""
        a, b = z
        s1s = self.mul(z, z)
        return 2 * s1s[0] + self.t * s1s[1]

    # units
    def unit_exp(self, u):
        """u = sgn * eps^m: returns (sgn, m), or None if u is not a unit"""
        if not self.is_unit(u): return None
        l1 = self.lg(u)[0]
        m = round(l1 / self.leps)
        w = self.mul(u, self.pw(self.eps, -m))
        if w == (1, 0): return (1, m)
        if w == (-1, 0): return (-1, m)
        return None

    # residue classes mod pi^K (HNF basis (A,0),(c,g))
    @lru_cache(maxsize=None)
    def hnf(self, K):
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
        return (x, y)          # idx((x, y)) = y A + x (qring.py returns (x + y c, y), which is not in class i when y c != 0 mod A)
    @lru_cache(maxsize=None)
    def odd_classes(self, K):
        return tuple(i for i in range(self.nclasses(K)) if self.odd(self.classrep(i, K)))

    # elements by Minkowski size
    def elements(self, bound):
        out = []
        R = int(math.isqrt(bound)) + 2
        for b in range(-R, R + 1):
            for a in range(-2 * R - abs(self.t * b), 2 * R + abs(self.t * b) + 1):
                if (a, b) != (0, 0) and self.size((a, b)) <= bound: out.append((a, b))
        return out
    def by_class(self, K, bound):
        d = {}
        for z in self.elements(bound):
            if self.odd(z): d.setdefault(self.idx(z, K), []).append(z)
        for v in d.values(): v.sort(key=lambda z: (self.size(z), z))
        return d
    def cand_lists(self, alpha, k, nsmall):
        """for each odd class c mod pi^k: digits of the class of -alpha c, the nsmall smallest (ties at the cut kept),
        and the smallest ones"""
        bound = 4 * self.q ** k + 16
        while True:
            bc = self.by_class(k, bound)
            if all(i in bc and len(bc[i]) >= nsmall for i in self.odd_classes(k)): break
            bound *= 2
        out = {}
        for i in self.odd_classes(k):
            c = self.classrep(i, k)
            j = self.idx(self.neg(self.mul(alpha, c)), k)
            L = bc[j]
            m = self.size(L[0]); cut = self.size(L[nsmall - 1])
            out[i] = ([z for z in L if self.size(z) <= cut], [z for z in L if self.size(z) == m])
        return out

    # the cone E = {sigma1 > 0, 1 <= |sigma1/sigma2| < eps^2}: one element per class of associates
    def inE(self, z):
        if self.sign1(z) <= 0 or not self.big1(z): return False
        return not self.big1(self.mul(z, self.epsinv))
    def rep(self, z):
        """representative in E and the unit u with z = u * rep"""
        u = (1, 0)
        while True:
            if not self.big1(z): z = self.mul(z, self.eps); u = self.mul(u, self.epsinv); continue
            w = self.mul(z, self.epsinv)
            if self.big1(w): z = w; u = self.mul(u, self.eps); continue
            break
        if self.sign1(z) < 0: z = self.neg(z); u = self.neg(u)
        return z, u

    def fmt(self, z):
        a, b = z; s = self.sym
        if b == 0: return str(a)
        if a == 0: return f"{b}{s}" if b not in (1, -1) else ('' if b == 1 else '-') + s
        return f"{a}{b:+d}{s}" if b not in (1, -1) else f"{a}{'+' if b == 1 else '-'}{s}"
    def ufmt(self, u):
        e = self.unit_exp(u)
        if e is None: return self.fmt(u)
        sg, m = e
        return ('-' if sg < 0 else '') + ('1' if m == 0 else f"e^{m}")

    # maps: residue-class table d (dict class index -> digit) at modulus pi^k
    def table_ok(self, alpha, tab, k):
        return all(self.idx(self.add(self.mul(alpha, self.classrep(i, k)), d), k) == 0 for i, d in tab.items())
    def S(self, alpha, tab, k, z):
        w = self.add(self.mul(alpha, z), tab[self.idx(z, k)])
        if w == (0, 0): return (0, 0), 0
        return self.oddpart(w)
    def commutes_pm1(self, tab, k):
        for i, d in tab.items():
            j = self.idx(self.neg(self.classrep(i, k)), k)
            if j == i or tab[j] != self.neg(d): return False
        return True

    def window_drifts(self, alpha, k):
        """predicted drift per odd step in each embedding, bits: l_j(alpha) - (k + 1/(q-1)) l_j(pi)"""
        m = k + 1 / (self.q - 1)
        la = self.lg(alpha)
        return tuple(la[j] - m * self.lpi[j] for j in range(2))


def describe(R, tab, k):
    return ", ".join(f"[{R.fmt(R.classrep(i, k))}]->{R.fmt(d)}" for i, d in sorted(tab.items()))


def odd_multipliers(R, k, assoc_range=6):
    """odd alpha up to sign with |N| in the norm window, all associates alpha eps^m, |m| <= assoc_range, reduced so
    that the representative of each class of associates is listed once per m"""
    lo, hi = R.q ** k, R.q ** (k + 1 / (R.q - 1))
    reps = {}
    B = int(4 * hi) + 20
    for z in R.elements(B):
        if not R.odd(z): continue
        n = abs(R.N(z))
        if lo < n < hi:
            r, _ = R.rep(z); reps[r] = n
    return sorted(reps.items(), key=lambda x: (x[1], x[0]))
