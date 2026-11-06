"""Is "S (or T) sends associates to associates" strictly weaker than "the digit table
commutes with the units" for maps of our form?  Pair test (see quotient.md, Section 5).

For a digit table d at modulus P^k, the condition S(uz) ~ S(z) for z in an odd class c
involves only the two digits d(c) and d(uc).  So it suffices to test pairs: for each odd
class c, unit u != 1, digit delta1 in the class of -alpha*c and digit delta2 in the class of
-alpha*u*c, check S(uz) ~ S(z) (and T(uz) ~ T(z)) for odd z in c up to a norm bound.  A
non-commuting associate-respecting table exists only if some pair with delta2 != u*delta1
passes.
"""
import itertools, sys

# ---------- Gaussian integers ----------
def gmul(x, y): return (x[0]*y[0]-x[1]*y[1], x[0]*y[1]+x[1]*y[0])
def gadd(x, y): return (x[0]+y[0], x[1]+y[1])
def gnorm(x): return x[0]*x[0]+x[1]*x[1]
GU = [(1,0),(0,1),(-1,0),(0,-1)]
def godd(z): return (z[0]-z[1]) % 2 != 0
def gpipow(k):
    r = (1,0)
    for _ in range(k): r = gmul(r, (1,1))
    return r
def gkey(z, k):
    M = gpipow(k); w = gmul(z, (M[0], -M[1])); m = 2**k
    return (w[0] % m, w[1] % m)
