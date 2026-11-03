"""m1.py: round 2, milestone 1 -- cheaper multiply and exact division for the complement layout.

Every program is a list of instructions, one tuple each, so its count is its length (cost 2 only where
one tuple stands for two machine instructions: x86 cqo+idiv). Width n is a parameter, so the same program
runs on n = 4, 6, 8 (every pair) and n = 64 (random pairs). Shift amounts and bit positions are written
in terms of n (n-1 = sign start, n-2 = axis).

Checks, per program:
  - in range: the result word equals enc(true value, axis) exactly (multiply: (-1)^(a1 a2) v1 v2 on axis
    a1^a2; divide: trunc(v1/v2) * (-1)^(a2 and not a1) on axis a1^a2, for every v2 != 0, exact or not);
  - overflow: reports whether the out-of-range results are the (n-1)-bit wrap (axis kept) or something else;
  - the machine divide never traps (divisor 0 excluded; INT_MIN / -1 checked).

  python3 -I m1.py        # about 1 min
"""
import random
import sys

# ---------------------------------------------------------------- machine model at width n
def mk(n):
    F = (1 << n) - 1
    def sx(x): return x - (1 << n) if x >> (n - 1) else x
    def sar(x, k): return (sx(x) >> k) & F
    def tdiv(a, b):
        a, b = sx(a), sx(b)
        if b == 0 or (a == -(1 << (n - 1)) and b == -1): raise ZeroDivisionError('trap')
        q = abs(a) // abs(b)
        return (q if (a < 0) == (b < 0) else -q) & F
    def pext(x, m):
        r = j = 0
        for i in range(n):
            if m >> i & 1:
                r |= (x >> i & 1) << j; j += 1
        return r
    def pdep(x, m):
        r = j = 0
        for i in range(n):
            if m >> i & 1:
                r |= (x >> j & 1) << i; j += 1
        return r
    def bfxil(d, s, lsb, w):           # ARM64: d[0:w) := s[lsb:lsb+w)
        mw = (1 << w) - 1
        return (d & ~mw & F) | ((s >> lsb) & mw)
    def bfi(d, s, lsb, w):             # ARM64: d[lsb:lsb+w) := s[0:w)
        mw = ((1 << w) - 1) << lsb
        return (d & ~mw & F) | ((s << lsb) & mw)
    ops = {
        # generic (RV64-style; also x86-64 / ARM64 single instructions)
        'shl': lambda a, k: (a << k) & F, 'shr': lambda a, k: a >> k, 'sar': sar,
        'and': lambda a, b: a & b, 'or': lambda a, b: a | b, 'xor': lambda a, b: a ^ b,
        'andn': lambda a, b: a & ~b & F,                      # a AND NOT b (x86 BMI1 andn, ARM64 bic)
        'add': lambda a, b: (a + b) & F, 'sub': lambda a, b: (a - b) & F,
        'neg': lambda a: -a & F, 'not': lambda a: ~a & F,
        'mul': lambda a, b: (a * b) & F,                      # low half
        'mulh': lambda a, b: ((sx(a) * sx(b)) >> n) & F,      # signed high half (RV mulh, ARM64 smulh)
        'div': tdiv,                                          # RV div, ARM64 sdiv: truncating
        # x86-64
        'lea': lambda a, b, sc: (a + b * sc) & F,             # lea d, [a + b*sc]
        'imulw': lambda a, b: ((sx(a) * sx(b)) & F, ((sx(a) * sx(b)) >> n) & F),  # one-operand imul: rax, rdx
        'shrd': lambda lo, hi, k: ((lo >> k) | (hi << (n - k))) & F,
        'test_s': lambda a, b: (a & b) >> (n - 1),            # test a,b -> SF
        'andn_s': lambda a, b: (a & ~b & F) >> (n - 1),       # BMI1 andn sets SF from its result
        'bt': lambda a, k: a >> k & 1,                        # bt a,k -> CF
        'cmov': lambda f, a, b: a if f else b,                # cmovs/cmovc d, a  (d = b otherwise)
        'idiv': tdiv,                                         # cqo; idiv  (cost 2)
        'pext': pext, 'pdep': pdep,                           # BMI2
        # ARM64
        'bfxil': bfxil, 'bfi': bfi,
        'ubfx': lambda a, lsb, w: (a >> lsb) & ((1 << w) - 1),
        'eor_lsr': lambda a, b, k: a ^ (b >> k),              # eor d, a, b, lsr #k
        'tst_lsr_z': lambda a, b, k: int((a & (b >> k)) == 0),     # tst a, b, lsr #k -> Z
        'bics_lsr_z': lambda a, b, k: int((a & ~(b >> k) & F) == 0),  # bics xzr, a, b, lsr #k -> Z
        'cneg_ne': lambda a, z: a if z else -a & F,           # cneg d, a, ne
    }
    return F, ops

