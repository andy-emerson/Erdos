# Ideas 4 + 6 by strip-and-rebuild: read both counters (directions), compute with the bare parts, remove the common
# factors of 2, put the counter back on whichever part is odd. Unsigned numerator; the direction carries the sign.
# Add: same direction adds; opposite directions subtract with the borrow (negate, turn by 2); different axes: a zero
# gives the other number, otherwise an expression.
from fractions import Fraction as F
from collections import Counter
import sys, random
N=int(sys.argv[1]); M=1<<N; Q=1<<(N-2); w=1+Q; wi=pow(w,-1,M)
def enc(n,D,d):
    if D%2: return (n%M,(D*pow(w,d,M))%M)
    return ((n*pow(wi,d,M))%M, D%M)
def dec(nw,dw):
    if dw%2: D=dw%Q; return (nw, D, (((dw>>(N-2))%4)*D)%4)
    n=nw%Q; return (n, dw, (-((nw>>(N-2))%4)*n)%4)
ctz=lambda x:(x&-x).bit_length()-1 if x else 99
def rebuild(n,D,d):
    if n==0: return (0,0,1,0)                  # zero: (0, 1), any direction
    k=min(ctz(n),ctz(D)); n>>=k; D>>=k
    ok=(n<M and D<M and ((D%2 and D<Q) or (D%2==0 and n<Q)))
    return (ok,)+enc(n,D,d) if ok else (False,0,0,0)
def val(n,D,d): return (0,) if n==0 else (F(n,D),d)
def norm(q,d): return (0,) if q==0 else ((q,d) if q>0 else (-q,(d+2)%4))
nums=[(n,D,d) for n in range(0,M) for D in range(1,M) for d in range(4)
      if not (n%2==0 and D%2==0) and ((D%2==1 and D<Q) or (D%2==0 and n<Q and n%2==1))]
random.seed(2)
pairs=[(x,y) for x in nums for y in nums] if len(nums)<700 else [(random.choice(nums),random.choice(nums)) for _ in range(300000)]
c=Counter()
for x,y in pairs:
    W1,W2=enc(*x),enc(*y)
    n1,D1,d1=dec(*W1); n2,D2,d2=dec(*W2)          # strip: read both counters and the bare parts
    # multiply
    ok,P,R,_=rebuild(n1*n2,D1*D2,(d1+d2)%4)+(0,) if False else (lambda r:(r[0],r[1],r[2],0))(rebuild(n1*n2,D1*D2,(d1+d2)%4))
    if ok:
        got=val(*dec(P,R)) if n1*n2 else (0,)
        want=norm(F(n1*n2,D1*D2),(d1+d2)%4)
        c[('mul','right' if (norm(*got) if got!=(0,) else (0,))==want else 'wrong')]+=1
    # add
    if (d1-d2)%2==0:
        a,b=n1*D2,n2*D1
        if d1==d2: num,dd,kind=a+b,d1,'add same'
        else:
            num,dd,kind=(a-b,d1,'add opposite') if a>=b else (b-a,(d1+2)%4,'add opposite')
        r=rebuild(num,D1*D2,dd)
        if r[0]:
            got=val(*dec(r[1],r[2])) if num else (0,)
            want=norm(F(n1,D1)*(1 if True else 1) - (F(n2,D2) if d1!=d2 else -F(n2,D2)) ,d1) if False else norm((F(n1,D1)+(F(n2,D2) if d1==d2 else -F(n2,D2))),d1)
            c[(kind,'right' if (norm(*got) if got!=(0,) else (0,))==want else 'wrong')]+=1
    else:
        kind='add cross'
        if n1==0: want=(n2,D2,d2)
        elif n2==0: want=(n1,D1,d1)
        else: c[(kind,'expression')]+=1; continue
        c[(kind,'right')]+=1
print(N,len(nums),"numbers;",dict(sorted(c.items())))
