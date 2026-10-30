"""Phase one: the variants of the format compared with each other only (no outside libraries as contenders).

  S1  scalar layouts: packed power of two vs header byte + word; fitted vs uniform header; slash bits top/bottom
  S2  array representations by width (64 .. 2048): _BitInt, compile-time limbs, run-time limbs, residues, mpz
  S3  growth chains through every width, the same widening rule for every representation
  S4  the threshold n*: read from S2 and S3 (where each operation's fastest representation changes)
  S5  the widening rule: whole vector vs per element, by share of wide values
  S6  exact matrix products: mmg (float dgemm), mmgc (mmg in bounded memory), mmi (AVX-512 IFMA kernel)

Every timed result is checked: S1 against the reference coder, S2/S3/S5 against Python integers, S6 inside
p1_gemm.c (classical product at n <= 128, Freivalds at every size). Times are medians of several runs.
"""
import json
import math
import random
import sys
import time
from fractions import Fraction
from pathlib import Path

import numpy as np

import reuse  # noqa: F401
import precision as PR
from formats import round_to_format
from run_benchmarks import metadata

import gen_p1_layouts as GL
import p1_arith_run as AR
import p1_gemm_run as GR
import p1_layout_run as LR
import p1_widen_run as WR

HERE = Path(__file__).resolve().parent


def verify(ok, what):
    if not ok:
        raise AssertionError(f"wrong result: {what}")


def fmt_t(sec):
    for unit, f in (("s", 1), ("ms", 1e3), ("µs", 1e6), ("ns", 1e9)):
        if sec * f >= 1:
            return f"{sec * f:.3g} {unit}"
    return f"{sec * 1e9:.3g} ns"


# ---------------------------------------------------------------- S1 scalar layouts

def precision_of(D, xs):
    """Median relative error of rounding random reals to D data bits, and the worst case (beside 1)."""
    errs = []
    for x in xs:
        if x.numerator.bit_length() + (x.denominator - 1).bit_length() <= D:
            r = x
        else:
            p, q = round_to_format(x.numerator, x.denominator,
                                   lambda a, b: a.bit_length() + (b - 1).bit_length() <= D, 2**D - 1)
            r = Fraction(p, q)
        errs.append(float(abs(r - x) / x))
    return float(np.median(errs)), float(PR.worst_relative_halfgap(1, 1, D))


def s1(n, reps, samples):
    rng = random.Random(7)
    xs = [Fraction(rng.getrandbits(300) | 1, rng.getrandbits(300) | 1) for _ in range(samples)]
    static, timing = [], []
    for name, L in LR.LAYOUTS.items():
        med_err, worst = precision_of(L["D"], xs) if L["order"] == "top" else (None, None)
        static.append(dict(layout=name, total_bits=L["total"], unit_bits=L["U"], data_bits=L["D"],
                           median_error=med_err, worst_error=worst))
        for op in ("dec", "enc", "mul", "promo", "move"):
            if op == "promo" and L["word"] == 128:
                continue
            if op == "move" and (L["order"] == "bot" or L["hdr"] == "uni" and L["order"] == "bot"):
                continue
            I = LR.Inputs(name, n, op, seed=11)
            sts = LR.storages(name) if op in ("dec", "enc", "mul") else (("soa",) if name.startswith("t") else ("flat",))
            for st in sts:
                med, mn, fails, bpv, lines = I.run(st, reps)
                verify(I.check(st, lines, fails), f"S1 {name} {op} {st}")
                per = n // 2 if op == "mul" else n
                timing.append(dict(layout=name, op=op, storage=st, ns_per_value=med / per * 1e9,
                                   min_ns=mn / per * 1e9, bytes_per_value=bpv or None, failures=fails))
    return static, timing


