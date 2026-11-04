from alg import *
from math import log2, sqrt
# proven bound for H: x_{k+1} <= min(x_{k-1}/2^jmin, 2^(1-c/2) x_k) + x_k/sqrt2, from x0=x1=2^31.5
for c in (6,8,10,12):
    jmin=max(2,c>>1)
    x0=x1=2**31.5; k=0
    while x1>=sqrt(5):
        x0,x1=x1, min(x0/2**jmin, 2**(1-c/2)*x1)+x1/sqrt(2); k+=1
    print("H c=%d proven steps <= %d"%(c,k))
# empirical
random.seed(5)
pairs=[]
for _ in range(3000):
    while True:
        a=(random.randrange(-2**31,2**31),random.randrange(-2**31,2**31))
        b=(random.randrange(-2**31,2**31),random.randrange(-2**31,2**31))
        if (b[0]+b[1])%2: break
    pairs.append((a,b))
def run(name,f):
    st=[]; res=[f(a,b,st) for a,b in pairs]
    print("%-14s mean %.2f max %d"%(name,sum(st)/len(st),max(st))); return res
ref=run("W eps",sym_W)
for c in (6,8,10,12):
    r=run("H c=%d"%c, lambda a,b,st,c=c: sym_H(a,b,st,c=c)); assert r==ref
run("E Euclid",sym_E)
for d in (1,2,3,4):
    r=run("S delta=%d"%d, lambda a,b,st,d=d: sym_S(a,b,st,delta=d)); assert r==ref
print("nonzero symbols", sum(r is not None for r in ref))
