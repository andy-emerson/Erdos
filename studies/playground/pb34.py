# Study 34: B-native methods for every operation of study 32's table, against A at its best (same op model).
# B: x = 2^(k>>1) (1+i)^(k&1) w, w odd (split convention, study 32). Stored either as
#   BD: unit-digit words (k, A, B) as in study 32, or
#   BH: half coordinates (k, p, q), w = (1+p+q, q-p), (1+i)w = (2p+1, 2q+1), p, q signed n-bit.
#   The two differ by two XORs each way: q = A ^ c, p = B ^ q; A = q ^ c, B = p ^ q (c = 2^(n-1) - 1;
#   with sign-extending loads, as A's int32 loads). So BD costs BH + 2 per B operand read + 2 per B result written,
#   except the unit operations, which act on the digit words directly.
# Key identity: w = 1 + (1-i) z with z = p + iq. Then
#   product   z(w1 w2) = z1 + w1 z2          (odd times odd stays odd: no strip)
#   difference w1 - w2 = (1-i)(z1 - z2)      (two subtractions, already divided by 1-i)
#   sum       w1 + w2 = (1-i)(z1 + z2 + 1 + i)
#   quotient  z(w1 / w2) = (z1 - z2) / w2    (exact; mod 2^n is enough, since z is n-bit)
#   norm      N(w) = 2(p(p+1) + q(q+1)) + 1
# Op model: study 32 (rep32.Ops): alu, sh, mul, ctz, br, ld, div; every op counts 1. Estimates, not timings.
# Usage: python3 pb34.py [table|part2|all]   (all: about 40 s)
import sys, random
from collections import Counter, defaultdict
sys.path.insert(0, __file__.rsplit('/', 1)[0] if '/' in __file__ else '.')
import adic31 as S31
import rep32 as R32
import gcd33 as G33
from rep32 import Ops, INF, gmul, norm

M64 = (1 << 64) - 1
def sx(v, n): v &= (1 << n) - 1; return v - (1 << n) if v >> (n - 1) else v
def ctz(x): return (x & -x).bit_length() - 1
ZH = (INF, 0, 0)

# ---------------- reference converters (uncounted) ----------------
def fits(v, n): return -(1 << (n - 1)) <= v < (1 << (n - 1))
def mkH(x, n):
    s = S31.to_split(x)
    if s[0] >= INF: return ZH
    k, a, b = s; p, q = (a - b - 1) >> 1, (a + b - 1) >> 1
    assert fits(p, n) and fits(q, n), ("out of range", x, n)
    return (k, p, q)
def valH(s, n):
    k, p, q = s
    if k >= INF: return (0, 0)
    assert fits(p, n) and fits(q, n), ("result out of range", s, n)
    return S31.from_split((k, 1 + p + q, q - p))
def h2d_ref(s, n):
    k, p, q = s; c = (1 << (n - 1)) - 1; M = (1 << n) - 1
    return (k, (q ^ c) & M, (p ^ q) & M)
def d2h_ref(s, n):
    k, A, B = s; c = (1 << (n - 1)) - 1
    q = sx(A ^ c, n); return (k, sx(B ^ q, n), q)
def mkD(x, n): return h2d_ref(mkH(x, n), n)
def valD(s, n): return valH(d2h_ref(s, n), n)
def D2H(s, n, o): o('alu', 2); return d2h_ref(s, n)          # counted: q = A ^ c, p = B ^ q
def H2D(s, n, o): o('alu', 2); return h2d_ref(s, n)          # counted: A = q ^ c, B = p ^ q

# ---------------- conversions pair <-> H (counted) ----------------
def h2a(s, n, o):                      # x = 2^h w (e = 0) or 2^h (1+i) w = 2^h (2p+1, 2q+1) (e = 1)
    k, p, q = s
    o('br')
    if k >= INF: return (0, 0)
    o('sh'); o('alu'); h = k >> 1; e = k & 1
    o('br')
    if e: o('sh', 2); o('alu', 2); a, b = 2 * p + 1, 2 * q + 1
    else: o('alu', 3); a, b = 1 + p + q, q - p
    o('sh', 2); return (a << h, b << h)
def strip_H(x, y, o):                  # nonzero pair -> m, f, H coords of delta, where x + iy = 2^m (1+i)^f delta: 12 ops
    o('ctz'); m = ctz(x | y)           # (the or x | y is counted by the caller's zero test)
    o('sh', 2); x >>= m; y >>= m
    o('alu', 3); f = x & y & 1; nm = f - 1
    o('alu', 4); Rr = x - (y & nm); Ss = y + (x & nm)
    o('sh', 2); return m, f, Rr >> 1, Ss >> 1
def a2h(x, o):                         # split form: x = 2^m (1+i)^f delta, so w = delta, k = 2m + f; no rotation
    o('alu'); o('br')
    if x[0] | x[1] == 0: return ZH
    m, f, p, q = strip_H(x[0], x[1], o)
    o('sh'); o('alu'); return ((m << 1) + f, p, q)

# ---------------- 2-adic inverse (shared by A and B) ----------------
INV8 = G33.INV8
def inv_mod(N, bits, o):               # N odd: inverse mod 2^bits by a 128-entry byte table and Newton rounds
    o('alu'); o('ld'); x = INV8[N & 255]; b = 8; mk = (1 << bits) - 1
    while b < bits: o('mul', 2); o('alu'); x = (x * (2 - N * x)) & mk; b *= 2
    return x & mk

# =============== add and subtract ===============
def H_addsub(x, y, n, o, sub=False):
    (k1, p1, q1), (k2, p2, q2) = x, y
    o('alu'); o('br'); d = k2 - k1
    if d == 0:
        o('br')
        if k1 >= INF: return ZH
        if sub: o('alu', 2); Dp, Dq = p1 - p2, q1 - q2                 # w1 - w2 = (1-i)(z1 - z2)
        else: o('alu', 4); Dp, Dq = p1 + p2 + 1, q1 + q2 + 1          # w1 + w2 = (1-i)(z1 + z2 + 1 + i)
        o('alu'); o('br')
        if Dp | Dq == 0: return ZH
        m, f, p, q = strip_H(Dp, Dq, o)
        o('sh'); o('alu', 3); k = k1 + 1 + (m << 1) + f
        # x1 +- x2 = (-i) 2^(h+m) (1+i)^(e+f+1) delta: w = delta, except w = -i delta when e = f = 0
        o('alu', 2); o('br')
        if ((k1 | f) & 1) == 0: o('alu'); p, q = q, ~p
        return (k, p, q)
    # unequal k: the larger-k operand is added into the smaller one's z, no strip
    neg_small = False
    o('br')
    if d < 0:
        o('alu'); d = -d; (k1, p1, q1), (k2, p2, q2) = (k2, p2, q2), (k1, p1, q1)
        neg_small = sub                 # x - y with y small: result = -(w_y) + ...: z(-w) = ~z
    o('br')
    if k2 >= INF:                       # the other operand is zero
        if neg_small: o('alu', 2); return (k1, ~p1, ~q1)
        return (k1, p1, q1)
    if neg_small: o('alu', 2); p1, q1 = ~p1, ~q1
    sgn = -1 if (sub and not neg_small) else 1
    # z' = z_small + sgn * i^r (1+i)^(d-1) w_large, r = 1 iff e_small = 0 and d odd
    o('alu'); o('sh'); h = (d - 1) >> 1
    o('alu'); o('br')
    if d & 1:
        o('alu', 3); a, b = 1 + p2 + q2, q2 - p2                      # w
        o('alu', 1); o('br')
        if (k1 & 1) == 0: o('alu'); a, b = -b, a                      # times i
    else:
        o('sh', 2); o('alu', 2); a, b = 2 * p2 + 1, 2 * q2 + 1         # (1+i) w
    o('sh', 2); a <<= h; b <<= h
    o('alu', 2)
    return (k1, p1 + sgn * a, q1 + sgn * b)
