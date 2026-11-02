# Study 2: the overflow marker.
# A Gaussian integer a + bi packed in one word of 2K bits: a in the low half, b in the high half, signed K-bit lanes.
# MIN = -2^(K-1) is the infinity marker (a word is infinity when either half is MIN); finite halves are -(2^(K-1)-1) .. 2^(K-1)-1.
# add, sub, mul wrap per half; when the true value of either half leaves the symmetric range, the result becomes
# infinity, by a mask and a conditional choice (no branch). Infinite inputs give infinity.
# Two ways to mark, both checked: "force" sets each offending half to MIN (stays inside the halves, result not canonical);
# "canon" replaces the whole word by pack(MIN, MIN) (needs one compare of the whole word).
# Plain word operations only; the Python ifs are in the checker only.
# Usage: python3 ovf.py K   (K <= 5 exhaustive over all word pairs, else 200000 random pairs)
import sys, random
K = int(sys.argv[1]) if len(sys.argv) > 1 else 4
W = 2*K; MW = (1 << W) - 1; ML = (1 << K) - 1
H = (1 << (K-1)) | (1 << (W-1)); NH = ~H & MW
MIN = -(1 << (K-1)); HI = (1 << (K-1)) - 1
def s(x, k=K): x &= (1 << k) - 1; return x - (1 << k) if x >> (k-1) else x
def pack(a, b): return ((b & ML) << K) | (a & ML)
def unpack(w): return s(w & ML), s(w >> K)
INF = H

def inf_bits(x):                                   # top bit of each half equal to MIN (study 1): 5 (4 with andn)
    y = ((x & NH) + NH) & MW
    return x & (~y & MW) & H
def force(r, t):                                   # each half whose top bit is set in t becomes MIN: 5 (4 with andn)
    m = (t - (t >> (K-1))) & MW
    return (r & (~m & MW)) | t
def canon(r, t):                                   # whole word becomes INF when t != 0: compare 1, mask 1, choice 3 = 5
    k = int(t != 0); m = (0 - k) & MW
    return r ^ ((r ^ INF) & m)

# --- add. Packed wrapping add (HD 2-18), 5. Signed overflow of a half: x, y same sign, r different: (x^r) AND (y^r) AND H, 4.
# A half that wraps without signed overflow is never out of the symmetric range except when it equals MIN,
# and then it is already the marker. So the halves to mark are: overflow OR inf(x) OR inf(y).
def add(x, y, mark=force):
    r = (((x & NH) + (y & NH)) ^ ((x ^ y) & H)) & MW                 # 5
    o = (x ^ r) & (y ^ r) & H                                         # 4
    t = o | inf_bits(x) | inf_bits(y)                                 # 2 OR + 5 + 5
    if mark is canon: t |= inf_bits(r)                                # canonical only: a half that wrapped to exactly MIN (true value MIN, no
                                                                      # signed overflow) already reads as infinity but is not the word INF: + 5 + OR 1
    return mark(r, t)                                                 # 5.  force total 26 (andn 23); canon total 32 (andn 29)
# --- sub. Packed wrapping subtract (HD 2-18), 5 (x^~y: XOR+NOT). Overflow: x, y different sign, r different from x: (x^y) AND (x^r) AND H, 4.
def sub(x, y, mark=force):
    r = (((x | H) - (y & NH)) ^ ((x ^ ~y) & H)) & MW                 # 5
    o = (x ^ y) & (x ^ r) & H                                         # 4
    t = o | inf_bits(x) | inf_bits(y)
    if mark is canon: t |= inf_bits(r)
    return mark(r, t)                                                 # force total 26 (23), canon 32 (29)
# --- mul. Extract signed halves, 4 products in the 2K-bit word (|ac - bd| <= 2 HI^2 < 2^(2K-1): exact for finite inputs).
# A product half v is in range when (v + HI) read unsigned is <= 2 HI: ADD, compare = 2 per half.
def mul(x, y):
    a = s(x & ML); b = s(x >> K); c = s(y & ML); d = s(y >> K)       # 6 (SHL+SAR, SAR, twice)
    re = a*c - b*d; im = a*d + b*c                                    # 4 MUL, SUB, ADD
    r = pack(re, im)                                                  # AND, SHL, OR = 3
    k = int(((re + HI) & MW) > 2*HI) | int(((im + HI) & MW) > 2*HI)  # 2 + 2 + OR 1
    k |= (a == MIN) | (b == MIN) | (c == MIN) | (d == MIN)           # 4 compares, 4 OR
    m = (0 - k) & MW                                                  # 1
    return r ^ ((r ^ INF) & m)                                        # 3       total 15 + 5 + 8 + 1 + 3 = 32

def isinf(w): a, b = unpack(w); return a == MIN or b == MIN
inrange = lambda v: -HI <= v <= HI

def check(samples):
    st = {}
    def bump(k): st[k] = st.get(k, 0) + 1
    for x, y in samples:
        a, b = unpack(x); c, d = unpack(y)
        anyinf = isinf(x) or isinf(y)
        ops = (('add force', add(x, y), (a+c, b+d)), ('add canon', add(x, y, canon), (a+c, b+d)),
               ('sub force', sub(x, y), (a-c, b-d)), ('sub canon', sub(x, y, canon), (a-c, b-d)),
               ('mul canon', mul(x, y), (a*c-b*d, a*d+b*c)))
        for name, got, want in ops:
            canonical = name.endswith('canon')
            if anyinf: case = 'inf input'
            elif inrange(want[0]) and inrange(want[1]): case = 'in range'
            else: case = 'out of range'
            bump(f'{name}: {case}')
            if case == 'in range': ok = unpack(got) == want
            else: ok = (got == INF) if canonical else isinf(got)
            if not ok: bump(f'{name}: FAIL {case}')
    return st

if K <= 5: samples = [(x, y) for x in range(1 << W) for y in range(1 << W)]
else:
    random.seed(2)
    edge = [HI, -HI, HI-1, -HI+1, 0, 1, -1, 1 << (K//2), -(1 << (K//2)), (1 << (K//2)) - 1]
    def rnd():
        r = random.random()
        if r < 0.05: return pack(MIN, random.getrandbits(K))
        if r < 0.10: return pack(random.getrandbits(K), MIN)
        if r < 0.30: return pack(random.choice(edge), random.choice(edge))
        if r < 0.60: return pack(random.randint(-(1 << (K//2)), 1 << (K//2)), random.randint(-(1 << (K//2)), 1 << (K//2)))
        return random.getrandbits(W)
    samples = [(rnd(), rnd()) for _ in range(200000)]
print(f"K={K}: {len(samples)} pairs")
for k, v in sorted(check(samples).items()): print(f"  {k}: {v}")
