from jsr import R
from math import sqrt
def bound(delta, x0=2**31.5, gmax=60):
    c={g:R(g+delta)/2**(g/2) for g in range(1,gmax+1)}
    S={1:(x0,x0)}; k=0
    while max(v[1] for v in S.values())>=sqrt(5):
        T={}
        for g,(xp,xc) in S.items():
            for g2 in range(delta,gmax+1):
                xn=2**(-g2/2)*(2**(-g/2)*xp+c[g]*xc)
                o=T.get(g2,(0,0)); T[g2]=(max(o[0],xc),max(o[1],xn))
        S=T; k+=1
        if k>2000: return None
    return k
for d in (2,3,4,5,6): print("S delta=%d proven steps <= %s (31-bit inputs)"%(d,bound(d)))
