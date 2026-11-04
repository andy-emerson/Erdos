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
# Usage: python3 pb34.py [verify|count|all]
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
    ("norm", 'val', norm, R32.A_norm, R32.A_norm, R32.B_norm, D_via_H(H_norm, 1, 'v'), H_norm),
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

if __name__ == "__main__" and len(sys.argv) > 1 and sys.argv[1] == "table":
    verify_table(); print_table(count_table())
