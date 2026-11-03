# Study 8: Gauss's three-multiply complex product against the four-multiply product.
#   k1 = c(a + b),  k2 = a(d - c),  k3 = b(c + d);   re = k1 - k3 = ac - bd,   im = k1 + k2 = ad + bc.
# Word: 2K bits, a (or c) in the low half, b (or d) in the high half. The arithmetic runs on extracted halves in 2K-bit
# words (as at K = 32 with 64-bit registers), wrapping mod 2^(2K) like the machine.
# (1) wrapping: halves are int-K (all values, MIN included); result halves wrap mod 2^K, as study 1's mul.
#     Also checked: for halves in the symmetric range the 2K-bit re and im are the exact true values (so study 2's
#     range test applies unchanged).
# (2) mod n (residues in [0, n), n < 2^(K-1)), Barrett as study 4. Every intermediate is kept non-negative:
#     a + b < 2n, (d + n) - c in (0, 2n), c + d < 2n, so k1, k2, k3 < 2n^2;
#     re_t = (k1 + 2n^2) - k3 in (0, 4n^2),  im_t = k1 + k2 = ad + bc + an = ad + bc (mod n), < 4n^2 < 2^(2K).
# Usage: python3 gauss3.py K   (K <= 5 exhaustive wrapping; mod n exhaustive for small n; K = 32 random)
import sys, random, itertools
K = int(sys.argv[1]) if len(sys.argv) > 1 else 4
W = 2*K; MW = (1 << W) - 1; ML = (1 << K) - 1
MIN = -(1 << (K-1)); HI = (1 << (K-1)) - 1
def s(x, k): x &= (1 << k) - 1; return x - (1 << k) if x >> (k-1) else x
def pack(a, b): return ((b & ML) << K) | (a & ML)
def unpack(w): return s(w, K), s(w >> K, K)
w2 = lambda v: s(v, W)                                  # a 2K-bit signed machine word

# ---- (1) wrapping
def mul4(x, y):                 # study 1: extract 6, 4 MUL, SUB, ADD, pack 3 = 15; 4 multiplies; chain 2 + 1 + 1 + 2 = 6
    a, b = unpack(x); c, d = unpack(y)
    re, im = w2(w2(a*c) - w2(b*d)), w2(w2(a*d) + w2(b*c))
    return pack(re, im), (re, im)
def mul3(x, y):                 # extract 6, 3 add/sub, 3 MUL, SUB, ADD, pack 3 = 17; 3 multiplies; chain 2 + 1 + 1 + 1 + 2 = 7
    a, b = unpack(x); c, d = unpack(y)
    k1 = w2(c * w2(a + b)); k2 = w2(a * w2(d - c)); k3 = w2(b * w2(c + d))
    re, im = w2(k1 - k3), w2(k1 + k2)
    return pack(re, im), (re, im)

# ---- (2) mod n, Barrett (study 4: 7 instructions, 2 multiplies, chain 7)
def barrett(t, n, m):
    q = (t * m) >> W; r = (t - q * n) & MW; r = (r - n) & MW
    return (r + (n & (MW if r >> (W-1) else 0))) & MW
def lanes(w): return w & ML, (w >> K) & ML
def mulmod4(x, y, n, m):        # study 4: extract 4, 4 MUL, 3 add/sub, 2 x Barrett 14, pack 2 = 27; 8 multiplies; chain 13
    (a, b), (c, d) = lanes(x), lanes(y)
    return pack(barrett(a*c + (n*n - b*d), n, m), barrett(a*d + b*c, n, m))
def mulmod3(x, y, n, m):        # extract 4, a+b 1, (d+n)-c 2, c+d 1, 3 MUL, re 2, im 1, 2 x Barrett 14, pack 2 = 30; 7 multiplies
    (a, b), (c, d) = lanes(x), lanes(y)     # chain: extract 1, (d+n)-c 2, MUL 1, (k1+2n^2)-k3 2... (k1 + 2n^2 runs beside k3) 1,
    k1 = c * (a + b); k2 = a * ((d + n) - c); k3 = b * (c + d)   #   Barrett 7, pack 2 = 1 + 2 + 1 + 1 + 7 + 2 = 14
    return pack(barrett((k1 + 2*n*n) - k3, n, m), barrett(k1 + k2, n, m))

st = {}
def chk(k, ok):
    st[k + ' cases'] = st.get(k + ' cases', 0) + 1
    if not ok: st[k + ' FAIL'] = st.get(k + ' FAIL', 0) + 1
def test_wrap(x, y):
    a, b = unpack(x); c, d = unpack(y)
    w3, (re3, im3) = mul3(x, y); w4, (re4, im4) = mul4(x, y)
    chk('wrap: mul3 = mul4 (halves mod 2^K)', w3 == w4)
    chk('wrap: mul3 = true value mod 2^K', unpack(w3) == (s(a*c - b*d, K), s(a*d + b*c, K)))
    if MIN not in (a, b, c, d):
        chk('symmetric range: 2K-bit re, im exact', (re3, im3) == (a*c - b*d, a*d + b*c))
    else:
        if (re3, im3) != (a*c - b*d, a*d + b*c): st['MIN input: 2K-bit re or im not exact (wrapped)'] = st.get('MIN input: 2K-bit re or im not exact (wrapped)', 0) + 1
def test_mod(n, a, b, c, d):
    m = (1 << W) // n; x, y = pack(a, b), pack(c, d); want = ((a*c - b*d) % n, (a*d + b*c) % n)
    chk('mod n: mulmod3 correct', lanes(mulmod3(x, y, n, m)) == want)
    chk('mod n: mulmod4 correct', lanes(mulmod4(x, y, n, m)) == want)

if K <= 5:
    for x in range(1 << W):
        for y in range(1 << W): test_wrap(x, y)
    for n in range(2, 1 << (K-1)):
        for a, b, c, d in itertools.product(range(n), repeat=4): test_mod(n, a, b, c, d)
    print(f"K={K}: wrapping exhaustive over {1 << (2*W)} word pairs; mod n exhaustive for n = 2 .. {(1 << (K-1)) - 1}")
else:
    random.seed(8)
    edge = [0, 1, -1, HI, -HI, MIN, HI - 1, 1 << 16, -(1 << 16)]
    for i in range(200000):
        if i % 4 == 0: x, y = pack(random.choice(edge), random.choice(edge)), pack(random.choice(edge), random.choice(edge))
        else: x, y = random.getrandbits(W), random.getrandbits(W)
        test_wrap(x, y)
    ns = [2147483647, 2147483629, 2147483587, 2147483579, 2147483646, 2147483645, 65537, 3]
    for n in ns:
        e = [0, 1, n - 1, n - 2, n // 2]
        for i in range(25000):
            if i % 5 == 0: test_mod(n, *(random.choice(e) for _ in range(4)))
            else: test_mod(n, *(random.randrange(n) for _ in range(4)))
    print(f"K={K}: wrapping 200000 random pairs; mod n 25000 random quadruples for n in {ns}")
for k, v in sorted(st.items()): print(f"  {k}: {v}")
