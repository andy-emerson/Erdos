"""Round m3's workloads, exact references and predicted figures (rule-m3, Workloads and Run plan step 0; spec v0.3 §16.9
items 1-2). From m2_gen.py (unchanged, m2's), whose drawing and references this imports and uses unchanged, so that every
job is the same job as in m1 or m2:

    python3 m3_gen.py OUTDIR --op add|mul|sop|pos [--jobs 1,6] [--toy]

add, mul: m1_gen's own draw (seeds 20261029 + j, + 100 + j); sop, pos: m2's (+ 200 + j, + 300 + j). The grid of rule-m3:
jobs 1-12 (n = 16, 32, 64), and sop's 9 (no rho = 16). This writes:
    OUTDIR/job-NN.wl          the workload (m2's format)
    OUTDIR/job-NN.ref         the result by Python's fractions, row-major (the read-out's format)
    OUTDIR/job-NN.<X>.ref     sop, pos: each intermediate X by Python's fractions (the gate checks them)
    OUTDIR/job-NN.m3.json     the model's predicted figures (m3_model.predict), and the realized sharing
    OUTDIR/gen.json           every job's summary, with each file's sha256 (Run plan step 0)
The runner calls this as its own process, so that the runner never imports more than it needs (sympy among them).
"""
import argparse
import json
import sys
from pathlib import Path

import m2_gen
import m3_common as C
import m3_model


def gen_job(op, job, out, toy):
    n, s, inputs = m2_gen.workload(op, job, toy)
    wl, ref, js = out / f"job-{job:02d}.wl", out / f"job-{job:02d}.ref", out / f"job-{job:02d}.m3.json"
    C.write_workload(wl, op, n, job, inputs)
    want = m2_gen.reference(op, n, inputs)
    res = C.SET_RESULT[op]
    C.write_pairs(ref, want[res])
    files = [wl, ref, js]
    for x in C.SET_INTERMEDIATES[op]:
        f = out / f"job-{job:02d}.{x}.ref"
        C.write_pairs(f, want[x])
        files.append(f)
    p = m3_model.predict(op, n, inputs)
    for k in C.SET_MATRICES[op]:
        if k in C.SET_INPUTS[op]:
            continue
        assert [tuple(v) for v in p["values"][k]] == want[k], f"{op} job {job}: the model's {k} differs from fractions"
        for v in want[k]:                                  # every value fits 64 bits (v0.2 §15.5 item 8)
            assert abs(v[0]) <= m3_model.LIM and v[1] <= m3_model.LIM
    pk = p["sizings"]["pack"]
    realized = {"s": s, "distinct": {k: len(set(p["values"][k])) for k in C.SET_MATRICES[op]},
                "W": {k: pk["stor_end"][k]["W"] for k in C.SET_MATRICES[op]},
                "E": {r: p["factored"][r]["E"] for r in ("primes", "coprime")},
                "nq": p["factored"]["coprime"]["nq"]}
    for k in C.SET_INPUTS[op]:
        assert realized["distinct"][k] == s
    meta = {"op": op, "job": job, "name": C.job_name(op, job), "n": n, "rho": C.job_rho(job), "s": s, "toy": toy,
            "seed": C.job_seed(op, job), "nops": p["nops"], "realized": realized,
            "predicted": {k: v for k, v in p.items() if k != "values"}}
    js.write_text(json.dumps(meta, indent=0))
    fac = {r: {"factorings": p["factored"][r]["factorings"], "facc": p["factored"][r]["facc"]}
           for r in ("primes", "coprime")}
    return {"op": op, "job": job, "name": meta["name"], "n": n, "rho": meta["rho"], "s": s, "nops": p["nops"],
            "toy": toy, "sha256": {f.name: C.sha256(f) for f in files}, "realized": realized, "ncase": p["ncase"],
            "steps": p["steps"],
            "packs": {k: {f: x[f] for f in ("W", "whb", "whp", "bytes_list", "bytes_map", "rewritten")}
                      for x in pk["packs"] for k in (x["name"],)}, "factored": fac}


def generate(out, op, jobs, toy=False):
    out = Path(out)
    bad = C.forbidden(out, op)
    if bad:
        raise SystemExit(f"refusing to write: {bad}")
    out.mkdir(parents=True, exist_ok=True)
    summary = {}
    for job in sorted(jobs):
        summary[str(job)] = gen_job(op, job, out, toy)
    (out / "gen.json").write_text(json.dumps(summary, indent=1))
    return summary


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--op", choices=C.OPS, required=True)
    ap.add_argument("--jobs", default="")
    ap.add_argument("--toy", action="store_true")
    a = ap.parse_args()
    jobs = [int(x) for x in a.jobs.split(",")] if a.jobs else list(C.JOBS_OF[a.op])
    bad = [j for j in jobs if j not in C.JOBS_OF[a.op]]
    if bad:
        raise SystemExit(f"jobs {bad} are not in the {a.op} set's grid (rule-m3, Workloads)")
    s = generate(a.out, a.op, jobs, a.toy)
    for j, m in s.items():
        print(m["name"], f"n={m['n']} rho={m['rho']} s={m['s']} ops={m['nops']}", "ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
