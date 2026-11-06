"""Does symmetry follow from the cycle criterion?  (collatz/research/symmetry-test.md)

Criterion: every nonzero orbit reaches a cycle that contains a unit (literal cycles, not up to
units).  Rules: Collatz-like alpha at level k, digit set with one element per odd class mod
(1+i)^k, minimal (each digit of least norm in its class), every tie-break allowed.

usage:
  symtest.py ties                      least-norm elements per odd class, Z[i] levels 2..7, Z levels 1..6
  symtest.py gauss k B [cap] [--all]   every alpha (all associates) x every minimal digit set,
                                       reduced modulo the conjugacy (alpha,D)~(alpha,uD);
                                       --all runs every pair
  symtest.py gsample k B per seed      every alpha x `per` random non-symmetric minimal digit sets
  symtest.py gsymnm k B nsmall         symmetric rules with each orbit digit among the nsmall least-norm elements
  symtest.py zsymnm k B nsmall [sample]  the same in Z (needs zany)
  symtest.py zloop k B vmax nfree sample   Z rules with the unit loop 1 <-> -1 (needs zany)
  symtest.py gnonmin k B n seed [maxnorm]  n random non-symmetric digit sets (each digit odd, of
                                       norm <= maxnorm, in its class) with random alpha
  symtest.py int k B                   Z, every m with 2^k < |m| < 2^(k+1), every minimal digit set
  symtest.py inonmin k B n seed dmax   Z, n random non-symmetric digit sets with |d| <= dmax
"""
import sys, os, itertools, random, subprocess
import rules as R

HERE = os.path.dirname(os.path.abspath(__file__))
SCR = '/tmp/claude-0/-home-claude/6c273ef9-4bf4-5ea2-bd9e-566136af1776/scratchpad'
U = R.UNITS

def conj(z): return (z[0], -z[1])

