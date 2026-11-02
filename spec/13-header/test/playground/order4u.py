# Idea 4 with an UNSIGNED numerator: the denominator word D * w^d (w = 1 + 2^(N-2)) carries the whole direction,
# the numerator n is a plain unsigned magnitude. Value (n/D) i^d. Odd D only.
#   multiply: raw (n1*n2, w1*w2)
#   add, same direction: (n1*D2 + n2*D1, w1*D2)            (one tag kept, as in the fixed add)
#   add, opposite:       diff = n1*D2 - n2*D1 with its borrow b; b=1 -> negate diff and turn the denominator by w^2
# MODE=dbl: numerator stored doubled (2n) and the add done RAW on whole words (no tag stripping), to see whether
# doubling alone removes the need for the fix.
from fractions import Fraction as F
from collections import Counter
import sys
N=int(sys.argv[1]); MODE=sys.argv[2] if len(sys.argv)>2 else 'u'
M=1<<N; T=1<<(N-1); Q=1<<(N-2); w=1+Q; LOW=Q-1
enc=lambda D,d:(D*pow(w,d,M))%M
dec={enc(D,d):(D,d) for D in range(1,Q,2) for d in range(4)}
def norm(q,d): return (0,) if q==0 else ((q,d) if q>0 else (-q,(d+2)%4))
c=Counter()
for D1 in range(1,Q,2):
 for D2 in range(1,Q,2):
  for d1 in range(4):
   for d2 in range(4):
    for n1 in range(0,M):
     for n2 in range(0,M):
      w1,w2=enc(D1,d1),enc(D2,d2)
      if MODE=='u':
        if D1*D2<Q and n1*n2<M:
          r=dec.get((w1*w2)%M); ok = r and norm(F(n1*n2,r[0]),r[1])==norm(F(n1*n2,D1*D2),(d1+d2)%4)
          c[('mul','right' if ok else 'wrong')]+=1
        if (d1-d2)%2==0 and D1*D2<Q:
          if d1==d2:
            if n1*D2+n2*D1>=M: continue
            num=(n1*D2+n2*D1)%M; den=(w1*D2)%M; t=norm(F(n1*D2+n2*D1,D1*D2),d1); k='add same'
          else:
            a,b_=n1*D2,n2*D1
            if a>=M or b_>=M: continue
            diff=(a-b_)%M; bor=a<b_
            num=(-diff)%M if bor else diff; den=(w1*D2*(1+T if bor else 1))%M
            t=norm(F(a-b_,D1*D2),d1); k='add opposite'
          r=dec.get(den); ok = r and norm(F(num,r[0]),r[1])==t
          c[(k,'right' if ok else 'wrong')]+=1
      else:  # doubled numerator, raw add on whole words
        if (d1-d2)%2==0 and d1==d2 and D1*D2<Q and 2*(n1*D2+n2*D1)<M:
          num=(2*n1*w2+2*n2*w1)%M; den=(w1*w2)%M
          r=dec.get(den); t=norm(F(n1*D2+n2*D1,D1*D2),d1)
          ok = r and num%2==0 and norm(F(num//2,r[0]),r[1])==t
          c[('raw add, doubled numerator, same direction','right' if ok else 'wrong')]+=1
print(N,MODE,dict(c))