COST = {'idiv': 2}

def run(prog, n, w1, w2):
    F, ops = mk(n)
    env = {'w1': w1, 'w2': w2, 'G': 1 << (n - 2), 'NG': ~(1 << (n - 2)) & F, 'S': 1 << (n - 1), 'F': F}
    for ins in prog:
        dst, op, *args = ins
        vals = [env[a] if isinstance(a, str) else a for a in args]
        out = ops[op](*vals)
        if isinstance(dst, tuple):
            for d, v in zip(dst, out): env[d] = v
        else:
            env[dst] = out
    return env['r']

def count(prog): return sum(COST.get(i[1], 1) for i in prog)

# ---------------------------------------------------------------- the programs (built per n)
def fix(src, dst='r'):
    """r = p XOR ((p XOR w1 XOR w2) AND G): bit n-2 := a1 XOR a2.  4"""
    return [('f1', 'xor', src, 'w1'), ('f1', 'xor', 'f1', 'w2'), ('f1', 'and', 'f1', 'G'), (dst, 'xor', src, 'f1')]

def sext4(w, x, n):      # round 1: x = w XOR ((w XOR (w shr 1)) AND G)
    return [(x, 'shr', w, 1), (x, 'xor', x, w), (x, 'and', x, 'G'), (x, 'xor', x, w)]

def two_v(w, i, n):      # L = w shl 1, h = L sar 1, D = w + h = 2v  (3; L and h kept)
    return [('L' + i, 'shl', w, 1), ('h' + i, 'sar', 'L' + i, 1), ('D' + i, 'add', w, 'h' + i)]

def mul18(n):            # round 1, as built
    return (sext4('w1', 'x1', n) + sext4('w2', 'x2', n) +
            [('m', 'and', 'w1', 'w2'), ('m', 'shl', 'm', 1), ('m', 'sar', 'm', n - 1),
             ('x1', 'xor', 'x1', 'm'), ('x1', 'sub', 'x1', 'm'), ('p', 'mul', 'x1', 'x2')] + fix('p'))

def mul17(n):            # generic: x = (w + ((w shl 1) sar 1)) sar 1, mask from the L's
    return (two_v('w1', '1', n) + [('x1', 'sar', 'D1', 1)] + two_v('w2', '2', n) + [('x2', 'sar', 'D2', 1)] +
            [('m', 'and', 'L1', 'L2'), ('m', 'sar', 'm', n - 1),
             ('x1', 'xor', 'x1', 'm'), ('x1', 'sub', 'x1', 'm'), ('p', 'mul', 'x1', 'x2')] + fix('p'))

def mul17b(n):           # generic, scaled: P = (+-2 v1) * v2 = 2p, then sar 1 and the fix
    return (two_v('w1', '1', n) + two_v('w2', '2', n) + [('x2', 'sar', 'D2', 1)] +
            [('m', 'and', 'L1', 'L2'), ('m', 'sar', 'm', n - 1),
             ('D1', 'xor', 'D1', 'm'), ('D1', 'sub', 'D1', 'm'), ('P', 'mul', 'D1', 'x2'),
             ('p', 'sar', 'P', 1)] + fix('p'))

