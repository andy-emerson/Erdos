"""m3.py: milestone 3, the superoptimizer's best programs as full routines (raw products included),
counted and checked (n = 3..6 exhaustive, n = 64 random), plus the shared correction with the
operation's mask k (0 for add, all ones for multiply).

  python3 -I m3.py
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
from rat import report, W, run, data

RAW_ADD = [('pa', 'mul', 'n1', 'd2'), ('pb', 'mul', 'n2', 'd1'), ('P', 'add', 'pa', 'pb'), ('Q', 'mul', 'd1', 'd2')]
RAW_MUL = [('P', 'mul', 'n1', 'n2'), ('Q', 'mul', 'd1', 'd2')]

# offset layout, add, post-correction (search: 5, shortest; 30 hits, one family). 4 + 5 = 9
ADD_OFF_POST = RAW_ADD + [
    ('A', 'and', 'd1', 'T'),           # T*a
    ('den', 'or', 'Q', 'A'),           # a = 0: Q is exact; a = 1: OR sets the bit
    ('x', 'xor', 'n1', 'n2'),
    ('x', 'mul', 'x', 'A'),            # T * a * parity(n1 + n2) = the leak
    ('num', 'xor', 'P', 'x'),
]
# the same without a multiply in the fix: 4 + 6 = 10
ADD_OFF_POST_NOMUL = RAW_ADD + [
    ('A', 'and', 'd1', 'T'),
    ('den', 'or', 'Q', 'A'),
    ('x', 'xor', 'n1', 'n2'),
    ('x', 'shl', 'x', 'top'),
    ('x', 'and', 'x', 'A'),
    ('num', 'xor', 'P', 'x'),
]
# offset layout, multiply: a denominator fix found by the search (4, shortest)
MUL_OFF_DEN = RAW_MUL + [
    ('v', 'sub', 'Q', 'd2'),
    ('v', 'xor', 'd1', 'v'),
    ('v', 'and', 'v', 'T'),
    ('den', 'xor', 'Q', 'v'),
    ('m', 'and', 'd1', 'd2'),
    ('m', 'sar', 'm', 'top'),
    ('x', 'xor', 'P', 'm'),
    ('num', 'sub', 'x', 'm'),
]
# neg layout, add, post-correction (search: 4, shortest). 4 + 4 = 8
ADD_NEG_POST = RAW_ADD + [
    ('m', 'sar', 'd1', 'top'),
    ('s', 'or', 'm', 'ONE'),
    ('num', 'mul', 'P', 's'),
    ('den', 'mul', 'Q', 's'),
]
# neg layout, multiply (search: 4, shortest; den = Q). 2 + 4 = 6
MUL_NEG_POST = RAW_MUL + [
    ('m', 'and', 'd1', 'd2'),
    ('m', 'sar', 'm', 'top'),
    ('s', 'or', 'm', 'ONE'),
    ('num', 'mul', 'P', 's'),
]                                     # den = Q as it is

# shared post-correction with the operation's mask k, neg layout: 6
#   m = (d1 AND d2) sar (n-1)   add: mask of a;  mul: mask of a1 a2
#   num = P * (m OR 1);  den = Q * ((m AND NOT k) OR 1)
SHARED_K = [
    ('m', 'and', 'd1', 'd2'),
    ('m', 'sar', 'm', 'top'),
    ('s', 'or', 'm', 'ONE'),
    ('num', 'mul', 'P', 's'),
    ('t', 'andn', 'm', 'k'),
    ('t', 'or', 't', 'ONE'),
    ('den', 'mul', 'Q', 't'),
]


# the same with the operation given as k = -1 (add) / +1 (multiply): 6
#   num = P * s;  den = Q * (s AND k),   s = ((d1 AND d2) sar (n-1)) OR 1
SHARED_K2 = [
    ('m', 'and', 'd1', 'd2'),
    ('m', 'sar', 'm', 'top'),
    ('s', 'or', 'm', 'ONE'),
    ('num', 'mul', 'P', 's'),
    ('t', 'and', 's', 'k'),
    ('den', 'mul', 'Q', 't'),
]


def check_shared(prog, layout, ns=(3, 4, 5, 6, 64), kadd=0, kmul=-1, name='shared fix with op mask k'):
    res = []; ok = True
    for n in ns:
        w = W(n); tot = 0; bad = 0
        for op, raw, k in (('add', RAW_ADD, U0), ('mul', RAW_MUL, None)):
            cnt, env, en, ed, alt, zero = data(op, n, layout)
            env = dict(env); env['k'] = np.full(cnt, (kadd if op == 'add' else kmul) & int(w.F), dtype=np.uint64)
            r = run(raw + prog, env, w)
            b = (r['num'] != en) | ((r['den'] != ed) & ((r['den'] != alt) | ~zero))
            bad += int(b.sum()); tot += cnt
        ok &= bad == 0
        res.append(f"n={n}: {tot} cases, {bad} fail")
    print(f"{'OK  ' if ok else 'FAIL'} {name:34s} {len(prog):2d} instr | " + "; ".join(res))


U0 = 0

if __name__ == '__main__':
    report('add off, post fix (search)', ADD_OFF_POST, 'add')
    report('add off, post fix, no mul', ADD_OFF_POST_NOMUL, 'add')
    report('mul off, den fix (search)', MUL_OFF_DEN, 'mul')
    report('add neg, post fix (search)', ADD_NEG_POST, 'add', layout='neg')
    report('mul neg, post fix (search)', MUL_NEG_POST, 'mul', layout='neg', out=('num', 'Q'))
    check_shared(SHARED_K, 'neg')
    check_shared(SHARED_K2, 'neg', kadd=-1, kmul=1, name='shared fix, k = -1 add / +1 mul')
