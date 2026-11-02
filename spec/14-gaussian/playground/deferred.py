# Study 7: the deferred check, one shared path for overflow and infinity.
# Packed word of 2K bits, a in the low half, b in the high half, signed K-bit halves; MIN = -2^(K-1) marks infinity.
# Each operation does its plain arithmetic and ORs one per-half test of its RESULT into a running word (sticky).
# Inputs are not tested per operation. The external inputs of a run are tested once; every later operand is the
# result of an earlier operation, which was tested. Once a sticky word is nonzero its element is infinity, so its
# value no longer matters (it may be garbage). At the end of the run (or the array), one check turns every
# flagged element into the canonical infinity pack(MIN, MIN).
# The per-operation test catches exactly "a result half outside the symmetric range", given operands in range:
#   add/sub: signed overflow of the half (4) OR the result half equals MIN (5): a wrap to exactly MIN has no signed overflow.
#   mul: the 2K-bit range test of study 2 on each part (catches MIN too).
# Two variants are checked:
#   V1 per-element sticky: a sticky word per element, held in a register through the element's run; one canonical choice
#      per element at the end.
#   V2 one sticky word for the whole array: each element's tests are ORed into one word. Once per array: if it is 0 the
#      array is exact; if not, the array is recomputed with study 2's per-operation marking (a single branch per array).
# Correctness is checked against study 2's semantics: each operation gives infinity when an input is infinity or a true
# result half leaves the symmetric range; the reference keeps infinity as None.
# Usage: python3 deferred.py K   (K = 3, 4: exhaustive runs as described in studies.md; K = 32: random)
import sys, random, itertools
K = int(sys.argv[1]) if len(sys.argv) > 1 else 3
W = 2*K; MW = (1 << W) - 1; ML = (1 << K) - 1
H = (1 << (K-1)) | (1 << (W-1)); NH = ~H & MW
MIN = -(1 << (K-1)); HI = (1 << (K-1)) - 1
def s(x, k=K): x &= (1 << k) - 1; return x - (1 << k) if x >> (k-1) else x
def pack(a, b): return ((b & ML) << K) | (a & ML)
def unpack(w): return s(w & ML), s(w >> K)
INF = H

def inf_bits(x):                                    # study 1: top bit of each half equal to MIN. 5 (4 with andn)
    y = ((x & NH) + NH) & MW
    return x & (~y & MW) & H
# --- plain operations with their deferred test. Each returns (result, test word); the caller ORs the test into sticky.
def add(x, y):
    r = (((x & NH) + (y & NH)) ^ ((x ^ y) & H)) & MW            # 5
    return r, ((x ^ r) & (y ^ r) & H) | inf_bits(r)             # overflow 4, MIN result 5, OR 1          -> 15, + sticky OR 1 = 16
def sub(x, y):
    r = (((x | H) - (y & NH)) ^ ((x ^ ~y) & H)) & MW            # 5
    return r, ((x ^ y) & (x ^ r) & H) | inf_bits(r)             # 16 with the sticky OR (15 with andn)
def mul(x, y):
    a, b = s(x & ML), s(x >> K); c, d = s(y & ML), s(y >> K)    # 6
    re = s(a*c - b*d, W); im = s(a*d + b*c, W)                  # 4 MUL, SUB, ADD (2K-bit words: wraps like the machine)
    r = pack(re, im)                                            # 3
    k = int(((re + HI) & MW) > 2*HI) | int(((im + HI) & MW) > 2*HI)   # 2 + 2 + OR 1
    return r, k                                                 # 15 + 5 = 20, + sticky OR 1 = 21
OPS = {'add': add, 'sub': sub, 'mul': mul}
def canon(r, t):                                    # INF where t != 0: compare 1, mask 1, choice 3 = 5
    m = (0 - int(t != 0)) & MW
    return r ^ ((r ^ INF) & m)

# --- a run: acc = x0; then acc = op(acc, x_j) or op(x_j, acc) (side chosen per step). External inputs: x0, x1, ...
def run_v1(xs, steps):                              # per-element sticky
    st = 0
    for x in xs: st |= inf_bits(x)                  # once per external input: 5 + OR 1
    acc = xs[0]
    for (op, swap), x in zip(steps, xs[1:]):
        r, t = OPS[op](x, acc) if swap else OPS[op](acc, x)
        acc = r; st |= t
    return canon(acc, st), st                       # once per element: 5
# study 2 per-operation marking (for V2's slow path): every operation tests its inputs too
def ovf_op(op, x, y):
    r, t = OPS[op](x, y)
    if op == 'mul': t |= int((inf_bits(x) | inf_bits(y)) != 0)
    else: t |= inf_bits(x) | inf_bits(y)
    return canon(r, t)
