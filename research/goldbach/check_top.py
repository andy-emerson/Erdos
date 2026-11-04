"""Independent checks on gg.c output files: (1) target counts by a direct formula,
(2) every reported hardest representation is valid (sympy primality, exact angle tests),
(3) minimality of the reported value for those z (exhaustive search over smaller p1)."""
import sys, re, math
from sympy import isprime
def gp(x,y):
    if x==0: return abs(y)%4==3 and isprime(abs(y))
    if y==0: return abs(x)%4==3 and isprime(abs(x))
    return isprime(x*x+y*y)
def count(lo,hi,bmin):
    c=0
    for a in range(math.isqrt(hi)+1):
        # b in [bmin, a], b = a mod 2, lo <= a^2+b^2 <= hi
        bl=0 if lo<=a*a else math.isqrt(lo-a*a-1)+1
        bl=max(bl,bmin); bh=min(a,math.isqrt(hi-a*a)) if hi>=a*a else -1
        if bl>bh: continue
        if (bl+a)%2: bl+=1
        if bl<=bh: c+=(bh-bl)//2+1
    return c
def okE(a,b,x,y,strict=True):
    nz=a*a+b*b; d=x*a+y*b; d2=nz-d; c=abs(x*b-y*a)
    return d>c and d2>c if strict else (d>=c and d2>=c and d>0 and d2>0)
def okF(a,b,x,y):
    nz=a*a+b*b; d=x*a+y*b; d2=nz-d; c=x*b-y*a
    return d>0 and d2>0 and 3*c*c<d*d and 3*c*c<d2*d2
for fn in sys.argv[1:]:
    t=open(fn).read()
    lo,hi=map(int,re.search(r"NMIN (\d+) NMAX (\d+)",t).groups())
    for var,cnt in re.findall(r"variant (\w): targets (\d+)",t):
        exp={'E':count(max(lo,3),hi,0),'F':count(max(lo,11),hi,0),'Q':count(lo,hi,2)}[var]
        print(fn,var,"targets",cnt,"expected",exp,"OK" if int(cnt)==exp else "MISMATCH")
    cur=None
    for line in t.splitlines():
        m=re.match(r"variant (\w)",line)
        if m: cur=m.group(1)
        m=re.search(r"z=(-?\d+)\+(-?\d+)i N=\d+  value (\d+)  p1=(-?\d+)\+(-?\d+)i",line)
        if not m: continue
        a,b,v,x,y=map(int,m.groups())
        assert gp(x,y) and gp(a-x,b-y), line
        if cur=='E':
            assert okE(a,b,x,y) and x*x+y*y==v
            R=math.isqrt(v)
            assert not any(gp(u,w) and gp(a-u,b-w) and okE(a,b,u,w) for u in range(-R,R+1) for w in range(-R,R+1) if u*u+w*w<v), line
        elif cur=='F':
            assert okF(a,b,x,y) and x*x+y*y==v
            R=math.isqrt(v)
            assert not any(gp(u,w) and gp(a-u,b-w) and okF(a,b,u,w) for u in range(-R,R+1) for w in range(-R,R+1) if u*u+w*w<v), line
        else:
            assert x>0 and y>0 and a-x>0 and b-y>0 and x+y==v
            assert not any(gp(u,k-u) and gp(a-u,b-k+u) for k in range(2,v) for u in range(1,k) if k-u<b and u<a), line
    print(fn,"top representations valid and minimal")
