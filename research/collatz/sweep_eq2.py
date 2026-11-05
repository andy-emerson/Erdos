"""Unit-equivariant digit maps for modulus (1+i)^k: digit set = union of unit orbits,
one orbit per unit-orbit of odd classes mod (1+i)^k, representatives of norm <= extra + minimal norm.
alpha ranges over odd elements up to units with 2^k < N(alpha) < 2^(k+1).
usage: sweep_eq2.py k B [slack] [--cap N] [--only FILE] [--setup]
  --only FILE: run only the rules listed in FILE, one per line as "ax,ay g1x,g1y;g2x,g2y;..."
  --setup: print classes, orbits, candidates, alphas and the rule count, run nothing.
Residues: the class mod (1+i)^k is fixed by z mod 2^ceil(k/2), so tables are mod 4 (k <= 4),
mod 8 (k = 5, 6) or mod 16 (k = 7, 8).  Level 7 added 4 Nov; gcz now runs non-quiet so all raw
cycles are grouped (quiet printed at most 40)."""
import sys, itertools, rules as R, exact as E
args = sys.argv[1:]
cap = 50000; only = None; setup = False
if '--cap' in args: i = args.index('--cap'); cap = int(args[i+1]); del args[i:i+2]
if '--only' in args: i = args.index('--only'); only = args[i+1]; del args[i:i+2]
if '--setup' in args: setup = True; args.remove('--setup')
k, B = int(args[0]), int(args[1]); slack = int(args[2]) if len(args) > 2 else 0
assert 3 <= k <= 8
M = 4 if k <= 4 else 8 if k <= 6 else 16
RA = 12 if k <= 6 else 16      # alpha range: N(alpha) < 2^(k+1)
RD = 12 if k <= 6 else 24      # range for least-norm class elements
def same(z, w): return R.val(z[0]-w[0], z[1]-w[1]) >= k if z != w else True
odd = [(x, y) for x in range(-RD, RD+1) for y in range(-RD, RD+1) if (x + y) % 2]
# classes: canonical label = min residue mod M
reps = [(a, b) for a in range(M) for b in range(M) if (a + b) % 2]
def lab(z):
    return min(r for r in reps if same(r, z))
classes = sorted(set(lab(r) for r in reps))
assert len(classes) == 2**(k-1)
# unit orbits of classes
orbits = []
seen = set()
for c in classes:
    if c in seen: continue
    orb = sorted(set(lab(R.mul(u, c)) for u in R.UNITS))
    seen |= set(orb); orbits.append(orb)
cand = []
for orb in orbits:
    els = [z for z in odd if lab(z) == orb[0]]
    mn = min(R.norm(z) for z in els)
    assert mn + slack < (RD + 1)**2   # every element of that norm lies in the search box
    cand.append([z for z in els if R.norm(z) <= mn + slack])
alphas = {}
for x in range(-RA, RA+1):
    for y in range(-RA, RA+1):
        n = x*x + y*y
        if (x + y) % 2 and 2**k < n < 2**(k+1):
            a = R.assoc_min((x, y)); alphas[a] = 1
print(f"k={k}: {len(classes)} classes, {len(orbits)} unit orbits, candidates per orbit {[len(c) for c in cand]}, {len(alphas)} alphas", flush=True)
if setup:
    for orb, c in zip(orbits, cand):
        print(f"  orbit {orb}: least norm {R.norm(c[0])}, candidates {c}")
    nd = 1
    for c in cand: nd *= len(c)
    print(f"  alphas (up to units) {sorted(alphas, key=R.key)}")
    print(f"  norms {sorted(set(R.norm(a) for a in alphas))}")
    print(f"  digit sets {nd}, rules {nd*len(alphas)}")
    sys.exit(0)
todo = None
if only:
    todo = set()
    for l in open(only):
        if not l.strip() or l.startswith('#'): continue
        a, g = l.split()
        todo.add((tuple(map(int, a.split(','))), tuple(tuple(map(int, x.split(','))) for x in g.split(';'))))
for al in sorted(alphas, key=R.key):
    for choice in itertools.product(*cand):
        if todo is not None and (al, tuple(choice)) not in todo: continue
        D = [R.mul(u, g) for g in choice for u in R.UNITS]
        try:
            rows = R.table(R.digit_rule(al, D, k), M=M)
        except AssertionError:
            continue
        h, stats, cyc = R.parse(R.run(rows, B, cap=cap, tag=f'e{k}', quiet=False))
        g = [c for c in R.group_mod_units(cyc, E.make_S(al, D, k)) if c['min'] != (0, 0)]
        mexc = float(stats.split('maxexc_log2 ')[1].split()[0]); mtot = int(stats.split('maxtot ')[1].split()[0])
        print(f"N={R.norm(al)} alpha={al} gens={list(choice)} unres={h['unresolved']} esc={h.get('escaped', 0)} rawcyc={h['cycles']} classes={len(g)} maxtot={mtot} maxexc={mexc:.1f} cyc={[(c['min'], c['len'], c['odd'], round(c['count']/h['starts'],4)) for c in g[:5]]}", flush=True)
