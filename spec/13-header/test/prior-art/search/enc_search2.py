# Header encodings with E(d+2) = ~E(d) (lb-loaded headers xor to an exact mask on one axis).
# Shortest program (<= 3 ops, 64-bit regs, sign-extended byte inputs) whose LOW BYTE is E(d1+d2) / E(d1-d2).
import sys
F=(1<<64)-1
def sx(b): return (b-256)&F if b&0x80 else b
def sar(x,k): return ((x-(1<<64) if x>>63 else x)>>k)&F
P=[(a,b) for a in range(4) for b in range(4)]
BIN=[("^",lambda x,y:x^y),("&",lambda x,y:x&y),("|",lambda x,y:x|y),("+",lambda x,y:(x+y)&F),("-",lambda x,y:(x-y)&F)]
SH=(1,2,6,7,8,56,57,63)
UN=[("shl%d"%k,(lambda k:lambda x:(x<<k)&F)(k)) for k in SH]+[("shr%d"%k,(lambda k:lambda x:x>>k)(k)) for k in SH]+[("sar%d"%k,(lambda k:lambda x:sar(x,k))(k)) for k in SH]
CONSTS=(1,0x3F,0x40,0x7F,0x80,0xC0,0xFF,F)
def search(E,op):
    want=tuple(E[op(a,b)%4] for a,b in P)
    lo=lambda v: tuple(x&0xFF for x in v)
    V0={tuple(sx(E[a]) for a,b in P):"e1", tuple(sx(E[b]) for a,b in P):"e2"}
    C={tuple([c]*16):hex(c) for c in CONSTS}
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
    base=dict(V0); base.update(C)
    V1={}; gen(V0,base,V1,base); gen(C,V0,V1,base); un(V0,V1,base)
    for v,e in V1.items():
        if lo(v)==want: return 1,e
    known=dict(base); known.update(V1)
    V2={}; gen(V1,base,V2,known); gen(base,V1,V2,known); un(V1,V2,known)
    for v,e in V2.items():
        if lo(v)==want: return 2,e
    # 3 ops: (V2 op base), (base op V2), (V1 op V1), un(V2): only low byte matters
    for A,B in ((V2,base),(base,V2),(V1,V1)):
        for va,ea in A.items():
            for vb,eb in B.items():
                for nm,f in BIN:
                    if tuple(f(x,y)&0xFF for x,y in zip(va,vb))==want: return 3,"(%s %s %s)"%(ea,nm,eb)
    for va,ea in V2.items():
        for nm,f in UN:
            if tuple(f(x)&0xFF for x in va)==want: return 3,"%s(%s)"%(nm,ea)
    return None
XS=[int(v,16) for v in sys.argv[2].split(",")]; which=sys.argv[1]; op=(lambda a,b:a+b) if which=="mul" else (lambda a,b:a-b)
seen=set(); out=[]
for x in range(256):
    for y in range(256):
        E=[x,y,x^0xFF,y^0xFF]
        if len(set(E))<4: continue
        # identity must be E[0]; canonical: skip y > ~y duplicates (E(1)<->E(3) swap is conjugation, different encoding) keep all
        if x not in XS: continue
        r=search(E,op)
        if r: out.append((r[0],hex(x),hex(y),r[1])); print(out[-1],flush=True)
print("best:",sorted(out)[:5] if out else "none <= 3 ops")
