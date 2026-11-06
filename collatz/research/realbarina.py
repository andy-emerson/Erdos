"""realbarina.py: Barina-type maps read literally on R in real quadratic rings (real-quadratic.md, Sections 4 and 5).

For each ring and modulus pi^k, multipliers: every associate alpha eps^m in the per-embedding window (max_j D_j < 0),
plus the nearest unbalanced associates as controls. Tables: (C) commuting with -1 (k >= kappa + 1), a digit per
orbit {c, -c} among the nsmall smallest of its class; (N) all tables with digits among nsmall smallest (or a sample).
Filter: realfd.c literal mode (every odd start with |N| <= B1 in the box reaches a cycle containing a unit; escape
2^60), then B2. Each survivor is checked in Python: the units +-eps^n, |n| <= 40, and 200 random starts with
embeddings near 2^50 must reach a trivial cycle; recorded are the trivial cycles (number of units on each, mixed or
unit cycles), the periodic units (exponents), and how many units u (|n| <= 40) have S(u) a unit.
usage: python3 realbarina.py RING k nsmallC nsmallN [nsample]     log: logs/realq_barina_<RING>_k<k>.txt
"""
import sys, itertools, random, math, collections
import realq as Q, realrun as RN

L = []
def out(s): print(s, flush=True); L.append(s)


def pm1_orbits(R, k):
    seen = set(); orbs = []
    for i in R.odd_classes(k):
        if i in seen: continue
        j = R.idx(R.neg(R.classrep(i, k)), k)
        seen |= {i, j}; orbs.append((i, j))
    return orbs


def comm_tables(R, a, k, ns, nsample, rnd):
    cl = R.cand_lists(a, k, ns); orbs = pm1_orbits(R, k)
    if any(i == j for i, j in orbs): return []
    lists = [cl[i][0] for i, j in orbs]
    if nsample and math.prod(len(x) for x in lists) > nsample:
        chs = set(itertools.product(*[cl[i][1] for i, j in orbs]))
        n0 = len(chs)
        while len(chs) < nsample + n0: chs.add(tuple(rnd.choice(x) for x in lists))
        chs = sorted(chs)
    else: chs = itertools.product(*lists)
    res = []
    for ch in chs:
        t = {}
        for (i, j), d in zip(orbs, ch): t[i] = d; t[j] = R.neg(d)
        res.append(t)
    return res


def all_tables(R, a, k, ns, nsample, rnd):
    cl = R.cand_lists(a, k, ns); oc = R.odd_classes(k)
    lists = [cl[i][0] for i in oc]
    if nsample and math.prod(len(x) for x in lists) > nsample:
        chs = set(itertools.product(*[cl[i][1] for i in oc])); n0 = len(chs)
        while len(chs) < nsample + n0: chs.add(tuple(rnd.choice(x) for x in lists))
        chs = sorted(chs)
    else: chs = itertools.product(*lists)
    return [dict(zip(oc, ch)) for ch in chs]


def follow(R, a, tab, k, z, memo, cycles, cap=20000):
    """follow z to a cycle; memo maps points to cycle ids; returns cycle id or 'esc'/'cap'/'zero'"""
    path = []; pos = {}
    while True:
        if z in memo: res = memo[z]; break
        if z in pos:
            cyc = path[pos[z]:]; cycles.append(cyc); res = len(cycles) - 1
            break
        if max(R.lg(z)) > 300: res = 'esc'; break
        if len(path) > cap: res = 'cap'; break
        pos[z] = len(path); path.append(z)
        z = R.S(a, tab, k, z)[0]
        if z == (0, 0): res = 'zero'; break
    for p in path: memo[p] = res
    return res


def unit_census(R, a, tab, k, rnd, nunits=40, nrand=200):
    memo = {}; cycles = []; fates = collections.Counter()
    for n in range(-nunits, nunits + 1):
        for s in (1, -1):
            u = R.mul((s, 0), R.pw(R.eps, n))
            r = follow(R, a, tab, k, u, memo, cycles)
            fates['unit->' + ('cycle' if isinstance(r, int) else r)] += 1
    for _ in range(nrand):
        while True:
            z = (rnd.randint(-2 ** 50, 2 ** 50), rnd.randint(-2 ** 38, 2 ** 38))
            if R.odd(z): break
        r = follow(R, a, tab, k, z, memo, cycles)
        fates['rand->' + ('cycle' if isinstance(r, int) else r)] += 1
    triv = [c for c in cycles if any(R.is_unit(x) for x in c)]
    other = [c for c in cycles if not any(R.is_unit(x) for x in c)]
    per = []
    for c in triv:
        us = sorted(R.unit_exp(x) for x in c if R.is_unit(x))
        per.append((len(c), len(us), len(us) == len(c), us))
    tou = 0
    for n in range(-nunits, nunits + 1):
        for s in (1, -1):
            u = R.mul((s, 0), R.pw(R.eps, n))
            if R.is_unit(R.S(a, tab, k, u)[0]): tou += 1
    return triv, other, per, fates, tou