def mul_x86(n):          # x86-64: D = 2v (lea, sar, lea), widening imul gives +-4p in rdx:rax,
    #                      hi' = 2 rdx + A (lea), shrd rax, hi', 2.   14 (+ mov copies)
    return [('L1', 'lea', 'w1', 'w1', 1), ('h1', 'sar', 'L1', 1), ('D1', 'lea', 'w1', 'h1', 1),
            ('L2', 'lea', 'w2', 'w2', 1), ('h2', 'sar', 'L2', 1), ('D2', 'lea', 'w2', 'h2', 1),
            ('k', 'test_s', 'L1', 'L2'), ('t', 'neg', 'D1'), ('D1', 'cmov', 'k', 't', 'D1'),
            (('lo', 'hi'), 'imulw', 'D1', 'D2'),
            ('A', 'xor', 'L1', 'L2'), ('A', 'shr', 'A', n - 1),
            ('hi', 'lea', 'A', 'hi', 2), ('r', 'shrd', 'lo', 'hi', 2)]

def mul_generic_wide(n):  # the same idea without widening multiply or funnel shift (RV64 mul + mulh)
    return [('L1', 'shl', 'w1', 1), ('h1', 'sar', 'L1', 1), ('D1', 'add', 'w1', 'h1'),
            ('L2', 'shl', 'w2', 1), ('h2', 'sar', 'L2', 1), ('D2', 'add', 'w2', 'h2'),
            ('m', 'and', 'L1', 'L2'), ('m', 'sar', 'm', n - 1),
            ('D1', 'xor', 'D1', 'm'), ('D1', 'sub', 'D1', 'm'),
            ('lo', 'mul', 'D1', 'D2'), ('hi', 'mulh', 'D1', 'D2'),
            ('A', 'xor', 'L1', 'L2'), ('A', 'shr', 'A', n - 1), ('hi', 'add', 'hi', 'hi'), ('hi', 'or', 'hi', 'A'),
            ('hi', 'shl', 'hi', n - 2), ('lo', 'shr', 'lo', 2), ('r', 'or', 'lo', 'hi')]

def mul_bmi2(n):         # x86-64 BMI2: pext drops the axis bit (63-bit value), pdep puts it back.  11
    return [('y1', 'pext', 'w1', 'NG'), ('y2', 'pext', 'w2', 'NG'),
            ('t', 'and', 'w1', 'w2'), ('k', 'bt', 't', n - 2), ('u', 'neg', 'y1'), ('y1', 'cmov', 'k', 'u', 'y1'),
            ('P', 'mul', 'y1', 'y2'), ('r', 'pdep', 'P', 'NG'),
            ('u', 'xor', 'w1', 'w2'), ('u', 'and', 'u', 'G'), ('r', 'or', 'r', 'u')]

def mul_bmi2_nocmov(n):  # the same with the mask form of the conditional negate.  12
    return [('y1', 'pext', 'w1', 'NG'), ('y2', 'pext', 'w2', 'NG'),
            ('m', 'and', 'w1', 'w2'), ('m', 'shl', 'm', 1), ('m', 'sar', 'm', n - 1),
            ('y1', 'xor', 'y1', 'm'), ('y1', 'sub', 'y1', 'm'),
            ('P', 'mul', 'y1', 'y2'), ('r', 'pdep', 'P', 'NG'),
            ('u', 'xor', 'w1', 'w2'), ('u', 'and', 'u', 'G'), ('r', 'or', 'r', 'u')]

def mul_a64(n):          # ARM64: asr+bfxil sign extension, ubfx/tst/cneg for i*i, eor/bfi for the axis.  10
    return [('x1', 'sar', 'w1', 1), ('x1', 'bfxil', 'x1', 'w1', 0, n - 2),
            ('x2', 'sar', 'w2', 1), ('x2', 'bfxil', 'x2', 'w2', 0, n - 2),
            ('p', 'mul', 'x1', 'x2'),
            ('t1', 'ubfx', 'w1', n - 2, 1), ('z', 'tst_lsr_z', 't1', 'w2', n - 2), ('p', 'cneg_ne', 'p', 'z'),
            ('u', 'eor_lsr', 't1', 'w2', n - 2), ('r', 'bfi', 'p', 'u', n - 2, 1)]

