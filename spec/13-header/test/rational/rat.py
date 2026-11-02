"""rat.py: the checker for rational-research.md (spec 13, complex rationals on an axis).

A value (num/den) * i^a is two n-bit words:
  num word: a signed integer (two's complement), carrying the sign;
  den word: D + T*a, with T = 2^(n-1), D in 1 .. T-1 (bits 0..n-2), a = the axis in the top bit.

A formula is a straight-line program in a small DSL: one tuple per instruction, so the instruction
count is len(program) and the code checked is exactly the code counted. Operands are names (inputs
n1 d1 n2 d2, or earlier results) or the constants T, NT (= NOT T), ONE; shift amounts are the
symbols 'top' (= n-1) or an int. Inputs and constants are free (masks assumed made, per the book).

    ('x', 'xor', 'n1', 'n2')      x = n1 XOR n2
    ('x', 'shl', 'x', 'top')      x = x shl (n-1)

Ops: add sub mul and or xor andn (a AND NOT b) not neg shl shr sar.

Checks (all vectorised with numpy uint64, words masked to n bits):
  * exhaustive on n = 3..6: every numerator pair, every denominator pair with D1*D2 < T, every axis
    pair, kept when the true numerator fits in n signed bits (the cases fixes.py checks);
  * random n = 64: skewed sizes, plus the extremes (true numerator = -T or T-1, D1*D2 = T-1, D = 1).
The result must equal the expected words exactly: num = true numerator mod 2^n over D1*D2 (no
reduction), den = D1*D2 + T*axis. When the true numerator is 0 the axis bit of den is not checked
(either zero is accepted, as in fixes.py).
"""
import random
import numpy as np

U = np.uint64


class W:
    """n-bit word arithmetic on uint64 arrays."""
    def __init__(self, n):
        self.n = n
        self.F = U((1 << n) - 1)
        self.T = U(1 << (n - 1))

    def m(self, x):
        return x & self.F

    def sar(self, x, k):
        # (x XOR T) shr k - (T shr k), masked: the sign-filling shift on n bits
        return self.m(((x ^ self.T) >> U(k)) - (self.T >> U(k)))


def run(prog, env, w):
    """Evaluate prog on env (dict name -> uint64 array). Returns the env with all results."""
    n = w.n
    env = dict(env)
    env['T'] = w.T
    env['NT'] = w.m(~w.T)
    env['ONE'] = U(1)
    def g(o):
        return env[o]
    def k(o):
        return n - 1 if o == 'top' else int(o)
    with np.errstate(over='ignore'):
        for ins in prog:
            d, op = ins[0], ins[1]
            if op == 'add':   r = g(ins[2]) + g(ins[3])
            elif op == 'sub': r = g(ins[2]) - g(ins[3])
            elif op == 'mul': r = g(ins[2]) * g(ins[3])
            elif op == 'and': r = g(ins[2]) & g(ins[3])
            elif op == 'or':  r = g(ins[2]) | g(ins[3])
            elif op == 'xor': r = g(ins[2]) ^ g(ins[3])
            elif op == 'andn': r = g(ins[2]) & ~g(ins[3])
            elif op == 'not': r = ~g(ins[2])
            elif op == 'neg': r = U(0) - g(ins[2])
            elif op == 'shl': r = g(ins[2]) << U(k(ins[3]))
            elif op == 'shr': r = w.m(g(ins[2])) >> U(k(ins[3]))
            elif op == 'sar': r = w.sar(w.m(g(ins[2])), k(ins[3]))
            else: raise ValueError(op)
            env[d] = w.m(r)
    return env


# ---------------- truth ----------------
# Each op: (a1, a2) allowed?, true numerator (python int over the common den), result axis,
# and the expected den's D part.  den part is D1*D2 for add/sub/mul; division is handled separately.

