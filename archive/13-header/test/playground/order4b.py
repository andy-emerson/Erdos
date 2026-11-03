# Idea 4, add fixed: cross products use the stripped denominators D (low N-2 bits), and the result's denominator is
# w1 * D2 (one tag kept). Opposite directions: negate n2 first. Odd denominators only (even D cannot hold the tag).
from fractions import Fraction as F
from collections import Counter
import sys
N=int(sys.argv[1]); M=1<<N; T=1<<(N-1); Q=1<<(N-2); w=1+Q; LOW=Q-1
sn=lambda x:(x%M)-M if (x%M)&T else x%M
enc=lambda D,d:(D*pow(w,d,M))%M
dec={}
for D in range(1,Q,2):
    for d in range(4): dec[enc(D,d)]=(D,d)
def val(nw,dw):
    c=dec.get(dw%M)
    if not c: return None
    D,d=c; return (F(sn(nw),D),d)
def norm(v):
    q,d=v
    if q==0: return (0,)
    if q<0: q,d=-q,(d+2)%4
    return (q,d)
c=Counter()
for D1 in range(1,Q,2):
 for D2 in range(1,Q,2):
  for d1 in range(4):
   for d2 in range(4):
    if (d1-d2)%2: continue
    for n1 in range(-T,T):
     for n2 in range(-T,T):
      if D1*D2>=Q: continue
      w1,w2=enc(D1,d1),enc(D2,d2)
      m2=n2 if d1==d2 else -n2
      tn=n1*D2+m2*D1
      if not(-T<=tn<T): continue
      num=n1*(w2&LOW)+m2*(w1&LOW); den=w1*(w2&LOW)
      r=val(num,den); t=(F(tn,D1*D2),d1)
      c[('same' if d1==d2 else 'opposite','right' if r and norm(r)==norm(t) else 'wrong')]+=1
print(N,dict(c))
