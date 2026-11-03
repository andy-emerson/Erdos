# Is there a <=3-op program for the split layout's product direction D (bits 63 and 0, rest 0) or quotient direction,
# from w1, w2, x1 = w1&F, x2 = w2&F and constants?  Fingerprint on random 64-bit samples covering all 16 direction pairs.
import random, sys
random.seed(1)
F64=(1<<64)-1; S=1<<63; F=S-2; K=S+1
def sar(x,k): return ((x-(1<<64) if x>>63 else x)>>k)&F64
samples=[]
for d1 in range(4):
    for d2 in range(4):
        for _ in range(4):
            m1=random.getrandbits(62); m2=random.getrandbits(62)
            samples.append((d1,d2,((d1>>1)<<63)|(m1<<1)|(d1&1),((d2>>1)<<63)|(m2<<1)|(d2&1)))
def enc_d(d): return ((d>>1)<<63)|(d&1)
which=sys.argv[1]
tgt=tuple(enc_d((d1+d2)%4 if which=="mul" else (d1-d2)%4) for d1,d2,_,_ in samples)
V0={}
def add(dct,v,e):
    if v not in dct: dct[v]=e
for nm,f in (("w1",lambda a,b:a),("w2",lambda a,b:b),("x1",lambda a,b:a&F),("x2",lambda a,b:b&F)):
    add(V0,tuple(f(a,b) for _,_,a,b in samples),nm)
for c in (K,F,S,1,F64):
    add(V0,tuple([c]*len(samples)),hex(c))
BIN=[("^",lambda x,y:x^y),("&",lambda x,y:x&y),("|",lambda x,y:x|y),("+",lambda x,y:(x+y)&F64),("-",lambda x,y:(x-y)&F64),("andn",lambda x,y:x&~y&F64)]
UN=[("shl63",lambda x:(x<<63)&F64),("shl1",lambda x:(x<<1)&F64),("shr63",lambda x:x>>63),("shr1",lambda x:x>>1),("sar63",lambda x:sar(x,63)),("sar1",lambda x:sar(x,1)),("rol1",lambda x:((x<<1)|(x>>63))&F64),("ror1",lambda x:((x>>1)|(x<<63))&F64)]
def step(A,B,out,known):
    for va,ea in A.items():
        for vb,eb in B.items():
            for nm,f in BIN:
                v=tuple(f(x,y) for x,y in zip(va,vb))
                if v not in known and v not in out: out[v]="(%s %s %s)"%(ea,nm,eb)
def unary(A,out,known):
    for va,ea in A.items():
        for nm,f in UN:
            v=tuple(f(x) for x in va)
            if v not in known and v not in out: out[v]="%s(%s)"%(nm,ea)
V1={}; step(V0,V0,V1,V0); unary(V0,V1,V0)
print("V1",len(V1), "hit:", V1.get(tgt))
known=dict(V0); known.update(V1)
V2={}; step(V1,V0,V2,known); step(V0,V1,V2,known); unary(V1,V2,known)
print("V2",len(V2), "hit:", V2.get(tgt))
known.update(V2)
hits=[]
# 3 ops: (V2 op V0), (V0 op V2), (V1 op V1), unary(V2)
for A,B in ((V2,V0),(V0,V2),(V1,V1)):
    for va,ea in A.items():
        for vb,eb in B.items():
            for nm,f in BIN:
                if tuple(f(x,y) for x,y in zip(va,vb))==tgt: hits.append("(%s %s %s)"%(ea,nm,eb))
for va,ea in V2.items():
    for nm,f in UN:
        if tuple(f(x) for x in va)==tgt: hits.append("%s(%s)"%(nm,ea))
print("3-op hits:",len(hits)); print("\n".join(sorted(set(hits),key=len)[:20]))
