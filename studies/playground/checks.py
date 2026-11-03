# Study 15: the overflow and shrink checks recounted with the cheapest correct mask forms.
# A word of 2h bits holds two h-bit halves (real low). Two variants:
#   A: -2^(h-1) reserved (symmetric range |v| <= 2^(h-1) - 1); inputs are in that range.
#   B: full two's-complement range [-2^(h-1), 2^(h-1) - 1]; inputs are any values.
# Two models:
#   scalar: the packed word in a 2h-bit register (64 bits at h = 32); halves interact through carries.
#   vector: h-bit lanes (32-bit at h = 32), lanes independent; mul by the widening h x h -> 2h lane multiply.
# Every check ORs a contribution into a sticky word; the mask that picks the meaningful bits is applied once per array
# (deferred). Each check is verified: the flag (contribution AND end mask != 0) is set iff the true result does not fit,
# results equal the true values mod 2^h, and a sticky over a whole array is set iff any element does not fit.
# Usage: python3 checks.py
import random, itertools

def consts(h):
    W = 2*h; MW = (1 << W) - 1; ML = (1 << h) - 1
    H = (1 << (h-1)) | (1 << (W-1)); NH = ~H & MW; BIT = 1 << h; HB = 1 << (W-1)
    return W, MW, ML, H, NH, BIT, HB
def sx(v, k): v &= (1 << k) - 1; return v - (1 << k) if v >> (k-1) else v
def halves(w, h): return sx(w, h), sx(w >> h, h)
def pack(a, b, h): ML = (1 << h) - 1; return ((b & ML) << h) | (a & ML)
def fits(v, h, var):
    lo = -(1 << (h-1)) + (1 if var == 'A' else 0); return lo <= v <= (1 << (h-1)) - 1

# ---------------- scalar model (packed word). Each returns (result word, sticky contribution); END masks below.
def s_add(x, y, h, var):
    W, MW, ML, H, NH, BIT, HB = consts(h)
    s = (x + y) & MW; t = x ^ y; r = (s - ((t ^ s) & BIT)) & MW        # ADD, XOR, XOR, AND, SUB = 5
    c = (x ^ r) & ~t & MW                                               # XOR, ANDN = 2 (3 without andn); + OR sticky 1
    if var == 'A': c |= r & ~(((r & NH) + NH) & MW) & MW                # half = -2^(h-1): AND, ADD, ANDN = 3; + OR 1
    return r, c
def s_sub(x, y, h, var):
    W, MW, ML, H, NH, BIT, HB = consts(h)
    s = (x - y) & MW; t = x ^ y; r = (s + ((t ^ s) & BIT)) & MW        # SUB, XOR, XOR, AND, ADD = 5
    c = t & (x ^ r)                                                     # XOR, AND = 2; + OR sticky 1
    if var == 'A': c |= r & ~(((r & NH) + NH) & MW) & MW
    return r, c
def s_neg(x, h, var):
    W, MW, ML, H, NH, BIT, HB = consts(h)
    s = (0 - x) & MW; r = (s + ((x ^ s) & BIT)) & MW                   # NEG, XOR, AND, ADD = 4
    return r, (x & r if var == 'B' else 0)                              # B: AND = 1; + OR sticky 1.  A: none
def s_conj(x, h, var):
    W, MW, ML, H, NH, BIT, HB = consts(h)
    r = ((x & ML) - (x & ~ML & MW)) & MW                                # AND, AND, SUB = 3 (lo - hi 2^h: no borrow)
    return r, (x & r if var == 'B' else 0)                              # B: AND = 1; + OR into its own sticky (end mask HB)
