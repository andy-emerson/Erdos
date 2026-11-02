# Ideas 4 + 6: the 2-bit direction counter rides on whichever part of the fraction is odd.
# Numbers: unsigned n, positive D, direction d mod 4, never both n and D even. The ring fraction is n / (D w^d),
# w = 1 + 2^(N-2):  D odd -> words (n, D w^d)  [counter on the denominator, n plain, full width]
#                   D even -> words (n w^-d, D) [n odd, counter on the numerator, D plain, full width]
# The part carrying the counter must be below 2^(N-2). Multiply: raw word products, then shift out the common factors
# of 2 of both words (idea 6's cheap piece), then read the result by the same rule. Cases by where each operand's
# counter sits.
from fractions import Fraction as F
from collections import Counter
from math import gcd
import sys
FIX=len(sys.argv)>2
N=int(sys.argv[1]); M=1<<N; Q=1<<(N-2); w=1+Q; wi=pow(w,-1,M)
def enc(n,D,d):
    if D%2: return (n%M,(D*pow(w,d,M))%M)
    return ((n*pow(wi,d,M))%M, D%M)
def dec(nw,dw):
    if dw%2:                                   # counter on the denominator
        D=dw%Q; x=(dw>>(N-2))%4; d=(x*D)%4     # D odd: D^-1 = D mod 4
        return (nw, D, d)
    if nw%2==0: return None                    # both even: not a valid form
    n=nw%Q; x=(nw>>(N-2))%4; d=(-x*n)%4
    return (n, dw, d)
def ctz(x): return (x&-x).bit_length()-1 if x else N
def value(t):
    if t is None: return None
    n,D,d=t
    if D==0: return None
    q=F(n,D); return (0,) if q==0 else (q,d)
c=Counter()
nums=[(n,D,d) for n in range(0,M) for D in range(1,M) for d in range(4)
      if not (n%2==0 and D%2==0) and ((D%2==1 and D<Q) or (D%2==0 and n<Q and n%2==1))]
import random; random.seed(1)
pairs=[(x,y) for x in nums for y in nums] if len(nums)<700 else [(random.choice(nums),random.choice(nums)) for _ in range(400000)]
for (n1,D1,d1),(n2,D2,d2) in pairs:
    tn,tD=n1*n2,D1*D2
    g=gcd(tn,tD) if tn else tD
    rn,rD=tn//g if tn else 0,tD//g if tn else 1
    # the true result must be storable: drop common 2s only, and the part carrying the counter must fit
    k=min(ctz(tn) if tn else N, ctz(tD)); sn_,sD=tn>>k,tD>>k
    if sn_>=M or sD>=M: continue
    if sD%2==1 and sD>=Q: continue
    if sD%2==0 and sn_>=Q: continue
    a=enc(n1,D1,d1); b=enc(n2,D2,d2)
    P=(a[0]*b[0])%M; R=(a[1]*b[1])%M
    if FIX and (D1%2)!=(D2%2):                 # mixed: move the denominator's counter to the numerator side
        dw=a[1] if D1%2 else b[1]              # the operand whose counter is on its (odd) denominator
        Dd=dw%Q; x=((dw>>(N-2))*Dd)%4          # its direction
        u=(1+Q*((-x)%4))%M                     # w^-x = 1 + 2^(N-2) * (-x mod 4)
        P=(P*u)%M; R=(R*u)%M
    k=min(ctz(P),ctz(R)); P>>=k; R>>=k
    got=value(dec(P,R)); want=(0,) if tn==0 else (F(tn,tD),(d1+d2)%4)
    case=('den' if D1%2 else 'num')+'/'+('den' if D2%2 else 'num')
    c[(case,'right' if got==want else 'wrong')]+=1
print(N,len(nums),"numbers;",dict(sorted(c.items())))
