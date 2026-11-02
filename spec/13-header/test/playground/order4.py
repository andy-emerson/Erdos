# Idea 4: the direction as a unit of order 4 on the denominator: word = D * w^d mod 2^N, w = 1 + 2^(N-2).
# Numerator signed. Value (n/D) * i^d. Raw machine ops; the only fix: opposite directions in add, negate n2 and turn
# its denominator by w^2 (= multiply by 1 + 2^(N-1)) before the raw add.
from fractions import Fraction as F
from collections import Counter
import sys
N=int(sys.argv[1]); M=1<<N; T=1<<(N-1); Q=1<<(N-2); w=1+Q
sn=lambda x:(x%M)-M if (x%M)&T else x%M
enc=lambda D,d:(D*pow(w,d,M))%M
dec={}
for D in range(1,Q):
    for d in range(4):
        dec.setdefault(enc(D,d),[]).append((D,d))
def val(nw,dw):
    c=dec.get(dw%M)
    if not c or len(c)>1: return None        # undecodable (an even D, or out of range)
    D,d=c[0]; return (F(sn(nw),D)*1, d)
def norm(v):                                 # (q, d) -> canonical: q>=0 with d, zero as 0
    q,d=v
    if q==0: return (0,)
    if q<0: q,d=-q,(d+2)%4
    return (q,d)
c=Counter()
for D1 in range(1,Q):
 for D2 in range(1,Q):
  for d1 in range(4):
   for d2 in range(4):
    for n1 in range(-T,T):
     for n2 in range(-T,T):
      par=f"D parity {D1%2}{D2%2}"
      w1,w2=enc(D1,d1),enc(D2,d2)
      # multiply
      if D1*D2<Q and -T<=n1*n2<T:
        r=val(n1*n2,w1*w2)
        t=(F(n1*n2,D1*D2),(d1+d2)%4)
        c[('mul',par,'right' if r and norm(r)==norm(t) else 'wrong')]+=1
      # add on one axis
      if (d1-d2)%2==0 and D1*D2<Q:
        m2,ww2=n2,w2
        if d2!=d1: m2,ww2=-n2,(w2*(1+T))%M     # the fix: turn the second operand to the first's direction
        tn=n1*D2+m2*D1
        if -T<=tn<T:
          r=val(n1*ww2+m2*w1,w1*ww2)
          t=(F(tn,D1*D2),d1)
          kind='same direction' if d1==d2 else 'opposite (fixed)'
          c[('add',kind,par,'right' if r and norm(r)==norm(t) else 'wrong')]+=1
print(f"N={N}, w={w}")
for k in sorted(c): print(k,c[k])
