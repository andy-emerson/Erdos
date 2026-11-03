# Study 18: Gaussian matrix products, direct against residue (study 16), and where the residue form starts paying off.
# C = A B, A n x k, B k x m, entries with b-bit halves (full two's-complement range). |re C|, |im C| <= k 2^(2b-1), so the
# result needs R = 2b + ceil(log2 k) + 1 bits, chosen in advance (no runtime overflow check).
# Direct (vector, layout (b) of study 17, separate re/im arrays): products at double width, sums at the width R needs.
# Residue: m 31-bit primes p = 1 (mod 4), M > 2^R; per prime the split form (u, v), so a Gaussian multiply-add is two lane
# multiply-adds. Lazy accumulation: raw 62-bit products summed in 64-bit lanes and reduced by Montgomery REDC (R = 2^32)
# every 2 terms (REDC needs t < p 2^32 and 2 (p - 1)^2 < p 2^32). The R^-1 factors are folded into the conversion out.
# Usage: python3 matmul.py
import random, math, itertools
from fractions import Fraction

# ---------------------------------------------------------------- cost model (instructions; 128-bit vector registers)
def need_bits(b, k): return 2 * b + max(0, math.ceil(math.log2(k))) + 1
def primes_for(R, pbits=31): return math.ceil((R + 0.01) / (pbits - 0.0001))
# per element-term (one complex multiply-add C_ij += A_it B_tj), direct, layout (b):
#   b = 8:  16-bit inputs, 16 -> 32 widening multiply-accumulate (NEON smlal, RVV vwmacc; B keeps a negated copy of im for
#           the -bd term), 4 per register of four 32-bit accumulators                                        -> 1
#   b = 16: 32-bit inputs, 32 -> 64 widening multiply-accumulate, 4 per register of two 64-bit accumulators   -> 2
#   b = 32: 4 widening 32 -> 64 multiplies; ac - bd is exact in 64 bits (1), but ad + bc reaches 2^63 at the full-range
#           corner (all halves -2^31), so ad and bc are split separately. Each split: low 32 (AND), high 32 (arithmetic
#           shift), added to two 64-bit sums (4). Per two elements: 4 + (1 + 4) + (4 + 4) = 17                 -> 8.5
#   b = 64: scalar (no portable vector 64 x 64 -> 128): 4 products (MUL, MULHI) 8, re and im as 2-word sub/add 8,
#           two 3-word accumulates 16                                                                        -> 32
T_D = {8: 1.0, 16: 2.0, 32: 8.5, 64: 32.0}
P_D = {8: 0.25, 16: 0.5, 32: 0.0, 64: 0.0}        # per input entry: widen to the multiply's input width (and B's -im copy)
Q_D = {8: 0.0, 16: 0.0, 32: 8.0, 64: 0.0}         # per output entry: combine the split sums (b = 32)
A_OVERHEAD = {8: 0.75, 16: 1.5, 32: 1.5, 64: 0.0} # layout (a): per-term lane rearrangement (shifts, pair swap; study 17)
# residue, per residue-lane-term (32-bit residue lanes, 4 per register): widening multiply 2 + 64-bit add 2 per register,
# and every 2 terms: REDC 10 + add mod p 3 per register  -> (4 + 13/2) / 4 = 2.625 per lane-term; 2 lanes per prime
def T_R(mp, lazy=True): return 2 * mp * ((4 + 13 / 2) / 4 if lazy else (2 + 10 + 3) / 4)
def I_R(b, mp): return (7 * mp + 0.5) if b <= 32 else (14 * mp + 1)       # conversion in per entry (study 16; b = 64 split)
def O_R(mp):                                                                 # conversion out per entry (study 16, vector part
    words = lambda bits: -(-bits // 64)                                      # + scalar Horner and centering)
    horner = sum(2 if s == 2 else 5 * words(31 * (s - 1)) for s in range(2, mp + 1))
    return 7.5 * mp + 4 * mp * (mp - 1) + 2 * (horner + 5 * words(31 * mp))
def direct_cost(n, k, m, b, layout='b'):
    t = T_D[b] + (A_OVERHEAD[b] if layout == 'a' else 0)
    return n * k * m * t + (n * k + k * m) * P_D[b] + n * m * Q_D[b]
def residue_cost(n, k, m, b, lazy=True, pbits=31):
    mp = primes_for(need_bits(b, k), pbits)
    t = T_R(mp, lazy) if pbits == 31 else 2 * mp * ((4 + 13 / (2 ** (32 - pbits))) / 4)
    return n * k * m * t + (n * k + k * m) * I_R(b, mp) + n * m * O_R(mp), mp

# ---------------------------------------------------------------- functional models (verified against exact products)
def sx(v, w): v &= (1 << w) - 1; return v - (1 << w) if v >> (w - 1) else v
def gm(x, y): return (x[0]*y[0] - x[1]*y[1], x[0]*y[1] + x[1]*y[0])
def exact(A, B):
    n, k, m = len(A), len(B), len(B[0])
    return [[tuple(sum(gm(A[i][t], B[t][j])[c] for t in range(k)) for c in (0, 1)) for j in range(m)] for i in range(n)]
def direct(A, B, b):                       # the chosen widths, wrapping like the machine; exact iff the widths suffice
    n, k, m = len(A), len(B), len(B[0]); R = need_bits(b, k)
    out = [[None] * m for _ in range(n)]
    for i in range(n):
        for j in range(m):
            if b <= 16 or b < 32:          # one accumulator of width 32 (b <= 8) or 64
                Wa = 32 if b <= 8 else 64; assert R <= Wa
                re = im = 0
                for t in range(k):
                    (a, bb), (c, d) = A[i][t], B[t][j]
                    re = sx(re + a*c - bb*d, Wa); im = sx(im + a*d + bb*c, Wa)
                out[i][j] = (re, im)
            elif b == 32:                  # exact 64-bit parts, split sums (low 32 unsigned, high 32 signed) in 64-bit lanes
                acc = [[0, 0], [0, 0]]
                for t in range(k):
                    (a, bb), (c, d) = A[i][t], B[t][j]
                    for q, part in ((0, sx(a*c - bb*d, 64)), (1, sx(a*d, 64)), (1, sx(bb*c, 64))):
                        acc[q][0] = sx(acc[q][0] + (part & 0xffffffff), 64); acc[q][1] = sx(acc[q][1] + (part >> 32), 64)
                out[i][j] = tuple(acc[q][1] * (1 << 32) + acc[q][0] for q in (0, 1))
            else:                          # b = 64: 3-word (192-bit) accumulators
                re = im = 0
                for t in range(k):
                    (a, bb), (c, d) = A[i][t], B[t][j]
                    re = sx(re + a*c - bb*d, 192); im = sx(im + a*d + bb*c, 192)
                out[i][j] = (re, im)
    return out
def isprime(n):
    if n < 2: return False
    for q in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        if n % q == 0: return n == q
    d, s = n - 1, 0
    while d % 2 == 0: d //= 2; s += 1
    for a in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        x = pow(a, d, n)
        if x in (1, n - 1): continue
        for _ in range(s - 1):
            x = x * x % n
            if x == n - 1: break
        else: return False
    return True
PR31 = [p for p in range((1 << 31) - 1, (1 << 31) - 6000, -1) if isprime(p) and p % 4 == 1][:8]
def residue(A, B, primes, h=32, every=2):  # Montgomery lanes, R = 2^h, raw products summed and reduced every `every` terms
    n, k, m = len(A), len(B), len(B[0]); Rm = 1 << h; M = math.prod(primes)
    res = {}
    for p in primes:
        t_ = pow(next(c for c in itertools.count(2) if pow(c, (p - 1) // 2, p) == p - 1), (p - 1) // 4, p)
        ninv = (-pow(p, -1, Rm)) % Rm
        def redc(T):
            assert T < p * Rm
            mm = ((T & (Rm - 1)) * ninv) & (Rm - 1); u = (T + mm * p) >> h; return u - p if u >= p else u
        split = lambda z: (((z[0] + z[1] * t_) % p), ((z[0] - z[1] * t_) % p))     # conversion in (functional)
        As = [[split(z) for z in row] for row in A]; Bs = [[split(z) for z in row] for row in B]
        for i in range(n):
            for j in range(m):
                uv = []
                for c in (0, 1):
                    acc, raw, cnt, blocks = 0, 0, 0, 0
                    for tt in range(k):
                        raw += As[i][tt][c] * Bs[tt][j][c]; cnt += 1
                        if cnt == every: acc = (acc + redc(raw)) % p; raw = cnt = 0; blocks += 1
                    if cnt: acc = (acc + redc(raw)) % p; blocks += 1
                    uv.append(acc * Rm % p)                 # undo R^-1 (folded into the conversion-out constants in a build)
                res[(p, i, j)] = uv
    out = [[None] * m for _ in range(n)]
    for i in range(n):
        for j in range(m):
            parts = []
            for c in (0, 1):
                r = 0; Mi = 1
                for p in primes:                            # CRT (functional; Garner in a build)
                    t_ = pow(next(cc for cc in itertools.count(2) if pow(cc, (p - 1) // 2, p) == p - 1), (p - 1) // 4, p)
                    u, v = res[(p, i, j)]
                    val = (u + v) * pow(2, -1, p) % p if c == 0 else (u - v) * pow(2 * t_, -1, p) % p
                    r = r + Mi * ((val - r) * pow(Mi, -1, p) % p); Mi *= p
                parts.append(r - M if r > M // 2 else r)
            out[i][j] = tuple(parts)
    return out

res_ = {}
def chk(k, ok):
    res_[k + ' cases'] = res_.get(k + ' cases', 0) + 1
    if not ok: res_[k + ' FAIL'] = res_.get(k + ' FAIL', 0) + 1
random.seed(18)
# exhaustive tiny cases with toy primes (lanes h = 8, R = 2^8): b = 1 (halves -1, 0), 2 x 2 times 2 x 2, every pair of
# matrices; b = 2 (halves -2 .. 1), 1 x 2 times 2 x 1, every pair. Toy primes 5 and 13 (M = 65 > 2^R for these R).
tiny = [(x, y) for x in (-1, 0) for y in (-1, 0)]
for A in itertools.product(tiny, repeat=4):
    Am = [list(A[0:2]), list(A[2:4])]
    for B in itertools.product(tiny, repeat=4):
        Bm = [list(B[0:2]), list(B[2:4])]; E = exact(Am, Bm)
        chk('tiny b = 1, 2 x 2 x 2: residue (toy primes 5, 13, h = 8) = exact', residue(Am, Bm, [5, 13], h=8) == E)
        chk('tiny b = 1, 2 x 2 x 2: direct bound holds', all(abs(v) <= 2 ** (need_bits(1, 2) - 1) for row in E for z in row for v in z))
el2 = [(x, y) for x in range(-2, 2) for y in range(-2, 2)]
for A in itertools.product(el2, repeat=2):
    for B in itertools.product(el2, repeat=2):
        Am, Bm = [list(A)], [[B[0]], [B[1]]]; E = exact(Am, Bm)
        chk('tiny b = 2, 1 x 2 x 1: residue (toy primes 5, 13, h = 8) = exact', residue(Am, Bm, [5, 13], h=8) == E)
# random at each b and n with the real choices: direct widths, 31-bit primes, R = 2^32
for b in (8, 16, 32, 64):
    lo, hi = -(1 << (b - 1)), (1 << (b - 1)) - 1
    pick = lambda: random.choice([random.randint(lo, hi), lo, hi])
    for n in (2, 3, 4, 8, 16):
        for rep in range(3 if n < 16 else 1):
            A = [[(pick(), pick()) for _ in range(n)] for _ in range(n)]; B = [[(pick(), pick()) for _ in range(n)] for _ in range(n)]
            if rep == 0: A[0][0] = B[0][0] = (lo, lo)                    # the extreme corner
            E = exact(A, B); mp = primes_for(need_bits(b, n))
            chk(f'random b = {b}: direct = exact', direct(A, B, b) == E)
            chk(f'random b = {b}: residue ({mp} primes) = exact', residue(A, B, PR31[:mp]) == E)
    # matrix-vector (k x 1)
    A = [[(pick(), pick()) for _ in range(8)] for _ in range(8)]; B = [[(pick(), pick())] for _ in range(8)]
    mp = primes_for(need_bits(b, 8)); E = exact(A, B)
    chk(f'matrix-vector b = {b}: direct and residue = exact', direct(A, B, b) == E and residue(A, B, PR31[:mp]) == E)
# the 27-bit-prime variant (REDC every 32 terms): functional check with k = 40 and 70 (several blocks) at b = 32 and 64
PR27 = [p for p in range((1 << 27) - 1, (1 << 27) - 20000, -1) if isprime(p) and p % 4 == 1][:8]
for b in (32, 64):
    lo, hi = -(1 << (b - 1)), (1 << (b - 1)) - 1
    for kk in (40, 70):
        for rep in range(2):
            A = [[(random.choice([lo, hi, random.randint(lo, hi)]), random.choice([lo, hi, random.randint(lo, hi)])) for _ in range(kk)] for _ in range(3)]
            B = [[(random.choice([lo, hi, random.randint(lo, hi)]), random.choice([lo, hi, random.randint(lo, hi)]))] for _ in range(kk)]
            mp = primes_for(need_bits(b, kk), 27)
            chk(f'27-bit primes, REDC every 32 terms, b = {b}: residue = exact', residue(A, B, PR27[:mp], every=32) == exact(A, B))
for k, v in sorted(res_.items()): print(f"  {k}: {v}")

# ---------------------------------------------------------------- the sweep: square n x n, k = m = n
print("\n  square n x n, total instructions (direct layout b / direct layout a / residue lazy / residue reduce-every-term):")
cross = {}
for b in (8, 16, 32, 64):
    row = []
    for n in range(2, 33):
        d, da = direct_cost(n, n, n, b), direct_cost(n, n, n, b, 'a')
        r, mp = residue_cost(n, n, n, b); r2, _ = residue_cost(n, n, n, b, lazy=False)
        row.append((n, d, da, r, r2, mp))
        for name, val in (('b', d), ('a', da)):
            if r < val and (b, name) not in cross: cross[(b, name)] = n
    print(f"  b = {b}: " + "; ".join(f"n={n}: {d:.0f}/{da:.0f}/{r:.0f}/{r2:.0f} (m={mp})" for n, d, da, r, r2, mp in row if n in (2, 4, 8, 16, 32)))
    # beyond 32, for the crossover
    for n in range(33, 4097):
        d, da = direct_cost(n, n, n, b), direct_cost(n, n, n, b, 'a'); r, mp = residue_cost(n, n, n, b)
        for name, val in (('b', d), ('a', da)):
            if r < val and (b, name) not in cross: cross[(b, name)] = n
print("  per-term costs: direct", T_D, " residue (lazy) per prime count:", {mp: T_R(mp) for mp in range(1, 7)})
print("  crossover n (residue cheaper from this n on; none = not up to 4096):")
for b in (8, 16, 32, 64):
    mp = primes_for(need_bits(b, 32))
    print(f"    b = {b}: primes {mp} (at n = 32); against layout b: {cross.get((b, 'b'), 'none')}, against layout a: {cross.get((b, 'a'), 'none')}")
# variant: 27-bit primes, REDC every 2^(32-27) = 32 terms
print("  variant, 27-bit primes (REDC every 32 terms):")
for b in (8, 16, 32, 64):
    c27 = None
    for n in range(2, 4097):
        r, mp = residue_cost(n, n, n, b, pbits=27)
        if r < direct_cost(n, n, n, b): c27 = (n, mp); break
    print(f"    b = {b}: crossover against layout b: {c27}")
# matrix-vector and powers
print("  matrix-vector (n x n times n x 1), residue minus direct, per n:")
for b in (32, 64):
    print("    b =", b, [(n, round(residue_cost(n, n, 1, b)[0] - direct_cost(n, n, 1, b))) for n in (8, 32, 128, 1024)])
