"""The m4 correctness check (test/DECISIONS-m4.md): on every m3 workload (build/m3/work-<op>/job-NN.wl, all four ops,
checked against m3's gen.json sha256), the reference build (mh2.c's Hash-V-pack, TC with 08's gcd) against the m4 build
(Hash-V-pack-4, mh4.c; TC with the m4 gcd), both from m4_libconf.py. Nothing timed: the test build's gate, the counting
build's count and the memory build's mem mode, one run each.

For each workload and each pair (Hash-V-pack / Hash-V-pack-4; TC with 08's gcd / with the m4 gcd), in each build:
  - the result's read-out (every cell, numerator and denominator), and in the gate each intermediate's, are byte-equal
    between the two, and equal to the exact reference (job-NN.ref, job-NN.<X>.ref);
  - the per-operation record (each operation's case, gcds, gcd work, divisions, multiplications, additions) is
    byte-equal;
  - the gate's dump: TC has none; Hash-V-pack-4's slots, list order and handles differ from mh2.c's by design (the
    bound 2RK, the index table, the list in the order of arrival), so the dump is compared by value: for every matrix,
    every handle field's value (list[handle]) and tag, field by field; the list as a set (W entries); every packing's
    (h) as the set of the values held; the sections in the same order;
  - the printed counters and figures are equal, line for line; for Hash-V-pack, except the row's label and the fields
    that follow by design from its sizes and layout: stor's T, w, whb, probes, longest, widenings, copied, bvals; pack's
    T, w, whb, rewritten, bytes_list, bytes_map, bytes_table, bytes_cells_build. The memory build's figures are
    reported (--mem-report), not compared.
Usage: python3 m4_check.py [--jobs N] [--ops add,mul,sop,pos] [--mem-report FILE]
"""
import argparse
import hashlib
import json
import re
import struct
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent
WORK = HERE / "build" / "m3"
BIN = HERE / "build" / "m4"
OPS = ("add", "mul", "sop", "pos")
M4_IMPL = {"TC": "TC", "MHVP": "MHV4"}             # the m4 side's binary for each reference binary
# Hash-V-pack's fields that differ by design (its sizes and layout)
WIDTH_FIELDS = {"stor": ("T", "w", "whb", "probes", "longest", "widenings", "copied", "bvals"),
                "pack": ("T", "w", "whb", "rewritten", "bytes_list", "bytes_map", "bytes_table", "bytes_cells_build")}


def dump_sections(path):
    """[(name, [int64])]: the gate's dump, section by section"""
    b = Path(path).read_bytes()
    out, i = [], 0
    while i < len(b):
        name = b[i:i + 8].rstrip(b"\0").decode()
        n = struct.unpack_from("<q", b, i + 8)[0]
        out.append((name, list(struct.unpack_from(f"<{n}q", b, i + 16))))
        i += 16 + 8 * n
    return out


def dump_values(path):
    """the dump by value: for tab<X> the sorted list, for cel<X> each field's (value, tag) (with the tab<X> before
    it), for hb<X> the sorted values held"""
    out, tab = [], {}
    for name, v in dump_sections(path):
        if name.startswith("tab"):
            tab[name[3:]] = v
            out.append((name, sorted(v)))
        elif name.startswith("cel"):
            t = tab[name[3:]]
            out.append((name, [(t[f >> 1], f & 1) for f in v]))
        elif name.startswith("hb"):
            out.append((name, sorted(x for x in v if x != -1)))
        else:
            out.append((name, v))
    return out


def workloads(ops):
    out = []
    for op in ops:
        d = WORK / f"work-{op}"
        gen = json.loads((d / "gen.json").read_text())
        for job, g in sorted(gen.items(), key=lambda kv: int(kv[0])):
            wl = d / f"job-{int(job):02d}.wl"
            if not wl.exists():
                continue
            for name, want in g["sha256"].items():
                if hashlib.sha256((d / name).read_bytes()).hexdigest() != want:
                    sys.exit(f"{d / name}: sha256 differs from m3's gen.json")
            out.append((op, int(job), wl))
    return out


def norm(text, impl):
    """the printed lines, with Hash-V-pack's by-design width fields left out"""
    lines = []
    for ln in text.splitlines():
        kind = ln.split(" ", 1)[0]
        if impl == "MHVP" and kind in WIDTH_FIELDS:
            for f in WIDTH_FIELDS[kind]:
                ln = re.sub(rf" {f}=\d+", "", ln)
        if impl == "MHVP" and kind == "ok":  # the row's label: Hash-V-pack / Hash-V-pack-4
            ln = re.sub(r" impl=\S+", "", ln)
        lines.append(ln)
    return lines


