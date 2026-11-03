# Study 12: a family of word sizes, growing and shrinking a whole array.
# A word of width 2h holds two h-bit two's-complement halves (real part low). Finite halves lie in the symmetric range
# |v| <= 2^(h-1) - 1 (-2^(h-1) counts as not fitting). An array has one width for all its elements.
# Each operation runs at the array's width and ORs every element's "doesn't fit" test into one accumulator (deferred
# check, study 7). If the accumulator is set at the end, the whole array (and the operand array) is copied to width 2h
# (word 4h) and the operation is redone. After each operation the array shrinks to h/2 while every element passes the rule:
#   rule A: fits in h/2 bits:              |v| <= 2^(h/2-1) - 1
#   rule B: one bit of margin, h/2 - 1:    |v| <= 2^(h/2-2) - 1
# Detection: add/sub by the sign test on the packed word (study 7: overflow 4 + MIN result 5) or by computing at double width
# and applying the fit test; mul at double width (re = ac - bd fits in 2h bits: checked) then the fit test.
# neg, conj, division (exact and with remainder, study 6) and mod n never leave the range (checked).
# Usage: python3 family.py exhaustive | random | walks
import sys, random, itertools
LADDER = [4, 8, 16, 32, 64, 128]

def C(h):                                  # constants of width h (word 2h)
    W = 2*h; MW = (1 << W) - 1; ML = (1 << h) - 1
    H = (1 << (h-1)) | (1 << (W-1)); NH = ~H & MW; HI = (1 << (h-1)) - 1
    return W, MW, ML, H, NH, HI
def sx(v, k): v &= (1 << k) - 1; return v - (1 << k) if v >> (k-1) else v
def pack(re, im, h): ML = (1 << h) - 1; return ((im & ML) << h) | (re & ML)
def unpack(w, h): return sx(w, h), sx(w >> h, h)
def fits(v, h): return -((1 << (h-1)) - 1) <= v <= (1 << (h-1)) - 1
def rule_ok(v, h, rule): return fits(v, h - (1 if rule == 'B' else 0))   # shrink target width h

# ---------- the fit tests
def ufit_bad(v, h):        # extracted half v (a 2h-bit signed value): not in the symmetric h-bit range. ADD, unsigned compare: 2
    HI = (1 << (h-1)) - 1; return int(((v + HI) & ((1 << (2*h)) - 1)) > 2*HI)
def packed_shrink_bad(w, h, g):    # word of width 2h; is either h-bit half outside the symmetric g-bit range (g < h)?
    W, MW, ML, H, NH, _ = C(h)     # u = v + (2^(g-1) - 1), t = v + 2^(g-1) per half (constants below 2^(h-1): no carry
    lo = w & NH; top = w & H       # between halves); v fits exactly when u and t both have no bit at or above g.
    u = ((lo + pack((1 << (g-1)) - 1, (1 << (g-1)) - 1, h)) ^ top) & MW
    t = ((lo + pack(1 << (g-1), 1 << (g-1), h)) ^ top) & MW
    U = pack(ML ^ ((1 << g) - 1), ML ^ ((1 << g) - 1), h)            # bits g .. h-1 of each half
    return (u | t) & U             # AND, AND, ADD, XOR, ADD, XOR, OR, AND = 8; + OR into the accumulator 1
def inf_bits(x, h):                # study 1: halves equal to -2^(h-1). 5
    W, MW, ML, H, NH, _ = C(h); y = ((x & NH) + NH) & MW; return x & (~y & MW) & H

# ---------- element operations at width h: return (result word(s), bad)
def add_sign(x, y, h):
    W, MW, ML, H, NH, _ = C(h); r = (((x & NH) + (y & NH)) ^ ((x ^ y) & H)) & MW
    return r, ((x ^ r) & (y ^ r) & H) | inf_bits(r, h)                 # 5 + 4 + 5 + OR 1 (+ OR acc 1) = 16
def sub_sign(x, y, h):
    W, MW, ML, H, NH, _ = C(h); r = (((x | H) - (y & NH)) ^ ((x ^ ~y) & H)) & MW
    return r, ((x ^ y) & (x ^ r) & H) | inf_bits(r, h)                 # 16
