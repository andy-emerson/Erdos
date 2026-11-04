"""Merge gg.c outputs over disjoint norm ranges: totals, fails, global hardest cases, histograms.
usage: python3 merge.py run_1e9.txt chunks/run_*e9.txt"""
import sys, re
from collections import defaultdict
tot = defaultdict(int); fails = defaultdict(int); ns = defaultdict(int); top = defaultdict(list); hist = defaultdict(lambda: defaultdict(int))
ranges = []
for fn in sys.argv[1:]:
    t = open(fn).read(); cur = None
    ranges.append(tuple(map(int, re.search(r"NMIN (\d+) NMAX (\d+)", t).groups())))
    for line in t.splitlines():
        m = re.match(r"variant (\w): targets (\d+).*fails (\d+), nonstrict_only (\d+)", line)
        if m:
            cur = m.group(1); tot[cur] += int(m.group(2)); fails[cur] += int(m.group(3)); ns[cur] += int(m.group(4)); continue
        m = re.search(r"z=(-?\d+)\+(-?\d+)i N=(\d+)  value (\d+)  p1=(-?\d+)\+(-?\d+)i", line)
        if m: top[cur].append(tuple(map(int, m.groups())))
        if "hist" in line:
            for k, v in re.findall(r"\[(\d+)\](\d+)", line): hist[cur][int(k)] += int(v)
ranges.sort()
print("ranges", ranges[0][0], "to", ranges[-1][1], "contiguous:", all(ranges[i][1] + 1 >= ranges[i + 1][0] for i in range(len(ranges) - 1)))
for v in sorted(tot):
    print(f"variant {v}: targets {tot[v]}, fails {fails[v]}, nonstrict_only {ns[v]}")
    for a, b, n, val, x, y in sorted(top[v], key=lambda r: (-r[3], r[2]))[:10]:
        print(f"   z={a}+{b}i N={n} value {val} p1={x}{y:+d}i")
    print("   hist log2:", dict(sorted(hist[v].items())))
