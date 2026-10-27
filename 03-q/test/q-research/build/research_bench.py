"""Research measurements: LCF order keys, quote notation's 2-adic arithmetic, and the plane variant.
Every result is checked against an exact computation before it is reported."""
import os

for var in ("OPENBLAS_NUM_THREADS", "OMP_NUM_THREADS", "MKL_NUM_THREADS"):
    os.environ[var] = "1"

import json
import math
import random
import subprocess
import sys
import time
from fractions import Fraction
from pathlib import Path

import numpy as np
import gmpy2

import reuse  # noqa: F401
from harness import Report, measure
from run_benchmarks import metadata, verify
from q72 import fits
from svec import SVec
from cxcol import CxCol
import cbase

import lcf
import lcf_bound
import lcfkern as LK
import hensel_run as H
import plane as P
import plane_format as PF
import cplx_run as CR

HERE = Path(__file__).resolve().parent


class CStats(cbase.Stats):
    pass


def build():
    cb = HERE.parent / "cbench"
    for src, out, libs in (("hensel.c", "hensel", ["-lgmp", "-lm"]), ("cplx.c", "cplx", ["-lflint", "-lgmp"])):
        if not (cb / out).exists() or (cb / out).stat().st_mtime < (cb / src).stat().st_mtime:
            subprocess.run(["gcc", "-O3", "-march=native", "-Wall", "-o", str(cb / out), str(cb / src), *libs], check=True)


# ---------------------------------------------------------------- R1. LCF order keys

def q72_sample(n, seed=11):
    """A mix of Q72 values: decimals of several scales, ratios of integers, and wide values."""
    rng = random.Random(seed)
    neg, ps, qs = [], [], []
    while len(ps) < n:
        k = rng.random()
        if k < 0.4:
            p, q = rng.randrange(0, 10**rng.randint(1, 15)), 10**rng.randint(0, 6)
        elif k < 0.7:
            p, q = rng.randrange(0, 1001), rng.randrange(1, 1001)
        else:
            p, q = rng.randrange(0, 2**rng.randint(1, 63)), rng.randrange(1, 2**rng.randint(1, 30))
        g = math.gcd(p, q) or 1
        p, q = p // g, q // g
        if p == 0:
            q = 1
        if fits(p, q) or p == 0:
            neg.append(rng.random() < 0.5)
            ps.append(p)
            qs.append(q)
    return np.array(neg), np.array(ps, dtype=np.uint64), np.array(qs, dtype=np.uint64)


def bench_lcf(report, n):
    neg, p, q = q72_sample(n)
    fr = [Fraction(-int(a) if s else int(a), int(b)) for s, a, b in zip(neg, p, q)]
    want = sorted(fr)
    f = np.array([float(x) for x in fr])
    mpq = [gmpy2.mpq(int(x.numerator), int(x.denominator)) for x in fr]
    kh, kl = np.empty(n, np.uint64), np.empty(n, np.uint64)

    def by_keys():
        LK.keys(neg, p, q, kh, kl)
        return np.lexsort((kl, kh))

    def by_float():
        order = np.argsort(f, kind="stable")
        LK.fix_ties(order, f, neg, p, q)
        return order

    def keys_merge():
        LK.keys(neg, p, q, kh, kl)
        return LK.merge_sort_keys(kh, kl)

    def by_float_converting():
        g = np.array([float(x) for x in fr])        # the correctly rounded conversion (Fraction.__float__)
        order = np.argsort(g, kind="stable")
        LK.fix_ties(order, g, neg, p, q)
        return order

    con = {"LCF keys (128-bit), then numpy lexsort": by_keys,
           "LCF keys (128-bit), then the same merge sort": keys_merge,
           "exact merge sort, cross-multiplied comparisons": lambda: LK.merge_sort_exact(neg, p, q),
           "float64 sort, then exact sort of equal floats (floats given)": by_float,
           "float64 sort, then exact sort of equal floats (including conversion)": by_float_converting,
           "GMP mpq, Python sort": lambda: sorted(mpq)}
    res = measure(con, gc_off=False)
    extra = {}
    for k, fn in con.items():
        out = fn()
        got = [as_fr(v) for v in out] if k.startswith("GMP") else [fr[i] for i in out]
        extra[k] = dict(error=verify(got == want, f"LCF sort {k}", "0 (exact order)"))
    extra["LCF keys (128-bit), then numpy lexsort"]["note"] = "keys: 16 bytes per value; encoding included"
    extra["LCF keys (128-bit), then the same merge sort"]["note"] = "the merge sort of the exact method, on keys"
    extra["float64 sort, then exact sort of equal floats (floats given)"]["note"] = (
        "correctly rounded floats computed before timing; float(p)/float(q) would not be monotonic")
    extra["float64 sort, then exact sort of equal floats (including conversion)"]["note"] = (
        "conversion by Python's exact Fraction-to-float")
    report.add_measured("R1 sort Q72 values", n, res, "exact merge sort, cross-multiplied comparisons", **extra)
    res = measure({"LCF keys, encoding only": lambda: LK.keys(neg, p, q, kh, kl)})
    report.add_measured("R1 encode keys", n, res, None, **{"LCF keys, encoding only": dict(error="0 (exact)")})
    # the key orders equal values equally and distinct values strictly
    LK.keys(neg, p, q, kh, kl)
    order = np.lexsort((kl, kh))
    s = [fr[i] for i in order]
    k2 = [(int(kh[i]) << 64) | int(kl[i]) for i in order]
    verify(all((k2[i] == k2[i + 1]) == (s[i] == s[i + 1]) for i in range(n - 1)), "LCF keys are injective")


