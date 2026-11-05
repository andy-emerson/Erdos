"""Symmetric integer rules at level 2 (paper draft 2, Section 3.6).

Odd x -> (m x + d) / 2^v with m in {5, 7}, d in {+1, -1} chosen so that 4 | m x + d,
and 2^v the full power of 2. These rules commute with x -> -x.
1. Cycles up to sign reached from every odd x with 1 <= x < 10^5, for m = 5 and m = 7.
2. For m = 7: every odd x with 3 <= x <= 10^7 reaches |y| < x (drop below start); with
   symmetry under x -> -x this gives: every odd x with |x| <= 10^7 reaches +-1.
Also prints the cycles of 3x+1 on the negative integers.
Usage: python3 zsym.py   (about 15 s)
"""

def S(m, x):
    d = 1 if (-m * x) % 4 == 1 else -1
    y = m * x + d
    while y % 2 == 0:
        y //= 2
    return y

def cycle(f, x, cap=10**5):
    seen, seq = {}, []
    while x not in seen and len(seq) < cap:
        seen[x] = len(seq); seq.append(x); x = f(x)
    return None if x not in seen else seq[seen[x]:]

def T3(x): return x // 2 if x % 2 == 0 else 3 * x + 1

neg = {}
for x in range(-10**5, 0):
    c = cycle(T3, x); neg[max(c)] = len(c)
print('3x+1 on negatives: cycles (largest element: length in steps)', neg)

for m in (5, 7):
    cyc, unres = {}, 0
    for x in range(1, 10**5, 2):
        c = cycle(lambda t: S(m, t), x, 10**4)
        if c is None: unres += 1; continue
        cyc[min(abs(t) for t in c)] = len(c)
    print(f'{m}x+-1: cycles up to sign (least |element|: odd steps)', sorted(cyc.items()), 'unresolved', unres)

B = 10**7; worst = 0
for x in range(3, B + 1, 2):
    y, n = x, 0
    while abs(y) >= x:
        y = S(7, y); n += 1
        assert n < 10**5, x
    worst = max(worst, n)
print(f'7x+-1: every odd x in [3, {B}] drops below |x|; max odd steps {worst}; S(1) = {S(7, 1)}')