def s1_markdown(static, timing):
    out = ["### Sizes and precision (slash bits at the top; the bottom order has the same sizes)", "",
           "| Layout | Total bits | Unit bits | Data bits | Data bits per stored bit | Median relative error | Worst (beside 1) |",
           "|---|---|---|---|---|---|---|"]
    for r in static:
        if r["median_error"] is None:
            continue
        out.append(f"| {r['layout']} | {r['total_bits']} | {r['unit_bits']} | {r['data_bits']} | "
                   f"{r['data_bits'] / r['total_bits']:.3f} | {r['median_error']:.2e} | {r['worst_error']:.2e} |")
    out += ["", "### Cost per value (ns, median), by storage", "",
            "| Layout | Storage | Bytes per value | Decode | Encode | Multiply (decode both, multiply, reduce, encode) | "
            "Promote to the next size | Complex to quaternion, same size |", "|---|---|---|---|---|---|---|---|"]
    keys = {}
    for r in timing:
        keys.setdefault((r["layout"], r["storage"]), {})[r["op"]] = r
    for (lay, st), ops in keys.items():
        if st == "soa" and all(o not in ops for o in ("dec", "enc", "mul")):
            continue
        g = lambda o: f"{ops[o]['ns_per_value']:.2f}" if o in ops else "—"
        pm = {o: r for (l2, s2), rr in keys.items() if l2 == lay for o, r in rr.items() if o in ("promo", "move")}
        mv = pm.get("move")
        mvs = "—" if not mv else f"{mv['ns_per_value']:.2f}" + (f" ({mv['failures']} no longer fit)" if mv["failures"] else "")
        bpv = next((r["bytes_per_value"] for r in ops.values() if r["bytes_per_value"]), None)
        out.append(f"| {lay} | {st} | {bpv or '—'} | {g('dec')} | {g('enc')} | {g('mul')} | "
                   f"{pm['promo']['ns_per_value']:.2f} | {mvs} |" if "promo" in pm else
                   f"| {lay} | {st} | {bpv or '—'} | {g('dec')} | {g('enc')} | {g('mul')} | — | {mvs} |")
    return "\n".join(out)


# ---------------------------------------------------------------- S2 and S3 arrays

def s2(n, reps):
    rows = []
    for W in AR.WIDTHS:
        I = AR.WidthInputs(W, n, seed=5)
        for op in AR.OPS:
            want = I.expected(op)
            for rep in AR.REPS:
                if op == "widen" and (rep == "mpz" or W == 64):     # mpz has no width; at 64, W/2 is the same tier
                    continue
                med, mn, got = I.run(rep, op, reps)
                verify(got == want, f"S2 W={W} {op} {rep}")
                rows.append(dict(width=W, op=op, rep=rep, median=med, min=mn))
    return rows


def s3(n, reps):
    C = AR.ChainInputs(n, extremes=False)
    steps = AR.steps_for_widths(C)
    rows = []
    for W, K in sorted(steps.items()):
        want = C.expected(K)
        for rep in AR.REPS:
            med, mn, w, got = C.run(rep, K, reps)
            verify(got == want, f"S3 K={K} {rep}")
            if rep in ("bi", "lc", "lr"):
                verify(w == W, f"S3 K={K} {rep} reached width {w}, not {W}")
            rows.append(dict(width=W, steps=K, rep=rep, median=med, min=mn, final_width=w))
    K = max(steps.values()) + 12
    for rep in AR.REPS:
        verify(C.run(rep, K, 1)[3] is None, f"S3 cap {rep}")
    rows.append(dict(width="cap", steps=K, rep="all", note="every representation stops at the 2048-bit cap"))
    return rows


OPNAMES = {"add": "a + b", "mul": "a b (inputs at W/2; residues pre-widened)", "dot": "dot product (same)",
           "mulw": "a b including widening the inputs", "dotw": "dot product including widening the inputs",
           "div1": "exact / 999", "div2": "exact / 1000", "sort": "sort", "scan": "bit-length scan",
           "widen": "widen one vector W/2 to W"}
TIE = 0.10            # differences within 10% are treated as ties (repeated builds moved some timings by ~10%)


def with_widening(rows):
    """Add mulw and dotw: residues read inputs already extended to width W, so their cost of getting there (two
    vectors widened) is added; the other representations multiply W/2 inputs directly, so theirs is unchanged."""
    by = {}
    for r in rows:
        by[(r["width"], r["op"], r["rep"])] = r
    out = list(rows)
    for (W, op, rep), r in list(by.items()):
        if op in ("mul", "dot"):
            extra = 2 * by[(W, "widen", rep)]["median"] if rep == "rns" and (W, "widen", rep) in by else 0.0
            out.append(dict(width=W, op=op + "w", rep=rep, median=r["median"] + extra, min=r["min"] + extra))
    return out


