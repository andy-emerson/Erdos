"""m1.py: milestone 1, the current corrections (fixes.py) as instruction sequences, counted and checked.

  python3 -I m1.py        # n = 3..6 exhaustive, n = 64 random
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from rat import report

# raw machine arithmetic, no correction (should fail whenever an axis is 1)
ADD_RAW = [
    ('p', 'mul', 'n1', 'd2'),
    ('q', 'mul', 'n2', 'd1'),
    ('num', 'add', 'p', 'q'),
    ('den', 'mul', 'd1', 'd2'),
]
MUL_RAW = [
    ('num', 'mul', 'n1', 'n2'),
    ('den', 'mul', 'd1', 'd2'),
]

# ---- add, fixes.py taken literally ----
# num: flip top bit by a*((n1 xor n2) and 1); den: flip top bit by a*(1 xor ((d1 xor d2) and 1))
ADD_LITERAL = ADD_RAW[:3] + [
    ('pa', 'xor', 'n1', 'n2'),
    ('pa', 'and', 'pa', 'ONE'),        # parity of n1 + n2
    ('a', 'shr', 'd1', 'top'),         # axis flag
    ('pa', 'and', 'pa', 'a'),
    ('pa', 'shl', 'pa', 'top'),
    ('num', 'xor', 'num0', 'pa'),
] + [
    ('den0', 'mul', 'd1', 'd2'),
    ('pd', 'xor', 'd1', 'd2'),         # top bits cancel (same axis): low bit = parity of D1 + D2
    ('pd', 'not', 'pd'),               # 1 when even
    ('pd', 'and', 'pd', 'ONE'),
    ('pd', 'and', 'pd', 'a'),
    ('pd', 'shl', 'pd', 'top'),
    ('den', 'xor', 'den0', 'pd'),
]
ADD_LITERAL[2] = ('num0', 'add', 'p', 'q')

# ---- add, the same corrections written tightly ----
#   num = (n1*d2 + n2*d1) XOR (((n1 XOR n2) shl (n-1)) AND d1)       raw 3 + fix 4
#   den = den0 XOR ((den0 XOR d1) AND T)  (set top bit to a)          raw 1 + fix 3
ADD_CUR = [
    ('p', 'mul', 'n1', 'd2'),
    ('q', 'mul', 'n2', 'd1'),
    ('num0', 'add', 'p', 'q'),
    ('f', 'xor', 'n1', 'n2'),
    ('f', 'shl', 'f', 'top'),          # T * parity(n1 + n2)
    ('f', 'and', 'f', 'd1'),           # ... kept when a = 1
    ('num', 'xor', 'num0', 'f'),
    ('den0', 'mul', 'd1', 'd2'),
    ('e', 'xor', 'den0', 'd1'),
    ('e', 'and', 'e', 'T'),
    ('den', 'xor', 'den0', 'e'),
]

# ---- multiply, fixes.py taken literally ----
# num: negate when a1 and a2; den: flip top bit by (a1 and D2 even) xor (a2 and D1 even)
MUL_LITERAL = [
    ('num0', 'mul', 'n1', 'n2'),
    ('m', 'and', 'd1', 'd2'),
    ('m', 'sar', 'm', 'top'),          # mask of a1 AND a2
    ('x', 'xor', 'num0', 'm'),
    ('num', 'sub', 'x', 'm'),
    ('den0', 'mul', 'd1', 'd2'),
    ('f1', 'not', 'd2'),
    ('f1', 'shl', 'f1', 'top'),        # T * (D2 even)
    ('f1', 'and', 'f1', 'd1'),         # T * (a1 and D2 even)
    ('f2', 'not', 'd1'),
    ('f2', 'shl', 'f2', 'top'),
    ('f2', 'and', 'f2', 'd2'),         # T * (a2 and D1 even)
    ('f', 'xor', 'f1', 'f2'),
    ('den', 'xor', 'den0', 'f'),
]

# ---- multiply, the same corrections written tightly ----
#   num = ((n1*n2) XOR m) - m,  m = (d1 AND d2) sar (n-1)            raw 1 + fix 4
#   den = den0 XOR ((den0 XOR d1 XOR d2) AND T)  (top bit := a1 XOR a2)  raw 1 + fix 4
MUL_CUR = [
    ('num0', 'mul', 'n1', 'n2'),
    ('m', 'and', 'd1', 'd2'),
    ('m', 'sar', 'm', 'top'),
    ('x', 'xor', 'num0', 'm'),
    ('num', 'sub', 'x', 'm'),
    ('den0', 'mul', 'd1', 'd2'),
    ('e', 'xor', 'd1', 'd2'),
    ('e', 'xor', 'e', 'den0'),
    ('e', 'and', 'e', 'T'),
    ('den', 'xor', 'den0', 'e'),
]

if __name__ == '__main__':
    report('add raw (no fix)', ADD_RAW, 'add')
    report('add, fixes.py literal', ADD_LITERAL, 'add')
    report('add, current tight', ADD_CUR, 'add')
    report('mul raw (no fix)', MUL_RAW, 'mul')
    report('mul, fixes.py literal', MUL_LITERAL, 'mul')
    report('mul, current tight', MUL_CUR, 'mul')
    report('add, current tight, strict zero', ADD_CUR, 'add', zero_any_axis=False)
    report('mul, current tight, strict zero', MUL_CUR, 'mul', zero_any_axis=False)
