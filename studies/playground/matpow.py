# Study 22: powers A^e of a square n x n Gaussian matrix, residue form against direct (study 18's cost model).
# Entry bound: |z| <= 2^(b - 1/2) for A (b-bit halves); A^(i+j) entries <= n |A^i| |A^j|, so log2 |A^j| <= j (b - 1/2)
# + (j - 1) log2 n, and halves of A^j need x_j = floor(log2 bound) + 2 bits (sign included). Sizes chosen in advance.
# Direct: products by repeated multiplication (A^j = A^(j-1) A) and by repeated squaring (e a power of 2 here).
#   Per term (one complex multiply-add) with operand halves of x and y bits into a result of R bits:
#   both <= 32 bits: study 18's vector forms (1 for <= 8, 2 for <= 16, 8.5 for <= 32);
#   otherwise scalar multiword, calibrated to study 18's b = 64 (32 per term): with wx, wy, wp, wR the 64-bit word counts of
#   the operands, the exact real product and the accumulator: 4 real products of 2 wx wy + 4 (wx wy - 1),
#   2 combines of 2 wp, 2 accumulates of 2 wR + 2. Grows with the square of the width.
# Residue: m primes for the final size (31-bit primes, or the 27-bit lazy variant), Montgomery form throughout (products
#   of Montgomery-form values summed lazily and reduced stay in Montgomery form), convert in once, all products in residue
#   form (repeated squaring), convert out once.
# Usage: python3 matpow.py
import math, random, itertools

