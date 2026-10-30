"""The checks of round m2's harness that compare a run's output with exact figures (spec v0.2 §15.7; rule-m2 items 3 and
12): parsing the driver's lines, the gate's exact checks against the model's prediction, the per-operation records;
and m1's helpers, used unchanged (the read-out check, the timed blocks' limits, the pilot's projection: rule-m2 items
4-5 and the Run plan carry them over). Pure Python, no numpy.
"""
import hashlib

import m2_common as C
from m1_checks import (host_allowance, host_rates, kv, other_limit, pilot_projection,  # noqa: F401  (m1's, unchanged)
                       readout_check, runner_ticks_check)


def parse_counts(lines):
    """{(matrix, phase, name): total over core and store} from ctr and xctr lines (one matrix each)"""
    tot = {}
    for l in lines:
        t = l.split()
        if t and t[0] in ("ctr", "xctr"):
            k = (t[1], t[2], t[4])
            tot[k] = tot.get(k, 0) + int(t[5])
    return tot


def parse_stor(lines):
    """{when: {matrix: {field: value}}} from stor lines (when: built, end)"""
    out = {}
    for l in lines:
        t = l.split()
        if t and t[0] == "stor":
            out.setdefault(t[1], {})[t[2]] = kv(t[3:])
    return out


def parse_opc(lines):
    """{result: {field: value}} from opc lines (each step's own counters)"""
    return {l.split()[1]: kv(l.split()[2:]) for l in lines if l.startswith("opc ")}


def parse_events(lines, kind):
    """[{name, fields...}] of growth or pack lines, in their order"""
    out = []
    for l in lines:
        t = l.split()
        if t and t[0] == kind:
            g = kv(t[2:])
            g["name"] = t[1]
            out.append(g)
    return out


def parse_growths(lines):
    return parse_events(lines, "growth")


def parse_packs(lines):
    return parse_events(lines, "pack")


def parse_memrun(lines):
    """{when: {field: value}} from the memory run's memrun lines (built_<X>, after_creation, op, end, growth)"""
    out = {}
    for l in lines:
        t = l.split()
        if t and t[0] == "memrun":
            out[t[1]] = kv(t[2:])
    return out


def exact_checks(impl, op, lines, pred, dump=None):
    """Spec v0.2 §15.7 item 2 on a gate (test build) or counting run: every counter of every matrix per phase equal to
    the model's (an input's entry, a step's arithmetic); for the hash rows every store's figures after its build and at
    its end, every growth and packing in order, each step's cross tests, matches and cross reads; with a gate's dump,
    each table (or list) slot by slot, each matrix's handle fields and (Hash-V-pack) each build table (h), equal to
    the model's and so to Hash-V-fixed's table. Returns a list of failures."""
    fails = []
    tot = parse_counts(lines)
    names = C.SET_MATRICES[op]
    for name in names:
        want = pred["counters"].get(name, {})
        for ph in C.PHASES:
            w = want.get(ph, {})
            keys = set(w) | {k for (m, p, k) in tot if m == name and p == ph}
            for k in sorted(keys):
                g, e = tot.get((name, ph, k), 0), w.get(k, 0)
                if k == "ev_overflows":
                    e = 0
                if g != e:
                    fails.append(f"{name} {ph} {k}: {g} != predicted {e}")
    if {m for (m, _, _) in tot} - set(names):
        fails.append(f"counters of an unknown matrix: {sorted({m for (m, _, _) in tot} - set(names))}")
    if impl == "TC":
        if parse_stor(lines) or parse_growths(lines) or parse_packs(lines):
            fails.append("TC printed store figures")
        return fails
    sz = pred["sizings"][C.SIZING[impl]]
    st = parse_stor(lines)
    for when, key, which in (("built", "stor_built", C.SET_INPUTS[op]), ("end", "stor_end", names)):
        for r in which:
            got = st.get(when, {}).get(r)
            if got is None:
                fails.append(f"no store figures for {r} {when}")
                continue
            for f in C.STORE_FIELDS:
                if got.get(f) != sz[key][r][f]:
                    fails.append(f"store {r} {when} {f}: {got.get(f)} != predicted {sz[key][r][f]}")
    if parse_growths(lines) != sz["growths"]:
        fails.append(f"growths differ from the predicted ones ({len(parse_growths(lines))} against "
                     f"{len(sz['growths'])})")
    if parse_packs(lines) != sz["packs"]:
        fails.append(f"packings differ from the predicted ones ({len(parse_packs(lines))} against {len(sz['packs'])})")
    opc = parse_opc(lines)
    for z, s in pred["steps"].items():
        e = {f: s[f] for f in C.OPC_FIELDS}
        if opc.get(z) != e:
            fails.append(f"step {z}'s operation counters {opc.get(z)} != predicted {e}")
    if dump is not None:
        for r in names:
            if hashlib.sha256(dump.get(f"tab{r}", b"")).hexdigest() != sz["tables"][r]:
                fails.append(f"{r}'s table (or list) differs from the predicted one, slot by slot")
            if hashlib.sha256(dump.get(f"cel{r}", b"")).hexdigest() != sz["cells"][r]:
                fails.append(f"{r}'s handle fields differ from the predicted ones")
            if impl == "MHVP":
                if hashlib.sha256(dump.get(f"hb{r}", b"")).hexdigest() != sz["hb"][r]:
                    fails.append(f"{r}'s build table (h) differs from Hash-V-fixed's table, slot by slot")
            elif f"hb{r}" in dump:
                fails.append(f"{r}: a build table dumped by a row that does not pack")
    return fails