def H_add(x, y, n, o): return H_addsub(x, y, n, o, False)
def H_sub(x, y, n, o): return H_addsub(x, y, n, o, True)

# =============== multiply ===============
def H_mul(x, y, n, o):
    (k1, p1, q1), (k2, p2, q2) = x, y
    o('alu'); o('br')
    if (k1 | k2) >= INF: return ZH
    o('alu', 3); a, b = p1 + q1 + 1, q1 - p1                          # w1
    o('mul', 4); o('alu', 2); t0, t1 = a * p2 - b * q2, a * q2 + b * p2
    o('alu', 2); p, q = p1 + t0, q1 + t1                              # z(w1 w2) = z1 + w1 z2
    o('alu'); k = k1 + k2
    o('alu', 2); o('br')
    if k1 & k2 & 1: o('alu'); p, q = ~q, p                            # (1+i)^2 = 2i: times i
    return (k, p, q)

# =============== norm ===============
def H_norm(x, n, o):
    k, p, q = x
    o('br')
    if k >= INF: return 0
    o('alu', 2); o('mul', 2); o('alu'); T = p * (p + 1) + q * (q + 1)
    o('sh'); o('alu'); o('sh'); return (2 * T + 1) << k

# =============== unit operations, (1+i)-power division, tests ===============
def H_neg(x, n, o): o('alu', 2); return (x[0], ~x[1], ~x[2])
def H_muli(x, n, o): o('alu'); return (x[0], ~x[2], x[1])
def H_conj(x, n, o):                   # conj(w) swaps p and q; with e = 1, conj(1+i) = -i(1+i): (p, ~q)
    k, p, q = x
    o('alu'); o('br')
    if k & 1 and k < INF: o('alu'); return (k, p, ~q)
    return (k, q, p)
def H_rot(p, q, r, o):                 # w i^r in H: i (~q, p), -1 (~p, ~q), -i (q, ~p); jump table
    o('br'); r &= 3
    if r == 1: o('alu'); return ~q, p
    if r == 2: o('alu', 2); return ~p, ~q
    if r == 3: o('alu'); return q, ~p
    return p, q
def H_mul1i(x, n, o):
    k, p, q = x
    o('alu', 2); o('br')
    if k & 1 and k < INF: o('alu'); p, q = ~q, p
    return (k + 1, p, q)
def H_exdiv_pij(x, j, n, o):
    k, p, q = x
    if k >= INF: o('br'); return x
    o('alu'); k2 = k - j
    o('sh', 2); o('alu'); r = (k2 >> 1) - (k >> 1)
    p, q = H_rot(p, q, r, o)
    return (k2, p, q)
def H_val(x, n, o): return x[0]
def H_eq(x, y, n, o): o('alu', 5); return x == y
def H_iszero(x, n, o): o('alu'); return x[0] >= INF
def H_div1i(x, n, o): o('alu'); return x[0] >= 1
def H_div2(x, n, o): o('alu'); return x[0] >= 2

# =============== exact division ===============
def H_exdiv(x, y, n, o):               # x / y known exact: k = k1 - k2, z = (z1 - z2) / w2 mod 2^n, no strip
    (k1, p1, q1), (k2, p2, q2) = x, y
    o('br')
    if k1 >= INF: return ZH
    o('alu'); k = k1 - k2
    o('alu', 3); a, b = p2 + q2 + 1, p2 - q2                          # conj(w2) = a + ib
    o('mul', 2); o('alu'); N = a * a + b * b                           # odd: no ctz, no shift
    inv = inv_mod(N, n, o)
    o('alu', 2); d0, d1 = p1 - p2, q1 - q2
    o('mul', 4); o('alu', 2); e0, e1 = d0 * a - d1 * b, d0 * b + d1 * a
    o('mul', 2); p, q = sx(e0 * inv, n), sx(e1 * inv, n)               # n-bit results: low half of the product
    o('alu', 2); o('br')
    if (k2 & ~k1) & 1: o('alu'); p, q = q, ~p                         # k1 even, k2 odd: w = -i w1 / w2
    return (k, p, q)
def A_exdiv2(x, y, o, W=32):           # A at its best: the same table inverse, and only mod 2^32 (q is int32)
    o('mul', 4); o('alu', 2); p0, p1 = x[0] * y[0] + x[1] * y[1], x[1] * y[0] - x[0] * y[1]
    o('mul', 2); o('alu'); N = y[0] * y[0] + y[1] * y[1]
    o('ctz'); o('sh'); s = ctz(N); Nn = N >> s
    inv = inv_mod(Nn, W, o)
    o('sh', 2); o('mul', 2); return (sx((p0 >> s) * inv, W), sx((p1 >> s) * inv, W))

# =============== division with remainder (top-down: through pairs) ===============
def H_divrem(x, y, n, o):
    q, r = R32.A_divrem(h2a(x, n, o), h2a(y, n, o), o)
    return a2h(q, o), a2h(r, o)

# =============== the operation table: A old / A best / BD old / BD best / BH best ===============
def D_via_H(fh, nb_in=2, out='B'):     # digits: convert the B operands to H (2 ops each), run fh, convert results back
    def f(*args):
        *xs, n, o = args
        hs = [D2H(s, n, o) for s in xs[:nb_in]] + list(xs[nb_in:])
        r = fh(*hs, n, o)
        if out == 'B': return H2D(r, n, o)
        if out == 'B2': return tuple(H2D(t, n, o) for t in r)
        return r
    f.__name__ = 'D_' + fh.__name__; return f