def run_study2(xs, steps):
    acc = xs[0]
    for (op, swap), x in zip(steps, xs[1:]): acc = ovf_op(op, x, acc) if swap else ovf_op(op, acc, x)
    return canon(acc, inf_bits(acc))
def array_v2(arrays, steps):                        # one sticky word for the whole array
    g = 0; out = []
    for xs in arrays:
        for x in xs: g |= inf_bits(x)
        acc = xs[0]
        for (op, swap), x in zip(steps, xs[1:]):
            r, t = OPS[op](x, acc) if swap else OPS[op](acc, x)
            acc = r; g |= t
        out.append(acc)
    if g == 0: return out, False                    # once per array: compare (and the one branch)
    return [run_study2(xs, steps) for xs in arrays], True    # slow path: recompute with study 2's per-operation marking

# --- reference: study 2's semantics on true values (None = infinity)
def ref(xs, steps):
    def val(w): a, b = unpack(w); return None if MIN in (a, b) else (a, b)
    acc = val(xs[0])
    for (op, swap), w in zip(steps, xs[1:]):
        x = val(w); p, q = (x, acc) if swap else (acc, x)
        if p is None or q is None: acc = None; continue
        (a, b), (c, d) = p, q
        v = {'add': (a+c, b+d), 'sub': (a-c, b-d), 'mul': (a*c - b*d, a*d + b*c)}[op]
        acc = v if all(-HI <= t <= HI for t in v) else None
    return INF if acc is None else pack(*acc)

stats = {}
def chk(k, ok):
    stats[k + ' cases'] = stats.get(k + ' cases', 0) + 1
    if not ok: stats[k + ' FAIL'] = stats.get(k + ' FAIL', 0) + 1
def test_run(xs, steps):
    want = ref(xs, steps); got, st = run_v1(xs, steps)
    chk('V1 per-element sticky = study 2', got == want)
    chk('study 2 per-operation chain = reference', run_study2(xs, steps) == want)
    return want

allsteps = lambda n: list(itertools.product(itertools.product(OPS, (False, True)), repeat=n))
words = range(1 << W)
if K <= 4:
    lengths = (1, 2) if K == 3 else (1,)
    for n in lengths:
        for steps in allsteps(n):
            for xs in itertools.product(words, repeat=n + 1): test_run(list(xs), steps)
    random.seed(7)
    for i in range(20000):                          # longer random runs at the same K
        n = random.randint(3, 8); steps = [(random.choice(list(OPS)), random.random() < 0.5) for _ in range(n)]
        test_run([random.choice(words) for _ in range(n + 1)], steps)
    arr_n, arr_count, small = 64, 2000, 1 << (K - 2)
else:
    random.seed(7)
    def rnd():
        r = random.random()
        if r < 0.02: return pack(MIN, random.getrandbits(K))
        if r < 0.04: return pack(random.getrandbits(K), MIN)
        if r < 0.20: return pack(random.choice([HI, -HI, HI - 1, 1 << 30, -(1 << 30)]), random.choice([HI, -HI, 0, 1, -1]))
        if r < 0.70: return pack(random.randint(-(1 << 10), 1 << 10), random.randint(-(1 << 10), 1 << 10))
        return random.getrandbits(W)
    for i in range(100000):
        n = random.randint(1, 16); steps = [(random.choice(list(OPS)), random.random() < 0.5) for _ in range(n)]
        test_run([rnd() for _ in range(n + 1)], steps)
    arr_n, arr_count, small = 256, 400, 1 << 7
# --- whole arrays (V2): some arrays all small (no flag expected), some with random words (flags expected)
random.seed(77); fast = slow = 0
for i in range(arr_count):
    n = random.randint(1, 4); steps = [(random.choice(list(OPS)), random.random() < 0.5) for _ in range(n)]
    if i % 2 == 0: arrays = [[pack(random.randint(-small, small), random.randint(-small, small)) for _ in range(n + 1)] for _ in range(arr_n)]
    else: arrays = [[random.getrandbits(W) for _ in range(n + 1)] for _ in range(arr_n)]
    out, recomputed = array_v2(arrays, steps)
    chk('V2 whole-array sticky = study 2 (array)', out == [ref(xs, steps) for xs in arrays])
    if recomputed: slow += 1
    else: fast += 1
    if not recomputed: chk('V2 fast path only when no element is infinity', all(ref(xs, steps) != INF for xs in arrays))
print(f"K={K}")
for k, v in sorted(stats.items()): print(f"  {k}: {v}")
print(f"  V2 arrays: {fast} exact on the fast path, {slow} recomputed")