def as_fr(v):
    return Fraction(int(v.numerator), int(v.denominator))


def lcf_lengths():
    return {"lexibinary (LCF's code)": dict(searched=lcf_width_search(lcf.lexibinary),
                                            bound=lcf_bound.bound(lcf.lexibinary)),
            "delta code": dict(searched=lcf_width_search(lcf.delta), bound=lcf_bound.bound(lcf.delta))}


def lcf_width_search(qcode, beam=50):
    ch = ([0, 1, 2, 3, 4, 5, 6, 7, 8, 15, 16, 31, 32, 63, 64, 127, 255, 1023, 2**16 - 1, 2**32 - 1]
          + [2**k - 1 for k in range(40, 65, 4)])
    best = (0, 0, 1)
    frontier = {}
    for a0 in ch + [2**64 - 1, 2**63]:
        frontier.setdefault((a0.bit_length(), 1), []).append((len(qcode(a0 + 1)), (a0, 1, 1, 0)))
        if a0:
            best = max(best, (len(qcode(a0 + 1)), a0, 1))
    while frontier:
        nxt = {}
        for _, states in frontier.items():
            for c, (pp, qq, p0, q0) in sorted(states, reverse=True)[:beam]:
                for a in ch[1:] + [2**k + 1 for k in range(1, 40, 3)] + [2**k for k in range(1, 64, 5)]:
                    np_, nq = a * pp + p0, a * qq + q0
                    if np_.bit_length() + (nq - 1).bit_length() > 65:
                        continue
                    nc = c + len(qcode(a))
                    if a >= 2 and fits(np_, nq):
                        best = max(best, (nc, np_, nq))
                    nxt.setdefault((np_.bit_length(), nq.bit_length()), []).append((nc, (np_, nq, pp, qq)))
        frontier = nxt
    c, pp, qq = best
    assert len(lcf.code(pp, qq, qcode=qcode)[0]) == c
    return dict(bits=c, at=f"{pp}/{qq}")


# ---------------------------------------------------------------- R2. quote notation: 2-adic arithmetic

def bench_hensel(report, n):
    rng = np.random.default_rng(81)
    for label, nb, dmax in (("unrelated denominators 1..1000", 10**6, 1000), ("decimals, 3 places", 10**7, None)):
        nums = [rng.integers(-nb, nb + 1, size=n) for _ in range(3)]
        dens = [rng.integers(1, dmax + 1, size=n) if dmax else np.full(n, 1000) for _ in range(3)]
        for K in (1, 2, 3, 4):
            want = H.exact(nums, dens, K)
            tag = f"R2 x <- x*b + c, {K} step{'s' if K > 1 else ''}, {label}"
            for m, name in (("mpq", "GMP mpq_t, reduced every step (C)"),
                            ("pairs", "128-bit numerator and denominator, gcd at the end (C)"),
                            ("hensel", "2-adic residues mod 2^128, reconstructed at the end (C)")):
                med, mn, falls, got = H.run(m, nums, dens, K, reps=5)
                verify(got == want, f"{tag} {m}")
                report.add(tag, n, name, cbase.Stats(med, mn), baseline=(m == "mpq"), error="0 (every element)",
                           note=f"{falls} of {n} elements redone with GMP" if m != "mpq" else "")


