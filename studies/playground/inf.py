# Study 1: the infinity marker.
# A Gaussian integer a + bi packed in one word of 2K bits: a in the low half, b in the high half, each a signed K-bit lane.
# MIN = -2^(K-1) is never a value. A number is the single point at infinity when either half is MIN.
# Finite halves lie in -(2^(K-1)-1) .. 2^(K-1)-1 (the symmetric range).
# Operations: add, sub, neg, mul, conj, mul_i. Any infinite input gives infinity. Overflow is NOT handled here:
# a finite result outside the symmetric range wraps (mod 2^K per half), and is reported, not corrected.
# Plain word operations only; no branches inside the operations (the Python ifs are in the checker only).
# Usage: python3 inf.py K   (K <= 5 exhaustive over all word pairs, else 200000 random pairs)
import sys, random
K = int(sys.argv[1]) if len(sys.argv) > 1 else 4
W = 2*K; MW = (1 << W) - 1; ML = (1 << K) - 1
H = (1 << (K-1)) | (1 << (W-1))          # top bit of each half; also the canonical infinity pack(MIN, MIN)
NH = ~H & MW                             # all bits except each half's top bit
MIN = -(1 << (K-1)); HI = (1 << (K-1)) - 1
def s(x, k=K): x &= (1 << k) - 1; return x - (1 << k) if x >> (k-1) else x
def pack(a, b): return ((b & ML) << K) | (a & ML)
def unpack(w): return s(w & ML), s(w >> K)
INF = H

# --- per-half infinity detector: the top bit of each half that equals MIN, all other bits 0.
#  y = (x AND NH) + NH: the top bit of each half of y is 1 exactly when the half's low K-1 bits are not all 0
#  (each half's sum is below 2^K, so no carry crosses into the next half).
#  A half is MIN when its top bit is 1 and its low bits are 0: x AND NOT y AND H.
def inf_bits(x):
    y = ((x & NH) + NH) & MW                  # 2: AND, ADD
    return x & (~y & MW) & H                  # 3: NOT, AND, AND   (2 with andn/bic)        total 5 (4)
# --- force each marked half to MIN: t holds top bits of the halves to force.
#  m = t - (t shr (K-1)) is the low K-1 bits of each marked half (each half's subtraction is 2^(K-1) - 1, no borrow out).
def force(r, t):
    m = (t - (t >> (K-1))) & MW               # 2: SHR, SUB
    return (r & (~m & MW)) | t                # 3: NOT, AND, OR   (2 with andn/bic)         total 5 (4)

# --- the plain operations (toy.py): wrapping per half
def add_raw(x, y): return (((x & NH) + (y & NH)) ^ ((x ^ y) & H)) & MW            # 5 (HD 2-18 packed add)
def sub_raw(x, y): return (((x | H) - (y & NH)) ^ ((x ^ ~y) & H)) & MW            # 5 (HD 2-18 packed subtract; x^~y is XOR+NOT, or 1 xnor/eon)
# negate both halves: 0 - x packed: (H - (x AND NH)) XOR (NOT x AND H)
def neg(x): return ((H - (x & NH)) ^ (~x & H)) & MW                               # 5: AND, SUB, NOT, AND, XOR  (4 with andn)
def rot(x): return ((x >> K) | (x << K)) & MW                                      # 1 rotate (3 as SHR, SHL, OR)
def neg_low(x): return (x & (ML << K)) | ((-(x & ML)) & ML)                       # AND, AND, NEG(2), AND, OR = 6
def mul_i(x): return neg_low(rot(x))                                               # 7
def conj(x): return ((-(x >> K) & ML) << K) | (x & ML)                             # SHR, NEG(2), AND, SHL, AND, OR = 7