def bits_bound(b, n, j): lb = j * (b - 0.5) + (j - 1) * math.log2(n); return math.floor(lb) + 2
def words(bits): return max(1, -(-bits // 64))
def term_direct(x, y, R):
    if x <= 32 and y <= 32:
        mx = max(x, y)
        if mx <= 8 and R <= 32: return 1.0
        if mx <= 16 and R <= 64: return 2.0
        return 8.5
    wx, wy, wp, wR = words(x), words(y), words(x + y + 1), words(R)
    return 4 * (2 * wx * wy + 4 * (wx * wy - 1)) + 2 * (2 * wp) + 2 * (2 * wR + 2)
def direct_mult(n, e, b):                  # A^j = A^(j-1) A, j = 2 .. e
    return sum(n ** 3 * term_direct(bits_bound(b, n, j - 1), b, bits_bound(b, n, j)) for j in range(2, e + 1))
def direct_square(n, e, b):                # e = 2^s: s squarings
    s = int(math.log2(e)); return sum(n ** 3 * term_direct(bits_bound(b, n, 2 ** i), bits_bound(b, n, 2 ** i), bits_bound(b, n, 2 ** (i + 1))) for i in range(s))
def O_R(mp):                               # conversion out per entry (study 16 / 18)
    horner = sum(2 if s == 2 else 5 * words(31 * (s - 1)) for s in range(2, mp + 1))
    return 7.5 * mp + 4 * mp * (mp - 1) + 2 * (horner + 5 * words(31 * mp))
def residue(n, e, b, pbits=31):
    x = bits_bound(b, n, e); mp = math.ceil((x + 0.01) / (pbits - 0.0001))
    per_lane = (4 + 13 / (2 if pbits == 31 else min(n, 2 ** (32 - pbits)))) / 4
    products = int(math.log2(e))
    return n * n * (7 * mp + 0.5) + products * n ** 3 * 2 * mp * per_lane + n * n * O_R(mp), mp

# ---------------- verification: exact big-int powers against (i) the advance bound (the direct widths) and (ii) the residue
def isprime(q):
    if q < 2: return False
    for r in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        if q % r == 0: return q == r
    d, s = q - 1, 0
    while d % 2 == 0: d //= 2; s += 1
    for a in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        y = pow(a, d, q)
        if y in (1, q - 1): continue
        for _ in range(s - 1):
            y = y * y % q
            if y == q - 1: break
        else: return False
    return True
P31 = [q for q in range((1 << 31) - 1, (1 << 31) - 40000, -1) if isprime(q) and q % 4 == 1]
P27 = [q for q in range((1 << 27) - 1, (1 << 27) - 40000, -1) if isprime(q) and q % 4 == 1]
def gm(u, v): return (u[0]*v[0] - u[1]*v[1], u[0]*v[1] + u[1]*v[0])
def matmul(X, Y):
    n = len(X); return [[tuple(sum(gm(X[i][t], Y[t][j])[c] for t in range(n)) for c in (0, 1)) for j in range(n)] for i in range(n)]
def matpow_exact(A, e):
    R = A
    for _ in range(e - 1): R = matmul(R, A)
    return R
def residue_power(A, e, primes, every):    # repeated squaring (e a power of 2) or multiplication, Montgomery form, lazy REDC
    n = len(A); h = 32; Rm = 1 << h; M = math.prod(primes); out = {}
    for p in primes:
        t_ = pow(next(c for c in itertools.count(2) if pow(c, (p - 1) // 2, p) == p - 1), (p - 1) // 4, p)
        ninv = (-pow(p, -1, Rm)) % Rm
        def redc(T):
            assert T < p * Rm
            mm = ((T & (Rm - 1)) * ninv) & (Rm - 1); u = (T + mm * p) >> h; return u - p if u >= p else u
        def mm_(X, Y):                     # plane product, lazy: raw sums of `every` products, then REDC, add mod p
            Z = [[0] * n for _ in range(n)]
            for i in range(n):
                for j in range(n):
                    acc = raw = cnt = 0
                    for t in range(n):
                        raw += X[i][t] * Y[t][j]; cnt += 1
                        if cnt == every: acc = (acc + redc(raw)) % p; raw = cnt = 0
                    if cnt: acc = (acc + redc(raw)) % p
                    Z[i][j] = acc
            return Z
        planes = []
        for c in (0, 1):                   # u plane (a + bt), v plane (a - bt), in Montgomery form (times R)
            P = [[((z[0] + (z[1] if c == 0 else -z[1]) * t_) * Rm) % p for z in row] for row in A]
            if e & (e - 1) == 0:
                for _ in range(int(math.log2(e))): P = mm_(P, P)
            else:
                Q = P
                for _ in range(e - 1): Q = mm_(Q, P)
                P = Q
            planes.append([[v * pow(Rm, -1, p) % p for v in row] for row in P])   # out of Montgomery form
        out[p] = planes
    res = [[None] * n for _ in range(n)]
    for i in range(n):
        for j in range(n):
            parts = []
            for c in (0, 1):
                r = 0; Mi = 1
                for p in primes:
                    t_ = pow(next(cc for cc in itertools.count(2) if pow(cc, (p - 1) // 2, p) == p - 1), (p - 1) // 4, p)
                    u, v = out[p][0][i][j], out[p][1][i][j]
                    val = (u + v) * pow(2, -1, p) % p if c == 0 else (u - v) * pow(2 * t_, -1, p) % p
                    r = r + Mi * ((val - r) * pow(Mi, -1, p) % p); Mi *= p
                parts.append(r - M if r > M // 2 else r)
            res[i][j] = tuple(parts)
    return res
st = {}
def chk(k, ok):
    st[k + ' cases'] = st.get(k + ' cases', 0) + 1
    if not ok: st[k + ' FAIL'] = st.get(k + ' FAIL', 0) + 1
random.seed(22)
for b in (8, 16, 32):
    lo, hi = -(1 << (b - 1)), (1 << (b - 1)) - 1
    for n in (2, 3, 4, 8, 16):
        for e in ((2, 3, 4) if n <= 4 else (2, 4)) + ((8, 16) if n <= 3 else ()):
            A = [[(random.choice([lo, hi, random.randint(lo, hi)]), random.choice([lo, hi, random.randint(lo, hi)])) for _ in range(n)] for _ in range(n)]
            if random.random() < 0.5: A = [[(lo, lo)] * n for _ in range(n)]          # the extreme matrix (largest growth)
            E = matpow_exact(A, e); x = bits_bound(b, n, e)
            chk('direct widths: every entry of A^e fits the advance bound', all(-(1 << (x - 1)) <= v < (1 << (x - 1)) for row in E for z in row for v in z))
            mp = math.ceil((x + 0.01) / 30.9999)
            chk('residue (31-bit primes, REDC every 2 terms) = exact A^e', residue_power(A, e, P31[:mp], 2) == E)
            mp27 = math.ceil((x + 0.01) / 26.9999)
            chk('residue (27-bit primes, REDC every min(n, 32) terms) = exact A^e', residue_power(A, e, P27[:mp27], min(n, 32)) == E)
for k, v in sorted(st.items()): print(f"  {k}: {v}")

# ---------------- the table
print("\n  total instructions: direct by repeated multiplication / direct by squaring / residue 31-bit / residue 27-bit (primes)")
win = {}
for b in (8, 16, 32):
    print(f"  b = {b}:")
    for n in (2, 3, 4, 8, 16):
        cells = []
        for e in (2, 4, 8, 16, 32, 64):
            dm, ds = direct_mult(n, e, b), direct_square(n, e, b)
            r31, m31 = residue(n, e, b); r27, m27 = residue(n, e, b, 27)
            cells.append(f"e={e}: {dm:.0f}/{ds:.0f}/{r31:.0f}/{r27:.0f} ({m31},{m27})")
            for name, r in (('31', r31), ('27', r27)):
                if r < min(dm, ds) and (b, n, name) not in win: win[(b, n, name)] = e
        print("    n = %d: " % n + "; ".join(cells))
print("\n  smallest e (of 2, 4, ..., 64) at which the residue form beats the better direct method:")
for b in (8, 16, 32):
    print(f"    b = {b}: " + "; ".join(f"n = {n}: 31-bit {win.get((b, n, '31'), '>64')}, 27-bit {win.get((b, n, '27'), '>64')}" for n in (2, 3, 4, 8, 16)))
