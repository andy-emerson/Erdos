# Recursive (half-gcd) binary gcd and quartic residue symbol in Z[i], built on step S.
# Plain Python, exact integers only.  See recursive.md.
#
# State convention.  A state is a pair (A, Y): A primary (A = 1 mod pi^3, pi = 1+i),
# Y = 0 or v(Y) = g >= 1.  The target symbol is [Y/A].  One step:
#   Y = i^s pi^g C, C primary           e += g k1(A) + s ki(A) + recip(C, A)   (now [A/C])
#   q = minimal rep of A/C mod pi^(g+delta)
#   r = A - q C,  Y' = r / pi^g          e += g k1(C)                         (now [Y'/C])
#   new state (C, Y').
# In matrix form 2^g (C, Y')^T = S (A, Y)^T with S = i^-g [[0, i^-s pi^g], [pi^g, -q i^-s]].
# A product of steps gives 2^j (A_m, Y_m)^T = R (A_0, Y_0)^T, j = total valuation consumed.
# The quotient sequence is the one of alg.sym_S (same q, same gaps, same units).

import sys
sys.setrecursionlimit(10000)

# ---------------------------------------------------------------- operation counter
class Counter:
    def __init__(self):
        self.mults = []          # (bits_a, bits_b) of every counted integer product
        self.calls = 0           # hgcd calls (all)
        self.leaves = 0          # hgcd calls answered by the quadratic base case
        self.leafsteps = 0       # steps done in leaves
        self.midsteps = 0        # explicit middle steps
        self.maxdepth = 0
        self.inv_bits = []       # sizes h of the inverses mod 2^h
CNT = None
def imul(a, b):
    if CNT is not None and a and b:
        CNT.mults.append((abs(a).bit_length(), abs(b).bit_length()))
    return a * b

# ---------------------------------------------------------------- Gaussian arithmetic
def gmul(x, y):                       # counted (4 integer products)
    return (imul(x[0], y[0]) - imul(x[1], y[1]), imul(x[0], y[1]) + imul(x[1], y[0]))
def gadd(x, y): return (x[0] + y[0], x[1] + y[1])
def gsub(x, y): return (x[0] - y[0], x[1] - y[1])
def unit(s, z):                       # i^s * z, free
    s %= 4; x, y = z
    return [(x, y), (-y, x), (-x, -y), (y, -x)][s]