def add_dbl(x, y, h):
    (a, b), (c, d) = unpack(x, h), unpack(y, h); re, im = a + c, b + d  # extract 6, ADD 2
    return pack(re, im, h), ufit_bad(re, h) | ufit_bad(im, h)           # fit 4, OR 1, pack 3 (+ OR acc 1) = 17
def sub_dbl(x, y, h):
    (a, b), (c, d) = unpack(x, h), unpack(y, h); re, im = a - c, b - d
    return pack(re, im, h), ufit_bad(re, h) | ufit_bad(im, h)
def mul_el(x, y, h):
    (a, b), (c, d) = unpack(x, h), unpack(y, h)
    re, im = sx(a*c - b*d, 2*h), sx(a*d + b*c, 2*h)                     # 2h-bit words (wrap like the machine)
    assert (re, im) == (a*c - b*d, a*d + b*c), "ac - bd left the double width"
    return pack(re, im, h), ufit_bad(re, h) | ufit_bad(im, h)           # 15 + fit 4 + OR 1 (+ OR acc 1) = 21
def neg_el(x, h):
    W, MW, ML, H, NH, _ = C(h); return ((H - (x & NH)) ^ (~x & H)) & MW, 0
def conj_el(x, h):
    ML = (1 << h) - 1; return (((-(x >> h)) & ML) << h) | (x & ML), 0
def tdiv(x, n): q = abs(x) // n; return q if x >= 0 else -q
def rdiv(x, n):                                                         # study 6's rounding
    q = tdiv(x, n); r = x - q*n; m = -1 if r < 0 else 0; q += m; r += n & m; return q + int(r >= n - r)
def divx_el(x, y, h):                                                   # exact division (y divides x), study 6
    (a1, a2), (b1, b2) = unpack(x, h), unpack(y, h); n = b1*b1 + b2*b2
    return pack(tdiv(a1*b1 + a2*b2, n), tdiv(a2*b1 - a1*b2, n), h), 0
def divmod_el(x, y, h):                                                 # quotient (nearest) and remainder, study 6
    (a1, a2), (b1, b2) = unpack(x, h), unpack(y, h); n = b1*b1 + b2*b2
    q1, q2 = rdiv(a1*b1 + a2*b2, n), rdiv(a2*b1 - a1*b2, n)
    return (pack(q1, q2, h), pack(a1 - (q1*b1 - q2*b2), a2 - (q1*b2 + q2*b1), h)), 0
OPS2 = {'add_sign': add_sign, 'sub_sign': sub_sign, 'add_dbl': add_dbl, 'sub_dbl': sub_dbl, 'mul': mul_el, 'divx': divx_el}
EXACT = {'add': lambda p, q: (p[0]+q[0], p[1]+q[1]), 'sub': lambda p, q: (p[0]-q[0], p[1]-q[1]),
         'mul': lambda p, q: (p[0]*q[0]-p[1]*q[1], p[0]*q[1]+p[1]*q[0])}

# ---------- arrays
class Arr:
    def __init__(s, vals, h): s.h = h; s.w = [pack(re, im, h) for re, im in vals]
    def vals(s): return [unpack(w, s.h) for w in s.w]
def widen(A, h):                      # copy to width h: per element extract 3, pack 3 = 6 per doubling
    v = A.vals(); assert all(fits(t, h) for p in v for t in p); return Arr(v, h)
def minimal(vals, rule):              # the width a fresh array gets: the smallest on the ladder passing the rule
    for h in LADDER:
        if all(fits(t, h - (1 if rule == 'B' and h > LADDER[0] else 0)) for p in vals for t in p): return h
    raise OverflowError
stats = {}; counters = {'promote': 0, 'shrink': 0, 'ops': 0, 'double_promote': 0}
def chk(k, ok):
    stats[k + ' cases'] = stats.get(k + ' cases', 0) + 1
    if not ok: stats[k + ' FAIL'] = stats.get(k + ' FAIL', 0) + 1