def truth(op, n1, D1, a1, n2, D2, a2):
    """Return (tn, tD, ta) or None when the op is not defined on this pair (e.g. add on different axes)."""
    if op in ('add', 'sub'):
        if a1 != a2:
            return None
        tn = n1 * D2 + n2 * D1 if op == 'add' else n1 * D2 - n2 * D1
        return tn, D1 * D2, a1
    if op == 'mul':
        return n1 * n2 * (-1 if a1 & a2 else 1), D1 * D2, a1 ^ a2
    if op == 'neg':                       # unary, on (n1, D1, a1)
        return -n1, D1, a1
    if op == 'conj':                      # unary: conjugate, i -> -i
        return (-n1 if a1 else n1), D1, a1
    if op == 'div':                       # (n1/D1 i^a1) / (n2/D2 i^a2) = n1 D2 / (D1 n2) * i^(a1-a2)
        if n2 == 0:
            return None
        tn, tD = n1 * D2, D1 * n2
        if a2 and not a1:                 # i^-1 = -i
            tn = -tn
        if tD < 0:
            tn, tD = -tn, -tD
        return tn, tD, a1 ^ a2
    raise ValueError(op)


def cases_small(n, op):
    T = 1 << (n - 1)
    rows = []
    Ds = range(1, T)
    ns = range(-T, T)
    unary = op in ('neg', 'conj')
    for D1 in Ds:
        for D2 in ([1] if unary else Ds):
            if op != 'div' and D1 * D2 >= T:
                continue
            for a1 in (0, 1):
                for a2 in ((0,) if unary else (0, 1)):
                    for n1 in ns:
                        for n2 in ((0,) if unary else ns):
                            t = truth(op, n1, D1, a1, n2, D2, a2)
                            if t is None:
                                continue
                            tn, tD, ta = t
                            if not (-T <= tn < T and 1 <= tD < T):
                                continue
                            rows.append((n1, D1, a1, n2, D2, a2, tn, tD, ta))
    return rows


def _rbits(k):
    """A random integer of about k bits, skewed: often small, sometimes all ones / powers of two."""
    if k <= 0:
        return 0
    c = random.random()
    if c < 0.1:
        return (1 << k) - 1
    if c < 0.2:
        return 1 << (k - 1)
    return random.getrandbits(k)


