# Study 17: one type, three memory layouts, counted at the vector level (full-range variant, sticky checks of study 15).
#   (a) interleaved: h-bit lanes re, im, re, im ... (the packed word, viewed as lanes)
#   (b) structure of arrays: one array of real parts, one of imaginary parts; a register holds 4 re or 4 im
#   (c) whole-word: the packed word in 2h-bit lanes, 2h-bit instructions only (study 15's scalar forms, per lane)
# A small vector machine: a register is 4h bits (128 at h = 32), viewed as 4 h-bit lanes or 2 2h-bit lanes. Every
# primitive increments a counter, so the counts below are exact for the instruction sequences written here.
# Primitives exist on SSE4.1/AVX2, NEON and RVV except where noted:
#   mul_even  = x86 pmuldq (signed low h bits of each 2h lane, 2h-bit product); NEON/RVV do the same through wmul.
#   wmul_lo/hi = NEON smull/smull2 (lanes 0-1 / 2-3, widened); RVV vwmul gives both in one register group.
#   narrow2   = NEON uzp1 on the h-bit view (or xtn + xtn2), RVV vnsrl; x86 needs 2 (shift + blend), counted separately.
#   swap_pairs = x86 pshufd, NEON rev64, RVV vrgather (or a slide pair).
# Verification: results equal the true value mod 2^h per half; the sticky flag (contribution AND end mask != 0) is set iff
# some element's true result does not fit in h-bit two's complement; the shrink flag iff some half does not fit in h/2 bits.
# Usage: python3 layout.py
import random, itertools

CNT = {}
def tick(k): CNT[k] = CNT.get(k, 0) + 1
class V:                                       # vector machine with lane width h (register = 4 h-bit lanes)
    def __init__(s, h): s.h = h; s.M = (1 << h) - 1; s.M2 = (1 << (2*h)) - 1
    def to2(s, x): h = s.h; return (x[0] | x[1] << h, x[2] | x[3] << h)            # reinterpret (free)
    def to1(s, X): h, M = s.h, s.M; return (X[0] & M, X[0] >> h, X[1] & M, X[1] >> h)
    def op(s, f, *regs, w=None):              # lane-wise op on registers of equal lane count
        tick('op'); Mw = s.M if len(regs[0]) == 4 else s.M2
        return tuple(f(*ls) & Mw for ls in zip(*regs))
    def add(s, x, y): return s.op(lambda a, b: a + b, x, y)
    def sub(s, x, y): return s.op(lambda a, b: a - b, x, y)
    def xor(s, x, y): return s.op(lambda a, b: a ^ b, x, y)
    def and_(s, x, y): return s.op(lambda a, b: a & b, x, y)
    def andn(s, x, y): return s.op(lambda a, b: a & ~b, x, y)                         # x AND NOT y (pandn/bic; RVV base: 2)
    def or_(s, x, y): return s.op(lambda a, b: a | b, x, y)
    def shl(s, x, k): return s.op(lambda a: a << k, x)
    def shr(s, x, k): return s.op(lambda a: a >> k, x)
    def sx(s, v, k): v &= (1 << k) - 1; return v - (1 << k) if v >> (k-1) else v
    def mul_even(s, X, Y):                     # 2h-lane registers: signed low h bits times signed low h bits
        tick('op'); tick('mul'); h = s.h; return tuple((s.sx(a, h) * s.sx(b, h)) & s.M2 for a, b in zip(X, Y))
    def wmul_lo(s, x, y): tick('op'); tick('mul'); h = s.h; return tuple((s.sx(x[i], h) * s.sx(y[i], h)) & s.M2 for i in (0, 1))
    def wmul_hi(s, x, y): tick('op'); tick('mul'); h = s.h; return tuple((s.sx(x[i], h) * s.sx(y[i], h)) & s.M2 for i in (2, 3))
    def narrow2(s, P, Q): tick('op'); tick('shuffle'); return (P[0] & s.M, P[1] & s.M, Q[0] & s.M, Q[1] & s.M)
    def swap_pairs(s, x): tick('op'); tick('shuffle'); return (x[1], x[0], x[3], x[2])
    def blend_odd(s, x, y): tick('op'); tick('shuffle'); return (x[0], y[1], x[2], y[3])  # even lanes from x, odd from y

def C1(v, h, c): return (c & v.M,) * 4            # broadcast constants (free: loaded once per loop)
def C2(v, c): return (c & v.M2,) * 2

