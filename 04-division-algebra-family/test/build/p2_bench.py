"""Phase two: the format against GMP and FLINT, one table per width, rows by operation.

  E  element by element (vectors over a shared denominator): add, multiply, dot product, exact division, sort,
     bit-length scan, growth chain, decimal text in and out
  S  single values: the packed format as a machine type (64 and 128 bits) against GMP and FLINT rationals
  L  linear algebra: matrix-vector, dense matrix product, sparse matrix-vector, linear solve (p2_linalg)

The format is what phases one and the threshold test settled: compile-time routines up to the threshold and
run-time limbs above it (THRESHOLD below, from results/threshold.md), the uniform header and the packed layout
for single values, and the IFMA kernel for matrix products. Every result is checked.
"""
import json
import sys
import time
from pathlib import Path

import numpy as np

import reuse  # noqa: F401
from run_benchmarks import metadata

import p1_arith_run as AR
import p2_elem_run as EL
import p2_scalar_run as SC

HERE = Path(__file__).resolve().parent
WIDTHS = (128, 256, 512, 1024, 2048)
CONTENDERS = {"fmt": "the format", "mpz": "GMP mpz_t, shared denominator", "mpq": "GMP mpq_t",
              "fmpz": "FLINT fmpz, shared denominator", "fmpq": "FLINT fmpq"}
INFO = {"bi": "the format, two's complement _BitInt (for information)"}
ROUTINE = {"lc": "compile-time limbs", "lr": "run-time limbs, GMP kernels", "bi": "_BitInt"}
TIE = 0.05
ROUNDS = 3
NAMES = {"add": "a + b", "mul": "a b", "dot": "dot product", "div1": "divide by 999 (exact)",
         "div2": "divide by 1000 (exact)", "sort": "sort", "scan": "bit-length scan (shared denominators only)",
         "chain": "growth chain to this width", "dec_in": "decimal text in", "dec_out": "decimal text out"}


def threshold_choice():
    """The format's candidate routines at each width. Up to the threshold the format is its whole small set: the
    compile-time routines (_BitInt and compile-time limbs) and the assembly kernels, which take the length as an
    argument and are shared with the large set (p1_arith's lr runs them); above it, run-time limbs only. Whether
    1024 is in the small set comes from the threshold test."""
    t = json.loads((HERE.parent / "results" / "threshold.json").read_text())
    small_1024 = t["decision"]["rep_at_1024"] == "lc"
    small = ["lc", "lr"]          # one layout (sign and magnitude); lr is the same layout running GMP's kernels
    return {128: small, 256: small, 512: small, 1024: small if small_1024 else ["lr"], 2048: ["lr"]}


def fmt_t(sec):
    for unit, f in (("s", 1), ("ms", 1e3), ("µs", 1e6), ("ns", 1e9)):
        if sec * f >= 1:
            return f"{sec * f:.3g} {unit}"
    return f"{sec * 1e9:.3g} ns"


