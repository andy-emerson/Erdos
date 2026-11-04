"""Joseph-type maps on Z[i]: z/2 on 2Z[i]; (alpha z + beta_c)/2 on the three other classes c mod 2."""
import itertools, rules as R
def jtable(alpha, betas):  # betas: dict class(a%2,b%2) -> beta
    rows = []
    for a in range(4):
        for b in range(4):
            c = (a % 2, b % 2)
            if c == (0, 0): rows.append((a, b, (1, 0), (0, 0), (2, 0)))
            else: rows.append((a, b, alpha, betas[c], (2, 0)))
    return rows
if __name__ == '__main__':
    print('Joseph alpha=3:', R.run(jtable((3, 0), {(1, 0): (1, 0), (0, 1): (0, 1), (1, 1): (1, 1)}), 10**5, cap=20000).split('\n')[:4])
    small = [(x, y) for x in range(-2, 3) for y in range(-2, 3) if x*x + y*y <= 2]
    for al in [(2, 1), (1, 2), (2, -1), (1, -2)]:
        cands = {}
        for c in [(1, 0), (0, 1), (1, 1)]:
            ac = R.mul(al, c)
            cands[c] = [d for d in small if (ac[0] + d[0]) % 2 == 0 and (ac[1] + d[1]) % 2 == 0]
        res = []
        for ch in itertools.product(*cands.values()):
            h, st, cyc = R.parse(R.run(jtable(al, dict(zip(cands, ch))), 10**5, cap=20000))
            nz = [c for c in cyc if c['min'] != (0, 0)]
            res.append((h['unresolved'], len(nz)))
        print('alpha', al, 'maps', len(res), 'all-converge', sum(1 for r in res if r[0] == 0),
              'single-cycle', sum(1 for r in res if r == (0, 1)), 'cycle counts', sorted(r[1] for r in res if r[0] == 0)[:20])