def D_divrem(x, y, n, o):              # digits: study 32's 11-op decode is better than D2H + h2a; encode via H
    q, r = R32.A_divrem(R32.b2a(x, n, o), R32.b2a(y, n, o), o)
    return H2D(a2h(q, o), n, o), H2D(a2h(r, o), n, o)
def b2a_d(s, n, o): return R32.b2a(s, n, o)
# digit-native unit operations (study 32), on the digit form of this study (zero code (INF, c, 0) is harmless)
def D_neg(x, n, o): return R32.B_neg(x, n, o)
def D_conj(x, n, o): return R32.B_conj(x, n, o)
def D_muli(x, n, o): return R32.B_muli(x, n, o)
def D_mul1i(x, n, o): return R32.B_mul1i(x, n, o)
def D_exdiv_pij(x, j, n, o): return R32.B_exdiv_pij(x, j, n, o)
def D_val(x, n, o): return x[0]
def D_eq(x, y, n, o): o('alu', 5); return x == y
def D_iszero(x, n, o): o('alu'); return x[0] >= INF
def D_div1i(x, n, o): o('alu'); return x[0] >= 1
def D_div2(x, n, o): o('alu'); return x[0] >= 2

def A_val(x, o): return R32.A_val0(x, o)
TABLE = [  # name, kind, reference, A study 32, A best, B study 32 (digits), BD best, BH best
    ("add", 'bin', lambda x, y: (x[0]+y[0], x[1]+y[1]), R32.A_add, R32.A_add, R32.B_add, D_via_H(H_add), H_add),
    ("subtract", 'bin', lambda x, y: (x[0]-y[0], x[1]-y[1]), R32.A_sub, R32.A_sub, R32.B_sub, D_via_H(H_sub), H_sub),
    ("negate", 'un', lambda x: (-x[0], -x[1]), R32.A_neg, R32.A_neg, R32.B_neg, D_neg, H_neg),
    ("conjugate", 'un', lambda x: (x[0], -x[1]), R32.A_conj, R32.A_conj, R32.B_conj, D_conj, H_conj),
    ("times i", 'un', lambda x: (-x[1], x[0]), R32.A_muli, R32.A_muli, R32.B_muli, D_muli, H_muli),
    ("times 1+i", 'un', lambda x: (x[0]-x[1], x[0]+x[1]), R32.A_mul1i, R32.A_mul1i, R32.B_mul1i, D_mul1i, H_mul1i),
    ("multiply", 'mul', gmul, R32.A_mul, R32.A_mul, R32.B_mul, D_via_H(H_mul), H_mul),
    ("norm", 'val', norm, R32.A_norm, R32.A_norm, R32.B_norm, R32.B_norm, H_norm),
    ("valuation", 'valk', None, R32.A_val0, R32.A_val0, R32.B_val, D_val, H_val),
    ("equality", 'pred2', lambda x, y: x == y, R32.A_eq, R32.A_eq, R32.B_eq, D_eq, H_eq),
    ("is zero", 'pred1', lambda x: x == (0, 0), R32.A_iszero, R32.A_iszero, R32.B_iszero, D_iszero, H_iszero),
    ("divisible by 1+i", 'pred1', lambda x: (x[0] - x[1]) % 2 == 0, R32.A_div1i, R32.A_div1i, R32.B_div1i, D_div1i, H_div1i),
    ("divisible by 2", 'pred1', lambda x: x[0] % 2 == 0 and x[1] % 2 == 0, R32.A_div2, R32.A_div2, R32.B_div2, D_div2, H_div2),
    ("exact / (1+i)^j", 'pij', R32.ref_pipow, R32.A_exdiv_pij, R32.A_exdiv_pij, R32.B_exdiv_pij, D_exdiv_pij, H_exdiv_pij),
    ("exact / general", 'exdiv', None, R32.A_exdiv, A_exdiv2, R32.B_exdiv, D_via_H(H_exdiv), H_exdiv),
    ("division with remainder", 'divrem', R32.ref_divrem, R32.A_divrem, R32.A_divrem, R32.B_divrem, D_divrem, H_divrem),
]
REPS = ['A0', 'A', 'B0', 'BD', 'BH']   # B0: study 32's B (digit words, its own zero code)
def st_of(rep, x, n):
    if rep in ('A0', 'A'): return x
    if rep == 'B0': return R32.mkB(x, n)
    if rep == 'BD': return mkD(x, n)
    return mkH(x, n)
def val_of(rep, s, n):
    if rep in ('A0', 'A'): return s
    if rep == 'B0': return R32.valB(s, n)
    if rep == 'BD': return valD(s, n)
    return valH(s, n)
def call_rep(rep, f, args, n, o): return f(*args, o) if rep in ('A0', 'A') else f(*args, n, o)

def run_case(op, x, y, n, stats=None, j=None):
    name, kind, ref, *fs = op
    for ri, rep in enumerate(REPS):
        f = fs[ri]; o = Ops(); sx_, sy_ = st_of(rep, x, n), st_of(rep, y, n)
        if kind in ('bin', 'mul'): got = val_of(rep, call_rep(rep, f, (sx_, sy_), n, o), n); exp = ref(x, y)
        elif kind == 'un': got = val_of(rep, call_rep(rep, f, (sx_,), n, o), n); exp = ref(x)
        elif kind == 'val': got = call_rep(rep, f, (sx_,), n, o); exp = ref(x)
        elif kind == 'valk':
            got = call_rep(rep, f, (sx_,), n, o); exp = R32.pij_val(x) if x != (0, 0) else INF
            if exp == INF: got = INF if got >= 64 else got
        elif kind == 'pred1': got = call_rep(rep, f, (sx_,), n, o); exp = ref(x)
        elif kind == 'pred2': got = call_rep(rep, f, (sx_, sy_), n, o); exp = ref(x, y)
        elif kind == 'pij': got = val_of(rep, call_rep(rep, f, (sx_, j), n, o), n); exp = R32.ref_pipow(x, j)
        elif kind == 'exdiv':
            sxy = st_of(rep, gmul(x, y), n)
            got = val_of(rep, call_rep(rep, f, (sxy, sy_), n, o), n); exp = x
        elif kind == 'divrem':
            q, r = call_rep(rep, f, (sx_, sy_), n, o); got = (val_of(rep, q, n), val_of(rep, r, n)); exp = ref(x, y)
        assert got == exp, (name, rep, x, y, got, exp)
        if stats is not None: stats[(name, rep)].update(o); stats[(name, rep, 'n')]['n'] += 1

