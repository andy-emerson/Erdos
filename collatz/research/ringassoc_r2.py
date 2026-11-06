"""ringassoc_r2.py: Z[sqrt2], residue-class maps against the full unit group (rings-test.md). For k = 1..4 and units
u in {-1, eps, eps^2, eps^4}: pair test as in ringchecks.py assoc, associates now meaning equal classes in E
(same rep under +-eps^n). A pair passes if S(uz) ~ S(z) at every test point of the class.
usage: python3 ringassoc_r2.py"""
import collections
import qring as Q, ringtest as RT, ringchecks as RC
R = Q.Ring('Zr2')
def S1(a, d, z):
    w = R.add(R.mul(a, z), d); return R.oddpart(w)[0] if w != (0, 0) else (0, 0)
def ass(x, y): return x == y == (0, 0) or (x != (0, 0) and y != (0, 0) and Q.zr2_rep(x) == Q.zr2_rep(y))
units = {'-1': (-1, 0), 'eps': Q.EPS, 'eps^2': Q.zr2_mul(Q.EPS, Q.EPS), 'eps^4': (17, 12)}
pts = [z for z in R.elements(300) if R.odd(z)]
digs = [z for z in R.elements(30) if R.odd(z)]
L = ["# Z[sqrt2]: pair test against units -1, eps, eps^2, eps^4 (digits a^2+2b^2 <= 30, test points <= 300)"]
for k in range(1, 5):
    al = RT.multipliers(R, max(k, 2))[:3]
    for un, u in units.items():
        same = R.idx(u, k) == R.idx((1, 0), k)
        npairs = ncom = ncomp = nbad = 0
        for a in al:
            for c in R.odd_classes(k):
                cz = R.classrep(c, k); uc = R.idx(R.mul(u, cz), k)
                P = [z for z in pts if R.idx(z, k) == c]
                D1 = [d for d in digs if R.idx(d, k) == R.idx(R.neg(R.mul(a, cz)), k)]
                D2 = [d for d in digs if R.idx(d, k) == R.idx(R.neg(R.mul(a, R.classrep(uc, k))), k)]
                for d1 in D1:
                    for d2 in D2:
                        if uc == c and d2 != d1: continue
                        npairs += 1
                        ok = all(ass(S1(a, d2, R.mul(u, z)), S1(a, d1, z)) for z in P)
                        com = d2 == R.mul(u, d1)
                        if com: ncom += 1; ncomp += ok
                        else: nbad += ok
        L.append(f"k={k} u={un} (u = 1 mod pi^k: {same}): pairs {npairs}, commuting pairs {ncom} (passing {ncomp}), non-commuting passing {nbad}")
RT.log(R, 'assoc', L)
