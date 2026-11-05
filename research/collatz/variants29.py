# All modulus-4 expressions with multiplier an associate or conjugate of 5+2i and minimal digits
# (every tie-break), tested for triviality on odd z with N(z) <= B; lists the trivial cycles of S.
import itertools, sys
B = int(sys.argv[1]) if len(sys.argv) > 1 else 20000
U = [(1,0),(0,1),(-1,0),(0,-1)]
def mul(x,y): return (x[0]*y[0]-x[1]*y[1], x[0]*y[1]+x[1]*y[0])
def N(z): return z[0]*z[0]+z[1]*z[1]
def odd(z): return (z[0]+z[1]) % 2 == 1
def divpi(z): return ((z[0]+z[1])//2, (z[1]-z[0])//2)
def key(z): return (z[0] % 4, z[1] % 4)
# least-norm elements of each odd class mod 4
cls = {}
for a in range(-3,4):
    for b in range(-3,4):
        z=(a,b)
        if odd(z): cls.setdefault(key(z),[]).append(z)
least = {}
for k,v in cls.items():
    m=min(N(z) for z in v); least[k]=[z for z in v if N(z)==m]
keys=sorted(least)
choices=[least[k] for k in keys]
def S(alpha,D,z):
    w=mul(alpha,z); d=D[key((-w[0],-w[1]))]; w=(w[0]+d[0],w[1]+d[1])
    while not odd(w):
        if w==(0,0): return w
        w=divpi(w)
    return w
def symmetric(D):
    return all(D[key(mul(u,z))]==mul(u,D[key(z)]) for z in [c[0] for c in choices] for u in U)
def cyc(alpha,D,z0):
    seen=[];z=z0
    while z not in seen:
        seen.append(z); z=S(alpha,D,z)
    return seen[seen.index(z):]
def triv(alpha,D):
    for a in range(-400,401):
        for b in range(-400,401):
            z=(a,b)
            if not odd(z) or N(z)>B: continue
            for _ in range(3000):
                if z in U: break
                z=S(alpha,D,z)
                if N(z)>10**60: return False
            else: return False
    return True
def name(z):
    a,b=z
    if b==0: return str(a)
    s=('' if a==0 else str(a)) + ('+' if b>0 and a!=0 else ('-' if b<0 else '')) + ('' if abs(b)==1 else str(abs(b))) + 'i'
    return s if not (a==0 and b<0) else '-'+(('' if abs(b)==1 else str(abs(b)))+'i')
alphas=[mul(u,(5,2)) for u in U]+[mul(u,(5,-2)) for u in U]
for alpha in alphas:
    for pick in itertools.product(*choices):
        D=dict(zip(keys,pick))
        if triv(alpha,D):
            cycles=[]
            for u in U:
                c=cyc(alpha,D,u)
                if set(c) not in [set(x) for x in cycles]: cycles.append(c)
            tied=[name(D[k]) for k in keys if len(least[k])>1]
            print(name(alpha), '| tied digits', ' '.join(tied), '| symmetric' if symmetric(D) else '| not symmetric',
                  '|', '; '.join(' -> '.join(name(z) for z in c+[c[0]]) for c in cycles))
