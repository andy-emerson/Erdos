"""The Nim build's correctness check (README.md, "Verification"): every job of the given workload directories, through
both Nim models (build/bin/bench-{NCTL,NPK}-*) in every build, against the simulator (sim.py) and the exact references.
Nothing timed.

    python3 check.py DIR [DIR ...] [--jobs N] [--out FILE]

DIR holds job-NN.wl files: m3's workloads (test/build/m3/work-<op>, with their job-NN.ref, job-NN.<X>.ref) or this
build's v0.4 ones (gen.py: job-NN.ref.json). For each job:
  - the simulator's values against Python's fractions: every unmarked cell equal; every cell whose exact value does
    not fit 64 bits marked;
  - each model, each build (timing: time 1 1; counting: count; test: gate; memory: mem):
      the result's read-out, and in the gate each intermediate's, cell for cell equal to the simulator's (the mark as
      0/0), so every unmarked cell equals the exact reference;
      the driver's "marks" line (its scan after the clock): the simulator's count and cells;
  - counting and test builds, each model: every ctr/xctr line (core counts, cmp, cases) of every matrix, and the
    "mrk" lines (new, passed on, mark tests), equal to the simulator's; the per-operation record equal to the
    simulator's, record for record;
  - Packed: the "stor" and "pack" lines (W, T, widths, found, new, probes, longest, fields narrowed, bytes), the "opc"
    lines (cross tests, matches, cross reads); in the gate, the dump: each (h) at its pack slot for slot, each (l)
    entry for entry in arrival order, every handle field;
  - memory build: every memrun figure of the accounted total (acct, peak_acct, base_acct) equal to the simulator's
    event-by-event accounting; heap figures reported only;
  - Packed, counting and test builds: the row buffer's bytes per mul step (24 n) and the K-Q3 read counter (0);
  - the TC gate (tc3gate_m4: tc3.c, the new gcd, §17.6's summation), run up to the first overflow: on a job the
    simulator gives no overflow, it completes and Control's per-operation record and read-out equal its own; on one
    with overflows, it stops at the simulator's first overflowing operation, and Control's records before it equal
    its own;
  - Control against Packed: the read-outs, intermediates, per-operation records, counters and marks, byte-equal or
    line-equal.
Prints one line per job and a summary; a list of every difference. --out FILE writes each job's figures (marks,
records, memory) as JSON.
"""
import argparse
import json
import re
import struct
import subprocess
import sys
import tempfile
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent
BIN = HERE.parent / "build" / "bin"
sys.path.insert(0, str(HERE.parents[1] / "test"))
import m2_common as C  # noqa: E402
import sim  # noqa: E402

BUILT = {"add": ("C",), "mul": ("C",), "sop": ("P", "Q", "R"), "pos": ("S", "U", "R")}


def read_pairs(path, q):
    v = struct.unpack(f"<{2 * q}q", Path(path).read_bytes())
    return list(zip(v[:q], v[q:]))


def dump_sections(path):
    b, out, i = Path(path).read_bytes(), [], 0
    while i < len(b):
        name = b[i:i + 8].rstrip(b"\0").decode()
        n = struct.unpack_from("<q", b, i + 8)[0]
        out.append((name, list(struct.unpack_from(f"<{n}q", b, i + 16))))
        i += 16 + 8 * n
    return out


def exact_refs(wl, op, n, inputs):
    j = wl.with_suffix(".ref.json")
    if j.exists():
        return {k: [tuple(p) for p in v] for k, v in json.loads(j.read_text()).items()}
    return sim.exact(op, n, inputs)


def run(exe, args):
    p = subprocess.run([str(exe)] + [str(a) for a in args], capture_output=True, text=True)
    return p.returncode, p.stdout + p.stderr