# --- operations with the infinity rule
def add(x, y): return force(add_raw(x, y), inf_bits(x) | inf_bits(y))   # 5 + (5+5+1) + 5 = 21  (andn: 5+4+4+1+4 = 18)
def sub(x, y): return force(sub_raw(x, y), inf_bits(x) | inf_bits(y))   # same, 21 (18)
# Unary operations need no check: -MIN = MIN mod 2^K, so negating, swapping or keeping a half leaves MIN as MIN.
# (The result is an infinity, though not always the canonical word H: neg(pack(MIN,3)) = pack(MIN,-3).)
# mul: extract the four signed halves, four products, pack; infinity by four compares on the extracted halves.
def mul(x, y):
    a = s(x & ML)                                       # SHL, SAR = 2 (sign-extend the low half)
    b = s(x >> K)                                       # SAR = 1
    c = s(y & ML); d = s(y >> K)                        # 2 + 1
    r = pack(a*c - b*d, a*d + b*c)                      # 4 MUL, SUB, ADD; pack = AND, SHL, OR = 3
    k = (a == MIN) | (b == MIN) | (c == MIN) | (d == MIN)   # 4 compares, 3 OR
    m = (0 - k) & MW                                    # 1
    return r ^ ((r ^ INF) & m)                          # conditional choice 3
# mul total: extract 6, products 4, add/sub 2, pack 3 = 15; infinity 4 + 3 + 1 + 3 = 11; total 26.

def isinf(w): a, b = unpack(w); return a == MIN or b == MIN
inrange = lambda v: -HI <= v <= HI

def check(samples):
    stats = {}
    def bump(k): stats[k] = stats.get(k, 0) + 1
    for x, y in samples:
        a, b = unpack(x); c, d = unpack(y)
        xi, yi = isinf(x), isinf(y)
        bin_ops = {'add': (add(x, y), (a+c, b+d)), 'sub': (sub(x, y), (a-c, b-d)), 'mul': (mul(x, y), (a*c-b*d, a*d+b*c))}
        un_ops = {'neg': (neg(x), (-a, -b)), 'conj': (conj(x), (a, -b)), 'mul_i': (mul_i(x), (-b, a))}
        for name, (got, want) in bin_ops.items():
            bump(name + ' cases')
            if xi or yi:
                if not isinf(got): bump(name + ' FAIL inf in, finite out')
                # undefined points of the Riemann sphere: the toy returns infinity for them too
                if xi and yi and name in ('add', 'sub'): bump(name + ' inf op inf (undefined; toy gives inf)')
                if name == 'mul' and ((xi and (c, d) == (0, 0)) or (yi and (a, b) == (0, 0))): bump('mul 0*inf (undefined; toy gives inf)')
            elif inrange(want[0]) and inrange(want[1]):
                if unpack(got) != want: bump(name + ' FAIL finite in range')
            else:
                bump(name + ' overflow (not handled)')
                if isinf(got): bump(name + ' overflow -> reads as infinity (spurious)')
                else: bump(name + ' overflow -> wrong finite value (wrapped)')
        for name, (got, want) in un_ops.items():
            if K <= 5 and y != 0: continue                   # unary: exhaustive runs test each x once (the y = 0 row)
            if xi:
                if not isinf(got): bump(name + ' FAIL inf in, finite out')
                bump(name + ' inf cases')
            else:
                if not (inrange(want[0]) and inrange(want[1])): bump(name + ' FAIL result out of range')
                if unpack(got) != want: bump(name + ' FAIL finite')
                bump(name + ' finite cases')
    return stats

if K <= 5: samples = [(x, y) for x in range(1 << W) for y in range(1 << W)]
else:
    random.seed(1)
    def rnd():   # mostly finite, some infinities, some near the range ends
        r = random.random()
        if r < 0.05: return pack(MIN, random.getrandbits(K))
        if r < 0.10: return pack(random.getrandbits(K), MIN)
        if r < 0.20: return pack(random.choice([HI, -HI, 0, 1, -1]), random.choice([HI, -HI, 0, 1, -1]))
        if r < 0.50: return pack(random.randint(-(1 << (K//2)), 1 << (K//2)), random.randint(-(1 << (K//2)), 1 << (K//2)))
        return random.getrandbits(W)
    samples = [(rnd(), rnd()) for _ in range(200000)]
print(f"K={K}: {len(samples)} pairs")
for k, v in sorted(check(samples).items()): print(f"  {k}: {v}")
