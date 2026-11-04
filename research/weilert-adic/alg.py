from g import *

def centred(x,m):  # x mod m in [-m/2, m/2)
    x%=m
    return x-m if x>=m//2 else x
def inv2(n,j):  # n odd -> n^-1 mod 2^j
    return pow(n,-1,1<<j)
def hensel_q(a,b,j):
    """q = a/b mod 2^j, centred coordinates"""
    M=1<<j; n=N(b)
    p=mul(a,conj(b)); iv=inv2(n,j)
    return (centred(p[0]*iv,M), centred(p[1]*iv,M))
def blen(x): return x.bit_length()

def reduce_term(r, e, mod):
    """r = i^s pi^t c with c primary; return (c, e + t k1(mod) + s ki(mod))"""
    t,w=strip(r); s,c=primary(w)
    return c,(e+t*k1(mod)+s*ki(mod))%4,t

def setup(a,b):
    """target [a/b], b odd. returns (x,y,e) with x,y primary, [a/b] = i^e [x/y]; or None if a=0"""
    s,b=primary(b)  # symbol depends on the ideal only
    if a==(0,0): return None
    x,e,_=reduce_term(a,0,b)
    return x,b,e

def finish(y,e):
    return e%4 if y==(1,0) else None

# W: Weilert / Damgard-Frandsen eps step: replace larger by primary part of difference
def sym_W(a,b,stats=None):
    st=setup(a,b)
    if st is None: return None if primary(b)[1]!=(1,0) else 0
    x,y,e=st; steps=0  # want [x/y]
    while True:
        if N(x)<N(y):
            e=(e+rec(x,y))%4; x,y=y,x
        if y==(1,0): break
        d=sub(x,y)
        if d==(0,0): break     # x==y != 1 -> gcd nontrivial
        x,e,_=reduce_term(d,e,y); steps+=1
    if stats is not None: stats.append(steps)
    return finish(y,e)

# H: study 33 Hensel quotient, replace larger
def sym_H(a,b,stats=None,c=6):
    st=setup(a,b)
    if st is None: return None if primary(b)[1]!=(1,0) else 0
    x,y,e=st; steps=0
    while True:
        if N(x)<N(y):
            e=(e+rec(x,y))%4; x,y=y,x
        if y==(1,0): break
        L=blen(N(x))-blen(N(y))
        j=max(2,(L+c)>>1)
        q=hensel_q(x,y,j)
        d=sub(x,mul(q,y))
        if d==(0,0): break
        x,e,_=reduce_term(d,e,y); steps+=1
    if stats is not None: stats.append(steps)
    return finish(y,e)

# E: Eisenstein / Euclid with nearest rounding, remainder made primary
def sym_E(a,b,stats=None):
    st=setup(a,b)
    if st is None: return None if primary(b)[1]!=(1,0) else 0
    x,y,e=st; steps=0
    while True:
        if N(x)<N(y):
            e=(e+rec(x,y))%4; x,y=y,x
        if y==(1,0): break
        d=gmod(x,y)
        if d==(0,0): break
        x,e,_=reduce_term(d,e,y); steps+=1
    if stats is not None: stats.append(steps)
    return finish(y,e)

# S: Gaussian Stehle-Zimmermann: valuation driven, always swap, no size compare
def pimod_q(A,B,m):
    """q = A/B mod (1+i)^m, minimal representative (Babai rounding in lattice (1+i)^m Z[i])"""
    h=(m+1)//2
    q=hensel_q(A,B,h)             # correct mod 2^h hence mod pi^m
    # reduce q modulo lattice pi^m to nearest small rep
    P=(1,0)
    for _ in range(m): P=mul(P,(1,1))
    return sub(q, mul(rdiv(q,P),P))
def sym_S(a,b,stats=None,cap=100000,delta=1):
    st=setup(a,b)
    if st is None: return None if primary(b)[1]!=(1,0) else 0
    A,B,e=st; g=1; steps=0   # target [A/B]; A replaced by A - qB
    while True:
        if B==(1,0): break
        q=pimod_q(A,B,g+delta)        # A - qB divisible by pi^(g+1)
        r=sub(A,mul(q,B))
        if r==(0,0): break
        C,e,t=reduce_term(r,e,B)
        e=(e+rec(C,B))%4         # [C/B] -> [B/C]
        A,B=B,C; g=t-g
        steps+=1
        if steps>cap: raise RuntimeError("no termination")
    if stats is not None: stats.append(steps)
    return finish(B,e)
