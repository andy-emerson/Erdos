# alpha = 2-i, modulus (1+i)^2: where do orbits enter the trivial cycle 1 -> -i -> -1 -> 1?
from collections import Counter

def mul(x, y): return (x[0]*y[0] - x[1]*y[1], x[0]*y[1] + x[1]*y[0])
def odd(z): return (z[0] + z[1]) % 2 == 1
def divpi(z): return ((z[0] + z[1]) // 2, (z[1] - z[0]) // 2)

A = (2, -1)

def S(z):
    w = mul(A, z)
    d = (0, 1) if z[0] % 2 == 1 else (1, 0)   # digit i when a is odd, 1 when b is odd
    w = (w[0] + d[0], w[1] + d[1])
    while not odd(w):
        w = divpi(w)
    return w

C = {(1, 0), (0, -1), (-1, 0)}
I = (0, 1)
entry = Counter()
via_i = 0
total = 0
for a in range(-300, 301):
    for b in range(-300, 301):
        z = (a, b)
        if not odd(z) or a*a + b*b > 90000 or z in C or z == I:
            continue
        total += 1
        seen_i = False
        while z not in C:
            if z == I:
                seen_i = True
            z = S(z)
        entry[z] += 1
        via_i += seen_i
print('odd starts with N <= 90000 (excluding the units):', total)
print('first cycle element reached:', dict(entry))
print('orbits passing through i:', via_i)