# ---------------------------------------------------------------- R3. the plane variant

def bench_plane(report, n, places, bound):
    rng = np.random.default_rng(90 + places)
    den = 10**places
    v = [rng.integers(-bound * den, bound * den + 1, size=n) for _ in range(6)]
    xr, xi, yr, yi, wr, wi = v
    F = [[Fraction(int(a), den) for a in col] for col in v]
    x, y, w = list(zip(F[0], F[1])), list(zip(F[2], F[3])), list(zip(F[4], F[5]))
    mul = [(a[0] * b[0] - a[1] * b[1], a[0] * b[1] + a[1] * b[0]) for a, b in zip(x, y)]
    want = {"mul": mul, "fma": [(m[0] + c[0], m[1] + c[1]) for m, c in zip(mul, w)],
            "dot": [(sum(m[0] for m in mul), sum(m[1] for m in mul))],
            "abs2": [(sum(a[0] ** 2 + a[1] ** 2 for a in x), Fraction(0))]}
    PX, PY, PW = (P.PVec.decimals(a, b, den) for a, b in ((xr, xi), (yr, yi), (wr, wi)))

    def axis(a, b):
        return CxCol("two", SVec.decimals(a, den), SVec.decimals(b, den))

    AX, AY, AW = axis(xr, xi), axis(yr, yi), axis(wr, wi)
    plane_ops = {"mul": lambda: PX * PY, "fma": lambda: PX * PY + PW, "dot": lambda: (PX * PY).total2(),
                 "abs2": lambda: PX.abs2().total()}
    axis_ops = {"mul": lambda: AX * AY, "fma": lambda: AX * AY + AW, "dot": lambda: (AX * AY).total2(),
                "abs2": lambda: AX.abs2().total2()}

    def norm(op, out, kind):
        if op in ("mul", "fma"):
            return out.values() if kind == "plane" else [tuple(map(Fraction, z)) for z in out.values()]
        if op == "dot":
            return [tuple(out)]
        return [(out, Fraction(0))] if kind == "plane" else [tuple(out)]

    inputs = CR.Inputs(*v, den)
    for op in ("mul", "fma", "dot", "abs2"):
        tag = f"R3 complex {op}, {places} places, parts below {bound:g}"
        QX, QY, QW = (P.APair.decimals(a, b, den) for a, b in ((xr, xi), (yr, yi), (wr, wi)))
        pair_ops = {"mul": lambda: QX * QY, "fma": lambda: QX * QY + QW, "dot": lambda: (QX * QY).total2(),
                    "abs2": lambda: QX.abs2().total()}
        res = measure({"plane vector (A + Bi)/D": plane_ops[op],
                       "axis pair, written like the plane vector": pair_ops[op],
                       "pair of axis vectors (CxCol)": axis_ops[op]}, gc_off=False)
        po, ao, qo = plane_ops[op](), axis_ops[op](), pair_ops[op]()
        report.add_measured(tag, n, res, None, **{
            "plane vector (A + Bi)/D": dict(error=verify(norm(op, po, "plane") == want[op], tag + " plane"),
                                            bytes_per_value=16 if op in ("mul", "fma") and po.re.tier == "s64" else None,
                                            note=f"tier {po.re.tier}" if op in ("mul", "fma") else ""),
            "axis pair, written like the plane vector": dict(
                error=verify(norm(op, qo, "plane") == want[op], tag + " apair"),
                bytes_per_value=16 if op in ("mul", "fma") and qo.re.tier == "s64" else None,
                note="same code as the plane vector; each part keeps its own denominator"),
            "pair of axis vectors (CxCol)": dict(error=verify(norm(op, ao, "axis") == want[op], tag + " axis"),
                                                 note="the benchmark's complex columns, which reduce after every "
                                                      "step" + (f"; tiers {ao.a.tier}, {ao.b.tier}"
                                                                if op in ("mul", "fma") else ""))})
        for who in CR.NAMES:
            med, mn, got = inputs.run(who, op, 5)
            if got is None:
                report.add(tag, n, CR.NAMES[who], cbase.Stats(med, mn), error="overflow detected")
                continue
            verify(got == want[op], f"{tag} {who}")
            report.add(tag, n, CR.NAMES[who], cbase.Stats(med, mn), baseline=(who == "mpq"),
                       error="0 (every element)" if op in ("mul", "fma") else "0 (exact)")


