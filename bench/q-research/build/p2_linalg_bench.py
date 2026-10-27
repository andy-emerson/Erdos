"""Phase two, linear algebra: matrix-vector, dense matrix product, sparse matrix-vector and linear solve, the format
against GMP and FLINT (FLINT 3.0.1 built with BLAS; the install is p2_linalg_run.FLINTB, recorded in each row).
Every result is checked inside p2_linalg.c.

FLINT gets the same rule as the format: for each (op, n, width) every candidate routine in p2_linalg_run.METHODS
(fmpz_mat_mul / _blas / _multi_mod / _fft, the solve variants, ...) is run once for calibration, the fastest is
chosen, and the time reported comes from a separate run with that routine. A candidate that exceeds CAL_TIMEOUT
in calibration, or is unavailable in the FLINT build, is skipped (recorded). GMP has one routine per operation.

For matrix-vector and sparse matrix-vector up to the threshold, the format may use its compile-time multiply or the
assembly kernels (both belong to its small set): the faster is chosen on one run and the time reported from a
separate run. Above the threshold it uses run-time limbs. Its dense product is the IFMA kernel; its solve is the
multimodular Cramer method on IFMA residues.
"""
import json
import subprocess
from pathlib import Path

import p2_linalg_run as LR

HERE = Path(__file__).resolve().parent
NAMES = {"ours": "the format", "gmp_mpz": "GMP mpz_t, shared denominator", "gmp_mpq": "GMP mpq_t",
         "flint_fmpz": "FLINT fmpz_mat, shared denominator", "flint_fmpq": "FLINT fmpq_mat"}
OPNAMES = {"mv": "matrix-vector", "mm": "dense matrix product", "spmv": "sparse matrix-vector (1% nonzero)",
           "solve": "linear solve"}
SLOW = {("mm", 256, "gmp_mpq"), ("solve", 64, "gmp_mpq")}          # tens of seconds per run: omitted
CAL_TIMEOUT = 120                                                    # seconds, one calibration run of one routine


def threshold_small_at_1024():
    t = json.loads((HERE.parent / "results" / "threshold.json").read_text())
    return t["decision"]["rep_at_1024"] == "lc"


def fmt_t(sec):
    for unit, f in (("s", 1), ("ms", 1e3), ("µs", 1e6)):
        if sec * f >= 1:
            return f"{sec * f:.3g} {unit}"
    return f"{sec * 1e9:.3g} ns"


def run(quick=False):
    LR.build()
    small1024 = threshold_small_at_1024()
    widths = (128, 1024) if quick else LR.WIDTHS
    sizes = {"mv": (64,), "mm": (32,), "spmv": (500,), "solve": (8,)} if quick else LR.SIZES
    reps = 1 if quick else 5
    rows = []
    for op in LR.OPS:
        for n in sizes[op]:
            for W in widths:
                for c in LR.CONTENDERS:
                    if (op, n, c) in SLOW and not quick:
                        continue
                    if c == "ours" and op in ("mv", "spmv") and (W < 1024 or (W == 1024 and small1024)):
                        cal = {rt: LR.run(op, c, n, W, reps=reps, rt=rt)["median"] for rt in (False, True)}
                        rt = min(cal, key=cal.get)
                        r = LR.run(op, c, n, W, reps=reps, rt=rt)
                        r["routine"] = "assembly kernels" if rt else "compile-time limbs"
                        r["calibration"] = {("assembly kernels" if k else "compile-time limbs"): v for k, v in cal.items()}
                    elif c == "ours":
                        rt = op in ("mv", "spmv")             # above the threshold: run-time limbs, at 1024 too
                        r = LR.run(op, c, n, W, reps=reps, rt=rt)
                        r["routine"] = {"mv": "run-time limbs", "spmv": "run-time limbs", "mm": "IFMA kernel",
                                        "solve": "multimodular Cramer, IFMA"}[op]
                    elif (c, op) in LR.METHODS:
                        cal = {}
                        for m in LR.METHODS[(c, op)]:
                            try:
                                cal[m] = LR.run(op, c, n, W, reps=reps, method=m, timeout=CAL_TIMEOUT)["median"]
                            except subprocess.TimeoutExpired:
                                cal[m] = "timeout"             # too slow to be the best: skipped
                            except RuntimeError:
                                cal[m] = "unavailable"         # e.g. fmpz_mat_mul_blas in a FLINT without BLAS
                        done = {m: v for m, v in cal.items() if isinstance(v, float)}
                        m = min(done, key=done.get)
                        r = LR.run(op, c, n, W, reps=reps, method=m)
                        r["routine"] = m
                        r["calibration"] = cal
                    else:
                        r = LR.run(op, c, n, W, reps=reps)
                    r.update(op=op, n=n, width=W, contender=c, flint=str(LR.FLINTB))
                    rows.append(r)
    return rows, markdown(rows)


def markdown(rows):
    flint = sorted({r.get("flint", "") for r in rows})
    out = ["Ratio = best library / the format (above 1: the format is faster); peak memory is the rise during the "
           "run. GMP rationals are omitted where one run takes tens of seconds (dense product at 256, solve at 64). "
           "FLINT's routine is chosen per cell on a calibration run (listed under each table); FLINT install: "
           + ", ".join(flint) + ".", ""]
    for op in LR.OPS:
        rs = [r for r in rows if r["op"] == op]
        for n in sorted({r["n"] for r in rs}):
            out += [f"### {OPNAMES[op]}, n = {n}", "",
                    "| Width | " + " | ".join(NAMES.values()) + " | Best library / the format | Memory: the format / best library |",
                    "|---|" + "---|" * (len(NAMES) + 2)]
            for W in sorted({r["width"] for r in rs if r["n"] == n}):
                t = {r["contender"]: r for r in rs if r["n"] == n and r["width"] == W}
                libs = {k: v for k, v in t.items() if k != "ours"}
                bl = min(libs, key=lambda k: libs[k]["median"])
                best = min(v["median"] for v in t.values())
                cell = lambda k: "—" if k not in t else (("**" + fmt_t(t[k]["median"]) + "**") if t[k]["median"] == best else fmt_t(t[k]["median"]))
                o = t["ours"]
                out.append(f"| {W} | " + " | ".join(cell(k) for k in NAMES) +
                           f" | {libs[bl]['median'] / o['median']:.2f} | +{o['peak_mb']:.0f} / +{libs[bl]['peak_mb']:.0f} MB |")
            out.append("")
            out.append("Routines: " + "; ".join(
                f"{W}: " + ", ".join(f"{NAMES[r['contender']]} {r['routine']}" for r in rs
                                     if r["n"] == n and r["width"] == W and r.get("routine"))
                for W in sorted({r["width"] for r in rs if r["n"] == n})) + ".")
            out.append("")
    return "\n".join(out)
