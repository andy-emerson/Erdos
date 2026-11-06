"""fdbasics.py: checks for Sections 1 and 2 of fundamental-domain.md.

1. 3x+1 on N is the fundamental-domain map (F = positives, modulus 2, digit 1), and it is the
   quotient of the commuting map 3z + sgn(z); the digit sgn(z) is not a function of z mod 2^j.
2. On Z[i], for a fundamental domain F, (alpha, d) and (u alpha, u d) give the same map on F,
   and (F, d) and (uF, d^u) induce the same map on R/U (spot checks).
3. Primary representatives: the commuting extension of a primary-F table at modulus (1+i)^k,
   k >= 3, is a residue-class table (checked by enumeration on a ball).
Output: logs/fdbasics.txt
"""
import itertools, random

# ---------- integers ----------
def odd_part(w):
    v = 0
    while w % 2 == 0:
        w //= 2; v += 1
    return w, v

def S_pos(n, m, dig, k):
    """fundamental-domain map on odd positive n: |(m n + d(n mod 2^k))| / 2^v"""
    w = m * n + dig[n % (2 ** k)]
    if w == 0:
        return 0
    return abs(odd_part(w)[0])

def collatz_acc(n):
    return odd_part(3 * n + 1)[0]

def lift(z):  # 3z + sgn(z), accelerated
    return odd_part(3 * z + (1 if z > 0 else -1))[0]

out = []
ok = all(S_pos(n, 3, {1: 1}, 1) == collatz_acc(n) for n in range(1, 200001, 2))
out.append(f"3x+1 = fundamental-domain map (F = positives, m = 3, d = 1) on odd n <= 2e5: {ok}")
ok = all(lift(-n) == -lift(n) and abs(lift(n)) == collatz_acc(n) for n in range(1, 200001, 2))
out.append(f"3z+sgn(z) commutes with -1 and |lift(n)| = C(n) on odd |n| <= 2e5: {ok}")
# digit sgn(z) is not a function of z mod 2^j: z = 1 and z = 1 - 2^j are congruent, digits differ
out.append("digit of the lift at 1 and 1-2^j (j=1..20): " +
           str(all((1 if 1 > 0 else -1) != (1 if 1 - 2 ** j > 0 else -1) for j in range(1, 21))))
# C(5)=16, C(-5)=-14 under 3x+1 extended to Z by the formula
out.append(f"3x+1 by formula on Z: 3*5+1 = {3*5+1}, 3*(-5)+1 = {3*(-5)+1} (not associates)")
# 2-adic primary domain: n = 1 mod 4 representatives; 3x+1 there is 3x +- 1
def S_prim(n, m, d):
    w = m * n + d
    o = odd_part(w)[0]
    return o if o % 4 == 1 else -o
def pm(n):  # 3x+-1 accelerated, on the representative = 1 mod 4
    w = 3 * n + (1 if n % 4 == 1 else -1)
    return odd_part(w)[0]
ok = all(S_prim(n, 3, 1) == (pm(n) if pm(n) % 4 == 1 else -pm(n)) for n in range(-100001, 100002, 4) if n % 4 == 1)
v4 = all(odd_part(3 * n + 1)[1] >= 2 for n in range(-100003, 100002) if n % 4 == 1)
out.append(f"primary integer domain (n = 1 mod 4): 3n+1 always divisible by 4: {v4}; map = 3x+-1 on classes: {ok}")

# ---------- Gaussian integers ----------
U = [(1, 0), (0, 1), (-1, 0), (0, -1)]
def mul(x, y):
    return (x[0] * y[0] - x[1] * y[1], x[0] * y[1] + x[1] * y[0])
def add(x, y):
    return (x[0] + y[0], x[1] + y[1])
def N(x):
    return x[0] ** 2 + x[1] ** 2
def gpi_odd(w):
    a, b = w; v = 0
    while (a + b) % 2 == 0:
        a, b = (a + b) // 2, (b - a) // 2; v += 1
    return (a, b), v
def inF(F, z):
    a, b = z
    if F == 'Q': return a > 0 and b >= 0
    if F == 'Qp': return a >= 0 and b > 0
    if F == 'C': return a > 0 and -a < b <= a
    if F == 'P': return (a % 4, b % 4) in ((1, 0), (3, 2))
def rep(F, z):
    for u in U:
        w = mul(u, z)
        if inF(F, w):
            return w
    raise ValueError(z)
def dvd_pik(x, k):
    a, b = x
    if k == 1: return (a + b) % 2 == 0
    if k == 2: return a % 2 == 0 and b % 2 == 0
    if k == 3: return (a % 4, b % 4) in ((0, 0), (2, 2))
    if k == 4: return a % 4 == 0 and b % 4 == 0
def cls4(z):
    return (z[0] % 4, z[1] % 4)
