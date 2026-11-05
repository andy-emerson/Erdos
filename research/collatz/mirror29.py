# Do the 8 symmetric variations of 5+2i have the same orbit structure up to units and reflection?
# Compare, for each start z, the sequence of norms under S for every variation against 5+2i at z or conj(z).
U=[(1,0),(0,1),(-1,0),(0,-1)]
def mul(x,y): return (x[0]*y[0]-x[1]*y[1], x[0]*y[1]+x[1]*y[0])
def N(z): return z[0]*z[0]+z[1]*z[1]
def odd(z): return (z[0]+z[1])%2==1
def divpi(z): return ((z[0]+z[1])//2,(z[1]-z[0])//2)
def key(z): return (z[0]%4, z[1]%4)
def table(gen):
    D={}
    for u in U:
        for g in [(1,0),gen]:
            d=mul(u,g); D[key(d)]=d
    return D
DH=table((2,1)); DM=table((2,-1))
def S(a,D,z):
    w=mul(a,z); d=D[key((-w[0],-w[1]))]; w=(w[0]+d[0],w[1]+d[1])
    while not odd(w): w=divpi(w)
    return w
def norms(a,D,z,n=60):
    out=[]
    for _ in range(n):
        out.append(N(z)); z=S(a,D,z)
    return out
H=(5,2); M=(5,-2)
vars=[(mul(u,H),DH,'H') for u in U]+[(mul(u,M),DM,'M') for u in U]
bad=0; tested=0
for a in range(-60,61):
    for b in range(-60,61):
        z=(a,b)
        if not odd(z): continue
        ref=norms(H,DH,z); refc=norms(H,DH,(a,-b))
        for al,D,f in vars:
            tested+=1
            if norms(al,D,z)!=(ref if f=='H' else refc): bad+=1
print('comparisons',tested,'mismatches',bad)
