"""fdsummary.py: summary of the fdrun.py logs (counts, commuting tables, first maps, mirror check).
Output: logs/fdsummary.txt
"""
import re, os, fdrun
HERE = os.path.dirname(os.path.abspath(__file__))
U = [(1, 0), (0, 1), (-1, 0), (0, -1)]

def commutes(tab):
    t = dict(zip(fdrun.OC, tab))
    for c in fdrun.OC:
        for u in U[1:]:
            uc = fdrun.mul(u, c); uc = (uc[0] % 4, uc[1] % 4)
            if t[uc] != fdrun.mul(u, t[c]): return False
    return True

def conj(z): return (z[0], -z[1])

def load(F, k, n):
    path = os.path.join(HERE, "logs", f"fd_{F}_k{k}_n{n}.txt")
    if not os.path.exists(path): return None
    tabs = {}
    for a in fdrun.ALPHAS[k]:
        for j, (tab, choice, reps) in enumerate(fdrun.tables(F, a, k, n)):
            tabs[f"a{fdrun.fmt(a)}_{j}"] = (a, tab, sum(fdrun.N(d) for d in choice))
    res = {}
    for line in open(path).read().split("\n")[2 + len(fdrun.ALPHAS[k]):]:
        if not line.strip(): continue
        tag = line.split()[0]
        ok = "stage2: PASS" in line
        res[tag] = ok
    return tabs, res

out = []
for k, n in ((2, 25), (3, 25), (4, 5), (4, 13)):
    for F in ('Q', 'R', 'C', 'P'):
        L = load(F, k, n)
        if L is None: continue
        tabs, res = L
        out.append(f"F={F} k={k} digits norm <= {n}:")
        for a in fdrun.ALPHAS[k]:
            tags = [t for t in tabs if tabs[t][0] == a]
            ps = [t for t in tags if res.get(t)]
            comm = [t for t in ps if commutes(tabs[t][1])] if F != 'P' else []
            first = min(ps, key=lambda t: tabs[t][2]) if ps else None
            minnorm = min((tabs[t][2] for t in tags), default=None)
            nmin = sum(1 for t in ps if tabs[t][2] == minnorm)
            out.append(f"  alpha {fdrun.fmt(a)} N={fdrun.N(a)}: tables {len(tags)}, Collatz to 1e6: {len(ps)}"
                       + (f", commuting among them {len(comm)}" if F != 'P' else "")
                       + f", smallest-digit tables passing {nmin}"
                       + (f", least total digit norm {tabs[first][2]}: " + " ".join(fdrun.fmt(d) for d in tabs[first][1]) if first else ""))
        # mirror check for C (conjugation symmetric on odd elements)
        if F == 'C':
            passset = {(tabs[t][0], tuple(tabs[t][1])) for t in tabs if res.get(t)}
            bad = 0
            for (a, tab) in passset:
                t = dict(zip(fdrun.OC, tab))
                mt = tuple(conj(t[((c[0]) % 4, (-c[1]) % 4)]) for c in fdrun.OC)
                if (conj(a), mt) not in passset: bad += 1
            out.append(f"  mirror check (alpha,d) passes iff (conj alpha, conj d conj) passes: {len(passset)} passing maps, {bad} without passing mirror")
open(os.path.join(HERE, "logs", "fdsummary.txt"), "w").write("\n".join(out) + "\n")
print("\n".join(out))
