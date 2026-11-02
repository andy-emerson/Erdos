"""collide.py: can ONE post-correction program fix both add and multiply?

A program sees only (n1, d1, n2, d2, P, Q), P/Q = the raw products of whichever operation ran.
If an add lane and a multiply lane have identical inputs but different required outputs, no program
of any length (without being told the operation) can fix both. This finds such collisions.

  python3 -I collide.py     # n = 3..8, both layouts
"""
import sys

def lanes(n, layout, op):
    T = 1 << (n - 1); F = (1 << n) - 1
    dw = (lambda D, a: (D + T * a) & F) if layout == 'off' else (lambda D, a: (-D if a else D) & F)
    for D1 in range(1, T):
        for D2 in range(1, T):
            if D1 * D2 >= T: continue
            for a1 in (0, 1):
                for a2 in (0, 1):
                    if op == 'add' and a1 != a2: continue
                    w1, w2 = dw(D1, a1), dw(D2, a2)
                    for n1 in range(-T, T):
                        for n2 in range(-T, T):
                            if op == 'add':
                                t, ta, P = n1 * D2 + n2 * D1, a1, n1 * w2 + n2 * w1
                            else:
                                t, ta, P = n1 * n2 * (-1 if a1 & a2 else 1), a1 ^ a2, n1 * n2
                            if not -T <= t < T: continue
                            key = (n1 & F, w1, n2 & F, w2, P & F, (w1 * w2) & F)
                            dens = {dw(D1 * D2, ta)} if t else {dw(D1 * D2, 0), dw(D1 * D2, 1)}
                            yield key, t & F, dens, (n1, D1, a1, n2, D2, a2)

for layout in ('off', 'neg'):
    for n in range(3, 9):
        add = {}
        for k, tn, dens, src in lanes(n, layout, 'add'):
            add[k] = (tn, dens, src)
        found = None; count = 0
        for k, tn, dens, src in lanes(n, layout, 'mul'):
            if k in add:
                atn, adens, asrc = add[k]
                if atn != tn or not (adens & dens):
                    count += 1
                    if found is None: found = (asrc, src, atn, tn, adens, dens)
        print(f"{layout} n={n}: {count} colliding lanes" + (f"; e.g. add {found[0]} vs mul {found[1]} (n1,D1,a1,n2,D2,a2)" if found else ""))
