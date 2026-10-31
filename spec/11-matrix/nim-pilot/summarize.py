"""summarize.py: the ratio tables of REPORT.md from results/{ir,mem,values}.csv (python3 summarize.py)."""
import csv
from pathlib import Path

R = Path(__file__).resolve().parent / "results"
ir = list(csv.DictReader(open(R / "ir.csv")))
mem = list(csv.DictReader(open(R / "mem.csv")))
val = list(csv.DictReader(open(R / "values.csv")))
V = {(r["op"], r["job"], r["row"]): r for r in ir}
M = {(r["op"], r["job"], r["row"]): r for r in mem}
rows = sorted({r["row"] for r in ir} - {"C"}, key=["Nim", "Nim-exc", "Nim-if", "Nim-O3"].index)

print("values:", sum(1 for r in val), "jobs;", "all identical to the reference and to C:",
      all(all(v == "True" for k, v in r.items() if k not in ("op", "job", "n", "cells")) for r in val),
      "cells:", sum(int(r["cells"]) for r in val))
print()
print("| op | n | job | C build Ir/input | C op Ir | " + " | ".join(f"{x} build | {x} op" for x in rows) + " |")
print("| --- " * (5 + 2 * len(rows)) + "|")
for r in val:
    op, job, n = r["op"], r["job"], r["n"]
    c = V[(op, job, "C")]
    cells = []
    for x in rows:
        v = V[(op, job, x)]
        cells += [f"{int(v['build_per_input']) / int(c['build_per_input']):.3f}",
                  f"{int(v['op_ir']) / int(c['op_ir']):.3f}"]
    print(f"| {op} | {n} | {job} | {int(c['build_per_input']):,} | {int(c['op_ir']):,} | " + " | ".join(cells) + " |")
print()
for x in rows:
    for op in ("add", "mul"):
        b = [int(V[(op, r["job"], x)]["build_per_input"]) / int(V[(op, r["job"], "C")]["build_per_input"])
             for r in val if r["op"] == op]
        o = [int(V[(op, r["job"], x)]["op_ir"]) / int(V[(op, r["job"], "C")]["op_ir"]) for r in val if r["op"] == op]
        print(f"{x} {op}: build {min(b):.3f}-{max(b):.3f}, op {min(o):.3f}-{max(o):.3f}")
print()
print("| op | n | job | build peak acct C / Nim | build peak heap C / Nim | op peak acct C / Nim | op peak heap C / Nim |")
print("| --- " * 7 + "|")
same = True
for r in val:
    op, job, n = r["op"], r["job"], r["n"]
    c, m = M[(op, job, "C")], M[(op, job, "Nim")]
    f = [(c[k], m[k]) for k in ("build_peak_acct", "build_peak_heap", "op_peak_acct", "op_peak_heap")]
    same &= all(a == b for a, b in f) and c["end_acct"] == m["end_acct"] and c["end_heap"] == m["end_heap"]
    print(f"| {op} | {n} | {job} | " + " | ".join(f"{int(a):,} / {int(b):,}" for a, b in f) + " |")
print("memory identical in every job:", same)