def parse_triv(line):
    if ' PASS trivial cycles ' not in line: return None
    return int(line.split(' PASS trivial cycles ')[1].split(':')[0])


def main():
    nm = sys.argv[1]; k = int(sys.argv[2]); nsC = int(sys.argv[3]); nsN = int(sys.argv[4])
    smp = int(sys.argv[5]) if len(sys.argv) > 5 else 0
    B1, B2 = 3000, 30000
    R = Q.RQ(nm); rnd = random.Random(13)
    kappa = {'Zr2': 2, 'Zr3': 2, 'Zg5': 1}[nm]
    out(f"# {nm}, modulus pi^{k} (pi = {R.fmt(R.pi)}), Barina-type maps read literally on R; kappa(+-1) = {kappa}; "
        f"filter |N| <= {B1} then {B2} (box starts), then units +-eps^n (|n| <= 40) and 200 random starts near 2^50")
    alphas = []
    for a0, n in Q.odd_multipliers(R, k):
        for m in range(-12, 13):
            a = R.mul(a0, R.pw(R.eps, m)); D = R.window_drifts(a, k)
            if max(D) < 0.25: alphas.append((a, n, D))
    for a, n, D in alphas:
        tag = 'per-embedding window' if max(D) < 0 else 'control (expands in an embedding)'
        for kind in ('C', 'N'):
            if kind == 'C':
                if k < kappa + 1: continue
                tabs = comm_tables(R, a, k, nsC, smp, rnd)
            else:
                tabs = all_tables(R, a, k, nsN, smp, rnd)
            if not tabs: continue
            res = RN.run(R, 'A', k, [(f't{j}', a, t) for j, t in enumerate(tabs)], 2, B1, cap=20000)
            p1 = [t for t, l in zip(tabs, res) if ' PASS ' in l]
            fails = collections.Counter()
            for l in res:
                if ' FAIL cycle' in l: fails['other cycle'] += 1
                elif ' FAIL code 1' in l: fails['escape'] += 1
                elif ' FAIL code 3' in l: fails['cap'] += 1
                elif ' FAIL code 2' in l: fails['zero'] += 1
            res2 = RN.run(R, 'A', k, [(f't{j}', a, t) for j, t in enumerate(p1)], 2, B2, cap=20000) if p1 else []
            p2 = [(t, l) for t, l in zip(p1, res2) if ' PASS ' in l]
            ncomm = sum(1 for t in tabs if R.commutes_pm1(t, k))
            out(f"\nalpha={R.fmt(a)} |N|={n} D=({D[0]:+.3f},{D[1]:+.3f}) [{tag}] tables {kind} ({len(tabs)}, {ncomm} commute with -1): "
                f"pass {B1}: {len(p1)}, pass {B2}: {len(p2)}; failures at {B1}: {dict(fails)}")
            shown = 0
            hist = collections.Counter()
            for t, l in p2:
                triv, other, per, fates, tou = unit_census(R, a, t, k, rnd)
                ok = not other and all(f.endswith('cycle') for f in fates)
                ntc = len(triv)
                hist[(ok, ntc, tuple(sorted((x[1], x[2]) for x in per)))] += 1
                if shown < 6:
                    shown += 1
                    out(f"  {Q.describe(R, t, k)} commutes(-1)={R.commutes_pm1(t, k)}: engine '{l.split(' PASS ')[1][:160]}'")
                    out(f"     census: Collatz map (units and random starts too): {ok}; trivial cycles {ntc}: " +
                        "; ".join(f"len {x[0]} with {x[1]} units{' (unit cycle)' if x[2] else ''} {[('-' if s < 0 else '') + 'e^' + str(m) for s, m in x[3]]}" for x in per) +
                        f"; other cycles {len(other)}; fates {dict(fates)}; units u (|n|<=40) with S(u) a unit: {tou}")
            if p2:
                out("  structure of the passes (Collatz map, number of trivial cycles, (units per cycle, unit cycle)): " +
                    "; ".join(f"{v}x {key}" for key, v in sorted(hist.items(), key=lambda x: -x[1])[:12]))
    open(f'logs/realq_barina_{nm}_k{k}.txt', 'w').write("\n".join(L) + "\n")


if __name__ == '__main__': main()
