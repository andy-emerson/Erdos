# Study 13: inverses mod n by the norm route.
# (1) Rational modulus n: x = a + bi is a unit mod n exactly when N(x) = a^2 + b^2 is, and x^-1 = conj(x) N(x)^-1 mod n.
#     The integer inverse of N(x) mod n by (i) extended Euclid (data-dependent loop, one divide per step) or
#     (ii) Bernstein-Yang divsteps with a fixed iteration count (n odd; no multiply, no divide, masks only).
# (2) Gaussian modulus nu = c + di with gcd(c, d) = 1: Z[i]/(nu) = Z/N(nu), i -> t = -c d^-1 mod N(nu).
#     x = p + qi -> p + q t mod N; invert the integer; the integer z is itself a Gaussian representative of x^-1 mod nu
#     (reduce it mod nu for a small one). The plain norm trick is wrong here (nu = 2 + i, x = 2 - i: checked).
# (3) Batch inversion (Montgomery's trick) over an array sharing n: one inverse and 3(k - 1) Gaussian products mod n.
# Residues lie in [0, n); n < 2^31 (31-bit halves). Reductions are study 4's Barrett.
# Usage: python3 inv_norm.py small | random | bound
import sys, random, math
W = 64; MW = (1 << W) - 1
def barrett(t, n):                       # 7 instructions, 2 multiplies (m = floor(2^64 / n) precomputed per modulus)
    m = (1 << W) // n; r = (t - ((t * m) >> W) * n) & MW; r = (r - n) & MW
    return (r + (n & (MW if r >> 63 else 0))) & MW

# ---- integer inverses of g mod n (0 <= g < n). Each returns (inverse or None, steps).
def inv_euclid(g, n):                    # per step: DIV, MUL, SUB (remainder), MUL, SUB (coefficient), loop test = 6
    r0, r1, t0, t1, steps = n, g, 0, 1, 0
    while r1:
        q = r0 // r1; r0, r1 = r1, r0 - q * r1; t0, t1 = t1, t0 - q * t1; steps += 1
    if r0 != 1: return None, steps
    return t0 + (n if t0 < 0 else 0), steps                  # final: SAR, AND, ADD (3)
def by_iterations(d):                    # Bernstein-Yang's iteration bound for d-bit inputs (from memory; checked by 'bound')
    return (49 * d + 80) // 17 if d < 46 else (49 * d + 57) // 17
def inv_divsteps(g, n, m):               # n odd. Invariants: f = d g0, g = e g0 (mod n). 37 instructions per divstep.
    delta, f, gg, d, e = 1, n, g, 0, 1
    for _ in range(m):
        c = -int(delta > 0 and (gg & 1) == 1)                  # compare, AND, AND, NEG (4)
        t = (f ^ gg) & c; f ^= t; gg ^= t; gg = (gg ^ c) - c    # swap 4, negate g 2: (f, g) <- (g, -f)
        delta = ((delta ^ c) - c) + 1                           # negate 2, +1 (3)
        t = (d ^ e) & c; d ^= t; e ^= t                         # swap 4
        e = e ^ ((e ^ (n - e)) & c)                             # e <- n - e (e in [0, n]): SUB, choice 3 (4)
        b = -(gg & 1)                                           # AND, NEG (2)
        gg = (gg + (f & b)) >> 1                                # AND, ADD, SAR (3)
        e = e + (d & b) - n                                     # AND, ADD, SUB (3)
        e = e + (n & (-1 if e < 0 else 0))                      # SAR, AND, ADD (3): e back in [0, n]
        e = (e + (n & -(e & 1))) >> 1                           # halve mod n: AND, NEG, AND, ADD, SHR (5)
    if gg != 0: return None, -1                                 # the bound failed (never happens if the bound holds)
    if abs(f) != 1: return None, m                              # not invertible: f = +-gcd
    inv = d if f == 1 else n - d                                # sign fix: SAR, choice (4)
    return (0 if inv == n else inv), m                          # final reduce (2)

# ---- the norm route for a rational modulus
def inv_gauss_rational(x, n, method):
    a, b = x                                                    # extract 4
    N = barrett(a * a + b * b, n)                               # 2 MUL, ADD, Barrett 7 (10)
    if method == 'euclid': ni, steps = inv_euclid(N, n)
    else: ni, steps = inv_divsteps(N, n, by_iterations(31))
    if ni is None: return None, steps
    return (barrett(a * ni, n), barrett(((n - b) % n) * ni, n)), steps   # MUL + Barrett 8, SUB + MUL + Barrett 9, pack 2
