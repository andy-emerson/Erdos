from g import *
from math import sqrt
def pim(m):
    P=(1,0)
    for _ in range(m): P=mul(P,(1,1))
    return P
def rho(m):
    P=pim(m); n=N(P)
    best={}
    R=int(sqrt(n))+3
    for x in range(-R,R+1):
        for y in range(-R,R+1):
            if (x+y)%2==0: continue
            r=gmod((x,y),P)
            # canonical class key: reduce to a fixed rep
            k=r
            # use a canonical form: find rep via Babai then normalise by trying lattice shifts
            key=None
            for dx in range(-1,2):
                for dy in range(-1,2):
                    c=add(r,mul((dx,dy),P))
                    if key is None or c<key: pass
            # simpler: class key = (x,y) mod lattice via Hermite normal form
            best.setdefault(hnf_key((x,y),P),[]).append(x*x+y*y)
    return sqrt(max(min(v) for v in best.values())), len(best)
def hnf_key(z,P):
    # lattice spanned by P and iP; Z[i]/(P) ~ Z/N(P) when P primitive, else use pair reduction
    # generic: reduce using integer lattice basis HNF
    a,b=P; v1=(a,b); v2=(-b,a)
    # HNF of rows v1,v2
    import math
    M=[list(v1),list(v2)]
    # column 0 gcd
    (x1,y1),(x2,y2)=M
    g,s,t=egcd(x1,x2)
    r1=(g, s*y1+t*y2)
    r2=(0, (x1*y2-x2*y1)//g)
    d=abs(r2[1])
    x,y=z
    k=x//g; x-=k*g; y-=k*r1[1]
    y%=d
    return (x,y)
def egcd(a,b):
    if b==0: return (abs(a), 1 if a>=0 else -1, 0)
    g,s,t=egcd(b,a%b); return (g,t,s-(a//b)*t)
if __name__=="__main__":
 for m in range(1,14):
  r,c=rho(m); print(m, c, round(r,4), "rho/|pi|^(m-1) =", round(r/2**((m-1)/2),4), " rho/|pi|^(m-2)=",round(r/2**((m-2)/2),4))