def verify_table():
    print("operation table: verification against big integers")
    n = 8; R = 8; box = [(a, b) for a in range(-R, R + 1) for b in range(-R, R + 1)]
    for op in TABLE:
        kind = op[1]; cnt = 0
        for x in box:
            for y in box:
                if kind in ('mul', 'exdiv') and max(map(abs, x + y)) > 4: continue
                if kind in ('exdiv', 'divrem') and y == (0, 0): continue
                if kind == 'pij':
                    if y != (0, 0): continue
                    for j in range(R32.pij_val(x) + 1 if x != (0, 0) else 6): run_case(op, x, y, n, j=j); cnt += 1
                    continue
                run_case(op, x, y, n); cnt += 1
        print("  %-24s exhaustive |a|,|b| <= %d (B with 8 digits): %d cases" % (op[0], R, cnt))

def count_table(NR=3000, seed=11):
    n = 32; rnd = random.Random(seed); stats = defaultdict(Counter)
    for op in TABLE:
        for _ in range(NR):
            x, y = R32.pick_args(op, rnd, 1 << 28)
            if op[1] == 'pij':
                jj = R32.pij_val(x) if x != (0, 0) else 0
                run_case(op, x, y, n, stats, j=rnd.randint(0, jj)); continue
            run_case(op, x, y, n, stats)
    print("full width (A int32, B 32 digits), %d random cases per operation (study 32's operands): exact" % NR)
    return stats
def cost(stats, name, rep): return sum(stats[(name, rep)].values()) / stats[(name, rep, 'n')]['n']
def print_table(stats):
    print("| operation | A (study 32) | A best | B (study 32) | B digits best | B half coords best |")
    print("|---|---|---|---|---|---|")
    for op in TABLE:
        print("| %s | %s |" % (op[0], " | ".join(R32.fmt(stats[(op[0], r)], stats[(op[0], r, 'n')]['n']) for r in REPS)))

def _main_table():
    verify_table(); print_table(count_table())

# =============== gcd family: the Hensel-quotient loop of study 33 (on Z = (1+i) * odd part) ===============
HC = G33.HC
def strip_Zm(x, y, o):                 # as study 33's strip_to_Z (14 ops with the norm), returning m and e apart
    o('alu'); o('ctz'); m = ctz(x | y)
    o('sh', 2); x >>= m; y >>= m
    o('alu', 3); e = x & y & 1; nm = e - 1
    o('alu', 4); Rr = x - (y & nm); Ss = y + (x & nm)
    o('mul', 2); o('alu'); return Rr, Ss, m, e, Rr * Rr + Ss * Ss
def hensel_q(a, b, nb, L, o, c=HC, jcap=None):   # study 33's quotient step: q = a/b mod 2^j, centred
    o('alu', 3); o('sh'); j = max(2, (L + c) >> 1)
    if jcap is not None: o('alu', 2); j = min(j, jcap)
    o('sh'); inv = G33.inv_mod64(nb >> 1, o)
    o('mul', 4); o('alu', 2); w = (a[0] * b[0] + a[1] * b[1], a[1] * b[0] - a[0] * b[1])
    o('sh', 2); o('mul', 2); w0 = G33.s64((w[0] >> 1) * inv); w1 = G33.s64((w[1] >> 1) * inv)
    o('alu'); o('sh', 4); sft = 64 - j; return (G33.s64(w0 << sft) >> sft, G33.s64(w1 << sft) >> sft)
def z_loop(a, b, na, nb, o, unit_exit=False, c=HC):
    o('alu', 2); R32.cswap_ops(o, 3)
    if na < nb: a, b, na, nb = b, a, nb, na
    steps = 0
    while True:
        if unit_exit:                  # b is (1+i) times a unit: the gcd is a unit, stop
            o('alu'); o('br')
            if nb == 2: return b, nb, steps
        o('alu'); L = G33.bitlen(na, o) - G33.bitlen(nb, o)
        q = hensel_q(a, b, nb, L, o, c)
        o('mul', 4); o('alu', 2); qb = (q[0] * b[0] - q[1] * b[1], q[0] * b[1] + q[1] * b[0])
        o('alu', 2); dx, dy = a[0] - qb[0], a[1] - qb[1]
        o('alu'); o('br'); steps += 1
        if dx | dy == 0: return b, nb, steps
        Rr, Ss, _, _, nz = strip_Zm(dx, dy, o)
        a, na = (Rr, Ss), nz
        o('alu', 2); R32.cswap_ops(o, 3)
        if na < nb: a, b, na, nb = b, a, nb, na
def h2Z(p, q, o): o('sh', 2); o('alu', 2); return (2 * p + 1, 2 * q + 1)
def Z2h(Rr, Ss, o): o('sh', 2); return (Rr >> 1, Ss >> 1)       # R, S odd: (R - 1) / 2 = R >> 1
def gcd_H(x, y, n, o, unit_exit=False):           # B in half coordinates: valuation and odd part are stored
    o('alu'); o('br')
    if (x[0] | y[0]) >= INF: return (x if y[0] >= INF else y)
    o('alu', 3); k = min(x[0], y[0])
    a = h2Z(x[1], x[2], o); b = h2Z(y[1], y[2], o)
    o('mul', 4); o('alu', 2); na = a[0] ** 2 + a[1] ** 2; nb = b[0] ** 2 + b[1] ** 2
    g, _, steps = z_loop(a, b, na, nb, o, unit_exit)
    p, q = Z2h(*g, o)
    return (k, p, q)
def gcd_A(x, y, o, unit_exit=False):               # study 33's AZ with the same loop
    o('alu', 4); o('br')
    if x == (0, 0) or y == (0, 0): return x if y == (0, 0) else y
    R1, S1, m1, e1, na = strip_Zm(*x, o); R2, S2, m2, e2, nb = strip_Zm(*y, o)
    o('sh', 2); o('alu', 2); o('alu', 3); k = min(2 * m1 + e1, 2 * m2 + e2)
    (Rr, Ss), _, steps = z_loop((R1, S1), (R2, S2), na, nb, o, unit_exit)
    o('alu'); o('br')
    if k == 0: o('alu', 2); o('sh', 2); return ((Rr + Ss) >> 1, (Ss - Rr) >> 1)
    k -= 1
    o('sh', 3); o('alu'); o('br'); g = (Rr << (k >> 1), Ss << (k >> 1))
    if k & 1: o('alu', 2); g = (g[0] - g[1], g[0] + g[1])
    return g
LAST_STEPS = [0]

