# Benchmarks

## Contenders

- **A. Column operations** on $10^6$ decimals. Baseline: GMP `mpq_t` arrays in C. Also FLINT `fmpq` in C, GMP `mpz_t` and FLINT `fmpz` numerators over the shared denominator in C (FLINT stores small integers inline, so this is the closest existing analogue of the vector form), and int64 numerators in C: unchecked, with every overflow checked, and checked with the whole operation redone in 128-bit integers when anything overflows, which is the vector form's own strategy, compiled. The variance in C accumulates squares in 128-bit integers, as a C programmer would. From Python: FLINT `fmpq`, Python `Fraction` and Python `decimal` object arrays (`decimal` with 80 digits and every rounding trapped, so it is exact or fails), GMP `mpz` numerators over a shared denominator, NumPy int64 numerators over a shared denominator (exact only when nothing overflows; run only where that was proven, and it checks nothing), and the two full vector layouts. Statistics on object arrays use sums of $x$ and $x^2$ and one exact division, the efficient exact method. Group sums on object arrays are a loop, the standard way to group them.
- **U. Unfavourable workloads.** Per-element denominators that are unrelated (1 to 1000, so the common denominator has over a thousand bits) or drawn from a few small values (1 to 12); a growth chain of ten steps of $x \leftarrow x b + c$, in which the shared denominator grows a thousandfold each step; and $a b + c$ with no, one, 1% or all elements too wide for 64-bit products, since one wide element widens the whole vector while FLINT promotes element by element. For U1 and U2 the vector form is timed both with and without the conversion into it (the least common multiple and rescaling), since that conversion is part of its real cost. U4 also squares $ab$, so that one wide element pushes the whole vector past 128 bits into Python integers, against FLINT, which promotes element by element.
- **Summary.** A table of the signed vector form's time against GMP `mpq_t`, FLINT `fmpz` numerators, checked int64 and widening int64, all compiled, for every benchmark where they meet.
- **B. Matrix product.** Baseline: FLINT `fmpq_mat`. Also FLINT `fmpz_mat` and NumPy int64 on the numerators; NumPy float64 on the numerators, which is exact only while every partial sum stays below $2^{53}$ and checks nothing; and the exact float-BLAS product, which checks that bound and splits into limbs when it fails (6 and 9 places).
- **C. Linear solve.** Baseline: FLINT `fmpq_mat`. Also FLINT `fmpz_mat` (Dixon) and the v0.1 exact solver chain, which is not specific to Q72; it is reported for how many answers fit one Q72.
- **D. Pipelines.** Baseline: the same pipeline written as a loop over GMP `mpq` scalars; also over Python `Fraction`, and both vector layouts. All are checked against the v0.1 reference, written independently.
- **L. Layouts.** The three layouts' fused kernels, and the two exact-division methods.
- **E. The cost of exactness.** NumPy float64 against the signed vector form, for context.
- **F. Precision and exactness tables.**

Bytes per value count stored arrays; the column is blank where values are Python objects.