def s_mul(x, y, h, var, form='add'):
    a, b = halves(x, h); c, d = halves(y, h); W = 2*h; MW = (1 << W) - 1
    re, im = sx(a*c - b*d, W), sx(a*d + b*c, W)                         # extract 6, 4 MUL, SUB, ADD, pack 3 = 15
    r = pack(re, im, h); C = 1 << (h-1); HI = C - 1
    if form == 'shift':                                                 # B semantics: (v shl h) sar h XOR v: 3 per part
        f = lambda v: (sx((v << h) & MW, W) >> h) ^ v
        cc = (f(re) | f(im)) & MW                                       # 3 + 3 + OR + OR sticky = 8 (end mask: all bits)
        if var == 'A': cc |= int(re == -C) | int(im == -C)             # A: + 2 compares, 2 OR
        return r, cc
    if var == 'B': cc = ((re + C) | (im + C)) & MW                     # ADD, ADD, OR = 3; + OR sticky 1 (end mask: bits >= h)
    else: cc = ((re + HI) | (re + C) | (im + HI) | (im + C)) & MW      # 4 ADD, 3 OR = 7; + OR sticky 1
    return r, cc
def s_shrink(w, h, g, var):                                             # do both halves fit in g bits? (g < h)
    W, MW, ML, H, NH, BIT, HB = consts(h); lo, top = w & NH, w & H     # AND, AND
    Cg = pack(1 << (g-1), 1 << (g-1), h)
    t = ((lo + Cg) ^ top) & MW                                          # ADD, XOR      (B: 4 in all; + OR sticky 1)
    if var == 'B': return t
    u = ((lo + pack((1 << (g-1)) - 1, (1 << (g-1)) - 1, h)) ^ top) & MW  # ADD, XOR, OR  (A: 7; + OR sticky 1)
    return u | t
def s_end(h, kind, g=None):
    W, MW, ML, H, NH, BIT, HB = consts(h)
    if kind in ('add', 'sub', 'neg'): return H
    if kind == 'conj': return HB
    if kind == 'mul': return MW ^ ((1 << h) - 1)                        # bits h .. 2h-1 of the 2h-bit parts
    if kind == 'mulshift': return MW
    if kind == 'shrink': return pack(ML ^ ((1 << g) - 1), ML ^ ((1 << g) - 1), h)

# ---------------- vector model (h-bit lanes, independent). Lane values as h-bit words.
def v_add(x, y, h, var):
    M = (1 << h) - 1; r = (x + y) & M                                   # 1
    c = (x ^ r) & (y ^ r)                                               # XOR, XOR, AND = 3; + OR sticky 1 (end: sign bit)
    if var == 'A': c |= M if r == 1 << (h-1) else 0                     # compare-equal to -2^(h-1) 1; + OR 1
    return r, c
def v_sub(x, y, h, var):
    M = (1 << h) - 1; r = (x - y) & M
    c = (x ^ y) & (x ^ r)
    if var == 'A': c |= M if r == 1 << (h-1) else 0
    return r, c
def v_neg(x, h, var):
    M = (1 << h) - 1; r = (0 - x) & M                                   # 1
    return r, (x & r if var == 'B' else 0)                              # B: AND 1 + OR 1
def v_conj_hi(x, h, var):                                              # the imaginary lane of conj (the real lane is unchanged)
    M = (1 << h) - 1; m = M; r = ((x ^ m) - m) & M                      # (x XOR m) - m with m = 0 / all-ones by lane: 2
    return r, (x & r if var == 'B' else 0)                              # B: AND 1 + OR 1 (own sticky, end mask: odd lanes' signs)
def v_mul_check(re, im, h, var):                                       # re, im: 2h-bit lane values from the widening multiply
    W = 2*h; MW = (1 << W) - 1; C = 1 << (h-1); HI = C - 1
    if var == 'B': return ((re + C) | (im + C)) & MW                   # ADD, ADD, OR + OR sticky = 4 (end: high h bits)
    return ((re + HI) | (re + C) | (im + HI) | (im + C)) & MW          # 4 ADD, 3 OR + OR sticky = 8
def v_shrink(x, h, g, var):                                            # h-bit lane x (as an h-bit word): fits in g bits?
    M = (1 << h) - 1; t = (x + (1 << (g-1))) & M                        # ADD 1 (+ OR sticky 1)  (end mask: bits g .. h-1)
    if var == 'B': return t
    return t | ((x + (1 << (g-1)) - 1) & M)                             # A: ADD, ADD, OR (+ OR sticky) = 4

res = {}
def chk(k, ok):
    res[k + ' cases'] = res.get(k + ' cases', 0) + 1
    if not ok: res[k + ' FAIL'] = res.get(k + ' FAIL', 0) + 1
