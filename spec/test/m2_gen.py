"""Round m2's workloads, exact references and predicted figures (rule-m2, Workloads and Run plan step 0; spec v0.2
§15.5 item 7, §15.7 items 1-2). New in this round.

    python3 m2_gen.py OUTDIR --op add|mul|sop|pos [--jobs 1,6] [--toy]

add and mul: carried over unchanged from rule-m1: m1_gen.py's own functions draw the job (seeds 20261029 + j and
20261029 + 100 + j) and write the workload and the reference, so that step 0's files equal m1's by sha256 (rule-m2,
Workloads). sop and pos (new): A, B, C, D each drawn as m1 draws A and B (m1_gen.draw_matrix, unchanged: numerators
uniform in [-32767, 32767], denominators among the 30 divisors of 720, s = n^2 / rho distinct values, each matrix its
own pool), from one random.Random stream per job, in the order A, B, C, D; job j seeded 20261029 + 200 + j (sop) or
20261029 + 300 + j (pos). This writes:
    OUTDIR/job-NN.wl          the workload (m2_common.write_workload)
    OUTDIR/job-NN.ref         the result by Python's fractions, row-major (the read-out's format)
    OUTDIR/job-NN.<X>.ref     sop, pos: each intermediate X by Python's fractions (the gate checks them)
    OUTDIR/job-NN.m2.json     the model's predicted figures (m2_model.predict), and the realized sharing
    OUTDIR/gen.json           every job's summary, with each file's sha256 (Run plan step 0)
The runner calls this as its own process, so that the runner never imports more than it needs.
"""
import argparse
import json
import random
import sys
from fractions import Fraction
from pathlib import Path

import m1_gen
import m2_common as C
import m2_model


def workload(op, job, toy=False):
    """(n, s, [the inputs' pairs as drawn])"""
    if op in ("add", "mul"):
        n, s, A, B = m1_gen.workload(op, job, toy)         # m1's, unchanged (seeds included)
        return n, s, [A, B]
    n, s = C.job_n(job, toy), C.job_s(job, toy)
    rng = random.Random(C.job_seed(op, job))
    return n, s, [m1_gen.draw_matrix(rng, n, s) for _ in C.SET_INPUTS[op]]


def reference(op, n, inputs):
    """every matrix's exact value by Python's fractions, {name: [(num, den)]}, independent of the model"""
    F = {name: [Fraction(m, d) for m, d in X] for name, X in zip(C.SET_INPUTS[op], inputs)}
    for kind, x, y, z in C.SET_STEPS[op]:
        if kind == "add":
            F[z] = [a + b for a, b in zip(F[x], F[y])]
        else:
            out = []
            for i in range(n):
                row = F[x][i * n:(i + 1) * n]
                for j in range(n):
                    s = Fraction(0)
                    for k in range(n):
                        s += row[k] * F[y][k * n + j]
                    out.append(s)
            F[z] = out
    return {k: [(v.numerator, v.denominator) for v in X] for k, X in F.items()}


def gen_job(op, job, out, toy):
    n, s, inputs = workload(op, job, toy)
    wl, ref, js = out / f"job-{job:02d}.wl", out / f"job-{job:02d}.ref", out / f"job-{job:02d}.m2.json"
    C.write_workload(wl, op, n, job, inputs)
    want = reference(op, n, inputs)
    res = C.SET_RESULT[op]
    C.write_pairs(ref, want[res])
    files = [wl, ref, js]
    for x in C.SET_INTERMEDIATES[op]:
        f = out / f"job-{job:02d}.{x}.ref"
        C.write_pairs(f, want[x])
        files.append(f)
    p = m2_model.predict(op, n, inputs)
    for k in C.SET_MATRICES[op]:
        if k in C.SET_INPUTS[op]:
            continue
        assert [tuple(v) for v in p["values"][k]] == want[k], f"{op} job {job}: the model's {k} differs from fractions"
        for v in want[k]:                                  # every value fits 64 bits (v0.2 §15.5 item 8)
            assert abs(v[0]) <= m2_model.LIM and v[1] <= m2_model.LIM
    realized = {"s": s, "distinct": {k: len(set(p["values"][k])) for k in C.SET_MATRICES[op]},
                "W": {k: p["sizings"]["fixed"]["stor_end"][k]["W"] for k in C.SET_MATRICES[op]}}
    for k in C.SET_INPUTS[op]:
        assert realized["distinct"][k] == s
    meta = {"op": op, "job": job, "name": C.job_name(op, job), "n": n, "rho": C.job_rho(job), "s": s, "toy": toy,
            "seed": C.job_seed(op, job), "nops": p["nops"], "realized": realized,
            "predicted": {k: v for k, v in p.items() if k != "values"}}
    js.write_text(json.dumps(meta, indent=0))
    return {"op": op, "job": job, "name": meta["name"], "n": n, "rho": meta["rho"], "s": s, "nops": p["nops"],
            "toy": toy, "sha256": {f.name: C.sha256(f) for f in files}, "realized": realized, "ncase": p["ncase"],
            "steps": p["steps"],
            "packs": {k: {f: x[f] for f in ("W", "whb", "whp", "bytes_list", "bytes_map", "rewritten")}
                      for x in p["sizings"]["pack"]["packs"] for k in (x["name"],)}}


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
    jobs = [int(x) for x in a.jobs.split(",")] if a.jobs else list(C.JOBS)
    s = generate(a.out, a.op, jobs, a.toy)
    for j, m in s.items():
        print(m["name"], f"n={m['n']} rho={m['rho']} s={m['s']} ops={m['nops']}", "ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
