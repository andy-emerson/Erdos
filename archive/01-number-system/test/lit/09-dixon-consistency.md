# 9. Is the Dixon-on-BLAS gain consistent?

In Chapter 7, variant A, Dixon lifting on BLAS, beat FLINT by 10 to 15 percent on random systems at $n = 500$ and $1000$. Each size was tested with one matrix, on one machine. A gain of that size could depend on the particular matrix or on the run, so this chapter tests it across many cases and summarises the result with an interval rather than a single ratio.

## 9.1 Design

**Cases.** Every combination of:

- size $n \in \{200, 400, 600, 800, 1000\}$;
- entry magnitude: two-place decimals of magnitude up to $10^2$ (integer entries up to $10^4$), and three-place decimals of magnitude up to $10^3$ (entries up to $10^6$, as in Chapter 7);
- three random seeds, each giving a fresh matrix and right-hand side. The generator seeds are derived from (size, magnitude, seed, role) through NumPy's `SeedSequence`, so no two cases, and no matrix and right-hand side, share a random stream. Reusing a plain seed across sizes would make a larger matrix start with the same entries as a smaller one.

That is 30 cases. The two magnitudes change the size of the exact answer, and so the number of lifting steps, without changing anything else.

**Contenders.** FLINT's `fmpz_mat.solve` and variant A, `dixon_solve(..., early=False)`, both starting from the same NumPy arrays and both timed including their conversions, as in Chapter 7. Early termination is left off. It is a separate variant, and on random right-hand sides with three-place data Chapter 7 found it could not stop Dixon early. The two-place data here has not been checked for that. Both contenders are timed with the Chapter 1 harness, interleaved within each case, so slow stretches of machine time hit both equally.

**Per-case statistic.** The ratio $\rho = \tilde t_A / \tilde t_{\mathrm{FLINT}}$ of median times, where $\rho < 1$ means variant A is faster.

**Summary.** Ratios are averaged with the geometric mean, the right average for ratios, since a 2× gain and a 2× loss should cancel. The cases are a fixed grid of 10 cells (size × magnitude) with 3 seeds each, and Chapter 7 already showed that the ratio depends strongly on $n$. So the verdict is reported **per size**, not as one pooled number. A pooled geometric mean below 1 could hide a loss at small $n$.

- **Per cell:** the three seeds' ratios, and whether all three are below 1.
- **Per size:** the geometric mean over its 6 cases, with a 95% *stratified* bootstrap interval. Within each of the size's two magnitude cells, resample the 3 seeds with replacement, keep the two cells equally weighted, repeat 10,000 times, and take the 2.5th and 97.5th percentiles. Only the seeds are random draws, so only they are resampled. With 3 seeds per cell this interval is rough, and it should be read as a measure of seed-to-seed variation, not as a precise bound.
- **Overall:** the same stratified interval over the whole grid. It describes the equal-weight average of these 10 cells, a design choice, not a population of matrix sizes.

**Decision rule, fixed before the run.** At each size, variant A is called **consistently faster** if the upper end of the size's interval is below 1 *and* A beat FLINT on every seed in both of its cells. It is called **consistently slower** if the lower end is above 1 and FLINT won every seed. Otherwise the result is **mixed**. The rule is stated in the code and applied mechanically.

None of these intervals says anything about other machines.

**Check.** Every variant-A answer is compared with FLINT's exactly. A mismatch stops the run.

