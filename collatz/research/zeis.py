"""Symmetric Collatz-type rules on the Eisenstein integers Z[w], w = exp(2 pi i/3).

z = a + b w is stored as (a, b); w^2 = -1 - w; N(a + b w) = a^2 - a b + b^2.
2 is inert (norm 4, residue field F_4), the six units are +-1, +-w, +-w^2, and a rule at
level k has digits for the odd classes mod 2^k (z odd = 2 does not divide z = a, b not both even):
    S(z) = (alpha z + d(z)) / 2^v,  2^k | alpha z + d(z),  2^v the full power of 2.
Symmetric rules exist exactly from k = 2 (integer-census.md, Theorem 1).
Since v - k has P(v - k >= j) = 4^(-j), the mean of v is k + 1/3 and the Collatz-like
window is 4^k < N(alpha) < 4^(k + 1/3).

Usage: python3 zeis.py info k            minimal digit sets and Collatz-like multipliers
       python3 zeis.py census k B        cycles up to units over N(z) <= B, every rule at level k
       python3 zeis.py drop k a b B      every odd z with 1 < N(z) <= B drops below N(z)
"""
import sys, math

UNITS = [(1, 0), (0, 1), (-1, -1), (-1, 0), (0, -1), (1, 1)]
def mul(x, y):
    a, b = x; c, d = y
    return (a * c - b * d, a * d + b * c - b * d)
def add(x, y): return (x[0] + y[0], x[1] + y[1])
def N(x): a, b = x; return a * a - a * b + b * b
def odd(x): return (x[0] | x[1]) & 1
def conj(x): a, b = x; return (a - b, -b)   # a + b w^2
def canon(x):
    """The associate of x != 0 in the sector 0 <= arg < 60 degrees, that is 0 <= b < a."""
    for u in UNITS:
        a, b = mul(u, x)
        if 0 <= b < a: return (a, b)
    raise ValueError(x)

def min_digit_sets(k):
    """For each odd class mod 2^k the elements of least norm; orbits under units."""
    q = 2 ** k; R = q + 2
    best = {}
    for a in range(-R, R + 1):
        for b in range(-R, R + 1):
            z = (a, b)
            if not odd(z): continue
            c = (a % q, b % q); n = N(z)
            if c not in best or n < best[c][0]: best[c] = (n, [z])
            elif n == best[c][0]: best[c][1].append(z)
    assert len(best) == q * q - (q // 2) ** 2
    seen, orbits = set(), []
    for c in sorted(best):
        if c in seen: continue
        orb = {((u[0] * c[0] - u[1] * c[1]) % q, (u[0] * c[1] + u[1] * c[0] - u[1] * c[1]) % q) for u in UNITS}
        assert len(orb) == 6, 'units do not act freely'
        seen |= orb; orbits.append((c, best[c]))
    return best, orbits

def multipliers(k, hi_exp=1 / 3):
    lo, hi = 4 ** k, 4 ** (k + hi_exp)
    R = int(math.isqrt(int(2 * hi))) + 3
    found = set()
    for a in range(-R, R + 1):
        for b in range(-R, R + 1):
            z = (a, b)
            if odd(z) and lo < N(z) < hi: found.add(canon(z))
    return sorted(found, key=lambda z: (N(z), z))

def digit_table(k, D):
    q = 2 ** k
    return {(d[0] % q, d[1] % q): d for d in D}

def make_S(alpha, k, table):
    q = 2 ** k
    def S(z):
        w = mul(alpha, z)
        d = table[((-w[0]) % q, (-w[1]) % q)]
        a, b = add(w, d)
        while not ((a | b) & 1): a >>= 1; b >>= 1
        return (a, b)
    return S

def census(alpha, k, D, B, cap=20000, esc=2 ** 200):
    S = make_S(alpha, k, digit_table(k, D))
    res, cycles, unres = {}, {}, 0
    R = math.isqrt(4 * B // 3) + 1
    starts = {canon((a, b)) for a in range(-R, R + 1) for b in range(-R, R + 1)
              if odd((a, b)) and N((a, b)) <= B}
    for s in sorted(starts, key=N):
        if s in res: continue
        path, pos, z = [], {}, s
        while True:
            c = canon(z)
            if c in res: cid = res[c]; break
            if c in pos:
                cyc = path[pos[c]:]; cid = min(cyc, key=lambda t: (N(t), t))
                cycles[cid] = len(cyc); break
            if len(path) > cap or N(z) > esc: cid = None; break
            pos[c] = len(path); path.append(c); z = S(z)
        if cid is None: unres += 1
        for c in path:
            if N(c) <= B: res[c] = cid
    return cycles, unres

def drop(alpha, k, D, B, cap=10 ** 5):
    S = make_S(alpha, k, digit_table(k, D))
    R = math.isqrt(4 * B // 3) + 1; worst = 0
    for a in range(1, R + 1):            # one associate of each class suffices (symmetry):
        for b in range(0, a):            # the sector 0 <= b < a
            z = (a, b)
            if not odd(z) or N(z) > B or N(z) == 1: continue
            n0, y, n = N(z), z, 0
            while N(y) >= n0:
                y = S(y); n += 1
                if n > cap: return ('FAIL', z)
            worst = max(worst, n)
    return ('ok', worst)

if __name__ == '__main__':
    mode, k = sys.argv[1], int(sys.argv[2])
    best, orbits = min_digit_sets(k)
    nsets = 1
    for c, (n, els) in orbits: nsets *= len(els)
    if mode == 'info':
        print(f'level {k}: {len(best)} odd classes, {len(orbits)} orbits of 6, {nsets} minimal symmetric digit set(s)')
        for c, (n, els) in orbits: print('  orbit of class', c, 'least norm', n, 'elements', els)
        for e, name in ((1 / 3, 'Collatz-like 4^k < N < 4^(k+1/3)'), (1, 'naive 4^k < N < 4^(k+1)')):
            ms = multipliers(k, e)
            print(f'  {name}: {len(ms)} multipliers up to units, norms {sorted({N(m) for m in ms})}')
        sys.exit()
    # build the minimal symmetric digit set(s): unit multiples of a least-norm element per orbit
    import itertools
    choices = [els for c, (n, els) in orbits]
    Ds = [[mul(u, r) for r in pick for u in UNITS] for pick in itertools.product(*choices)]
    if mode == 'census':
        B = int(sys.argv[3])
        for alpha in multipliers(k):
            for i, D in enumerate(Ds):
                cyc, unres = census(alpha, k, D, B)
                drift = math.log(N(alpha)) - (k + 1 / 3) * math.log(4)
                ls = sorted(cyc.items(), key=lambda t: (N(t[0]), t[0]))
                print(f'k={k} alpha={alpha} N={N(alpha)} conj={canon(conj(alpha))} D#{i} drift={drift:+.4f} '
                      f'B={B} cycles={len(cyc)} unresolved={unres} ' + ' '.join(f'{c}:{l}' for c, l in ls[:12]), flush=True)
    elif mode == 'drop':
        alpha = (int(sys.argv[3]), int(sys.argv[4])); B = int(sys.argv[5])
        for i, D in enumerate(Ds): print(f'k={k} alpha={alpha} D#{i} drop B={B}:', drop(alpha, k, D, B), flush=True)