def plane_format_markdown(n):
    rows = PF.table(n)
    lines = ["| Values | P80 (16 + 64 bits) | 2 x Q40 (80 bits) | P152 (24 + 128 bits) | 2 x Q72 (144 bits) "
             "| median bits, plane | median bits, pair |", "|---|---|---|---|---|---|---|"]
    for r in rows:
        lines.append(f"| {r['values']} | {100 * r['p80']:.3g}% | {100 * r['q40x2']:.3g}% | {100 * r['p152']:.3g}% "
                     f"| {100 * r['q72x2']:.3g}% | {r['plane_bits']:g} | {r['pair_bits']:g} |")
    return "\n".join(lines), rows


def run_tests():
    import test_research
    import test_plane
    for mod in (test_research, test_plane):
        for name in sorted(x for x in dir(mod) if x.startswith("test_")):
            t = time.perf_counter()
            getattr(mod, name)()
            print(f"  pass  {mod.__name__}.{name}  ({time.perf_counter() - t:.1f} s)", flush=True)


def main():
    quick = "--quick" in sys.argv
    build()
    print("correctness tests")
    run_tests()
    meta = metadata()
    meta["compiled"] = "gcc -O3 -march=native; GMP 6.3.0, FLINT 3.0.1"
    sort_n, hen_n, plane_n, fmt_n = (2 * 10**4, 3000, 2 * 10**4, 2000) if quick else (10**6, 10**5, 10**6, 20000)
    R1, R2, R3 = Report(), Report(), Report()
    t = time.perf_counter()
    widths = lcf_lengths()
    print(f"  done  LCF widths  ({time.perf_counter() - t:.1f} s)", flush=True)
    steps = [("R1 LCF", lambda: bench_lcf(R1, sort_n)), ("R2 2-adic", lambda: bench_hensel(R2, hen_n)),
             ("R3 plane, 3 places", lambda: bench_plane(R3, plane_n, 3, 10**3)),
             ("R3 plane, 6 places", lambda: bench_plane(R3, plane_n // 4, 6, 10**6))]
    for name, step in steps:
        t = time.perf_counter()
        step()
        print(f"  done  {name}  ({time.perf_counter() - t:.1f} s)", flush=True)
    fmt_md, fmt_rows = plane_format_markdown(fmt_n)
    out = HERE.parent / "results"
    out.mkdir(exist_ok=True)
    suffix = "-quick" if quick else ""
    header = "\n".join(f"- **{k}**: {v}" for k, v in meta.items())
    wid = "\n".join(f"| {k} | {v['searched']['bits']} (at {v['searched']['at']}) | {v['bound']} |"
                    for k, v in widths.items())
    text = (f"# Research measurements\n\n{header}\n\n"
            "## R1. LCF order keys\n\nLongest key for any Q72 magnitude (bits, without the sign bit):\n\n"
            "| Quotient code | longest found by search | proven upper bound |\n|---|---|---|\n" + wid + "\n\n"
            f"{R1.markdown()}\n\n"
            f"## R2. Quote notation: 2-adic arithmetic against deferred and per-step reduction\n\n{R2.markdown()}\n\n"
            f"## R3. The plane variant\n\nFormat: share of values that fit, and bits needed\n\n{fmt_md}\n\n"
            f"Speed (baseline GMP mpq_t pairs from C)\n\n{R3.markdown()}\n")
    (out / f"research{suffix}.md").write_text(text)
    (out / f"research{suffix}.json").write_text(json.dumps(
        {"metadata": meta, "lcf_widths": widths, "R1": R1.raw(), "R2": R2.raw(), "R3": R3.raw(),
         "plane_format": fmt_rows}, indent=1, default=str))
    print(text)


if __name__ == "__main__":
    main()
