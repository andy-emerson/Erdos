"""The checks of the matrix round's harness that compare a run's output with exact figures (spec v0.1 §10; rule-m1
items 3 and 12): parsing the driver's lines, the read-out against the reference, the gate's exact checks against the
model's prediction, the per-operation records; and round 3's helpers for the timed blocks (reps, CPU time, the
disturbance check's limits, the pilot's projection). From round 3's r3_checks.py. Pure Python, no numpy.
"""
import hashlib
import math
import struct
from pathlib import Path

import m1_common as C


def kv(tokens):
    return {k: int(v) for k, v in (t.split("=", 1) for t in tokens if "=" in t)}


def parse_counts(lines):
    """{(phase, name): total over core and store} from ctr and xctr lines (summed over A, B, C by the driver)"""
    tot = {}
    for l in lines:
        t = l.split()
        if t and t[0] in ("ctr", "xctr"):
            tot[(t[1], t[3])] = tot.get((t[1], t[3]), 0) + int(t[4])
    return tot


def parse_stor(lines):
    """{when: {role: {field: value}}} from stor lines"""
    out = {}
    for l in lines:
        t = l.split()
        if t and t[0] == "stor":
            out.setdefault(t[1], {})[t[2]] = kv(t[3:])
    return out


def parse_opc(lines):
    """{when: {field: value}} from opc lines (the operation's own counters)"""
    out = {}
    for l in lines:
        t = l.split()
        if t and t[0] == "opc":
            out[t[1]] = kv(t[2:])
    return out


def parse_growths(lines):
    """[{role, fields, W, T_old, reinserted, rprobes, rewritten, wprobes, bytes_new, bytes_map}] in their order"""
    out = []
    for l in lines:
        t = l.split()
        if t and t[0] == "growth":
            g = kv(t[2:])
            g["role"] = t[1]
            out.append(g)
    return out


def parse_mem(lines):
    """{when: {"items": {(role, name): (used, allocated)}, "tau": bytes, "N": cells}} from mem, memtau, memN lines"""
    out = {}
    for l in lines:
        t = l.split()
        if not t:
            continue
        if t[0] == "mem":
            out.setdefault(t[1], {"items": {}})["items"][(t[2], " ".join(t[3:-2]))] = (int(t[-2]), int(t[-1]))
        elif t[0] == "memtau":
            out.setdefault(t[1], {"items": {}})["tau"] = int(t[2])
        elif t[0] == "memN":
            out.setdefault(t[1], {"items": {}})["N"] = int(t[2])
    return out


def parse_memrun(lines):
    """{when: {heap, used, cells, ...}} from the memory run's memrun lines (after_creation, end, peak)"""
    out = {}
    for l in lines:
        t = l.split()
        if t and t[0] == "memrun":
            out[t[1]] = kv(t[2:])
    return out


def readout_check(readout, reference):
    """None, or the first cell of C that differs from the exact reference (rule-m1 item 12). The read-out and the
    reference have one format (int64 num[n^2], int64 den[n^2]), so equal files are equal values."""
    try:
        got = Path(readout).read_bytes()
    except OSError:
        return {"cell": None, "what": "no read-out"}
    want = reference if isinstance(reference, bytes) else Path(reference).read_bytes()
    if got == want:
        return None
    if len(got) != len(want):
        return {"cell": None, "what": f"read-out has {len(got) // 16} cells, reference {len(want) // 16}"}
    n = len(want) // 16
    gn, gd = struct.unpack(f"<{n}q", got[:8 * n]), struct.unpack(f"<{n}q", got[8 * n:])
    wn, wd = struct.unpack(f"<{n}q", want[:8 * n]), struct.unpack(f"<{n}q", want[8 * n:])
    bad = [i for i in range(n) if gn[i] != wn[i] or gd[i] != wd[i]]
    i = bad[0]
    return {"cell": i, "got": f"{gn[i]}/{gd[i]}", "want": f"{wn[i]}/{wd[i]}", "mismatches": len(bad)}


def predicted_counters(impl, pred):
    """{phase: {name: value}} the counting and test builds of a row must print: every row's arithmetic counters, and
    the K-Q3 reads of the M rows"""
    out = {ph: dict(v) for ph, v in pred["counters"].items()}
    if impl in C.M_IMPLS:
        for ph, v in pred["reads_M"].items():
            if v:
                out.setdefault(ph, {})["reads"] = v
    return out


def sizing(impl):
    return "grow" if impl in C.GROW_IMPLS else "fixed"


