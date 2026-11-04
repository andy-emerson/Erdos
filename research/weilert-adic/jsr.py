from rho import rho
EX={m:rho(m)[0] for m in range(1,12)}
def R(m): return EX[m] if m in EX else 2**(m/2)/2**0.5   # covering radius bound beyond 11
def rate(delta, steps=300, gmax=40):
    c={g:R(g+delta)/2**(g/2) for g in range(1,gmax+1)}
    S={1:(1.0,1.0)}
    for k in range(steps):
        T={}
        for g,(xp,xc) in S.items():
            for g2 in range(delta,gmax+1):
                xn=2**(-g2/2)*(2**(-g/2)*xp+c[g]*xc)
                old=T.get(g2,(0,0))
                T[g2]=(max(old[0],xc),max(old[1],xn))
        m=max(max(v) for v in T.values())
        S={g:(v[0]/m,v[1]/m) for g,v in T.items()}
    return m, max(T, key=lambda g: T[g][1])
for d in (1,2,3,4):
    print("precision g+%d: growth bound per step %.4f (worst gap %d)"%((d,)+rate(d)))
