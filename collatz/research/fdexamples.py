"""fdexamples.py: independent (pure Python) check of the example maps of fundamental-domain.md.
For each example: full classification of every odd z in F with N(z) <= B (every start reaches the
representative of [1], and [1] is periodic), the cycle of [1], and the orbit of a sample start.
Output: logs/fdexamples.txt
"""
U = [(1, 0), (0, 1), (-1, 0), (0, -1)]
def mul(x, y): return (x[0]*y[0]-x[1]*y[1], x[0]*y[1]+x[1]*y[0])
def add(x, y): return (x[0]+y[0], x[1]+y[1])
def N(x): return x[0]**2+x[1]**2
def gpi_odd(w):
    a, b = w; v = 0
    while (a+b) % 2 == 0:
        a, b = (a+b)//2, (b-a)//2; v += 1
    return (a, b), v
def inF(F, z):
    a, b = z
    if F == 'Q': return a > 0 and b >= 0
    if F == 'R': return a >= 0 and b > 0
    if F == 'C': return a > 0 and -a < b <= a
    if F == 'P': return (a % 4, b % 4) in ((1, 0), (3, 2))
def rep(F, z):
    for u in U:
        w = mul(u, z)
        if inF(F, w): return w
    raise ValueError(z)
def cls4(z): return (z[0] % 4, z[1] % 4)

def fmt(z): return f"{z[0]}{z[1]:+d}i"

def make(alpha, k, digs):
    """digs: dict class-representative -> digit, classes mod pi^k given by a representative"""
    from fdrun import groups, OC
    t = {}
    for g in groups(k):
        for r, d in digs.items():
            if r in g:
                for c in g: t[c] = d
    return t

def S(F, alpha, t, z):
    w = add(mul(alpha, z), t[cls4(z)])
    o, v = gpi_odd(w)
    return rep(F, o), v

def classify(F, alpha, t, B, cap=10000):
    one = rep(F, (1, 0))
    cyc = [one]; x = S(F, alpha, t, one)[0]
    while x != one and len(cyc) < 100:
        cyc.append(x); x = S(F, alpha, t, x)[0]
    if x != one: return None, "1 not periodic"
    good = set(cyc)
    r = int(B ** 0.5) + 1
    starts = sorted([(a, b) for a in range(-r, r + 1) for b in range(-r, r + 1)
                     if (a + b) % 2 and a * a + b * b <= B and inF(F, (a, b))], key=N)
    maxlen = 0
    for z in starts:
        path = []; x = z; seen = set()
        while x not in good:
            if x in seen or len(path) > cap: return cyc, f"fails at {fmt(z)}"
            seen.add(x); path.append(x); x = S(F, alpha, t, x)[0]
        good.update(path); maxlen = max(maxlen, len(path))
    return cyc, f"pass, {len(starts)} starts, longest {maxlen} odd steps"

def orbit(F, alpha, t, z, one):
    out = [z]
    while z != one and len(out) < 200:
        z, v = S(F, alpha, t, z); out.append(z)
    return out

EX = [
    ("cone, the 2+i map (smallest digits)", 'C', (2, 1), 2, {(0, 1): (-1, 0), (1, 0): (0, -1)}),
    ("cone, mirror: 2-i", 'C', (2, -1), 2, {(0, 1): (-1, 0), (1, 0): (0, 1)}),
    ("quadrant, 2+i, digits -1-2i, -i", 'Q', (2, 1), 2, {(0, 1): (-1, -2), (1, 0): (0, -1)}),
    ("quadrant, 2-i, digits 1, 2+i", 'Q', (2, -1), 2, {(0, 1): (1, 0), (1, 0): (2, 1)}),
    ("primary, 2+i, digit i (effective modulus pi^3)", 'P', (2, 1), 2, {(1, 0): (0, 1)}),
    ("quadrant, 3, first by digit norm", 'Q', (3, 0), 3, {(0, 1): (0, 1), (0, 3): (-2, 1), (1, 0): (-1, 2), (1, 2): (-1, 0)}),
]

lines = []
for name, F, alpha, k, digs in EX:
    t = make(alpha, k, digs)
    cyc, res = classify(F, alpha, t, 20000)
    one = rep(F, (1, 0))
    lines.append(f"{name}: F={F} alpha={fmt(alpha)} k={k} digits " +
                 ", ".join(f"[{fmt(c)}]->{fmt(d)}" for c, d in digs.items()))
    lines.append(f"  N <= 20000: {res}; cycle of [1]: {' -> '.join(map(fmt, cyc or []))}")
    for z0 in ((7, 4), (11, 6), (27, 10)):
        z = rep(F, z0)
        o = orbit(F, alpha, t, z, one)
        lines.append(f"  orbit of [{fmt(z0)}]: " + " -> ".join(f"{fmt(x)}({N(x)})" for x in o))
open('/home/claude/erdos/collatz/research/logs/fdexamples.txt', 'w').write("\n".join(lines) + "\n")
print("\n".join(lines))
