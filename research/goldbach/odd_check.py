"""Odd targets: Knill's remark '4+13i is the smallest Gaussian integer not a sum of two Gaussian primes; the smallest real one is 29'.
An odd z = p1+p2 needs one summand even, i.e. an associate of 1+i. Check both the unrestricted and the open-quadrant reading."""
from sympy import isprime
def gp(x,y):
    if x==0: return abs(y)%4==3 and isprime(abs(y))
    if y==0: return abs(x)%4==3 and isprime(abs(x))
    return isprime(x*x+y*y)
units=[(1,1),(-1,1),(-1,-1),(1,-1)]
unr=[];quad=[]
for a in range(-40,41):
    for b in range(-40,41):
        if (a+b)%2==0: continue
        if not any(gp(a-u,b-v) for u,v in units): unr.append((a*a+b*b,a,b))
        if a>=1 and b>=1 and not (a>1 and b>1 and gp(a-1,b-1)): quad.append((a*a+b*b,a,b))
unr.sort(); quad.sort()
print("unrestricted, odd, smallest norms:", unr[:12])
print("first-quadrant a,b>=1 odd not sum of two primes in open quadrant, smallest:", quad[:12])
print("odd real x>0 not a sum (unrestricted):", sorted(a for n,a,b in unr if b==0 and a>0)[:10])
print("first quadrant a,b>=1 unrestricted fails:", [t for t in unr if t[1]>=1 and t[2]>=1][:8])
