"""fdrun.py: search for Collatz maps on Z[i]/U among fundamental-domain maps (fdsearch.c).

usage: python3 fdrun.py F k maxnorm [B] [B2]
  F in Q, R, C, P; k in 2, 3, 4; digits per class: all elements of the class of -alpha c mod pi^k
  with norm <= maxnorm; multipliers: the window norms at pi^k, one per class of associates
  (associates give the same map on F, Section 2 of fundamental-domain.md), conjugates included.
  Stage 1: full classification of odd z in F with N(z) <= B (default 1e5).
  Stage 2: survivors, drop-below-start for B < N(z) <= B2 (default 1e6).
Output: logs/fd_<F>_k<k>_n<maxnorm>.txt (all PASS lines, failure statistics).
"""
import sys, subprocess, itertools, collections, os
HERE = os.path.dirname(os.path.abspath(__file__))
OC = [(0, 1), (0, 3), (1, 0), (1, 2), (2, 1), (2, 3), (3, 0), (3, 2)]
ALPHAS = {2: [(2, 1), (2, -1)], 3: [(3, 0), (3, 2), (3, -2)],
          4: [(4, 1), (4, -1), (5, 0), (4, 3), (4, -3), (5, 2), (5, -2)]}

def mul(x, y): return (x[0]*y[0]-x[1]*y[1], x[0]*y[1]+x[1]*y[0])
def N(x): return x[0]**2+x[1]**2
def dvd(x, k):
    a, b = x[0] % 4, x[1] % 4
    return {1: (a+b) % 2 == 0, 2: a % 2 == 0 and b % 2 == 0,
            3: (a, b) in ((0, 0), (2, 2)), 4: a == 0 and b == 0}[k]
def groups(k):
    gs = []
    for c in OC:
        for g in gs:
            if dvd((c[0]-g[0][0], c[1]-g[0][1]), k): g.append(c); break
        else: gs.append([c])
    return gs
def relevant(F, g):
    if F != 'P': return True
    return any(c in ((1, 0), (3, 2)) for c in g)
def cands(alpha, c, k, maxnorm):
    t = mul((-alpha[0], -alpha[1]), c)
    r = int(maxnorm**0.5)+1
    L = [(x, y) for x in range(-r, r+1) for y in range(-r, r+1)
         if x*x+y*y <= maxnorm and dvd((x-t[0], y-t[1]), k)]
    import math
    L.sort(key=lambda z: (N(z), math.atan2(z[1], z[0])))
    return L

def tables(F, alpha, k, maxnorm):
    gs = groups(k)
    lists = []
    for g in gs:
        cl = cands(alpha, g[0], k, maxnorm)
        lists.append(cl if relevant(F, g) else cl[:1])
    for choice in itertools.product(*lists):
        t = {}
        for g, d in zip(gs, choice):
            for c in g: t[c] = d
        yield [t[c] for c in OC], [choice[i] for i, g in enumerate(gs) if relevant(F, g)], [g[0] for g in gs if relevant(F, g)]

def fmt(z): return f"{z[0]}{z[1]:+d}i"

def run(F, mode, B, N0, jobs, cap=100000):
    inp = f"{F} {B} {cap} {mode} {N0}\n" + "".join(
        f"{tag} {a[0]} {a[1]} " + " ".join(f"{d[0]} {d[1]}" for d in tab) + "\n" for tag, a, tab in jobs)
    p = subprocess.run([os.path.join(HERE, "fdsearch")], input=inp, capture_output=True, text=True)
    return p.stdout.strip().split("\n") if p.stdout.strip() else []

def main():
    F, k, maxnorm = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
    B = int(float(sys.argv[4])) if len(sys.argv) > 4 else 10**5
    B2 = int(float(sys.argv[5])) if len(sys.argv) > 5 else 10**6
    jobs, info = [], {}
    for alpha in ALPHAS[k]:
        for j, (tab, choice, reps) in enumerate(tables(F, alpha, k, maxnorm)):
            tag = f"a{fmt(alpha)}_{j}"
            jobs.append((tag, alpha, tab))
            info[tag] = (alpha, choice, reps, sum(N(d) for d in choice))
    out = run(F, 0, B, 0, jobs)
    passed = [l.split()[0] for l in out if " PASS " in l]
    reasons = collections.Counter()
    per_alpha = collections.Counter(); tot_alpha = collections.Counter()
    for l in out:
        tag = l.split()[0]; tot_alpha[info[tag][0]] += 1
        if " PASS " in l: per_alpha[info[tag][0]] += 1
        else:
            r = l.split(" FAIL ")[1]
            reasons["cycle" if r.startswith("cycle") else "1 not periodic" if r.startswith("1 not") else "escape/cap/zero"] += 1
    stage1 = {l.split()[0]: l for l in out if " PASS " in l}
    jobs2 = [j for j in jobs if j[0] in stage1]
    out2 = run(F, 1, B2, B, jobs2) if jobs2 else []
    stage2 = {l.split()[0]: l for l in out2}
    lines = [f"F={F} k={k} digits of norm <= {maxnorm} per class; stage 1 full to N <= {B}; stage 2 drop-below-start to {B2}",
             f"maps tested {len(jobs)}; pass stage 1 {len(passed)}; failures {dict(reasons)}"]
    for a in ALPHAS[k]:
        p2 = sum(1 for t in stage2 if info[t][0] == a and " PASS " in stage2[t])
        lines.append(f"  alpha {fmt(a)} (norm {N(a)}): tested {tot_alpha[a]}, pass {per_alpha[a]} at {B}, pass {p2} at {B2}")
    rows = []
    for t in passed:
        a, choice, reps, tn = info[t]
        ok2 = " PASS " in stage2.get(t, "")
        rows.append((N(a), tn, t, a, choice, reps, ok2))
    rows.sort()
    for (na, tn, t, a, choice, reps, ok2) in rows:
        dig = ", ".join(f"[{fmt(r)}]->{fmt(d)}" for r, d in zip(reps, choice))
        lines.append(f"{t} alpha={fmt(a)} total digit norm {tn} table {dig} | {stage1[t].split(' PASS ')[1]} | stage2: {stage2.get(t, '').split(t)[-1].strip()}")
    path = os.path.join(HERE, "logs", f"fd_{F}_k{k}_n{maxnorm}.txt")
    open(path, "w").write("\n".join(lines) + "\n")
    print("\n".join(lines[:3 + len(ALPHAS[k])]))
    print(f"(full list in {path})")

if __name__ == "__main__":
    main()
