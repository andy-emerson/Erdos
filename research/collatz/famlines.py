"""Position of cycle elements relative to the lines x = +-1, y = +-1, x - y = +-1, x + y = +-1 (families.md, Section 6).
For each cycle: the share of its odd elements lying exactly on one of these lines, and the least
absolute distance (min of |x|, |y|, |x-y|, |x+y|, which is 1 on the lines).
usage: famlines.py OUT.json [...]"""
import sys, json, math
from collections import Counter
for f in sys.argv[1:]:
    d = json.load(open(f)); C = [c for c in d['cycles'] if c['odd'] >= 2]
    on_tot = 0; el_tot = 0; per = Counter(); share = []
    for c in C:
        on = sum(1 for x, y in c['els'] if min(abs(x), abs(y), abs(x - y), abs(x + y)) == 1)
        on_tot += on; el_tot += len(c['els']); share.append(on / len(c['els']))
        per[(c['odd'], 'all on' if on == len(c['els']) else 'some on' if on else 'none on')] += 1
    # reference: share of odd z with N(z) in [10^4, 10^8] on the lines is about 8/(pi*|z|) ... report empirical small-norm baseline
    print(f"{f.split('/')[-1]}: {len(C)} cycles (n >= 2), {el_tot} elements, {on_tot} on the lines ({on_tot/el_tot:.4f});"
          f" cycles by (n, on-line status): {sorted(per.items())[:12]}")
