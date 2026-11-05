# alpha = 2-i, modulus 2i: where do orbits enter the trivial cycle 1 -> -i -> -1 -> 1?
from collections import Counter
def mul(x,y): return (x[0]*y[0]-x[1]*y[1], x[0]*y[1]+x[1]*y[0])
def odd(z): return (z[0]+z[1])%2==1
def divpi(z): return ((z[0]+z[1])//2,(z[1]-z[0])//2)
A=(2,-1)
def S(z):
    w=mul(A,z); d=(0,1) if z[0]%2==1 else (1,0)   # digit i for a odd, b even; 1 for a even, b odd
    w=(w[0]+d[0],w[1]+d[1])
    while not odd(w): w=divpi(w)
    return w
C={(1,0),(0,-1),(-1,0)}
c=Counter(); viai=0; tot=0
for a in range(-300,301):
    for b in range(-300,301):
        z=(a,b)
        if not odd(z) or a*a+b*b>90000 or z in C or z==(0,1): continue
        tot+=1; prev=None; seen_i=False
        while z not in C:
            if z==(0,1): seen_i=True
            prev=z; z=S(z)
        c[z]+=1; viai+=seen_i
print('starts',tot,'enter at',dict(c),'pass through i',viai)
