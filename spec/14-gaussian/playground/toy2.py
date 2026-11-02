# Toy 2: plain word operations only (no vector, no chip-specific instructions). Lane width K, word 2K.
#  (1) mod n, lazy: n < 2^(K-3) (headroom: lanes reach 6n and the packed compare needs lanes below 2^(K-1)); lanes kept in [0, 2n) (unsigned). Then lane sums stay below 2^K, so an ordinary
#      2K-bit add/subtract never carries or borrows between lanes:  add = 1 op,  sub = x + (4n - y) = 2 ops.
#      reduce(x): bring each lane from [0, 4n) back to [0, 2n) (or [0, n) for output) with packed compare masks.
#  (2) multiply mod n by Barrett reduction: no division at run time (m = floor(4^K / n) precomputed).
#  (3) wrapping (int-K lanes): add with a per-lane signed-overflow flag.
import sys, random
K=int(sys.argv[1]) if len(sys.argv)>1 else 4
W=2*K; MW=(1<<W)-1; ML=(1<<K)-1; H=(1<<(K-1))|(1<<(W-1)); LO=(1<<K)|1   # LO: a 1 at the bottom of each lane
def pack(a,b): return ((b&ML)<<K)|(a&ML)
lanes=lambda w:(w&ML,(w>>K)&ML)
s=lambda x:((x&ML)-(1<<K)) if (x&ML)>>(K-1) else x&ML

# (1) lazy mod-n add/sub: 1 and 2 word ops
add_lazy=lambda x,y: (x+y)&MW
def sub_lazy(x,y,N4): return (x+((N4-y)&MW))&MW          # N4 = pack(4n,4n) - ... : per lane 4n - y >= 0, no borrow
def spread_ge(x,C):          # per lane: all ones where lane(x) >= lane(C); lanes < 2^(K-1) so the subtraction's top bit decides
    t=((x|H)-C)&MW            # set each lane's top bit first, so no borrow crosses lanes (HD packed subtract)
    return ((((t&H)>>(K-1))&LO)*ML)&MW
def reduce(x,C):              # lane -= C_lane where lane >= C_lane  (C = pack(c,c)); 5-6 word ops
    return (x-(spread_ge(x,C)&C))&MW
# (2) Barrett: for t < n^2 * 2 (fits 2K bits), q = (t * m) >> 2K, r = t - q n, one conditional subtract
def barrett(t,n,m):
    q=(t*m)>>(2*K); r=t-q*n
    return r-n if r>=n else r
def mul_mod(x,y,n,m):
    a,b=lanes(x); c,d=lanes(y)             # lanes in [0, n)
    re=a*c+(n*n-b*d)                       # kept non-negative, < 2 n^2
    im=a*d+b*c                              # < 2 n^2
    return pack(barrett(re,n,m),barrett(im,n,m))
# (3) wrapping add with an overflow flag (sticky OR across a run): HD packed add, then per-lane signed overflow
def add_wrap(x,y): return (((x&~H&MW)+(y&~H&MW))^((x^y)&H))&MW
def ovf(x,y,r): return ((x^r)&(y^r)&H)!=0                  # 3 word ops (+1 sticky OR)

random.seed(3); bad={}
def chk(k,ok):
    if not ok: bad[k]=bad.get(k,0)+1
ns=[3,5,7] if K<=6 else [2**29-3, 65537, 1000003]
for n in ns:
    if 8*n>(1<<K): continue
    N4=pack(4*n,4*n); N2=pack(2*n,2*n); N1=pack(n,n); m=(1<<(2*K))//n
    vals=range(2*n)
    pairs=[(pack(a,b),pack(c,d)) for a in vals for b in vals for c in vals for d in vals] if K<=6 else \
          [(pack(random.randrange(2*n),random.randrange(2*n)),pack(random.randrange(2*n),random.randrange(2*n))) for _ in range(200000)]
    for x,y in pairs:
        (a,b),(c,d)=lanes(x),lanes(y)
        r=add_lazy(x,y); chk('add_lazy', lanes(r)==(a+c,b+d))                          # exact, unreduced
        r2=reduce(r,N2); chk('add then reduce to [0,2n)', all(l%n==v%n and l<2*n for l,v in zip(lanes(r2),(a+c,b+d))))
        r=sub_lazy(x,y,N4); chk('sub_lazy', [l%n for l in lanes(r)]==[(a-c)%n,(b-d)%n] and max(lanes(r))<6*n)
        out=reduce(reduce(x,N1),N1)                                                     # [0,2n) -> [0,n)
        chk('reduce to [0,n)', lanes(out)==(a%n,b%n))
        xr,yr=out,reduce(reduce(y,N1),N1)
        (a1,b1),(c1,d1)=lanes(xr),lanes(yr)
        chk('mul_mod (Barrett)', lanes(mul_mod(xr,yr,n,m))==((a1*c1-b1*d1)%n,(a1*d1+b1*c1)%n))
# wrapping add with overflow flag
pairs=[(x,y) for x in range(1<<W) for y in range(1<<W)] if K<=4 else [(random.getrandbits(W),random.getrandbits(W)) for _ in range(200000)]
for x,y in pairs:
    r=add_wrap(x,y); (a,b),(c,d)=(s(x),s(x>>K)),(s(y),s(y>>K))
    lo=-(1<<(K-1)); hi=(1<<(K-1))-1
    chk('overflow flag exact', ovf(x,y,r)==(not(lo<=a+c<=hi and lo<=b+d<=hi)))
print(f"K={K}: failures:", bad or "none")