def s2_markdown(rows, n):
    rows = with_widening(rows)
    out = [f"Times for $n = {n}$ elements. In bold: the fastest, and any within {TIE:.0%} of it (a tie). At width 64 "
           "residues are a plain int64 vector.", "",
           "| Width | Operation | " + " | ".join(AR.REPS) + " |", "|---|---|" + "---|" * len(AR.REPS)]
    by = {}
    for r in rows:
        by.setdefault((r["width"], r["op"]), {})[r["rep"]] = r["median"]
    order = ("add", "mul", "mulw", "dot", "dotw", "div1", "div2", "sort", "scan", "widen")
    for (W, op), t in sorted(by.items(), key=lambda kv: (kv[0][0], order.index(kv[0][1]))):
        best = min(t.values())
        cell = lambda rep: "—" if rep not in t else (f"**{fmt_t(t[rep])}**" if t[rep] <= best * (1 + TIE) else fmt_t(t[rep]))
        out.append(f"| {W} | {OPNAMES[op]} | " + " | ".join(cell(rep) for rep in AR.REPS) + " |")
    return "\n".join(out)


def s3_markdown(rows, n):
    out = [f"$x \\leftarrow x b + c \\cdot 1000^{{k+1}}$ on $n = {n}$ 3-place decimals; each step count is the largest whose "
           "proven bound fits the width. Residues widen in steps of one 62-bit prime; the others double.", "",
           "| Width reached | Steps | " + " | ".join(AR.REPS) + " | Residue capacity at the end |",
           "|---|---|" + "---|" * len(AR.REPS) + "---|"]
    by = {}
    for r in rows:
        if r["width"] == "cap":
            continue
        by.setdefault((r["width"], r["steps"]), {})[r["rep"]] = r
    for (W, K), t in by.items():
        best = min(v["median"] for v in t.values())
        cell = lambda rep: f"**{fmt_t(t[rep]['median'])}**" if t[rep]["median"] <= best * (1 + TIE) else fmt_t(t[rep]["median"])
        out.append(f"| {W} | {K} | " + " | ".join(cell(rep) for rep in AR.REPS) + f" | {t['rns']['final_width']} bits |")
    cap = [r for r in rows if r["width"] == "cap"][0]
    out.append(f"\nAt {cap['steps']} steps every representation stopped at the 2048-bit cap before overflowing.")
    return "\n".join(out)


def s4_markdown(s2rows):
    """For each operation: the fastest compile-time representation (bi, lc) against the fastest run-time one
    (lr, rns, mpz) at each width, and the first width from which the run-time side stays ahead beyond the tie band.
    Residues at width 64 are a plain int64 vector, so they count on the compile-time side there."""
    by = {}
    for r in with_widening(s2rows):
        if r["op"] in ("mul", "dot"):
            continue                     # use the rows that include widening
        by.setdefault(r["op"], {}).setdefault(r["width"], {})[r["rep"]] = r["median"]
    out = ["Ratio = fastest compile-time representation (_BitInt, compile-time limbs; residues at 64 bits) / fastest "
           f"run-time one (run-time limbs, residues, mpz); above 1 means the run-time side is faster. Within "
           f"{TIE:.0%} either way counts as a tie.", "",
           "| Operation | " + " | ".join(str(W) for W in AR.WIDTHS) + " | Flips at |", "|---|" + "---|" * (len(AR.WIDTHS) + 1)]
    for op, ws in by.items():
        ratios, flip = [], None
        for W in AR.WIDTHS:
            if W not in ws:
                ratios.append(float("nan"))
                continue
            t = ws[W]
            sm = ("bi", "lc", "rns") if W == 64 else ("bi", "lc")
            lg = ("lr", "mpz") if W == 64 else ("lr", "rns", "mpz")
            small = min(t[r] for r in sm if r in t)
            large = min(t[r] for r in lg if r in t)
            ratios.append(small / large)
        for i, W in enumerate(AR.WIDTHS):
            if all(x > 1 + TIE for x in ratios[i:] if x == x):
                flip = W
                break
        out.append(f"| {OPNAMES[op]} | " + " | ".join(f"{x:.2f}" for x in ratios) + f" | {flip or 'never (to 2048)'} |")
    return "\n".join(out)


# ---------------------------------------------------------------- S5 widening rule

def s5(n, reps):
    rows = []
    for share in WR.SHARES:
        I = WR.Inputs(n, share, seed=9)
        for op in ("sq", "sumsq"):
            want = I.expected(op)
            for rule in WR.RULES:
                med, mn, wide, got = I.run(rule, op, reps)
                verify(got == want, f"S5 {share} {op} {rule}")
                rows.append(dict(share=share, op=op, rule=rule, median=med, min=mn, wide=wide))
    return rows


