"""ringlift.py: lifts of the Collatz-type maps on Z[sqrt2]/U (rings-test.md). For each map that passes (log of
ringtest.py fd Zr2 E ...), follow the commuting-free lift: from 1, apply w -> oddpart(alpha w + d(rep class)) without
rep, p steps (p the period of [1]); the result is a unit u = +-eps^m. If m != 0, no unit is periodic in the lift
(u^n != 1), so the lift has no trivial cycle although every class reaches [1] (the converse of A4 fails without
finite U). usage: python3 ringlift.py k nsmall [nsample]"""
import sys, collections
import qring as Q, ringrun as RR, ringtest as RT
R = Q.Ring('Zr2'); k = int(sys.argv[1]); ns = int(sys.argv[2]); smp = int(sys.argv[3]) if len(sys.argv) > 3 else 0
def eps_exp(u):
    m = 0; x = u
    if x in ((1, 0), (-1, 0)): return 0, x[0]
    for s in (1, -1):
        x = u; m = 0
        for j in range(1, 200):
            x = Q.zr2_mul(x, Q.EPSINV if s == 1 else Q.EPS)
            if x in ((1, 0), (-1, 0)): return s * j, x[0]
    return None, None
cnt = collections.Counter()
for a in RT.multipliers(R, k):
    tabs = RT.fd_tables(R, 'E', a, k, ns, nsample=smp)
    out = RR.run(R, 'E', k, [(f't{j}', a, t[0]) for j, t in enumerate(tabs)], 0, 10 ** 5)
    for t, l in zip(tabs, out):
        if ' PASS ' not in l: continue
        p = int(l.split('len ')[1].split(':')[0])
        # D(z) = u d(f) for z = u f, f in E: the commuting extension; follow it from 1
        z = (1, 0)
        for _ in range(p):
            f = Q.zr2_rep(z)
            u = R.div_exact(z, f)
            d = R.mul(u, t[0][R.idx(f, k)])
            z = R.oddpart(R.add(R.mul(a, z), d))[0]
        m, sg = eps_exp(z)
        cnt[(Q.fmt(R, a), 'S^p(1) = +-1' if m == 0 else 'S^p(1) = +-eps^m, m != 0')] += 1
        if cnt[(Q.fmt(R, a), 'first')] == 0:
            cnt[(Q.fmt(R, a), 'first')] = 1
            print(f"alpha={Q.fmt(R, a)} {RT.describe(R, t[3], t[4], k)}: period of [1] {p}, S^p(1) = {Q.fmt(R, z)} = {'-' if sg == -1 else ''}eps^{m}")
for kk, v in sorted(cnt.items()):
    if kk[1] != 'first': print(kk, v)
