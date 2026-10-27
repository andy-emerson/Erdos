"""Research for the division-algebra family: fixed-width uN tiers (B), quaternion arrays (C), and the precision
of each member (D). Every result is checked against Python's exact Fractions before it is reported."""
import os

for var in ("OPENBLAS_NUM_THREADS", "OMP_NUM_THREADS", "MKL_NUM_THREADS"):
    os.environ[var] = "1"

import json
import math
import random
import sys
import time
from fractions import Fraction
from pathlib import Path

import numpy as np

import reuse  # noqa: F401
from harness import Report
from run_benchmarks import metadata, verify
import cbase
import precision as PR
from formats import round_to_format

import family as FA
import quat_run as QR
import un_run as UR
import oct_run as OR
import gemm_run as GR
import uniform as UNI

HERE = Path(__file__).resolve().parent


def frac_chain(a, b, c, den, K):
    F = lambda v: Fraction(int(v), den)
    x = [F(v) for v in a]
    bb, cc = [F(v) for v in b], [F(v) for v in c]
    for _ in range(K):
        x = [xi * bi + ci for xi, bi, ci in zip(x, bb, cc)]
    return x


# ---------------------------------------------------------------- B. fixed-width uN tiers

def bench_un(report, n, steps):
    rng = np.random.default_rng(101)
    den = 1000
    a0 = rng.integers(-10**7, 10**7 + 1, size=n)                 # 3-place decimals below 10^4
    b = rng.integers(-10**4, 10**4 + 1, size=n)                  # below 10 in magnitude
    c = rng.integers(-10**6, 10**6 + 1, size=n)
    for label, share in (("none wide", 0), ("1% wide", n // 100), ("all wide", n)):
        a = a0.copy()
        idx = rng.choice(n, size=share, replace=False) if share else np.array([], dtype=np.int64)
        a[idx] = rng.integers(10**15, 10**16, size=share) * np.where(rng.random(share) < 0.5, -1, 1)
        inp = UR.Inputs(a, b, c, den)
        F = lambda v: Fraction(int(v), den)
        sq = [(F(x) * F(y)) ** 2 for x, y in zip(a, b)]
        for op, want in (("sq", sq), ("sumsq", [sum(sq)])):
            tag = f"B {'(a*b)^2' if op == 'sq' else 'sum of (a*b)^2'}, {label}"
            for m, name in UR.NAMES.items():
                if "@" in m or m == "flc":                       # pre-built 2048-bit variants: chains only
                    continue
                med, mn, w, got = inp.run(m, op, 0, reps=7)
                verify(got == want, f"{tag} {m}")
                report.add(tag, n, name, cbase.Stats(med, mn), baseline=(m == "mpz"),
                           error="0 (every element)" if op == "sq" else "0 (exact)",
                           note=f"vector width {w} bits" if m in ("un", "fl") else "")
        if share:
            continue
        F = lambda v: Fraction(int(v), den)
        xs, bb, cc, done = [F(v) for v in a], [F(v) for v in b], [F(v) for v in c], 0
        for K in sorted(steps):
            for _ in range(K - done):                 # the exact reference, advanced step by step
                xs = [xi * bi + ci for xi, bi, ci in zip(xs, bb, cc)]
            done = K
            want = xs
            tag = f"B growth chain x <- x*b + c, {K} steps"
            for m, name in UR.NAMES.items():
                med, mn, w, got = inp.run(m, "chain", K, reps=5)
                if m in UR.FIXED and got is None:
                    report.add(tag, n, name, None, error="exceeds the 2048-bit cap",
                               note="detected before overflow; would continue in arbitrary precision")
                    continue
                verify(got == want, f"{tag} {m}")
                report.add(tag, n, name, cbase.Stats(med, mn), baseline=(m == "mpz"), error="0 (every element)",
                           note=f"ends at width {w} bits" if m in UR.FIXED else "")


# ---------------------------------------------------------------- C. quaternion arrays

def bench_quat(report, n, places, bound):
    rng = np.random.default_rng(200 + places)
    den = 10**places
    mk = lambda: [rng.integers(-bound * den, bound * den + 1, size=n) for _ in range(4)]
    p, q, v = mk(), mk(), mk()
    v[0][:] = 0                                                  # rotate pure vectors
    want = QR.reference(p, q, v, den)
    inp = QR.Inputs(p, q, v, den)
    names = {"add": "p + q", "mul": "Hamilton product p q", "norm": "sum of squared norms", "rot": "rotation q v q*"}
    for op in ("add", "mul", "norm", "rot"):
        tag = f"C {names[op]}, {places} places, components below {bound:g}"
        for m, name in QR.NAMES.items():
            med, mn, got = inp.run(m, op, reps=7)
            if got == "exceeds":
                report.add(tag, n, name, None, error="needs more than 128 bits", note="refused, not wrapped")
                continue
            if got is None:
                report.add(tag, n, name, cbase.Stats(med, mn), error="overflow detected")
                continue
            verify(got == want[op], f"{tag} {m}")
            wide = max(abs(int(x)) for comp in (p + q) for x in comp).bit_length() * 2 + 2 > 63
            report.add(tag, n, name, cbase.Stats(med, mn), baseline=(m == "fmpz"),
                       error="0 (exact)" if op == "norm" else "0 (every element)",
                       bytes_per_value=(64 if wide and op == "mul" else 32) if m == "vec" and op in ("add", "mul") else None)


# ---------------------------------------------------------------- E. octonion arrays

def bench_oct(report, n, places, bound):
    rng = np.random.default_rng(300 + places)
    den = 10**places
    mk = lambda: [rng.integers(-bound * den, bound * den + 1, size=n) for _ in range(8)]
    p, q = mk(), mk()
    want = OR.reference(p, q, den)
    inp = OR.Inputs(p, q, den)
    names = {"add": "p + q", "mul": "octonion product p q", "norm": "sum of squared norms"}
    for op in ("add", "mul", "norm"):
        tag = f"E {names[op]}, {places} places, components below {bound:g}"
        for m, name in OR.NAMES.items():
            med, mn, got = inp.run(m, op, reps=7)
            if got == "exceeds":
                report.add(tag, n, name, None, error="needs more than 128 bits", note="refused, not wrapped")
                continue
            if got is None:
                report.add(tag, n, name, cbase.Stats(med, mn), error="overflow detected")
                continue
            verify(got == want[op], f"{tag} {m}")
            report.add(tag, n, name, cbase.Stats(med, mn), baseline=(m == "fmpz"),
                       error="0 (exact)" if op == "norm" else "0 (every element)")


# ---------------------------------------------------------------- G. exact matrix products with wide entries

def bench_gemm(report, sizes, widths):
    for n in sizes:
        for bits in widths:
            tag = f"G {n} x {n} product, entries of {bits} bits"
            for m, name in GR.NAMES.items():
                if m == "fmpq" and n * n * bits > 256 * 256 * 1024:
                    continue                      # FLINT clears denominators and then multiplies as fmpz
                r = GR.run(m, n, bits, reps=3)    # every method is checked against FLINT inside gemm.c
                note = (f"{r['primes']} primes; reduce {r['reduce']*1e3:.0f} ms, dgemm {r['gemm']*1e3:.0f} ms, "
                        f"CRT {r['crt']*1e3:.0f} ms; " if "primes" in r else "") + f"peak memory +{r['peak_mb']:.0f} MB"
                report.add(tag, n, name, cbase.Stats(r["median"], r["min"]), baseline=(m == "fmpz"),
                           error="0 (every entry)", note=note)


# ---------------------------------------------------------------- D. precision of each member

def round_word(x, D):
    m = abs(x)
    if m > 2**D - 1:
        return None
    if m.numerator.bit_length() + (m.denominator - 1).bit_length() <= D:
        return m
    p, q = round_to_format(m.numerator, m.denominator, lambda a, b: a.bit_length() + (b - 1).bit_length() <= D,
                           2**D - 1)
    return Fraction(p, q)


def member_precision(samples):
    rng = random.Random(7)
    xs = [Fraction(rng.getrandbits(400) | 1, rng.getrandbits(400) | 1) for _ in range(samples)]
    rows = []
    members = [("reals", 1, 128), ("complex", 2, 64), ("quaternions", 4, 32), ("octonions", 8, 16),
               ("quaternions, promoted once", 4, 64), ("quaternions, promoted twice", 4, 128)]
    for name, dim, word in members:
        U, h, b, D = FA.layout(dim, word)
        errs = []
        for x in xs:
            r = round_word(x, D)
            errs.append(float(abs(r - x) / x))
        med = float(np.median(errs))
        worst = PR.worst_relative_halfgap(1, 1, D)          # beside 1, the simplest fraction: the worst case
        p_med = next(p for p in range(2, 200) if np.median([float(abs(PR.round_float(x, p) - x) / x)
                                                           for x in xs]) <= med)
        p_w = math.ceil(-math.log2(float(worst)))
        rows.append(dict(member=name, total_bits=8 + word, unit_bits=U, slash_bits=h + b, borrowed=b, data_bits=D,
                         median=med, worst=float(worst), float_bits_median=p_med, float_bits_worst=p_w,
                         largest=f"2^{D} - 1", smallest=f"2^-{D - 1}"))
    return rows


def precision_markdown(rows):
    out = ["| Member | Size | Unit bits | Slash bits (borrowed) | Data bits | Median relative error | "
           "Worst (beside 1) | Float significand to match: median | worst |", "|---|---|---|---|---|---|---|---|---|"]
    for r in rows:
        out.append(f"| {r['member']} | {r['total_bits']} | {r['unit_bits']} | {r['slash_bits']} ({r['borrowed']}) | "
                   f"{r['data_bits']} | {r['median']:.1e} | {r['worst']:.1e} | {r['float_bits_median']} | "
                   f"{r['float_bits_worst']} |")
    return "\n".join(out)


def run_tests():
    import test_family
    for name in sorted(x for x in dir(test_family) if x.startswith("test_")):
        t = time.perf_counter()
        getattr(test_family, name)()
        print(f"  pass  test_family.{name}  ({time.perf_counter() - t:.1f} s)", flush=True)


def main():
    quick = "--quick" in sys.argv
    UR.build()
    QR.build()
    OR.build()
    GR.build()
    print("correctness tests")
    run_tests()
    meta = metadata()
    meta["compiled"] = ("section B (un.c, every method in it): clang 18 -O3 -march=native, for _BitInt; "
                        "sections C, E, G (quat.c, oct.c, gemm.c): gcc 13 -O3 -march=native; GMP 6.3.0, FLINT 3.0.1; "
                        "G uses OpenBLAS 0.3.34 (scipy-openblas32 wheel) under every method, one thread, and FLINT 3.0.1 "
                        "configured with BLAS (build_flint_blas.sh) except the row marked without BLAS")
    un_n, q_n, q_n6, samples = (3000, 3000, 2000, 60) if quick else (10**5, 2 * 10**5, 10**5, 300)
    steps = (3, 10, 25) if quick else (3, 10, 25, 40, 70, 100, 140, 200)
    RB, RC, RE, RG = Report(), Report(), Report(), Report()
    o_n, o_n6 = (3000, 2000) if quick else (10**5, 5 * 10**4)
    g_sizes, g_widths = ((64, 128), (64, 1024)) if quick else ((128, 256, 512), (64, 256, 1024, 2048))
    for name, step in (("B uN", lambda: bench_un(RB, un_n, steps)),
                       ("C quaternions, 3 places", lambda: bench_quat(RC, q_n, 3, 10**3)),
                       ("C quaternions, 6 places", lambda: bench_quat(RC, q_n6, 6, 10**6)),
                       ("E octonions, 3 places", lambda: bench_oct(RE, o_n, 3, 10**3)),
                       ("E octonions, 6 places", lambda: bench_oct(RE, o_n6, 6, 10**6)),
                       ("G exact matrix products", lambda: bench_gemm(RG, g_sizes, g_widths))):
        t = time.perf_counter()
        step()
        print(f"  done  {name}  ({time.perf_counter() - t:.1f} s)", flush=True)
    t = time.perf_counter()
    prec = member_precision(samples)
    print(f"  done  D precision  ({time.perf_counter() - t:.1f} s)", flush=True)
    uni = UNI.compare()
    dec = GR.decode(10**6 if quick else 10**7)
    out = HERE.parent / "results"
    out.mkdir(exist_ok=True)
    suffix = "-quick" if quick else ""
    header = "\n".join(f"- **{k}**: {v}" for k, v in meta.items())
    text = (f"# Research for the division-algebra family\n\n{header}\n\n"
            f"## B. Fixed-width uN tiers (baseline GMP mpz_t numerators over the shared denominator, from C)\n\n"
            "The uN vector, GMP mpz_t and FLINT fmpz all keep integers over the shared denominator and never reduce; "
            "GMP mpq_t reduces every operation, so its column measures rationals, not widths.\n\n"
            f"{RB.markdown()}\n\n"
            f"## C. Quaternion arrays (baseline FLINT fmpz over one denominator per value, from C)\n\n{RC.markdown()}\n\n"
            f"## D. Precision of each member\n\n{precision_markdown(prec)}\n\n"
            f"## E. Octonion arrays (baseline FLINT fmpz over one denominator per value, from C)\n\n{RE.markdown()}\n\n"
            "## F. The uniform header (octonion layout for every algebra, U16 base) against the fitted header\n\n"
            f"{UNI.markdown(uni)}\n\n"
            "Decoding a U64 value, median ns per value over random patterns (C, gcc -O3, branch-free sign):\n\n"
            "| Data | Fitted complex layout | Uniform layout | One decoder, layout given at run time |\n|---|---|---|---|\n"
            f"| 4096 values, in cache | {dec['fitted_cached']:.2f} | {dec['uniform_cached']:.2f} | {dec['runtime_cached']:.2f} |\n"
            f"| 10^7 values, from memory | {dec['fitted']:.2f} | {dec['uniform']:.2f} | {dec['runtime']:.2f} |\n\n"
            "## G. Exact matrix products with wide entries (baseline FLINT fmpz_mat_mul with BLAS, from C; one thread)\n\n"
            f"{RG.markdown()}\n")
    (out / f"family{suffix}.md").write_text(text)
    (out / f"family{suffix}.json").write_text(json.dumps({"metadata": meta, "B": RB.raw(), "C": RC.raw(),
                                                          "D": prec, "E": RE.raw(), "F": {"layouts": uni, "decode_ns": dec},
                                                          "G": RG.raw()}, indent=1, default=str))
    print(text)


if __name__ == "__main__":
    main()
