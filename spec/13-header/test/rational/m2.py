"""m2.py: milestone 2, better formulas by reasoning, counted and checked.

Two denominator layouts (top bit = axis a in both):
  offset (the design):  d = D + T*a
  neg (proposed):       d = (-1)^a * D, two's complement (1 <= D < T, so the top bit is a)

  python3 -I m2.py        # n = 3..6 exhaustive, n = 64 random 
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from rat import report
from m1 import ADD_CUR, MUL_CUR

# ================= offset layout: d = D + T*a =================

# add: strip the axis from both denominators, plain rational add, put the axis back. 8
ADD_OFF = [
    ('D1', 'and', 'd1', 'NT'),
    ('D2', 'and', 'd2', 'NT'),
    ('A', 'xor', 'd1', 'D1'),          # T*a
    ('p', 'mul', 'n1', 'D2'),
    ('q', 'mul', 'n2', 'D1'),
    ('num', 'add', 'p', 'q'),
    ('e', 'mul', 'D1', 'D2'),
    ('den', 'or', 'e', 'A'),
]
# the same, axis by AND and the axis put back by add (ARM64: madd den, D1, D2, A)
ADD_OFF_B = [
    ('D1', 'and', 'd1', 'NT'),
    ('D2', 'and', 'd2', 'NT'),
    ('A', 'and', 'd1', 'T'),
    ('p', 'mul', 'n1', 'D2'),
    ('q', 'mul', 'n2', 'D1'),
    ('num', 'add', 'p', 'q'),
    ('e', 'mul', 'D1', 'D2'),
    ('den', 'add', 'e', 'A'),
]
SUB_OFF = [i if i[0] != 'num' else ('num', 'sub', 'p', 'q') for i in ADD_OFF]

# negate and conjugate (either layout)
NEG = [('num', 'neg', 'n1')]                          # den unchanged
CONJ = [
    ('m', 'sar', 'd1', 'top'),
    ('x', 'xor', 'n1', 'm'),
    ('num', 'sub', 'x', 'm'),
]

# division, offset layout. q = n1 D2 / (D1 |n2|) on axis a1 XOR a2,
# numerator sign sigma(n2) * (-1)^(a2 and not a1)  (i^-1 = -i).  15
DIV_OFF = [
    ('D1', 'and', 'd1', 'NT'),
    ('D2', 'and', 'd2', 'NT'),
    ('m2', 'sar', 'n2', 'top'),
    ('x', 'xor', 'n2', 'm2'),
    ('N2', 'sub', 'x', 'm2'),          # |n2|
    ('e', 'mul', 'D1', 'N2'),
    ('ax', 'xor', 'd1', 'd2'),
    ('ax', 'and', 'ax', 'T'),
    ('den', 'or', 'e', 'ax'),
    ('p', 'mul', 'n1', 'D2'),
    ('v', 'andn', 'd2', 'd1'),         # top bit: a2 and not a1
    ('v', 'xor', 'v', 'n2'),
    ('v', 'sar', 'v', 'top'),
    ('y', 'xor', 'p', 'v'),
    ('num', 'sub', 'y', 'v'),
]

# ================= neg layout: d = (-1)^a * D =================

# multiply: the denominator product is exact; only i*i = -1 remains. 6
MUL_NEG = [
    ('num0', 'mul', 'n1', 'n2'),
    ('m', 'and', 'd1', 'd2'),
    ('m', 'sar', 'm', 'top'),          # mask of a1 AND a2
    ('x', 'xor', 'num0', 'm'),
    ('num', 'sub', 'x', 'm'),
    ('den', 'mul', 'd1', 'd2'),
]

# add: negate operand 2 as a pair (n2, d2) -> s*(n2, d2), s = (-1)^a: same fraction, now d2' = D2.
# Then the raw add is exact: num = n1 D2 + (s n2)(s D1), den = d1 D2 = s D1 D2.   8
ADD_NEG = [
    ('m', 'sar', 'd2', 'top'),
    ('s', 'or', 'm', 'ONE'),           # s = +1 / -1
    ('n2s', 'mul', 'n2', 's'),
    ('D2', 'mul', 'd2', 's'),          # |d2|
    ('p', 'mul', 'n1', 'D2'),
    ('q', 'mul', 'n2s', 'd1'),
    ('num', 'add', 'p', 'q'),
    ('den', 'mul', 'd1', 'D2'),
]
# the same with the conditional negate by mask (xor; sub) in place of the multiply by s. 9
ADD_NEG_X = [
    ('m', 'sar', 'd2', 'top'),
    ('x', 'xor', 'n2', 'm'),
    ('n2s', 'sub', 'x', 'm'),
    ('y', 'xor', 'd2', 'm'),
    ('D2', 'sub', 'y', 'm'),
    ('p', 'mul', 'n1', 'D2'),
    ('q', 'mul', 'n2s', 'd1'),
    ('num', 'add', 'p', 'q'),
    ('den', 'mul', 'd1', 'D2'),
]
# raw add, then negate both result words by the axis mask. 9
ADD_NEG_POST = [
    ('p', 'mul', 'n1', 'd2'),
    ('q', 'mul', 'n2', 'd1'),
    ('r', 'add', 'p', 'q'),
    ('e', 'mul', 'd1', 'd2'),
    ('m', 'sar', 'd1', 'top'),
    ('x', 'xor', 'r', 'm'),
    ('num', 'sub', 'x', 'm'),
    ('y', 'xor', 'e', 'm'),
    ('den', 'sub', 'y', 'm'),
]
SUB_NEG = [i if i[0] != 'num' else ('num', 'sub', 'p', 'q') for i in ADD_NEG]

# division, neg layout.  p = n1 d2, q = d1 n2;
#   den = q * sigma(n2) s2            mask mu = (d2 XOR n2) sar (n-1)
#   num = p * sigma(n2) (-1)^(a1 a2)  mask nu = ((d1 AND d2) XOR n2) sar (n-1)     11
DIV_NEG = [
    ('p', 'mul', 'n1', 'd2'),
    ('q', 'mul', 'd1', 'n2'),
    ('mu', 'xor', 'd2', 'n2'),
    ('mu', 'sar', 'mu', 'top'),
    ('y', 'xor', 'q', 'mu'),
    ('den', 'sub', 'y', 'mu'),
    ('nu', 'and', 'd1', 'd2'),
    ('nu', 'xor', 'nu', 'n2'),
    ('nu', 'sar', 'nu', 'top'),
    ('x', 'xor', 'p', 'nu'),
    ('num', 'sub', 'x', 'nu'),
]

# ---- one shared correction, neg layout: m = (d1 AND d2) sar (n-1), negate operand 2 by m ----
# multiply negates n2 only; add (same axis, so m is the mask of a) negates n2 and d2.
SHARED = [
    ('m', 'and', 'd1', 'd2'),
    ('m', 'sar', 'm', 'top'),
    ('s', 'or', 'm', 'ONE'),
    ('n2s', 'mul', 'n2', 's'),
]
MUL_SHARED = SHARED + [
    ('num', 'mul', 'n1', 'n2s'),
    ('den', 'mul', 'd1', 'd2'),
]                                                   # 6
ADD_SHARED = SHARED + [
    ('D2', 'mul', 'd2', 's'),
    ('p', 'mul', 'n1', 'D2'),
    ('q', 'mul', 'n2s', 'd1'),
    ('num', 'add', 'p', 'q'),
    ('den', 'mul', 'd1', 'D2'),
]                                                   # 9 (8 with m = d2 sar (n-1), ADD_NEG)

if __name__ == '__main__':
    print('--- offset layout (d = D + T a)')
    report('add, current (m1)', ADD_CUR, 'add')
    report('add, strip both', ADD_OFF, 'add')
    report('add, strip both, and/add form', ADD_OFF_B, 'add')
    report('sub, strip both', SUB_OFF, 'sub')
    report('mul, current (m1)', MUL_CUR, 'mul')
    report('neg', NEG, 'neg', out=('num', 'd1'))
    report('conj', CONJ, 'conj', out=('num', 'd1'))
    report('div', DIV_OFF, 'div', ns=(3, 4, 5, 6, 64))
    print('--- neg layout (d = (-1)^a D)')
    report('mul', MUL_NEG, 'mul', layout='neg')
    report('add, pre-negate pair by mul', ADD_NEG, 'add', layout='neg')
    report('add, pre-negate pair by mask', ADD_NEG_X, 'add', layout='neg')
    report('add, post-negate pair', ADD_NEG_POST, 'add', layout='neg')
    report('sub, pre-negate pair by mul', SUB_NEG, 'sub', layout='neg')
    report('neg', NEG, 'neg', out=('num', 'd1'), layout='neg')
    report('conj', CONJ, 'conj', out=('num', 'd1'), layout='neg')
    report('div', DIV_NEG, 'div', ns=(3, 4, 5, 6, 64), layout='neg')
    report('shared prefix: mul', MUL_SHARED, 'mul', layout='neg')
    report('shared prefix: add', ADD_SHARED, 'add', layout='neg')
    print('--- strict zero (axis checked on zero results)')
    for nm, pr, op, lay in (('add off', ADD_OFF, 'add', 'offset'), ('add neg', ADD_NEG, 'add', 'neg'),
                            ('mul neg', MUL_NEG, 'mul', 'neg'), ('sub neg', SUB_NEG, 'sub', 'neg'),
                            ('div off', DIV_OFF, 'div', 'offset'), ('div neg', DIV_NEG, 'div', 'neg')):
        report(nm + ' strict', pr, op, layout=lay, zero_any_axis=False,
               ns=(3, 4, 5, 6, 64) if op == 'div' else (3, 4, 5, 6, 64))
