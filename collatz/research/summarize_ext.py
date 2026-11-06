"""Summary of the checkpointed verification logs of cycle-bounds.md, Section 7: for each log the
covered range (checked to be a contiguous union of chunks), starts, failures and cycles reported,
the longest stopping time (steps of T; odd steps for the cone) and largest excursion (log2 of the
norm ratio; bits of |z| for the cone) with their starts, and the CPU time.
For the 11 +- 2i maps the FAIL lines (escapes past 2^110) are matched against the PASS lines of
the unbounded follow-up gstop_big.py (logs/ext2/*.big).

usage: python3 summarize_ext.py logs/alpha5p2i_k4_ext.stop logs/ext2/*.stop > logs/ext_summary.txt
"""
import os
import re
import sys

R = re.compile(r"range \((\d+),(\d+)\] starts (\d+) fails (\d+) cycles (\d+) maxstop (\d+) at (-?\d+) (-?\d+) "
               r"meanstop ([\d.]+) maxexc_(?:log2|bits) ([\d.]+)(?: at (-?\d+) (-?\d+))? cpu ([\d.]+)")


def main():
    for path in sys.argv[1:]:
        rows, fails = [], []
        for line in open(path):
            m = R.match(line)
            if m:
                rows.append(m.groups())
            elif line.startswith("FAIL"):
                fails.append(tuple(line.split()[1:3]))
            elif line.startswith("cycle-min"):
                fails.append(("cycle",) + tuple(line.split()[1:3]))
        rows.sort(key=lambda g: int(g[0]))
        contiguous = all(rows[i][1] == rows[i + 1][0] for i in range(len(rows) - 1))
        st = sum(int(g[2]) for g in rows)
        nf = sum(int(g[3]) for g in rows)
        nc = sum(int(g[4]) for g in rows)
        ms = max(rows, key=lambda g: int(g[5]))
        me = max(rows, key=lambda g: float(g[9]))
        cpu = sum(float(g[12]) for g in rows)
        resolved = ""
        big = path[:-5] + ".big"
        if fails and os.path.exists(big):
            ok = {tuple(l.split()[1:3]) for l in open(big) if l.startswith("PASS")}
            resolved = "; FAIL starts resolved by gstop_big.py: %d of %d" % (len(set(fails) & ok), len(set(fails)))
        print("%s: (%s, %s] %d chunks, contiguous %s, starts %d, fails %d, cycles %d, longest %s at (%s, %s), "
              "excursion %s at (%s, %s), cpu %.0f s%s" % (
                  os.path.basename(path), rows[0][0], rows[-1][1], len(rows), contiguous, st, nf, nc,
                  ms[5], ms[6], ms[7], me[9], me[10] or '-', me[11] or '-', cpu, resolved))


if __name__ == "__main__":
    main()
