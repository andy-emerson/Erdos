import random, sys
random.seed(1)

def s(v, w):  # signed wrap to w bits
    v &= (1 << w) - 1
    return v - (1 << w) if v >> (w - 1) else v

def inv2(n, w):  # odd n, inverse mod 2^w by Newton (Hensel lifting)
    x = n  # correct to 3 bits
    bits = 3
    while bits < w:
        x = x * (2 - n * x) % (1 << w); bits *= 2
    return x % (1 << w)

def fits(v, w):
    return -(1 << (w - 1)) <= v < (1 << (w - 1))

def ctz(v):
    return (v & -v).bit_length() - 1 if v else 99

# (a) divisibility test: strip y to odd y'', Hensel quotient q mod 2^w, then y | x iff
#     the low bits of x allow the strip and q*y'' fits w bits (its low bits equal x's automatically).
def divides_hensel(x, y, w):
    a, b = x; c, d = y
    if c == 0 and d == 0:
        return a == 0 and b == 0
    k = min(ctz(c), ctz(d))
    # strip 2^k from y; x must have 2^k
    if min(ctz(a), ctz(b)) < k:
        return False
    a >>= k; b >>= k; c >>= k; d >>= k
    if (c + d) % 2 == 0:  # y has one more 1+i; x must too
        if (a + b) % 2:
            return False
        a, b = (a + b) >> 1, (b - a) >> 1   # x/(1+i)
        c, d = (c + d) >> 1, (d - c) >> 1
    N = c * c + d * d  # odd
    Ni = inv2(N, w)
    u = a * c + b * d; v = b * c - a * d   # x * conj(y)
    qr = s(u * Ni, w); qi = s(v * Ni, w)
    # back product q*y'' ; its parts are congruent to x's mod 2^w
    pr = qr * c - qi * d; pi = qr * d + qi * c
    assert s(pr, w) == s(a, w) and s(pi, w) == s(b, w)
    return fits(pr, w) and fits(pi, w)

def divides_ref(x, y):
    a, b = x; c, d = y
    if c == 0 and d == 0:
        return a == 0 and b == 0
    N = c * c + d * d
    return (a * c + b * d) % N == 0 and (b * c - a * d) % N == 0

def check_a(w, lo, hi):
    bad = 0; cnt = 0; edge = 0
    R = range(lo, hi)
    for a in R:
        for b in R:
            for c in R:
                for d in R:
                    r1 = divides_ref((a, b), (c, d))
                    r2 = divides_hensel((a, b), (c, d), w)
                    cnt += 1
                    if r1 != r2:
                        bad += 1
                        if bad <= 5: print('  mismatch', (a, b), (c, d), r1, r2)
    print(f'(a) w={w} range [{lo},{hi}) pairs={cnt} mismatches={bad}')

check_a(6, -32, 32) if '--big' in sys.argv else check_a(5, -16, 16)

def check_a_random(w, n):
    bad = 0; hits = 0
    for _ in range(n):
        c = random.randrange(-(1 << (w-1)) + 1, 1 << (w-1)); d = random.randrange(-(1 << (w-1)) + 1, 1 << (w-1))
        if random.random() < 0.5:
            # build a multiple that fits
            sh = random.randrange(0, w)
            qa = random.randrange(-(1 << max(0, w-1-sh)), 1 << max(0, w-1-sh)); qb = random.randrange(-(1 << max(0, w-1-sh)), 1 << max(0, w-1-sh))
            a = qa * c - qb * d; b = qa * d + qb * c
            if not (fits(a, w) and fits(b, w)): continue
        else:
            a = random.randrange(-(1 << (w-1)), 1 << (w-1)); b = random.randrange(-(1 << (w-1)), 1 << (w-1))
        r1 = divides_ref((a, b), (c, d)); r2 = divides_hensel((a, b), (c, d), w)
        hits += r1
        bad += r1 != r2
    print(f'(a) random w={w}: {n} cases, {hits} divisible, mismatches={bad}')
check_a_random(32, 200000)

