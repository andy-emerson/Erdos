"""Random starts with |x|, |y| <= 1000 for alpha = 9, 11 at k = 6 and each digit set: iterate S until |z|^2 > 2^120
(or 3000 odd steps) and report which line the escaped point is near (families.md, Section 6).  usage: python3 famescape.py"""
import sys, random; sys.path.insert(0,'.')
import rules as R, exact as E
from collections import Counter
sets={'A':[(0,1),(0,3),(1,2),(1,-4),(1,-2),(2,3),(2,-3),(3,-4)],'B':[(0,1),(0,3),(1,2),(1,-4),(1,-2),(2,3),(2,-3),(3,4)],
      'C':[(0,1),(0,3),(1,2),(1,4),(1,-2),(2,3),(2,-3),(3,-4)],'D':[(0,1),(0,3),(1,2),(1,4),(1,-2),(2,3),(2,-3),(3,4)]}
random.seed(1)
for al in [(-9,0),(-11,0)]:
  for nm,G in sets.items():
    D=[R.mul(u,g) for g in G for u in R.UNITS]; S=E.make_S(al,D,6)
    lines=Counter(); esc=0; tried=0
    for t in range(300):
        z=(random.randint(-1000,1000),random.randint(-1000,1000))
        if (z[0]+z[1])%2==0: continue
        tried+=1; vs=[]
        for s in range(3000):
            z,v=S(z); vs.append(v)
            if R.norm(z)>2**120: break
        if R.norm(z)>2**120:
            esc+=1
            # describe tail: which of x, y, x-y, x+y is small (relative)
            x,y=z; d={'x':abs(x),'y':abs(y),'x-y':abs(x-y),'x+y':abs(x+y)}
            m=min(d,key=d.get)
            # normalise by units: report the small coordinate value
            lines[(m, d[m] if d[m]<100 else "big")]+=1
    print(al,nm,'tried',tried,'escaped',esc, sorted(lines.items(), key=str))
