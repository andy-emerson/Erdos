"""Internal check (symmetry-test.md, Section 9): Eisenstein rules at levels 2 and 3 that satisfy the
trivial-cycle criterion, literal cycles, and whether their trivial cycle moves between units.
Window 4^k < N(alpha) < 4^(k+1/3) (integer-census.md, 2.4); level 1 has no multiplier.
usage: eistest.py min k B [per]        every alpha (all six associates) x minimal digit sets
                                        (per > 0: at most `per` random tie-breaks per alpha)
       eistest.py nonmin k B n seed maxnorm   random non-minimal digit sets, random alpha
"""
import sys, random, itertools
import zeis as Z

def alphas(k):
    out = set()
    for a in Z.multipliers(k):
        for u in Z.UNITS: out.add(Z.mul(u, a))
    return sorted(out, key=lambda z: (Z.N(z), z))

def make_S(alpha, dig, k):
    q = 2 ** k
    def S(z):
        x = Z.mul(alpha, z); c = ((-x[0]) % q, (-x[1]) % q)
        a, b = Z.add(x, dig[c])
        while a % 2 == 0 and b % 2 == 0: a //= 2; b //= 2
        return (a, b)
    return S

def census(S, B, cap=20000):
    R = int(B ** 0.5) + 2
    st = {}; cycles = []; unres = 0
    for a in range(-R, R + 1):
        for b in range(-R, R + 1):
            z = (a, b)
            if not Z.odd(z) or Z.N(z) > B or z in st: continue
            path = []; seen = {}; res = None; w = z
            while True:
                if w in st: res = st[w]; break
                if w in seen:
                    cyc = path[seen[w]:]; res = len(cycles); cycles.append(cyc); break
                seen[w] = len(path); path.append(w)
                if len(path) > cap or abs(w[0]) + abs(w[1]) > 10 ** 30: res = -1; break
                w = S(w)
            if res == -1: unres += 1
            for p in path: st[p] = res
    return cycles, unres

def test(alpha, dig, k, B):
    S = make_S(alpha, dig, k)
    cyc, unres = census(S, B)
    triv = [c for c in cyc if any(Z.N(w) == 1 for w in c)]
    ok = unres == 0 and len(triv) == len(cyc)
    moving = [c for c in triv if sum(1 for w in c if Z.N(w) == 1) >= 2]
    return ok, cyc, triv, moving, unres

def classes_least(k):
    best, orbits = Z.min_digit_sets(k)
    return sorted(best.items())

def is_sym(dig, k):
    q = 2 ** k
    for c, d in dig.items():
        for u in Z.UNITS:
            uc = Z.mul(u, c); uc = (uc[0] % q, uc[1] % q)
            if dig[uc] != Z.mul(u, d): return False
    return True

def show(alpha, dig, k, ok, cyc, triv, moving, unres, extra=''):
    print(f"k={k} alpha={alpha} N={Z.N(alpha)} sym={is_sym(dig, k)} unres={unres} cycles={len(cyc)} trivial={len(triv)} "
          f"CRIT={'YES' if ok else 'no'} moving_unit_cycle={bool(moving)} "
          f"triv={[sorted(w for w in c if Z.N(w) == 1) for c in triv]} other_mins={[min(c, key=Z.N) for c in cyc if c not in triv][:5]}{extra}", flush=True)

def run_min(k, B, per, seed=1):
    rnd = random.Random(seed)
    L = classes_least(k)
    choices = list(itertools.product(*[v[1] for c, v in L]))
    print(f"# Eisenstein level {k}: {len(alphas(k))} multipliers (all associates), {len(choices)} minimal digit sets, B={B}")
    n = nok = nmov = 0
    for alpha in alphas(k):
        ch = choices if not per or per >= len(choices) else rnd.sample(choices, per)
        for c in ch:
            dig = {cl: d for (cl, _), d in zip(L, c)}
            r = test(alpha, dig, k, B); n += 1; nok += r[0]; nmov += r[0] and bool(r[3])
            show(alpha, dig, k, *r)
    print(f"# level {k}: rules {n}, criterion {nok}, of which with a trivial cycle through two or more units {nmov}")

def run_nonmin(k, B, n, seed, maxnorm):
    rnd = random.Random(seed)
    q = 2 ** k
    L = classes_least(k)
    cand = {}
    for a in range(-12, 13):
        for b in range(-12, 13):
            z = (a, b)
            if Z.odd(z) and Z.N(z) <= maxnorm: cand.setdefault((a % q, b % q), []).append(z)
    A = alphas(k); nok = nmov = 0
    print(f"# Eisenstein level {k}: {n} random digit sets with N(d) <= {maxnorm}, B={B}")
    for _ in range(n):
        dig = {cl: rnd.choice(cand[cl]) for cl, _ in L}
        alpha = rnd.choice(A)
        r = test(alpha, dig, k, B); nok += r[0]; nmov += r[0] and bool(r[3])
        show(alpha, dig, k, *r, extra=f' digits={sorted(dig.values())}')
    print(f"# sample: {n}, criterion {nok}, with a trivial cycle through two or more units {nmov}")

if __name__ == '__main__':
    a = sys.argv[1:]
    if a[0] == 'min': run_min(int(a[1]), int(a[2]), int(a[3]) if len(a) > 3 else 0)
    else: run_nonmin(int(a[1]), int(a[2]), int(a[3]), int(a[4]), int(a[5]))
