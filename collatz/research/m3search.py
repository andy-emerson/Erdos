"""Collatz maps on Z[i] at modulus (1+i)^3 with digit tables that need not commute with the units.
(collatz/research/modulus3-search.md)

usage:
  m3search.py prune maxnorm               candidate digits per class, and what the fixed-point lemma removes
  m3search.py run nsmall|nN B cap tag [red]    run m3search on every multiplier; candidates are the nsmall
                                          smallest elements of each class (e.g. 6) or all of norm <= N (e.g. n100)
"""
import sys, os, subprocess
import rules as R

HERE = os.path.dirname(os.path.abspath(__file__))
SCR = '/tmp/claude-0/-home-claude/6c273ef9-4bf4-5ea2-bd9e-566136af1776/scratchpad'
U = R.UNITS
PI = (1, 1)

def pipow(v):
    z = (1, 0)
    for _ in range(v): z = R.mul(z, PI)
    return z

def alphas():
    A = [(x, y) for x in range(-4, 5) for y in range(-4, 5) if (x + y) % 2 and 8 < x * x + y * y < 16]
    return sorted(A, key=R.key)

def cls(z):
    """index c with z = U[c] mod (1+i)^3, z odd."""
    c = [j for j, u in enumerate(U) if R.val(z[0] - u[0], z[1] - u[1]) >= 3]
    assert len(c) == 1
    return c[0]

def divides(a, b):
    """b / a in Z[i] or None."""
    n = R.norm(a)
    x, y = R.mul(b, (a[0], -a[1]))
    if x % n or y % n: return None
    return (x // n, y // n)

def odd(z): return (z[0] + z[1]) % 2 == 1

def candidates(alpha, maxnorm):
    """for each class c of z, the digits d with (1+i)^3 | alpha U[c] + d, sorted by (norm, re, im)."""
    box = int(maxnorm ** 0.5) + 1
    out = [[] for _ in range(4)]
    for x in range(-box, box + 1):
        for y in range(-box, box + 1):
            d = (x, y)
            if not odd(d) or R.norm(d) > maxnorm: continue
            for c, u in enumerate(U):
                if R.val(*R.add(R.mul(alpha, u), d)) >= 3: out[c].append(d)
    for o in out: o.sort(key=R.key)
    return out

def forbidden(alpha, d):
    """reason the digit d forces failure, or None.
    fixed: d = (pi^v - alpha) z with v >= 3, z odd, not a unit -> S(z) = z (Lemma 5.1, eps = 1).
    zero:  d = -alpha z with z odd -> alpha z + d = 0 and the orbit of z ends at 0."""
    z = divides(alpha, d)
    if z is not None and odd((-z[0], -z[1])): return ('zero', (-z[0], -z[1]))
    v = 3
    while True:
        e = R.add(pipow(v), (-alpha[0], -alpha[1]))
        if R.norm(e) > R.norm(d): break
        z = divides(e, d)
        if z is not None and odd(z) and R.norm(z) > 1: return ('fixed', z, v)
        v += 1
    return None

def prune(maxnorm):
    for alpha in alphas():
        C = candidates(alpha, maxnorm)
        line = []
        for c in range(4):
            keep = [d for d in C[c] if forbidden(alpha, d) is None]
            line.append(f"{len(keep)}/{len(C[c])}")
        tot = 1; tot0 = 1
        for c in range(4):
            tot0 *= len(C[c]); tot *= len([d for d in C[c] if forbidden(alpha, d) is None])
        print(f"alpha={alpha} N={R.norm(alpha)} kept/candidates per class {line} tables {tot0} -> {tot}")

def run(spec, B, cap, tag, red=0):
    exe = os.path.join(HERE, 'm3search')
    for alpha in alphas():
        if spec.startswith('n'):
            C = candidates(alpha, int(spec[1:]))
        else:
            C = [c[:int(spec)] for c in candidates(alpha, 200)]
        tot0 = 1
        for c in C: tot0 *= len(c)
        reasons = {}
        K = []
        for c in range(4):
            k = []
            for d in C[c]:
                r = forbidden(alpha, d)
                if r is None: k.append(d)
                else: reasons[r[0]] = reasons.get(r[0], 0) + 1
            K.append(k)
        path = f"{SCR}/m3_{tag}_{alpha[0]}_{alpha[1]}.in"
        with open(path, 'w') as f:
            f.write(f"{alpha[0]} {alpha[1]} {B} {cap} {red}\n")
            for k in K: f.write(f"{len(k)} " + " ".join(f"{d[0]} {d[1]}" for d in k) + "\n")
        print(f"# alpha={alpha} N={R.norm(alpha)} candidates per class {[len(c) for c in C]} tables {tot0}; "
              f"removed digits {reasons}; kept per class {[len(k) for k in K]}", flush=True)
        if not spec.startswith('n'): print("#   candidates " + " | ".join(str(c) for c in C), flush=True)
        out = subprocess.run([exe, path], capture_output=True, text=True, check=True).stdout
        sys.stdout.write(out); sys.stdout.flush()

if __name__ == '__main__':
    a = sys.argv[1:]
    if a[0] == 'prune': prune(int(a[1]))
    elif a[0] == 'run': run(a[1], int(a[2]), int(a[3]), a[4], int(a[5]) if len(a) > 5 else 0)
