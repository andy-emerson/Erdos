"""realrun.py: Python side of realfd.c for the rings of realq.py (real-quadratic.md).

run(R, F, K, jobs, mode, B, N0=0, cap=100000) with jobs = [(tag, alpha, table)], F = 'E' (cone, maps on R/U) or
'A' (literal, on R); modes as in ringfd.c (0 classification on F, 1 drop below start, 2 literal with trivial cycles).
"""
import subprocess, os

HERE = os.path.dirname(os.path.abspath(__file__))
ENGINE = os.path.join(HERE, 'realfd')


def run(R, F, K, jobs, mode, B, N0=0, cap=100000):
    A, c, g = R.hnf(K)
    U = [(1, 0), R.eps]
    h = f"{R.t} {R.n} {R.pi[0]} {R.pi[1]} {A} {c} {g} {F} {int(B)} {cap} {mode} {int(N0)} {len(U)} " + \
        " ".join(f"{u[0]} {u[1]}" for u in U)
    M = R.nclasses(K)
    inp = [h]
    for tag, alpha, tab in jobs:
        row = []
        for i in range(M):
            d = tab.get(i, (0, 0))
            row.append(f"{d[0]} {d[1]}")
        inp.append(f"{tag} {alpha[0]} {alpha[1]} " + " ".join(row))
    p = subprocess.run(['nice', ENGINE], input="\n".join(inp) + "\n", capture_output=True, text=True)
    if p.returncode: raise RuntimeError(p.stderr)
    return [l for l in p.stdout.split("\n") if l.strip()]
