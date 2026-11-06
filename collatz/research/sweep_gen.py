"""Enumerate digit-rule maps: odd z -> alpha z + d(z), d(z) taken per class of z mod (1+i)^k
from small odd candidates so that (1+i)^k | alpha z + d; then divide by 1+i while possible.
Usage: sweep_gen.py k B maxdigitnorm alphaNorms(comma) > out"""
import sys, itertools, rules as R
k, B, dn = int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3])
anorms = [int(x) for x in sys.argv[4].split(',')]
odd_small = [(x, y) for x in range(-3, 4) for y in range(-3, 4) if (x + y) % 2 and x*x + y*y <= dn]
alphas = [(x, y) for x in range(-6, 7) for y in range(-6, 7) if (x + y) % 2 and x*x + y*y in anorms]
# odd residues mod 4, grouped into classes mod (1+i)^k
odd_res = [(a, b) for a in range(4) for b in range(4) if (a + b) % 2]
def cls(r):  # canonical class label mod (1+i)^k: the set of odd residues congruent to r
    return min(s for s in odd_res if R.val(*R.add(s, R.mul((-1, 0), r))) >= k or s == r)
classes = sorted(set(cls(r) for r in odd_res))
results = []
for al in alphas:
    # candidates per class
    cand = []
    for c in classes:
        az = R.mul(al, c)
        cand.append([d for d in odd_small if R.val(*R.add(az, d)) >= k])
    for choice in itertools.product(*cand):
        dmap = {c: d for c, d in zip(classes, choice)}
        def f(r, al=al, dmap=dmap): return (al, dmap[cls(r)], (1, 0))
        h, stats, cyc = R.parse(R.run(R.table(f), B, cap=20000, tag=f'g{k}'))
        nz = [c for c in cyc if c['min'] != (0, 0)]
        top = max((c['count'] for c in nz), default=0)
        maxexc = float(stats.split('maxexc_log2 ')[1].split()[0])
        results.append((h['unresolved'], len(nz), -top, al, choice, maxexc, [ (c['min'], c['len'], c['odd'], c['count']) for c in nz[:6]]))
        print(h['unresolved'], len(nz), top / h['starts'], al, choice, maxexc, [(c['min'], c['len'], c['odd'], c['count']) for c in nz[:6]], flush=True)
