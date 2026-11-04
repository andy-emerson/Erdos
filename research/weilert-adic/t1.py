# check supplements and reciprocity on composite primary moduli
from g import *
random.seed(1)
bad=0; n=0
for _ in range(3000):
    fb=[rand_prime(random.choice([6,10,16,24])) for _ in range(random.randint(1,4))]
    b=prod(fb); assert is_primary(b)
    if direct(I,fb)!=ki(b): bad+=1
    if direct((1,1),fb)!=k1(b): bad+=1
    fa=[rand_prime(random.choice([6,10,16,24])) for _ in range(random.randint(1,4))]
    a=prod(fa)
    da=direct(a,fb); db=direct(b,fa)
    if da is None or db is None: continue
    n+=1
    if (da-db-rec(a,b))%4: bad+=1
print("bad",bad,"reciprocity cases",n)