# ---------------- (a) interleaved h-bit lanes: register = (re0, im0, re1, im1)
def a_add(v, x, y): r = v.add(x, y); return r, v.and_(v.xor(x, r), v.xor(y, r))
def a_sub(v, x, y): r = v.sub(x, y); return r, v.and_(v.xor(x, y), v.xor(x, r))
def a_neg(v, x): r = v.sub((0,) * 4, x); return r, v.and_(x, r)
def a_conj(v, x):
    m = (0, v.M, 0, v.M); r = v.sub(v.xor(x, m), m); return r, v.and_(x, r)      # check: own sticky, end mask odd lanes' signs
def a_mul(v, x, y):                                # x86 form (pmuldq): 11 instructions, 4 multiplies
    h = v.h; X, Y = v.to2(x), v.to2(y)
    ac = v.mul_even(X, Y); Xs = v.shr(X, h); Ys = v.shr(Y, h); bd = v.mul_even(Xs, Ys)
    Ysw = v.to2(v.swap_pairs(y)); ad = v.mul_even(X, Ysw); bc = v.mul_even(Xs, Y)
    re = v.sub(ac, bd); im = v.add(ad, bc)
    r = v.blend_odd(v.to1(re), v.to1(v.shl(im, h)))
    c = v.or_(v.add(re, C2(v, 1 << (h-1))), v.add(im, C2(v, 1 << (h-1))))          # check 3 (+ sticky OR)
    return r, c
def a_shrink(v, x, g): return v.add(x, C1(v, v.h, 1 << (g-1)))                    # 1 (+ sticky OR)

# ---------------- (b) structure of arrays: registers R = (re0..re3), I = (im0..im3)
def b_add(v, R1, I1, R2, I2): (r, c1), (i, c2) = a_add(v, R1, R2), a_add(v, I1, I2); return r, i, v.or_(c1, c2)
def b_sub(v, R1, I1, R2, I2): (r, c1), (i, c2) = a_sub(v, R1, R2), a_sub(v, I1, I2); return r, i, v.or_(c1, c2)
def b_neg(v, R, I): (r, c1), (i, c2) = a_neg(v, R), a_neg(v, I); return r, i, v.or_(c1, c2)
def b_conj(v, R, I): i, c = a_neg(v, I); return R, i, c                          # only the imaginary array is touched
def b_mul(v, A, B, Cr, D, form='generic'):
    h = v.h; Ch = C2(v, 1 << (h-1))
    if form == 'generic':                          # widening multiply of low and high lanes (smull/smull2, vwmul)
        ac = (v.wmul_lo(A, Cr), v.wmul_hi(A, Cr)); bd = (v.wmul_lo(B, D), v.wmul_hi(B, D))
        ad = (v.wmul_lo(A, D), v.wmul_hi(A, D)); bc = (v.wmul_lo(B, Cr), v.wmul_hi(B, Cr))
        re = [v.sub(ac[k], bd[k]) for k in (0, 1)]; im = [v.add(ad[k], bc[k]) for k in (0, 1)]
        c = v.or_(v.or_(v.add(re[0], Ch), v.add(re[1], Ch)), v.or_(v.add(im[0], Ch), v.add(im[1], Ch)))
        return v.narrow2(re[0], re[1]), v.narrow2(im[0], im[1]), c
    # x86 form: pmuldq takes even lanes; odd lanes by shifting each input once; narrow by shift + blend
    E = lambda x: v.to2(x); O = lambda x: v.shr(v.to2(x), h)
    As, Bs, Cs, Ds = O(A), O(B), O(Cr), O(D)
    ac = (v.mul_even(E(A), E(Cr)), v.mul_even(As, Cs)); bd = (v.mul_even(E(B), E(D)), v.mul_even(Bs, Ds))
    ad = (v.mul_even(E(A), E(D)), v.mul_even(As, Ds)); bc = (v.mul_even(E(B), E(Cr)), v.mul_even(Bs, Cs))
    re = [v.sub(ac[k], bd[k]) for k in (0, 1)]; im = [v.add(ad[k], bc[k]) for k in (0, 1)]
    c = v.or_(v.or_(v.add(re[0], Ch), v.add(re[1], Ch)), v.or_(v.add(im[0], Ch), v.add(im[1], Ch)))
    nar = lambda P: v.blend_odd(v.to1(P[0]), v.to1(v.shl(P[1], h)))                  # even elements in even lanes, odd moved up
    return nar(re), nar(im), c

