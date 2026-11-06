"""Toy test: are the dominant cycle lengths the best approximations of alpha^n by a unit times (1+i)^V?
(convergents.md)

For a Gaussian rule (alpha, level k) and each n, let
    rel(n, V, eps) = |1 - alpha^n / (eps (1+i)^V)| = |eps (1+i)^V - alpha^n| / 2^(V/2),
    best(n) = min over V and the four units of rel(n, V, eps).
n is a record (best simultaneous approximation) if best(n) < best(m) for every m < n.  For real alpha only
even V can be close, and the records are the denominators of the convergents of log2|alpha|.
E(n, V, eps) is the heuristic count of families.md, Section 3, computed in floating point.

usage:
  convergents.py gauss NMAX FILE...      FILE = families.py output (.txt); one block per rule
  convergents.py int MODE M B NMAX       integer analogue on odd x with |x| <= B:
                                         MODE sym: x -> (m x + d)/2^v, d = +-1 with 4 | m x + d (level 2, symmetric)
                                         MODE plus: x -> (m x + 1)/2^v (level 1)
  convergents.py cf X                    continued fraction and convergents of X (a float expression)"""
import sys, math, re
from fractions import Fraction

UNITS = [(1, 0), (0, 1), (-1, 0), (0, -1)]
L2 = math.log(2)

def logC(a, b):
    return math.lgamma(a + 1) - math.lgamma(b + 1) - math.lgamma(a - b + 1)

def rel_g(alpha, n, V, eps):
    la = 0.5 * math.log(alpha[0]**2 + alpha[1]**2)
    lr = n * la - V * L2 / 2
    th = n * math.atan2(alpha[1], alpha[0]) - V * math.pi / 4 - math.atan2(eps[1], eps[0])
    return abs(1 - math.exp(lr) * complex(math.cos(th), math.sin(th)))

def E_g(k, n, V, rel):
    e = V - k * n
    if e < 0 or rel == 0: return 0.0
    lg = (k - 1) * n * L2 + logC(e + n - 1, n - 1) - math.log(4 * n) - V * L2 - 2 * math.log(rel)
    return math.exp(lg)

def best_table(alpha, k, nmax):
    """for each n: (best rel, V, eps, E at that (V, eps), total E over V, eps)"""
    l2N = math.log2(alpha[0]**2 + alpha[1]**2)
    out = {}
    for n in range(1, nmax + 1):
        c = n * l2N; b = None; Et = 0.0
        for V in range(max(k * n, math.floor(c) - 3), math.ceil(c) + 4):
            for eps in UNITS:
                r = rel_g(alpha, n, V, eps); E = E_g(k, n, V, r); Et += E
                if b is None or r < b[0]: b = (r, V, eps, E)
        out[n] = b + (Et,)
    return out

def records(best):
    rec, m = [], float('inf')
    for n in sorted(best):
        if best[n][0] < m: rec.append(n); m = best[n][0]
    return rec

def cf(x, terms=12):
    a, p, q, conv = [], [1, 0], [0, 1], []
    y = x
    for _ in range(terms):
        ai = math.floor(y); a.append(ai)
        p.append(ai * p[-1] + p[-2]); q.append(ai * q[-1] + q[-2]); conv.append((p[-1], q[-1]))
        if y - ai < 1e-12: break
        y = 1 / (y - ai)
    return a, conv

def parse_fam(path):
    k = alpha = None; cyc = []
    for line in open(path):
        if line.startswith('# k='):
            m = re.match(r'# k=(\d+) alpha=\((-?\d+), (-?\d+)\)', line)
            k = int(m.group(1)); alpha = (int(m.group(2)), int(m.group(3)))
        elif not line.startswith('#') and line.strip():
            f = line.split()
            cyc.append((int(f[1]), int(f[2]), tuple(map(int, f[3].split(',')))))
    return k, alpha, cyc

