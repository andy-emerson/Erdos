"""ringfdforced.py: forced fixed classes for Collatz-type maps (rings-test.md). If eta = eps pi^v - alpha is a unit
(any unit eps, v >= k) and z = eta^(-1) d(c) is a non-unit element of F, then S_F(z) = rep_F(eps z) = z: [z] is a
fixed class other than [1]. For each (ring, F, k) the script regenerates the tables of the search, predicts the
failures, and compares with the engine.
usage: python3 ringfdforced.py"""
import collections
import qring as Q, ringrun as RR, ringtest as RT
cases = [('Zs2', 'R', 3, 4, 0), ('Zs2', 'H', 3, 4, 0), ('Zw', 'S', 2, 2, 2000), ('Z11', 'R', 2, 2, 0), ('Zi', 'Q', 3, 4, 0), ('Zi', 'C', 3, 4, 0)]
L = ["# forced fixed classes for Collatz-type maps: predicted failures against the engine (B = 1e5)"]
for nm, F, k, ns, smp in cases:
    R = Q.Ring(nm)
    c = collections.Counter()
    for a in RT.multipliers(R, k):
        etas = []
        for v in range(k, k + 6):
            for e in R.U:
                eta = R.sub(R.mul(e, R.pw(R.pi, v)), a)
                if R.is_unit(eta): etas.append(eta)
        if not etas: continue
        tabs = RT.fd_tables(R, F, a, k, ns, nsample=smp)
        out = RR.run(R, F, k, [(f't{j}', a, t[0]) for j, t in enumerate(tabs)], 0, 10 ** 5)
        for t, l in zip(tabs, out):
            pred = False
            for i in R.odd_classes(k):
                for eta in etas:
                    z = R.div_exact(t[0][i], eta)
                    if z is not None and Q.inF(R, F, z) and not R.is_unit(z): pred = True
            ok = ' PASS ' in l
            c[(Q.fmt(R, a), 'predicted fail' if pred else 'not predicted', 'passes' if ok else 'fails')] += 1
    L.append(f"{nm} F={F} k={k}: " + "; ".join(f"{a} {p}, {o}: {n}" for (a, p, o), n in sorted(c.items())))
RT.log(Q.Ring('Z'), 'fdforced', L)