def goddpart(w):
    v = 0
    while w != (0,0) and (w[0]-w[1]) % 2 == 0:
        w = ((w[0]+w[1])//2, (w[1]-w[0])//2); v += 1
    return w
def gassoc(x, y): return any(gmul(u, x) == y for u in GU)

def gauss_test(k, alphas, dig_norm, zbound):
    # odd points by class
    R = int(zbound**0.5)+1
    pts = {}
    for a in range(-R, R+1):
        for b in range(-R, R+1):
            z = (a,b)
            if godd(z) and gnorm(z) <= zbound:
                pts.setdefault(gkey(z,k), []).append(z)
    Rd = int(dig_norm**0.5)+1
    cands = {}
    for a in range(-Rd, Rd+1):
        for b in range(-Rd, Rd+1):
            z=(a,b)
            if godd(z) and gnorm(z) <= dig_norm:
                cands.setdefault(gkey(z,k), []).append(z)
    classes = sorted(pts)
    rep = {c: min(pts[c], key=gnorm) for c in classes}
    stats = dict(pairs=0, comm=0, passS_nc=0, passT_nc=0, passS_c=0, failS_c=0)
    found = []
    for al in alphas:
        for c in classes:
            z0 = rep[c]
            for u in GU[1:]:
                uc = gkey(gmul(u, z0), k)
                c1 = gkey(gmul((-al[0],-al[1]), z0), k)
                c2 = gkey(gmul((-al[0],-al[1]), gmul(u, z0)), k)
                for d1 in cands.get(c1, []):
                    for d2 in cands.get(c2, []):
                        stats['pairs'] += 1
                        commuting = (gmul(u, d1) == d2)
                        okS = okT = True
                        for z in pts[c]:
                            w1 = gadd(gmul(al, z), d1)
                            w2 = gadd(gmul(al, gmul(u, z)), d2)
                            if okT and not gassoc(w1, w2): okT = False
                            if okS and not gassoc(goddpart(w1), goddpart(w2)): okS = False
                            if not okS and not okT: break
                        if commuting:
                            stats['comm'] += 1
                            stats['passS_c' if okS else 'failS_c'] += 1
                        else:
                            if okS: stats['passS_nc'] += 1; found.append(('S', al, c, u, d1, d2))
                            if okT: stats['passT_nc'] += 1; found.append(('T', al, c, u, d1, d2))
    return stats, found

def godd_alphas(maxn):
    R = int(maxn**0.5)+1
    return [(a,b) for a in range(-R,R+1) for b in range(-R,R+1)
            if godd((a,b)) and gnorm((a,b)) <= maxn]

# ---------- integers ----------
def zoddpart(w):
    while w != 0 and w % 2 == 0: w //= 2
    return w
def int_test(k, ms, dmax, zmax):
    M = 2**k
    stats = dict(pairs=0, comm=0, passS_nc=0, passT_nc=0, passS_c=0, failS_c=0)
    found = []
    for m in ms:
        for c in range(1, M, 2):
            zs = [z for z in range(-zmax, zmax+1) if z % M == c]
            for d1 in range(-dmax, dmax+1):
                if (m*c + d1) % M: continue
                for d2 in range(-dmax, dmax+1):
                    if (-m*c + d2) % M: continue
                    stats['pairs'] += 1
                    commuting = (d2 == -d1)
                    okS = okT = True
                    for z in zs:
                        w1, w2 = m*z + d1, -m*z + d2
                        if okT and abs(w1) != abs(w2): okT = False
                        if okS and abs(zoddpart(w1)) != abs(zoddpart(w2)): okS = False
                        if not okS and not okT: break
                    if commuting:
                        stats['comm'] += 1
                        stats['passS_c' if okS else 'failS_c'] += 1
                    else:
                        if okS: stats['passS_nc'] += 1; found.append(('S', m, c, d1, d2))
                        if okT: stats['passT_nc'] += 1; found.append(('T', m, c, d1, d2))
    return stats, found

if __name__ == '__main__' and len(sys.argv) == 1:
    al = godd_alphas(61)
    for k, dn in [(3, 65), (4, 65)]:
        st, f = gauss_test(k, al, dn, 400)
        print(f"Z[i] k={k}: {len(al)} odd alpha with N<=61, digits N<={dn}, z with N<=400:", st)
        for x in f[:10]: print("   found", x)
    ms = [m for m in range(-63, 64, 2)]
    for k in (2, 3, 4):
        st, f = int_test(k, ms, 65, 3000)
        print(f"Z k={k} (modulus {2**k}): odd |m|<=63, |d|<=65, |z|<=3000:", st)
        for x in f[:10]: print("   found", x)

def bound_check():
    """Theorem 5.1's bound: for a non-commuting pair with e = d2 - u*d1, every odd z in the
    class with N(w(z)) > 4N(e) and N(w(uz)) > 4N(e) violates S(uz) ~ S(z).  Check it, and
    also run levels k = 1, 2 (Z[i]) and k = 1 (Z), where no table commutes."""
    bad = 0; tested = 0
    for k in (1, 2, 3, 4):
        st, f = gauss_test(k, godd_alphas(29), 41, 200)
        print(f"Z[i] k={k}, N(alpha)<=29, digits N<=41, z N<=200:", st)
    for k in (1, 3):
        R = 15
        pts = [(a,b) for a in range(-R,R+1) for b in range(-R,R+1) if godd((a,b)) and gnorm((a,b))<=200]
        for al in godd_alphas(29):
            for z in pts:
                for u in GU[1:]:
                    for d1 in [(a,b) for a in range(-5,6) for b in range(-5,6) if godd((a,b))]:
                        w1 = gadd(gmul(al,z), d1)
                        if (gkey(w1, k) != (0,0)): continue
                        for e in [(a,b) for a in range(-4,5) for b in range(-4,5) if (a,b)!=(0,0) and not godd((a,b))]:
                            d2 = gadd(gmul(u,d1), e)
                            w2 = gadd(gmul(al, gmul(u,z)), d2)
                            if gnorm(w1) > 4*gnorm(e) and gnorm(w2) > 4*gnorm(e):
                                tested += 1
                                if gassoc(goddpart(w1), goddpart(w2)): bad += 1
    print("bound check (k=1,3 sample): tested", tested, "violations of the bound", bad)
    for k in (1,):
        st, f = int_test(k, list(range(-63,64,2)), 65, 3000)
        print(f"Z k=1 (modulus 2): ", st)

if __name__ == '__main__' and len(sys.argv) > 1 and sys.argv[1] == 'bound':
    bound_check()
