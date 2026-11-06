"""ringfive.py: how the rational multiplier 5 fails (rings-test.md): failure lines of the smallest tables and a cycle
count on R/U (Python census up to |N| <= B, cycles up to units, starts unresolved after cap steps).
usage: python3 ringfive.py B"""
import sys, collections
import qring as Q, ringrun as RR, ringtest as RT
B = int(float(sys.argv[1]))
cases = [('Zi', 'Q', 4), ('Zs2', 'R', 4), ('Z7', 'R', 4), ('Zw', 'S', 2), ('Z11', 'R', 2), ('Zr2', 'E', 4)]
for nm, F, k in cases:
    R = Q.Ring(nm); a = (5, 0)
    tabs = RT.fd_tables(R, F, a, k, 1)
    for t in tabs[:2]:
        tab = t[0]
        line = RR.run(R, F, k, [('t', a, tab)], 0, B)[0]
        # census of cycles on F
        seen = {}; cycles = []; unres = 0
        starts = [z for z in (R.elements(B) if not R.real else []) if R.odd(z) and Q.inF(R, F, z)] if not R.real else None
        if R.real:
            import math
            s = math.sqrt(B); starts = [(x, y) for x in range(-int(2.5*s)-3, int(2.5*s)+4) for y in range(-int(1.8*s)-3, int(1.8*s)+4)
                                       if (x, y) != (0, 0) and abs(R.N((x, y))) <= B and R.odd((x, y)) and Q.zr2_inE((x, y))]
        for z in starts:
            if z in seen: continue
            path = []; idx = {}; w = z; res = None
            while True:
                if w in seen: res = seen[w]; break
                if w in idx: cycles.append(path[idx[w]:]); res = len(cycles) - 1; break
                if len(path) > 3000 or abs(w[0]) + abs(w[1]) > 10 ** 25: res = -1; unres += 1; break
                idx[w] = len(path); path.append(w)
                w = RR.apply_S(R, a, tab, k, w, F)[0]
            for p in path: seen[p] = res
        lens = sorted(len(c) for c in cycles)
        print(f"{nm} F={F} k={k} digits {RT.describe(R, t[3], t[4], k)[:80]} | {line.split(' ', 1)[1][:70]} | cycles on R/U to {B}: {len(cycles)} (lengths {lens[:12]}), unresolved {unres}")
