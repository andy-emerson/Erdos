# truncation property of the valuation-driven sequence S: perturb inputs by (1+i)^M * X,
# find the first step whose (q, t, s) differs, report min over trials of 2*V - M,
# where V = unnormalised valuation reached BEFORE that step (v of the current B).
from alg import *
def seq(A,B,delta,cap=200):
    out=[]; g=1; v=1  # v = virtual valuation of B (A at 0)
    vB=1
    while B!=(1,0) and len(out)<cap:
        q=pimod_q(A,B,g+delta); r=sub(A,mul(q,B))
        if r==(0,0): out.append((q,None,None,vB)); break
        t,w=strip(r); s,C=primary(w)
        out.append((q,t,s,vB))
        A,B=B,C; g=t-g; vB=vB+g
    return out
P=lambda m: [(1,0)] if False else None
def pim(m):
    z=(1,0)
    for _ in range(m): z=mul(z,(1,1))
    return z
random.seed(7)
for delta in (1,2,4):
  worst=10**9
  for trial in range(1500):
    while True:
        A=(random.randrange(-2**40,2**40),random.randrange(-2**40,2**40))
        B=(random.randrange(-2**40,2**40),random.randrange(-2**40,2**40))
        if (A[0]+A[1])%2 and (B[0]+B[1])%2: break
    A=primary(A)[1]; B=primary(B)[1]
    M=random.randint(10,60)
    X=(random.randrange(-99,99),random.randrange(-99,99)); Y=(random.randrange(-99,99),random.randrange(-99,99))
    A2=add(A,mul(pim(M),X)); B2=add(B,mul(pim(M),Y))
    s1=seq(A,B,delta); s2=seq(A2,B2,delta)
    for k in range(min(len(s1),len(s2))):
        if s1[k][:3]!=s2[k][:3]:
            worst=min(worst, 2*s1[k][3]-M); break
  print("delta",delta,"min over trials of 2*v(B) - M at first differing step:",worst)
