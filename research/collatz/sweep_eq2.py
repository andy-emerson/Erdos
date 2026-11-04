"""Unit-equivariant digit maps for modulus (1+i)^k: digit set = union of unit orbits,
one orbit per unit-orbit of odd classes mod (1+i)^k, representatives of norm <= extra + minimal norm.
alpha ranges over odd elements up to units with 2^k < N(alpha) < 2^(k+1).
usage: sweep_eq2.py k B [slack]"""
import sys, itertools, rules as R, exact as E
k, B = int(sys.argv[1]), int(sys.argv[2]); slack = int(sys.argv[3]) if len(sys.argv) > 3 else 0
M = 4 if k <= 4 else 8
def same(z, w): return R.val(z[0]-w[0], z[1]-w[1]) >= k if z != w else True
odd = [(x, y) for x in range(-12, 13) for y in range(-12, 13) if (x + y) % 2]
# classes: canonical label = min residue mod M
reps = [(a, b) for a in range(M) for b in range(M) if (a + b) % 2]
def lab(z):
    return min(r for r in reps if same(r, z))
classes = sorted(set(lab(r) for r in reps))
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
    cand.append([z for z in els if R.norm(z) <= mn + slack])
alphas = {}
for x in range(-12, 13):
    for y in range(-12, 13):
        n = x*x + y*y
        if (x + y) % 2 and 2**k < n < 2**(k+1):
            a = R.assoc_min((x, y)); alphas[a] = 1
print(f"k={k}: {len(classes)} classes, {len(orbits)} unit orbits, candidates per orbit {[len(c) for c in cand]}, {len(alphas)} alphas", flush=True)
for al in sorted(alphas, key=R.key):
    for choice in itertools.product(*cand):
        D = [R.mul(u, g) for g in choice for u in R.UNITS]
        try:
            rows = R.table(R.digit_rule(al, D, k), M=M)
        except AssertionError:
            continue
        h, stats, cyc = R.parse(R.run(rows, B, cap=50000, tag=f'e{k}'))
        g = [c for c in R.group_mod_units(cyc, E.make_S(al, D, k)) if c['min'] != (0, 0)]
        mexc = float(stats.split('maxexc_log2 ')[1].split()[0]); mtot = int(stats.split('maxtot ')[1].split()[0])
        print(f"N={R.norm(al)} alpha={al} gens={list(choice)} unres={h['unresolved']} classes={len(g)} maxtot={mtot} maxexc={mexc:.1f} cyc={[(c['min'], c['len'], c['odd'], round(c['count']/h['starts'],4)) for c in g[:5]]}", flush=True)
