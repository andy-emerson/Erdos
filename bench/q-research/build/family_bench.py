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
                med, mn, w, got = inp.run(m, op, 0, reps=7)
                verify(got == want, f"{tag} {m}")
                report.add(tag, n, name, cbase.Stats(med, mn), baseline=(m == "mpz"),
                           error="0 (every element)" if op == "sq" else "0 (exact)",
                           note=f"vector width {w} bits" if m == "un" else "")
        if share:
            continue
        for K in steps:
            want = frac_chain(a, b, c, den, K)
            tag = f"B growth chain x <- x*b + c, {K} steps"
            for m, name in UR.NAMES.items():
                med, mn, w, got = inp.run(m, "chain", K, reps=5)
                if m == "un" and got is None:
                    report.add(tag, n, name, None, error="exceeds the 1024-bit cap",
                               note="detected before overflow; would continue in arbitrary precision")
                    continue
                verify(got == want, f"{tag} {m}")
                report.add(tag, n, name, cbase.Stats(med, mn), baseline=(m == "mpz"), error="0 (every element)",
                           note=f"ends at width {w} bits" if m == "un" else "")


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
    print("correctness tests")
    run_tests()
    meta = metadata()
    meta["compiled"] = ("section B (un.c, every method in it): clang 18 -O3 -march=native, for _BitInt; "
                        "section C (quat.c): gcc 13 -O3 -march=native; GMP 6.3.0, FLINT 3.0.1")
    un_n, q_n, q_n6, samples = (3000, 3000, 2000, 60) if quick else (10**5, 2 * 10**5, 10**5, 300)
    steps = (3, 10, 25) if quick else (3, 10, 25, 40, 70, 90)
    RB, RC = Report(), Report()
    for name, step in (("B uN", lambda: bench_un(RB, un_n, steps)),
                       ("C quaternions, 3 places", lambda: bench_quat(RC, q_n, 3, 10**3)),
                       ("C quaternions, 6 places", lambda: bench_quat(RC, q_n6, 6, 10**6))):
        t = time.perf_counter()
        step()
        print(f"  done  {name}  ({time.perf_counter() - t:.1f} s)", flush=True)
    t = time.perf_counter()
    prec = member_precision(samples)
    print(f"  done  D precision  ({time.perf_counter() - t:.1f} s)", flush=True)
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
            f"## D. Precision of each member\n\n{precision_markdown(prec)}\n")
    (out / f"family{suffix}.md").write_text(text)
    (out / f"family{suffix}.json").write_text(json.dumps({"metadata": meta, "B": RB.raw(), "C": RC.raw(),
                                                          "D": prec}, indent=1, default=str))
    print(text)


if __name__ == "__main__":
    main()
