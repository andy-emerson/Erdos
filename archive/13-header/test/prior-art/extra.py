import random
# 1. Hurchalla/Dumas inverse mod 2^64 for odd a (8 muls)
F=(1<<64)-1
def inv64(a):
    x=((3*a)^2)&F; y=(1-a*x)&F
    for i in range(4):
        x=(x*(1+y))&F
        if i<3: y=(y*y)&F
    return x
bad=sum(((a*inv64(a))&F)!=1 for a in [random.getrandbits(64)|1 for _ in range(200000)]+[1,3,F])
print("inv64 bad",bad)
# x0 = (3a) xor 2 good to 5 bits: check all odd a mod 32
print("x0 5 bits:", all((a*((3*a)^2))%32==1 for a in range(1,32,2)))
# 2. exact division by inverse, 8-bit, exhaustive: q = (m1 >> k) * inv(m2 >> k) mod 2^8, k = ctz(m2)
R=8;N=1<<R;bad=0
inv=lambda a: next(x for x in range(N) if (a*x)%N==1)
for m2 in range(1,N):
    k=(m2&-m2).bit_length()-1; i=inv(m2>>k)
    for q in range(N//m2+1):
        m1=q*m2
        if m1<N: bad+= ((m1>>k)*i)%N!=q
print("exact-div by inverse bad",bad)
# 3. header add with selects (csel/cmov/Zbb): W=max, V=min, r = W + (V^s) - s, hd = lt ? h2 : h1 ; 8-bit full width
N=256;F=255;I=[(1,0),(0,1),(-1,0),(0,-1)];val=lambda m,d:(m*I[d][0],m*I[d][1]);bad=0
for d1 in range(4):
 for d2 in range(4):
  h1,h2=d1<<6,d2<<6
  for m1 in range(N):
   for m2 in range(N):
    t=h1^h2; s=F if t&0x80 else 0; lt=m1<m2
    W,V=(m2,m1) if lt else (m1,m2); r=(W+(V^s)-s)&F; hd=h2 if lt else h1
    v1,v2=val(m1,d1),val(m2,d2); v=(v1[0]+v2[0],v1[1]+v2[1])
    if v[0] and v[1]: continue
    if abs(v[0])+abs(v[1])<N: bad+= val(r,hd>>6)!=v
print("header select variant bad",bad)