def exact_checks(impl, lines, pred, dump=None):
    """Spec §10 item 2 on a gate (test build) or counting run: every counter per phase equal to the model's; for the
    hash rows every store's figures after creation and at the end (W, T, w, w_h, fields, found/new, probes, longest,
    widenings, slots copied, growths, reinsertions and their probes, fields rewritten and the lookup's rewrite
    probes), every growth, the operation's cross tests, cross reads and tau's finds; with a gate's dump, each table
    slot by slot, each matrix's handle fields and tau position by position (by sha256). Returns a list of failures."""
    fails = []
    tot = parse_counts(lines)
    want = predicted_counters(impl, pred)
    for ph in C.PHASES:
        w = want.get(ph, {})
        names = set(w) | {n for (p, n) in tot if p == ph}
        for n in sorted(names):
            g, e = tot.get((ph, n), 0), w.get(n, 0)
            if n == "ev_overflows":
                e = 0
            if g != e:
                fails.append(f"{ph} {n}: {g} != predicted {e}")
    if impl == "TC":
        if parse_stor(lines) or parse_growths(lines):
            fails.append("TC printed store figures")
        return fails
    sz = pred["sizings"][sizing(impl)]
    mp = impl == "MHVGM"
    st = parse_stor(lines)
    for when in ("after_creation", "end"):
        key = f"stor_{when}" + ("_map" if mp else "")
        for r in C.ROLES:
            got = st.get(when, {}).get(r)
            if got is None:
                fails.append(f"no store figures for {r} {when}")
                continue
            for f in C.STORE_FIELDS:
                if got[f] != sz[key][r][f]:
                    fails.append(f"store {r} {when} {f}: {got[f]} != predicted {sz[key][r][f]}")
    g = parse_growths(lines)
    wg = [dict(x, wprobes=0 if mp else x["wprobes"], bytes_map=x["bytes_map"] if mp else 0) for x in sz["growths"]]
    if g != wg:
        fails.append(f"growths differ from the predicted ones ({len(g)} against {len(wg)})")
    opc = parse_opc(lines)
    z = {f: 0 for f in C.OPC_FIELDS}
    if opc.get("after_creation") != z:
        fails.append(f"operation counters after creation {opc.get('after_creation')} != {z}")
    e = dict(cross=pred["cross"], cross_match=pred["cross_match"],
             cross_reads=0 if impl in C.M_IMPLS else pred["cross_reads_V"],
             **(sz["tau_counts"] if impl in C.M_IMPLS else dict(tau_found=0, tau_none=0, tau_probes=0)))
    if opc.get("end") != e:
        fails.append(f"operation counters {opc.get('end')} != predicted {e}")
    if dump is not None:
        for r in C.ROLES:
            if hashlib.sha256(dump.get(f"tab{r}", b"")).hexdigest() != sz["tables"][r]:
                fails.append(f"{r}'s table differs from the predicted one, slot by slot")
            if hashlib.sha256(dump.get(f"cel{r}", b"")).hexdigest() != sz["cells"][r]:
                fails.append(f"{r}'s handle fields differ from the predicted ones")
        if impl in C.M_IMPLS and hashlib.sha256(dump.get("tau", b"")).hexdigest() != sz["tau"]:
            fails.append("tau differs from the predicted one, position by position")
        if impl not in C.M_IMPLS and "tau" in dump:
            fails.append("a V row dumped a tau")
    return fails


def gcd_work(lines):
    """G: the gcd work of entry and the operation (rule-m1 item 7), from a counting run's lines."""
    return sum(v for (ph, n), v in parse_counts(lines).items() if n == "gcd_work" and ph in C.PHASES)


def arith_counts(lines):
    """every counter but the K-Q3 reads (the M rows' own), per phase: equal between the rows (spec §10 items 3-6)"""
    return {k: v for k, v in parse_counts(lines).items() if k[1] != "reads"}


def store_split(lines):
    """per store, after creation and at the end: W and the found/new split (equal between the hash rows, §10 items 4-5)"""
    st = parse_stor(lines)
    return {f"{w}:{r}": {k: st[w][r][k] for k in ("W", "found", "new")} for w in st for r in st[w]}


# ------------------------------------------------------------------------------------------------- timing (round 3)
def parse_reps(lines, kind=("calib", "rep")):
    """[(index or r, total, creation, op, read, tau)] from calib or rep lines"""
    out = []
    for l in lines:
        t = l.split()
        if t and t[0] in kind:
            out.append(tuple(int(x) for x in t[1:7]))
    return out


def parse_rep_cpu(lines, kind=("calib", "rep")):
    """[(cpu_ns, span_ns) or None] from calib or rep lines, in the order of parse_reps (reported only)"""
    out = []
    for l in lines:
        t = l.split()
        if t and t[0] in kind:
            d = dict(x.split("=", 1) for x in t[7:] if "=" in x)
            out.append((int(d["cpu"]), int(d["span"])) if "cpu" in d and "span" in d else None)
    return out


def host_allowance(rate, seconds):
    """rule-m1 item 5 (carried over): over a block of L seconds a host process may use at most 5 + ceil(2 R_h L)"""
    return C.HOST_BASE_TICKS + math.ceil(C.HOST_FACTOR * rate * seconds)


def host_rates(checks):
    """rule-m1 item 5 (carried over): from the pilot's disturbance checks, ({pid: measured}, {pid: R_h}); R_h at least
    1 tick over the longest pilot block"""
    measured = {}
    for d in checks:
        for pid, h in d["hosts"].items():
            measured[pid] = max(measured.get(pid, 0.0), h["rate"])
    longest = max((d["seconds"] for d in checks if d.get("seconds")), default=None)
    floor = C.HOST_RATE_MIN_TICKS / longest if longest else 0.0
    return measured, {pid: max(r, floor) for pid, r in measured.items()}


def other_limit(seconds):
    """rule-m1 item 5 (carried over): 5 + ceil(1 x L) ticks for any other process over a block of L seconds"""
    return C.DIST_OTHER_TICKS + math.ceil(C.DIST_OTHER_RATE * seconds)


def runner_ticks_check(ticks, launches):
    """rule-m1 item 5 (carried over): the runner's ticks per launch, reported; more than 1 tick per launch plus 2 over a
    block is a harness bug. Returns (ticks per launch or None, over the limit)."""
    if ticks is None:
        return None, False
    per = ticks / launches if launches else None
    return per, ticks > C.RUNNER_TICKS_PER_LAUNCH_MAX * launches + C.RUNNER_TICKS_BLOCK_ALLOWANCE


def pilot_projection(elapsed_s, p_small_s, p_large_s, n_large, n_rest):
    """rule-m1, Run plan step 3: elapsed + 1.2 x 5 x (N_128 P_13 + N_rest P_6), in seconds"""
    return elapsed_s + C.PILOT_MARGIN * C.PILOT_SCALE * (n_large * p_large_s + n_rest * p_small_s)