def mul_pi(z, m):                     # pi^m z, free: pi^2 = 2i
    x, y = unit(m // 2, z); x <<= (m // 2); y <<= (m // 2)
    if m % 2: x, y = x - y, x + y
    return (x, y)
def div_pi(z, m):                     # z / pi^m, exact, free
    x, y = z; h = m // 2
    if m % 2:                          # divide by (1+i): (x+iy)(1-i)/2
        x, y = (x + y), (y - x)
        assert x % 2 == 0 and y % 2 == 0
        x >>= 1; y >>= 1
    assert x % (1 << h) == 0 and y % (1 << h) == 0
    return unit(-h, (x >> h, y >> h))
def v2(n): return (n & -n).bit_length() - 1
def val(z):                           # pi-adic valuation, None for 0
    x, y = z
    if x == 0 and y == 0: return None
    h = min(v2(x) if x else 10**18, v2(y) if y else 10**18)
    x >>= h; y >>= h
    return 2 * h + (1 if (x & 1) and (y & 1) else 0)
def primary(w):                       # w odd -> (s, p), w = i^s p, p primary
    for s in range(4):
        p = unit(-s, w)
        if p[1] % 2 == 0 and (p[0] + p[1]) % 4 == 1: return s, p
    raise ValueError(w)
def trunc(z, h):                      # z mod 2^h coordinatewise (finer than mod pi^(2h))
    M = (1 << h) - 1
    return (z[0] & M, z[1] & M)
def bits(z): return max(abs(z[0]).bit_length(), abs(z[1]).bit_length())

# symbol exponents (as in g.py)
def k1(b): c, d = b; return ((c - d - d * d - 1) // 4) % 4
def ki(b): c, d = b; return (-(c - 1) // 2) % 4
def recip(x, y): return 0 if ((x[0] - 1) // 2) * ((y[0] - 1) // 2) % 2 == 0 else 2

# ---------------------------------------------------------------- the quotient
def centred(x, M):
    x %= M
    return x - M if x >= M // 2 else x
def minrep(A, C, m):
    """minimal representative of A/C mod pi^m, identical to alg.pimod_q.
    Only A, C mod 2^h are used, h = ceil(m/2)."""
    h = (m + 1) // 2; M = 1 << h
    a = trunc(A, h); c = trunc(C, h)
    n = (c[0] * c[0] + c[1] * c[1]) & (M - 1)         # N(C) mod 2^h (h-bit operands)
    if CNT is not None:
        CNT.inv_bits.append(h); CNT.mults.extend([(h, h)] * 2)
    iv = pow(n, -1, M)
    p = gmul(a, (c[0], -c[1]))                         # A conj(C), h-bit operands
    q = (centred(imul(p[0] & (M - 1), iv), M), centred(imul(p[1] & (M - 1), iv), M))
    # reduce modulo the lattice pi^m Z[i]: q - round(q/P) P, P = pi^m (shifts only)
    P = mul_pi((1, 0), m); nP = 1 << m
    u = (q[0] * P[0] + q[1] * P[1], q[1] * P[0] - q[0] * P[1])   # q conj(P): a shift
    rr = ((2 * u[0] + nP) // (2 * nP), (2 * u[1] + nP) // (2 * nP))
    return gsub(q, mul_pi(rr, m))

# ---------------------------------------------------------------- one step
def step(A, Y, delta):
    """one step from state (A, Y), Y != 0. Returns (C, Y', g, de, q, s, S)."""
    g = val(Y); w = div_pi(Y, g); s, C = primary(w)
    de = g * k1(A) + s * ki(A) + recip(C, A)
    q = minrep(A, C, g + delta)
    r = gsub(A, gmul(q, C))
    de += g * k1(C)
    Yn = div_pi(r, g)
    pg = mul_pi((1, 0), g)
    S = ((unit(-g, (0, 0)), unit(-g - s, pg)),
         (unit(-g, pg), unit(-g - s, (-q[0], -q[1]))))
    return C, Yn, g, de % 4, q, s, S

I2 = (((1, 0), (0, 0)), ((0, 0), (1, 0)))
def mmul(X, Y):  # 2x2 over Z[i], counted
    return tuple(tuple(gadd(gmul(X[r][0], Y[0][c]), gmul(X[r][1], Y[1][c])) for c in range(2))
                 for r in range(2))
def mmul_step(S, R):
    """S R where S is a step matrix: the entries of S other than q are units times pi^g,
    so only the products by -q i^-s are real multiplications."""
    def sc(z, w):   # z * w where z is a unit times a power of pi: free; else counted
        if z == (0, 0): return (0, 0)
        if (z[0] == 0 or z[1] == 0 or abs(z[0]) == abs(z[1])) and bin(abs(z[0]) | abs(z[1])).count('1') == 1:
            return (z[0] * w[0] - z[1] * w[1], z[0] * w[1] + z[1] * w[0])
        return gmul(z, w)
    return tuple(tuple(gadd(sc(S[r][0], R[0][c]), sc(S[r][1], R[1][c])) for c in range(2))
                 for r in range(2))
def apply(R, A, Y, j):
    x = gadd(gmul(R[0][0], A), gmul(R[0][1], Y))
    y = gadd(gmul(R[1][0], A), gmul(R[1][1], Y))
    sh = 1 << j
    assert x[0] % sh == 0 and x[1] % sh == 0 and y[0] % sh == 0 and y[1] % sh == 0
    return (x[0] >> j, x[1] >> j), (y[0] >> j, y[1] >> j)

# ---------------------------------------------------------------- naive loop
def naive(A, Y, K, delta, record=None):
    """steps while the pending valuation j + v(Y) <= K (K=None: until Y = 0).
    Returns (j, R, e, A, Y, nsteps)."""
    j = 0; R = I2; e = 0; n = 0
    while Y != (0, 0):
        g = val(Y)
        if K is not None and j + g > K: break
        A, Y, g, de, q, s, S = step(A, Y, delta)
        if record is not None: record.append((q, g, s))
        R = mmul_step(S, R); j += g; e += de; n += 1
    return j, R, e % 4, A, Y, n

# ---------------------------------------------------------------- recursion
LEAF = 24
def c0(delta): return max(delta, 7)
def hgcd(A, Y, K, delta, record=None, depth=0):
    """all steps whose pending valuation is <= K, using only A, Y mod pi^(2K + c0).
    Returns (j, R, e) with 2^j (A_m, Y_m) = R (A, Y)."""
    if CNT is not None:
        CNT.calls += 1; CNT.maxdepth = max(CNT.maxdepth, depth)
    if Y == (0, 0) or val(Y) > K: return 0, I2, 0
    if K <= LEAF:
        if CNT is not None: CNT.leaves += 1
        j, R, e, _, _, n = naive(A, Y, K, delta, record)
        if CNT is not None: CNT.leafsteps += n
        return j, R, e
    hc = (c0(delta) + 1) // 2
    K1 = K // 2
    j1, R1, e1 = hgcd(trunc(A, K1 + hc), trunc(Y, K1 + hc), K1, delta, record, depth + 1)
    A1, Y1 = apply(R1, A, Y, j1)
    if Y1 == (0, 0) or j1 + val(Y1) > K: return j1, R1, e1
    C, Y2, g, e0, q, s, S = step(A1, Y1, delta)      # middle step, full precision
    if CNT is not None: CNT.midsteps += 1
    if record is not None: record.append((q, g, s))
    j = j1 + g; K2 = K - j
    j2, R2, e2 = hgcd(trunc(C, K2 + hc), trunc(Y2, K2 + hc), K2, delta, record, depth + 1)
    R = mmul(R2, mmul_step(S, R1))
    return j + j2, R, (e1 + e0 + e2) % 4

# ---------------------------------------------------------------- full gcd and symbol
def setup(a, b):
    """[a/b] = i^e [Y/A] with (A, Y) the start state (A0, pi B0); None if a = 0."""
    _, B0 = primary(b)
    if a == (0, 0): return None, B0
    t = val(a); s, A0 = primary(div_pi(a, t))
    e = t * k1(B0) + s * ki(B0) + recip(A0, B0) - k1(A0)
    return (A0, mul_pi(B0, 1), e % 4), B0

def quartic(a, b, delta=2, recursive=True, record=None, early=True, alpha=1.0, rounds=None):
    """returns (symbol exponent or None, gcd primary).  b must be odd."""
    st, B0 = setup(a, b)
    if st is None: return (0 if B0 == (1, 0) else None), B0
    A, Y, e = st
    while Y != (0, 0):
        if early and A == (1, 0): break       # [Y/1] = 1, gcd 1
        if recursive:
            K = max(val(Y), int(alpha * max(bits(A), bits(Y))))
            j, R, de = hgcd(A, Y, K, delta, record)
            A, Y = apply(R, A, Y, j)
        else:
            _, _, de, A, Y, _ = naive(A, Y, None, delta, record)
        if rounds is not None: rounds.append((bits(A), bits(Y)))
        e += de
    return (e % 4 if A == (1, 0) else None), A
