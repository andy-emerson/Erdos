"""ringrun.py: Python side of ringfd.c (rings-test.md).

run(R, F, K, jobs, mode, B, N0=0, cap=100000, flags=None) with jobs = [(tag, alpha, table)], table a dict
class index mod pi^K -> digit (odd classes only); returns the output lines.
"""
import subprocess, os
import qring as Q

HERE = os.path.dirname(os.path.abspath(__file__))
ENGINE = os.path.join(HERE, 'ringfd')


def header(R, F, K, mode, B, N0, cap):
    A, c, g = R.hnf(K)
    if R.dim == 1: t, n = 0, 1
    else: t, n = R.t, R.n
    U = R.U if R.U else [(1, 0)]
    h = f"{t} {n} {R.pi[0]} {R.pi[1]} {A} {c} {g} {F} {int(B)} {cap} {mode} {int(N0)} {len(U)} " + \
        " ".join(f"{u[0]} {u[1]}" for u in U)
    return h


def run(R, F, K, jobs, mode, B, N0=0, cap=100000, flags=None):
    M = R.nclasses(K)
    inp = [header(R, F, K, mode, B, N0, cap)]
    if F == 'P': inp.append(" ".join(str(int(flags[i])) for i in range(M)))
    for tag, alpha, tab in jobs:
        row = []
        for i in range(M):
            d = tab.get(i, (0, 0))
            row.append(f"{d[0]} {d[1]}")
        inp.append(f"{tag} {alpha[0]} {alpha[1]} " + " ".join(row))
    p = subprocess.run(['nice', ENGINE], input="\n".join(inp) + "\n", capture_output=True, text=True)
    if p.returncode: raise RuntimeError(p.stderr)
    return [l for l in p.stdout.split("\n") if l.strip()]


def table_from_set(R, alpha, K, Dset):
    """digit table of the digit set D: on the odd class c, the element of D in the class of -alpha c"""
    byc = {}
    for d in Dset: byc.setdefault(R.idx(d, K), d)
    tab = {}
    for i in R.odd_classes(K):
        c = R.classrep(i, K)
        j = R.idx(R.neg(R.mul(alpha, c)), K)
        tab[i] = byc[j]
    return tab


def lift_table(R, tab, k, K):
    """a table at modulus pi^k as a table at pi^K (K >= k)"""
    out = {}
    for i in R.odd_classes(K):
        out[i] = tab[R.idx(R.classrep(i, K), k)]
    return out


def commutes(R, tab, K):
    for i, d in tab.items():
        c = R.classrep(i, K)
        for u in R.U:
            j = R.idx(R.mul(u, c), K)
            if tab[j] != R.mul(u, d): return False
    return True


def apply_S(R, alpha, tab, K, z, F='A'):
    w = R.add(R.mul(alpha, z), tab[R.idx(z, K)])
    if w == (0, 0): return (0, 0), None
    w, v = R.oddpart(w)
    return Q.rep(R, F, w), v
