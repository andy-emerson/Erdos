"""The checks of round 2's harness that compare a run's output with exact figures (spec v0.2 §14.1; rule-r2 items 3
and 12): parsing the driver's lines, the read-out against the reference, the gate's exact checks against the
generator's prediction, and the profiling build's slowdown. Pure Python, no numpy (the runner imports it).
"""
import math
import struct
from pathlib import Path

import r2_common as C

# the counters both rows make, and those that are Q32's alone (the whole numbers, lookups and reads)
BOTH = ("gcd", "gcd_work", "div_exact", "mul", "add", "cmp", "zero_results")
Q32_ONLY = ("reads", "reads_skipped", "intern_found", "intern_appended", "lookup_search")


def parse_counts(lines):
    """{(phase, name): total over core and store} from ctr and xctr lines."""
    tot = {}
    for l in lines:
        t = l.split()
        if t and t[0] in ("ctr", "xctr"):
            tot[(t[1], t[3])] = tot.get((t[1], t[3]), 0) + int(t[4])
    return tot


def parse_mem(lines):
    """{when: {"items": {name: (used, allocated)}, "N": n}} from mem and memN lines (an item's name may hold spaces)."""
    out = {}
    for l in lines:
        t = l.split()
        if not t:
            continue
        if t[0] == "mem":
            out.setdefault(t[1], {"items": {}})["items"][" ".join(t[2:-2])] = (int(t[-2]), int(t[-1]))
        elif t[0] == "memN":
            out.setdefault(t[1], {"items": {}})["N"] = int(t[2])
    return out


def readout_check(readout, reference, workload=None):
    """None, or the first stored value that differs from the exact reference (rule-r2 item 12). The read-out and the
    reference have one format (int64 num[n], int64 den[n]), so equal files are equal values."""
    try:
        got = Path(readout).read_bytes()
    except OSError:
        return {"call": None, "what": "no read-out"}
    want = reference if isinstance(reference, bytes) else Path(reference).read_bytes()
    if got == want:
        return None
    if len(got) != len(want):
        return {"call": None, "what": f"read-out has {len(got) // 16} values, reference {len(want) // 16}"}
    n = len(want) // 16
    gn, gd = struct.unpack(f"<{n}q", got[:8 * n]), struct.unpack(f"<{n}q", got[8 * n:])
    wn, wd = struct.unpack(f"<{n}q", want[:8 * n]), struct.unpack(f"<{n}q", want[8 * n:])
    bad = [i for i in range(n) if gn[i] != wn[i] or gd[i] != wd[i]]
    i = bad[0]
    return {"call": i, "got": f"{gn[i]}/{gd[i]}", "want": f"{wn[i]}/{wd[i]}", "mismatches": len(bad)}


def exact_checks(impl, lines, pred, wholes_sha256=None):
    """Spec v0.2 §14.1 item 2 on a gate run (test build): every counter the generator predicts, per phase, equal to
    the row's; for Q32 also W after entry and at the end, and the whole-number list. Returns a list of failures."""
    fails = []
    tot = parse_counts(lines)
    pc = pred["counters"]
    for ph in C.PHASES:
        want = pc.get(ph, {})
        names = set(want) | {n for (p, n) in tot if p == ph}
        for n in sorted(names):
            q32_only = n in Q32_ONLY or n.startswith(("kq1_", "kq2_")) or n == "lookup_probe"
            if n == "lookup_probe":
                continue                                   # probes depend on the hash: reported, not predicted
            w = want.get(n, 0)
            if impl == "TC" and q32_only:
                w = 0                                      # TC has no whole numbers, lookups or reads
            if n == "ev_overflows":
                w = 0
            g = tot.get((ph, n), 0)
            if g != w:
                fails.append(f"{ph} {n}: {g} != predicted {w}")
    if impl == "Q32":
        wl = {l.split()[1]: int(l.split()[2]) for l in lines if l.startswith("wholes ")}
        if wl.get("after_entry") != pred["W_after_entry"]:
            fails.append(f"W after entry {wl.get('after_entry')} != predicted {pred['W_after_entry']}")
        if wl.get("end") != pred["W_end"]:
            fails.append(f"W at the end {wl.get('end')} != predicted {pred['W_end']}")
        if wholes_sha256 is not None and wholes_sha256 != pred.get("W_sha256"):
            fails.append("the whole-number list differs from the predicted one")
    return fails


def gcd_work(lines):
    """G: the gcd work of entry and the operations (rule-r2 item 7), from a counting run's lines."""
    return sum(v for (ph, n), v in parse_counts(lines).items() if n == "gcd_work" and ph in C.PHASES)


def parse_reps(lines, kind=("calib", "rep")):
    """[(index or r, total, entry, ops, read)] from calib or rep lines."""
    out = []
    for l in lines:
        t = l.split()
        if t and t[0] in kind:
            out.append(tuple(int(x) for x in t[1:6]))
    return out


def parse_rep_cpu(lines, kind=("calib", "rep")):
    """[(cpu_ns, span_ns) or None] from calib or rep lines, in the order of parse_reps: the measured process's own CPU
    time (CLOCK_PROCESS_CPUTIME_ID) and the CLOCK_MONOTONIC time over the same whole rep (rule-r2 item 4, revision of
    Oct 29, 2026: reported only). None for a line without them."""
    out = []
    for l in lines:
        t = l.split()
        if t and t[0] in kind:
            kv = dict(x.split("=", 1) for x in t[6:] if "=" in x)
            out.append((int(kv["cpu"]), int(kv["span"])) if "cpu" in kv and "span" in kv else None)
    return out


