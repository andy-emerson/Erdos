# Toy: a Gaussian integer a + bi packed in one word, two signed lanes: a in the low lane, b in the high lane.
# Lane width K bits, word 2K bits (K = 32 is the real thing; small K for exhaustive checks).
# Each operation is written with plain word operations, as a machine would do it without vector instructions,
# and checked against true Gaussian-integer arithmetic (wrapping each part mod 2^K like int32, or mod n).
import sys, random
K=int(sys.argv[1]) if len(sys.argv)>1 else 4
W=2*K; MW=(1<<W)-1; ML=(1<<K)-1; H=(1<<(K-1))|(1<<(W-1))   # H: the top bit of each lane
def s(x,k=K):  x&=(1<<k)-1; return x-(1<<k) if x>>(k-1) else x
def pack(a,b): return ((b&ML)<<K)|(a&ML)
def unpack(w): return s(w&ML), s(w>>K)
wrap=lambda v: s(v)

# --- add: lane-wise, carries must not cross from the low lane into the high one (Hacker's Delight 2-18 packed add)
def add(x,y): return (((x&~H&MW)+(y&~H&MW)) ^ ((x^y)&H)) & MW            # 5 operations
def add_fix(x,y):                                                           # alternative: full add, remove the low lane's carry
    t=(x+y)&MW; c=(((x&ML)+(y&ML))>>K)&1; return (t-(c<<K))&MW              # 7
# --- subtract: borrows must not cross
def sub(x,y): return (((x|H)-(y&~H&MW)) ^ ((x^~y)&H)) & MW                 # 5 (HD 2-18 packed subtract)
# --- multiply by i: (a+bi)i = -b + ai: swap the lanes (rotate by K), negate the new low lane
def rot(x): return ((x>>K)|(x<<K))&MW
def neg_low(x): return (x & (ML<<K)) | ((-(x&ML))&ML)
def mul_i(x): return neg_low(rot(x))
# --- conjugate: negate the high lane
def conj(x): return ((((-(x>>K))&ML))<<K) | (x&ML)
# --- multiply, wrapping each part mod 2^K: four products of sign-extended lanes
def mul(x,y):
    a,b=unpack(x); c,d=unpack(y); return pack(a*c-b*d, a*d+b*c)            # ~ 2 extracts each, 4 mul, 2 add, pack
# --- norm a^2 + b^2 (one ordinary integer, 2K bits)
def norm(x): a,b=unpack(x); return a*a+b*b

def check_wrap(samples):
    bad={}
    for x,y in samples:
        a,b=unpack(x); c,d=unpack(y)
        tests={'add':(add(x,y),(wrap(a+c),wrap(b+d))),'add_fix':(add_fix(x,y),(wrap(a+c),wrap(b+d))),
               'sub':(sub(x,y),(wrap(a-c),wrap(b-d))),'mul_i':(mul_i(x),(wrap(-b),a)),
               'conj':(conj(x),(a,wrap(-b))),'mul':(mul(x,y),(wrap(a*c-b*d),wrap(a*d+b*c)))}
        for k,(got,want) in tests.items():
            if unpack(got)!=want: bad[k]=bad.get(k,0)+1
    return bad
if K<=6: samples=[(x,y) for x in range(1<<W) for y in range(1<<W)]
else: samples=[(random.getrandbits(W),random.getrandbits(W)) for _ in range(200000)]
print(f"K={K}: {len(samples)} pairs; failures by operation (wrapping lanes):", check_wrap(samples) or "none")

# --- mod n: lanes hold a, b in [0, n), n < 2^(K-1) so a lane sum never overflows its lane
def spread(t):                       # per lane: all ones where the lane's top bit is set (multiply spreads a 1 bit)
    return (((t&H)>>(K-1)) * ML) & MW
def add_mod(x,y,N2):                  # N2 = pack(n, n)
    t=sub(add(x,y),N2)               # lane-wise (a+c-n, b+d-n), may go negative
    return add(t, spread(t)&N2)      # add n back where negative                         ~ 5+5+3+5
def sub_mod(x,y,N2):
    t=sub(x,y); return add(t, spread(t)&N2)
def mul_mod(x,y,n):
    a,b=x&ML,x>>K; c,d=y&ML,y>>K
    return pack((a*c-b*d)%n,(a*d+b*c)%n)
def check_mod(n,samples):
    N2=pack(n,n); bad={}
    for x,y in samples:
        a,b=x&ML,x>>K; c,d=y&ML,y>>K
        if not(a<n and b<n and c<n and d<n): continue
        for k,got,want in (('add_mod',add_mod(x,y,N2),((a+c)%n,(b+d)%n)),
                           ('sub_mod',sub_mod(x,y,N2),((a-c)%n,(b-d)%n)),
                           ('mul_mod',mul_mod(x,y,n),((a*c-b*d)%n,(a*d+b*c)%n))):
            if (got&ML,got>>K)!=want: bad[k]=bad.get(k,0)+1
    return bad
for n in ([3,5,7] if K<=4 else [2**31-1, 2**30+3, 65537]):
    print(f"K={K}, n={n}: failures (mod n):", check_mod(n,samples) or "none")
