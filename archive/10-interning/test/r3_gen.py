"""Round 3's workloads, references and predicted figures (rule-r3, Workloads; spec v0.3 §12.1 item 2). From round
2's r2_gen.py (unchanged).

    python3 r3_gen.py OUTDIR [--jobs 1,3] [--toy]

The workloads are round 1's and round 2's: gen.py is called unchanged (gen.gen_job, master seed 20261029, job n seeded
20261029 + n), so every workload file is byte-identical to round 1's; outside toy mode each is checked by sha256
against round 1's pinned state.json (r3_common.ROUND1_STATE). gen.py also writes its own round-1 files (job-NN.case,
job-NN.json), which round 3 does not use. For every job within capacity, the model of spec v0.3 (r3_model.py) then
runs over the calls; its values are checked against the exact reference, and its arithmetic counters against round
2's model (r2_model.py: v0.3 keeps round 2's arithmetic). This writes:
    OUTDIR/job-NN.case2     the case of v0.2 §9 of every call (int8; -1 for an entry)
    OUTDIR/job-NN.r3.json   the predicted figures (W after entry and at the end; every counter per phase, with
                            Q32-index's entries examined and Q32-hash's probes and longest store; the storage totals
                            after entry and at the end; the sha256 of Q32-index's list and of Q32-hash's table; the
                            table's fill), the realized reuse (rule-r3 item 13, as rule-r2) and the round-1 workload
                            check
    OUTDIR/gen.json         a summary of every job (constructible, over capacity, workload checks)
The runner calls this as its own process, so that the runner never imports numpy (test/DECISIONS.md R2-H3).
"""
import argparse
import hashlib
import json
import struct
import sys
from collections import Counter
from pathlib import Path

import common as C1          # round 1's formats (read only)
import gen                    # round 1's generator, unchanged
import r2_model
import r3_common as C
import r3_model


def reuse(ops, a, b, vals, n_in, cases):
    """Realized reuse (rule-r3 item 13, as rule-r2): N, W, W/N, 2N/W after entry and at the end, the case shares per algorithm,
    and the share of operands that are earlier results."""
    def wholes(vs):
        s = {0, 1}                           # stored by create in both Q32 rows (spec v0.3 §17.2), counted in W
        for m, d in vs:
            s.add(abs(m))
            s.add(d)
        return len(s)
    N_e, N = n_in, len(ops)
    W_e, W = wholes(vals[:n_in]), wholes(vals)
    share = {}
    for code, ph in ((1, "add"), (2, "sub"), (3, "mul"), (4, "div")):
        idx = [i for i, o in enumerate(ops) if o == code]
        if idx:
            cnt = Counter(C.CASES[cases[i]] for i in idx)
            share[ph] = {k: cnt.get(k, 0) / len(idx) for k in C.CASES_OF[ph]}
    total = res = 0
    for i, o in enumerate(ops):
        if o:
            for h in (a[i], b[i]):
                total += 1
                res += ops[h] != 0
    return {"after_entry": {"N": N_e, "W": W_e, "W_per_N": W_e / N_e, "places_per_whole": 2 * N_e / W_e},
            "end": {"N": N, "W": W, "W_per_N": W / N, "places_per_whole": 2 * N / W},
            "case_share": share,
            "operands": {"operands": total, "result_operands": res, "result_operand_share": res / total if total else None}}


