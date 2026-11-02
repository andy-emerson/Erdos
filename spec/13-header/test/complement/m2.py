"""m2.py: round 2, milestone 2 -- full add and full subtract on the axes, and the superoptimizer's findings.

Programs use m1.py's instruction DSL (one tuple per instruction), plus RISC-V Zbb rol/ror/maxu/minu and
sltu/seqz/snez. Rules (as complement-research.md, "Full add on the axes"):
  same axis -> the sum (difference), wrapped like an (n-1)-bit integer;
  different axes, one operand zero -> the other operand (subtract: w1 zero -> -w2 on w2's axis);
  both nonzero on different axes -> e = 1 (the result word is then free).
A zero result may have either axis. e must be 0 exactly when the result is defined.
Exhaustive on n = 4, 6, 8 (every pair) and random + edge pairs at n = 64.

  python3 -I m2.py
"""
import os, random, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import m1

def ext_ops(n):
    F, ops = m1.mk(n)
    ops.update({
        'rol': lambda a, k: ((a << k) | (a >> (n - k))) & F,
        'ror': lambda a, k: ((a >> k) | (a << (n - k))) & F,
        'maxu': max, 'minu': min,
        'sltu': lambda a, b: int(a < b), 'seqz': lambda a: int(a == 0), 'snez': lambda a: int(a != 0),
        'one': lambda: 1,
    })
    return F, ops

def run(prog, n, w1, w2):
    F, ops = ext_ops(n)
    env = {'w1': w1, 'w2': w2, 'G': 1 << (n - 2), 'NG': ~(1 << (n - 2)) & F, 'S': 1 << (n - 1), 'ONE': 1}
    for dst, op, *args in prog:
        env[dst] = ops[op](*[env[a] if isinstance(a, str) else a for a in args])
    return env['r'], env.get('e', None)

FIX = [('f', 'xor', 'b', 'src'), ('f', 'and', 'f', 'G'), ('r', 'xor', 'b', 'f')]          # 3

def full11(n):     # round 1: 11 (+5 for e)
    return [('t', 'xor', 'w1', 'w2'), ('z1', 'and', 'w1', 'NG'), ('b', 'or', 'w2', 'G'), ('b', 'add', 'z1', 'b'),
            ('Z1', 'seqz', 'z1'), ('Z1', 'neg', 'Z1'), ('src', 'and', 't', 'Z1'), ('src', 'xor', 'w1', 'src')] + FIX + \
           [('e', 'andn', 't', 'Z1'), ('e', 'shr', 'e', n - 2), ('z2', 'and', 'w2', 'NG'), ('z2', 'snez', 'z2'),
            ('e', 'and', 'e', 'z2')]

def axis_src(n):   # Zbb: the operand whose (count, sign, axis) key is larger: 4; leaves rho1, rho2
    return [('p1', 'rol', 'w1', 2), ('p2', 'rol', 'w2', 2), ('src', 'maxu', 'p1', 'p2'), ('src', 'ror', 'src', 2)]

def eflag(n):      # e = (1 <u minu(rho1, rho2)) AND (rho1 XOR rho2): 4  (bit 0)
    return [('e', 'minu', 'p1', 'p2'), ('e', 'sltu', 'ONE', 'e'), ('x', 'xor', 'p1', 'p2'), ('e', 'and', 'e', 'x')]

def full_zbb(n):   # 10 (+4 for e)
    return [('z1', 'and', 'w1', 'NG'), ('b', 'or', 'w2', 'G'), ('b', 'add', 'z1', 'b')] + axis_src(n) + FIX + eflag(n)

def fsub_zbb(n):   # 10 (+4 for e): no negate first; (w1 OR G) - (w2 OR G) passes the borrow on every input
    return [('d1', 'or', 'w1', 'G'), ('d2', 'or', 'w2', 'G'), ('b', 'sub', 'd1', 'd2')] + axis_src(n) + FIX + eflag(n)

def fsub_generic(n):   # without Zbb: z1 - z2 passes the borrow on every input; z2 is shared with e: 11 (+4)
    return [('t', 'xor', 'w1', 'w2'), ('z1', 'and', 'w1', 'NG'), ('z2', 'and', 'w2', 'NG'), ('b', 'sub', 'z1', 'z2'),
            ('Z1', 'seqz', 'z1'), ('Z1', 'neg', 'Z1'), ('src', 'and', 't', 'Z1'), ('src', 'xor', 'w1', 'src')] + FIX + \
           [('e', 'andn', 't', 'Z1'), ('e', 'shr', 'e', n - 2), ('z2', 'snez', 'z2'), ('e', 'and', 'e', 'z2')]

PROGS = [('full add, round 1', full11, False), ('full add, Zbb', full_zbb, False),
         ('full sub, Zbb', fsub_zbb, True), ('full sub, generic', fsub_generic, True)]

def check(n, pairs):
    val, ax, enc, inr = m1.layout(n); F = (1 << n) - 1; G = 1 << (n - 2)
    iz = lambda w: (w & ~G & F) == 0
    out = {}
    for name, f, sub in PROGS:
        prog = f(n); bad = 0
        for w1, w2 in pairs:
            r, e = run(prog, n, w1, w2)
            v1, v2, a1, a2 = val(w1), val(w2), ax(w1), ax(w2)
            if a1 == a2: v, a = (v1 - v2 if sub else v1 + v2), a1
            elif iz(w1): v, a = (-v2 if sub else v2), a2
            elif iz(w2): v, a = v1, a1
            else:
                bad += e != 1; continue
            want = enc(v, a)
            ok = e == 0 and (r == want or (iz(want) and r == want ^ G))
            bad += not ok
        out[name] = (len(prog), bad)
    return out

def main():
    tot = 0
    for n in (4, 6, 8):
        W = range(1 << n)
        res = check(n, [(a, b) for a in W for b in W])
        print(f"n={n}: " + "; ".join(f"{k} {c} instr, {b} failures" for k, (c, b) in res.items()))
        tot += sum(b for _, b in res.values())
    n = 64; rnd = random.Random(5); G = 1 << 62; C = G - 1
    sp = [0, 1, 2, C, C - 1, G, 1 << 63, (1 << 63) | C, (1 << 63) | 1, (1 << 64) - 1]
    def rw():
        k = rnd.random()
        if k < 0.3: return rnd.choice(sp) ^ (rnd.getrandbits(1) << 62)
        return rnd.getrandbits(64)
    pairs = []
    for _ in range(60000):
        a, b = rw(), rw()
        k = rnd.random()
        if k < 0.5: b = (b & ~G) | (a & G)                  # same axis
        elif k < 0.75: a = rnd.getrandbits(1) << 62        # w1 a zero
        elif k < 0.95: b = rnd.getrandbits(1) << 62        # w2 a zero
        pairs.append((a, b))
    res = check(n, pairs)
    print(f"n=64 ({len(pairs)} pairs): " + "; ".join(f"{k} {b} failures" for k, (c, b) in res.items()))
    tot += sum(b for _, b in res.values())
    print("TOTAL failures:", tot)
    sys.exit(1 if tot else 0)

if __name__ == '__main__':
    main()