# (b) exact integer sqrt of a perfect square by 2-adic Newton (inverse sqrt), w = 64
def isqrt_exact(N, w=64):
    if N == 0: return 0
    t = ctz(N)
    if t % 2: return None
    M = N >> t
    if M % 8 != 1: return None          # odd squares are 1 mod 8
    # y = M^(-1/2) mod 2^w : y <- y(3 - M y^2)/2, start y = 1 (correct mod 8 since M = 1 mod 8)
    mod = 1 << w
    y = 1; bits = 3
    while bits < w - 1:
        y = (y * (3 - M * y * y) >> 1) % mod  # exact halving: 3 - M y^2 is even
        bits = 2 * bits - 2
    r = (M * y) % mod                    # a 2-adic sqrt of M mod 2^(w-1)
    # four roots mod 2^(w-1): +-r, +-r + 2^(w-2); take the smallest nonnegative
    h = 1 << (w - 2); m2 = 1 << (w - 1)
    cands = [r % m2, (-r) % m2, (r + h) % m2, (-r + h) % m2]
    r = min(cands)
    if r * r != M: return None          # in the textbook: r < 2^((w-1)/2) test, then r*r = M follows
    return r << (t // 2)

bad = 0
for _ in range(100000):
    x = random.randrange(0, 1 << 31)
    N = x * x
    if isqrt_exact(N) != x: bad += 1
for _ in range(100000):
    N = random.randrange(0, 1 << 62)
    r = isqrt_exact(N)
    import math
    ok = (r == math.isqrt(N)) if math.isqrt(N) ** 2 == N else r is None
    bad += not ok
for N in range(0, 1 << 16):
    import math
    r = isqrt_exact(N); ok = (r == math.isqrt(N)) if math.isqrt(N) ** 2 == N else r is None
    bad += not ok
print('(b) exact sqrt by 2-adic Newton: mismatches', bad)
# Gaussian absolute value: Pythagorean cases
trip = sum(1 for a in range(-60, 61) for b in range(-60, 61) if isqrt_exact(a*a+b*b) is not None)
print('    points in [-60,60]^2 with exact |x|:', trip)

# (c) bulk coefficient fix in the extended gcd: a = ua A' + va B' divisible by (1+i)^t, B' odd.
def gmul(x, y): return (x[0]*y[0]-x[1]*y[1], x[0]*y[1]+x[1]*y[0])
def gsub(x, y): return (x[0]-y[0], x[1]-y[1])
def gadd(x, y): return (x[0]+y[0], x[1]+y[1])
def val(x):
    a, b = x
    if a == 0 and b == 0: return 999
    v = 0
    while (a + b) % 2 == 0:
        a, b = (a + b) // 2, (b - a) // 2; v += 1
    return v
def gdiv_pi(x, t):
    a, b = x
    for _ in range(t): a, b = (a + b) // 2, (b - a) // 2
    return (a, b)
def ginv_mod2(y, w):  # y odd, inverse mod 2^w in Z[i]
    N = y[0]**2 + y[1]**2; Ni = inv2(N, w)
    return ((y[0] * Ni) % (1 << w), (-y[1] * Ni) % (1 << w))
bad = 0; maxbits = 0
for _ in range(20000):
    A = (random.randrange(-2**31, 2**31), random.randrange(-2**31, 2**31))
    B = (random.randrange(-2**31, 2**31) | 1, random.randrange(-2**31, 2**31) & ~1)  # B odd
    ua = (random.randrange(-2**33, 2**33), random.randrange(-2**33, 2**33))
    va = (random.randrange(-2**33, 2**33), random.randrange(-2**33, 2**33))
    a = gadd(gmul(ua, A), gmul(va, B))
    t = min(val(a), 40)
    if t == 0: continue
    w = (t + 1) // 2 + 1   # (1+i)^t divides 2^ceil(t/2); work mod 2^w, w >= ceil(t/2)
    Bi = ginv_mod2(B, w)
    m = gmul(ua, Bi); m = ((-m[0]) % (1 << w), (-m[1]) % (1 << w))
    m = (s(m[0], w), s(m[1], w))
    u2 = gadd(ua, gmul(m, B)); v2 = gsub(va, gmul(m, A))
    if val(u2) < t or val(v2) < t: bad += 1; continue
    u3 = gdiv_pi(u2, t); v3 = gdiv_pi(v2, t)
    if gadd(gmul(u3, A), gmul(v3, B)) != gdiv_pi(a, t): bad += 1
print('(c) bulk Montgomery-style coefficient fix: failures', bad)