def host_allowance(rate, seconds):
    """rule-r2 item 5.2 (revision of Oct 29, 2026, approved by the author): over a block of L seconds a host process may
    use at most 5 + ceil(2 R_h L) ticks, with R_h its rate from the pilot (host_rates)."""
    return C.HOST_BASE_TICKS + math.ceil(C.HOST_FACTOR * rate * seconds)


def host_rates(checks):
    """rule-r2 item 5.1: from the pilot's disturbance checks, ({pid: measured}, {pid: R_h}). measured is each host's
    highest rate over the blocks; R_h is that rate, but at least 1 tick over the longest pilot block, the finest rate
    the pilot can resolve (a host with no tick in the pilot is not taken to use none). The ceiling applies to the
    measured rate."""
    measured = {}
    for d in checks:
        for pid, h in d["hosts"].items():
            measured[pid] = max(measured.get(pid, 0.0), h["rate"])
    longest = max((d["seconds"] for d in checks if d.get("seconds")), default=None)
    floor = C.HOST_RATE_MIN_TICKS / longest if longest else 0.0
    return measured, {pid: max(r, floor) for pid, r in measured.items()}


def other_limit(seconds):
    """rule-r2 item 5: the ticks a process other than the runner, a host or a new one may use over a block of L seconds
    before the block is disturbed: 5 + ceil(1 x L) (a floor plus a rate of 1 tick per second)."""
    return C.DIST_OTHER_TICKS + math.ceil(C.DIST_OTHER_RATE * seconds)


def runner_ticks_check(ticks, launches):
    """rule-r2 item 5 (second revision of Oct 29, 2026): the runner's own ticks over a block are recorded and reported,
    in total and per launch (the timed invocations it started in the block), and are not judged against the timings.
    The one sanity limit: more than 1 tick per launch plus 2 over the block is a harness bug; the 2 are a fixed
    allowance for the block's own two snapshots, which cost the same whatever its launches (minor fix found in
    review). Returns (ticks per launch, or None for a block with no launch; True if over the limit). A block with no
    launch is held to the allowance of 2 (such a block should not occur)."""
    if ticks is None:
        return None, False
    per = ticks / launches if launches else None
    return per, ticks > C.RUNNER_TICKS_PER_LAUNCH_MAX * launches + C.RUNNER_TICKS_BLOCK_ALLOWANCE


def pilot_projection(elapsed_s, p_s, p_prof_s):
    """rule-r2, Run plan step 3: elapsed + 5 min x max(1, P / 0.5 min) + 36 x P_prof, in seconds."""
    return elapsed_s + C.PILOT_REST_MIN * 60 * max(1.0, p_s / (C.PILOT_NOMINAL_MIN * 60)) + C.PILOT_PROF_FACTOR * p_prof_s


def slowdown(prof_total_per_run, timing_median_per_run):
    """rule-r2 item 10, spec §13.4 item 8: the profiling invocation's batch total over the timing build's median total
    for the same row and job; and whether it is within the limit."""
    s = prof_total_per_run / timing_median_per_run
    return s, s <= C.PROF_SLOWDOWN_MAX


def parse_prof(lines):
    """A profiling invocation's output: probe cost, TSC rate, K, and per rep its batch totals (ns), sampled calls and
    accumulators {"cyc": {(jp, case, phase): (cycles, segments)}, "calls": {(jp, case): (n, outer)},
    "m": {jp: (cycles, n)}}."""
    out = {"reps": {}}
    for l in lines:
        t = l.split()
        if not t:
            continue
        if t[0] == "probe_cost":
            out["probe_cost"] = float(t[1])
        elif t[0] == "tsc_hz":
            out["tsc_hz"] = float(t[1])
        elif t[0] == "prof_k":
            out["K"] = int(t[1])
        elif t[0] == "prep":
            out["reps"].setdefault(int(t[1]), {"cyc": {}, "calls": {}, "m": {}}).update(
                total=int(t[2]), entry=int(t[3]), ops=int(t[4]), read=int(t[5]), sampled=int(t[6].split("=")[1]))
        elif t[0] == "pf":
            out["reps"].setdefault(int(t[1]), {"cyc": {}, "calls": {}, "m": {}})["cyc"][(int(t[2]), t[3], int(t[4]))] = \
                (int(t[5]), int(t[6]))
        elif t[0] == "pfcalls":
            out["reps"].setdefault(int(t[1]), {"cyc": {}, "calls": {}, "m": {}})["calls"][(int(t[2]), t[3])] = \
                (int(t[4]), int(t[5]))
        elif t[0] == "pfm":
            out["reps"].setdefault(int(t[1]), {"cyc": {}, "calls": {}, "m": {}})["m"][int(t[2])] = (int(t[3]), int(t[4]))
    return out


def pack_prof(rep):
    """A profiling rep's accumulators with string keys, for state.json."""
    return {"total": rep["total"], "entry": rep["entry"], "ops": rep["ops"], "read": rep["read"],
            "sampled": rep["sampled"],
            "cyc": [[jp, k, ph, c, s] for (jp, k, ph), (c, s) in sorted(rep["cyc"].items())],
            "calls": [[jp, k, n, o] for (jp, k), (n, o) in sorted(rep["calls"].items())],
            "m": [[jp, c, n] for jp, (c, n) in sorted(rep["m"].items())]}
