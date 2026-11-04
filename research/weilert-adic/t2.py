from alg import *
import sys
random.seed(2)
algs={'W':sym_W,'H':sym_H,'E':sym_E}
bad={k:0 for k in algs}; st={k:[] for k in algs}; cnt=0
for _ in range(4000):
    fb=[rand_prime(random.choice([6,10,16,24,30])) for _ in range(random.randint(1,4))]
    b=prod(fb)
    b=mul(b,ipow(random.randint(0,3)))   # random associate
    a=(random.getrandbits(40)-(1<<39), random.getrandbits(40)-(1<<39))
    if random.random()<0.2: a=mul(a,(1,1)) 
    if random.random()<0.1: a=mul(a,fb[0])
    ref=direct(a,fb); cnt+=1
    for k,f in algs.items():
        if f(a,b,st[k])!=ref: bad[k]+=1
print(cnt,bad)
for k in st: print(k, sum(st[k])/len(st[k]), max(st[k]))