def classes(k, box=24):
    """odd classes mod (1+i)^k, each as the sorted list of its odd elements in the box."""
    M = 2 ** ((k + 1) // 2)          # (1+i)^k divides 2^ceil(k/2)
    reps = [(a, b) for a in range(M) for b in range(M) if (a + b) % 2]
    lab = {}
    out = []
    for r in reps:
        if r in lab: continue
        cl = [s for s in reps if s == r or R.val(s[0] - r[0], s[1] - r[1]) >= k]
        for s in cl: lab[s] = len(out)
        out.append(r)
    assert len(out) == 2 ** (k - 1)
    els = [[] for _ in out]
    for x in range(-box, box + 1):
        for y in range(-box, box + 1):
            if (x + y) % 2: els[lab[(x % M, y % M)]].append((x, y))
    return M, lab, els

def least(k):
    M, lab, els = classes(k)
    res = []
    for e in els:
        n = min(R.norm(z) for z in e)
        assert n < 24 * 24
        res.append(sorted(z for z in e if R.norm(z) == n))
    return M, lab, res

def ties():
    print("Z[i]: odd classes mod (1+i)^k, least-norm elements")
    for k in range(2, 8):
        M, lab, L = least(k)
        nsets = 1
        for l in L: nsets *= len(l)
        tied = [l for l in L if len(l) > 1]
        # symmetric minimal sets: choices closed under units
        sym = 0
        for ch in itertools.product(*L):
            S = set(ch)
            if all(R.mul(u, d) in S for d in S for u in U): sym += 1
        print(f"k={k}: {len(L)} odd classes, {len(tied)} with ties, minimal digit sets {nsets}, symmetric {sym}")
        for l in tied: print(f"    tie norm {R.norm(l[0])}: {l}")
    print("Z: odd classes mod 2^k, elements of least |x|")
    for k in range(1, 7):
        L = []
        for c in range(1, 2 ** k, 2):
            e = [x for x in range(-2 ** k - 1, 2 ** k + 2) if (x - c) % 2 ** k == 0]
            n = min(abs(x) for x in e); L.append([x for x in e if abs(x) == n])
        nsets = 1
        for l in L: nsets *= len(l)
        print(f"k={k}: {len(L)} odd classes, ties {[l for l in L if len(l) > 1]}, minimal digit sets {nsets}")

# ---------------------------------------------------------------- Gaussian runs
def alphas_all(k):
    A = []
    for x in range(-16, 17):
        for y in range(-16, 17):
            if (x + y) % 2 and 2 ** k < x * x + y * y < 2 ** (k + 1): A.append((x, y))
    return sorted(A, key=R.key)

def digit_of(alpha, D, k):
    """map odd residue -> digit, as dict on the residues mod M used by gcz (M=4 or 8)."""
    def d(z):
        az = R.mul(alpha, z)
        c = [g for g in D if R.val(*R.add(az, g)) >= k]
        assert len(c) == 1, (alpha, z, c)
        return c[0]
    return d

def rule_rows(alpha, D, k):
    M = 4 if k <= 4 else 8
    dg = digit_of(alpha, D, k)
    return R.table(lambda r: (alpha, dg(r), (1, 0)), M=M)

def Sexact(alpha, D, k):
    dg = digit_of(alpha, D, k)
    def S(z):
        x, y = R.add(R.mul(alpha, z), dg(z)); v = 0
        while (x + y) % 2 == 0: x, y = (x + y) // 2, (y - x) // 2; v += 1
        return (x, y)
    return S

def cyc_elems(S, z0, nmax=10 ** 6):
    """odd elements of the cycle through odd z0, iterating S exactly; None if z0 is not on a cycle."""
    seq = [z0]; z = S(z0)
    while z != z0:
        seq.append(z); z = S(z)
        if len(seq) > nmax: return None
    return seq

def oddpart(z):
    x, y = z
    while (x + y) % 2 == 0 and (x, y) != (0, 0): x, y = (x + y) // 2, (y - x) // 2
    return (x, y)

def gauss_run(alpha, D, k, B, cap, tag):
    rows = rule_rows(alpha, D, k)
    out = R.run(rows, B, cap=cap, tag=tag, quiet=False)
    h, stats, cyc = R.parse(out)
    S = Sexact(alpha, D, k)
    cl = []
    for c in cyc:
        if c['min'] == (0, 0): continue
        z0 = oddpart(c['min'])
        el = cyc_elems(S, z0)
        assert el is not None and min(R.norm(w) for w in el) <= R.norm(c['min']), (alpha, D, c)
        unit = any(R.norm(w) == 1 for w in el) or R.norm(c['min']) == 1
        cl.append(dict(min=c['min'], odd=len(el), count=c['count'], unit=unit,
                       maxnorm=c['maxnorm'], els=el))
    return h, stats, cl

def canon(alpha, D):
    """canonical representative of (alpha, D) under the conjugacy z -> uz, which sends D to uD.
    Complex conjugation is not a conjugacy of non-symmetric rules: conj(z/(1+i)) = i conj(z)/(1+i)."""
    return (alpha, min(tuple(sorted(R.mul(u, d) for d in D)) for u in U))

def is_sym(D):
    S = set(D)
    return all(R.mul(u, d) in S for d in S for u in U)

def report(alpha, D, k, h, cl, extra=''):
    ok = h['unresolved'] == 0 and all(c['unit'] for c in cl)
    nt = sum(1 for c in cl if not c['unit'])
    print(f"k={k} alpha={alpha} N={R.norm(alpha)} D={list(D)} sym={is_sym(D)} unres={h['unresolved']} "
          f"esc={h.get('escaped', 0)} cycles={len(cl)} trivial={len(cl) - nt} nontrivial={nt} CRIT={'YES' if ok else 'no'} "
          f"cyc={[(c['min'], c['odd'], c['count'], 'U' if c['unit'] else '-') for c in sorted(cl, key=lambda c: -c['count'])[:8]]}{extra}",
          flush=True)
    return ok

def gauss(k, B, cap, allpairs):
    M, lab, L = least(k)
    sets = [list(ch) for ch in itertools.product(*L)]
    A = alphas_all(k)
    done = set(); n = 0; nok = 0
    print(f"# level {k}: {len(A)} multipliers (all associates), {len(sets)} minimal digit sets, B={B}")
    for alpha in A:
        for D in sets:
            c = canon(alpha, D)
            if not allpairs:
                if c in done: continue
                done.add(c)
            h, st, cl = gauss_run(alpha, D, k, B, cap, f'sym{k}')
            n += 1; nok += report(alpha, D, k, h, cl)
    print(f"# level {k}: rules run {n}, satisfying the criterion {nok}")

def gsample(k, B, per, seed, cap=50000):
    """every alpha (all associates) x `per` random non-symmetric minimal digit sets."""
    rnd = random.Random(seed)
    M, lab, L = least(k)
    A = alphas_all(k)
    n = 0; nok = 0
    print(f"# level {k} sample: {len(A)} multipliers (all associates), {per} random non-symmetric minimal sets each, B={B}")
    for alpha in A:
        seen = set(); j = 0
        while j < per:
            D = [rnd.choice(l) for l in L]
            c = canon(alpha, D)
            if is_sym(D) or c in seen: continue
            seen.add(c); j += 1
            h, st, cl = gauss_run(alpha, D, k, B, cap, f'gs{k}')
            n += 1; nok += report(alpha, D, k, h, cl)
    print(f"# level {k} sample: rules run {n}, satisfying the criterion {nok}")

def sym_orbits(k):
    """unit orbits of odd classes mod (1+i)^k (k >= 3): list of (representative class index, elements sorted by norm)."""
    M, lab, els = classes(k)
    seen = set(); out = []
    for i, e in enumerate(els):
        if i in seen: continue
        r = e[0]
        orb = {lab[(R.mul(u, r)[0] % M, R.mul(u, r)[1] % M)] for u in U}
        seen |= orb
        out.append(sorted(e, key=R.key))
    return out

def gsymnm(k, B, nsmall, cap=50000):
    """symmetric rules, digit of each orbit's representative class among its nsmall least-norm elements."""
    orbs = sym_orbits(k)
    cand = [o[:nsmall] for o in orbs]
    A = sorted({R.assoc_min(a) for a in alphas_all(k)}, key=R.key)
    print(f"# symmetric non-minimal: level {k}, {len(A)} multipliers up to units, candidates per orbit {cand}")
    n = nok = nokmin = 0
    for alpha in A:
        for ch in itertools.product(*cand):
            D = [R.mul(u, g) for g in ch for u in U]
            mn = all(R.norm(g) == R.norm(o[0]) for g, o in zip(ch, orbs))
            h, st, cl = gauss_run(alpha, D, k, B, cap, f'sn{k}')
            n += 1; ok = report(alpha, list(ch), k, h, cl, extra=f' minimal={mn} digitnorms={[R.norm(g) for g in ch]}')
            nok += ok; nokmin += ok and mn
    print(f"# level {k}: symmetric rules {n}, passing {nok}, of which minimal {nokmin}")

def zsymnm(k, B, nsmall, sample=0, seed=1):
    """integer symmetric rules, digit for class c (0<c<2^(k-1)) among the nsmall smallest |x| = c mod 2^k, d(-c) = -d(c)."""
    rnd = random.Random(seed)
    cs = list(range(1, 2 ** (k - 1), 2))
    cand = [sorted([x for x in range(-8 * 2 ** k, 8 * 2 ** k) if (x - c) % 2 ** k == 0], key=lambda x: (abs(x), x))[:nsmall] for c in cs]
    ms = list(range(2 ** k + 1, 2 ** (k + 1), 2))
    allch = list(itertools.product(*cand))
    todo = [(m, ch) for m in ms for ch in allch]
    if sample and sample < len(todo): todo = rnd.sample(todo, sample)
    print(f"# Z symmetric non-minimal: level {k}, candidates {dict(zip(cs, cand))}, rules {len(todo)}{' (sample)' if sample else ''}, B={B}")
    n = nok = nokmin = 0
    for m, ch in todo:
        dig = {}
        for c, d in zip(cs, ch): dig[c] = d; dig[2 ** k - c] = -d
        args = [os.path.join(HERE, 'zany'), str(k), str(m), str(B)] + [str(dig[c]) for c in range(1, 2 ** k, 2)]
        out = subprocess.run(args, capture_output=True, text=True, check=True).stdout.strip()
        ok = out.split('|')[0].split()[5] == '1'
        mn = all(abs(d) < 2 ** (k - 1) for d in ch)
        n += 1; nok += ok; nokmin += ok and mn
        print(f"k={k} m={m} digits={list(ch)} minimal={mn} CRIT={'YES' if ok else 'no'} :: {out}", flush=True)
    print(f"# Z level {k}: symmetric rules {n}, passing {nok}, of which minimal {nokmin}")

def zloop(k, B, vmax, nfree, sample, seed=1, dmax=64):
    """integer rules with S(1) = -1 and S(-1) = 1: the digits of the classes of -m and m are forced
    to -m - 2^v and m + 2^v' (v, v' in [k, k+vmax]); the other classes get one of their nfree
    smallest-|d| elements.  Runs zany on |x| <= B."""
    rnd = random.Random(seed)
    K = 2 ** k
    todo = []
    for m in [s * mm for mm in range(K + 1, 2 * K, 2) for s in (1, -1)]:
        ca, cb = (-m) % K, m % K
        for v in range(k, k + vmax + 1):
            for w in range(k, k + vmax + 1):
                da, db = -m - 2 ** v, m + 2 ** w
                if k == 1:
                    if da != db: continue
                elif ca == cb: continue
                if abs(da) > dmax or abs(db) > dmax: continue
                free = [c for c in range(1, K, 2) if c not in (ca, cb)]
                opts = [sorted([x for x in range(-4 * K, 4 * K) if (x - c) % K == 0], key=lambda x: (abs(x), x))[:nfree] for c in free]
                for ch in itertools.product(*opts):
                    dig = dict(zip(free, ch)); dig[ca] = da; dig[cb] = db
                    todo.append((m, v, w, dig))
    n_all = len(todo)
    if sample and sample < len(todo): todo = rnd.sample(todo, sample)
    print(f"# Z unit loops 1 <-> -1: level {k}, rules {n_all}{f', sample {len(todo)}' if sample and sample < n_all else ''}, B={B}")
    nok = nonly = 0
    for m, v, w, dig in todo:
        args = [os.path.join(HERE, 'zany'), str(k), str(m), str(B)] + [str(dig[c]) for c in range(1, K, 2)]
        out = subprocess.run(args, capture_output=True, text=True, check=True).stdout.strip()
        f = out.split('|')[0].split(); ok = f[5] == '1'; only = ok and f[3] == '1'
        symm = all(dig[(K - c) % K] == -dig[c] for c in dig) if k > 1 else False
        nok += ok; nonly += only
        print(f"k={k} m={m} v={v} v'={w} digits={dict(sorted(dig.items()))} sym={symm} CRIT={'YES' if ok else 'no'} loop_only={only} :: {out}", flush=True)
    print(f"# level {k}: run {len(todo)}, criterion {nok}, every orbit into the loop {nonly}")

def gnonmin(k, B, n, seed, maxnorm, cap=50000):
    rnd = random.Random(seed)
    M, lab, els = classes(k)
    cand = [[z for z in e if R.norm(z) <= maxnorm] for e in els]
    A = alphas_all(k)
    nok = 0; nrun = 0; nsym_ok = 0
    print(f"# non-minimal sample: level {k}, {n} rules, digits of norm <= {maxnorm}, candidates per class {[len(c) for c in cand]}")
    while nrun < n:
        D = [rnd.choice(c) for c in cand]
        if is_sym(D): continue
        alpha = rnd.choice(A)
        h, st, cl = gauss_run(alpha, D, k, B, cap, f'nm{k}')
        nrun += 1; nok += report(alpha, D, k, h, cl)
    print(f"# sample: {nrun} non-symmetric rules, satisfying the criterion {nok}")

# ---------------------------------------------------------------- integers
def int_census(m, dig, k, B, cap=10 ** 6):
    """literal cycles of x -> (m x + d)/2^v on all odd 0 < |x| <= B; dig: residue mod 2^k -> digit."""
    mask = 2 ** k - 1
    def S(x):
        y = m * x + dig[(-m * x) & mask]
        while y % 2 == 0: y //= 2
        return y
    st = {}           # odd x -> cycle id (memo for |x| <= B)
    cycles = []; unres = 0; peak = 0
    for x0 in range(-B, B + 1):
        if x0 % 2 == 0 or x0 in st: continue
        path = []; x = x0; seen = {}
        res = None
        while True:
            if x in st: res = st[x]; break
            if x in seen:
                i = seen[x]; cyc = path[i:]
                res = len(cycles); cycles.append(dict(min=min(cyc, key=abs), odd=len(cyc), count=0, els=cyc))
                break
            seen[x] = len(path); path.append(x)
            if abs(x) > peak: peak = abs(x)
            if len(path) > cap or abs(x) > 2 ** 400: res = -1; break
            x = S(x)
        if res == -1: unres += 1
        for w in path:
            if abs(w) <= B: st[w] = res
    for x, c in st.items():
        if c >= 0: cycles[c]['count'] += 1
    return cycles, unres, peak

def int_run(k, B):
    print(f"# Z level {k}: B={B}")
    nok = 0; nrun = 0
    for m in list(range(2 ** k + 1, 2 ** (k + 1), 2)) + list(range(-2 ** (k + 1) + 1, -2 ** k, 2)):
        L = []
        for c in range(1, 2 ** k, 2):
            e = [x for x in range(-2 ** k - 1, 2 ** k + 2) if (x - c) % 2 ** k == 0]
            n = min(abs(x) for x in e); L.append((c, [x for x in e if abs(x) == n]))
        for ch in itertools.product(*[l for _, l in L]):
            dig = {c: d for (c, _), d in zip(L, ch)}
            cyc, unres, peak = int_census(m, dig, k, B)
            ok = unres == 0 and all(any(abs(w) == 1 for w in c['els']) for c in cyc)
            nrun += 1; nok += ok
            print(f"k={k} m={m} digits={dict(sorted(dig.items()))} unres={unres} cycles={len(cyc)} CRIT={'YES' if ok else 'no'} "
                  f"cyc={[(c['min'], c['odd'], c['count']) for c in sorted(cyc, key=lambda c: -c['count'])[:8]]}", flush=True)
    print(f"# Z level {k}: rules {nrun}, satisfying {nok}")

def inonmin(k, B, n, seed, dmax):
    rnd = random.Random(seed)
    nok = 0; nrun = 0
    print(f"# Z non-minimal sample: level {k}, {n} rules, |d| <= {dmax}")
    while nrun < n:
        dig = {c: rnd.choice([x for x in range(-dmax, dmax + 1) if (x - c) % 2 ** k == 0]) for c in range(1, 2 ** k, 2)}
        if all(dig[(-c) % 2 ** k] == -dig[c] for c in dig): continue
        m = rnd.choice([s * mm for mm in range(2 ** k + 1, 2 ** (k + 1), 2) for s in (1, -1)])
        args = [os.path.join(HERE, 'zany'), str(k), str(m), str(B)] + [str(dig[c]) for c in range(1, 2 ** k, 2)]
        out = subprocess.run(args, capture_output=True, text=True, check=True).stdout.strip()
        ok = out.split('|')[0].split()[5] == '1'
        nrun += 1; nok += ok
        print(f"k={k} m={m} digits={dict(sorted(dig.items()))} CRIT={'YES' if ok else 'no'} :: {out}", flush=True)
    print(f"# sample: {nrun} non-symmetric rules, satisfying {nok}")

if __name__ == '__main__':
    a = sys.argv[1:]
    if a[0] == 'ties': ties()
    elif a[0] == 'gauss': gauss(int(a[1]), int(a[2]), int(a[3]) if len(a) > 3 and a[3][0] != '-' else 50000, '--all' in a)
    elif a[0] == 'gsample': gsample(int(a[1]), int(a[2]), int(a[3]), int(a[4]))
    elif a[0] == 'gsymnm': gsymnm(int(a[1]), int(a[2]), int(a[3]))
    elif a[0] == 'zsymnm': zsymnm(int(a[1]), int(a[2]), int(a[3]), int(a[4]) if len(a) > 4 else 0)
    elif a[0] == 'zloop': zloop(int(a[1]), int(a[2]), int(a[3]), int(a[4]), int(a[5]))
    elif a[0] == 'gnonmin': gnonmin(int(a[1]), int(a[2]), int(a[3]), int(a[4]), int(a[5]) if len(a) > 5 else 25)
    elif a[0] == 'int': int_run(int(a[1]), int(a[2]))
    elif a[0] == 'inonmin': inonmin(int(a[1]), int(a[2]), int(a[3]), int(a[4]), int(a[5]))
