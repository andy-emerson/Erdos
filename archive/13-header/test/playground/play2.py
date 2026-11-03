from fractions import Fraction as F
import sys
def run(N):
    M=1<<N; TOP=1<<(N-1); LO=-TOP; HI=TOP-1; DMAX=TOP-1
    sn=lambda x:(x%M)-M if (x%M)&TOP else x%M
    dd=lambda x:((x%M)&(TOP-1),(x%M)>>(N-1))
    def val(n,dw):
        D,a=dd(dw); return None if D==0 else (F(sn(n),D),a)
    same=lambda r,t: r is not None and ((r[0]==0 and t[0]==0) or r==t)
    stats={'mul ri':[0,0],'mul ii axis':[0,0],'add ii':[0,0],'mul rr':[0,0],'add rr':[0,0]}
    for n1 in range(LO,HI+1):
     for D1 in range(1,DMAX+1):
      for a1 in (0,1):
       d1=D1|(a1<<(N-1))
       for n2 in range(LO,HI+1):
        for D2 in range(1,DMAX+1):
         for a2 in (0,1):
          d2=D2|(a2<<(N-1))
          # multiply
          tn=n1*n2*(-1 if a1 and a2 else 1); tD=D1*D2
          if LO<=tn<=HI and tD<=DMAX:
            r=val(n1*n2,d1*d2); t=(F(tn,tD),a1^a2); ok=same(r,t)
            if a1==a2==0: k='mul rr'; pred=True
            elif a1!=a2:
                k='mul ri'; Dreal=D1 if a1==0 else D2
                pred=(Dreal%2==1) or tn==0
            else:
                k='mul ii axis'
                ok=(r is not None) and (dd(d1*d2)[1]==0 or r[0]==0)   # axis only
                pred=((D1+D2)%2==0) or (n1*n2)%M==0
            stats[k][0]+=1; stats[k][1]+=(ok!=pred)
          # add, same axis
          if a1==a2:
            tn=n1*D2+n2*D1
            if LO<=tn<=HI and tD<=DMAX:
              r=val(n1*d2+n2*d1,d1*d2); t=(F(tn,tD),a1); ok=same(r,t)
              if a1==0: k='add rr'; pred=True
              else:
                k='add ii'; pred=((n1+n2)%2==0 and (D1+D2)%2==1) or (tn==0 and (n1*d2+n2*d1)%M==0)
              stats[k][0]+=1; stats[k][1]+=(ok!=pred)
    print(f"n={N}:", ", ".join(f"{k}: {v[0]} pairs, {v[1]} against the prediction" for k,v in stats.items()))
for N in map(int,sys.argv[1:]): run(N)
