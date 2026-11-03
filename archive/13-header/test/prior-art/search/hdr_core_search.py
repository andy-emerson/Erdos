# Gray header add: given m1, m2, t (= e1 xor e2), L (= -(m1<u m2)), b (= t and L), is there a <=4-op program for the
# magnitude r?  Samples: random 64-bit, all 16 direction pairs, with zeros on cross axes (valid cases only).
import random, sys
random.seed(3)
F=(1<<64)-1
def sx(b): return (b|(F^0xFF)) if b&0x80 else b
G=[0x00,0x40,0xFF,0xBF]
smp=[]
for d1 in range(4):
    for d2 in range(4):
        for k in range(6):
            m1=random.getrandbits(62); m2=random.getrandbits(62)
            if k%2: m1,m2=m2,m1
            if (d1^d2)&1:                    # different axes: one operand zero
                if k<3: m1=0
                else: m2=0
            t=sx(G[d1])^sx(G[d2]); L=(0-int(m1<m2))&F; b=t&L
            same=(d1==d2); opp=(d1^d2)==2
            r=m1+m2 if not opp else abs(m1-m2)
            smp.append((m1,m2,t,L,b,r))
tgt=tuple(s[5] for s in smp)
V0={}
for i,nm in enumerate(("m1","m2","t","L","b")): V0.setdefault(tuple(s[i] for s in smp),nm)
for c in (1,F): V0.setdefault(tuple([c]*len(smp)),hex(c))
BIN=[("^",lambda x,y:x^y),("&",lambda x,y:x&y),("|",lambda x,y:x|y),("+",lambda x,y:(x+y)&F),("-",lambda x,y:(x-y)&F),("andn",lambda x,y:x&~y&F)]
def gen(A,B,dst,known):
    for va,ea in A.items():
        for vb,eb in B.items():
            for nm,f in BIN:
                v=tuple(f(x,y) for x,y in zip(va,vb))
                if v not in known and v not in dst: dst[v]="(%s %s %s)"%(ea,nm,eb)
V1={}; gen(V0,V0,V1,V0); known={**V0,**V1}
V2={}; gen(V1,V0,V2,known); gen(V0,V1,V2,known); known.update(V2)
V3={}; gen(V2,V0,V3,known); gen(V0,V2,V3,known); gen(V1,V1,V3,known); known.update(V3)
print("sizes",len(V0),len(V1),len(V2),len(V3))
for l,D in ((1,V1),(2,V2),(3,V3)):
    if tgt in D: print("hit",l,D[tgt]); sys.exit()
lv={}
for D,l in ((V0,0),(V1,1),(V2,2),(V3,3)):
    for v in D: lv.setdefault(v,l)
hits=[]
for B,lb in ((V0,0),(V1,1)):
    for vb,eb in B.items():
        for nm,inv in (("^",lambda t,b:t^b),("+",lambda t,b:(t-b)&F),("-",lambda t,b:(t+b)&F),("r-",lambda t,b:(b-t)&F)):
            va=tuple(inv(x,y) for x,y in zip(tgt,vb))
            if lv.get(va,9)<=3-lb: hits.append("(%s %s %s)"%(known[va],nm,eb))
for A,B in ((V3,V0),(V2,V1)):
    for vb,eb in B.items():
        for va,ea in A.items():
            for nm,f in (BIN[1],BIN[2],BIN[5]):
                if tuple(f(x,y) for x,y in zip(va,vb))==tgt or tuple(f(y,x) for x,y in zip(va,vb))==tgt: hits.append("(%s %s %s)"%(ea,nm,eb))
print("4-op hits",len(hits)); print("\n".join(sorted(set(hits),key=len)[:10]))
