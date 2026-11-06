"""ringreal.py: Barina-type maps on Z[sqrt 2] read literally (rings-test.md, Section on Z[sqrt2]).

Starts: odd a + b sqrt2 with |a|, |b| <= X. Each orbit of S is followed until it closes a cycle, escapes
(max(|a|,|b|) > 10^30) or runs cap steps. Reported: trivial cycles (containing a unit, |N| = 1), other cycles,
escapes, and for escaping orbits the median |N| at escape and which embedding is the larger.
usage: python3 ringreal.py X cap
"""
import sys, math, collections, itertools
import qring as Q, ringrun as RR, ringtest as RT

R = Q.Ring('Zr2')
X = int(sys.argv[1]) if __name__ == '__main__' else 30; CAP = int(sys.argv[2]) if __name__ == '__main__' else 3000


def census(alpha, tab, k):
    seen = {}; cycles = []; esc = []; capn = 0
    for a in range(-X, X + 1):
        for b in range(-X, X + 1):
            z = (a, b)
            if not R.odd(z) or z in seen: continue
            path = []; idx = {}; w = z; res = None
            while True:
                if w in seen: res = seen[w]; break
                if w in idx: cycles.append(path[idx[w]:]); res = ('c', len(cycles) - 1); break
                if max(abs(w[0]), abs(w[1])) > 10 ** 30: res = ('e',); esc.append(w); break
                if len(path) > CAP: res = ('cap',); capn += 1; break
                idx[w] = len(path); path.append(w)
                w = RR.apply_S(R, alpha, tab, k, w)[0]
                if w == (0, 0): res = ('zero',); break
            for p in path: seen[p] = res
    triv = [c for c in cycles if any(abs(R.N(x)) == 1 for x in c)]
    other = [c for c in cycles if c not in triv]
    return triv, other, esc, capn


def emb(z):
    s1, s2 = R.cx(z)
    return math.log2(abs(s1)) if s1 else -99, math.log2(abs(s2)) if s2 else -99


def main():
    L = [f"# Barina-type maps on Z[sqrt2] read literally, starts |a|, |b| <= {X}, cap {CAP} odd steps"]
    cases = [((3, 1), 2), ((1, 2), 2), ((3, 0), 3), ((-1, 3), 4), ((1, 3), 4), ((5, 0), 4), ((5, 1), 4)]
    for alpha, k in cases:
        s1, s2 = R.cx(alpha); lim = (k + 1) / 2
        L.append(f"alpha={Q.fmt(R, alpha)} N={R.N(alpha)} k={k}: log2|sigma1| - (k+1)/2 = {math.log2(abs(s1)) - lim:+.3f}, "
                 f"log2|sigma2| - (k+1)/2 = {math.log2(abs(s2)) - lim:+.3f} (drift per odd step in each embedding)")
        cl = RT.cand_lists(R, alpha, k, 1)
        oc = R.odd_classes(k)
        for ch in itertools.product(*[cl[i][1] for i in oc]):
            tab = dict(zip(oc, ch))
            com = all(tab[R.idx(R.neg(R.classrep(i, k)), k)] == R.neg(tab[i]) for i in oc) and \
                all(R.idx(R.neg(R.classrep(i, k)), k) != i for i in oc)
            triv, other, esc, capn = census(alpha, tab, k)
            ex = ''
            if esc:
                e = [emb(w) for w in esc[:200]]
                big1 = sum(1 for x in e if x[0] > x[1])
                ex = (f" escapes: |N| at escape median {sorted(abs(R.N(w)) for w in esc)[len(esc)//2]}, "
                      f"sigma1 larger in {big1} of {len(e)}")
            L.append(f"   digits {RT.describe(R, ch, oc, k)} commutes with -1: {com}: trivial cycles {len(triv)} "
                     f"(units {[sorted(Q.fmt(R, x) for x in c if abs(R.N(x)) == 1)[:4] for c in triv][:4]}), other cycles {len(other)} "
                     f"(min |N| {[min(abs(R.N(x)) for x in c) for c in other][:6]}), escaped {len(esc)}, cap {capn}{ex}")
    RT.log(R, 'real_literal', L)


if __name__ == '__main__': main()
