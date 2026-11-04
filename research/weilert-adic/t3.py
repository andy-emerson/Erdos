from alg import *
random.seed(3)
algs={'W':sym_W,'H':sym_H,'E':sym_E,'S':sym_S}
bad={k:0 for k in algs}; st={k:[] for k in algs}
for _ in range(3000):
    fb=[rand_prime(random.choice([6,10,16,24,30])) for _ in range(random.randint(1,4))]
    b=prod(fb); b=mul(b,ipow(random.randint(0,3)))
    a=(random.getrandbits(40)-(1<<39), random.getrandbits(40)-(1<<39))
    if random.random()<0.2: a=mul(a,(1,1))
    if random.random()<0.1: a=mul(a,fb[0])
    ref=direct(a,fb)
    for k,f in algs.items():
        if f(a,b,st[k])!=ref: bad[k]+=1
print(bad)
for k in st: print(k, sum(st[k])/len(st[k]), max(st[k]))
# random 31-bit odd pairs: agreement and steps
st={k:[] for k in algs}; dis=0
for _ in range(3000):
    while True:
        a=(random.randrange(-2**31,2**31),random.randrange(-2**31,2**31))
        b=(random.randrange(-2**31,2**31),random.randrange(-2**31,2**31))
        if (b[0]+b[1])%2: break
    r=[f(a,b,st[k]) for k,f in algs.items()]
    if len(set(r))>1: dis+=1
print("31-bit disagreements",dis)
for k in st: print(k, sum(st[k])/len(st[k]), max(st[k]))