# =============== extended gcd (shared core) ===============
# The loop keeps, for each loop value v (Z coordinates), a cofactor c with v = c * u_l (mod u_s), where u_s, u_l are
# the odd parts of the operands with the smaller and the larger valuation. The strip of each step divides by 2^m
# exactly; the cofactor follows by a Montgomery step: r = -c u_s^-1 mod 2^m (centred), c' = (c + r u_s) / 2^m,
# then times (1+i) when the step's e = 0 (a' = dx (1+i)^(1-e) / 2^m). No divides, no reduction mod u_s otherwise.
JCAP = 26                              # quotient precision cap: keeps q * c within 64 bits
def cmul(x, y, o): o('mul', 4); o('alu', 2); return (x[0] * y[0] - x[1] * y[1], x[0] * y[1] + x[1] * y[0])
def mont2(c, J, mod, nminv, o, check=False):     # c / 2^J mod mod, J <= 28 per chunk
    out = c
    while J > 0:
        Jc = min(J, 28); J -= Jc
        t = cmul(out, nminv, o)
        o('alu'); o('sh', 4); s = 64 - Jc; r = (G33.s64(t[0] << s) >> s, G33.s64(t[1] << s) >> s)
        rm = cmul(r, mod, o)
        o('alu', 2); o('sh', 2); n0, n1 = out[0] + rm[0], out[1] + rm[1]
        if check: assert (n0 | n1) & ((1 << Jc) - 1) == 0 and max(abs(n0), abs(n1)) < 1 << 63
        out = (n0 >> Jc, n1 >> Jc)
        if J > 0: o('br')
    return out
STATS = Counter()
def xz_loop(a, b, na, nb, ca, cb, mod, nminv, o, check=False, c=HC):
    o('alu', 2); R32.cswap_ops(o, 5)
    if na < nb: a, b, na, nb, ca, cb = b, a, nb, na, cb, ca
    steps = 0
    while True:
        o('alu'); L = G33.bitlen(na, o) - G33.bitlen(nb, o)
        q = hensel_q(a, b, nb, L, o, c, JCAP)
        qb = cmul(q, b, o)
        o('alu', 2); dx, dy = a[0] - qb[0], a[1] - qb[1]
        qc = cmul(q, cb, o)
        o('alu', 2); cd = (ca[0] - qc[0], ca[1] - qc[1])
        if check: assert max(abs(v) for v in qb + qc + cd) < 1 << 63
        o('alu'); o('br'); steps += 1
        if dx | dy == 0: return b, cb, steps
        Rr, Ss, m, e, nz = strip_Zm(dx, dy, o)
        ch = mont2(cd, m, mod, nminv, o, check)
        o('br')
        if e == 0: o('alu', 2); ch = (ch[0] - ch[1], ch[0] + ch[1])
        if check: STATS['cbits'] = max(STATS['cbits'], max(abs(ch[0]), abs(ch[1])).bit_length() - mod_bits(mod))
        a, na, ca = (Rr, Ss), nz, ch
        o('alu', 2); R32.cswap_ops(o, 5)
        if na < nb: a, b, na, nb, ca, cb = b, a, nb, na, cb, ca
def mod_bits(m): return max(abs(m[0]), abs(m[1])).bit_length()
def rotP(z, r, o): return S31.rot_branchy(z[0], z[1], r, o)
def xgcd_core(Zs, Zl, nzs, nzl, us, ul, d, o, check=False):
    """odd parts u_s, u_l (Z = (1+i) u), d = k_l - k_s >= 0. Returns g' (odd gcd, an associate), T_s, T_l with
    T_s u_s + T_l (1+i)^d u_l = g'. Everything 64-bit; the last step divides exactly mod 2^64."""
    # -u_s^-1 mod 2^64 = -conj(u_s) * N(u_s)^-1
    o('sh'); inv = G33.inv_mod64(nzs >> 1, o)
    o('alu'); o('mul', 2); nminv = (G33.s64(-us[0] * inv), G33.s64(us[1] * inv))
    g, cb, steps = xz_loop(Zs, Zl, nzs, nzl, (0, 0), (1, 1), us, nminv, o, check)
    LAST_STEPS[0] = steps
    # b = (1+i) g' = cb u_l (mod u_s): T_l = cb (1+i)^-(d+1) mod u_s = cb (1+i)^(2J - D) i^-J / 2^J, D = d + 1
    o('alu', 2); o('sh'); D = d + 1; J = (D + 1) >> 1
    o('br')
    if D & 1: o('alu', 2); cb = (cb[0] - cb[1], cb[0] + cb[1])
    Tl = mont2(cb, J, us, nminv, o, check)
    o('alu'); Tl = rotP(Tl, -J, o)
    # g' = g / (1+i); T_s = (T_l (1+i)^d u_l - g') * (-u_s^-1) mod 2^64 (exact division)
    o('alu', 2); o('sh', 2); gp = ((g[0] + g[1]) >> 1, (g[1] - g[0]) >> 1)
    o('sh'); o('alu'); h = d >> 1
    v = rotP(ul, h, o)
    o('sh', 2); v = (v[0] << h, v[1] << h)
    o('br')
    if d & 1: o('alu', 2); v = (v[0] - v[1], v[0] + v[1])
    t = cmul(Tl, v, o)
    o('alu', 2); t = (t[0] - gp[0], t[1] - gp[1])
    Ts = cmul(t, nminv, o); Ts = (G33.s64(Ts[0]), G33.s64(Ts[1]))
    return g, gp, Ts, Tl
def xgcd_A(x, y, o, check=False):      # nonzero x, y; returns g, s, t with s x + t y = g
    R1, S1, m1, e1, n1 = strip_Zm(*x, o); R2, S2, m2, e2, n2 = strip_Zm(*y, o)
    o('sh', 2); o('alu', 2); k1, k2 = 2 * m1 + e1, 2 * m2 + e2
    o('alu', 4); o('sh', 4); u1 = ((R1 + S1) >> 1, (S1 - R1) >> 1); u2 = ((R2 + S2) >> 1, (S2 - R2) >> 1)
    o('alu'); o('br'); sw = k1 > k2                       # the smaller valuation is the modulus side
    if sw: (R1, S1, n1, u1, k1), (R2, S2, n2, u2, k2) = (R2, S2, n2, u2, k2), (R1, S1, n1, u1, k1)
    o('alu'); d = k2 - k1
    g, gp, Ts, Tl = xgcd_core((R1, S1), (R2, S2), n1, n2, u1, u2, d, o, check)
    # gcd = (1+i)^k1 g' = 2^h i^h (1+i)^e g', and (1+i) g' = g (the loop's Z value)
    o('sh'); o('alu'); o('br'); h, e = k1 >> 1, k1 & 1
    z = g if e else gp
    z = rotP(z, h, o); o('sh', 2); G = (z[0] << h, z[1] << h)
    # x = (-i)^(k>>1) (1+i)^k u: the cofactors take the inverse units i^(k>>1)
    o('sh'); Ts = rotP(Ts, h, o); o('sh'); Tl = rotP(Tl, k2 >> 1, o)
    return (G, Tl, Ts) if sw else (G, Ts, Tl)