def okin(w, h, var): a, b = halves(w, h); return var == 'B' or (fits(a, h, 'A') and fits(b, h, 'A'))

def check_scalar_bin(x, y, h, var):
    a, b = halves(x, h); c, d = halves(y, h)
    for name, f, tv in (('add', s_add, (a + c, b + d)), ('sub', s_sub, (a - c, b - d))):
        r, cc = f(x, y, h, var)
        bad = not (fits(tv[0], h, var) and fits(tv[1], h, var))
        chk(f'scalar {name} {var}: flag iff the true result does not fit', ((cc & s_end(h, name)) != 0) == bad)
        chk(f'scalar {name} {var}: result = true value mod 2^h', halves(r, h) == (sx(tv[0], h), sx(tv[1], h)))
    for form in ('add', 'shift'):
        r, cc = s_mul(x, y, h, var, form)
        tv = (a*c - b*d, a*d + b*c); bad = not (fits(tv[0], h, var) and fits(tv[1], h, var))
        end = s_end(h, 'mul' if form == 'add' else 'mulshift')
        chk(f'scalar mul {var} ({form} form): flag iff the true result does not fit', ((cc & end) != 0) == bad)
        chk(f'scalar mul {var}: result = true value mod 2^h', halves(r, h) == (sx(tv[0], h), sx(tv[1], h)))
def check_scalar_un(x, h, var):
    a, b = halves(x, h)
    for name, f, tv in (('neg', s_neg, (-a, -b)), ('conj', s_conj, (a, -b))):
        r, cc = f(x, h, var)
        bad = not (fits(tv[0], h, var) and fits(tv[1], h, var))
        chk(f'scalar {name} {var}: flag iff the true result does not fit', ((cc & s_end(h, name)) != 0) == bad)
        chk(f'scalar {name} {var}: result = true value mod 2^h', halves(r, h) == (sx(tv[0], h), sx(tv[1], h)))
    for g in range(2, h):
        cc = s_shrink(x, h, g, var)
        chk(f'scalar shrink {var}: flag iff a half does not fit in g bits', ((cc & s_end(h, 'shrink', g)) != 0) ==
            (not (fits(a, g, var) and fits(b, g, var))))
def check_vector(x, y, h, var):                                        # one lane pair (h-bit words)
    M = (1 << h) - 1; a, c = sx(x, h), sx(y, h); S = 1 << (h-1)
    for name, f, tv in (('add', v_add, a + c), ('sub', v_sub, a - c)):
        r, cc = f(x, y, h, var)
        chk(f'vector {name} {var}: flag iff the true result does not fit', ((cc & S) != 0) == (not fits(tv, h, var)))
        chk(f'vector {name} {var}: result = true value mod 2^h', sx(r, h) == sx(tv, h))
def check_vector_un(x, h, var):
    a = sx(x, h); S = 1 << (h-1)
    for name, f in (('neg', v_neg), ('conj (imaginary lane)', v_conj_hi)):
        r, cc = f(x, h, var)
        chk(f'vector {name} {var}: flag iff the true result does not fit', ((cc & S) != 0) == (not fits(-a, h, var)))
        chk(f'vector {name} {var}: result = true value mod 2^h', sx(r, h) == sx(-a, h))
    for g in range(2, h):
        cc = v_shrink(x, h, g, var); U = ((1 << h) - 1) ^ ((1 << g) - 1)
        chk(f'vector shrink {var}: flag iff the lane does not fit in g bits', ((cc & U) != 0) == (not fits(a, g, var)))
def check_vector_mul(v, h, var):                                       # every 2h-bit part value v (re = v, im = 0)
    W = 2*h; MW = (1 << W) - 1; end = MW ^ ((1 << h) - 1)
    chk(f'vector mul {var}: flag iff the part does not fit', ((v_mul_check(v, 0, h, var) & end) != 0) == (not fits(v, h, var)))
    chk(f'vector mul {var}: flag iff the part does not fit (imaginary)', ((v_mul_check(0, v, h, var) & end) != 0) == (not fits(v, h, var)))