def div18(n):            # round 1, as built (andn counted as 1: the research's 18)
    return (sext4('w1', 'x1', n) + sext4('w2', 'x2', n) +
            [('m', 'andn', 'w2', 'w1'), ('m', 'shl', 'm', 1), ('m', 'sar', 'm', n - 1),
             ('q', 'div', 'x1', 'x2'), ('q', 'xor', 'q', 'm'), ('q', 'sub', 'q', 'm')] + fix('q'))

def div15(n):            # generic: (2 v1) / (2 v2) = v1 / v2, mask from the L's.  15 (16 without andn)
    return (two_v('w1', '1', n) + two_v('w2', '2', n) +
            [('m', 'andn', 'L2', 'L1'), ('m', 'sar', 'm', n - 1),
             ('q', 'div', 'D1', 'D2'), ('q', 'xor', 'q', 'm'), ('q', 'sub', 'q', 'm')] + fix('q'))

def div_x86(n):          # x86-64 (BMI1 andn sets SF): 15 incl. cqo (16 without BMI1: not + test)
    return [('L1', 'lea', 'w1', 'w1', 1), ('h1', 'sar', 'L1', 1), ('D1', 'lea', 'w1', 'h1', 1),
            ('L2', 'lea', 'w2', 'w2', 1), ('h2', 'sar', 'L2', 1), ('D2', 'lea', 'w2', 'h2', 1),
            ('q', 'idiv', 'D1', 'D2'),
            ('k', 'andn_s', 'L2', 'L1'), ('t', 'neg', 'q'), ('q', 'cmov', 'k', 't', 'q')] + fix('q')

def div_a64(n):          # ARM64: 10
    return [('x1', 'sar', 'w1', 1), ('x1', 'bfxil', 'x1', 'w1', 0, n - 2),
            ('x2', 'sar', 'w2', 1), ('x2', 'bfxil', 'x2', 'w2', 0, n - 2),
            ('q', 'div', 'x1', 'x2'),
            ('t2', 'ubfx', 'w2', n - 2, 1), ('z', 'bics_lsr_z', 't2', 'w1', n - 2), ('q', 'cneg_ne', 'q', 'z'),
            ('u', 'eor_lsr', 't2', 'w1', n - 2), ('r', 'bfi', 'q', 'u', n - 2, 1)]

MUL = [('mul18 (round 1)', mul18), ('mul17 generic', mul17), ('mul17b generic, scaled', mul17b),
       ('mul x86-64 widening', mul_x86), ('mul generic mul+mulh', mul_generic_wide),
       ('mul x86-64 BMI2 + cmov', mul_bmi2), ('mul BMI2, mask form', mul_bmi2_nocmov), ('mul ARM64', mul_a64)]
DIV = [('div18 (round 1)', div18), ('div15 generic', div15), ('div x86-64', div_x86), ('div ARM64', div_a64)]

# ---------------------------------------------------------------- layout and references
def layout(n):
    F = (1 << n) - 1; G = 1 << (n - 2); S = 1 << (n - 1); C = G - 1; H = G
    def val(w): return (w & C) - H * (w >> (n - 1))
    def ax(w): return (w >> (n - 2)) & 1
    def enc(v, a):
        v &= (1 << (n - 1)) - 1
        return ((v >> (n - 2)) << (n - 1)) | (a << (n - 2)) | (v & C)
    def inr(v): return -H <= v < H
    return val, ax, enc, inr

def tq(a, b):
    q = abs(a) // abs(b)
    return q if (a < 0) == (b < 0) else -q

