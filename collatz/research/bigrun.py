"""Full classification of an equivariant digit rule to norm B with exact regrouping of cycles."""
import sys, rules as R, exact as E
al = tuple(map(int, sys.argv[1].split(','))); k = int(sys.argv[2]); B = int(sys.argv[3])
gens = [tuple(map(int, g.split(','))) for g in sys.argv[4].split(';')]
D = [R.mul(u, g) for g in gens for u in R.UNITS]
M = 4 if k <= 4 else 8
out = R.run(R.table(R.digit_rule(al, D, k), M=M), B, cap=1000000, tag='big', quiet=False)
h, stats, cyc = R.parse(out)
print(f"alpha={al} k={k} digits={gens} B={B}: starts {h['starts']} unresolved {h['unresolved']} | {stats}")
for c in R.group_mod_units(cyc, E.make_S(al, D, k)):
    print(f"  class {c['min']} N={R.norm(c['min'])} odd {c['odd']} T-steps {c['len']} share {c['count']/h['starts']:.6f}")