def bench_elem(n, reps, choice, rounds):
    """Every row: the format (its routine chosen on a calibration run), the libraries, and _BitInt for information,
    each timed in `rounds` interleaved rounds (order rotated); the reported time is the median over rounds."""
    rows = []
    C = EL.ChainInputs(n, extremes=False)
    steps = AR.steps_for_widths(C, WIDTHS)
    for W in WIDTHS:
        I = EL.WidthInputs(W, n, seed=23)
        for op in EL.OPS + ("chain",):
            K = steps[W] if op == "chain" else None
            want = C.expected(K) if op == "chain" else I.expected(op)

            def timed(rep):
                if op == "chain":
                    med, mn, w, got = C.run(rep, K, max(3, reps // 2))
                else:
                    med, mn, got = I.run(rep, op, reps)
                    if got is None:
                        return None
                if got != want:
                    raise AssertionError(f"wrong result: E {W} {op} {rep}")
                return med
            cands = choice[W] if op not in ("dec_in", "dec_out") else ["lr"]     # decimal: the same limb code for both
            cal = {r: timed(r) for r in cands} if len(cands) > 1 else {cands[0]: 0}
            fmt_rep = min(cal, key=cal.get)
            who = {"fmt": fmt_rep, "mpz": "mpz", "mpq": "mpq", "fmpz": "fmpz", "fmpq": "fmpq"}
            if W <= 1024:
                who["bi"] = "bi"
            ts = {c: [] for c in who}
            order = list(who)
            for r in range(rounds):
                for c in order[r % len(order):] + order[:r % len(order)]:
                    v = timed(who[c])
                    if v is not None:
                        ts[c].append(v)
            for c, v in ts.items():
                if v:
                    rows.append(dict(width=W, op=op, contender=c, rep=who[c], median=float(np.median(v)),
                                     rounds=v, steps=K))
    return rows


def bench_scalar(n, reps, rounds):
    """add on operands of a third of the data bits (sums need three operand sizes); mul and sort also on operands of
    half the data bits (products need two)."""
    rows = []
    for W in (64, 128):
        for fill, ops in (("third", ("add", "mul", "sort")), ("half", ("mul", "sort"))):
            I = SC.Inputs(W, n, seed=29, fill=fill)
            for op in ops:
                want = I.expected(op)
                ts = {rep: [] for rep in SC.REPS}
                order = list(SC.REPS)
                for r in range(rounds):
                    for rep in order[r % 3:] + order[:r % 3]:
                        med, mn, fails, got = I.run(rep, op, reps)
                        if fails or got != want:
                            raise AssertionError(f"wrong result: S {W} {op} {rep}")
                        ts[rep].append(med)
                for rep, v in ts.items():
                    rows.append(dict(width=W, fill=fill, op=op, contender=rep, median=float(np.median(v)), rounds=v))
    return rows


def elem_markdown(rows, n, choice):
    out = []
    for W in WIDTHS:
        rs = [r for r in rows if r["width"] == W]
        info = W <= 1024
        out += [f"### {W} bits", "", "The format: sign and magnitude limbs, " +
                ("compile-time or run-time routines (GMP's kernels), whichever was faster on a calibration run (noted)" if len(choice[W]) > 1
                 else "run-time limbs with GMP's kernels") +
                f". Times: median over {ROUNDS} interleaved rounds. Ratio = best library / the format (above 1: the format "
                f"is faster). Bold: the fastest, unless another is within {TIE:.0%} (a tie).", "",
                "| Operation | " + " | ".join(CONTENDERS.values()) + " | Best library / the format |" + (f" {INFO['bi']} |" if info else ""),
                "|---|" + "---|" * (len(CONTENDERS) + 1 + info)]
        for op in EL.OPS + ("chain",):
            t = {r["contender"]: r["median"] for r in rs if r["op"] == op}
            if not t:
                continue
            main = {k: v for k, v in t.items() if k in CONTENDERS}
            srt = sorted(main.values())
            clear = len(srt) < 2 or srt[1] > srt[0] * (1 + TIE)
            libs = [v for k, v in main.items() if k != "fmt"]
            name = NAMES[op] + (f" ({[r for r in rs if r['op'] == op][0]['steps']} steps)" if op == "chain" else "")
            cells = [("**" + fmt_t(main[c]) + "**" if clear and main[c] == srt[0] else fmt_t(main[c])) if c in main else "—" for c in CONTENDERS]
            used = [r["rep"] for r in rs if r["op"] == op and r["contender"] == "fmt"][0]
            cells[0] += f" ({'limbs' if op in ('dec_in', 'dec_out') else ROUTINE[used]})"
            extra = (f" {fmt_t(t['bi'])} |" if "bi" in t else " — |") if info else ""
            out.append(f"| {name} | " + " | ".join(cells) + f" | {min(libs) / main['fmt']:.2f} |" + extra)
        out.append("")
    return "\n".join(out)


def scalar_markdown(rows, n):
    out = [f"$n = {n}$ independent values per operation, each with its own denominator; median over {ROUNDS} rounds. "
           "Operand sizes: a third of the data bits (17 bits at 64, 38 at 128), so that sums fit; and half (26 and 57 "
           "bits) for products and sorting.", "",
           "| Size | Operands | Operation | " + " | ".join(SC.REPS.values()) + " | Best library / the format |",
           "|---|---|---|" + "---|" * (len(SC.REPS) + 1)]
    for W in (64, 128):
        for fill in ("third", "half"):
            for op in ("add", "mul", "sort"):
                t = {r["contender"]: r["median"] for r in rows if r["width"] == W and r["op"] == op and r["fill"] == fill}
                if t:
                    out.append(f"| {W} | {fill} | {op} | " + " | ".join(fmt_t(t[c]) for c in SC.REPS) +
                               f" | {min(t['mpq'], t['fmpq']) / t['fmt']:.2f} |")
    return "\n".join(out)


def main():
    quick = "--quick" in sys.argv
    EL.build()
    SC.build()
    choice = threshold_choice()
    global ROUNDS
    n, reps, sn, sreps = (3000, 3, 20000, 3) if quick else (100000, 5, 1000000, 5)
    ROUNDS = 2 if quick else 3
    meta = metadata()
    meta["compiled"] = "p2_elem, p2_scalar: clang 18 -O3 -march=native; GMP 6.3.0, FLINT 3.0.1"
    res = {}
    for key, fn in (("E", lambda: bench_elem(n, reps, choice, ROUNDS)), ("S", lambda: bench_scalar(sn, sreps, ROUNDS))):
        t = time.perf_counter()
        res[key] = fn()
        print(f"  done  {key}  ({time.perf_counter() - t:.1f} s)", flush=True)
    linalg_md = ""
    try:
        import p2_linalg_bench as LB
        t = time.perf_counter()
        res["L"], linalg_md = LB.run(quick)
        print(f"  done  L  ({time.perf_counter() - t:.1f} s)", flush=True)
    except ImportError:
        pass
    header = "\n".join(f"- **{k}**: {v}" for k, v in meta.items())
    text = (f"# Phase two: the format against GMP and FLINT\n\n{header}\n\n"
            f"## E. Element by element, $n = {n}$, by width\n\n{elem_markdown(res['E'], n, choice)}\n"
            f"## S. Single values\n\n{scalar_markdown(res['S'], sn)}\n\n"
            + (f"## L. Linear algebra\n\n{linalg_md}\n" if linalg_md else ""))
    out = HERE.parent / "results"
    suffix = "-quick" if quick else ""
    (out / f"phase2{suffix}.md").write_text(text)
    (out / f"phase2{suffix}.json").write_text(json.dumps({"metadata": meta, "choice": choice, **res}, indent=1, default=str))
    print(text)


if __name__ == "__main__":
    main()