def check(n, pairs):
    val, ax, enc, inr = layout(n)
    res = {}
    for name, f in MUL + DIV:
        prog = f(n); ism = f in [g for _, g in MUL]
        bad = ovf_total = ovf_wrap = trap = exact = 0
        for w1, w2 in pairs:
            v1, v2, a1, a2 = val(w1), val(w2), ax(w1), ax(w2)
            if ism:
                t = v1 * v2 * (-1 if a1 and a2 else 1)
            else:
                if v2 == 0: continue
                t = tq(v1, v2) * (-1 if a2 and not a1 else 1)
                exact += v1 % v2 == 0
            want = enc(t, a1 ^ a2)
            try: r = run(prog, n, w1, w2)
            except ZeroDivisionError: trap += 1; continue
            if inr(t):
                if r != want: bad += 1
            else:
                ovf_total += 1; ovf_wrap += r == want
        res[name] = (count(prog), bad, ovf_total, ovf_wrap, trap)
    return res

def identities(n):
    """The facts the write-up uses, exhaustively: returns the number of failures."""
    val, ax, enc, inr = layout(n); F = (1 << n) - 1; bad = 0
    def h(w): return mk(n)[1]['sar']((w << 1) & F, 1)
    for w in range(1 << n):
        v = val(w)
        bad += (w + h(w)) & F != (2 * v) & F                      # I1: w + ((w shl 1) sar 1) = 2v
    for w1 in range(1 << n):
        for w2 in range(1 << n):
            v1, v2 = val(w1), val(w2)
            e1, e2 = ax(w1) ^ (w1 >> (n - 1)), ax(w2) ^ (w2 >> (n - 1))
            err = (e1 & w2 & 1) ^ (e2 & w1 & 1)
            M = (1 << (n - 1)) - 1
            bad += (w1 * w2) & M != (v1 * v2 + (err << (n - 2))) & M   # I2: raw product, parity error
            bad += ((w1 << 1) * (w2 << 1)) & F != (4 * v1 * v2) & F     # I3: (w1 shl 1)(w2 shl 1) = 4 v1 v2
    return bad

def main():
    allbad = 0
    for n in (4, 6, 8):
        b = identities(n); allbad += b
        print(f"n={n} identities I1-I3: {b} failures")
    for n in (4, 6, 8):
        W = range(1 << n)
        res = check(n, [(a, b) for a in W for b in W])
        print(f"n={n} (all {1 << 2 * n} pairs)")
        for k, (c, bad, ot, ow, tr) in res.items():
            print(f"  {k:28s} count {c:2d}  in-range failures {bad}  overflow cases {ot}: wrap {ow}  traps {tr}")
            allbad += bad + tr
    n = 64; rnd = random.Random(2026); val, ax, enc, inr = layout(n); H = 1 << 62
    def rv(bits): return rnd.getrandbits(bits) * rnd.choice((1, -1))
    pairs = []
    for _ in range(25000):                       # products that fit (mostly), all four axis pairs
        b1 = rnd.randint(0, 62); b2 = rnd.randint(0, max(0, 62 - b1))
        v1, v2 = rv(b1), rv(b2)
        if not inr(v1): v1 = -H
        pairs.append((enc(v1, rnd.getrandbits(1)), enc(v2, rnd.getrandbits(1))))
    for _ in range(15000):                       # exact quotients
        q, d = rv(rnd.randint(0, 61)), rv(rnd.randint(1, 61))
        if d == 0 or not inr(q * d): continue
        pairs.append((enc(q * d, rnd.getrandbits(1)), enc(d, rnd.getrandbits(1))))
    ext = [-H, -H + 1, -1, 0, 1, 2, H - 1, H // 2, -H // 2, 3, -3]
    for x in ext:
        for y in ext:
            for a in range(4): pairs.append((enc(x, a & 1), enc(y, a >> 1)))
    for _ in range(10000):                       # anything (overflow behaviour)
        pairs.append((rnd.getrandbits(64), rnd.getrandbits(64)))
    res = check(n, pairs)
    print(f"n=64 ({len(pairs)} random and edge pairs)")
    for k, (c, bad, ot, ow, tr) in res.items():
        print(f"  {k:28s} count {c:2d}  in-range failures {bad}  overflow cases {ot}: wrap {ow}  traps {tr}")
        allbad += bad + tr
    print("TOTAL in-range failures + traps:", allbad)
    sys.exit(1 if allbad else 0)

if __name__ == '__main__':
    main()
