import sys, rules as R
B = int(sys.argv[1])
maps = [('k3', (3,0), R.UNITS, 3), ('k3', (3,2), R.UNITS, 3)]
for al in [(4,1),(4,-1),(5,0),(4,3),(4,-3),(5,2),(5,-2)]:
    maps.append(('k4', al, R.D_A, 4))
for name, al, D, k in maps:
    out = R.run(R.table(R.digit_rule(al, D, k)), B)
    h, stats, cyc = R.parse(out)
    g = R.group_mod_units(cyc)
    print(f"== {name} alpha={al} N={R.norm(al)} | {stats}")
    print(f"   starts {h['starts']} unresolved {h['unresolved']} raw cycles {h['cycles']} unit-classes {len(g)}")
    for c in g[:12]:
        print(f"   min {c['min']} N={R.norm(c['min'])} len {c['len']} odd {c['odd']} share {c['count']/h['starts']:.5f} copies {c['ncopies']} maxN {c['maxnorm']:.0f}")