def gmulmod(x, y, n):                    # study 4: 27 instructions, 8 multiplies
    (a, b), (c, d) = x, y
    return (barrett(a * c + (n * n - b * d), n), barrett(a * d + b * c, n))

# ---- Gaussian modulus via Z/N(nu)
def gauss_mod_setup(c, d):               # once per modulus: N, and t = -c d^-1 mod N (one integer inverse mod N)
    N = c * c + d * d; di, _ = inv_euclid(d % N, N); return N, (-c * di) % N
def gdivides(nu, z):                     # nu | z in Z[i]
    c, d = nu; N = c * c + d * d; p, q = z
    return (p * c + q * d) % N == 0 and (q * c - p * d) % N == 0
def reduce_mod_nu(z, nu):                # nearest-rounding remainder (study 6): small representative
    c, d = nu; N = c * c + d * d; p, q = z
    rnd = lambda u: (2 * u + N) // (2 * N)
    k1, k2 = rnd(p * c + q * d), rnd(q * c - p * d)
    return (p - (k1 * c - k2 * d), q - (k1 * d + k2 * c))
def inv_gauss_nu(x, nu, setup):
    N, t = setup; p, q = x
    y = (p + q * t) % N                                         # map: one 64-bit mulmod (128-bit product) and an add mod N
    z, steps = inv_euclid(y, N)
    if z is None: return None, steps
    return reduce_mod_nu((z, 0), nu), steps                     # back: z + 0i, reduced mod nu (128-bit products)

st = {}; hist = {}
def chk(k, ok):
    st[k + ' cases'] = st.get(k + ' cases', 0) + 1
    if not ok: st[k + ' FAIL'] = st.get(k + ' FAIL', 0) + 1
def note(k, v): hist.setdefault(k, []).append(v)
def check_rational(x, n, brute=False):
    a, b = x; unit = math.gcd(a * a + b * b, n) == 1
    if brute:
        inv = [(c, d) for c in range(n) for d in range(n) if ((a * c - b * d) % n, (a * d + b * c) % n) == (1 % n, 0)]
        chk('rational n: unit iff N(x) is a unit (brute force)', bool(inv) == unit)
    for method in ('euclid', 'divsteps') if n % 2 == 1 else ('euclid',):
        r, steps = inv_gauss_rational(x, n, method)
        if method == 'euclid': note('euclid steps (rational n)', steps)
        if not unit: chk(f'rational n, {method}: non-unit reported', r is None)
        else:
            chk(f'rational n, {method}: x x^-1 = 1', r is not None and gmulmod(x, r, n) == (1 % n, 0))
            if brute: chk(f'rational n, {method}: equals the brute-force inverse', [r] == inv)
def check_nu(x, nu, setup, brute=False):
    c, d = nu; N = setup[0]
    r, steps = inv_gauss_nu(x, nu, setup); note('euclid steps (mod N(nu))', steps)
    if brute:
        reps = [(p, 0) for p in range(N)]                       # Z[i]/(nu) = Z/N: p = 0 .. N-1 are all the classes
        inv = [y for y in reps if gdivides(nu, ((x[0] * y[0] - x[1] * y[1]) - 1, x[0] * y[1] + x[1] * y[0]))]
        chk('Gaussian nu: invertible iff a brute-force inverse exists', (r is not None) == bool(inv))
    if r is not None:
        prod = (x[0] * r[0] - x[1] * r[1], x[0] * r[1] + x[1] * r[0])
        chk('Gaussian nu: x x^-1 = 1 mod nu, representative reduced (2 N(r) <= N(nu))',
            gdivides(nu, (prod[0] - 1, prod[1])) and 2 * (r[0] ** 2 + r[1] ** 2) <= N)
def batch(xs, n):                        # Montgomery's trick
    pre = [xs[0]]
    for x in xs[1:]: pre.append(gmulmod(pre[-1], x, n))         # k - 1 products
    t, _ = inv_gauss_rational(pre[-1], n, 'euclid')
    if t is None: return None                                   # one non-unit poisons the batch
    out = [None] * len(xs)
    for i in range(len(xs) - 1, 0, -1):
        out[i] = gmulmod(pre[i - 1], t, n); t = gmulmod(t, xs[i], n)   # 2 products per element
    out[0] = t; return out

