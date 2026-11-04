# exhaustive small check vs Euler criterion: all primary b with N(b) <= NB, a in box |a|<=A
from alg import *
NB=600; A=8
# gaussian primes (primary) with norm <= NB
primes=[]
for x in range(-30,31):
    for y in range(-30,31):
        n=x*x+y*y
        if n<2 or n>NB or (x+y)%2==0: continue
        z=(x,y)
        if is_primary(z) and (isprime(n) or (y==0 and isprime(-x) and (-x)%4==3)):
            primes.append(z)
def factor(b):
    fs=[]; n=N(b)
    for P in primes:
        while n>1 and N(P)<=n and gmod(b,P)==(0,0):
            b=rdiv(b,P); fs.append(P); n=N(b)
    assert n==1,(b)
    return fs
mods=[]
for x in range(-30,31):
    for y in range(-30,31):
        if (x+y)%2 and is_primary((x,y)) and N((x,y))<=NB: mods.append((x,y))
bad={k:0 for k in 'WHES'}; n=0
for b in mods:
    fb=factor(b)
    for u in range(4):
        bb=mul(b,ipow(u))
        for a0 in range(-A,A+1):
            for a1 in range(-A,A+1):
                if (a0,a1)==(0,0): continue
                ref=direct((a0,a1),fb); n+=1
                for k,f in (('W',sym_W),('H',sym_H),('E',sym_E),('S',sym_S)):
                    if f((a0,a1),bb)!=ref: bad[k]+=1
print("primary moduli",len(mods),"cases",n,"mismatches",bad)