def run(side, impl, build, mode, wl, tmp, extra=()):
    exe = BIN / side / f"bench-{impl}-{build}"
    base = Path(tmp) / f"{side}-{impl}-{mode}"
    ro, pc, dp = str(base) + ".ro", str(base) + ".pc", str(base) + ".dump"
    args = {"gate": [pc, dp], "count": [pc], "mem": []}[mode]
    p = subprocess.run([str(exe), mode, str(wl), ro] + args, capture_output=True, text=True)
    files = {"ro": Path(ro)}
    if mode != "mem":
        files["pc"] = Path(pc)
    if mode == "gate":
        files["dump"] = Path(dp)
        for x in Path(tmp).glob(f"{base.name}.ro.*"):
            files["ro." + x.name.rsplit(".", 1)[1]] = x
    return p, files


def check_one(item):
    op, job, wl = item
    tag = f"{op} job {job:02d}"
    bad, mem = [], {}
    ref = wl.with_suffix(".ref").read_bytes()
    with tempfile.TemporaryDirectory() as tmp:
        for impl in ("TC", "MHVP"):
            for build, mode in (("test", "gate"), ("counting", "count"), ("memory", "mem")):
                pr, fr = run("ref", impl, build, mode, wl, tmp)
                p4, f4 = run("m4", M4_IMPL[impl], build, mode, wl, tmp)
                what = f"{tag} {impl} {mode}"
                if pr.returncode or p4.returncode:
                    bad.append(f"{what}: exit {pr.returncode}/{p4.returncode}: {pr.stdout[-300:]} {p4.stdout[-300:]}")
                    continue
                if set(fr) != set(f4):
                    bad.append(f"{what}: output files differ: {sorted(fr)} / {sorted(f4)}")
                for k in fr:
                    if k not in f4:
                        continue
                    if k == "dump" and impl == "MHVP":
                        same = dump_values(fr[k]) == dump_values(f4[k])
                    else:
                        same = fr[k].read_bytes() == f4[k].read_bytes()
                    if not same:
                        bad.append(f"{what}: {k} differs")
                if fr["ro"].read_bytes() != ref:
                    bad.append(f"{what}: the reference build's read-out differs from the exact reference")
                if f4["ro"].read_bytes() != ref:
                    bad.append(f"{what}: the m4 read-out differs from the exact reference")
                for k in fr:
                    if k.startswith("ro.") and fr[k].read_bytes() != (wl.parent / f"job-{job:02d}.{k[3:]}.ref").read_bytes():
                        bad.append(f"{what}: intermediate {k[3:]} differs from its exact reference")
                if mode == "mem":
                    mem[impl] = (pr.stdout, p4.stdout)
                    continue
                a, b = norm(pr.stdout, impl), norm(p4.stdout, impl)
                if a != b:
                    diff = next((f"{x!r} / {y!r}" for x, y in zip(a, b) if x != y), f"{len(a)} / {len(b)} lines")
                    bad.append(f"{what}: printed lines differ: {diff}")
    return tag, bad, mem


def peak(text, key):
    m = re.search(rf"memrun op peak_acct=(\d+) peak_heap=(\d+)", text)
    return int(m.group(1 if key == "acct" else 2)) if m else None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--ops", default=",".join(OPS))
    ap.add_argument("--mem-report")
    a = ap.parse_args()
    items = workloads(a.ops.split(","))
    nbad, rows = 0, []
    with ThreadPoolExecutor(a.jobs) as ex:
        for tag, bad, mem in ex.map(check_one, items):
            print(f"{tag}: {'ok' if not bad else 'FAIL'}", flush=True)
            for b in bad:
                print("   ", b)
            nbad += len(bad)
            if "MHVP" in mem:
                r, m = mem["MHVP"]
                rows.append((tag, peak(r, "acct"), peak(m, "acct"), peak(r, "heap"), peak(m, "heap")))
    if a.mem_report:
        with open(a.mem_report, "w") as f:
            f.write("workload,mh2_op_peak_acct,mh4_op_peak_acct,mh2_op_peak_heap,mh4_op_peak_heap\n")
            for r in rows:
                f.write(",".join(str(x) for x in r) + "\n")
    print(f"m4_check (Hash-V-pack-4 and TC with the m4 gcd, against mh2.c and TC with 08's gcd): {len(items)} workloads x 2 pairs x 3 builds; {nbad} differences")
    sys.exit(1 if nbad else 0)


if __name__ == "__main__":
    main()