def SF(F, alpha, table, z):
    w = add(mul(alpha, z), table[cls4(z)])
    if w == (0, 0): return (0, 0)
    return rep(F, gpi_odd(w)[0])

random.seed(1)
oddcls = [(a, b) for a in range(4) for b in range(4) if (a + b) % 2]
def random_table(alpha, k, maxc=3):
    """random table at modulus pi^k (k<=4), as a dict on classes mod 4"""
    reps = {}
    t = {}
    for c in oddcls:
        key = None
        for c2 in reps:
            if dvd_pik((c[0] - c2[0], c[1] - c2[1]), k):
                key = c2; break
        if key is None:
            target = mul((-alpha[0], -alpha[1]), c)
            cands = [(x, y) for x in range(-5, 6) for y in range(-5, 6)
                     if dvd_pik((x - target[0], y - target[1]), k)]
            cands.sort(key=N)
            reps[c] = random.choice(cands[:6]); key = c
        t[c] = reps[key]
    return t

pts = [(a, b) for a in range(-40, 41) for b in range(-40, 41) if (a + b) % 2]
bad = 0; tests = 0
for F in ('Q', 'C', 'P'):
    for alpha, k in (((2, 1), 2), ((2, -1), 2), ((3, 0), 3), ((3, 2), 3), ((5, 2), 4), ((4, 1), 4)):
        for _ in range(20):
            t = random_table(alpha, k)
            for u in U[1:]:
                ua = mul(u, alpha); ut = {c: mul(u, t[c]) for c in t}
                for z in pts:
                    if not inF(F, z): continue
                    tests += 1
                    if SF(F, alpha, t, z) != SF(F, ua, ut, z): bad += 1
out.append(f"(alpha,d) vs (u alpha, u d) on F in Q,C,P: {tests} comparisons, {bad} differences")

# (F,d) and (uF, d^u) induce the same class map; uF for F=Q with u=i is the second quadrant
def inuF(z, u):  # z in uF iff u^-1 z in F (F = Q)
    uinv = {(1, 0): (1, 0), (0, 1): (0, -1), (-1, 0): (-1, 0), (0, -1): (0, 1)}[u]
    return inF('Q', mul(uinv, z))
def rep_uF(z, u):
    for v in U:
        w = mul(v, z)
        if inuF(w, u): return w
bad = 0; tests = 0
def cls_of(z):  # canonical class rep (first quadrant)
    return z if z == (0, 0) else rep('Q', z)
for alpha, k in (((2, 1), 2), ((3, 2), 3), ((5, 2), 4)):
    for _ in range(20):
        t = random_table(alpha, k)
        for u in U[1:]:
            uinv = {(0, 1): (0, -1), (-1, 0): (-1, 0), (0, -1): (0, 1)}[u]
            tu = {c: mul(u, t[cls4(mul(uinv, c))]) for c in oddcls}  # d^u(z) = u d(u^-1 z)
            for z in pts:
                if not inF('Q', z): continue
                tests += 1
                a1 = cls_of(SF('Q', alpha, t, z))
                uz = mul(u, z)
                w = add(mul(alpha, uz), tu[cls4(uz)])
                a2 = (0, 0) if w == (0, 0) else cls_of(rep_uF(gpi_odd(w)[0], u))
                if a1 != a2: bad += 1
out.append(f"(Q,d) vs (uQ,d^u) on R/U: {tests} comparisons, {bad} differences")

# primary F: commuting extension D(u f) = u d(f) is a function of z mod pi^max(k,3)
def ext_ok(F, alpha, k, t, modk):
    seen = {}
    for z in pts:
        f = rep(F, z)
        u = [v for v in U if mul(v, f) == z][0]
        D = mul(u, t[cls4(f)])
        # class of z mod pi^modk, encoded by residues mod 4 reduced
        key = None
        for c2 in seen:
            if dvd_pik((z[0] - c2[0], z[1] - c2[1]), modk):
                key = c2; break
        if key is None:
            seen[z] = D
        elif seen[key] != D:
            return False
    return True
res = {}
for F in ('P', 'Q', 'C'):
    cnt = 0; okc = 0
    for alpha, k in (((3, 0), 3), ((3, 2), 3), ((5, 2), 4), ((4, 1), 4)):
        for _ in range(10):
            t = random_table(alpha, k)
            cnt += 1
            okc += ext_ok(F, alpha, k, t, max(k, 3))
    res[F] = (okc, cnt)
out.append("commuting extension is a residue-class table (random tables, k = 3, 4): " +
           ", ".join(f"{F}: {a}/{b}" for F, (a, b) in res.items()))

open('/home/claude/erdos/collatz/research/logs/fdbasics.txt', 'w').write("\n".join(out) + "\n")
print("\n".join(out))
