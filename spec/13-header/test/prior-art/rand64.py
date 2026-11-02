import random
R=64; N=1<<R; F=N-1; n=62; M=(1<<n)-1; NOTM=F&~M
I=[(1,0),(0,1),(-1,0),(0,-1)]
val=lambda m,d:(m*I[d][0],m*I[d][1])
sar=lambda x,k:((x-N if x>>63 else x)>>k)&F
sx8=lambda h:(h-256)&F if h&0x80 else h
def rm(bits):
    r=random.random()
    return random.choice([0,1,2,(1<<bits)-1,(1<<bits)-2]) if r<0.2 else random.getrandbits(random.randint(1,bits))
bad=0
for _ in range(300000):
    d1,d2=random.randrange(4),random.randrange(4)
    if random.random()<0.5: d2=random.choice([d1,(d1+2)%4])
    # header, lb variant
    m1,m2=rm(64),rm(64)
    if random.random()<0.3: m2=m1+random.randint(-2,2) & F
    h1,h2=d1<<6,d2<<6
    t=sx8(h1)^sx8(h2); s=sar(t,63); r=(m1+(m2^s)-s)&F; L=(0-int(m1<m2))&F; mb=s&L; r=((r^mb)-mb)&F
    hd=(h1^(t&L))&0xFF; err=(t>>6)&int(m1!=0)&int(m2!=0)&1
    v1,v2=val(m1,d1),val(m2,d2); v=(v1[0]+v2[0],v1[1]+v2[1])
    if v[0] and v[1]: bad+= err!=1
    elif abs(v[0])+abs(v[1])<N: bad+= (err or val(r,hd>>6)!=v)
    # packed top and bottom add
    m1,m2=rm(62),rm(62)
    if random.random()<0.3: m2=max(0,min(M,m1+random.randint(-2,2)))
    v1,v2=val(m1,d1),val(m2,d2); v=(v1[0]+v2[0],v1[1]+v2[1])
    for top in (1,0):
        w1,w2=((d1<<n)|m1,(d2<<n)|m2) if top else ((m1<<2)|d1,(m2<<2)|d2)
        t=w1^w2
        if top: s=sar(t,63); L=sar(((w1&M)-(w2&M))&F,63); mask=M
        else: s=sar((t<<62)&F,63); L=(0-int(w1<(w2&~3&F)))&F; mask=F&~3
        tL=t&L; W=w1^tL; V=W^t; mn=V&mask; Rw=(W+(mn^s)-s)&F
        err=((t>>n) if top else t)&1&int(mn!=0)
        m,d=(Rw&M,Rw>>n) if top else (Rw>>2,Rw&3)
        if v[0] and v[1]: bad+= err!=1
        elif abs(v[0])+abs(v[1])<=M: bad+= (err or val(m,d)!=v)
    # packed mul/div
    a=rm(31); b=rm(31)
    w1,w2=(d1<<n)|a,(d2<<n)|b
    x,y=val(a,d1),val(b,d2); tr=(x[0]*y[0]-x[1]*y[1],x[0]*y[1]+x[1]*y[0])
    for res in (((a*b)|(((w1^a)+(w2^b))&F)), ((a*b)|((w1+w2-1)&NOTM))):
        bad+= val(res&M,res>>n)!=tr
    W1,W2=(a<<2)|d1,(b<<2)|d2
    res=(((W1>>2)*(W2&~3&F))&F)|((W1+W2)&3); bad+= val(res>>2,res&3)!=tr
    if b:
        p=a*b; P1,P2=(d1<<n)|p,(d2<<n)|b
        res=(p//b)|((P1-P2)&NOTM); bad+= val(res&M,(res>>n))!=val(a,(d1-d2)%4) and a!=0
        Q1,Q2=(p<<2)|d1,(b<<2)|d2
        res=((Q1&~3&F)//(Q2>>2))|((Q1-Q2)&3); bad+= val(res>>2,res&3)!=val(a,(d1-d2)%4) and a!=0
print("bad",bad)