def xgcd_H(x, y, n, o, check=False, nout=64):   # B in H: outputs g in H, s and t encoded into H (they need a strip)
    (k1, p1, q1), (k2, p2, q2) = x, y
    o('alu'); o('br'); sw = k1 > k2
    if sw: (k1, p1, q1), (k2, p2, q2) = (k2, p2, q2), (k1, p1, q1)
    Z1 = h2Z(p1, q1, o); Z2 = h2Z(p2, q2, o)
    o('alu', 6); u1 = (1 + p1 + q1, q1 - p1); u2 = (1 + p2 + q2, q2 - p2)
    o('mul', 4); o('alu', 2); n1 = Z1[0] ** 2 + Z1[1] ** 2; n2 = Z2[0] ** 2 + Z2[1] ** 2
    o('alu'); d = k2 - k1
    g, gp, Ts, Tl = xgcd_core(Z1, Z2, n1, n2, u1, u2, d, o, check)
    # gcd: k = k1, w = i^(k1 >> 1) g' (split form); H of g' from its Z value g directly
    p, q = Z2h(*g, o)
    o('sh'); p, q = H_rot(p, q, k1 >> 1, o)
    G = (k1, p, q)
    o('sh'); Ts = rotP(Ts, k1 >> 1, o); o('sh'); Tl = rotP(Tl, k2 >> 1, o)   # split form: x = (-i)^(k>>1) (1+i)^k w
    S = a2h(Ts, o); T = a2h(Tl, o)
    return (G, T, S) if sw else (G, S, T)

# =============== lcm, coprimality, divisibility ===============
def lcm_A(x, y, o):
    g = gcd_A(x, y, o); return R32.A_mul(x, A_exdiv2(y, g, o), o)
def lcm_H(x, y, n, o):                 # k = max(k1, k2) falls out of k1 + k2 - min
    g = gcd_H(x, y, n, o); return H_mul(x, H_exdiv(y, g, n, o), n, o)
def coprime_A(x, y, o):
    o('alu', 5); o('br')
    if ((x[0] ^ x[1]) & 1) == 0 and ((y[0] ^ y[1]) & 1) == 0: return False     # both divisible by 1+i
    o('alu'); o('br')
    if x == (0, 0) or y == (0, 0): return False if x == y else norm(x if y == (0, 0) else y) == 1
    R1, S1, _, _, na = strip_Zm(*x, o); R2, S2, _, _, nb = strip_Zm(*y, o)
    _, nb, _ = z_loop((R1, S1), (R2, S2), na, nb, o, True)
    o('alu'); return nb == 2
def coprime_H(x, y, n, o):
    o('alu', 2); o('br')
    if x[0] != 0 and y[0] != 0: return False                                   # both k >= 1 (zero has k = INF)
    o('alu'); o('br')
    if (x[0] | y[0]) >= INF: return isunit_H(x if y[0] >= INF else y, n, o)
    a = h2Z(x[1], x[2], o); b = h2Z(y[1], y[2], o)
    o('mul', 4); o('alu', 2); na = a[0] ** 2 + a[1] ** 2; nb = b[0] ** 2 + b[1] ** 2
    _, nb, _ = z_loop(a, b, na, nb, o, True)
    o('alu'); return nb == 2
def divides_A(y, x, o):                # does y divide x? (y nonzero)
    o('alu', 5); o('br')
    if ((y[0] ^ y[1]) & 1) == 0 and ((x[0] ^ x[1]) & 1) == 1: return False     # 1+i | y but not x
    q = A_exdiv2(x, y, o)
    qy = cmul(q, y, o)
    o('alu', 3); o('br'); return qy == x
def divides_H(y, x, n, o):
    (k2, p2, q2), (k1, p1, q1) = y, x
    o('br')
    if k1 >= INF: return True
    o('alu'); o('br')
    if k2 > k1: return False
    o('alu', 3); a, b = p2 + q2 + 1, p2 - q2                          # conj(w_y)
    o('mul', 2); o('alu'); N = a * a + b * b
    inv = inv_mod(N, n, o)
    o('alu', 2); d0, d1 = p1 - p2, q1 - q2
    o('mul', 4); o('alu', 2); e0, e1 = d0 * a - d1 * b, d0 * b + d1 * a
    o('mul', 2); zq = (sx(e0 * inv, n), sx(e1 * inv, n))
    t = cmul(zq, (a, -b), o)                                           # z_q w_y == z_x - z_y, wrapping 64-bit compare
    o('alu', 3); o('br'); return ((t[0] - d0) | (t[1] - d1)) & M64 == 0

# =============== standard associate, unit and associate tests ===============
def assoc_A(x, o):                     # rotate into re > 0, im >= 0 (zero stays zero); int32 parts
    a, b = x
    o('sh'); o('alu'); o('sh'); t1 = 1 if ((b << 32) + a) < 0 else 0        # (b, a) lexicographically negative
    o('alu', 2); o('sh'); s = 1 if a - 1 + t1 < 0 else 0
    o('sh'); o('alu'); idx = 2 * t1 + s
    return rotP(x, (0, 3, 1, 2)[idx], o)
def assoc_H(x, n, o):                  # rotate w to the class p, q even (w = 1 mod (1+i)^3): low bits index a jump table
    k, p, q = x
    o('alu', 2); o('sh'); o('alu'); idx = (p & 1) | ((q & 1) << 1)
    p, q = H_rot(p, q, (0, 3, 1, 2)[idx], o)                            # (1,0) -> -i, (0,1) -> i, (1,1) -> -1
    return (k, p, q)
def isunit_A(x, o): o('mul', 2); o('alu', 2); return norm(x) == 1
def isunit_H(x, n, o): o('alu', 2); o('sh'); o('alu', 3); return ((x[1] + 1) | (x[2] + 1) | (x[0] << 1)) in (0, 1)
def isunit_D(x, n, o):                 # on digit words directly: A in {c, c+1}, B in {0, M}, k = 0
    k, A, B = x; c = (1 << (n - 1)) - 1; M = (1 << n) - 1
    o('alu', 2); o('sh'); o('alu', 3); return (((A - c) & M) | ((B + 1) & M) | (k << 1)) in (0, 1)
def isassoc_A(x, y, o):                # N(x) = N(y) and x conj(y) real or imaginary
    o('mul', 4); o('alu', 3); o('br')
    if norm(x) != norm(y): return False
    o('mul', 4); o('alu', 2); P = (x[0] * y[0] + x[1] * y[1], x[1] * y[0] - x[0] * y[1])
    o('alu', 3); return P[0] == 0 or P[1] == 0
def isassoc_H(x, y, n, o):
    o('alu'); o('br')
    if x[0] != y[0]: return False
    return H_eq(assoc_H(x, n, o), assoc_H(y, n, o), n, o)

