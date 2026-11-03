# Study 4: multiply mod n, Barrett reduction against Montgomery reduction.
# Lanes hold residues a, b in [0, n), n < 2^(K-1), packed in a word of W = 2K bits: a in the low half, b in the high half.
# Gaussian product mod n: re = ac - bd, im = ad + bc. Each part is formed as a non-negative word below 2 n^2 < 2^(W-1):
#   re_t = ac + (n^2 - bd)   (n^2 a constant),   im_t = ad + bc,
# then reduced mod n by one of:
#  Barrett:    m = floor(2^W / n) (precomputed). q = mulhi(t, m), r = t - q n in [0, 2n), then subtract n where r >= n.
#  Montgomery: n odd, n' = n^-1 mod 2^W (precomputed), R = 2^W. m = t n' mod 2^W (the low word of one multiply),
#              u = mulhi(m, n). t - m n is 0 mod 2^W and t < 2^W, so (t - m n) / 2^W = -u, in (-n, 0]: then add n where negative.
#              This gives t R^-1 mod n, so values are kept in Montgomery form (x R mod n); see the conversion counts.
# Both use the high word of a W x W product (mulhi: x86-64 mul, ARM64 umulh, RISC-V mulhu) and the low word (mul).
# Usage: python3 reduce.py K   (K <= 8: exhaustive; K = 32: random)
import sys, random, itertools
K = int(sys.argv[1]) if len(sys.argv) > 1 else 4
W = 2*K; MW = (1 << W) - 1; ML = (1 << K) - 1
def pack(a, b): return ((b & ML) << K) | (a & ML)
def lanes(w): return w & ML, (w >> K) & ML
mulhi = lambda x, y: (x * y) >> W
mullo = lambda x, y: (x * y) & MW
sar_mask = lambda v: MW if (v >> (W-1)) & 1 else 0      # v sar (W-1): all ones where the top bit is set (one shift)

def barrett(t, n, m):                  # 7 instructions, 2 multiplies, dependent chain 7
    q = mulhi(t, m)                     # MULHI
    r = (t - mullo(q, n)) & MW          # MUL, SUB
    r = (r - n) & MW                    # SUB
    return (r + (n & sar_mask(r))) & MW # SAR, AND, ADD
def montgomery(t, n, ninv):            # 6 instructions, 2 multiplies, dependent chain 6
    mm = mullo(t, ninv)                 # MUL
    r = (0 - mulhi(mm, n)) & MW         # MULHI, NEG (SUB from 0)
    return (r + (n & sar_mask(r))) & MW # SAR, AND, ADD

# Gaussian product mod n. extract 4 (AND, SHR per word), 4 MUL, re: n^2 - bd and + ac (2), im: ADD (1), 2 reductions, pack SHL, OR (2).
#   total 13 + 2 x reduction: Barrett 27 (8 multiplies), Montgomery 25 (8 multiplies).
#   longest chain: extract 1, MUL 1, SUB/ADD 2 (n^2 - bd and ac in parallel, then the add), reduction, pack 2:
#   Barrett 1 + 1 + 2 + 7 + 2 = 13, Montgomery 1 + 1 + 2 + 6 + 2 = 12.
def gmul(x, y, n, red, const):
    a, b = lanes(x); c, d = lanes(y)
    re_t = a*c + (n*n - b*d); im_t = a*d + b*c
    return pack(red(re_t, n, const), red(im_t, n, const))

def isprime(p):
    if p < 2: return False
    for q in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        if p % q == 0: return p == q
    d, s = p - 1, 0
    while d % 2 == 0: d //= 2; s += 1
    for a in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        x = pow(a, d, p)
        if x in (1, p - 1): continue
        for _ in range(s - 1):
            x = x * x % p
            if x == p - 1: break
        else: return False
    return True

random.seed(4); st = {}
def bump(k): st[k] = st.get(k, 0) + 1
if K <= 8:
    ns = list(range(2, 1 << (K-1)))
else:
    top = (1 << (K-1)) - 1
    primes = [p for p in range(top, top - 400, -1) if isprime(p)][:4]
    ns = primes + [65537, 1000003, top - 1, top - 2, 3 * 5 * 7 * 11 * 13 * 17 * 19 * 23]  # composites: top - 1 (even), top - 2, 3*5*...*23 (odd)
    print("n:", [(n, 'prime' if isprime(n) else 'composite') for n in ns])
for n in ns:
    m = (1 << W) // n
    odd = n % 2 == 1
    ninv = pow(n, -1, 1 << W) if odd else None
    R = (1 << W) % n
    Rinv = pow(1 << W, -1, n) if odd else None
    # (1) the reductions alone, over every t < 2 n^2 (exhaustive at small K) or random t at K = 32
    ts = range(2*n*n) if K <= 8 else [random.randrange(2*n*n) for _ in range(20000)] + [0, 1, n, n*n, 2*n*n - 1]
    for t in ts:
        bump('barrett reduce cases')
        if barrett(t, n, m) != t % n: bump('barrett reduce FAIL')
        if odd:
            bump('montgomery reduce cases')
            if montgomery(t, n, ninv) != t * Rinv % n:
                bump('montgomery reduce FAIL')
    # (2) the Gaussian product mod n: all lane values for every n at K <= 5; at K = 6..8 for the three largest n <= 63
    # (n^4 lane tuples each); random at K = 32
    if K <= 5 or (K <= 8 and n in [v for v in ns if v <= 63][-3:]):
        quads = itertools.product(range(n), repeat=4)
    elif K <= 8: quads = []
    else: quads = [tuple(random.randrange(n) for _ in range(4)) for _ in range(20000)] + [(n-1,)*4, (0, n-1, n-1, 0), (n-1, 0, 0, n-1)]
    for a, b, c, d in quads:
        x, y = pack(a, b), pack(c, d); want = ((a*c - b*d) % n, (a*d + b*c) % n)
        bump('gaussian barrett cases')
        if lanes(gmul(x, y, n, barrett, m)) != want: bump('gaussian barrett FAIL')
        if odd:
            # Montgomery form: lanes hold aR mod n etc.; the product then holds (ac - bd) R mod n, (ad + bc) R mod n
            xm, ym = pack(a*R % n, b*R % n), pack(c*R % n, d*R % n)
            bump('gaussian montgomery cases')
            if lanes(gmul(xm, ym, n, montgomery, ninv)) != (want[0]*R % n, want[1]*R % n): bump('gaussian montgomery FAIL')
            # plain lanes through Montgomery give the product times R^-1: check too (one fix-up multiply recovers it)
            if lanes(gmul(x, y, n, montgomery, ninv)) != (want[0]*Rinv % n, want[1]*Rinv % n): bump('gaussian montgomery (plain lanes) FAIL')
print(f"K={K}: {len(ns)} moduli")
for k, v in sorted(st.items()): print(f"  {k}: {v}")
