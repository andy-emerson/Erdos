"""The checks of round m3's harness that compare a run's output with exact figures (spec v0.3 §16.9; rule-m3 items 3, 7
and 12): parsing the driver's lines (m2's, and the factored rows' fac, ffig and facc lines), the gate's exact checks
against the model's prediction, the adapted call-for-call check, and the pilot's projection of rule-m3 (new). m2's
checks for TC and Hash-V-pack and m1's helpers are used unchanged (the read-out check, the timed blocks' limits).
Pure Python, no numpy.
"""
import hashlib
import struct
from pathlib import Path

import m2_checks as K2
import m3_common as C
from m1_checks import host_allowance, host_rates, kv, readout_check, runner_ticks_check  # noqa: F401  (m1's)
from m2_checks import (arith_counts, gcd_work, kept_minimum, parse_counts, parse_memrun, parse_opc,  # noqa: F401
                       parse_packs, parse_rep_cpu, parse_reps, parse_stor, store_split)


def parse_fac(lines):
    """[{name, fields...}] of the factorings (fac lines), in their order"""
    return K2.parse_events(lines, "fac")


def parse_ffig(lines):
    """{when: {matrix: {field: value}}} from ffig lines (when: built, end)"""
    out = {}
    for l in lines:
        t = l.split()
        if t and t[0] == "ffig":
            out.setdefault(t[1], {})[t[2]] = kv(t[3:])
    return out


def parse_facc(lines):
    """{result: {field: value}} from facc lines (each step's factored arithmetic)"""
    return {l.split()[1]: kv(l.split()[2:]) for l in lines if l.startswith("facc ")}


ROW_OF = {"MHPP": "primes", "MHPC": "coprime"}


def counters_check(names, want_of, lines):
    fails = []
    tot = parse_counts(lines)
    for name in names:
        want = want_of.get(name, {})
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
    return fails


def exact_checks(impl, op, lines, pred, dump=None, test=False):
    """Spec v0.3 §16.9 item 2 on a gate (test build; test=True) or counting run. TC and Hash-V-pack: m2's exact checks,
    unchanged (every counter, every store, packing and step's cross tests; with a gate's dump, every list, handle field
    and build table (h)). The factored rows: every counter of every matrix per phase equal to the model's (TC's, the
    operations' gcds and gcd work as the row obtains them); every store figure, packing and step's cross tests equal to
    Hash-V-pack's (the same (h) and (l), §16.4 item 1); every factoring's counts in order; every matrix's (f) and (q)
    figures after its build and at its end; each step's factored arithmetic (the obtained gcds checked against Euclid:
    the test build's count, 0 in the counting build); with a gate's dump, every list, handle field and build table (h)
    equal to Hash-V-pack's, and every matrix's (f) and (q) entry for entry. Returns a list of failures."""
    if impl not in ROW_OF:
        return K2.exact_checks(impl, op, lines, pred, dump)
    f = pred["factored"][ROW_OF[impl]]
    names = C.SET_MATRICES[op]
    fails = counters_check(names, f["counters"], lines)
    sz = pred["sizings"]["pack"]
    st = parse_stor(lines)
    for when, key, which in (("built", "stor_built", C.SET_INPUTS[op]), ("end", "stor_end", names)):
        for r in which:
            got = st.get(when, {}).get(r)
            if got is None:
                fails.append(f"no store figures for {r} {when}")
                continue
            for k in C.STORE_FIELDS:
                if got.get(k) != sz[key][r][k]:
                    fails.append(f"store {r} {when} {k}: {got.get(k)} != Hash-V-pack's predicted {sz[key][r][k]}")
    if K2.parse_growths(lines):
        fails.append("a factored row printed a growth")
    if parse_packs(lines) != sz["packs"]:
        fails.append(f"packings differ from Hash-V-pack's predicted ones ({len(parse_packs(lines))} against "
                     f"{len(sz['packs'])})")
    opc = parse_opc(lines)
    for z, s in pred["steps"].items():
        e = {k: s[k] for k in C.OPC_FIELDS}
        if opc.get(z) != e:
            fails.append(f"step {z}'s operation counters {opc.get(z)} != predicted {e}")
    got = parse_fac(lines)
    if got != f["factorings"]:
        bad = next((i for i, (a, b) in enumerate(zip(got, f["factorings"])) if a != b), min(len(got),
                                                                                             len(f["factorings"])))
        fails.append(f"factorings differ from the predicted ones ({len(got)} against {len(f['factorings'])}; first at "
                     f"{bad}: {got[bad] if bad < len(got) else None} != {f['factorings'][bad] if bad < len(f['factorings']) else None})")
    ff = parse_ffig(lines)
    for when, key, which in (("built", "ffig_built", C.SET_INPUTS[op]), ("end", "ffig_end", names)):
        for r in which:
            if ff.get(when, {}).get(r) != f[key][r]:
                fails.append(f"(f) and (q) figures of {r} {when}: {ff.get(when, {}).get(r)} != predicted {f[key][r]}")
    fa = parse_facc(lines)
    for z, want in f["facc"].items():
        want = dict(want, gchecked=want["gchecked"] if test else 0)
        if fa.get(z) != want:
            diff = {k: (fa.get(z, {}).get(k), v) for k, v in want.items() if fa.get(z, {}).get(k) != v}
            fails.append(f"step {z}'s factored arithmetic differs from the predicted (got, predicted): {diff}")
        if fa.get(z, {}).get("zero_reads"):
            fails.append(f"step {z}: a read of 0's factorization")
    if set(fa) != set(f["facc"]):
        fails.append(f"factored-arithmetic lines of {sorted(fa)}, predicted {sorted(f['facc'])}")
    if dump is not None:
        for r in names:
            for sec, want, what in ((f"tab{r}", sz["tables"][r], "list (l), against Hash-V-pack's"),
                                    (f"cel{r}", sz["cells"][r], "handle fields, against Hash-V-pack's"),
                                    (f"hb{r}", sz["hb"][r], "build table (h), against Hash-V-pack's"),
                                    (f"ff{r}", f["dumps"][r]["ff"], "factorization list (f)"),
                                    (f"fq{r}", f["dumps"][r]["fq"], "coprime base (q)")):
                if hashlib.sha256(dump.get(sec, b"")).hexdigest() != want:
                    fails.append(f"{r}'s {what} differs from the predicted one, entry for entry")
    return fails


