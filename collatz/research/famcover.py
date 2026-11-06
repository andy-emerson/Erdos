"""Coverage of the one-element bound (families.md, Section 5): for each k = 7 rule, the largest N such that every
n <= N has maxbound_one(n) <= T, for several T; and the largest bounds for n <= 2000.  usage: python3 famcover.py"""
import sys; sys.path.insert(0,'.')
import famtheory as F
for k,a,dm in [(7,(-13,0),7),(7,(-15,0),7),(7,(-15,-2),7),(7,(-11,-10),7)]:
    rows=[]
    for n in range(1,2001):
        b,V,e=F.maxbound_one(k,a,n,dm); rows.append((b,n,V,e))
    # for thresholds: largest N such that all n<=N have bound <= T
    for T in (1.3e7,1.65e7,2.85e7,1e8,7e8,1e9):
        N=0
        for b,n,V,e in rows:
            if b>T: break
            N=n
        print(k,a,f"T={T:.3g}: all n<={N} covered; first exceed:", rows[N] if N<len(rows) else None)
    top=sorted(rows,reverse=True)[:6]; print('  largest bounds n<=2000:',[(f"{b:.2g}",n,V,e) for b,n,V,e in top])