# ---------------- (c) whole-word: 2h-bit lanes, register = (pack(re0, im0), pack(re1, im1))
def c_consts(v):
    h = v.h; H = (1 << (h-1)) | (1 << (2*h-1)); return H, v.M2 & ~H, 1 << h, v.M, v.M2 ^ v.M
def c_add(v, X, Y):
    H, NH, BIT, ML, MH = c_consts(v)
    s = v.add(X, Y); t = v.xor(X, Y); r = v.sub(s, v.and_(v.xor(t, s), C2(v, BIT)))
    return r, v.andn(v.xor(X, r), t)
def c_sub(v, X, Y):
    H, NH, BIT, ML, MH = c_consts(v)
    s = v.sub(X, Y); t = v.xor(X, Y); r = v.add(s, v.and_(v.xor(t, s), C2(v, BIT)))
    return r, v.and_(t, v.xor(X, r))
def c_neg(v, X):
    H, NH, BIT, ML, MH = c_consts(v)
    s = v.sub((0, 0), X); r = v.add(s, v.and_(v.xor(X, s), C2(v, BIT))); return r, v.and_(X, r)
def c_conj(v, X):
    H, NH, BIT, ML, MH = c_consts(v)
    r = v.sub(v.and_(X, C2(v, ML)), v.and_(X, C2(v, MH))); return r, v.and_(X, r)  # own sticky, end mask: bit 2h-1
def c_mul(v, X, Y):                                # the same instruction stream as (a): pmuldq reads each 2h lane's low half
    r, c = a_mul(v, v.to1(X), v.to1(Y)); return v.to2(r), c
def c_shrink(v, X, g):
    H, NH, BIT, ML, MH = c_consts(v); h = v.h; Cg = (1 << (g-1)) | (1 << (g-1)) << h
    return v.xor(v.add(v.and_(X, C2(v, NH)), C2(v, Cg)), v.and_(X, C2(v, H)))

# ---------------- harness
def sxv(x, k): x &= (1 << k) - 1; return x - (1 << k) if x >> (k-1) else x
def fits(x, k): return -(1 << (k-1)) <= x <= (1 << (k-1)) - 1
res = {}; per = {}
def chk(k, ok):
    res[k + ' cases'] = res.get(k + ' cases', 0) + 1
    if not ok: res[k + ' FAIL'] = res.get(k + ' FAIL', 0) + 1
def exact(op, p, q):
    if op == 'add': return (p[0] + q[0], p[1] + q[1])
    if op == 'sub': return (p[0] - q[0], p[1] - q[1])
    if op == 'neg': return (-p[0], -p[1])
    if op == 'conj': return (p[0], -p[1])
    if op == 'mul': return (p[0]*q[0] - p[1]*q[1], p[0]*q[1] + p[1]*q[0])

