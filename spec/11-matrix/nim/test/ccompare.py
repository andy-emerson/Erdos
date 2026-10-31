"""The Nim build against the C build (README.md, "Against C"), on m3's workloads (test/build/m3/work-<op>), where no value
overflows: Nim Control (bench-NCTL-*) against C's TC with the m4 gcd (tcm.c, test/build/m4/m4/bench-TC-*), and Nim
Packed (bench-NPK-*) against mh4.c (bench-MHV4-*). Nothing timed: the counting build's count, the test build's gate,
the memory build's mem.

    python3 ccompare.py [--ops add,mul,sop,pos] [--out FILE]

Compared, and every difference classified as one the spec asks for or reported:
  - the read-outs (and the gate's intermediates): byte-equal, and equal to m3's exact references;
  - the counters (ctr/xctr), line for line, after removing from C's lines what the spec does not ask of these builds
    (the known-answer counters k<n>_*, kq*, zero_results) and, in each multiplication step, the n^2 first additions
    0/1 + p of C's old summation (case add_zero_x, 8 comparisons each), which §17.6 removes;
  - the per-operation record: C's, with the record of each cell's first addition removed, against Nim's;
  - Packed's stor, pack and opc lines: equal;
  - the memory build's accounted figures: equal, except each mul step's peak, which §17.8 raises by the row buffer
    (n x 24 bytes) when the peak falls while it exists; heap figures reported.
"""
import argparse
import json
import struct
import subprocess
import sys
import tempfile
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
NIM = HERE.parent / "build" / "bin"
CB = HERE.parents[1] / "test" / "build" / "m4" / "m4"
WORK = HERE.parents[1] / "test" / "build" / "m3"
sys.path.insert(0, str(HERE.parents[1] / "test"))
import m2_common as C  # noqa: E402

PAIRS = {"NCTL": "TC", "NPK": "MHV4"}
C_ONLY = ("_k", "kq", "zero_results")   # xctr fields C prints that these builds do not keep


def run(exe, args):
    p = subprocess.run([str(exe)] + [str(a) for a in args], capture_output=True, text=True)
    return p.returncode, p.stdout


def ctr_lines(out):
    keep = []
    for ln in out.splitlines():
        if not ln.startswith(("ctr ", "xctr ")):
            continue
        f = ln.split()[4]
        if ln.startswith("xctr ") and (f.startswith("k") and f[1:2].isdigit() or f.startswith("kq") or f == "zero_results"):
            continue
        keep.append(ln)
    return keep


