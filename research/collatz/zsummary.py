"""Summarize the integer census logs (integer-census.md, Section 3).

Reads logs/zcensus_<set>_*.txt lines "k m set B ncyc capped escaped maxlog2 nbig | cycles" and
prints, per level: rules, single-cycle rules (one cycle up to sign and no unresolved start),
their multipliers, the drift ln m - (k+1) ln 2 of each, the cycle-count distribution of the
others, and the single-cycle counts by position of m in the window (tenths of (2^k, 2^(k+1))).
Usage: python3 zsummary.py logfile [logfile ...]
"""
import sys, math
from collections import defaultdict, Counter

rows = {}
for fn in sys.argv[1:]:
    for line in open(fn):
        if line.startswith('#') or not line.strip(): continue
        head = line.split('|')[0].split()
        k, m, ncyc, capped, esc = int(head[0]), int(head[1]), int(head[4]), int(head[5]), int(head[6])
        rows[(k, m)] = (ncyc, capped, esc, int(head[3]), line.split('|')[1].split())
levels = sorted({k for k, m in rows})
bins_all = defaultdict(lambda: [0, 0])
for k in levels:
    ms = sorted(m for kk, m in rows if kk == k)
    single = [m for m in ms if rows[(k, m)][0] == 1 and rows[(k, m)][1] == 0 and rows[(k, m)][2] == 0]
    unres = [m for m in ms if rows[(k, m)][1] or rows[(k, m)][2]]
    dist = Counter(rows[(k, m)][0] for m in ms if m not in single)
    drifts = [math.log(m) - (k + 1) * math.log(2) for m in single]
    print(f'level {k}: {len(ms)} rules, {len(single)} single-cycle; smallest m={ms[0]} cycles {rows[(k, ms[0])][0]}, '
          f'largest m={ms[-1]} cycles {rows[(k, ms[-1])][0]}; rules with unresolved starts: {len(unres)}')
    if single:
        print(f'  single-cycle m: {single}')
        print(f'  first single-cycle m = {single[0]} (rank {ms.index(single[0]) + 1} of {len(ms)}), '
              f'm/2^k in [{min(single) / 2**k:.3f}, {max(single) / 2**k:.3f}], drift in [{min(drifts):+.4f}, {max(drifts):+.4f}]')
    print(f'  cycle counts of the others (count: rules): {sorted(dist.items())}')
    b = [0] * 10; t = [0] * 10
    for m in ms:
        i = int(10 * (m / 2**k - 1)); t[i] += 1; bins_all[i][1] += 1
        if m in single: b[i] += 1; bins_all[i][0] += 1
    print('  single-cycle by tenth of window:', ' '.join(f'{x}/{y}' for x, y in zip(b, t)))
print('all levels, single-cycle by tenth of window (m/2^k - 1 in [i/10, (i+1)/10)):',
      ' '.join(f'{bins_all[i][0]}/{bins_all[i][1]}' for i in range(10)))