def cases_random(op, count, n=64, seed=1):
    random.seed(seed)
    T = 1 << (n - 1)
    rows = []
    unary = op in ('neg', 'conj')
    while len(rows) < count:
        bD1 = random.randint(1, n - 2)
        bD2 = 1 if unary else random.randint(1, max(1, n - 1 - bD1))
        D1 = max(1, _rbits(bD1)); D2 = 1 if unary else max(1, _rbits(bD2))
        if op == 'div':
            D2 = max(1, _rbits(random.randint(1, n - 2)))
        a1 = random.getrandbits(1); a2 = 0 if unary else random.getrandbits(1)
        if op in ('add', 'sub'):
            a2 = a1
        b1 = random.randint(0, n - 1); b2 = random.randint(0, n - 1)
        n1 = _rbits(b1) * random.choice((1, -1)); n2 = 0 if unary else _rbits(b2) * random.choice((1, -1))
        if random.random() < 0.02:
            n1 = -T
        if random.random() < 0.02 and op != 'div':
            n2 = -T
        t = truth(op, n1, D1, a1, n2, D2, a2)
        if t is None:
            continue
        tn, tD, ta = t
        if not (-T <= tn < T and 1 <= tD < T):
            continue
        rows.append((n1, D1, a1, n2, D2, a2, tn, tD, ta))
    # extremes: hit tn = -T and T-1, D1*D2 = T-1, D = 1
    ext = []
    for _ in range(count // 20):
        a1 = random.getrandbits(1); a2 = a1 if op in ('add', 'sub') else random.getrandbits(1)
        if op == 'mul':
            D1, D2 = 1, 1
            n1 = random.choice((1, -1, T - 1, -T, (1 << 31), -(1 << 31) - 1))
            n2 = random.choice((1, -1, 0)) if abs(n1) > 1 << 32 else random.choice((1 << 31, -(1 << 32), (1 << 32) - 1))
        elif op in ('add', 'sub'):
            D1 = D2 = 1
            n1 = random.choice((T - 1, -T, 0, -1, 1, T // 2, -T // 2))
            n2 = random.choice((T - 1, -T, 0, -1, 1, T // 2, -T // 2))
        else:
            continue
        t = truth(op, n1, D1, a1, n2, D2, a2)
        if t and -T <= t[0] < T:
            ext.append((n1, D1, a1, n2, D2, a2) + t)
    if op in ('add', 'sub', 'mul'):
        for _ in range(count // 20):  # D1*D2 = T-1 = 3*3*7*19*73*... use 7 * (T-1)/7
            D1, D2 = 7, (T - 1) // 7
            a1 = random.getrandbits(1); a2 = a1 if op != 'mul' else random.getrandbits(1)
            n1 = random.choice((1, -1, 0, 2)); n2 = random.choice((1, -1, 0, 3))
            t = truth(op, n1, D1, a1, n2, D2, a2)
            if t and -T <= t[0] < T:
                ext.append((n1, D1, a1, n2, D2, a2) + t)
    return rows + ext


def dword(D, a, n, layout):
    """The denominator word. offset: D + T*a (the design). neg: (-1)^a * D in two's complement
    (top bit is still a, since 1 <= D < T)."""
    T = 1 << (n - 1); F = (1 << n) - 1
    return (D + T * a) & F if layout == 'offset' else ((-D if a else D) & F)


def pack(rows, n, layout='offset'):
    """rows -> env of uint64 arrays (words) plus expected words."""
    T = 1 << (n - 1); F = (1 << n) - 1
    cols = list(zip(*rows))
    n1, D1, a1, n2, D2, a2, tn, tD, ta = cols
    A = lambda xs: np.array([x & F for x in xs], dtype=U)
    env = {
        'n1': A(n1), 'n2': A(n2),
        'd1': A([dword(D, a, n, layout) for D, a in zip(D1, a1)]),
        'd2': A([dword(D, a, n, layout) for D, a in zip(D2, a2)]),
    }
    exp_num = A(tn)
    exp_den = A([dword(D, a, n, layout) for D, a in zip(tD, ta)])
    alt_den = A([dword(D, 1 - a, n, layout) for D, a in zip(tD, ta)])   # the other zero
    zero = np.array([x == 0 for x in tn])
    return env, exp_num, exp_den, alt_den, zero


_cache = {}


_rows = {}


def data(op, n, layout='offset', rand_count=200000):
    key = (op, n, layout)
    if key not in _cache:
        if (op, n) not in _rows:
            _rows[(op, n)] = cases_small(n, op) if n <= 8 else cases_random(op, rand_count, n)
        rows = _rows[(op, n)]
        _cache[key] = (len(rows),) + pack(rows, n, layout)
    return _cache[key]


def check(prog, op, n, out=('num', 'den'), zero_any_axis=True, layout='offset'):
    """Number of failing cases (0 = correct) and the number of cases."""
    cnt, env, en, ed, alt, zero = data(op, n, layout)
    w = W(n)
    r = run(prog, env, w)
    bad = r[out[0]] != en
    if zero_any_axis:
        bad |= np.where(zero, (r[out[1]] != ed) & (r[out[1]] != alt), r[out[1]] != ed)
    else:
        bad |= r[out[1]] != ed
    return int(bad.sum()), cnt


def report(name, prog, op, ns=(3, 4, 5, 6, 64), **kw):
    res = []
    ok = True
    for n in ns:
        b, c = check(prog, op, n, **kw)
        ok &= b == 0
        res.append(f"n={n}: {c} cases, {b} fail")
    print(f"{'OK  ' if ok else 'FAIL'} {name:34s} {len(prog):2d} instr | " + "; ".join(res))
    return ok
