"""Follow the starts that zcz reports as escaping (|x| > 2^100) in exact big-integer arithmetic.

For a rule (k, m, sym|pos), rerun `zcz census` with ZCZ_SHOWESC=1 to get up to five escaping
starts, then iterate S from each with Brent's method until the orbit closes a cycle or the step cap is
reached. Prints, per start, log2 of the peak and the cycle reached (or the state at the cap).
Usage: python3 zesc.py k m [sym|pos] [cap]
"""
import subprocess, sys, os, math

here = os.path.dirname(os.path.abspath(__file__))
k, m = int(sys.argv[1]), int(sys.argv[2])
dset = sys.argv[3] if len(sys.argv) > 3 else 'sym'
cap = int(sys.argv[4]) if len(sys.argv) > 4 else 2 * 10**6
q, h = 2**k, 2**(k - 1)

def S(x):
    r = (-m * x) % q
    if dset == 'sym' and r > h: r -= q
    y = m * x + r
    return y >> ((y & -y).bit_length() - 1)

def cycle_key(x):
    seen = set()
    while x not in seen: seen.add(x); x = S(x)
    c, y = [x], S(x)
    while y != x: c.append(y); y = S(y)
    return (min(abs(t) for t in c) if dset == 'sym' else min(c)), len(c)

env = dict(os.environ, ZCZ_SHOWESC='1')
r = subprocess.run(['nice', os.path.join(here, 'zcz'), 'census', dset, str(k), str(m), '1000000'],
                   capture_output=True, text=True, env=env, check=True)
starts = [int(l.split()[1]) for l in r.stderr.split('\n') if l.startswith('escape')]
print(f'rule k={k} m={m} {dset}: drift {math.log(m) - (k + 1) * math.log(2):+.5f} nats per odd step; '
      f'census line: {" ".join(r.stdout.split()[:8])}')
for s in starts:
    # Brent's method on the full orbit, exact arithmetic
    tort, hare, power, lam, n, peak, found = s, S(s), 1, 1, 1, 0, False
    while n < cap:
        if tort == hare: found = True; break
        peak = max(peak, abs(hare).bit_length())
        if power == lam: tort, power, lam = hare, power * 2, 0
        hare = S(hare); lam += 1; n += 1
    if found:
        key, L = cycle_key(hare)
        print(f'  start {s}: peak 2^{peak}, reaches cycle {key} (odd length {L}) within {n} odd steps')
    else:
        print(f'  start {s}: no cycle within {n} odd steps, now at 2^{abs(hare).bit_length()}, peak 2^{peak}')