def s5_markdown(rows, n):
    out = [f"$n = {n}$; a share of the values is near $10^{{16}}$, so their squares need 256 bits.", "",
           "| Share wide | Operation | Whole vector | Per element | Whole / per element |", "|---|---|---|---|---|"]
    by = {}
    for r in rows:
        by.setdefault((r["share"], r["op"]), {})[r["rule"]] = r["median"]
    for (sh, op), t in by.items():
        out.append(f"| {sh:.1%} | {'(ab)^2' if op == 'sq' else 'sum of (ab)^2'} | {fmt_t(t['whole'])} | "
                   f"{fmt_t(t['elem'])} | {t['whole'] / t['elem']:.2f} |")
    return "\n".join(out)


# ---------------------------------------------------------------- S6 matrix products

def s6(sizes, widths, reps):
    rows = []
    for n in sizes:
        for bits in widths:
            for m in GR.METHODS if hasattr(GR, "METHODS") else ("mmg", "mmgc", "mmi"):
                r = GR.run(m, n, bits, reps=reps)            # checked inside p1_gemm.c; raises on a mismatch
                r.update(n=n, bits=bits, method=m)
                rows.append(r)
    return rows


def s6_markdown(rows):
    out = ["Peak memory is the rise above the inputs; part of it is shared by every method (input limbs, the limb "
           "output and the copy made for the check, estimated in the last column), so the method's own working memory "
           "is roughly the difference.", "",
           "| Size | Entry bits | Method | Median | Reduce | Product | Reconstruct | Primes | Peak memory | Shared part |",
           "|---|---|---|---|---|---|---|---|---|---|"]
    for r in rows:
        out.append(f"| {r['n']} | {r['bits']} | {r['method']} | {fmt_t(r['median'])} | {fmt_t(r['reduce'])} | "
                   f"{fmt_t(r['product'])} | {fmt_t(r['crt'])} | {r['primes']} | +{r['peak_mb']:.0f} MB | "
                   f"~{r['shared_mb']:.0f} MB |")
    return "\n".join(out)


def main():
    quick = "--quick" in sys.argv
    for m in (AR, LR, WR, GR):
        m.build()
    meta = metadata()
    meta["compiled"] = ("p1_arith, p1_layout, p1_widen: clang 18 -O3 -march=native; p1_gemm: gcc 13 -O3 -march=native "
                        "with AVX-512 IFMA; OpenBLAS 0.3.34 (scipy-openblas32), one thread; GMP 6.3.0")
    if quick:
        p = dict(s1n=20000, s1reps=3, samples=60, s2n=5000, s2reps=3, s3n=2000, s3reps=3, s5n=20000, s5reps=3,
                 sizes=(64,), widths=(64, 1024), s6reps=1)
    else:
        p = dict(s1n=200000, s1reps=7, samples=300, s2n=100000, s2reps=5, s3n=100000, s3reps=3, s5n=1000000,
                 s5reps=7, sizes=(128, 256, 512), widths=(64, 256, 1024, 2048), s6reps=3)
    res = {}
    for key, fn in (("S1", lambda: s1(p["s1n"], p["s1reps"], p["samples"])), ("S2", lambda: s2(p["s2n"], p["s2reps"])),
                    ("S3", lambda: s3(p["s3n"], p["s3reps"])), ("S5", lambda: s5(p["s5n"], p["s5reps"])),
                    ("S6", lambda: s6(p["sizes"], p["widths"], p["s6reps"]))):
        t = time.perf_counter()
        res[key] = fn()
        print(f"  done  {key}  ({time.perf_counter() - t:.1f} s)", flush=True)
    header = "\n".join(f"- **{k}**: {v}" for k, v in meta.items())
    text = (f"# Phase one: the variants compared with each other\n\n{header}\n\n"
            f"## S1. Scalar layouts\n\n{s1_markdown(*res['S1'])}\n\n"
            f"## S2. Array representations by width\n\n{s2_markdown(res['S2'], p['s2n'])}\n\n"
            f"## S3. Growth chains through every width\n\n{s3_markdown(res['S3'], p['s3n'])}\n\n"
            f"## S4. Where each operation flips to the run-time side\n\n{s4_markdown(res['S2'])}\n\n"
            f"## S5. The widening rule\n\n{s5_markdown(res['S5'], p['s5n'])}\n\n"
            f"## S6. Exact matrix products\n\n{s6_markdown(res['S6'])}\n")
    out = HERE.parent / "results"
    suffix = "-quick" if quick else ""
    (out / f"phase1{suffix}.md").write_text(text)
    (out / f"phase1{suffix}.json").write_text(json.dumps({"metadata": meta, "params": p, **res}, indent=1, default=str))
    print(text)


if __name__ == "__main__":
    main()
