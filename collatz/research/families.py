"""Full cycle data for one unit-equivariant digit rule (for the cycle-family study, families.md).

usage: families.py k ax,ay g1x,g1y;g2x,g2y;... B OUT [--gcz PATH] [--cap N]
  The digit set is the unit multiples of the listed generators (as in sweep_eq2.py).
  Runs gcz non-quiet on every z with N(z) <= B, groups the raw cycles up to units by exact
  iteration (exact.make_S, digit chosen from the congruence), and writes
    OUT.txt   one line per cycle up to units:
              canon odd V unit minN maxN basin vals
              canon = least-norm element of the cycle, first-quadrant associate (x > 0, y >= 0);
              odd = number of odd steps; V = total (1+i)-valuation over one projective period;
              unit = eps with S^odd(canon) = eps * canon; vals = valuation of each odd step from canon.
    OUT.json  the same plus the list of odd elements along the cycle from canon (actual S-orbit,
              not normalised), for the structure analysis.
  For more than 4096 raw cycles gcz needs a larger MAXC; build a copy with
    sed 's/#define MAXC 4096/#define MAXC 65536/' gcz.c > gczbig.c; gcc -O2 -o gczbig gczbig.c -lm
"""
import sys, json, itertools, rules as R, exact as E

def fq(z):
    """first-quadrant associate: x > 0, y >= 0"""
    for u in R.UNITS:
        w = R.mul(u, z)
        if w[0] > 0 and w[1] >= 0: return w
    raise ValueError(z)

def cycle_data(S, z0):
    """projective cycle through z0: returns dict with canon, elements from canon, vals, unit."""
    # find the cycle in associate classes
    seq = []; z = fq(z0)
    while True:
        seq.append(z); w, _ = S(z); z = fq(w)
        if z == seq[0]: break
        assert len(seq) < 10**6
    c = min(seq, key=R.key)
    c = fq(c)
    els = []; vals = []; z = c
    while True:
        els.append(z); w, v = S(z); vals.append(v); z = w
        if fq(z) == c: break
    unit = [u for u in R.UNITS if R.mul(u, c) == z][0]
    ns = [R.norm(e) for e in els]
    return dict(canon=c, odd=len(els), V=sum(vals), unit=unit, minN=min(ns), maxN=max(ns), vals=vals, els=els)

def main():
    a = sys.argv[1:]
    gczp = './gcz'; cap = 50000
    if '--gcz' in a: i = a.index('--gcz'); gczp = a[i+1]; del a[i:i+2]
    if '--cap' in a: i = a.index('--cap'); cap = int(a[i+1]); del a[i:i+2]
    k = int(a[0]); al = tuple(map(int, a[1].split(','))); gens = [tuple(map(int, g.split(','))) for g in a[2].split(';')]
    B = int(a[3]); out = a[4]
    M = 4 if k <= 4 else 8 if k <= 6 else 16
    D = [R.mul(u, g) for g in gens for u in R.UNITS]
    rows = R.table(R.digit_rule(al, D, k), M=M)
    import subprocess, os
    path = out + '.rule'; R.write(rows, path)
    res = subprocess.run([gczp, path, str(B), str(cap)], capture_output=True, text=True,
                         cwd=os.path.dirname(os.path.abspath(__file__)))
    os.remove(path)
    if res.returncode: raise RuntimeError(res.stderr)
    h, stats, cyc = R.parse(res.stdout)
    S = E.make_S(al, D, k)
    groups = {}
    for c in cyc:
        if c['min'] == (0, 0): continue
        # cheap pre-check: skip if an associate of this raw min is already in a known cycle
        m = fq(c['min'])
        hit = None
        for key, d in groups.items():
            if m in d['set']: hit = key; break
        if hit is None:
            d = cycle_data(S, c['min']); d['set'] = set(d['els']) | set(fq(e) for e in d['els']); d['basin'] = 0
            groups[d['canon']] = d; hit = d['canon']
        groups[hit]['basin'] += c['count']
    L = sorted(groups.values(), key=lambda d: (d['minN'], d['canon']))
    with open(out + '.txt', 'w') as f:
        f.write(f"# k={k} alpha={al} gens={gens} B={B} cap={cap}\n# gcz: {res.stdout.splitlines()[0]}\n# gcz: {stats}\n")
        f.write(f"# cycles up to units: {len(L)}\n# canon odd V unit minN maxN basin vals\n")
        for d in L:
            f.write(f"{d['canon'][0]},{d['canon'][1]} {d['odd']} {d['V']} {d['unit'][0]},{d['unit'][1]} {d['minN']} {d['maxN']} {d['basin']} {''.join(str(v) if v < 10 else '('+str(v)+')' for v in d['vals'])}\n")
    with open(out + '.json', 'w') as f:
        json.dump(dict(k=k, alpha=al, gens=gens, B=B, header=res.stdout.splitlines()[:2],
                       cycles=[{x: d[x] for x in ('canon', 'odd', 'V', 'unit', 'minN', 'maxN', 'basin', 'vals', 'els')} for d in L]), f)
    print(f"k={k} alpha={al} B={B}: {len(L)} cycles; {res.stdout.splitlines()[0]}", flush=True)

if __name__ == '__main__':
    main()
