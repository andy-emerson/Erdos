# Idea 2 alone: numerator stored doubled (2n), denominator word D + T*a (axis in the top bit). Raw machine ops,
# multiply's numerator shifted back once ((n1'*n2') sar 1). 4-bit clock.
from fractions import Fraction as F
from collections import Counter
import sys
N=int(sys.argv[1]) if len(sys.argv)>1 else 4; M=1<<N; T=1<<(N-1)
sn=lambda x:(x%M)-M if (x%M)&T else x%M
sar1=lambda x: sn(x)>>1
dd=lambda w:((w%M)&(T-1),(w%M)>>(N-1))
def val(nw,dw):
    D,a=dd(dw)
    if D==0 or sn(nw)%2: return None
    return (F(sn(nw)//2,D),a)
same=lambda r,t: r is not None and ((r[0]==0 and t[0]==0) or r==t)
c=Counter()
nlo,nhi=-T//2,T//2-1
for n1 in range(nlo,nhi+1):
 for n2 in range(nlo,nhi+1):
  for D1 in range(1,T):
   for D2 in range(1,T):
    for a1 in (0,1):
     for a2 in (0,1):
      w1,w2=(2*n1)%M,(2*n2)%M; d1,d2=D1|a1<<(N-1),D2|a2<<(N-1)
      ax={(0,0):'r·r',(0,1):'r·i',(1,0):'i·r',(1,1):'i·i'}[(a1,a2)]
      if D1*D2<T:
        tn=n1*n2*(-1 if a1&a2 else 1)
        if nlo<=tn<=nhi and nlo<=n1*n2<=nhi:
          r=val(sar1(w1)*w2 , d1*d2)
          t=(F(tn,D1*D2),a1^a2)
          ok=same(r,t); numok = r is not None and (abs(r[0])==abs(t[0]))
          c[('mul',ax,'D parity '+str(D1%2)+str(D2%2), 'right' if ok else ('axis or sign wrong' if numok else 'magnitude wrong'))]+=1
        if a1==a2:
          tn=n1*D2+n2*D1
          if nlo<=tn<=nhi:
            r=val(w1*d2+w2*d1, d1*d2); t=(F(tn,D1*D2),a1)
            numok = r is not None and r[0]==t[0]
            ok=same(r,t)
            c[('add',ax.replace('·','+'),'D parity '+str(D1%2)+str(D2%2), 'right' if ok else ('numerator right, axis wrong' if numok else 'numerator wrong'))]+=1
for k in sorted(c): print(k, c[k])
