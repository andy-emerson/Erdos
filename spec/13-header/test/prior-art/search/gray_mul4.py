# Gray header G = [0x00,0x40,0xFF,0xBF]: is there a 4-op program (sign-extended inputs) whose low byte is G(d1+d2)
# (mul) or G(d1-d2) (div)?  Final xor/add/sub found by low-byte lookup into levels 0-3; and/or direct on (V2 x V1).
import sys
F=(1<<64)-1
def sx(b): return (b|(F^0xFF)) if b&0x80 else b
def sar(x,k): return ((x-(1<<64) if x>>63 else x)>>k)&F
G=[0x00,0x40,0xFF,0xBF]
P=[(a,b) for a in range(4) for b in range(4)]
op=(lambda a,b:a+b) if sys.argv[1]=="mul" else (lambda a,b:a-b)
want=tuple(G[op(a,b)%4] for a,b in P)
BIN=[("^",lambda x,y:x^y),("&",lambda x,y:x&y),("|",lambda x,y:x|y),("+",lambda x,y:(x+y)&F),("-",lambda x,y:(x-y)&F),("andn",lambda x,y:x&~y&F)]
SH=(1,6,7,56,57,63)
UN=[("shl%d"%k,(lambda k:lambda x:(x<<k)&F)(k)) for k in SH]+[("shr%d"%k,(lambda k:lambda x:x>>k)(k)) for k in SH]+[("sar%d"%k,(lambda k:lambda x:sar(x,k))(k)) for k in SH]
V0={tuple(sx(G[a]) for a,b in P):"e1",tuple(sx(G[b]) for a,b in P):"e2"}
for c in (1,0x3F,0x40,0x7F,0x80,0xC0,0xFF,F): V0[tuple([c]*16)]=hex(c)
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
lo=lambda v:tuple(x&0xFF for x in v)
levels=[V0]; known=dict(V0)
for n in (1,2,3):
    new={}
    for i in range(n):
        gen(levels[i],levels[n-1-i],new,known)
    un(levels[n-1],new,known); known.update(new); levels.append(new)
    hit=[e for v,e in new.items() if lo(v)==want]
    print("level",n,len(new),"hits",hit[:3],flush=True)
    if hit: sys.exit()
idx={}
for l,D in enumerate(levels):
    for v,e in D.items(): idx.setdefault(lo(v),(l,e))
hits=[]
for lb,B in enumerate(levels[:2]):
    for vb,eb in B.items():
        for nm,inv in (("^",lambda t,b:t^b),("+",lambda t,b:(t-b)&0xFF),("-",lambda t,b:(t+b)&0xFF),("r-",lambda t,b:(b-t)&0xFF)):
            need=tuple(inv(t,b&0xFF) for t,b in zip(want,vb))
            if need in idx and idx[need][0]<=3-lb: hits.append("(%s %s %s)"%(idx[need][1],nm,eb))
for A,B in ((levels[2],levels[1]),(levels[3],levels[0])):
    for va,ea in A.items():
        for vb,eb in B.items():
            for nm,f in (BIN[1],BIN[2],BIN[5]):
                if lo(tuple(f(x,y) for x,y in zip(va,vb)))==want: hits.append("(%s %s %s)"%(ea,nm,eb))
for va,ea in levels[3].items():
    for nm,f in UN:
        if lo(tuple(f(x) for x in va))==want: hits.append("%s(%s)"%(nm,ea))
print("4-op hits",len(hits)); print("\n".join(sorted(set(hits),key=len)[:8]))