random.seed(15)
for var in ('A', 'B'):
    # h = 4: every word pair (scalar), every lane pair (vector), every 2h-bit part (mul checks)
    h = 4; words = [w for w in range(1 << 8) if okin(w, h, var)]
    for x in words:
        check_scalar_un(x, h, var)
        for y in words: check_scalar_bin(x, y, h, var)
    lanes = [v for v in range(1 << h) if var == 'B' or sx(v, h) != -(1 << (h-1))]
    for x in lanes:
        check_vector_un(x, h, var)
        for y in lanes: check_vector(x, y, h, var)
    for v in range(-(1 << 7), 1 << 7): check_vector_mul(v, h, var)
    # h = 8: unary scalar on every word; binary scalar exhaustive in each half with the other half from 20 edge/random pairs;
    # vector on every lane pair; mul checks on every 16-bit part; scalar mul on 100000 random pairs
    h = 8; words = [w for w in range(1 << 16) if okin(w, h, var)]
    for x in words: check_scalar_un(x, h, var)
    vals = [v for v in range(-128, 128) if var == 'B' or v != -128]
    edge = [-127, 127, -1, 0, 1, 64, -64, 63] + ([-128] if var == 'B' else [])
    others = [(random.choice(edge), random.choice(edge)) for _ in range(12)] + [(random.choice(vals), random.choice(vals)) for _ in range(8)]
    for (p, q) in others:
        for a in vals:
            for c in vals:
                check_scalar_bin(pack(a, p, h), pack(c, q, h), h, var)     # low halves exhaustive
                check_scalar_bin(pack(p, a, h), pack(q, c, h), h, var)     # high halves exhaustive
    lanes = [v for v in range(1 << h) if var == 'B' or sx(v, h) != -(1 << (h-1))]
    for x in lanes:
        check_vector_un(x, h, var)
        for y in lanes: check_vector(x, y, h, var)
    for v in range(-(1 << 15), 1 << 15): check_vector_mul(v, h, var)
    # h = 32: random
    h = 32; HI = (1 << 31) - 1
    def rw():
        pick = lambda: random.choice([random.randint(-HI, HI), random.choice([HI, -HI, 0, 1, -1, 1 << 30, -(1 << 30), 1 << 15, -(1 << 15)])]
                                     + ([-(1 << 31)] if var == 'B' else []))
        return pack(pick(), pick(), h)
    for i in range(100000):
        x, y = rw(), rw(); check_scalar_bin(x, y, h, var)
        if i % 10 == 0: check_scalar_un(x, h, var)
        check_vector(x & 0xffffffff, y & 0xffffffff, h, var)
        if i % 10 == 0: check_vector_un(x & 0xffffffff, h, var)
        check_vector_mul(sx(random.getrandbits(64), 64) >> random.randint(0, 40), h, var)
    # the deferred sticky over whole arrays: set iff any element does not fit (scalar add and mul, h = 8, arrays of 16)
    h = 8
    for i in range(20000):
        xs = [pack(random.choice(vals), random.choice(vals), h) for _ in range(16)]
        ys = [pack(random.choice(vals), random.choice(vals), h) if random.random() < 0.5 else pack(random.randint(-20, 20), random.randint(-20, 20), h) for _ in range(16)]
        if i % 2: xs = [pack(random.randint(-60, 60), random.randint(-60, 60), h) for _ in range(16)]
        for name, f, op, end in (('add', s_add, lambda p, q: (p[0]+q[0], p[1]+q[1]), s_end(h, 'add')),
                                 ('mul', s_mul, lambda p, q: (p[0]*q[0]-p[1]*q[1], p[0]*q[1]+p[1]*q[0]), s_end(h, 'mul'))):
            st = 0
            for x, y in zip(xs, ys): st |= f(x, y, h, var)[1]
            anybad = any(not (fits(t[0], h, var) and fits(t[1], h, var)) for t in (op(halves(x, h), halves(y, h)) for x, y in zip(xs, ys)))
            chk(f'deferred sticky over an array ({name}) {var}: set iff any element does not fit', ((st & end) != 0) == anybad)
for k, v in sorted(res.items()): print(f"  {k}: {v}")
