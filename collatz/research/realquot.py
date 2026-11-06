"""realquot.py: the quotient form of the Collatz property (real-quadratic.md, Section 4.5).

Quotient form: every nonzero (here: odd) orbit eventually enters U and stays there, S(U) in U, no finiteness of
the unit orbit required.
Part 1. Lifts L(u f) = u oddpart(alpha f + d(f)) of the Collatz-type maps on R/U (cone E, as in realunits.py).
  L(u) = u L(1), so S(U) in U iff L(1) is a unit, i.e. p = 1; then units move by u -> u w, w = L(1).
  Checked: L(+-eps^n) = +-eps^n w for |n| <= 40, and every start z = u f (f in E, |N(f)| <= 2000, u = +-eps^m,
  |m| <= 3) enters U. Examples in full; the p > 1 maps for contrast (L(1) not a unit; L^p(U) in U).
Part 2. Residue-class maps read literally: no table satisfies S(U) in U (only finitely many units go to units,
  by the S-unit theorem); counted: tables where every box start reaches a unit (the weak half), and the number
  of units +-eps^n, |n| <= 200, with S(u) a unit.
Part 3. Norm of the fundamental unit: the sign of N along unit walks.
log: logs/realq_quotient.txt
"""
import itertools, random, collections
import realq as Q, realrun as RN, realunits as RU, realbarina as RB

L = []
def out(s): print(s, flush=True); L.append(s)


def main():
    rnd = random.Random(23)
    out("# Part 1. lifts of Collatz-type maps on R/U: quotient-form Collatz maps (p = 1) and the rest")
    for nm, k, ns, smp in (('Zr2', 2, 12, 0), ('Zr2', 3, 5, 0), ('Zr3', 1, 40, 0), ('Zr3', 3, 4, 0), ('Zg5', 1, 8, 0), ('Zg5', 2, 2, 300)):
        R = Q.RQ(nm)
        stat = collections.Counter(); shown = collections.Counter()
        for a, n in Q.odd_multipliers(R, k):
            tabs = RU.tables(R, a, k, ns, smp, rnd)
            res = RN.run(R, 'E', k, [(f't{j}', a, t) for j, t in enumerate(tabs)], 0, 2 * 10 ** 4)
            for t, line in zip(tabs, res):
                if ' PASS ' not in line: continue
                p = int(line.split('len ')[1].split(':')[0])
                w1 = RU.lift_step(R, a, t, k, (1, 0))
                if not R.is_unit(w1):
                    stat['p>1: L(1) not a unit, S(U) not in U'] += 1
                    if shown['p>1'] < 1:
                        shown['p>1'] += 1
                        wp = RU.lift_w(R, a, t, k, p)
                        out(f"  {nm} k={k} alpha={R.fmt(a)} {Q.describe(R, t, k)}: p={p}, L(1)={R.fmt(w1)} (|N|={abs(R.N(w1))}), L^p(1)={R.ufmt(wp)}")
                    continue
                sg, j = R.unit_exp(w1)
                kind = 'w=1: every unit fixed' if (sg, j) == (1, 0) else ('w=-1: 2-cycles {u,-u}' if j == 0 else f"w={'-' if sg < 0 else ''}eps^{j}: {2 * abs(j)} endless walks u -> u w")
                stat[kind] += 1
                # checks
                okU = all(RU.lift_step(R, a, t, k, R.mul((s, 0), R.pw(R.eps, m))) == R.mul(R.mul((s, 0), R.pw(R.eps, m)), w1)
                          for m in range(-40, 41) for s in (1, -1))
                if shown[kind] < 2:
                    shown[kind] += 1
                    g, wk, bad, tot = RU.check_lift(R, a, t, k, w1)
                    out(f"  {nm} k={k} alpha={R.fmt(a)} |N|={n} digits {Q.describe(R, t, k)}: L(1) = {R.ufmt(w1)} ({kind}); "
                        f"L(u) = u L(1) on +-eps^n, |n| <= 40: {okU}; starts entering U: {g + wk} of {tot} ({bad} not)")
        out(f"{nm} k={k}: " + "; ".join(f"{v} {key}" for key, v in sorted(stat.items())))
    out("\n# Part 2. literal residue-class maps: the weak half (every box start |a|,|b| <= 25 reaches a unit) and S(U)")
    for nm, k, a, kind, ns in (('Zr2', 4, (1, 3), 'C', 4), ('Zr2', 4, (-1, 3), 'C', 4), ('Zr2', 3, (3, 0), 'C', 6),
                               ('Zr3', 1, (3, 2), 'N', 60), ('Zr3', 3, (21, 12), 'C', 6), ('Zg5', 1, (-1, 2), 'N', 8), ('Zg5', 2, (5, 0), 'C', 2)):
        R = Q.RQ(nm)
        tabs = RB.comm_tables(R, a, k, ns, 0, rnd) if kind == 'C' else RB.all_tables(R, a, k, ns, 0, rnd)
        weak = 0; mx = 0; mxw = 0
        starts = [(x, y) for x in range(-25, 26) for y in range(-25, 26) if R.odd((x, y))]
        for t in tabs:
            ok = True
            for z in starts:
                n = 0
                while not R.is_unit(z) and n < 3000 and max(R.lg(z)) < 200:
                    z = R.S(a, t, k, z)[0]; n += 1
                    if z == (0, 0): break
                if not R.is_unit(z): ok = False; break
            tu = RU_to_units(R, a, t, k)
            mx = max(mx, tu)
            if ok: weak += 1; mxw = max(mxw, tu)
        out(f"{nm} k={k} alpha={R.fmt(a)} tables {kind} ({len(tabs)}): every start reaches a unit in {weak}; "
            f"units +-eps^n (|n| <= 200, 802 units) with S(u) a unit: at most {mx} (at most {mxw} among the weak ones)")
    out("\n# Part 3. sign of N along unit walks (N(eps) = -1 in Zr2, Zg5; +1 in Zr3)")
    for nm in ('Zr2', 'Zr3', 'Zg5'):
        R = Q.RQ(nm)
        out(f"{nm}: N(eps) = {R.N(R.eps)}, N(pi) = {R.Npi}; units of norm -1: {'eps^odd' if R.N(R.eps) == -1 else 'none'}; "
            f"a walk u -> u eps^j alternates the sign of N iff N(eps) = -1 and j is odd")
    open('logs/realq_quotient.txt', 'w').write("\n".join(L) + "\n")


def RU_to_units(R, a, t, k, nmax=200):
    c = 0
    for n in range(-nmax, nmax + 1):
        for s in (1, -1):
            if R.is_unit(R.S(a, t, k, R.mul((s, 0), R.pw(R.eps, n)))[0]): c += 1
    return c


if __name__ == '__main__': main()
