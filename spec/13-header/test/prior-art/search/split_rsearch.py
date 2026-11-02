# Split layout: is there a <=4-op program for the whole result r = p | D (mul) or q | D (div) from
# w1, w2, x1 = w1&F, x2 = w2&F, p (= 2*m1*m2, or 2*q) and constants?  Final op: xor/add/sub by lookup; and/or/andn direct over V2 x V1.
import random, sys
random.seed(2)
F64=(1<<64)-1; S=1<<63; F=S-2; K=S+1
def sar(x,k): return ((x-(1<<64) if x>>63 else x)>>k)&F64
which=sys.argv[1]
smp=[]
for d1 in range(4):
    for d2 in range(4):
        for _ in range(3):
            if which=="mul": m1=random.getrandbits(30); m2=random.getrandbits(30); pr=m1*m2
            else: m2=random.getrandbits(30) or 1; q=random.getrandbits(30); m1=q*m2; pr=q
            dd=(d1+d2)%4 if which=="mul" else (d1-d2)%4
            w1=((d1>>1)<<63)|(m1<<1)|(d1&1); w2=((d2>>1)<<63)|(m2<<1)|(d2&1)
            smp.append((w1,w2,w1&F,w2&F,2*pr,((dd>>1)<<63)|(2*pr)|(dd&1)))
tgt=tuple(s[5] for s in smp)
V0={}
for i,nm in enumerate(("w1","w2","x1","x2","p")): V0.setdefault(tuple(s[i] for s in smp),nm)
for c in (K,F,S,1,F64): V0.setdefault(tuple([c]*len(smp)),hex(c))
BIN=[("^",lambda x,y:x^y),("&",lambda x,y:x&y),("|",lambda x,y:x|y),("+",lambda x,y:(x+y)&F64),("-",lambda x,y:(x-y)&F64),("andn",lambda x,y:x&~y&F64)]
UN=[(n,f) for k in (1,62,63) for n,f in (("shl%d"%k,(lambda k:lambda x:(x<<k)&F64)(k)),("shr%d"%k,(lambda k:lambda x:x>>k)(k)),("sar%d"%k,(lambda k:lambda x:sar(x,k))(k)))]+[("rol1",lambda x:((x<<1)|(x>>63))&F64),("ror1",lambda x:((x>>1)|(x<<63))&F64)]
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
V1={}; gen(V0,V0,V1,V0); un(V0,V1,V0)
known=dict(V0); known.update(V1)
V2={}; gen(V1,V0,V2,known); gen(V0,V1,V2,known); un(V1,V2,known)
known.update(V2)
print("sizes",len(V0),len(V1),len(V2))
for lvl,D in ((1,V1),(2,V2)):
    if tgt in D: print("hit at",lvl,D[tgt]); sys.exit()
# level 3 stored (for 4-op lookups); also check
V3={}; gen(V2,V0,V3,known); gen(V0,V2,V3,known); gen(V1,V1,V3,known); un(V2,V3,known)
print("V3",len(V3))
if tgt in V3: print("hit at 3", V3[tgt]); sys.exit()
hits=[]
# 4 ops = op(A,B) with ops(A)+ops(B)=3: (V3,V0) (V2,V1) and unary(V3).  xor/add/sub by lookup.
allA={**V0,**V1,**V2,**V3}
lv={}
for D,l in ((V0,0),(V1,1),(V2,2),(V3,3)):
    for v in D: lv.setdefault(v,l)
for B,lb in ((V0,0),(V1,1)):
    for vb,eb in B.items():
        need=3-lb
        for nm,inv in (("^",lambda t,b:t^b),("+",lambda t,b:(t-b)&F64),("-",lambda t,b:(t+b)&F64),("r-",lambda t,b:(b-t)&F64)):
            va=tuple(inv(t,b) for t,b in zip(tgt,vb))
            if lv.get(va,9)<=need: hits.append("(%s %s %s)"%(allA[va],nm,eb))
# or/and/andn with B in V0 or V1 and A in V3/V2 (direct)
for A,B in ((V3,V0),(V2,V1)):
    for vb,eb in B.items():
        for va,ea in A.items():
            for nm,f in BIN[1:3]+BIN[5:]:
                if tuple(f(x,y) for x,y in zip(va,vb))==tgt or tuple(f(y,x) for x,y in zip(va,vb))==tgt: hits.append("(%s %s %s)"%(ea,nm,eb))
print("4-op hits:",len(hits)); print("\n".join(sorted(set(hits),key=len)[:15]))
