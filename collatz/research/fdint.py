"""fdint.py: fundamental-domain maps on N = Z/{+-1} (F = positives), Section 7 of fundamental-domain.md.
S(n) = |m n + d(n mod 2^k)| / 2^v on odd n > 0, multipliers m > 0 in the window 2^k < m < 2^(k+1)
((-m, d) is the same map as (m, -d) on F), digits d = -m c mod 2^k with |d| <= DMAX.
Stage 1: every odd n <= B1 reaches 1 and 1 is periodic (cycle detection, cap).
Stage 2: drop below start for B1 < n <= B2.
A table commutes with -1 when d(-c) = -d(c) (then the map is the quotient of the residue-class map
(m, 2^k, d), Section 2); at modulus 2 no table does.
Also: the 2-adic primary domain F = {n = 1 mod 4} at modulus 2 (Section 7.3).
Output: logs/fdint.txt
"""
import itertools, sys

def oddpart(w):
    while w % 2 == 0: w //= 2
    return w

def run(m, k, tab, B1, B2, cap=20000, F='pos'):
    M = 2 ** k
    def S(n):
        w = m * n + tab[n % M]
        if w == 0: return 0
        o = oddpart(w)
        if F == 'pos': return abs(o)
        return o if o % 4 == 1 else -o
    one = 1
    cyc = [1]; x = S(1)
    while x != 1 and len(cyc) <= 200:
        if x == 0: return ("orbit of 1 reaches 0", None)
        cyc.append(x); x = S(x)
    if x != 1: return ("1 not periodic", None)
    good = bytearray(B1 + 2) if F == 'pos' else None
    goodset = set(cyc)
    for c in cyc:
        if F == 'pos' and c <= B1: good[c] = 1
    starts = range(3, B1 + 1, 2) if F == 'pos' else [n for n in range(-B1, B1 + 1) if n % 4 == 1]
    for n in starts:
        if F == 'pos' and good[n]: continue
        if F != 'pos' and n in goodset: continue
        path = []; x = n; seen = set()
        while True:
            if F == 'pos':
                if 0 < x <= B1 and good[x]: break
            elif x in goodset: break
            if x == 0: return (f"start {n} reaches 0", cyc)
            if x in seen:
                c = [x]; y = S(x)
                while y != x: c.append(y); y = S(y)
                return (f"cycle of {len(c)} odd steps, min {min(c, key=abs)}", cyc)
            if len(path) > cap or abs(x) > 2 ** 200: return (f"start {n} unresolved", cyc)
            seen.add(x); path.append(x); x = S(x)
        for p in path:
            if F == 'pos':
                if p <= B1: good[p] = 1
            else: goodset.add(p)
    if F == 'pos':
        for n in range(B1 + 1 if B1 % 2 == 0 else B1 + 2, B2 + 1, 2):
            x = n; s = 0
            while x >= n:
                x = S(x); s += 1
                if s > cap or x == 0: return (f"drop fails at {n}", cyc)
    return ("PASS", cyc)

def main():
    B1, B2 = 10 ** 5, 10 ** 6
    DMAX = 15
    lines = [f"F = positives, digits |d| <= {DMAX} (31 at modulus 2), stage 1 odd n <= {B1}, stage 2 drop to {B2}"]
    for k, ms in ((1, [3]), (2, [5, 7]), (3, [9, 11, 13, 15])):
        M = 2 ** k
        classes = list(range(1, M, 2))
        for m in ms:
            dm = 31 if k == 1 else DMAX
            lists = [[d for d in range(-dm, dm + 1) if (m * c + d) % M == 0] for c in classes]
            npass = 0; ncomm = 0; tested = 0; passed = []
            for choice in itertools.product(*lists):
                tab = dict(zip(classes, choice)); tested += 1
                res, cyc = run(m, k, tab, B1, B2)
                comm = all(tab[(-c) % M] == -tab[c] for c in classes) if k > 1 else False
                if res == "PASS":
                    npass += 1; ncomm += comm
                    passed.append((sum(abs(d) for d in choice), choice, comm, cyc))
                if k == 1:
                    lines.append(f"  modulus 2, {m}x{choice[0]:+d}: {res}" + (f"; cycle of 1: {cyc}" if cyc else ""))
            lines.append(f"modulus {M}, m = {m}: {tested} tables, {npass} Collatz maps on N to {B2}, of which {ncomm} commute with -1 (quotients of residue-class maps)")
            passed.sort()
            for s, ch, comm, cyc in passed[:12]:
                lines.append(f"    digits by class {classes}: {list(ch)}{' (commutes)' if comm else ''}; cycle of 1: {cyc}")
            if len(passed) > 12: lines.append(f"    ... {len(passed) - 12} more")
    # 2-adic primary domain at modulus 2
    lines.append("F = {n = 1 mod 4} (2-adic primary), modulus 2, all n = 1 mod 4 with |n| <= 1e5:")
    for d in (1, -1, 3, -3, 5, -5):
        res, cyc = run(3, 1, {1: d}, 10 ** 5, 0, F='prim')
        lines.append(f"  3x{d:+d}: {res}" + (f"; cycle of [1]: {cyc}" if cyc else ""))
    open('/home/claude/erdos/collatz/research/logs/fdint.txt', 'w').write("\n".join(lines) + "\n")
    print("\n".join(lines))

if __name__ == "__main__":
    main()