def records(path):
    b = Path(path).read_bytes()
    return [struct.unpack_from("<6h", b, 12 * i) for i in range(len(b) // 12)]


def strip_first_additions(rec, op, n):
    """C's record without each product cell's first addition (0/1 + p, case add_zero_x): C makes, per cell, n pairs
    (product, addition); §17.6's summation makes the product, then n - 1 pairs"""
    out, i = [], 0
    for kind, *_ in C.SET_STEPS[op]:
        if kind == "add":
            out += rec[i:i + n * n]
            i += n * n
        else:
            for _ in range(n * n):
                cell = rec[i:i + 2 * n]
                i += 2 * n
                assert cell[1][0] == 27, cell[1]          # add_zero_x
                out += [cell[0]] + cell[2:]
    assert i == len(rec)
    return out


def compare(op, job, wl, n):
    bad, mem = [], {}
    q = n * n
    steps = C.SET_STEPS[op]
    nmul = sum(1 for s in steps if s[0] == "mul")
    with tempfile.TemporaryDirectory() as tmp:
        for nim, c in PAIRS.items():
            for build, mode in (("counting", "count"), ("test", "gate"), ("memory", "mem")):
                outs = {}
                for side, exe in (("nim", NIM / f"bench-{nim}-{build}"), ("c", CB / f"bench-{c}-{build}")):
                    base = Path(tmp) / f"{side}-{nim}-{build}"
                    ro, pc, dp = f"{base}.ro", f"{base}.pc", f"{base}.dump"
                    args = {"count": [pc], "gate": [pc, dp], "mem": []}[mode]
                    rc, out = run(exe, [mode, wl, ro] + args)
                    if rc:
                        bad.append(f"{op}-{job} {side} {nim} {build}: exit {rc} {out[-300:]}")
                    outs[side] = (out, Path(ro).read_bytes(), pc, base)
                what = f"{op}-{job} {nim}/{c} {build}"
                (no, nro, npc, nbase), (co, cro, cpc, cbase) = outs["nim"], outs["c"]
                if nro != cro:
                    bad.append(f"{what}: read-outs differ")
                if nro != wl.with_suffix(".ref").read_bytes():
                    bad.append(f"{what}: Nim's read-out differs from m3's exact reference")
                for x in C.SET_INTERMEDIATES[op]:
                    if mode == "gate" and Path(f"{nbase}.ro.{x}").read_bytes() != Path(f"{cbase}.ro.{x}").read_bytes():
                        bad.append(f"{what}: intermediate {x} differs")
                if mode in ("count", "gate"):
                    nl, cl = Counter(ctr_lines(no)), Counter(ctr_lines(co))
                    # §17.6: C's n^2 first additions per mul step, case add_zero_x with 8 comparisons each
                    adj = Counter()
                    for kind, x, y, z in steps:
                        if kind == "mul":
                            zname = z
                            for ln in list(cl):
                                f = ln.split()
                                if f[1] == zname and f[2] == "add" and f[4] in ("case_add_zero_x", "cmp"):
                                    v = int(f[5]) - (q if f[4] == "case_add_zero_x" else 8 * q)
                                    cl.pop(ln)
                                    if v:
                                        adj[" ".join(f[:5] + [str(v)])] += 1
                    cl += adj
                    if nl != cl:
                        bad.append(f"{what}: counters differ: nim-only {sorted(nl - cl)[:3]} c-only {sorted(cl - nl)[:3]}")
                    nr, cr = records(npc), strip_first_additions(records(cpc), op, n)
                    if nr != cr:
                        bad.append(f"{what}: per-operation records differ ({len(nr)} / {len(cr)})")
                    if nim == "NPK":
                        def pk(o):
                            return [ln for ln in o.splitlines() if ln.startswith(("stor ", "pack ", "opc "))]
                        if pk(no) != pk(co):
                            d = [(a, b) for a, b in zip(pk(no), pk(co)) if a != b]
                            bad.append(f"{what}: stor/pack/opc differ: {d[:2]}")
                if mode == "mem":
                    def mm(o):
                        r = {}
                        for ln in o.splitlines():
                            if ln.startswith("memrun "):
                                f = ln.split()
                                r[f[1]] = {k: int(v) for k, v in (x.split("=") for x in f[2:])}
                        return r
                    nm, cm = mm(no), mm(co)
                    mem[nim] = {"nim": nm, "c": cm}
                    for key in cm:
                        for k in cm[key]:
                            if "heap" in k:
                                continue
                            a, b = nm[key][k], cm[key][k]
                            if a == b:
                                continue
                            if key == "op" and k == "peak_acct" and nim == "NPK" and nmul and a - b == 24 * n:
                                continue    # the row buffer, counted by §17.8 (C keeps it on the stack)
                            bad.append(f"{what}: memrun {key} {k}: nim {a}, c {b}")
    return bad, mem


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ops", default="add,mul,sop,pos")
    ap.add_argument("--out")
    a = ap.parse_args()
    allbad, mems = [], {}
    for op in a.ops.split(","):
        for wl in sorted((WORK / f"work-{op}").glob("job-*.wl")):
            _, n, job, _ = C.read_workload(wl)
            bad, mem = compare(op, job, wl, n)
            print(f"{op}-{job} n={n}: {'ok' if not bad else f'{len(bad)} differences'}", flush=True)
            allbad += bad
            mems[f"{op}-{job}"] = {"n": n, **mem}
    for b in allbad:
        print("DIFF", b)
    if a.out:
        Path(a.out).write_text(json.dumps(mems, indent=1))
    print(f"{len(mems)} jobs, {len(allbad)} differences")
    return 1 if allbad else 0


if __name__ == "__main__":
    sys.exit(main())
