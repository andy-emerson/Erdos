# Gaussian integer helpers, quartic symbol (direct and by gcd-like algorithms)
import random
from math import isqrt

def mul(x,y): return (x[0]*y[0]-x[1]*y[1], x[0]*y[1]+x[1]*y[0])
def sub(x,y): return (x[0]-y[0], x[1]-y[1])
def add(x,y): return (x[0]+y[0], x[1]+y[1])
def N(x): return x[0]*x[0]+x[1]*x[1]
def conj(x): return (x[0],-x[1])
I=(0,1); ONE=(1,0)
def ipow(s):
    return [(1,0),(0,1),(-1,0),(0,-1)][s%4]
def rdiv(a,b):  # nearest rounding quotient
    n=N(b); p=mul(a,conj(b))
    def r(u): return (2*u+n)//(2*n)
    return (r(p[0]), r(p[1]))
def gmod(a,b): return sub(a, mul(rdiv(a,b),b))

def strip(z):
    """z != 0 -> (t, w) with z = (1+i)^t * w, w odd (w is z/(1+i)^t exactly)"""
    x,y=z; t=0
    while (x-y)%2==0:
        x,y=(x+y)//2,(y-x)//2; t+=1
    return t,(x,y)
def primary(w):
    """w odd -> (s, p) with w = i^s * p, p primary (p = 1 mod (1+i)^3: b even, a+b = 1 mod 4)"""
    for s in range(4):
        p = mul(w, ipow(-s))
        if p[1]%2==0 and (p[0]+p[1])%4==1: return s,p
    raise ValueError(w)
def is_primary(p): return p[1]%2==0 and (p[0]+p[1])%4==1

# supplements, exponents mod 4 (symbol = i^e)
def k1(b):  # [(1+i)/b] for primary b=c+di
    c,d=b; return ((c-d-d*d-1)//4)%4
def ki(b):  # [i/b]
    c,d=b; return (-(c-1)//2)%4
def rec(x,y):  # [x/y] = (-1)^.. [y/x], as exponent of i
    return 0 if ((x[0]-1)//2)*((y[0]-1)//2)%2==0 else 2

# direct symbol modulo a Gaussian prime P (odd): alpha^((N P -1)/4) mod P
def powmod(a,e,m):
    r=(1,0); a=gmod(a,m)
    while e:
        if e&1: r=gmod(mul(r,a),m)
        a=gmod(mul(a,a),m); e>>=1
    return r
def sym_prime(a,P):
    r=powmod(a,(N(P)-1)//4,P)
    if r==(0,0): return None
    for s in range(4):
        if gmod(sub(r,ipow(s)),P)==(0,0): return s
    raise ValueError((a,P,r))
def isprime(n):
    if n<2: return False
    for p in (2,3,5,7,11,13,17,19,23,29,31,37):
        if n%p==0: return n==p
    d=n-1; s=0
    while d%2==0: d//=2; s+=1
    for a in (2,3,5,7,11,13,17,19,23,29,31,37):
        x=pow(a,d,n)
        if x in (1,n-1): continue
        for _ in range(s-1):
            x=x*x%n
            if x==n-1: break
        else: return False
    return True
def rand_prime(bits):
    """random primary Gaussian prime: split (norm p = 1 mod 4) or inert (-q, q=3 mod 4)"""
    while True:
        if random.random()<0.15:
            q=random.getrandbits(max(2,bits//2))|3
            if isprime(q) and q%4==3: return (-q,0)
        else:
            x=random.getrandbits(bits//2); y=random.getrandbits(bits//2)
            if (x+y)%2==1 and isprime(x*x+y*y):
                return primary((x,y))[1]
def direct(a,factors):
    e=0
    for P in factors:
        s=sym_prime(a,P)
        if s is None: return None
        e+=s
    return e%4
def prod(fs):
    r=(1,0)
    for f in fs: r=mul(r,f)
    return r