# ------------------------------------------------------------------------------------------------- call for call
def percall_core_sha(path):
    """the per-operation record with its gcds and gcd work cleared: the case, exact divisions, multiplications and
    additions of every operation (v0.3 §16.9 item 3, the adapted check: the gcds legitimately differ)"""
    raw = Path(path).read_bytes()
    n = len(raw) // 2
    v = list(struct.unpack(f"<{n}h", raw))
    for i in range(0, n, len(C.PERCALL_FIELDS)):
        v[i + 1] = v[i + 2] = 0
    return hashlib.sha256(struct.pack(f"<{n}h", *v)).hexdigest()


def gcd_work_by_phase(lines):
    """G per part (rule-m3 item 7: reported for the factored rows): {entry, add, mul} over every matrix"""
    out = {}
    for (m, ph, n), v in parse_counts(lines).items():
        if n == "gcd_work":
            out[ph] = out.get(ph, 0) + v
    return out


def op_counts_no_gcd(lines):
    """every arithmetic counter of every matrix, per phase, without the operations' gcds and gcd work (the factored
    rows against Hash-V-pack and TC: the same cases and core arithmetic; v0.3 §16.9 items 3-4)"""
    return {k: v for k, v in parse_counts(lines).items() if not (k[2] in ("gcd", "gcd_work") and k[1] != "entry")}


# ------------------------------------------------------------------------------------------------- the pilot
def pilot_projection(elapsed_s, blocks, n_of):
    """rule-m3, Run plan step 3: elapsed + 1.2 [ sum_c N_c (O_c + sum_rows (K_row,c + 6 P_row,c))
    + 1/2 sum_c N_c sum_rows 4 t_row,c ], in seconds. blocks: {c: {"O": s, "K": {row: s}, "P": {row: s}, "t": {row: s}}}
    with c the size class; n_of: {c: N_c}."""
    first = second = 0.0
    for c, b in blocks.items():
        N = n_of.get(c, 0)
        first += N * (b["O"] + sum(b["K"].get(r, 0.0) + C.PILOT_ROUND_SCALE * b["P"].get(r, 0.0)
                                   for r in set(b["K"]) | set(b["P"])))
        second += N * sum(C.PILOT_SIDE_RUNS * t for t in b["t"].values())
    return elapsed_s + C.PILOT_MARGIN * (first + second / C.PILOT_SIDE_PARALLEL)
