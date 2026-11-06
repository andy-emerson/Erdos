"""realunitmap.py: how Barina-type maps read literally on R act on the units (real-quadratic.md, Section 4.4).

Part 1 (census over tables). For each listed (ring, k, alpha) and its tables (commuting with -1 if possible, and
not), every unit +-eps^n, |n| <= 40, is followed to its cycle. Recorded per map: number of trivial cycles, units per
trivial cycle, unit cycles versus mixed cycles, the periodic units, whether some unit escapes, and how many units
u = +-eps^n with |n| <= 200 have S(u) a unit. Distribution over tables.
Part 2 (detail). For the Collatz maps on R at sqrt2^4 and a few other maps: S(+-eps^n) for |n| <= 8, and the
explanation of fixed units by eta = pi^v - alpha (z = eta^-1 d(c) fixed when it lies in c).
log: logs/realq_unitmap.txt
"""
import random, collections
import realq as Q, realbarina as RB

L = []
def out(s): print(s, flush=True); L.append(s)


def unit_struct(R, a, tab, k, nmax=40):
    memo = {}; cycles = []; esc = 0
    for n in range(-nmax, nmax + 1):
        for s in (1, -1):
            r = RB.follow(R, a, tab, k, R.mul((s, 0), R.pw(R.eps, n)), memo, cycles, cap=20000)
            if not isinstance(r, int): esc += 1
    triv = [c for c in cycles if any(R.is_unit(x) for x in c)]
    per = sorted((sum(1 for x in c if R.is_unit(x)), all(R.is_unit(x) for x in c)) for c in triv)
    pu = sorted(R.unit_exp(x) for c in triv for x in c if R.is_unit(x))
    return len(triv), per, pu, esc, len(cycles) - len(triv)


def to_units(R, a, tab, k, nmax=200):
    c = 0
    for n in range(-nmax, nmax + 1):
        for s in (1, -1):
            if R.is_unit(R.S(a, tab, k, R.mul((s, 0), R.pw(R.eps, n)))[0]): c += 1
    return c


def main():
    rnd = random.Random(17)
    out("# Part 1. trivial cycles of literal maps, found by following +-eps^n, |n| <= 40")
    cases = [('Zr2', 4, (1, 3), 4, 3, 600), ('Zr2', 3, (3, 0), 6, 4, 0), ('Zr3', 1, (3, 2), 0, 60, 0),
             ('Zr3', 3, (21, 12), 6, 4, 0), ('Zg5', 1, (-1, 2), 0, 8, 0), ('Zg5', 2, (5, 0), 3, 2, 600),
             ('Zr2', 2, (3, 1), 0, 12, 0)]
    for nm, k, a, nsC, nsN, smp in cases:
        R = Q.RQ(nm); D = R.window_drifts(a, k)
        for kind in ('C', 'N'):
            if kind == 'C':
                if not nsC: continue
                tabs = RB.comm_tables(R, a, k, nsC, smp, rnd)
            else:
                tabs = RB.all_tables(R, a, k, nsN, smp, rnd)
                tabs = [t for t in tabs if not R.commutes_pm1(t, k)]
            hist = collections.Counter(); cnt = collections.Counter(); ex = {}
            for t in tabs:
                ntc, per, pu, esc, oth = unit_struct(R, a, t, k)
                cnt[ntc] += 1
                shape = 'unit cycles only' if all(x[1] for x in per) else 'some mixed cycles'
                ups = collections.Counter(x[0] for x in per)
                hist[(shape, tuple(sorted(ups.items())))] += 1
                if ntc not in ex: ex[ntc] = (t, per, pu, esc)
            out(f"\n{nm} k={k} alpha={R.fmt(a)} D=({D[0]:+.3f},{D[1]:+.3f}) tables {kind} "
                f"({'commute with -1' if kind == 'C' else 'do not commute with -1'}): {len(tabs)}")
            out("  number of trivial cycles: " + ", ".join(f"{n}: {c}" for n, c in sorted(cnt.items())))
            out("  (cycle types, {units per trivial cycle: count}): " + "; ".join(f"{v}x {key[0]} {dict(key[1])}" for key, v in sorted(hist.items(), key=lambda x: -x[1])[:8]))
            for n in (min(cnt), max(cnt)):
                t, per, pu, esc = ex[n]
                out(f"  example with {n} trivial cycles: {Q.describe(R, t, k)}; periodic units "
                    f"{[('-' if s < 0 else '') + 'e^' + str(m) for s, m in pu]}; units escaping: {esc}; "
                    f"units |n| <= 200 with S(u) a unit: {to_units(R, a, t, k)}")
    out("\n# Part 2. S on units for the Collatz maps on R at sqrt2^4")
    R = Q.RQ('Zr2'); k = 4
    for a in ((1, 3), (-1, 3)):
        for t in RB.comm_tables(R, a, k, 4, 0, rnd):
            from realrun import run
            l = run(R, 'A', k, [('x', a, t)], 2, 30000, cap=20000)[0]
            if ' PASS ' not in l: continue
            out(f"alpha={R.fmt(a)} {Q.describe(R, t, k)}")
            for v in range(k, k + 4):
                eta = R.sub(R.pw(R.pi, v), a)
                out(f"  eta = pi^{v} - alpha = {R.fmt(eta)}, N = {R.N(eta)}{' (a unit: ' + R.ufmt(eta) + ')' if R.is_unit(eta) else ''}")
            row = []
            for n in range(-8, 9):
                u = R.pw(R.eps, n); w, v = R.S(a, t, k, u)
                row.append(f"e^{n}->{R.ufmt(w) if R.is_unit(w) else R.fmt(w)}(v={v})")
            out("  " + ", ".join(row))
    open('logs/realq_unitmap.txt', 'w').write("\n".join(L) + "\n")


if __name__ == '__main__': main()