```python {#dixon check}
SIZES = (200, 400, 600, 800, 1000)
MAGNITUDES = ((2, 10**2), (3, 10**3))
SEEDS = (101, 202, 303)


def geomean(xs):
    return math.exp(sum(math.log(x) for x in xs) / len(xs))


def stratified_ci(cells, reps=10000, seed=7):
    rng = random.Random(seed)
    logs = [[math.log(x) for x in cell] for cell in cells]
    means = []
    for _ in range(reps):
        cell_means = [sum(rng.choice(c) for _ in c) / len(c) for c in logs]
        means.append(math.exp(sum(cell_means) / len(cell_means)))
    means.sort()
    return means[int(0.025 * reps)], means[int(0.975 * reps) - 1]


def case_seed(*parts):
    return int(np.random.SeedSequence(list(parts)).generate_state(1)[0])


def run_case(n, digits, magnitude, seed):
    N, _, _ = decimal_matrix(n, n, digits=digits, magnitude=magnitude, seed=case_seed(n, digits, seed, 0))
    b = np.ascontiguousarray(decimal_matrix(n, 1, digits=digits, magnitude=magnitude,
                                            seed=case_seed(n, digits, seed, 1))[0][:, 0])
    FN = flint.fmpz_mat(N.tolist())
    X = FN.solve(flint.fmpz_mat(n, 1, b.tolist()))
    want = [Fraction(int(X[i, 0].p), int(X[i, 0].q)) for i in range(n)]
    nums, d, steps = dixon_solve(N, b, False)
    verify([Fraction(v, d) for v in nums] == want, f"case n={n} digits={digits} seed={seed}")
    res = measure({"FLINT": lambda: flint.fmpz_mat(N.tolist()).solve(flint.fmpz_mat(n, 1, b.tolist())),
                   "A: Dixon on BLAS": lambda: dixon_solve(N, b, False)})
    return res, steps


def main():
    meta = metadata()
    report = Report()
    cases = []
    for n in SIZES:
        for digits, magnitude in MAGNITUDES:
            for seed in SEEDS:
                t = time.perf_counter()
                res, steps = run_case(n, digits, magnitude, seed)
                ratio = res["A: Dixon on BLAS"].median / res["FLINT"].median
                cases.append(dict(n=n, digits=digits, magnitude=magnitude, seed=seed, ratio=ratio, steps=steps,
                                  flint=res["FLINT"].times, dixon=res["A: Dixon on BLAS"].times))
                report.add_measured(f"n={n}, {digits} places to 1e{int(math.log10(magnitude))}", f"seed {seed}", res,
                                    "FLINT", **{"A: Dixon on BLAS": dict(error="0 (matches FLINT)",
                                                                          note=f"{steps} steps")})
                print(f"  n={n} digits={digits} seed={seed}  ratio {ratio:.3f}  ({time.perf_counter() - t:.0f} s)",
                      flush=True)
    ratios = [c["ratio"] for c in cases]

    def cell(n, digits):
        return [c["ratio"] for c in cases if c["n"] == n and c["digits"] == digits]

    all_cells = [cell(n, d) for n in SIZES for d, _ in MAGNITUDES]
    lo, hi = stratified_ci(all_cells)
    lines = [f"- **cases**: {len(cases)} ({len(all_cells)} cells x {len(SEEDS)} seeds)",
             f"- **variant A faster in**: {sum(r < 1 for r in ratios)} of {len(cases)} cases",
             f"- **range of per-case ratios**: {min(ratios):.3f} to {max(ratios):.3f}",
             "",
             "| n | geometric mean A / FLINT | 95% stratified CI | cells where A won all seeds | verdict |",
             "|---|---|---|---|---|"]
    for n in SIZES:
        cells = [cell(n, d) for d, _ in MAGNITUDES]
        clo, chi = stratified_ci(cells)
        wins = sum(all(r < 1 for r in c) for c in cells)
        losses = sum(all(r > 1 for r in c) for c in cells)
        if chi < 1 and wins == len(cells):
            verdict = "consistently faster"
        elif clo > 1 and losses == len(cells):
            verdict = "consistently slower"
        else:
            verdict = "mixed"
        lines.append(f"| {n} | {geomean([r for c in cells for r in c]):.3f} | {clo:.3f} to {chi:.3f} | {wins} of {len(cells)} | {verdict} |")
    lines += ["",
              f"- **equal-weight grid average**: {geomean(ratios):.3f} (95% stratified CI {lo:.3f} to {hi:.3f})"]
    for digits, magnitude in MAGNITUDES:
        sub = [c["ratio"] for c in cases if c["digits"] == digits]
        lines.append(f"- **{digits}-place decimals to 1e{int(math.log10(magnitude))}**: geometric mean {geomean(sub):.3f}")
    summary = "\n".join(lines)
    out = Path(__file__).resolve().parent.parent / "results"
    out.mkdir(exist_ok=True)
    header = "\n".join(f"- **{k}**: {v}" for k, v in meta.items())
    (out / "dixon-consistency.md").write_text(
        f"# Dixon on BLAS vs FLINT: consistency check\n\n{header}\n\n## Summary\n\n{summary}\n\n## Cases\n\n{report.markdown()}\n")
    (out / "dixon-consistency.json").write_text(json.dumps({"metadata": meta, "cases": cases}, indent=1))
    print(summary)


if __name__ == "__main__":
    main()
```

The answer check runs once per case before timing, outside the timed region. The timed calls repeat the identical computation.

```python {file=build/run_dixon_check.py}
import math

from run_benchmarks import *
from run_benchmarks import metadata, verify

from fastsolve import dixon_solve


<<dixon check>>
```
