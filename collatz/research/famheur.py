"""Heuristic number of cycles summed over n (families.md, Section 3): for each n, V within 3 of n log2 N(alpha) and each unit,
E = 2^((k-1) n) C(V - k n + n - 1, n - 1) / (4 n N(den)), N(den) = 2^V rel^2; totals for n up to 100, 300, 1000, 3000, 20000.
usage: python3 famheur.py   (about 2 minutes)"""
import sys, math, cmath
sys.path.insert(0,'.')
import famtheory as F
def Erel(k,a,n,V,eps):
    # heuristic in logs, avoiding big ints: N(den)=2^V*rel^2
    A=math.hypot(*a); ang=n*math.atan2(a[1],a[0])-V*math.pi/4-math.atan2(eps[1],eps[0])
    r=math.exp(n*math.log(A)-V*math.log(2)/2); rel=abs(1-r*cmath.exp(1j*ang))
    e=V-k*n
    if e<0: return 0
    lg=(k-1)*n*math.log(2)+F.logC(e+n-1,n-1)-math.log(4*n)-V*math.log(2)-2*math.log(rel)
    return math.exp(lg)
for k,a in [(7,(-13,0)),(7,(-15,0)),(7,(-15,-2)),(7,(-11,-10)),(6,(-9,0)),(6,(-11,0))]:
    a2=a[0]**2+a[1]**2; x=math.log2(a2)-k
    tot={}; 
    for n in range(1,20001):
        Vc=n*math.log2(a2)
        s=0
        for V in range(max(k*n,int(Vc)-3),int(Vc)+5):
            for eps in F.UNITS: s+=Erel(k,a,n,V,eps)
        for lim in (100,300,1000,3000,20000):
            if n<=lim: tot[lim]=tot.get(lim,0)+s
    print(k,a,{l:round(v,1) for l,v in tot.items()})