# =============== powers ===============
def pow_pair(x, e, o):                 # left-to-right binary powering on a pair: square 5 ops, multiply 6
    acc = x
    for bit in bin(e)[3:]:
        o('alu', 2); o('mul', 2); o('sh'); acc = ((acc[0] + acc[1]) * (acc[0] - acc[1]), (acc[0] * acc[1]) << 1)
        o('br')
        if bit == '1': acc = R32.A_mul(acc, x, o)
    return acc
def pow_A(x, e, o): return pow_pair(x, e, o)
def pow_H(x, e, n, o):                 # k e, and w^e by pair powering of w (odd stays odd), then back to H
    k, p, q = x
    o('br')
    if k >= INF: return ZH
    o('alu', 3); w = (1 + p + q, q - p)
    w = pow_pair(w, e, o)
    o('alu', 2); o('sh', 2); p, q = (w[0] - w[1]) >> 1, (w[0] + w[1]) >> 1
    o('mul'); K = k * e
    o('alu'); o('br')
    if k & 1: o('sh'); p, q = H_rot(p, q, e >> 1, o)                    # (1+i)^e = 2^(e>>1) i^(e>>1) (1+i)^(e&1)
    return (K, p, q)
def pow_H_native(x, e, n, o):          # alternative: in H throughout, z(w^2) = z (1 + w), z(w w_x) = z_x + w_x z
    k, p, q = x
    o('br')
    if k >= INF: return ZH
    o('alu', 3); wx = (1 + p + q, q - p); zp, zq = p, q
    for bit in bin(e)[3:]:
        o('alu', 3); s = (2 + zp + zq, zq - zp)
        o('mul', 4); o('alu', 2); zp, zq = zp * s[0] - zq * s[1], zp * s[1] + zq * s[0]
        o('br')
        if bit == '1':
            t = cmul(wx, (zp, zq), o); o('alu', 2); zp, zq = p + t[0], q + t[1]
    o('mul'); K = k * e
    o('alu'); o('br')
    if k & 1: o('sh'); zp, zq = H_rot(zp, zq, e >> 1, o)
    return (K, zp, zq)

# =============== part 2: verification and counts ===============
assoc_norm = G33.assoc_norm
def ref_gcd(x, y): return assoc_norm(R32.ref_gcd(x, y))
def is_assoc(x, y): return R32.assoc(x, y) or x == y
def verify2():
    print("part 2: verification against big integers")
    n = 8; R = 7; box = [(a, b) for a in range(-R, R + 1) for b in range(-R, R + 1)]
    nz = [z for z in box if z != (0, 0)]
    cnt = Counter()
    for x in box:
        for y in box:
            g = ref_gcd(x, y)
            for ue in (False, True):
                assert assoc_norm(gcd_A(x, y, Ops(), ue)) == g
                assert assoc_norm(valH(gcd_H(mkH(x, n), mkH(y, n), n, Ops(), ue), n)) == g
            cop = (g == (1, 0))
            assert coprime_A(x, y, Ops()) == cop and coprime_H(mkH(x, n), mkH(y, n), n, Ops()) == cop, (x, y)
            assert isassoc_A(x, y, Ops()) == is_assoc(x, y) == isassoc_H(mkH(x, n), mkH(y, n), n, Ops())
            cnt['gcd'] += 1
            if x != (0, 0) and y != (0, 0):
                G, s, t = xgcd_A(x, y, Ops(), True)
                assert assoc_norm(G) == g and R32.gmul(s, x)[0] + R32.gmul(t, y)[0] == G[0] and gmul(s, x)[1] + gmul(t, y)[1] == G[1]
                G, s, t = xgcd_H(mkH(x, n), mkH(y, n), n, Ops(), True)
                G, s, t = valH(G, n), valH(s, 64), valH(t, 64)
                assert assoc_norm(G) == g and (gmul(s, x)[0] + gmul(t, y)[0], gmul(s, x)[1] + gmul(t, y)[1]) == G, (x, y)
                L = lcm_A(x, y, Ops()); LH = valH(lcm_H(mkH(x, n), mkH(y, n), n, Ops()), 64)
                ref_l = R32.ref_divrem(gmul(x, y), R32.ref_gcd(x, y))[0]
                assert is_assoc(L, ref_l) and is_assoc(LH, ref_l)
                cnt['xgcd'] += 1
            if y != (0, 0):
                dv = R32.ref_divrem(x, y)[1] == (0, 0)
                assert divides_A(y, x, Ops()) == dv and divides_H(mkH(y, n), mkH(x, n), n, Ops()) == dv, (x, y)
                cnt['divides'] += 1
    for x in box:
        a1 = assoc_A(x, Ops()); a2 = valH(assoc_H(mkH(x, n), n, Ops()), n)
        assert is_assoc(a1, x) and is_assoc(a2, x)
        assert isunit_A(x, Ops()) == isunit_H(mkH(x, n), n, Ops()) == isunit_D(mkD(x, n), n, Ops()) == (norm(x) == 1)
        for e in range(2, 6):
            ref = (1, 0)
            for _ in range(e): ref = gmul(ref, x)
            assert pow_A(x, e, Ops()) == ref and valH(pow_H(mkH(x, n), e, n, Ops()), 64) == ref
            assert valH(pow_H_native(mkH(x, n), e, n, Ops()), 64) == ref
    # canonical: one representative per associate class
    for rep in ('A', 'H'):
        seen = {}
        for x in nz:
            c = assoc_A(x, Ops()) if rep == 'A' else assoc_H(mkH(x, n), n, Ops())
            key = assoc_norm(x)
            assert seen.setdefault(key, c) == c
    print("  exhaustive |a|,|b| <= %d (B 8 digits): gcd (both exits), coprime, associate test %d pairs; xgcd (s x + t y = g exactly), "
          "lcm %d pairs; divisibility %d pairs; standard associate (one per class), unit test, powers e = 2..5 on all values"
          % (R, cnt['gcd'], cnt['xgcd'], cnt['divides']))
    rnd = random.Random(34); n = 32; NR = 3000
    for _ in range(NR):
        x, y = G33.sample(rnd, 31)
        if x == (0, 0) or y == (0, 0): continue
        g = ref_gcd(x, y)
        G, s, t = xgcd_A(x, y, Ops(), True)
        assert assoc_norm(G) == g and (gmul(s, x)[0] + gmul(t, y)[0], gmul(s, x)[1] + gmul(t, y)[1]) == G
        G, s, t = xgcd_H(mkH(x, n), mkH(y, n), n, Ops(), True)
        G, s, t = valH(G, n), valH(s, 64), valH(t, 64)
        assert assoc_norm(G) == g and (gmul(s, x)[0] + gmul(t, y)[0], gmul(s, x)[1] + gmul(t, y)[1]) == G
        STATS['sbits'] = max(STATS['sbits'], max(map(abs, s + t)).bit_length())
        assert assoc_norm(gcd_H(mkH(x, n), mkH(y, n), n, Ops(), True) and valH(gcd_H(mkH(x, n), mkH(y, n), n, Ops(), True), n)) == g
        assert assoc_norm(gcd_A(x, y, Ops(), True)) == g
        assert coprime_A(x, y, Ops()) == coprime_H(mkH(x, n), mkH(y, n), n, Ops()) == (g == (1, 0))
        ref_l = R32.ref_divrem(gmul(x, y), R32.ref_gcd(x, y))[0]
        assert is_assoc(lcm_A(x, y, Ops()), ref_l) and is_assoc(valH(lcm_H(mkH(x, n), mkH(y, n), n, Ops()), 64), ref_l)
        c = (rnd.randrange(-2**15, 2**15), rnd.randrange(-2**15, 2**15)); yy = (rnd.randrange(-2**15, 2**15), rnd.randrange(-2**15, 2**15))
        if yy != (0, 0):
            xx = gmul(c, yy)
            for X in (xx, (xx[0] + rnd.choice((1, 2, 1 << 20)), xx[1])):
                dv = R32.ref_divrem(X, yy)[1] == (0, 0)
                assert divides_A(yy, X, Ops()) == dv and divides_H(mkH(yy, n), mkH(X, n), n, Ops()) == dv
    print("  %d random 31-bit pairs (15%% common factor, 10%% common power of 1+i): xgcd, gcd, coprime, lcm, divisibility exact" % NR)
    print("  xgcd cofactors: largest |coordinate| %d bits; loop cofactors at most %d bits above the modulus"
          % (STATS['sbits'], STATS['cbits']))