def shrink(A, rule):
    while A.h > LADDER[0]:
        g = A.h // 2; gg = g - (1 if rule == 'B' else 0)
        acc = 0
        for w in A.w: acc |= packed_shrink_bad(w, A.h, gg)
        want = all(fits(t, gg) for p in A.vals() for t in p)            # the rule on exact values
        chk(f'shrink decision = rule {rule}', (acc == 0) == want)
        if acc: break
        A = widen(A, g); counters['shrink'] += 1
    return A
def binop(A, B, op, rule, exact_name):
    exact = [EXACT[exact_name](p, q) for p, q in zip(A.vals(), B.vals())] if exact_name else None
    h = max(A.h, B.h); A, B = widen(A, h), widen(B, h); levels = 0
    while True:
        acc = 0; out = []
        for x, y in zip(A.w, B.w):
            r, bad = OPS2[op](x, y, h); acc |= bad; out.append(r)
        if exact is not None:
            chk(f'{op}: promote iff some exact result does not fit', (acc != 0) == any(not fits(t, h) for p in exact for t in p))
        if acc == 0: break
        h *= 2; A, B = widen(A, h), widen(B, h); levels += 1; counters['promote'] += 1
    if levels > 1: counters['double_promote'] += 1
    R = Arr([], h); R.w = out; counters['ops'] += 1
    if exact is not None: chk(f'{op}: result = exact', R.vals() == exact)
    return shrink(R, rule)
def unop(A, f, rule, exact_f):
    R = Arr([], A.h); R.w = [f(x, A.h)[0] for x in A.w]; counters['ops'] += 1
    chk(f'{f.__name__}: result = exact (never promotes)', R.vals() == [exact_f(p) for p in A.vals()])
    return shrink(R, rule)
def divx(A, B, rule):                 # A = Q B elementwise
    h = max(A.h, B.h); A2, B2 = widen(A, h), widen(B, h)
    R = Arr([], h); R.w = [divx_el(x, y, h)[0] for x, y in zip(A2.w, B2.w)]; counters['ops'] += 1
    return shrink(R, rule)
def divmod_arr(A, B, rule, shrink_r=True):
    h = max(A.h, B.h); A2, B2 = widen(A, h), widen(B, h)
    Q, Rm = Arr([], h), Arr([], h)
    for x, y in zip(A2.w, B2.w):
        (q, r), _ = divmod_el(x, y, h); Q.w.append(q); Rm.w.append(r)
    for (a, b), (q, r) in zip(zip(A2.vals(), B2.vals()), zip(Q.vals(), Rm.vals())):   # exact check: a = q b + r, N(r) <= N(b)/2
        chk('divmod: a = q b + r, 2 N(r) <= N(b), q and r fit', (q[0]*b[0]-q[1]*b[1]+r[0], q[0]*b[1]+q[1]*b[0]+r[1]) == a
            and 2*(r[0]**2 + r[1]**2) <= b[0]**2 + b[1]**2)
    counters['ops'] += 1
    return shrink(Q, rule), (shrink(Rm, rule) if shrink_r else Rm)

# ---------- mod n at width h (n < 2^(h-1)): Barrett (study 4); residues never leave [0, n), the width is fixed by n
def barrett(t, n, h):
    W = 2*h; MW = (1 << W) - 1; m = (1 << W) // n
    r = (t - ((t * m) >> W) * n) & MW; r = (r - n) & MW; return (r + (n & (MW if r >> (W-1) else 0))) & MW
def mulmod_el(x, y, n, h):
    ML = (1 << h) - 1; a, b, c, d = x & ML, x >> h, y & ML, y >> h
    return pack(barrett(a*c + (n*n - b*d), n, h), barrett(a*d + b*c, n, h), h)
def addmod_el(x, y, n, h):
    ML = (1 << h) - 1; W = 2*h; MW = (1 << W) - 1
    f = lambda u, v: (lambda r: (r + (n & (MW if r >> (W-1) else 0))) & MW)((u + v - n) & MW)
    return pack(f(x & ML, y & ML), f(x >> h, y >> h), h)