def gen_job(job, out, toy, round1):
    meta1 = gen.gen_job(job, out, toy)                   # round 1's generator, unchanged
    wl = out / f"job-{job:02d}.wl"
    got = C.sha256(wl)
    m = {"job": job, "name": C.JOB_NAMES[job], "toy": toy, "workload_sha256": got}
    if round1 is not None:
        m["round1_sha256"] = round1.get(str(job))
        m["identical_to_round1"] = got == round1.get(str(job))
    m["W_end_round1_model"] = meta1["predicted"]["W_end"]
    if meta1["predicted"]["W_end"] > C.Q32_CAPACITY:
        m["over_capacity"] = True
        return m
    w = C1.read_workload(wl)
    ops, a, b = [int(x) for x in w["op"]], [int(x) for x in w["a"]], [int(x) for x in w["b"]]
    rn, rd = C1.read_pairs(out / f"job-{job:02d}.ref")
    p = r3_model.predict(ops, a, b)
    vals, cases, We, Wend, cnt = p["values"], p["cases"], p["W_after_entry"], p["W_end"], p["counters"]
    ref = list(zip((int(x) for x in rn), (int(x) for x in rd)))
    assert vals == ref, f"job {job}: the model's values differ from the exact reference"
    # The amendment of Oct 29, 2026 (spec v0.3 §17) changes the cases and the arithmetic from round 2's, so round 2's
    # model is no longer an equal check. What stays: the values (above), the whole numbers (round 2's set plus 0 and
    # 1, the list in arrival order after 0 and 1), and every operation outside the new cases taking round 2's case.
    v2, cases2, _, _, _, Wl2 = r2_model.predict(ops, a, b)
    assert v2 == vals, f"job {job}: the two models' values differ"
    assert p["list"][:2] == [0, 1] and set(p["list"]) == set(Wl2) | {0, 1}, f"job {job}: whole numbers differ"
    assert [v for v in p["list"][2:] if v not in (0, 1)] == [v for v in Wl2 if v not in (0, 1)], \
        f"job {job}: the arrival order differs from round 2's"
    new = {C.CASE[k] for k in C.CASES[21:]}
    import r2_common
    for i, (k3, k2) in enumerate(zip(cases, cases2)):
        if k3 >= 0 and k3 not in new:
            assert r2_common.CASES[k2] == C.CASES[k3], f"job {job}: call {i} takes {C.CASES[k3]}, round 2 {r2_common.CASES[k2]}"
    n_in = w["n_in"]
    r = reuse(ops, a, b, ref, n_in, cases)
    assert r["after_entry"]["W"] == We and r["end"]["W"] == Wend   # spec §12.1 item 2: W from the reference alone
    assert sum(1 for v in p["table"] if v >= 0) == Wend
    phase_counts = Counter(C.OPNAME[o] for o in ops)
    m.update(over_capacity=False, n=len(ops), n_in=n_in,
             predicted={"W_after_entry": We, "W_end": Wend, "counters": cnt,
                        "W_sha256": hashlib.sha256(struct.pack(f"<{len(p['list'])}q", *p["list"])).hexdigest(),
                        "T_sha256": hashlib.sha256(struct.pack(f"<{C.SLOTS}q", *p["table"])).hexdigest(),
                        "stor_after_entry": p["stor_after_entry"], "stor_end": p["stor_end"],
                        "fill_after_entry": We / C.SLOTS, "fill_end": Wend / C.SLOTS},
             reuse=r, phase_counts={("entry" if k == "enter" else k): v for k, v in phase_counts.items()},
             mix=meta1.get("mix"), H_max_bits=meta1.get("H_max_bits"))
    (out / f"job-{job:02d}.case2").write_bytes(bytes((c & 0xFF) for c in cases))
    return m


def generate(out, jobs, toy=False):
    out = Path(out)
    out.mkdir(parents=True, exist_ok=True)
    round1 = None
    if not toy:
        round1 = json.loads(C.ROUND1_STATE.read_text())["workload_sha256"]
    summary = {}
    for job in sorted(jobs):
        m = gen_job(job, out, toy, round1)
        (out / f"job-{job:02d}.r3.json").write_text(json.dumps(m, indent=1))
        summary[str(job)] = {k: v for k, v in m.items() if k not in ("predicted", "reuse")}
    (out / "gen.json").write_text(json.dumps(summary, indent=1))
    return summary


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--jobs", default="")
    ap.add_argument("--toy", action="store_true")
    a = ap.parse_args()
    jobs = [int(x) for x in a.jobs.split(",")] if a.jobs else list(C.ALL_JOBS)
    s = generate(a.out, jobs, a.toy)
    bad = [j for j, m in s.items() if m.get("identical_to_round1") is False]
    for j, m in s.items():
        print(j, m["name"], "over capacity" if m.get("over_capacity") else "ok",
              "" if a.toy else ("identical to round 1" if m.get("identical_to_round1") else "DIFFERS FROM ROUND 1"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
