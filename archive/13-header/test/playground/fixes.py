# The signed-numerator / complement-axis-denominator design (numerator: signed n; denominator word: D + T*a),
# raw machine arithmetic plus the predicted corrections, each a computed top-bit flip or a conditional negate.
#   add (same axis a):  num = n1*d2 + n2*d1, then flip num's top bit if a and (n1 xor n2) is odd
#                       den = d1*d2,         then flip den's top bit if a and (D1 xor D2) is even
#   mul:                num = n1*n2, negated if a1 and a2
#                       den = d1*d2, then flip den's top bit by (a1 and D2 even) xor (a2 and D1 even)
from fractions import Fraction as F
from collections import Counter
import sys
N=int(sys.argv[1]); M=1<<N; T=1<<(N-1)
sn=lambda x:(x%M)-M if (x%M)&T else x%M
def val(nw,dw):
    D,a=(dw%M)&(T-1),(dw%M)>>(N-1)
    return None if D==0 else (F(sn(nw),D),a)
same=lambda r,t: r is not None and ((r[0]==0 and t[0]==0) or r==t)
c=Counter()
for n1 in range(-T,T):
 for n2 in range(-T,T):
  for D1 in range(1,T):
   for D2 in range(1,T):
    if D1*D2>=T: continue
    for a1 in (0,1):
     for a2 in (0,1):
      d1,d2=D1|a1<<(N-1),D2|a2<<(N-1)
      # multiply
      tn=n1*n2*(-1 if a1&a2 else 1)
      if -T<=tn<T:
        num=(n1*n2)%M
        if a1&a2: num=(-num)%M
        den=(d1*d2)%M
        flip=((a1&(1-(d2&1)))^(a2&(1-(d1&1))))
        den^=flip<<(N-1)
        c[('mul','right' if same(val(num,den),(F(tn,D1*D2),a1^a2)) else 'WRONG')]+=1
      # add, same axis
      if a1==a2:
        tn=n1*D2+n2*D1
        if -T<=tn<T:
          a=a1
          num=(n1*d2+n2*d1)%M; num^=(a&((n1^n2)&1))<<(N-1)
          den=(d1*d2)%M;       den^=(a&(1^((d1^d2)&1)))<<(N-1)
          c[('add','right' if same(val(num,den),(F(tn,D1*D2),a)) else 'WRONG')]+=1
print(N,dict(c))
