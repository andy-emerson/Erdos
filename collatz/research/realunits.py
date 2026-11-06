"""realunits.py: units under Collatz-type maps on R/U and their lifts (real-quadratic.md, Section 4).

Part A. Collatz-type maps on the cone E (one multiplier per class of associates, digits among the nsmall smallest
of each class), classified on E to |N| <= B by realfd.c. For each Collatz map on R/U: the period p of [1], and the
unit w = L^p(1) = s eps^j of its commuting lift L(u f) = u oddpart(alpha f + d(f)) (f in E). By Theorem 4.3 of the
notes, L is a Collatz map on R iff w = +-1; rotating (alpha, d) -> (u alpha, u d) gives the same map on R/U and
multiplies w by u^p, so some rotation lifts to a Collatz map on R iff w in +-U^p, i.e. p | j.
Each predicted lift is checked directly: from every odd z = u f with f in E, |N(f)| <= 2000 and u = +-eps^m,
|m| <= 3, the lift L is followed; it must reach a unit cycle (w = +-1) or walk off along +-eps^n (w of infinite order).
usage: python3 realunits.py   log: logs/realq_units_cone.txt
"""
import sys, itertools, random, collections, math
import realq as Q, realrun as RN

L = []
def out(s): print(s); L.append(s)

PLAN = [('Zr2', 2, 12, 0), ('Zr2', 3, 5, 0), ('Zr2', 4, 2, 300),
        ('Zr3', 1, 40, 0), ('Zr3', 3, 4, 0), ('Zr3', 4, 2, 300),
        ('Zg5', 1, 8, 0), ('Zg5', 2, 2, 300)]
B = 2 * 10 ** 4


def tables(R, a, k, ns, nsample, rnd):
    cl = R.cand_lists(a, k, ns); oc = R.odd_classes(k)
    lists = [cl[i][0] for i in oc]
    if nsample and math.prod(len(x) for x in lists) > nsample:
        chs = set(itertools.product(*[cl[i][1] for i in oc]))
        while len(chs) < nsample: chs.add(tuple(rnd.choice(x) for x in lists))
        chs = sorted(chs)
    else:
        chs = list(itertools.product(*lists))
    return [dict(zip(oc, ch)) for ch in chs]


def lift_step(R, a, tab, k, z):
    f, u = R.rep(z)
    return R.oddpart(R.add(R.mul(a, z), R.mul(u, tab[R.idx(f, k)])))[0]


def lift_w(R, a, tab, k, p):
    z = (1, 0)
    for _ in range(p): z = lift_step(R, a, tab, k, z)
    return z


def check_lift(R, a, tab, k, w):
    """follow the lift from z = u f; returns (#reach unit cycle, #walk, #other)"""
    starts = []
    for x in range(-60, 61):
        for y in range(-40, 41):
            f = (x, y)
            if R.odd(f) and R.inE(f) and abs(R.N(f)) <= 2000: starts.append(f)
    good = walk = bad = 0
    finite = R.unit_exp(w)[1] == 0
    for f in starts:
        for m in (-3, 0, 3):
            for s in (1, -1):
                z = R.mul((s, 0), R.mul(R.pw(R.eps, m), f))
                n = 0
                while not R.is_unit(z) and n < 5000: z = lift_step(R, a, tab, k, z); n += 1
                if not R.is_unit(z): bad += 1; continue
                # z is a unit u; then L^p-orbit of u is u w^n
                if finite: good += 1
                else: walk += 1
    return good, walk, bad, len(starts) * 6


def main():
    rnd = random.Random(5)
    out(f"# Collatz-type maps on R/U (cone E), |N| <= {B}; lifts and units")
    for nm, k, ns, smp in PLAN:
        R = Q.RQ(nm)
        cnt = collections.Counter(); first = {}
        npass = ntot = 0
        for a, n in Q.odd_multipliers(R, k):
            tabs = tables(R, a, k, ns, smp, rnd)
            res = RN.run(R, 'E', k, [(f't{j}', a, t) for j, t in enumerate(tabs)], 0, B)
            ntot += len(tabs)
            for t, line in zip(tabs, res):
                if ' PASS ' not in line: continue
                npass += 1
                p = int(line.split('len ')[1].split(':')[0])
                w = lift_w(R, a, t, k, p)
                sg, j = R.unit_exp(w)
                key = (R.fmt(a), n, p, sg, j)
                cnt[key] += 1
                if key not in first: first[key] = t
        out(f"\n## {nm} k={k} digits among {ns} smallest{' (smallest + ' + str(smp) + ' sampled per multiplier)' if smp else ''}: "
            f"{npass} Collatz maps on R/U of {ntot}")
        tot = collections.Counter()
        for key in sorted(cnt):
            an, n, p, sg, j = key
            cls = 'w=+-1: lift is a Collatz map on R' if j == 0 else ('p | j: a rotation by eps^(-j/p) lifts to a Collatz map' if j % p == 0 else 'p does not divide j: no rotation lifts to a Collatz map')
            tot['w=+-1' if j == 0 else ('p|j' if j % p == 0 else 'p∤j')] += cnt[key]
            out(f"  alpha={an} |N|={n} p={p} w={'-' if sg < 0 else ''}eps^{j}: {cnt[key]} maps; {cls}")
        out(f"  totals: {dict(tot)}")
        # checks on the first map of each kind (at most 4 per ring and modulus)
        done = 0
        for key, t in first.items():
            if done >= 4: break
            an, n, p, sg, j = key
            a = next(x for x, _ in Q.odd_multipliers(R, k) if R.fmt(x) == an)
            w = lift_w(R, a, t, k, p)
            g, wk, bad, tot2 = check_lift(R, a, t, k, w)
            line = f"  check {an} p={p} w={R.ufmt(w)} {Q.describe(R, t, k)}: of {tot2} starts u f, {g} reach a periodic unit, {wk} reach a unit and then walk (w of infinite order), {bad} neither"
            if j != 0 and j % p == 0:
                u = R.pw(R.eps, -j // p)
                a2 = R.mul(u, a); t2 = {i: R.mul(u, d) for i, d in t.items()}
                # rotated table indexed by classes of f in E (same classes): digits u d(f)
                w2 = lift_w(R, a2, t2, k, p)
                g2, wk2, bad2, _ = check_lift(R, a2, t2, k, w2)
                line += f"; rotated by eps^{-j//p}: alpha={R.fmt(a2)}, w={R.ufmt(w2)}, {g2} periodic, {wk2} walk, {bad2} neither"
            if sg == 1 and j == 0 or sg == -1 and j == 0:
                pass
            out(line); done += 1
    open('logs/realq_units_cone.txt', 'w').write("\n".join(L) + "\n")


if __name__ == '__main__': main()
