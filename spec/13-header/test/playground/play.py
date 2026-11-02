from fractions import Fraction as F
from collections import Counter
N=4; M=1<<N; TOP=1<<(N-1)
def sn(x): x%=M; return x-M if x&TOP else x          # numerator: signed
def dd(x): x%=M; return (x&(TOP-1), x>>(N-1))         # denominator word: (count D, axis a)
def val(n,dw):
    D,a=dd(dw)
    if D==0: return None
    return (F(sn(n),D), a)                            # value q * i^a
def true_mul(x,y):
    (q1,a1),(q2,a2)=x,y
    q=q1*q2
    if a1 and a2: q=-q
    return (q, a1^a2)
def same(r,t):
    if r is None: return False
    if r[0]==0 and t[0]==0: return True
    return r==t
nums=range(-8,8); dens=[(D,a) for a in (0,1) for D in range(1,8)]
words=[(n%M, D|(a<<(N-1))) for n in nums for (D,a) in dens]
res={'mul':Counter(),'add':Counter()}
for (n1,d1) in words:
  for (n2,d2) in words:
    x=val(n1,d1); y=val(n2,d2)
    D1,a1=dd(d1); D2,a2=dd(d2)
    # multiply: raw machine products, only where the true unreduced parts fit (no overflow)
    tn=sn(n1)*sn(n2)*(-1 if a1 and a2 else 1); tD=D1*D2
    if -8<=tn<=7 and tD<=7:
        r=val(n1*n2, d1*d2)
        key=(a1,a2,'n1 odd' if sn(n1)%2 else 'n1 even', 'D1 odd' if D1%2 else 'D1 even','D2 odd' if D2%2 else 'D2 even')
        res['mul'][(key, same(r,true_mul(x,y)))]+=1
    # add, same axis only: raw (n1*d2 + n2*d1) / (d1*d2)
    if a1==a2:
        tn=sn(n1)*D2+sn(n2)*D1
        if -8<=tn<=7 and tD<=7:
            r=val(n1*d2+n2*d1, d1*d2)
            t=(x[0]+y[0], a1)
            key=(a1,a2,'n1 odd' if sn(n1)%2 else 'n1 even','n2 odd' if sn(n2)%2 else 'n2 even','D1 odd' if D1%2 else 'D1 even','D2 odd' if D2%2 else 'D2 even')
            res['add'][(key, same(r,t))]+=1
for op in res:
    print("==",op)
    keys=sorted({k for k,_ in res[op]})
    for k in keys:
        ok=res[op][(k,True)]; bad=res[op][(k,False)]
        if ok+bad: print(k, "right",ok,"wrong",bad)
