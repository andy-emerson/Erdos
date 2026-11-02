# Gray header: shortest program for the expression flag e = (axes differ) and m1 != 0 and m2 != 0 (0/1),
# from t, m1, m2, L, b, r (the add's values, free) and constants; ops incl. sltu and snez.
import random, sys
random.seed(4)
F=(1<<64)-1
def sx(b): return (b|(F^0xFF)) if b&0x80 else b
G=[0x00,0x40,0xFF,0xBF]
smp=[]
for d1 in range(4):
    for d2 in range(4):
        for k in range(8):
            m1=random.getrandbits(random.choice([1,8,63,64])); m2=random.getrandbits(random.choice([1,8,63,64]))
            if k==0: m1=0
            if k==1: m2=0
            if k==2: m1=m2=0
            if k==3: m2=m1
            t=sx(G[d1])^sx(G[d2]); L=(0-int(m1<m2))&F; b=t&L
            r=(m1+(m2^t)-t)&F; r=((r+b)^b)&F
            e=int(((d1^d2)&1)==1 and m1!=0 and m2!=0)
            smp.append((t,m1,m2,L,b,r,e))
tgt=tuple(s[6] for s in smp)
V0={}
for i,nm in enumerate(("t","m1","m2","L","b","r")): V0.setdefault(tuple(s[i] for s in smp),nm)
for c in (1,0x40,F): V0.setdefault(tuple([c]*len(smp)),hex(c))
BIN=[("^",lambda x,y:x^y),("&",lambda x,y:x&y),("|",lambda x,y:x|y),("+",lambda x,y:(x+y)&F),("-",lambda x,y:(x-y)&F),("sltu",lambda x,y:int(x<y))]
UN=[("snez",lambda x:int(x!=0)),("shr6",lambda x:x>>6),("shr7",lambda x:x>>7),("shr1",lambda x:x>>1),("shr63",lambda x:x>>63)]
def gen(A,B,dst,known):
    for va,ea in A.items():
        for vb,eb in B.items():
            for nm,f in BIN:
                v=tuple(f(x,y) for x,y in zip(va,vb))
                if v not in known and v not in dst: dst[v]="(%s %s %s)"%(ea,nm,eb)
def un(A,dst,known):
    for va,ea in A.items():
        for nm,f in UN:
            v=tuple(f(x) for x in va)
            if v not in known and v not in dst: dst[v]="%s(%s)"%(nm,ea)
lv={}
V0d=dict(V0); known=dict(V0); levels=[V0]
for n in range(1,5):
    new={}
    for i in range(n):
        j=n-1-i
        if i<len(levels) and j<len(levels): gen(levels[i],levels[j],new,known)
    un(levels[n-1],new,known)
    known.update(new); levels.append(new)
    print("level",n,len(new),flush=True)
    if tgt in new: print("HIT",n,new[tgt]); break
    if len(new)>300000: print("stop"); break