def gauss(nmax, files):
    print(f"# per rule: classes (n, V, eps) with count, rel, E, record?; then records n <= 300 with best rel, E and count")
    allrows = []
    for path in files:
        k, alpha, cyc = parse_fam(path)
        best = best_table(alpha, k, nmax); rec = set(records(best))
        cls = {}
        for c in cyc: cls[c] = cls.get(c, 0) + 1
        byn = {}
        for (n, V, eps), c in cls.items(): byn[n] = byn.get(n, 0) + c
        tag = path.rsplit('/', 1)[-1].rsplit('.', 1)[0]
        print(f"\n## {tag} k={k} alpha={alpha} cycles={len(cyc)}  log2N={math.log2(alpha[0]**2+alpha[1]**2):.6f}"
              f" arg/(pi/4)={math.atan2(alpha[1], alpha[0])/(math.pi/4):.6f}")
        rows = []
        for (n, V, eps), c in sorted(cls.items(), key=lambda t: -t[1]):
            r = rel_g(alpha, n, V, eps)
            isbest = (V, eps) == best[n][1:3] if n in best else None
            prior = min((best[m][0] for m in best if m < n), default=float('inf'))
            print(f"  class n={n} V={V} eps={eps} count={c} rel={r:.3g} E={E_g(k, n, V, r):.3g} "
                  f"record={'Y' if n in rec else 'n'} bestV={'Y' if isbest else 'n'} rel/prior_best={r/prior:.3g}")
            rows.append((n, V, eps, c, r, n in rec))
        print("  records:", ' '.join(f"{n}({best[n][0]:.2g},E={best[n][4]:.2g},obs={byn.get(n, 0)})" for n in sorted(rec) if n <= 300))
        # dominant: largest class with n >= 2
        nontriv = [x for x in rows if x[0] >= 2]
        dom = max(nontriv, key=lambda x: x[3]) if nontriv else None
        Ebest = max(range(2, nmax + 1), key=lambda n: best[n][4])
        cnt_rec = sum(x[3] for x in nontriv if x[5]); cnt_all = sum(x[3] for x in nontriv)
        allrows.append((tag, k, alpha, len(cyc), dom, dom[5] if dom else None, cnt_rec, cnt_all, Ebest, Ebest in rec,
                        sorted(n for n in rec if 2 <= n <= 300 and best[n][4] >= 1 and byn.get(n, 0) == 0)))
    print("\n# summary: tag k alpha cycles | dominant class (n,V,eps,count) record? | cycles (n>=2) at record n / all | "
          "n maximising total E, record? | records n<=300 with E>=1 and no cycle")
    for t in allrows:
        tag, k, alpha, nc, dom, dr, cr, ca, Eb, Ebr, miss = t
        d = f"({dom[0]},{dom[1]},{dom[2][0]},{dom[2][1]}) x{dom[3]}" if dom else '-'
        print(f"{tag} k={k} {alpha} {nc} | {d} {'Y' if dr else 'n'} | {cr}/{ca} | {Eb} {'Y' if Ebr else 'n'} | {miss}")

# ---------------- integer analogues ----------------

def step_int(mode, m, x):
    if mode == 'sym':
        d = 1 if (m * x + 1) % 4 == 0 else -1
    else:
        d = 1
    y = m * x + d; v = 0
    while y % 2 == 0: y //= 2; v += 1
    return y, v

def int_cycles(mode, m, B, cap=5000, esc=2**80):
    found = {}; done = set()
    for x0 in range(-B | 1, B + 1, 2):
        if x0 in done: continue
        path, seen, x = [], {}, x0
        for _ in range(cap):
            if x in done or abs(x) > esc: break
            if x in seen:
                cyc = path[seen[x]:]
                mn = min(cyc, key=abs) if 0 not in cyc else 0
                if mn not in found:
                    z, V = mn, 0
                    n = 0
                    while True:
                        z, v = step_int(mode, m, z); V += v; n += 1
                        if z == mn: break
                    found[mn] = (n, V)
                break
            seen[x] = len(path); path.append(x)
            x, _ = step_int(mode, m, x)
        done.update(path)
    return found