```python {file=build/run_bench.py}
import os

for var in ("OPENBLAS_NUM_THREADS", "OMP_NUM_THREADS", "MKL_NUM_THREADS"):
    os.environ[var] = "1"

import decimal
import json
import math
import sys
import time
from decimal import Decimal
from fractions import Fraction
from pathlib import Path

import numpy as np
import gmpy2

import reuse  # noqa: F401
import flint
from harness import Report, measure
from data import decimal_column, offset_column, group_keys, decimal_matrix
from run_benchmarks import metadata, verify
from run_integrated import e1_data, e1_python, e2_data, e2_python, e3_python
from core import exact_solve

import precision as P
import layouts as L
import svec as SV
from q72 import Q
from qvec import QVec
from svec import SVec
from cxcol import CxCol
from gemm import exact_gemm, Int128Matrix
import cbase

flint.ctx.threads = 1
# Python's decimal module, made exact: enough digits for every value here, and an error on any rounding.
decimal.setcontext(decimal.Context(prec=80, Emax=10**6, Emin=-10**6,
                                   traps=[decimal.Inexact, decimal.Overflow, decimal.Underflow,
                                          decimal.InvalidOperation, decimal.DivisionByZero]))

BYTES, SIGNED = "Q72 vector, turn bytes", "Q72 vector, signed"
VECS = {BYTES: QVec, SIGNED: SVec}
GMP, FMPQ, FRAC, DEC = "GMP mpq", "FLINT fmpq", "Python Fraction", "Python decimal"
MPZ, FIXED = "GMP mpz numerators", "NumPy int64 fixed point"
INT64_SAFE = 2**63 - 1
CGMP = cbase.NAMES["mpq"]       # the baseline: GMP rationals called from C


def c_rows(report, label, size, inputs, op, want, groups=1, kinds=("mpq", "fmpq", "mpz", "fmpz", "i64", "i64c", "i64w"),
           reps=7):
    """Time the compiled baselines and check every result line against the exact answer `want` (a list)."""
    for who in kinds:
        stats, lines = inputs.run(who, op, groups, reps)
        got = cbase.parse(lines)
        if got is None:
            report.add(label, size, cbase.NAMES[who], stats, baseline=(who == "mpq"), error="overflow detected",
                       note="result discarded; would need a wider type")
        elif got == want:
            report.add(label, size, cbase.NAMES[who], stats, baseline=(who == "mpq"), error="0 (every element)"
                       if len(want) > 1 else "0 (exact)", bytes_per_value=8 if who.startswith("i64") else None,
                       note="compiled; " + ("integers over one denominator" if who in ("mpz", "fmpz", "i64", "i64c", "i64w")
                                            else "rationals"))
        else:
            if who != "i64":     # only the unchecked contender may give a wrong answer
                raise SystemExit(f"verification failed: {label} {who}")
            report.add(label, size, cbase.NAMES[who], stats, error="wrong result", bytes_per_value=8,
                       note="compiled; nothing checks (int64 wrapped)")


def objects(kind, nums, den):
    """One column of decimals as an object array of the given exact type."""
    if kind == GMP:
        return np.array([gmpy2.mpq(int(v), den) for v in nums], dtype=object)
    if kind == FMPQ:
        return np.array([flint.fmpq(int(v), den) for v in nums], dtype=object)
    if kind == FRAC:
        return np.array([Fraction(int(v), den) for v in nums], dtype=object)
    places = round(math.log10(den))
    assert 10**places == den
    return np.array([Decimal(int(v)).scaleb(-places) for v in nums], dtype=object)


def as_fraction(v):
    if isinstance(v, Decimal):
        return Fraction(v)
    if isinstance(v, (Fraction, int)):
        return Fraction(v)
    return Fraction(int(v.numerator), int(v.denominator))


def nbytes(col):
    if isinstance(col, QVec):
        return {"n64": 9, "n128": 17}.get(col.tier)
    return {"s64": 8, "s128": 16}.get(col.tier)


def note_of(col):
    return f"tier {col.tier}"


def signed_over(col, D):
    """Every element of a real vector as a signed numerator over D (object array), or None."""
    D = int(D)
    if D % col.den:
        return None
    if isinstance(col, SVec):
        return None if (col.axis and not col.all_zero()) else col.big() * (D // col.den)
    if np.any((col.turn % 2 == 1) & ~col.is_zero()):
        return None
    return col.big() * (D // col.den) * np.where(col.turn == 2, -1, 1).astype(object)


def full_check(col, exact_num, D):
    got = signed_over(col, D)
    return got is not None and bool(np.all(got == exact_num))


def objects_check(arr, exact_num, D):
    return all(as_fraction(v) * D == e for v, e in zip(arr, exact_num))


def group_loop(keys, vals, groups, zero):
    """Per-group sums of an object array: the plain loop, which is how these types are grouped."""
    acc = [zero] * groups
    for k, v in zip(keys.tolist(), vals):
        acc[k] += v
    return acc


def obj_var(arr, n):
    s1 = arr.sum()
    s2 = (arr * arr).sum()
    return as_fraction(n * s2 - s1 * s1) / (n * (n - 1))


def int_var(nums_obj, n, den):
    s1 = nums_obj.sum()
    s2 = (nums_obj * nums_obj).sum()
    return Fraction(int(n * s2 - s1 * s1), n * (n - 1) * den * den)


# ---------------------------------------------------------------- A. column operations against the exact tools

def bench_sum_mean(report, n):
    nums, den, _ = decimal_column(n)
    objs = {k: objects(k, nums, den) for k in (GMP, FMPQ, FRAC, DEC)}
    mpz = np.array([gmpy2.mpz(int(v)) for v in nums], dtype=object)
    cols = {k: C.decimals(nums, den) for k, C in VECS.items()}
    exact = Fraction(int(nums.sum(dtype=object)), den)
    assert n * int(np.abs(nums).max()) <= INT64_SAFE
    for label, want, obj_fn, int_fn, vec_fn in (
            ("A1 sum", exact, lambda o: o.sum(), lambda s: Fraction(int(s), den), lambda c: c.total()),
            ("A2 mean", exact / n, lambda o: as_fraction(o.sum()) / n, lambda s: Fraction(int(s), den * n),
             lambda c: c.mean())):
        con = {**{k: (lambda o=o: obj_fn(o)) for k, o in objs.items()},
               MPZ: lambda: int_fn(mpz.sum()), FIXED: lambda: int_fn(nums.sum()),
               **{k: (lambda c=c: vec_fn(c)) for k, c in cols.items()}}
        res = measure(con, gc_off=False)
        report.add_measured(label, n, res, None, **{
            k: dict(error=verify(as_fraction(fn()) == want, f"{label} {k}", "0 (exact)"),
                    bytes_per_value=8 if k == FIXED else nbytes(cols[k]) if k in cols else None,
                    note="object array" if k in objs else "object array of numerators" if k == MPZ
                    else "int64 numerators, unchecked" if k == FIXED else note_of(cols[k]))
            for k, fn in con.items()})
        if label == "A1 sum":
            zeros = np.zeros(n, np.int64)
            c_rows(report, "A1 sum", n, cbase.Inputs(nums, zeros, zeros, zeros, den), "sum", [exact])


def bench_fma(report, n, digits, magnitude, label):
    (pa, den, _), (pb, _, _), (pc, _, _) = (decimal_column(n, digits, magnitude, seed=s) for s in (5, 6, 7))
    objs = {k: [objects(k, x, den) for x in (pa, pb, pc)] for k in (GMP, FMPQ, FRAC, DEC)}
    mpz = [np.array([gmpy2.mpz(int(v)) for v in x], dtype=object) for x in (pa, pb, pc)]
    cols = {k: [C.decimals(x, den) for x in (pa, pb, pc)] for k, C in VECS.items()}
    exact_num = pa.astype(object) * pb.astype(object) + pc.astype(object) * den
    D = den * den
    fixed_ok = int(np.abs(pa).max()) * int(np.abs(pb).max()) + int(np.abs(pc).max()) * den <= INT64_SAFE
    con = {**{k: (lambda o=o: o[0] * o[1] + o[2]) for k, o in objs.items()},
           MPZ: lambda: mpz[0] * mpz[1] + mpz[2] * den,
           **({FIXED: lambda: pa * pb + pc * den} if fixed_ok else {}),
           **{k: (lambda c=c: c[0] * c[1] + c[2]) for k, c in cols.items()}}
    res = measure(con, gc_off=False)
    extra = {}
    for k, fn in con.items():
        out = fn()
        if k in objs:
            ok, note, nb = objects_check(out, exact_num, D), "object array", None
        elif k == MPZ:
            ok, note, nb = bool(np.all(np.array([int(v) for v in out], dtype=object) == exact_num)), \
                "object array of numerators", None
        elif k == FIXED:
            ok, note, nb = bool(np.all(out.astype(object) == exact_num)), "int64 numerators, unchecked", 8
        else:
            ok, note, nb = full_check(out, exact_num, D), note_of(out), nbytes(out)
        extra[k] = dict(error=verify(ok, f"{label} {k}", "0 (every element)"), bytes_per_value=nb, note=note)
    report.add_measured(label, n, res, None, **extra)
    inputs = cbase.Inputs(pa, pb, pc, np.zeros(n, np.int64), den)
    c_rows(report, label, n, inputs, "fma", [Fraction(int(v), D) for v in exact_num])
    if not fixed_ok:
        report.add(label, n, FIXED, None, note="not run: a*b overflows int64")


def bench_groupby(report, n, groups=1000):
    nums, den, _ = decimal_column(n, seed=8)
    keys = group_keys(n, groups)
    objs = {k: objects(k, nums, den) for k in (GMP, FMPQ, FRAC, DEC)}
    zeros = {GMP: gmpy2.mpq(0), FMPQ: flint.fmpq(0), FRAC: Fraction(0), DEC: Decimal(0)}
    mpz = np.array([gmpy2.mpz(int(v)) for v in nums], dtype=object)
    cols = {k: C.decimals(nums, den) for k, C in VECS.items()}
    ref = np.zeros(groups, dtype=object)
    np.add.at(ref, keys, nums.astype(object))
    want = [Fraction(int(v), den) for v in ref]

    def fixed():
        out = np.zeros(groups, dtype=np.int64)
        np.add.at(out, keys, nums)
        return out

    con = {**{k: (lambda k=k, o=o: group_loop(keys, o, groups, zeros[k])) for k, o in objs.items()},
           MPZ: lambda: group_loop(keys, mpz, groups, gmpy2.mpz(0)), FIXED: fixed,
           **{k: (lambda c=c: c.group_numerators(keys, groups)) for k, c in cols.items()}}
    res = measure(con, gc_off=False)
    extra = {}
    for k, fn in con.items():
        out = fn()
        if k in cols:
            nums_, d = out
            got = [Fraction(int(v), d) for v in nums_]
        else:
            got = [Fraction(int(v), den) for v in out] if k in (MPZ, FIXED) else [as_fraction(v) for v in out]
        extra[k] = dict(error=verify(got == want, f"groupby {k}", "0 (exact)"),
                        note="loop over object array" if k in objs or k == MPZ
                        else "np.add.at, unchecked" if k == FIXED else "integer numerators over one denominator")
    report.add_measured("A5 group-by sum", f"{n}, G={groups}", res, None, **extra)
    zeros = np.zeros(n, np.int64)
    c_rows(report, "A5 group-by sum", f"{n}, G={groups}", cbase.Inputs(nums, zeros, zeros, keys, den), "group",
           want, groups)


def bench_variance(report, n):
    for label, (nums, den, _) in (("A6 variance", decimal_column(n, seed=9)),
                                  ("A7 variance, values near 1e6", offset_column(n))):
        objs = {k: objects(k, nums, den) for k in (GMP, FMPQ, FRAC, DEC)}
        mpz = np.array([gmpy2.mpz(int(v)) for v in nums], dtype=object)
        cols = {k: C.decimals(nums, den) for k, C in VECS.items()}
        xs = list(map(int, nums.tolist()))
        tot = sum(xs)
        want = Fraction(sum((n * x - tot) ** 2 for x in xs), n * n * (n - 1) * den * den)
        con = {**{k: (lambda o=o: obj_var(o, n)) for k, o in objs.items()},
               MPZ: lambda: int_var(mpz, n, den),
               **{k: (lambda c=c: c.var()) for k, c in cols.items()}}
        res = measure(con, gc_off=False)
        report.add_measured(label, n, res, None, **{
            k: dict(error=verify(fn() == want, f"{label} {k}", "0 (exact)"),
                    note="sums of x and x^2, one exact division" if (k in objs or k == MPZ) else "")
            for k, fn in con.items()})
        zeros = np.zeros(n, np.int64)
        c_rows(report, label, n, cbase.Inputs(nums, zeros, zeros, zeros, den), "var", [want])
        report.add(label, n, FIXED, None,
                   note=f"not run: the sum of squares reaches {float(n) * float(np.abs(nums).max()) ** 2:.0e}, past int64")


# ---------------------------------------------------------------- U. workloads that do not favour the vector form

def vec_of(fracs):
    """A signed vector over the least common multiple of the denominators (the vector form's only option)."""
    L = math.lcm(*[f.denominator for f in fracs])
    return SVec.from_ints([f.numerator * (L // f.denominator) for f in fracs], L)


def bench_unfavourable(report, n, chain_n):
    rng = np.random.default_rng(70)

    def fractions(nums, dens):
        return [Fraction(int(a), int(b)) for a, b in zip(nums, dens)]

    # U1, U2: per-element denominators, unrelated (1..1000) or drawn from a few small values (1..12)
    for label, dmax in (("U1 unrelated denominators 1..1000", 1000), ("U2 mixed small denominators 1..12", 12)):
        nums = [rng.integers(-10**6, 10**6 + 1, size=n) for _ in range(3)]
        dens = [rng.integers(1, dmax + 1, size=n) for _ in range(3)]
        F = [fractions(x, q) for x, q in zip(nums, dens)]
        V = [vec_of(f) for f in F]
        tiers = V[0].tier
        L_bits = V[0].den.bit_length()
        mpq_obj = [np.array([gmpy2.mpq(f.numerator, f.denominator) for f in col], dtype=object) for col in F]
        want_sum = sum(F[0], Fraction(0))
        want_fma = [x * y + z for x, y, z in zip(*F)]
        inputs = cbase.Inputs(nums[0], nums[1], nums[2], np.zeros(n, np.int64), 0, dens[0], dens[1], dens[2])
        WITH = SIGNED + ", including conversion"
        for op, want, vec_fn, conv_fn, obj_fn in (
                ("sum", [want_sum], lambda: V[0].total(), lambda: vec_of(F[0]).total(), lambda: mpq_obj[0].sum()),
                ("fma", want_fma, lambda: V[0] * V[1] + V[2],
                 lambda: vec_of(F[0]) * vec_of(F[1]) + vec_of(F[2]), lambda: mpq_obj[0] * mpq_obj[1] + mpq_obj[2])):
            res = measure({WITH: conv_fn, SIGNED: vec_fn, GMP: obj_fn}, gc_off=False)
            got_v = vec_fn()
            ok_v = (got_v == want[0]) if op == "sum" else [v.fraction() for v in got_v.values()] == want
            got_o = obj_fn()
            ok_o = (as_fraction(got_o) == want[0]) if op == "sum" else [as_fraction(v) for v in got_o] == want
            tag = f"{label}, {op}"
            got_w = conv_fn()
            ok_w = (got_w == want[0]) if op == "sum" else [v.fraction() for v in got_w.values()] == want
            report.add_measured(tag, n, res, None, **{
                WITH: dict(error=verify(ok_w, tag + " with conversion"),
                           note="least common multiple and rescaling, then the operation"),
                SIGNED: dict(error=verify(ok_v, tag), note=f"conversion excluded; common denominator {L_bits} bits; "
                                                         f"tier {tiers}"),
                GMP: dict(error=verify(ok_o, tag + " mpq"), note="object array")})
            c_rows(report, tag, n, inputs, op, want, kinds=("mpq", "fmpq"), reps=5)
        res = measure({"conversion into the vector form": lambda: vec_of(F[0])}, gc_off=False)
        verify(vec_of(F[0]).values() == V[0].values(), f"{label} conversion")
        report.add_measured(f"{label}, conversion", n, res, None, **{
            "conversion into the vector form": dict(error="0 (exact)",
                                                    note="least common multiple, then scaling every numerator")})

    # U3: a growth chain x <- x*b + c, ten steps, 3-place decimals: the shared denominator grows 10^3 a step
    steps = 10
    pa, den, _ = decimal_column(chain_n, 3, 10**3, seed=71)
    pb, _, _ = decimal_column(chain_n, 3, 10, seed=72)
    pc, _, _ = decimal_column(chain_n, 3, 10**3, seed=73)
    Va, Vb, Vc = (SVec.decimals(x, den) for x in (pa, pb, pc))
    Fa, Fb, Fc = (fractions(x, np.full(chain_n, den)) for x in (pa, pb, pc))
    want = Fa
    for _ in range(steps):
        want = [x * y + z for x, y, z in zip(want, Fb, Fc)]

    def vec_chain():
        x = Va
        for _ in range(steps):
            x = x * Vb + Vc
        return x

    res = measure({SIGNED: vec_chain}, gc_off=False)
    out = vec_chain()
    tag = f"U3 growth chain, {steps} steps of x*b + c"
    report.add_measured(tag, chain_n, res, None, **{
        SIGNED: dict(error=verify([v.fraction() for v in out.values()] == want, tag),
                     note=f"ends in tier {out.tier}, denominator {out.den.bit_length()} bits")})
    inputs = cbase.Inputs(pa, pb, pc, np.zeros(chain_n, np.int64), den)
    c_rows(report, tag, chain_n, inputs, "chain", want, groups=steps, kinds=("mpq", "fmpq"), reps=5)

    # U4: one wide element widens the whole vector; FLINT's integers promote element by element
    pa, den, _ = decimal_column(n, 3, 10**4, seed=74)
    pb, _, _ = decimal_column(n, 3, 10**4, seed=75)
    pc, _, _ = decimal_column(n, 3, 10**4, seed=76)
    for share, label in ((0, "none"), (1, "one element"), (n // 100, "1%"), (n, "all")):
        a = pa.copy()
        idx = rng.choice(n, size=share, replace=False) if share else np.array([], dtype=np.int64)
        a[idx] = rng.integers(10**15, 10**16, size=share) * np.where(rng.random(share) < 0.5, -1, 1)
        V = [SVec.decimals(x, den) for x in (a, pb, pc)]
        exact = a.astype(object) * pb.astype(object) + pc.astype(object) * den
        res = measure({SIGNED: lambda: V[0] * V[1] + V[2]}, gc_off=False)
        out = V[0] * V[1] + V[2]
        tag = f"U4 a*b+c, wide elements: {label}"
        report.add_measured(tag, n, res, None, **{
            SIGNED: dict(error=verify(full_check(out, exact, den * den), tag, "0 (every element)"),
                         note=f"result tier {out.tier}")})
        inputs = cbase.Inputs(a, pb, pc, np.zeros(n, np.int64), den)
        c_rows(report, tag, n, inputs, "fma", [Fraction(int(v), den * den) for v in exact],
               kinds=("mpq", "fmpz", "i64c", "i64w"), reps=5)
        # (a*b)^2 needs more than 128 bits as soon as one element is wide: the vector falls to Python integers
        exact2 = (a.astype(object) * pb.astype(object)) ** 2
        res = measure({SIGNED: lambda: (V[0] * V[1]) * (V[0] * V[1])}, gc_off=False)
        out = (V[0] * V[1]) * (V[0] * V[1])
        tag2 = f"U4 (a*b)^2, wide elements: {label}"
        report.add_measured(tag2, n, res, None, **{
            SIGNED: dict(error=verify(full_check(out, exact2, den**4), tag2, "0 (every element)"),
                         note=f"result tier {out.tier}")})
        c_rows(report, tag2, n, inputs, "sq", [Fraction(int(v), den**4) for v in exact2], kinds=("mpq", "mpz", "fmpz"),
               reps=5)


# ---------------------------------------------------------------- B. matrix product

def bench_matmul(report, n, digits=3, magnitude=10**3, label="B1 matrix product"):
    N1, den, _ = decimal_matrix(n, n, digits, magnitude, seed=10)
    N2, _, _ = decimal_matrix(n, n, digits, magnitude, seed=11)
    F1, F2 = flint.fmpz_mat(N1.tolist()), flint.fmpz_mat(N2.tolist())
    Q1, Q2 = flint.fmpq_mat(F1) / den, flint.fmpq_mat(F2) / den
    bound = int(np.abs(N1).max()) * int(np.abs(N2).max()) * n
    int64_ok = bound <= INT64_SAFE
    F64 = "NumPy float64 matmul (numerators, unchecked)"
    G = "exact float-BLAS product (numerators)"
    Nf1, Nf2 = N1.astype(np.float64), N2.astype(np.float64)
    con = {"FLINT fmpq_mat": lambda: Q1 * Q2, "FLINT fmpz_mat (numerators)": lambda: F1 * F2,
           **({"NumPy int64 matmul (numerators)": lambda: N1 @ N2} if int64_ok else {}),
           F64: lambda: Nf1 @ Nf2, G: lambda: exact_gemm(N1, N2)}
    res = measure(con)
    want = F1 * F2
    C = exact_gemm(N1, N2)
    rows = C.to_python() if isinstance(C, Int128Matrix) else C.tolist()
    ok = flint.fmpz_mat([[int(v) for v in row] for row in rows]) == want
    f64_exact = bool(np.array_equal(np.rint(Nf1 @ Nf2).astype(object),
                                    np.array([[int(want[i, j]) for j in range(n)] for i in range(n)], dtype=object)))
    report.add_measured(label, f"{n}, {digits} places", res, "FLINT fmpq_mat", **{
        F64: dict(error="0 (exact here)" if f64_exact else "wrong: rounding",
                  note=("bound below 2^53, so exact" if bound < 2**53 else "bound past 2^53") + "; nothing checks it"),
        "FLINT fmpq_mat": dict(error=verify(Q1 * Q2 == flint.fmpq_mat(want) / (den * den), label), note=""),
        "FLINT fmpz_mat (numerators)": dict(error="0 (exact)", note="shared denominator applied separately"),
        "NumPy int64 matmul (numerators)": dict(
            error=verify(flint.fmpz_mat((N1 @ N2).tolist()) == want, f"{label} int64") if int64_ok else None,
            note="no BLAS for integers; unchecked"),
        G: dict(error=verify(ok, label, "0 (equals FLINT)"),
                note=("one float GEMM" if bound < 2**53 else "split into limbs, ")
                + ("" if bound < 2**53 else ("128-bit result" if isinstance(C, Int128Matrix) else "int64 result"))
                + "; exact by bound"),
    })
    if not int64_ok:
        report.add(label, f"{n}, {digits} places", "NumPy int64 matmul (numerators)", None, note="not run: overflows int64")


# ---------------------------------------------------------------- C. linear solve

def q72_solve(N, b):
    """Solve N x = b exactly with the numeric-symbolic / Dixon / FLINT chain; report how many entries fit one Q72."""
    nums, d, method = exact_solve(N, b)
    xs = [Fraction(v, d) for v in nums]
    return xs, method, sum(Q.of(v).fits() for v in xs)


def bench_solve(report, n):
    N, den, _ = decimal_matrix(n, n, seed=12)
    for label, M in (("C1 solve, random right-hand side", decimal_matrix(n, 1, seed=13)[0][:, 0]),
                     ("C2 solve, planted integer answer", N @ np.random.default_rng(17).integers(-1000, 1001, size=n))):
        M = np.ascontiguousarray(M)
        FQ = flint.fmpq_mat(flint.fmpz_mat(N.tolist())) / den
        FB = flint.fmpq_mat(n, 1, [flint.fmpq(int(v), den) for v in M])
        FZ, FZb = flint.fmpz_mat(N.tolist()), flint.fmpz_mat(n, 1, [int(v) for v in M])
        X = FZ.solve(FZb)
        want = [Fraction(int(X[i, 0].p), int(X[i, 0].q)) for i in range(n)]
        xs, method, fit = q72_solve(N, M)
        verify(xs == want, label)
        FX = FQ.solve(FB)
        verify([Fraction(int(FX[i, 0].p), int(FX[i, 0].q)) for i in range(n)] == want, f"{label} fmpq_mat")
        res = measure({"FLINT fmpq_mat": lambda: FQ.solve(FB), "FLINT fmpz_mat": lambda: FZ.solve(FZb),
                       "exact solver chain (v0.1)": lambda: q72_solve(N, M)}, gc_off=False)
        report.add_measured(label, n, res, "FLINT fmpq_mat", **{
            "FLINT fmpq_mat": dict(error="0 (verified)"),
            "FLINT fmpz_mat": dict(error="0 (exact)", note="Dixon p-adic"),
            "exact solver chain (v0.1)": dict(error="0 (equals FLINT)",
                                              note=f"numeric-symbolic, Dixon, FLINT; used: {method}; {fit}/{n} entries fit one Q72"),
        })


# ---------------------------------------------------------------- D. pipelines

def e1_generic(X, y, C):
    n, p = X.shape
    cols = [C.decimals(X[:, j], 100) for j in range(p)] + [C.decimals(np.ones(n, dtype=np.int64), 1)]
    ycol = C.decimals(y, 10**5)
    k = len(cols)
    G = [[None] * k for _ in range(k)]
    for i in range(k):
        for j in range(i, k):
            G[i][j] = G[j][i] = (cols[i] * cols[j]).total()
    r = [(c * ycol).total() for c in cols]
    L = math.lcm(*[v.denominator for row in G for v in row], *[v.denominator for v in r])
    nums, d, method = exact_solve([[int(v * L) for v in row] for row in G], [int(v * L) for v in r])
    beta = [Fraction(v, d) for v in nums]
    return beta, f"solver: {method}; {sum(Q.of(b).fits() for b in beta)}/{len(beta)} coefficients fit one Q72"


def e2_generic(D, C, groups=50):
    n = len(D["price"])
    one = C.decimals(np.ones(n, dtype=np.int64), 1)
    price, qty = C.decimals(D["price"], 100), C.decimals(D["qty"], 1)
    disc, fx, tax = C.decimals(D["disc"], 1000), C.decimals(D["fx"], 10**6), C.decimals(D["tax"], 10**4)
    rev = price * qty * (one - disc)
    conv = rev * fx * (one + tax)
    totals = conv.group_sum(D["region"], groups)
    temp = C.decimals(D["temp"], 1000).group_mean_var(D["region"], groups)
    big_rev = rev.take(rev.greater(1000)).total()
    return totals + [m for m, _ in temp] + [v for _, v in temp] + [big_rev], \
        f"revenue: {note_of(rev)}; converted: {note_of(conv)}"


def e3_generic(x, C):
    x0, x1, x2, x3 = (CxCol.real(C.decimals(np.ascontiguousarray(x[j::4]), 1000)) for j in range(4))
    A, B = x0 - x2, x1 - x3
    X = [x0 + x1 + x2 + x3, A + B.rotate(3), x0 - x1 + x2 - x3, A + B.rotate(1)]
    cross = (X[1] * X[2]).total2()
    return [Xk.abs2().total() for Xk in X] + [cross[0], cross[1]], f"bin kinds {[Xk.kind for Xk in X]}"


def e1_exact(X, y, T):
    """The regression written directly in an exact scalar type T (GMP mpq or Fraction)."""
    from run_integrated import frac_gauss
    n, p = X.shape
    cols = [[T(int(v), 100) for v in X[:, j]] for j in range(p)] + [[T(1)] * n]
    ys = [T(int(v), 10**5) for v in y]
    k = len(cols)
    Gm = [[None] * k for _ in range(k)]
    for i in range(k):
        for j in range(i, k):
            Gm[i][j] = Gm[j][i] = sum((a * b for a, b in zip(cols[i], cols[j])), T(0))
    r = [sum((a * b for a, b in zip(c, ys)), T(0)) for c in cols]
    return frac_gauss(Gm, r)


def e2_exact(D, T, groups=50):
    tot = [T(0)] * groups
    s1, s2, cnt = [T(0)] * groups, [T(0)] * groups, [0] * groups
    big_rev = T(0)
    one = T(1)
    for pr, q, dc, g, f, tx, tp in zip(*(D[k].tolist() for k in ("price", "qty", "disc", "region", "fx", "tax", "temp"))):
        rev = T(pr, 100) * q * (one - T(dc, 1000))
        tot[g] += rev * T(f, 10**6) * (one + T(tx, 10**4))
        x = T(tp, 1000)
        s1[g] += x
        s2[g] += x * x
        cnt[g] += 1
        if rev > 1000:
            big_rev += rev
    means = [a / n for a, n in zip(s1, cnt)]
    vars_ = [(n * b - a * a) / (n * (n - 1)) for a, b, n in zip(s1, s2, cnt)]
    return tot + means + vars_ + [big_rev]


def e3_exact(x, T):
    """The four-point DFT stage on (re, im) pairs of T, multiplying by +-i as a rotation."""
    vals = [T(int(v), 1000) for v in x]
    zero = T(0)
    power = [zero] * 4
    cre, cim = zero, zero
    for b in range(0, len(vals), 4):
        x0, x1, x2, x3 = vals[b:b + 4]
        a_re, b_re = x0 - x2, x1 - x3          # A = x0 - x2 and B = x1 - x3 are real
        X = [(x0 + x1 + x2 + x3, zero), (a_re + zero, -b_re), (x0 - x1 + x2 - x3, zero), (a_re + zero, b_re)]
        for k in range(4):
            power[k] += X[k][0] * X[k][0] + X[k][1] * X[k][1]
        (p, q), (r, s_) = X[1], X[2]
        cre += p * r - q * s_
        cim += p * s_ + q * r
    return power + [cre, cim]


def bench_pipelines(report, sizes):
    for label, size, make, gen, ex, py in (
        ("D1 regression", sizes["e1"], lambda: e1_data(sizes["e1"], 8), e1_generic, e1_exact, e1_python),
        ("D2 table pipeline", sizes["e2"], lambda: (e2_data(sizes["e2"]),), e2_generic, e2_exact, e2_python),
        ("D3 complex pipeline", sizes["e3"],
         lambda: (np.random.default_rng(112).integers(-10**6, 10**6 + 1, size=sizes["e3"], dtype=np.int64),),
         e3_generic, e3_exact, e3_python),
    ):
        args = make()
        ref = py(*args)                               # the v0.1 reference, written independently
        notes = {}
        for k, C in VECS.items():
            out, notes[k] = gen(*args, C)
            verify(out == ref, f"{label} {k}")
        for T, k in ((gmpy2.mpq, GMP), (Fraction, FRAC)):
            verify([as_fraction(v) for v in ex(*args, T)] == ref, f"{label} {k}")
        res = measure({GMP: lambda: ex(*args, gmpy2.mpq), FRAC: lambda: ex(*args, Fraction),
                       **{k: (lambda C=C: gen(*args, C)) for k, C in VECS.items()}}, gc_off=False)
        report.add_measured(label, size, res, GMP, **{
            GMP: dict(error="0 (matches reference)", note="loop over scalars"),
            FRAC: dict(error="0 (matches reference)", note="loop over scalars"),
            **{k: dict(error="0 (matches reference)", note=notes[k]) for k in VECS}})


# ---------------------------------------------------------------- L. the layout experiment

def bench_layouts(report, n):
    """The three layouts of the vector form, doing the same work with the same overflow proof."""
    (pa, den, _), (pb, _, _), (pc, _, _) = (decimal_column(n, 3, 10**4, seed=s) for s in (5, 6, 7))
    keys = group_keys(n, 1000)
    oa, ob, oc = (x.astype(object) for x in (pa, pb, pc))
    want_fma = oa * ob + oc * den                      # Python integers, no wrapping
    g = np.zeros(1000, dtype=object)
    np.add.at(g, keys, oa)
    total, sq = int(oa.sum()), int((oa * oa).sum())
    # the caller-side proof every layout relies on: results fit one word, sums of squares split exactly
    bits = lambda x: int(np.abs(x).max()).bit_length()
    assert bits(pa) + bits(pb) + 1 <= 62 and bits(pc) + den.bit_length() + 1 <= 62
    assert bits(pa) < 32 and n < 2**31 and bits(pa) + n.bit_length() <= 63
    lay = {k: [L.Layout(k, x) for x in (pa, pb, pc)] for k in L.KINDS}
    nb = {k: v[0].nbytes() / n for k, v in lay.items()}
    for label, fn, check in (
            ("L1 convert from int64 numerators", lambda k: L.Layout(k, pa),
             lambda k, r: np.array_equal(r.signed().astype(object), oa)),
            ("L2 sum", lambda k: lay[k][0].sum(), lambda k, r: int(r) == total),
            ("L3 a*b+c", lambda k: lay[k][0].fma(lay[k][1], lay[k][2], den),
             lambda k, r: bool(np.all(r.signed().astype(object) == want_fma))),
            ("L4 group-by sum, G=1000", lambda k: lay[k][0].group_sum(keys, 1000),
             lambda k, r: bool(np.all(r.view(np.int64).astype(object) == g))),
            ("L5 sum and sum of squares", lambda k: lay[k][0].moments(),
             lambda k, r: int(r[0]) == total and (int(r[-2]) << 32) + int(r[-1]) == sq)):
        res = measure({k: (lambda k=k: fn(k)) for k in L.KINDS})
        report.add_measured(label, n, res, "turn bytes", **{
            k: dict(error=verify(check(k, fn(k)), f"{label} {k}", "0 (exact)"), bytes_per_value=nb[k]) for k in L.KINDS})
    x = pa * np.int64(48)
    res = {}
    for method in ("hardware", "inverse"):
        SV.DIVISION = method
        res[method] = measure({method: lambda: SV.exact_divide(x, 48)})[method]
        verify(np.array_equal(SV.exact_divide(x, 48), pa), f"exact division {method}")
    SV.DIVISION = "inverse"
    report.add("L6 exact division by a common factor", n, "divide instruction", res["hardware"], baseline=True,
               error="0 (exact)", note="unsigned divide of the magnitude, sign restored")
    report.add("L6 exact division by a common factor", n, "multiply by 2-adic inverse", res["inverse"],
               error="0 (exact)", note="quote notation's division")


# ---------------------------------------------------------------- E. the cost of exactness

def bench_cost(report, n, mat_n):
    """One line of context: NumPy float64, which rounds, against the signed vector form."""
    (pa, den, a), (pb, _, b), (pc, _, c) = (decimal_column(n, 3, 10**4, seed=s) for s in (5, 6, 7))
    cols = [SVec.decimals(x, den) for x in (pa, pb, pc)]
    N1, mden, A1 = decimal_matrix(mat_n, mat_n, 6, 10**3, seed=10)   # 6 places: the exact product splits into limbs
    N2, _, A2 = decimal_matrix(mat_n, mat_n, 6, 10**3, seed=11)
    nums = [x.astype(object) for x in (pa, pb, pc)]
    exact_fma = nums[0] * nums[1] + nums[2] * den
    xs = list(map(int, pa.tolist()))
    want = {
        "E1 sum": lambda r: r == Fraction(sum(xs), den),
        "E2 a*b+c": lambda r: full_check(r, exact_fma, den * den),
        "E3 variance": lambda r: r == Fraction(len(xs) * sum(x * x for x in xs) - sum(xs) ** 2,
                                               len(xs) * (len(xs) - 1) * den * den),
        f"E4 matrix product {mat_n}, 6 places": lambda r: flint.fmpz_mat(
            r.to_python() if isinstance(r, Int128Matrix) else r.tolist()) == flint.fmpz_mat(N1.tolist()) * flint.fmpz_mat(N2.tolist()),
    }
    for label, f64, q in (("E1 sum", lambda: a.sum(), lambda: cols[0].total()),
                          ("E2 a*b+c", lambda: a * b + c, lambda: cols[0] * cols[1] + cols[2]),
                          ("E3 variance", lambda: a.var(ddof=1), lambda: cols[0].var()),
                          (f"E4 matrix product {mat_n}, 6 places", lambda: A1 @ A2, lambda: exact_gemm(N1, N2))):
        name = SIGNED if not label.startswith("E4") else "exact float-BLAS product"
        res = measure({"NumPy float64": f64, name: q})
        report.add_measured(label, n if not label.startswith("E4") else mat_n, res, "NumPy float64",
                            **{"NumPy float64": dict(note="rounds"), name: dict(error=verify(want[label](q()), label))})


# ---------------------------------------------------------------- F. precision and exactness

def precision_markdown(samples):
    typ = P.typical_errors(samples=samples)
    worst = P.worst_case_profile()
    lines = ["| Magnitude | float64 median | Q72 median | float64 worst (sampled) | Q72 worst (sampled) | Q72 worst in [10^k/10, 10^k*10] |",
             "|---|---|---|---|---|---|"]
    for t, w in zip(typ, worst):
        q_max = "some samples out of range" if math.isinf(t["q72_max"]) else f"{t['q72_max']:.1e}"
        edge = " (gap to zero, at the bottom of the range)" if w["at"] == str(P.SMALLEST) else ""
        lines.append(f"| 1e{t['exp']} | {t['f64_median']:.1e} | {t['q72_median']:.1e} | {t['f64_max']:.1e} | "
                     f"{q_max} | {w['worst']:.1e} (beside {w['at']}){edge} |")
    med = float(np.median([t["q72_median"] for t in typ]))
    p_med, bits_med = P.float_median_bits(med)
    w_max = max(w["worst"] for w in worst if w["at"] != str(P.SMALLEST))  # excluding underflow to zero, as for floats
    p_w, bits_w = P.float_bits_to_match(w_max)
    lines += ["", "| To match Q72's ... | needs a float significand of | an IEEE-style float of (with an 8-bit exponent) |",
              "|---|---|---|",
              f"| typical (median) precision, {med:.1e} | {p_med} bits | {bits_med} bits |",
              f"| worst-case precision, {w_max:.1e} | {p_w} bits | {bits_w} bits |",
              "", "Q72 is 72 bits. Its range is 2^-63 to 2^64 - 1 (about 1.1e-19 to 1.8e19); float64's is about 1e-308 to 1e308."]
    rates = P.exactness_rates()
    lines += ["", "| Values | exact in binary64 | exact in decimal64 | exact in Q72 |", "|---|---|---|---|"]
    lines += [f"| {r['values']} | {100 * r['binary64']:.3g}% | {100 * r['decimal64']:.3g}% | {100 * r['q72']:.3g}% |"
              for r in rates]
    return "\n".join(lines), dict(typical=typ, worst=worst, median_bits=[p_med, bits_med], worst_bits=[p_w, bits_w],
                                  exactness=rates)


def key_ratios(reports):
    """Time of the signed vector form (including conversion into it, where that was timed) relative to the
    compiled contenders that matter, per benchmark."""
    rows = {}
    for rep_ in reports:
        for r in rep_.rows:
            if r["stats"] is not None:
                rows.setdefault((r["bench"], str(r["size"])), {})[r["name"]] = r["stats"].median
    others = [cbase.NAMES[k] for k in ("mpq", "fmpz", "i64c", "i64w")]
    out = ["| Benchmark | Size | " + " | ".join(f"vs {o}" for o in others) + " |",
           "|---|---|" + "---|" * len(others)]
    for (bench, size), t in rows.items():
        mine = t.get(SIGNED + ", including conversion", t.get(SIGNED))   # count the conversion where it was timed
        if mine is None:
            continue
        cells = [f"{t[o] / mine:.3g}x faster" if o in t and t[o] >= mine else
                 f"{mine / t[o]:.3g}x slower" if o in t else "" for o in others]
        out.append(f"| {bench} | {size} | " + " | ".join(cells) + " |")
    return "\n".join(out)


def run_tests():
    import test_scalar
    import test_columns
    import test_precision
    for mod in (test_scalar, test_columns, test_precision):
        for name in sorted(x for x in dir(mod) if x.startswith("test_")):
            t = time.perf_counter()
            getattr(mod, name)()
            print(f"  pass  {mod.__name__}.{name}  ({time.perf_counter() - t:.1f} s)", flush=True)


def main():
    quick = "--quick" in sys.argv
    print("correctness tests")
    run_tests()
    meta = metadata()
    meta["decimal"] = f"libmpdec {decimal.__libmpdec_version__}, precision 80, Inexact trapped"
    meta["gmpy2"] = gmpy2.version()
    meta["compiled baselines"] = "gcc -O3 -march=native; GMP 6.3.0, FLINT 3.0.1 (system packages)"
    col_n, lay_n = (2 * 10**4, 10**5) if quick else (10**6, 10**7)
    unf_n, chain_n = (5000, 2000) if quick else (10**5, 2 * 10**4)
    mats = (100,) if quick else (500, 1000)
    solves = (60,) if quick else (100, 500)
    sizes = dict(e1=2000, e2=5000, e3=4000) if quick else dict(e1=20000, e2=10**5, e3=10**5)
    A, B, C, D, Lr, E, Ur = Report(), Report(), Report(), Report(), Report(), Report(), Report()
    cbase.build()
    steps = [("A1-A2 sum and mean", lambda: bench_sum_mean(A, col_n)),
             ("A3 a*b+c", lambda: bench_fma(A, col_n, 3, 10**4, "A3 a*b+c, 3 places")),
             ("A4 a*b+c, 6 places", lambda: bench_fma(A, col_n, 6, 10**4, "A4 a*b+c, 6 places")),
             ("A5 group-by", lambda: bench_groupby(A, col_n)),
             ("A6-A7 variance", lambda: bench_variance(A, col_n)),
             ("U unfavourable", lambda: bench_unfavourable(Ur, unf_n, chain_n)),
             *[(f"B matmul {n}", lambda n=n: bench_matmul(B, n)) for n in mats],
             (f"B matmul {mats[0]}, 6 places", lambda: bench_matmul(B, mats[0], 6, 10**3, "B2 matrix product, 6 places")),
             (f"B matmul {mats[0]}, 9 places", lambda: bench_matmul(B, mats[0], 9, 10**3, "B3 matrix product, 9 places")),
             *[(f"C solve {n}", lambda n=n: bench_solve(C, n)) for n in solves],
             ("D pipelines", lambda: bench_pipelines(D, sizes)),
             ("L layouts", lambda: bench_layouts(Lr, lay_n)),
             ("E cost of exactness", lambda: bench_cost(E, col_n, mats[-1]))]
    for name, step in steps:
        t = time.perf_counter()
        step()
        print(f"  done  {name}  ({time.perf_counter() - t:.1f} s)", flush=True)
    t = time.perf_counter()
    prec_md, prec = precision_markdown(60 if quick else 300)
    print(f"  done  precision  ({time.perf_counter() - t:.1f} s)", flush=True)
    out = Path(__file__).resolve().parent.parent / "results"
    out.mkdir(exist_ok=True)
    suffix = "-quick" if quick else ""
    header = "\n".join(f"- **{k}**: {v}" for k, v in meta.items())
    text = (f"# The Q72 vector form against the exact tools\n\n{header}\n\n"
            f"## A. Column operations (baseline GMP mpq_t from C)\n\n{A.markdown()}\n\n"
            f"## U. Workloads that do not favour the vector form (baseline GMP mpq_t from C)\n\n{Ur.markdown()}\n\n"
            f"## Summary: the signed vector form against the compiled contenders\n\n{key_ratios([A, Ur])}\n\n"
            "Where a contender reported an overflow or gave a wrong answer, its time is still shown in the tables "
            "above; this summary compares times only.\n\n"
            f"## B. Matrix product (baseline FLINT fmpq_mat)\n\n{B.markdown()}\n\n"
            f"## C. Linear solve (baseline FLINT fmpq_mat)\n\n{C.markdown()}\n\n"
            f"## D. Pipelines (baseline GMP mpq)\n\n{D.markdown()}\n\n"
            f"## L. Layouts of the vector form (baseline turn bytes)\n\n{Lr.markdown()}\n\n"
            f"## E. The cost of exactness (NumPy float64 for context)\n\n{E.markdown()}\n\n"
            f"## F. Precision and exactness\n\n{prec_md}\n")
    (out / f"q{suffix}.md").write_text(text)
    (out / f"q{suffix}.json").write_text(json.dumps(
        {"metadata": meta, **{k: r.raw() for k, r in zip("ABCDLEU", (A, B, C, D, Lr, E, Ur))}, "precision": prec},
        indent=1, default=str))
    print(text)


if __name__ == "__main__":
    main()
```