def gcd_work(lines):
    """G: the gcd work of every matrix (entry and the steps), from a counting run's lines (rule-m2 item 7)."""
    return sum(v for (m, ph, n), v in parse_counts(lines).items() if n == "gcd_work" and ph in C.PHASES)


def arith_counts(lines):
    """every arithmetic counter of every matrix, per phase: equal between the rows (spec v0.2 §15.7 items 3-4)"""
    return parse_counts(lines)


def store_split(lines):
    """per store, after its build and at its end: W and the found/new split (equal between the hash rows, §15.7 item 4)"""
    st = parse_stor(lines)
    return {f"{w}:{r}": {k: st[w][r][k] for k in ("W", "found", "new")} for w in st for r in st[w]}


# ------------------------------------------------------------------------------------------------- timing
def parse_reps(lines, kind=("calib", "rep")):
    """[(index or r, total, op, read, (builds...))] from calib or rep lines"""
    out = []
    for l in lines:
        t = l.split()
        if t and t[0] in kind:
            d = dict(x.split("=", 1) for x in t[5:] if "=" in x)
            out.append((int(t[1]), int(t[2]), int(t[3]), int(t[4]), tuple(int(v) for v in d["builds"].split(","))))
    return out


def parse_rep_cpu(lines, kind=("calib", "rep")):
    """[(cpu_ns, span_ns) or None] from calib or rep lines, in the order of parse_reps (reported only)"""
    out = []
    for l in lines:
        t = l.split()
        if t and t[0] in kind:
            d = dict(x.split("=", 1) for x in t[5:] if "=" in x)
            out.append((int(d["cpu"]), int(d["span"])) if "cpu" in d and "span" in d else None)
    return out


def kept_minimum(reps, r):
    """rule-m2 item 4: of the reps kept (all if one, else reps 2 and 3), each timed quantity its own minimum, over r:
    (total, op, read, builds)"""
    kept = reps if len(reps) == 1 else reps[1:]
    nb = len(kept[0][4])
    return (min(x[1] for x in kept) / r, min(x[2] for x in kept) / r, min(x[3] for x in kept) / r,
            tuple(min(x[4][k] for x in kept) / r for k in range(nb)))