def integer(mode, m, B, nmax):
    l2m = math.log2(m)
    # best |1 - m^n / 2^V| over V; one-sided records too
    def rel(n, V): return abs(1 - 2 ** (n * l2m - V))
    rec, rec_up, rec_dn = [], [], []
    b = bu = bd = float('inf')
    for n in range(1, nmax + 1):
        c = n * l2m; Vu, Vd = math.ceil(c), math.floor(c)   # Vu: 2^V > m^n; Vd: 2^V < m^n
        ru, rd = rel(n, Vu), rel(n, Vd)
        if min(ru, rd) < b: rec.append(n); b = min(ru, rd)
        if ru < bu: rec_up.append(n); bu = ru
        if rd < bd: rec_dn.append(n); bd = rd
    a, conv = cf(l2m)
    cyc = int_cycles(mode, m, B)
    print(f"\n## {mode} m={m} B={B}: log2 m = {l2m:.6f} = [{a[0]}; {', '.join(map(str, a[1:]))}]")
    print("  convergents:", ' '.join(f"{p}/{q}" for p, q in conv if q <= nmax))
    print("  records two-sided:", [n for n in rec if n <= 300])
    print("  records with 2^V > m^n (x > 0 side for plus):", [n for n in rec_up if n <= 300])
    print("  records with 2^V < m^n (x < 0 side for plus):", [n for n in rec_dn if n <= 300])
    byn = {}
    for mn, (n, V) in sorted(found_sort(cyc)):
        byn.setdefault((n, V), []).append(mn)
    for (n, V), mins in sorted(byn.items()):
        side = 'up' if 2 ** (V - n * l2m) > 1 else 'dn'
        flags = ('rec' if n in rec else '') + (' rec_up' if n in rec_up else '') + (' rec_dn' if n in rec_dn else '')
        print(f"  n={n} V={V} rel={rel(n, V):.3g} side={side} cycles={len(mins)} mins={mins[:8]} {flags}")

def found_sort(cyc):
    return [(mn, nv) for mn, nv in sorted(cyc.items(), key=lambda t: (abs(t[0]), t[0]))]

def modonly(k, alpha, nmax):
    """records of the modulus condition alone, min over V of |n log2 N(alpha) - V| (convergents of
    log2 N(alpha)), against the records of the full condition, with the angle error at each."""
    l2N = math.log2(alpha[0]**2 + alpha[1]**2); a4 = math.atan2(alpha[1], alpha[0]) / (math.pi / 4)
    best = best_table(alpha, k, nmax); full = records(best)
    m, mod = float('inf'), []
    for n in range(1, nmax + 1):
        e = abs(n * l2N - round(n * l2N))
        if e < m: mod.append(n); m = e
    print(f"## k={k} alpha={alpha}: log2N = {l2N:.6f} = {cf(l2N)[0]}, arg/(pi/4) = {a4:.6f}")
    print("  n | modulus error |n log2N - V| | angle error (in units of pi/4, mod 2) | best rel | E(best) | full record?")
    for n in sorted(set(mod) | set(full)):
        V = round(n * l2N); ang = (n * a4 - V) % 2; ang = min(ang, 2 - ang)
        print(f"  {n} | {abs(n*l2N - V):.4f} | {ang:.4f} | {best[n][0]:.3g} | {best[n][3]:.3g} | "
              f"{'Y' if n in full else 'n'}{' (modulus record)' if n in mod else ''}")

if __name__ == '__main__':
    a = sys.argv[1:]
    if a[0] == 'modonly': modonly(int(a[1]), tuple(map(int, a[2].split(','))), int(a[3]))
    elif a[0] == 'gauss': gauss(int(a[1]), a[2:])
    elif a[0] == 'int': integer(a[1], int(a[2]), int(a[3]), int(a[4]))
    elif a[0] == 'cf':
        x = eval(a[1], {'log2': math.log2, 'math': math})
        print(cf(x, 15))