def run(layout, op, v, P, Q, g=None):             # P, Q: lists of elements (re, im); 2 elements (a, c) or 4 (b)
    h = v.h; M = v.M; before = CNT.get('op', 0); U1 = (M ^ ((1 << g) - 1)) if g else 0
    sign = lambda k: tuple(1 << (k-1) for _ in range(4))
    if layout in ('a', 'c'):
        x = (P[0][0] & M, P[0][1] & M, P[1][0] & M, P[1][1] & M); y = (Q[0][0] & M, Q[0][1] & M, Q[1][0] & M, Q[1][1] & M)
        if layout == 'a':
            f = {'add': a_add, 'sub': a_sub, 'mul': a_mul}
            if op in f: r, c = f[op](v, x, y)
            elif op == 'neg': r, c = a_neg(v, x)
            elif op == 'conj': r, c = a_conj(v, x)
            else: r, c = None, a_shrink(v, x, g)
            end = {'mul': (v.M2 ^ M,) * 2, 'conj': (0, 1 << (h-1), 0, 1 << (h-1)), 'shrink': (U1,) * 4}.get(op, sign(h))
            out = None if r is None else [(sxv(r[0], h), sxv(r[1], h)), (sxv(r[2], h), sxv(r[3], h))]
        else:
            X, Y = v.to2(x), v.to2(y)
            f = {'add': c_add, 'sub': c_sub, 'mul': c_mul}
            if op in f: R, c = f[op](v, X, Y)
            elif op == 'neg': R, c = c_neg(v, X)
            elif op == 'conj': R, c = c_conj(v, X)
            else: R, c = None, c_shrink(v, X, g)
            H = (1 << (h-1)) | (1 << (2*h-1)); U = (M ^ ((1 << g) - 1)) if g else 0
            end = {'mul': (v.M2 ^ M,) * 2, 'conj': (1 << (2*h-1),) * 2, 'shrink': (U | U << h,) * 2}.get(op, (H, H))
            out = None if R is None else [(sxv(w, h), sxv(w >> h, h)) for w in R]
    else:
        R1 = tuple(p[0] & M for p in P); I1 = tuple(p[1] & M for p in P); R2 = tuple(q[0] & M for q in Q); I2 = tuple(q[1] & M for q in Q)
        if op == 'add': r, i, c = b_add(v, R1, I1, R2, I2)
        elif op == 'sub': r, i, c = b_sub(v, R1, I1, R2, I2)
        elif op == 'neg': r, i, c = b_neg(v, R1, I1)
        elif op == 'conj': r, i, c = b_conj(v, R1, I1)
        elif op == 'mul': r, i, c = b_mul(v, R1, I1, R2, I2)
        elif op == 'mulx86': r, i, c = b_mul(v, R1, I1, R2, I2, form='x86'); op = 'mul'
        else: r = i = None; c = v.or_(a_shrink(v, R1, g), a_shrink(v, I1, g)); tick('sticky-or-correction')
        end = {'mul': (v.M2 ^ M,) * 2, 'shrink': (U1,) * 4}.get(op, sign(h))
        out = None if r is None else [(sxv(r[k], h), sxv(i[k], h)) for k in range(4)]
    flag = any(cc & e for cc, e in zip(c, end))
    if op == 'shrink':
        bad = any(not (fits(p[0], g) and fits(p[1], g)) for p in P)
    else:
        tru = [exact(op, p, q) for p, q in zip(P, Q)]
        bad = any(not (fits(t[0], h) and fits(t[1], h)) for t in tru)
        chk(f'({layout}) {op}: results = true values mod 2^h', out == [(sxv(t[0], h), sxv(t[1], h)) for t in tru])
    chk(f'({layout}) {op}: flag iff some element does not fit', flag == bad)
    return CNT.get('op', 0) - before

random.seed(17)
OPS = ['add', 'sub', 'neg', 'conj', 'mul', 'shrink']
for h in (4, 8, 32):
    v = V(h); lo, hi = -(1 << (h-1)), (1 << (h-1)) - 1
    if h == 4:
        el = [(a, b) for a in range(lo, hi + 1) for b in range(lo, hi + 1)]
        pairs = [(p, q) for p in el for q in el]                             # every element pair, h = 4
    elif h == 8:
        vals = range(lo, hi + 1); edge = [lo, hi, -1, 0, 1, 64, -64, 63]
        others = [(random.choice(edge), random.choice(edge)) for _ in range(6)] + [(random.randint(lo, hi), random.randint(lo, hi)) for _ in range(4)]
        pairs = []                                                           # real parts exhaustive with imaginary parts from 10
        for (bi, di) in others:                                              # pairs, and imaginary parts exhaustive with real
            for a in vals:                                                   # parts from the same 10 pairs
                for c in vals: pairs.append(((a, bi), (c, di))); pairs.append(((bi, a), (di, c)))
    else:
        R = lambda: random.choice([random.randint(lo, hi), random.choice([lo, hi, 0, 1, -1, 1 << 30, -(1 << 30), 1 << 15, -(1 << 15)])])
        pairs = [((R(), R()), (R(), R())) for _ in range(60000)]
    for layout in ('a', 'b', 'c'):
        n = 4 if layout == 'b' else 2
        for op in OPS + (['mulx86'] if layout == 'b' else []):
            if op == 'shrink':
                g = h // 2
            else: g = None
            # each element alone (others zero): exact per-element flags; then full registers: flag iff any element bad
            step = 1 if h == 4 else 7
            for k in range(0, len(pairs), step):
                p, q = pairs[k]
                P = [p] + [(0, 0)] * (n - 1); Q = [q] + [(0, 0)] * (n - 1)
                c = run(layout, op, v, P, Q, g)
                per[(layout, op)] = c
            for k in range(0, len(pairs) - n, n * (1 if h == 4 else 5)):
                grp = pairs[k:k + n]; run(layout, op, v, [p for p, q in grp], [q for p, q in grp], g)
for k, val in sorted(res.items()): print(f"  {k}: {val}")
print("  instructions per register (op), from the simulator:")
for (layout, op), c in sorted(per.items()): print(f"    ({layout}) {op}: {c}")