mode = sys.argv[1] if len(sys.argv) > 1 else 'small'
if mode == 'bound':
    # exhaustive check of the iteration bound at small d: every odd f < 2^d and every g < 2^d, count divsteps to g = 0
    def needed(f, g):
        delta, k = 1, 0
        while g:
            if delta > 0 and g & 1: delta, f, g = 1 - delta, g, (g - f) // 2
            else: delta, g = 1 + delta, (g + (g & 1) * f) // 2
            k += 1
        return k
    for dbits in range(1, 12):
        worst = max(needed(f, g) for f in range(1, 1 << dbits, 2) for g in range(1 << dbits))
        chk('divsteps: worst case <= the bound (all odd f, all g < 2^d)', worst <= by_iterations(dbits))
        print(f"  d = {dbits:2d}: worst {worst:3d}, bound {by_iterations(dbits):3d}")
    random.seed(13); w = 0
    for i in range(20000):
        n = random.randrange(1, 1 << 31) | 1; g = random.randrange(n); w = max(w, needed(n, g))
    print(f"  d = 31, 20000 random (n odd, g < n): worst {w}, bound {by_iterations(31)}")
elif mode == 'small':
    for n in range(2, 40):
        for a in range(n):
            for b in range(n): check_rational((a, b), n, brute=True)
    for c in range(-6, 7):
        for d in range(-6, 7):
            if d == 0 or math.gcd(c, d) != 1 or c * c + d * d < 2: continue
            nu = (c, d); setup = gauss_mod_setup(c, d)
            for p in range(-4, 5):
                for q in range(-4, 5): check_nu((p, q), nu, setup, brute=True)
    # the warning case: the norm trick fails for a Gaussian modulus
    nu, x = (2, 1), (2, -1)
    chk('norm trick fails for nu = 2 + i, x = 2 - i (N(x) = 5 not a unit mod 5, yet x = -1 mod nu)',
        math.gcd(5, 5) != 1 and gdivides(nu, (x[0] + 1, x[1])))
    for n in (5, 7, 9, 13, 15):
        for k in (1, 2, 5):
            for i in range(200):
                xs = [(random.randrange(n), random.randrange(n)) for _ in range(k)]
                b_ = batch(xs, n); ind = [inv_gauss_rational(x, n, 'euclid')[0] for x in xs]
                chk('batch = individual inverses (None when any element is not a unit)', b_ == (None if None in ind else ind))
    print("small: rational n = 2 .. 39, all residues, brute force; Gaussian nu with |c|, |d| <= 6, coprime, x in a 9 x 9 box, brute force")
else:
    random.seed(13)
    def isprime(p): return p > 1 and all(p % q for q in range(2, min(p, 50000))) and pow(2, p - 1, p) == 1
    top = (1 << 31) - 1
    ns = [p for p in range(top, top - 300, -1) if isprime(p)][:3] + [top - 1, 3 * 5 * 7 * 11 * 13 * 17 * 19 * 23, 65537 * 3 * 5]
    for n in ns:
        for i in range(4000):
            x = (random.randrange(n), random.randrange(n)) if i % 50 else (random.randrange(n), 0)
            check_rational(x, n)
        xs = [(random.randrange(n), random.randrange(n)) for _ in range(64)]
        b_ = batch(xs, n); ind = [inv_gauss_rational(x, n, 'euclid')[0] for x in xs]
        chk('batch (k = 64) = individual inverses', b_ == (None if None in ind else ind))
    for i in range(300):
        while True:
            c, d = random.randint(-top, top), random.randint(-top, top)
            if d and math.gcd(c, d) == 1: break
        nu = (c, d); setup = gauss_mod_setup(c, d)
        for j in range(30): check_nu((random.randint(-top, top), random.randint(-top, top)), nu, setup)
    print(f"random: rational n in {ns}, 4000 x each; 300 Gaussian nu with 31-bit coprime parts, 30 x each")
for k, v in sorted(st.items()): print(f"  {k}: {v}")
for k, v in hist.items(): print(f"  {k}: mean {sum(v) / len(v):.2f}, max {max(v)}")