mode = sys.argv[1] if len(sys.argv) > 1 else 'exhaustive'
if mode == 'exhaustive':
    # the packed fit test against the definition: every word of width 2h, every target width g < h (h = 4, 8)
    for h in (4, 8):
        for g in range(2, h):
            for w in range(1 << (2*h)):
                re, im = unpack(w, h)
                chk('packed fit test = definition (all words, h = 4, 8)', (packed_shrink_bad(w, h, g) == 0) == (fits(re, g) and fits(im, g)))
    # the extracted-half fit test: every 2h-bit value, h = 4, 8
    for h in (4, 8):
        for v in range(-(1 << (2*h-1)), 1 << (2*h-1)): chk('extracted fit test = definition (all 2h-bit values)', ufit_bad(v, h) == int(not fits(v, h)))
    # every pair of elements at the smallest width, as one-element arrays, every operation, both rules
    vals = range(-7, 8); elems = [(a, b) for a in vals for b in vals]
    for rule in ('A', 'B'):
        for p in elems:
            P = Arr([p], 4)
            unop(P, neg_el, rule, lambda t: (-t[0], -t[1])); unop(P, conj_el, rule, lambda t: (t[0], -t[1]))
            for q in elems:
                Q = Arr([q], 4)
                for op, ex in (('add_sign', 'add'), ('add_dbl', 'add'), ('sub_sign', 'sub'), ('sub_dbl', 'sub'), ('mul', 'mul')):
                    binop(P, Q, op, rule, ex)
                if q != (0, 0):
                    divmod_arr(P, Q, rule)
                    prod = EXACT['mul'](p, q)
                    if fits(prod[0], 4) and fits(prod[1], 4):
                        R = divx(Arr([prod], 4), Q, rule); chk('exact division (p q) / q = p', R.vals() == [p])
    # mod n, all residue pairs for n = 2 .. 7 at h = 4
    for n in range(2, 8):
        for a, b, c, d in itertools.product(range(n), repeat=4):
            x, y = pack(a, b, 4), pack(c, d, 4); M = lambda w: (w & 15, w >> 4)
            chk('mod n (h = 4): mul and add exact, never leave [0, n)', M(mulmod_el(x, y, n, 4)) == ((a*c - b*d) % n, (a*d + b*c) % n)
                and M(addmod_el(x, y, n, 4)) == ((a + c) % n, (b + d) % n))
    print("exhaustive: packed fit test h = 4, 8; all element pairs at h = 4 (one-element arrays), rules A and B; mod n at h = 4")
elif mode == 'random':
    random.seed(12)
    def rarr(h, n=8):
        HI = (1 << (h-1)) - 1
        pick = lambda: random.choice([random.randint(-HI, HI), random.choice([HI, -HI, HI - 1, (HI >> 1) + 1, -(HI >> 1) - 1, 0, 1])])
        return Arr([(pick(), pick()) for _ in range(n)], h)
    for h in (8, 16, 32, 64):
        for i in range(1500):
            for rule in ('A', 'B'):
                P, Q = rarr(h), rarr(random.choice([hh for hh in LADDER if hh <= h]))
                for op, ex in (('add_sign', 'add'), ('add_dbl', 'add'), ('sub_sign', 'sub'), ('sub_dbl', 'sub'), ('mul', 'mul')):
                    binop(P, Q, op, rule, ex)
                unop(P, neg_el, rule, lambda t: (-t[0], -t[1])); unop(P, conj_el, rule, lambda t: (t[0], -t[1]))
                if all(v != (0, 0) for v in Q.vals()): divmod_arr(P, Q, rule)
                # exact division: build A = q * b with the product in range
                b = [(random.randint(-15, 15) or 1, random.randint(-15, 15)) for _ in range(8)]
                HI = (1 << (h-1)) - 1; q = [(random.randint(-(HI >> 6), HI >> 6), random.randint(-(HI >> 6), HI >> 6)) for _ in range(8)]
                R = divx(Arr([EXACT['mul'](u, v) for u, v in zip(q, b)], h), Arr(b, h), rule); chk('exact division (q b) / b = q', R.vals() == q)
        for n in [(1 << (h-1)) - 1, (1 << (h-1)) - 3, 3]:
            for i in range(3000):
                a, b, c, d = (random.randrange(n) for _ in range(4)); x, y = pack(a, b, h), pack(c, d, h); ML = (1 << h) - 1
                M = lambda w: (w & ML, w >> h)
                chk('mod n: mul and add exact, never leave [0, n)', M(mulmod_el(x, y, n, h)) == ((a*c - b*d) % n, (a*d + b*c) % n)
                    and M(addmod_el(x, y, n, h)) == ((a + c) % n, (b + d) % n))
    print("random: arrays of 8 at h = 8, 16, 32, 64 (operands at any smaller width), rules A and B; mod n at each h")
