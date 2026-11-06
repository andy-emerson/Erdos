"""realwin.py: test of the per-embedding window (real-quadratic.md, Section 3).

For each ring, modulus pi^k and multiplier alpha (classes of associates with |N| near the norm window, and the
associates alpha eps^m around the balanced one), a residue-class table with smallest digits is read literally on R.
From NS random starts with both embeddings near 2^40, each orbit is followed; while both log2|sigma_j| stay above
12 bits (digits negligible) the mean change of log2|sigma_j| per odd step is accumulated and compared with the
predicted drift D_j = log2|sigma_j(alpha)| - (k + 1/(q-1)) log2|sigma_j(pi)|. The orbit is then run on to a fate:
'esc' (an embedding above 2^400; the larger embedding at escape is recorded), a trivial cycle (contains a unit),
another cycle, or 'cap'.
usage: python3 realwin.py [NS] [CAP]      log: logs/realq_window.txt
"""
import sys, math, random, collections
import realq as Q

NS = int(sys.argv[1]) if len(sys.argv) > 1 else 60
CAP = int(sys.argv[2]) if len(sys.argv) > 2 else 3000
L = []
def out(s): print(s); L.append(s)


def smallest_table(R, alpha, k, pick=0, nsmall=1, rnd=None):
    cl = R.cand_lists(alpha, k, nsmall)
    if rnd is None: return {i: cl[i][1][min(pick, len(cl[i][1]) - 1)] for i in R.odd_classes(k)}
    return {i: rnd.choice(cl[i][0]) for i in R.odd_classes(k)}


def run(R, alpha, tab, k, rnd):
    tot = [0.0, 0.0]; steps = 0; vsum = 0; vn = 0
    fate = collections.Counter(); escbig = collections.Counter()
    for s in range(NS):
        while True:
            z = (rnd.randint(-2 ** 40, 2 ** 40), rnd.randint(-2 ** 28, 2 ** 28))
            if R.odd(z):
                l = R.lg(z)
                if min(l) > 30: break
        l0 = R.lg(z); lp = l0; n = 0; phase = True
        seen = {}; res = None; z0 = z
        for n in range(CAP):
            z, v = R.S(alpha, tab, k, z)
            assert v >= k or z == (0, 0); vsum += v; vn += 1
            if z == (0, 0): res = 'zero'; break
            l = R.lg(z)
            if phase:
                if min(l) < 12 or n >= 400:
                    phase = False
                    tot[0] += lp[0] - l0[0]; tot[1] += lp[1] - l0[1]; steps += n
            lp = l
            if max(l) > 400:
                res = 'esc'; escbig['sigma1' if l[0] > l[1] else 'sigma2'] += 1; break
            if max(l) < 60:
                if z in seen:
                    # cycle: collect
                    cyc = [z]; w = R.S(alpha, tab, k, z)[0]
                    while w != z: cyc.append(w); w = R.S(alpha, tab, k, w)[0]
                    res = 'trivial' if any(R.is_unit(x) for x in cyc) else 'other'
                    break
                seen[z] = n
        if res is None:
            res = 'cap'
            if phase: tot[0] += lp[0] - l0[0]; tot[1] += lp[1] - l0[1]; steps += n
        fate[res] += 1
    sl = (tot[0] / steps, tot[1] / steps) if steps else (float('nan'),) * 2
    return sl, vsum / vn, fate, escbig, steps


def main():
    rnd = random.Random(7)
    out(f"# per-embedding window test: smallest-digit tables read literally on R; {NS} starts with embeddings near 2^40; "
        f"slopes = mean change of log2|sigma_j| per odd step while both embeddings exceed 2^12; fates after up to {CAP} odd steps")
    out("# columns: alpha (assoc. exponent m relative to the cone representative), |N|, norm drift, predicted D1 D2, "
        "measured slope1 slope2, mean v (predicted k + 1/(q-1)), fates")
    plan = {'Zr2': [1, 2, 3, 4, 5], 'Zr3': [1, 2, 3, 4], 'Zg5': [1, 2, 3]}
    for nm, ks in plan.items():
        R = Q.RQ(nm)
        out(f"\n## {nm}: pi = {R.fmt(R.pi)}, q = {R.q}, log2|sigma(pi)| = ({R.lpi[0]:.3f}, {R.lpi[1]:.3f}), "
            f"eps = {R.fmt(R.eps)}, log2 eps = {R.leps:.3f}")
        for k in ks:
            mlist = Q.odd_multipliers(R, k)
            # also one class just below and one just above the window
            lo, hi = R.q ** k, R.q ** (k + 1 / (R.q - 1))
            extra = {}
            for z in R.elements(int(8 * hi) + 40):
                if not R.odd(z) or R.is_unit(z): continue
                n = abs(R.N(z))
                if n < lo or n > hi:
                    r, _ = R.rep(z)
                    if r not in extra: extra[r] = n
            below = sorted([x for x in extra.items() if x[1] < lo], key=lambda x: -x[1])[:1]
            above = sorted([x for x in extra.items() if x[1] > hi], key=lambda x: x[1])[:1]
            cases = [(a, n, 'in') for a, n in mlist[:4]] + [(a, n, 'below') for a, n in below] + [(a, n, 'above') for a, n in above]
            out(f"k={k}: norm window ({lo}, {hi:.2f})")
            for a0, n, where in cases:
                D0 = R.window_drifts(a0, k)
                # balanced exponent: minimize max(D1 + m leps, D2 - m leps)
                mb = round((D0[1] - D0[0]) / (2 * R.leps))
                for m in range(mb - 1, mb + 2):
                    a = R.mul(a0, R.pw(R.eps, m))
                    D = R.window_drifts(a, k)
                    tab = smallest_table(R, a, k)
                    assert R.table_ok(a, tab, k)
                    sl, mv, fate, eb, st = run(R, a, tab, k, rnd)
                    ok = 'per-emb window holds' if max(D) < 0 else 'expands in an embedding'
                    out(f"  {where:5s} {R.fmt(a)} (m={m:+d}) |N|={n} normdrift={sum(D):+.3f} D=({D[0]:+.3f},{D[1]:+.3f}) "
                        f"slope=({sl[0]:+.3f},{sl[1]:+.3f}) [{st} steps] v={mv:.3f} ({k + 1/(R.q-1):.3f}) "
                        f"fates {dict(fate)} {('escape in ' + str(dict(eb))) if eb else ''} | {ok}")
    open('logs/realq_window.txt', 'w').write("\n".join(L) + "\n")


if __name__ == '__main__': main()