def count2(NR=3000, seed=9):
    rnd = random.Random(seed); n = 32; h = 1 << 30
    pairs = [((rnd.randrange(-h, h), rnd.randrange(-h, h)), (rnd.randrange(-h, h), rnd.randrange(-h, h))) for _ in range(NR)]
    res = {}
    def run(name, f):
        o = Ops()
        for x, y in pairs: f(x, y, o)
        res[name] = o; return o
    run('gcd A (study 33 AZ)', lambda x, y, o: G33.gcd_AZ(x, y, o))
    run('gcd A, unit exit', lambda x, y, o: gcd_A(x, y, o, True))
    run('gcd BD (study 33 BZ)', lambda x, y, o: G33.gcd_BZ(R32.mkB(x, n), R32.mkB(y, n), n, o))
    run('gcd BH', lambda x, y, o: gcd_H(mkH(x, n), mkH(y, n), n, o))
    run('gcd BH, unit exit', lambda x, y, o: gcd_H(mkH(x, n), mkH(y, n), n, o, True))
    st = []
    def xa(x, y, o): xgcd_A(x, y, o); st.append(LAST_STEPS[0])
    run('xgcd A', xa)
    run('xgcd BH', lambda x, y, o: xgcd_H(mkH(x, n), mkH(y, n), n, o))
    res['xgcd steps'] = sum(st) / len(st)
    run('lcm A', lambda x, y, o: lcm_A(x, y, o))
    run('lcm BH', lambda x, y, o: lcm_H(mkH(x, n), mkH(y, n), n, o))
    run('coprime A', lambda x, y, o: coprime_A(x, y, o))
    run('coprime BH', lambda x, y, o: coprime_H(mkH(x, n), mkH(y, n), n, o))
    run('divides A, random', lambda x, y, o: divides_A(y, x, o))
    run('divides BH, random', lambda x, y, o: divides_H(mkH(y, n), mkH(x, n), n, o))
    dp = []
    for _ in range(NR):
        yy = (rnd.randrange(-2**15, 2**15), rnd.randrange(-2**15, 2**15)) or (1, 0)
        if yy == (0, 0): yy = (1, 0)
        dp.append((gmul((rnd.randrange(-2**15, 2**15), rnd.randrange(-2**15, 2**15)), yy), yy))
    for nm, f in (('divides A, true', lambda x, y, o: divides_A(y, x, o)),
                  ('divides BH, true', lambda x, y, o: divides_H(mkH(y, n), mkH(x, n), n, o))):
        o = Ops()
        for x, y in dp: f(x, y, o)
        res[nm] = o
    vals = [p[0] for p in pairs]
    for nm, f in (('assoc A', lambda x, o: assoc_A(x, o)), ('assoc BH', lambda x, o: assoc_H(mkH(x, n), n, o)),
                  ('isunit A', lambda x, o: isunit_A(x, o)), ('isunit BH', lambda x, o: isunit_H(mkH(x, n), n, o)),
                  ('isunit BD', lambda x, o: isunit_D(mkD(x, n), n, o))):
        o = Ops()
        for x in vals: f(x, o)
        res[nm] = o
    U = [(1, 0), (0, 1), (-1, 0), (0, -1)]
    for mix in ('random', 'associates'):
        for nm, f in (('isassoc A', lambda x, y, o: isassoc_A(x, y, o)),
                      ('isassoc BH', lambda x, y, o: isassoc_H(mkH(x, n), mkH(y, n), n, o))):
            o = Ops()
            for x, y in pairs:
                if mix == 'associates': y = gmul(x, rnd.choice(U))
                assert f(x, y, Ops()) == (mix == 'associates')
                f(x, y, o)
            res[nm + ', ' + mix] = o
    # powers: x with a random valuation, exponent 2..16, values sized so x^e fits 64 bits for A
    o1, o2, o3 = Ops(), Ops(), Ops(); NP = 2000
    for _ in range(NP):
        e = rnd.randrange(2, 17); b = max(1, 60 // e - 1)
        x = (rnd.randrange(-2**b, 2**b), rnd.randrange(-2**b, 2**b))
        if x == (0, 0): x = (1, 1)
        ref = pow_A(x, e, o1)
        assert valH(pow_H(mkH(x, 64), e, 64, o2), 64) == ref and valH(pow_H_native(mkH(x, 64), e, 64, o3), 64) == ref
    res['pow A'], res['pow BH'], res['pow BH native'] = o1, o2, o3
    print("\npart 2 counts, %d random 31-bit pairs (same pairs for every variant; powers: %d cases, e = 2..16)" % (NR, NP))
    print("| operation | ops | mul | ld | br |")
    print("|---|---|---|---|---|")
    for k, v in res.items():
        if isinstance(v, float): print("| %s | %.2f |  |  |  |" % (k, v)); continue
        d = NP if k.startswith('pow') else NR
        print("| %s | %.1f | %.1f | %.1f | %.1f |" % (k, v.tot() / d, v['mul'] / d, v['ld'] / d, v['br'] / d))
    return res

if __name__ == "__main__":
    what = sys.argv[1] if len(sys.argv) > 1 else "all"
    if what in ("table", "all"): _main_table()
    if what in ("part2", "all"): verify2(); count2()