else:
    # ---------- bouncing: random walks of mixed operations, and hovering walks, under rules A and B
    LG = {h: i for i, h in enumerate(LADDER)}
    def walk_mixed(rule, seed, nops=4000, n=16):
        # magnitudes steered to wander across thresholds: a target bit size changes every 250 operations;
        # mul by small values (+~1.5 bits) or division by small values (-~1.5 bits) pulls toward it; add/sub, neg, conj in between.
        # Counted: the steps the array's width moves up and down the ladder (for any reason) after each operation.
        random.seed(seed); up = down = 0; widths = {}
        A = Arr([(random.randint(-3, 3), random.randint(-3, 3)) for _ in range(n)], 4)
        for i in range(nops):
            if i % 250 == 0: target = random.choice([5, 12, 20, 28, 45])
            h0 = A.h; vals = A.vals(); mag = max(1, max(abs(v) for p in vals for v in p)); bits = mag.bit_length()
            small = [(random.randint(-3, 3), random.randint(-3, 3)) for _ in range(n)]
            small = [p if p != (0, 0) else (1, 1) for p in small]
            r = random.random()
            if r < 0.3:
                if bits < target: A = binop(A, Arr(small, minimal(small, rule)), 'mul', rule, None)
                else: A, _ = divmod_arr(A, Arr(small, minimal(small, rule)), rule, shrink_r=False)
            elif r < 0.8:
                B = [(random.randint(-mag // 4, mag // 4), random.randint(-mag // 4, mag // 4)) for _ in range(n)]
                A = binop(A, Arr(B, minimal(B, rule)), random.choice(['add_sign', 'sub_sign']), rule, None)
            elif r < 0.9: A = unop(A, neg_el, rule, lambda t: (-t[0], -t[1]))
            else: A = unop(A, conj_el, rule, lambda t: (t[0], -t[1]))
            d = LG[A.h] - LG[h0]
            if d > 0: up += d
            else: down -= d
            widths[A.h] = widths.get(A.h, 0) + 1
        return up, down, nops, widths
    def walk_hover(rule, h, d, nops=4000, n=16):
        counters.update(promote=0, shrink=0, ops=0)
        HI = (1 << (h-1)) - 1
        A = Arr([(HI - d // 2, 0)] + [(1, 1)] * (n - 1), minimal([(HI - d // 2, 0)], rule))
        D = [(d, 0)] + [(0, 0)] * (n - 1); Darr = Arr(D, minimal(D, rule))
        for i in range(nops):
            A = binop(A, Darr, 'add_sign' if i % 2 == 0 else 'sub_sign', rule, None)
        return counters['promote'], counters['shrink'], counters['ops']
    print("bouncing (per 1,000 operations):")
    for rule in ('A', 'B'):
        for seed in (1, 2, 3):
            p, s, o, widths = walk_mixed(rule, seed)
            print(f"  rule {rule}, mixed walk seed {seed}: width steps up {1000*p/o:6.1f}, down {1000*s/o:6.1f} per 1,000 ops; "
                  f"time at each width {dict(sorted(widths.items()))}")
        for h, d in ((8, 10), (16, 100), (32, 1000)):
            p, s, o = walk_hover(rule, h, d)
            print(f"  rule {rule}, hover at the h = {h} threshold (+/- {d}): promotions {1000*p/o:6.1f}, shrinks {1000*s/o:6.1f} per 1,000 ops")
for k, v in sorted(stats.items()): print(f"  {k}: {v}")
if mode != 'walks': print(f"  promotions needing two levels at once: {counters['double_promote']}")
