"""check.py: the correctness check. Every result of every build x operation x job, from the binary that times that job
(<b>-j1 for job 1, <b> for the others), and Growing and Byte encoding at W = 32 besides, against Python's exact value.

A result equal to the exact value is exact; any other is counted inexact (spec §2). Each result is also checked against
the build's own model, so an inexact result is the build's wrap and nothing else:
- Control: uint64 arithmetic on the operands mod 2^64 (a zero divisor traps);
- Power-of-2 and Even flag: the spec's formulas on the operands mod 2^63; the flag bit must be the rule's
  (Power-of-2: shift counts as x86 takes them, mod 64; a ctz of 0, which gcc leaves undefined, is not modelled);
- Growing, Byte encoding: exact, except subtract with a < b: a - b mod 2^(W * max(words of a, b)).
A failure is a result that differs from its model, a wrong flag, or a trap the model does not predict.
Usage: python3 check.py [work dir] [bin dir] [out dir]"""
import json
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from math import gcd
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen  # noqa: E402

HERE = Path(__file__).resolve().parent.parent
BUILDS = ("control", "pow2", "even", "growing", "bytes")
M64, M63 = 2**64, 2**63


def ctz(x):
    return (x & -x).bit_length() - 1 if x else None


def model(build, w, op, a, b):
    if op == "absdiff2":
        op = "absdiff"
    """the build's expected value (raw, flag included for the flag builds), "trap", or None (not modelled)"""
    if build in ("growing", "bytes"):
        if op == "sub" and a < b:
            nw = max(-(-a.bit_length() // w), -(-b.bit_length() // w))
            return (a - b) % 2**(w * nw)
        return gen.exact(op, a, b)
    if build == "control":
        x, y = a % M64, b % M64
        if op == "divexact" and y == 0:
            return "trap"
        return {"add": (x + y) % M64, "sub": (x - y) % M64, "absdiff": abs(x - y), "mul": x * y % M64,
                "divexact": x // y if y else None, "gcd": gcd(x, y)}[op]
    x, y = a % M63, b % M63
    if build == "even":
        mk = lambda r: (r % M63) | ((r % M63) % 2 == 0) << 63          # noqa: E731
        if op == "divexact" and y == 0:
            return "trap"
        r = {"add": x + y, "sub": x - y, "absdiff": abs(x - y), "mul": x * y,
             "divexact": x // y if y else 0, "gcd": gcd(x, y)}[op]
        return mk(r % M64)
    p2 = lambda v: v != 0 and v & (v - 1) == 0                            # noqa: E731
    mk = lambda r: (r % M63) | p2(r % M63) << 63                          # noqa: E731
    fa, fb = p2(x), p2(y)
    if op in ("add", "sub", "absdiff"):
        return mk({"add": x + y, "sub": (x - y) % M64, "absdiff": abs(x - y)}[op])
    if op == "mul":
        return mk((y << ctz(x)) % M64 if fa else (x << ctz(y)) % M64 if fb else x * y % M64)
    if op == "divexact":
        if fb:
            return mk(x >> ctz(y))
        if fa:
            return None if y == 0 else mk((1 << ((ctz(x) - ctz(y)) % 64)) % M64)
        return "trap" if y == 0 else mk(x // y)
    if fa or fb:
        if x == 0 or y == 0:
            return None
        return mk(1 << min(ctz(x), ctz(y)))
    return mk(gcd(x, y))


def binary(bindir, build, k, w32=False):
    return Path(bindir) / (f"{build}-w32" if w32 else f"{build}-j1" if k == 1 else build)


def check_cell(bindir, outdir, build, w32, k, path, op, pairs):
    exe = binary(bindir, build, k, w32)
    out = Path(outdir) / f"{exe.name}-{k}-{op}.txt"
    p = subprocess.run([str(exe), "dump", str(path), op, str(out)], capture_output=True, text=True)
    w = 32 if w32 else 64
    cell = {"build": build, "W": w if build in ("growing", "bytes") else None, "job": k, "op": op,
            "binary": exe.name, "n": len(pairs), "exact": 0, "inexact": 0, "failures": [], "unmodelled": 0}
    expect = [model(build, w, op, a, b) for a, b in pairs]
    if p.returncode != 0:
        cell["trap"] = f"exit {p.returncode}" + (" (SIGFPE)" if p.returncode == -8 else "")
        cell["zero_divisors"] = sum(e == "trap" for e in expect)
        if not any(e == "trap" for e in expect):
            cell["failures"].append(f"unexpected exit {p.returncode}: {p.stderr.strip()[:200]}")
        return cell
    got = [int(x, 16) for x in out.read_text().split()]
    out.unlink()
    if any(e == "trap" for e in expect):
        cell["failures"].append("no trap where the model has a zero divisor")
    for j, ((a, b), g, e) in enumerate(zip(pairs, got, expect)):
        v = g % M63 if build in ("pow2", "even") else g
        if v == gen.exact(op, a, b):
            cell["exact"] += 1
        else:
            cell["inexact"] += 1
        if e is None:
            cell["unmodelled"] += 1
            if build == "pow2":
                f = g >> 63
                if f != (v != 0 and v & (v - 1) == 0):
                    cell["failures"].append(f"#{j}: wrong flag")
        elif g != e:
            if len(cell["failures"]) < 5:
                cell["failures"].append(f"#{j}: {op}({a:#x}, {b:#x}) = {g:#x}, model {e:#x}")
            else:
                cell["failures"].append("...")
    return cell


def check(workdir, bindir, outdir):
    outdir = Path(outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    jobs = {k: gen.read_job(Path(workdir) / f"job-{k}.bin")[1] for k in gen.JOBS}
    tasks = [(b, w32, k, op) for b in BUILDS for w32 in (False, True) if not w32 or b in ("growing", "bytes")
             for k in gen.JOBS for op in gen.OPS]
    with ThreadPoolExecutor(2) as ex:
        cells = list(ex.map(lambda t: check_cell(bindir, outdir, t[0], t[1], t[2], Path(workdir) / f"job-{t[2]}.bin",
                                                 t[3], jobs[t[2]][t[3]]), tasks))
    return cells


def summary(cells):
    lines = []
    fails = [c for c in cells if c["failures"]]
    traps = [c for c in cells if c.get("trap")]
    n = sum(c["n"] for c in cells if not c.get("trap"))
    lines.append(f"{len(cells)} cells, {n} results checked; failures in {len(fails)} cells; traps in {len(traps)} cells")
    for c in fails:
        lines.append(f"FAIL {c['binary']} job {c['job']} {c['op']}: {c['failures'][:3]}")
    for c in traps:
        lines.append(f"trap {c['binary']} job {c['job']} {c['op']}: {c['trap']}, {c['zero_divisors']} divisors are 0 "
                     f"in the build's width")
    return lines


def table(cells):
    """inexact results per build x operation x job (W = 64 builds)"""
    rows = ["| build | op | " + " | ".join(f"job {k}" for k in gen.JOBS) + " |",
            "| --- | --- | " + " | ".join("---" for _ in gen.JOBS) + " |"]
    for b in BUILDS:
        for op in gen.OPS:
            cs = {c["job"]: c for c in cells if c["build"] == b and c["op"] == op and c["W"] in (None, 64)}
            rows.append(f"| {b} | {op} | " + " | ".join(
                ("trap" if cs[k].get("trap") else f"{cs[k]['inexact']}") for k in gen.JOBS) + " |")
    return rows


if __name__ == "__main__":
    work = Path(sys.argv[1]) if len(sys.argv) > 1 else HERE / "build" / "work"
    bins = Path(sys.argv[2]) if len(sys.argv) > 2 else HERE / "build" / "bin"
    out = Path(sys.argv[3]) if len(sys.argv) > 3 else HERE / "build" / "check"
    cells = check(work, bins, out)
    (out / "check.json").write_text(json.dumps(cells, indent=1))
    print("\n".join(summary(cells)))
    print("\ninexact results (of N per cell):")
    print("\n".join(table(cells)))
    sys.exit(1 if any(c["failures"] for c in cells) else 0)
