# Study 3: a stack of words.
# A number is a stack of L words, lowest first. Word j holds digit j of the real part in its low half and digit j of the
# imaginary part in its high half. Lower words' halves are unsigned K-bit digits; the top word's halves are signed:
#   re = sum_{j<L-1} re_j 2^(Kj) + s(re_{L-1}) 2^(K(L-1)), the same for im (two's complement over KL bits).
# Halves carry into the same half of the next word (real into real, imaginary into imaginary), computed by word operations
# (no carry flag). add and sub wrap mod 2^(KL) per part and return a per-half overflow word for the top (study 2's test);
# mul is the schoolbook product truncated to L words (wraps mod 2^(KL) per part; overflow is not detected in this study).
# Usage: python3 stack.py K L [pairs]   (pairs omitted: exhaustive over all stack pairs; else that many random pairs)
import sys, random
K = int(sys.argv[1]); L = int(sys.argv[2]); NPAIRS = int(sys.argv[3]) if len(sys.argv) > 3 else 0
W = 2*K; MW = (1 << W) - 1; ML = (1 << K) - 1
H = (1 << (K-1)) | (1 << (W-1)); NH = ~H & MW; LO = (1 << K) | 1      # LO: a 1 at the bottom of each half
KL = K*L; MKL = (1 << KL) - 1
def sg(v, k): v &= (1 << k) - 1; return v - (1 << k) if v >> (k-1) else v

# --- add with per-half carries. Per word: packed add with carry-in c (c holds 0 or 1 at bit 0 and bit K):
#  (x AND NH) + (y AND NH) + c stays below 2^K in each half's low K-1 bits plus the top position, so nothing crosses halves.
#  The carry out of a half is the majority of x, y and the carry into the top bit: (x AND y) OR ((x OR y) AND NOT r), top bit.
def add(X, Y, cin=0):
    c = cin; R = []
    for j in range(L):
        x, y = X[j], Y[j]
        r = (((x & NH) + (y & NH) + c) ^ ((x ^ y) & H)) & MW           # AND, AND, ADD, ADD c, AND, XOR, XOR = 7 (6 when c = 0)
        if j < L - 1:                                                   # (unrolled at fixed L: not a data branch)
            c = (((x & y) | ((x | y) & ~r)) & H) >> (K-1)               # AND, OR, NOT, AND, OR, AND, SHR = 7 (6 with andn)
        R.append(r)
    o = (X[-1] ^ r) & (Y[-1] ^ r) & H                                    # top-word signed overflow per half: 4
    return R, o
# --- sub = x + NOT y + 1 in each half: the same add, carry-in LO. Shares add's code (RULES: prefer shared solutions).
def sub(X, Y):
    return add(X, [~y & MW for y in Y], LO)                             # NOT per word, then add with carry-in in word 0
# add:  word 0: 6 + 7, middle words 7 + 7, top word 7, overflow 4  ->  14L - 3       (andn: 13L - 2)
# sub:  L NOTs, word 0: 7 + 7, middle 7 + 7, top 7, overflow 4  ->  15L - 3          (andn: 14L - 2)

# --- multiply: re = ac - bd, im = ad + bc, each product truncated to L digits (mod 2^(KL): the same for signed and unsigned,
# so the top digit is used as unsigned). Digits are extracted from the halves (AND, SHR per word).
def mulpart(A, C):                     # schoolbook, row by row: t = r + a_i c_j + carry < 2^(2K), digit = t AND ML, carry = t SHR K
    r = [0]*L
    for i in range(L):
        carry = 0
        for j in range(L - i):
            t = r[i+j] + A[i]*C[j] + carry                              # MUL, ADD, ADD
            r[i+j] = t & ML; carry = t >> K                             # AND, SHR          5 per digit product
    return r
def digit_addsub(P, Q, sign):         # digit-wise P + Q or P - Q with carry: t = p +- q + c, digit = t AND ML, c = t SAR K (c is 0, 1 or -1)
    out = []; c = 0
    for j in range(L):
        t = P[j] + sign*Q[j] + c                                        # 2 (1 in digit 0)
        out.append(t & ML); c = t >> K                                  # AND, SAR (Python >> on ints is sar)    4 per digit
    return out
def mul(X, Y):
    A = [x & ML for x in X]; B = [x >> K for x in X]                    # 2 per word
    C = [y & ML for y in Y]; D = [y >> K for y in Y]                    # 2 per word
    re = digit_addsub(mulpart(A, C), mulpart(B, D), -1)
    im = digit_addsub(mulpart(A, D), mulpart(B, C), +1)
    return [re[j] | (im[j] << K) for j in range(L)]                     # SHL, OR per word
# mul: extract 4L; 4 truncated products of L(L+1)/2 digit products at 5 each = 10 L(L+1); combine 2 (4L - 1); pack 2L
#      total 10 L^2 + 24 L - 2   (L = 1: 32, L = 2: 86, L = 3: 160)

def val(X, half):                      # the true part (half 0 = real, 1 = imaginary)
    v = 0
    for j in range(L):
        d = (X[j] >> (K*half)) & ML
        v += (sg(d, K) if j == L - 1 else d) << (K*j)
    return v
def stack_of(re, im):
    return [(((re >> (K*j)) & ML) | (((im >> (K*j)) & ML) << K)) for j in range(L)]
lo, hi = -(1 << (KL-1)), (1 << (KL-1)) - 1

def check(pairs):
    st = {}
    def bump(k): st[k] = st.get(k, 0) + 1
    for X, Y in pairs:
        a, b, c, d = val(X, 0), val(X, 1), val(Y, 0), val(Y, 1)
        for name, (R, o), want in (('add', add(X, Y), (a+c, b+d)), ('sub', sub(X, Y), (a-c, b-d))):
            bump(name + ' cases')
            if (val(R, 0), val(R, 1)) != (sg(want[0], KL), sg(want[1], KL)): bump(name + ' FAIL value mod 2^(KL)')
            if (o != 0) != (not (lo <= want[0] <= hi and lo <= want[1] <= hi)): bump(name + ' FAIL overflow flag')
            if all(lo <= w <= hi for w in want): bump(name + ' in range')
        R = mul(X, Y); want = (a*c - b*d, a*d + b*c)
        bump('mul cases')
        if (val(R, 0), val(R, 1)) != (sg(want[0], KL), sg(want[1], KL)): bump('mul FAIL value mod 2^(KL)')
        if all(lo <= w <= hi for w in want): bump('mul in range (exact)')
    return st

if NPAIRS == 0:
    stacks = [stack_of(re, im) for re in range(1 << KL) for im in range(1 << KL)]
    pairs = ((X, Y) for X in stacks for Y in stacks); n = len(stacks)**2
else:
    random.seed(3)
    def rnd():
        r = random.random()
        if r < 0.3:   # digits from edge values
            e = [0, 1, ML, ML - 1, 1 << (K-1), (1 << (K-1)) - 1]
            return [random.choice(e) | (random.choice(e) << K) for _ in range(L)]
        if r < 0.5:   # small values, positive or negative
            return stack_of(random.randint(-(1 << K), 1 << K), random.randint(-(1 << K), 1 << K))
        return [random.getrandbits(W) for _ in range(L)]
    pairs = [(rnd(), rnd()) for _ in range(NPAIRS)]; n = NPAIRS
print(f"K={K} L={L}: {n} pairs ({'exhaustive' if NPAIRS == 0 else 'random'})")
for k, v in sorted(check(pairs).items()): print(f"  {k}: {v}")
