"""generalize.py: re-checks every program in a superoptimizer hit file (found on 4 bits) on n = 6, 8 exhaustively
and on 100,000 random n = 64 pairs, for same-axis add; also re-checks each OF@vK tag.
  python3 -I generalize.py add-L5-hits.txt
"""
import os, random, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from check import L

def run(prog, Lw, w1, w2):
    env = {'w1': w1, 'w2': w2}; F = Lw.F
    cs = {'1': 1, '~1': ~1 & F, 'G': Lw.G, '~G': ~Lw.G & F, 'S': Lw.S, '~S': ~Lw.S & F, 'K': Lw.K, 'C': Lw.C}
    ofs = {}
    for k, ins in enumerate(prog, 1):
        dst, rhs = ins.split('=')
        parts = rhs.split()
        if len(parts) == 2:
            op, a = parts; x = env[a]
            env[dst] = (~x if op == 'not' else -x) & F
            continue
        a, op, b = parts
        x = env[a]; y = env[b] if b in env else cs[b]
        if op == 'add': r = x + y; ofs[k] = Lw.of_add(x, y)
        elif op == 'sub': r = x - y; ofs[k] = Lw.of_sub(x, y)
        elif op == 'rsub': r = y - x; ofs[k] = Lw.of_sub(y, x)
        elif op == 'and': r = x & y
        elif op == 'or': r = x | y
        elif op == 'xor': r = x ^ y
        elif op == 'andn': r = x & ~y
        elif op == 'candn': r = y & ~x
        else: raise ValueError(op)
        env[dst] = r & F
    return env['r'], ofs

def main(path):
    progs = []
    for line in open(path):
        tag, body = line.split('|')
        progs.append(([int(t) for t in re.findall(r'OF@v(\d)', tag)], [s.strip() for s in body.split(';') if s.strip()]))
    rnd = random.Random(7); bad = 0
    for tags, prog in progs:
        for n in (6, 8, 64):
            Lw = L(n)
            if n < 64: pairs = [(a, b) for a in range(1 << n) for b in range(1 << n) if Lw.ax(a) == Lw.ax(b)]
            else:
                pairs = []
                for _ in range(20000):
                    a = rnd.getrandbits(64); b = (rnd.getrandbits(64) & ~Lw.G) | (a & Lw.G)
                    if rnd.random() < 0.3: b = (b & Lw.K) | (Lw.C - rnd.getrandbits(3))
                    pairs.append((a, b))
            for w1, w2 in pairs:
                s = Lw.val(w1) + Lw.val(w2)
                r, ofs = run(prog, Lw, w1, w2)
                if r != Lw.enc(s, Lw.ax(w1)) or any(ofs[k] != int(not Lw.inrange(s)) for k in tags):
                    bad += 1; print('FAIL', n, prog); break
    print(f"{len(progs)} programs, {bad} failures (n = 6, 8 exhaustive; 64 random)")

main(sys.argv[1])