def check_job(wl):
    wl = Path(wl)
    op, n, job, inputs = C.read_workload(wl)
    tag = f"{wl.parent.parent.name}/{wl.parent.name}/{op}-{job}"
    q = n * n
    bad = []
    p = sim.job(op, n, inputs)
    ex = exact_refs(wl, op, n, inputs)
    V = p["values"]
    names = list(sim.INPUTS[op]) + list(BUILT[op])
    # the simulator against fractions
    for x in BUILT[op]:
        for i, (got, want) in enumerate(zip(V[x], ex[x])):
            fits = abs(want[0]) <= sim.LIM and want[1] <= sim.LIM
            if got == sim.MARK:
                continue
            if not fits or got != want:
                bad.append(f"{tag}: simulator's {x}[{i}] = {got}, exact {want}")
    result = BUILT[op][-1]
    marked = [i for i, v in enumerate(V[result]) if v == sim.MARK]
    want_marks = f"marks {result} count={len(marked)} cells={','.join(map(str, marked))}"
    figs = {"op": op, "job": job, "n": n, "marks": {x: p["counts"][x].marks for x in BUILT[op]},
            "marked_cells": {x: sum(v == sim.MARK for v in V[x]) for x in BUILT[op]},
            "unmarked_fits": {x: sum(1 for v, e in zip(V[x], ex[x]) if v != sim.MARK) for x in BUILT[op]},
            "exact_past_64": {x: sum(1 for e in ex[x] if abs(e[0]) > sim.LIM or e[1] > sim.LIM) for x in BUILT[op]},
            "records": len(p["records"]), "mem": {}}
    want_ctr = set()
    for x in names:
        want_ctr |= p["counts"][x].lines(x)
    want_mrk = {f"mrk {x} new={p['counts'][x].marks[0]} passed={p['counts'][x].marks[1]} "
                f"tests={p['counts'][x].marks[2]}" for x in BUILT[op]}
    want_rec = b"".join(struct.pack("<6h", *r) for r in p["records"])
    ST = p["stores"]
    want_stor = set()
    for x in names:
        want_stor.add(sim.stor_line("end", x, ST[x]))
        if x in sim.INPUTS[op]:
            want_stor.add(sim.stor_line("built", x, ST[x]))
        want_stor.add(sim.pack_line(x, ST[x]))
    want_opc = {f"opc {x} cross={p['counts'][x].cross[0]} cross_match={p['counts'][x].cross[1]} "
                f"cross_reads={p['counts'][x].cross[2]}" for x in BUILT[op]}
    muls = {z for kind, _, _, z in sim.STEPS[op] if kind == "mul"}
    outs = {}
    with tempfile.TemporaryDirectory() as tmp:
        for impl in ("NCTL", "NPK"):
            for build, mode in (("timing", "time"), ("counting", "count"), ("test", "gate"), ("memory", "mem")):
                base = Path(tmp) / f"{impl}-{build}"
                ro, pc, dp = f"{base}.ro", f"{base}.pc", f"{base}.dump"
                args = {"time": [1, 1], "count": [pc], "gate": [pc, dp], "mem": []}[mode]
                rc, out = run(BIN / f"bench-{impl}-{build}", [mode, wl, ro] + args)
                what = f"{tag} {impl} {build}"
                if rc:
                    bad.append(f"{what}: exit {rc}: {out[-400:]}")
                    continue
                lines = out.splitlines()
                got = read_pairs(ro, q)
                if got != V[result]:
                    k = next(i for i, (a, b) in enumerate(zip(got, V[result])) if a != b)
                    bad.append(f"{what}: read-out differs from the simulator at cell {k}: {got[k]} / {V[result][k]}")
                if want_marks not in lines:
                    bad.append(f"{what}: the marks line differs: " + next((ln for ln in lines if ln.startswith("marks")), "none"))
                o = {"ro": Path(ro).read_bytes()}
                if mode in ("count", "gate"):
                    ctr = {ln for ln in lines if ln.startswith(("ctr ", "xctr "))}
                    if ctr != want_ctr:
                        bad.append(f"{what}: counters differ: extra {sorted(ctr - want_ctr)[:4]} missing {sorted(want_ctr - ctr)[:4]}")
                    mrk = {ln for ln in lines if ln.startswith("mrk ")}
                    if mrk != want_mrk:
                        bad.append(f"{what}: mrk differ: {sorted(mrk)} / {sorted(want_mrk)}")
                    rec = Path(pc).read_bytes()
                    if rec != want_rec:
                        bad.append(f"{what}: per-operation record differs ({len(rec) // 12} / {len(want_rec) // 12} records)")
                    o.update(ctr=ctr, mrk=mrk, rec=rec)
                    if impl == "NPK":
                        stor = {ln for ln in lines if ln.startswith(("stor ", "pack "))}
                        if stor != want_stor:
                            bad.append(f"{what}: stor/pack differ: extra {sorted(stor - want_stor)[:3]} missing {sorted(want_stor - stor)[:3]}")
                        rb = {ln for ln in lines if ln.startswith(("rowbuf ", "kq3 "))}
                        want_rb = {f"kq3 {x} reads=0" for x in names}
                        want_rb |= {f"rowbuf {x} bytes={24 * n if x in muls else 0}" for x in names}
                        if rb != want_rb:
                            bad.append(f"{what}: rowbuf/kq3 differ: {sorted(rb ^ want_rb)[:4]}")
                        opc = {ln for ln in lines if ln.startswith("opc ")}
                        if opc != want_opc:
                            bad.append(f"{what}: opc differ: {sorted(opc)} / {sorted(want_opc)}")
                if mode == "gate":
                    for x in BUILT[op][:-1]:
                        f = Path(f"{ro}.{x}")
                        if read_pairs(f, q) != V[x]:
                            bad.append(f"{what}: intermediate {x} differs from the simulator")
                        o["ro." + x] = f.read_bytes()
                    if impl == "NPK":
                        secs = dump_sections(dp)
                        want_secs = []
                        # the gate's order: each matrix's (h) at its pack; each matrix's (l) and fields at its end
                        order = list(sim.INPUTS[op])
                        packs = [("hb" + x, ST[x]["h"]) for x in order] + [("hb" + x, ST[x]["h"]) for x in BUILT[op]]
                        got_hb = [s for s in secs if s[0].startswith("hb")]
                        if got_hb != packs:
                            bad.append(f"{what}: (h) at the packs differs from the simulator's")
                        ends = {s[0]: s[1] for s in secs if not s[0].startswith("hb")}
                        for x in names:
                            if ends.get("tab" + x) != ST[x]["list"]:
                                bad.append(f"{what}: (l) of {x} differs from the simulator's")
                            if ends.get("cel" + x) != ST[x]["fields"]:
                                bad.append(f"{what}: the fields of {x} differ from the simulator's")
                        del want_secs
                if mode == "mem":
                    model = "packed" if impl == "NPK" else "control"
                    exp = p["memory"][model]
                    got = {}
                    for ln in lines:
                        if ln.startswith("memrun "):
                            f = ln.split()
                            got[f[1]] = {k: int(v) for k, v in (x.split("=") for x in f[2:])}
                    for key, want in exp.items():
                        for k, v in want.items():
                            if got.get(key, {}).get(k) != v:
                                bad.append(f"{what}: memrun {key} {k} = {got.get(key, {}).get(k)}, accounted {v}")
                    figs["mem"][impl] = got
                outs[(impl, mode)] = o
        # the TC gate (v0.1 §4; §17.10's TC-gate row; §17.13): tc3.c with the new gcd and §17.6's summation, up to the
        # first overflow; Control's per-operation record must equal it, operation for operation
        gro, gpc = Path(tmp) / "gate.ro", Path(tmp) / "gate.pc"
        rc, out = run(BIN / "tc3gate_m4", [wl, gro, gpc])
        ctl = outs.get(("NCTL", "count"))
        if rc:
            bad.append(f"{tag} TC gate: exit {rc}: {out[-300:]}")
        elif ctl is not None:
            grec = gpc.read_bytes()
            first = p["first_overflow"]
            if first is None:
                if "complete" not in out.split():
                    bad.append(f"{tag} TC gate: stopped on a job the simulator gives no overflow: {out.strip()}")
                if grec != ctl["rec"]:
                    bad.append(f"{tag} TC gate: Control's record differs from tc3.c's")
                if gro.read_bytes() != ctl["ro"]:
                    bad.append(f"{tag} TC gate: Control's read-out differs from tc3.c's")
            else:
                if f"at={first}" not in out:
                    bad.append(f"{tag} TC gate: did not stop at the simulator's first overflow, operation {first}: "
                               f"{out.strip()}")
                if grec != ctl["rec"][:12 * first]:
                    bad.append(f"{tag} TC gate: Control's record before the first overflow differs from tc3.c's")
            figs["gate"] = out.strip().splitlines()[-1]
        for mode in ("time", "count", "gate", "mem"):
            a, b = outs.get(("NCTL", mode)), outs.get(("NPK", mode))
            if a is None or b is None:
                continue
            for k in a:
                if k in b and a[k] != b[k]:
                    bad.append(f"{tag} {mode}: Control and Packed differ in {k}")
    return tag, bad, figs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dirs", nargs="+")
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--out")
    a = ap.parse_args()
    wls = [w for d in a.dirs for w in sorted(Path(d).glob("job-*.wl"))]
    allbad, figs = [], {}
    with ProcessPoolExecutor(a.jobs) as ex:
        for tag, bad, f in ex.map(check_job, wls):
            print(f"{tag}: {'ok' if not bad else f'{len(bad)} differences'} marks={f['marks']}", flush=True)
            allbad += bad
            figs[tag] = f
    if a.out:
        Path(a.out).write_text(json.dumps(figs, indent=1))
    for b in allbad:
        print("DIFF", b)
    print(f"{len(wls)} jobs, {len(allbad)} differences")
    return 1 if allbad else 0


if __name__ == "__main__":
    sys.exit(main())
