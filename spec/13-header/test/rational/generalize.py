"""generalize.py: re-check every hit printed by rsuper on wider words.

Each hit line ("v1=d1 and T; v2=Q or v1; ...  [num = last, den = v2]") is turned into the rat.py DSL
(with the raw products P, Q prepended in mode 1) and checked exhaustively on n = 3..6 and on the
random 64-bit cases. Only the outputs the search targeted are compared.

  python3 -I generalize.py logs/post-off-add-both-N3.txt [more logs]
"""
import os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
from rat import W, run, data

RAW = {
    'add': [('P_a', 'mul', 'n1', 'd2'), ('P_b', 'mul', 'n2', 'd1'), ('P', 'add', 'P_a', 'P_b'), ('Q', 'mul', 'd1', 'd2')],
    'sub': [('P_a', 'mul', 'n1', 'd2'), ('P_b', 'mul', 'n2', 'd1'), ('P', 'sub', 'P_a', 'P_b'), ('Q', 'mul', 'd1', 'd2')],
    'mul': [('P', 'mul', 'n1', 'n2'), ('Q', 'mul', 'd1', 'd2')],
}
CONST = {'1': 'ONE', 'T': 'T', '~T': 'NT', '-1': 'M1'}
BIN = {'+': 'add', '-': 'sub', '*': 'mul', 'and': 'and', 'or': 'or', 'xor': 'xor', 'andn': 'andn', 'rsub': 'rsub'}
UN = {'not': ('not', None), 'neg': ('neg', None), 'shl 1': ('shl', 1), 'shl N-1': ('shl', 'top'),
      'shr N-1': ('shr', 'top'), 'sar N-1': ('sar', 'top')}


def parse(line):
    body, outs = line.strip().rsplit('[', 1)
    prog = []
    for st in [x.strip() for x in body.split(';') if x.strip()]:
        d, e = st.split('=', 1)
        for u, (op, k) in UN.items():
            if e.startswith(u + ' '):
                a = e[len(u) + 1:]
                prog.append((d, op, a) + ((k,) if k is not None else ()))
                break
        else:
            a, op, b = e.split(' ')
            b = CONST.get(b, b)
            if op == 'rsub':
                prog.append((d, 'sub', b, a))
            else:
                prog.append((d, BIN[op], a, b))
    last = prog[-1][0]
    o = {}
    for part in outs.rstrip(']').split(','):
        k, v = [x.strip() for x in part.split('=')]
        o[k] = last if v == 'last' else v
    return prog, o


def check(prog, outs, op, layout, ns=(3, 4, 5, 6, 64)):
    fails = []
    for n in ns:
        cnt, env, en, ed, alt, zero = data(op, n, 'offset' if layout == 'off' else 'neg')
        r = run(prog, env, W(n))
        bad = np.zeros(cnt, dtype=bool)
        if 'num' in outs: bad |= r[outs['num']] != en
        if 'den' in outs: bad |= (r[outs['den']] != ed) & ((r[outs['den']] != alt) | ~zero)
        fails.append(int(bad.sum()))
    return fails


def main(paths):
    for path in paths:
        head = open(path).readline()
        m = re.search(r'layout=(\w+) op=(\w+) target=(\w+) mode=(\d)', head)
        layout, op, tgt, mode = m.group(1), m.group(2), m.group(3), int(m.group(4))
        ops = ['add', 'mul'] if op == 'am' else [op]
        hits = [l for l in open(path) if l.strip().startswith('v1=')]
        allok = 0
        for l in hits:
            p, o = parse(l)
            ok = True
            for oo in ops:
                full = (RAW[oo] if mode == 1 else []) + p
                f = check(full, o, oo, layout)
                if any(f):
                    ok = False
                    print(f"  FAIL on {oo} {f} (n=3,4,5,6,64): {l.strip()}")
            allok += ok
        print(f"{os.path.basename(path)}: {len(hits)} printed hits, {allok} hold on n = 3..6 and 64")


if __name__ == '__main__':
    main(sys.argv[1:])
