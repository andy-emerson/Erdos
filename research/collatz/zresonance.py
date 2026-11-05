"""Heuristic count of cycles at the resonances m^n ~ 2^V (integer-census.md, Section 4.4).

For a symmetric rule at level k the odd 2-adic integers with valuation vector (v_0..v_{n-1})
form 2^((k-1)n) classes, on each of which S^n is affine and onto; as in the paper (Section 5.6)
the expected number of cycles up to sign with n odd steps and total valuation V is
    E(n, V) = 2^((k-1)n) * binom(V - kn + n - 1, n - 1) / (2 n |2^V - m^n|).
Modes:
  python3 zresonance.py scan logfile k mlo mhi     E(2, 2k+1) against the 2-cycles found
  python3 zresonance.py rule logfile k m n         E(n, V) for the V nearest n log2 m, against
                                                   the n-cycles found
"""
import sys, math

def load(fn):
    rows = {}
    for line in open(fn):
        if line.startswith('#'): continue
        h, c = line.split('|'); h = h.split()
        rows[(int(h[0]), int(h[1]))] = [t.split(':') for t in c.split()]
    return rows

def E(k, m, n, V):
    return 2**((k - 1) * n) * math.comb(V - k * n + n - 1, n - 1) / (2 * n * abs(2**V - m**n))

mode, fn = sys.argv[1], sys.argv[2]
rows = load(fn)
if mode == 'scan':
    k, lo, hi = map(int, sys.argv[3:6])
    for m in range(lo, hi + 1, 2):
        if (k, m) not in rows: continue
        found = sum(1 for t in rows[(k, m)] if t[1] == '2')
        print(f'k={k} m={m}  2-cycles found {found:3d}  E(2,{2 * k + 1}) = {E(k, m, 2, 2 * k + 1):6.1f}')
else:
    k, m, n = map(int, sys.argv[3:6])
    V = round(n * math.log2(m))
    found = sum(1 for t in rows[(k, m)] if t[1] == str(n))
    print(f'k={k} m={m} n={n} V={V}: m^n / 2^V = {m**n / 2**V:.6f}, {n}-cycles found {found}, '
          f'E = {E(k, m, n, V):.1f}, all cycles {len(rows[(k, m)])}')
