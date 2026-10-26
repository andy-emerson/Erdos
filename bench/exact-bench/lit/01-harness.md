# 1. The measurement harness

Everything downstream rests on three measurements: how long an operation takes, how wrong its answer is, and how much memory each value needs. This chapter defines all three, and the statistics we use to summarise them.

## 1.1 Timing on a noisy machine

Let $T$ be the true cost of an operation on an idle machine. What we observe on a shared machine is

$$\hat T = T + N, \qquad N \ge 0,$$

where $N$ is interference from everything else: other processes, cache evictions, frequency changes. The noise is one-sided. Interference can only ever make a run slower, never faster. Two consequences follow.

The **mean** of $\hat T$ is a poor estimator, because a handful of large $N$ values drag it upward. The **minimum** estimates $T$ directly, since some runs will catch a quiet moment, but it is an optimistic extreme. The **median** sits between them and is robust to a minority of bad runs. This matches the guidance in the `pyperf` documentation, which recommends the median and the median absolute deviation over the mean and standard deviation when results are unstable. We report the median as the headline figure and the minimum beside it.

We summarise spread with the interquartile range relative to the median,

$$s = \frac{Q_3 - Q_1}{\operatorname{median}}.$$

When $s > 0.10$ we flag the measurement as noisy. A ratio between two noisy measurements should not be trusted to better than the spread.

## 1.2 Interleaving

Noise is not independent between runs. A neighbouring process might hog the CPU for ten seconds and then go quiet. If we ran all NumPy repetitions first and all exact repetitions afterwards, that burst would land entirely on one side. So the contenders take turns: in round $r$, each contender runs once, and the starting contender rotates by one position every round. A burst of interference then spreads roughly evenly across the contenders.

## 1.3 Warm-up and repetitions

The first call of anything is unrepresentative. Numba compiles on first call, memory pages are touched for the first time, caches are cold. Each contender is therefore called once before timing and the result discarded. The calibration step described next adds further untimed calls, so fast operations are warmed several times over.

**Very fast operations.** A single call that takes a microsecond is close to the resolution and overhead of the clock itself. So, as Python's `timeit` does, each sample of a fast contender times $k$ consecutive calls and divides by $k$. We choose the smallest power of two $k$ such that $k$ warm-up calls take at least $t_{\min} = 2$ ms. Contenders slower than that are timed one call per sample.

**Rounds.** The number of timed rounds is chosen from the per-sample duration $t_0$ of the slowest contender, aiming for a budget $B$ of wall-clock time per contender:

$$R = \min\!\left(R_{\max},\ \max\!\left(R_{\min},\ \left\lceil B / t_0 \right\rceil\right)\right).$$

With $B = 3$ s, $R_{\min} = 5$, $R_{\max} = 25$, a 10 ms operation gets 25 rounds and a 5 s operation gets 5.

**Garbage collection.** By default the collector is disabled during each timed sample and run between samples, so that one contender never pays for another's garbage. This mildly favours contenders that allocate many short-lived objects, because their collection cost is never timed. The scalar recurrences in Chapter 6 allocate an object per arithmetic operation, so they are measured with the collector left on, as a user's program would run them.

```python {#harness imports}
import gc
import math
import statistics
import time
from fractions import Fraction
```

`Stats` holds the raw per-call times, and the loop count used for each sample, so that nothing is thrown away. The report can always be recomputed from them.

```python {#harness stats}
class Stats:
    def __init__(self, times, loops=1):
        self.loops = loops
        self.times = sorted(times)
        self.median = statistics.median(self.times)
        self.min = self.times[0]
        q = statistics.quantiles(self.times, n=4, method="inclusive")
        self.spread = (q[2] - q[0]) / self.median if self.median > 0 else 0.0
        self.noisy = self.spread > NOISE_THRESHOLD
```

```python {#harness constants}
NOISE_THRESHOLD = 0.10
BUDGET_SECONDS = 3.0
MIN_ROUNDS = 5
MAX_ROUNDS = 25
MIN_SAMPLE_SECONDS = 2e-3
MAX_LOOPS = 2**16
```

`timed` runs one sample of `loops` calls. `calibrate` doubles the loop count until a sample takes at least $t_{\min}$. `measure` takes a dictionary of zero-argument callables, warms and calibrates each, chooses $R$, and runs the rotating rounds.

```python {#harness measure}
def timed(fn, loops=1, gc_off=True):
    gc.collect()
    if gc_off:
        gc.disable()
    try:
        start = time.perf_counter()
        for _ in range(loops):
            fn()
        return (time.perf_counter() - start) / loops
    finally:
        gc.enable()


def calibrate(fn, gc_off):
    loops = 1
    while True:
        t = timed(fn, loops, gc_off)
        if t * loops >= MIN_SAMPLE_SECONDS or loops >= MAX_LOOPS:
            return loops, t
        loops *= 2


def measure(contenders, gc_off=True):
    names = list(contenders)
    for name in names:
        timed(contenders[name], 1, gc_off)
    plan = {name: calibrate(contenders[name], gc_off) for name in names}
    slowest = max(t * loops for loops, t in plan.values())
    rounds = min(MAX_ROUNDS, max(MIN_ROUNDS, math.ceil(BUDGET_SECONDS / max(slowest, 1e-9))))
    times = {name: [] for name in names}
    for r in range(rounds):
        k = r % len(names)
        for name in names[k:] + names[:k]:
            times[name].append(timed(contenders[name], plan[name][0], gc_off))
    return {name: Stats(ts, plan[name][0]) for name, ts in times.items()}
```

## 1.4 Error

Every exact result in this project is a rational number, held as a Python `Fraction` or a FLINT `fmpq`. Every binary64 float is also a rational number: `Fraction(x)` recovers its value exactly, with no rounding. So the error of a float result $\hat y$ against an exact result $y$ can itself be computed exactly,

$$\varepsilon(\hat y, y) = \frac{|\hat y - y|}{|y|},$$

and converted to a float only at the very end for printing. This matters. If we computed the error in floating point, the error measurement would be contaminated by the very rounding it is trying to measure.

For vectors we report the normwise relative error $\lVert \hat y - y \rVert_\infty / \lVert y \rVert_\infty$, which is the usual quantity in numerical linear algebra and does not blow up when an individual component of $y$ happens to be near zero.

```python {#harness error}
def rel_error(approx, exact):
    exact = Fraction(exact)
    diff = abs(Fraction(float(approx)) - exact)
    if exact == 0:
        return float(diff)
    return float(diff / abs(exact))


def norm_rel_error(approx, exact):
    exact = [Fraction(e) for e in exact]
    scale = max(abs(e) for e in exact)
    diff = max(abs(Fraction(float(a)) - e) for a, e in zip(approx, exact))
    if scale == 0:
        return float(diff)
    return float(diff / scale)
```

A value of exactly `0.0` means the float answer was correct to the last bit. A value near $2^{-53} \approx 1.1 \times 10^{-16}$ is the best a float can usually do.

## 1.5 Memory per value

For fixed-width representations the memory per value is simply the width: 8 bytes for a binary64, 8 bytes for an `int64` numerator in a column whose denominator is shared, 16 bytes for a numerator–denominator pair. For arbitrary-precision results we report the information content, the number of bits in the numerator plus the number in the denominator, divided by 8. That is a lower bound on what any representation needs, which makes the comparison fair to FLINT rather than penalising it for allocator overhead.

```python {#harness memory}
def rational_bytes(q):
    q = Fraction(q)
    return (abs(q.numerator).bit_length() + q.denominator.bit_length() + 7) // 8
```

## 1.6 The report

Each benchmark produces rows. A row names the benchmark, the contender, the size, the timing statistics, and optionally an error and a memory figure. The ratio column compares every contender against the row marked as the baseline for that benchmark and size. The baseline is NumPy everywhere except the scalar recurrences in Chapter 6, which have no NumPy form and use a plain Python float loop instead.

```python {#harness report}
class Report:
    def __init__(self):
        self.rows = []

    def add(self, bench, size, name, stats, baseline=False, error=None, bytes_per_value=None, note=""):
        self.rows.append(dict(bench=bench, size=size, name=name, stats=stats, baseline=baseline,
                              error=error, bytes_per_value=bytes_per_value, note=note))

    def add_measured(self, bench, size, results, baseline, **extra):
        for name, stats in results.items():
            info = extra.get(name, {})
            self.add(bench, size, name, stats, baseline=(name == baseline), **info)

    def markdown(self):
        out = ["| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |",
               "|---|---|---|---|---|---|---|---|---|---|"]
        base = {(r["bench"], r["size"]): r["stats"].median for r in self.rows if r["baseline"] and r["stats"]}
        for r in self.rows:
            s = r["stats"]
            ref = base.get((r["bench"], r["size"]))
            ratio = f"{s.median / ref:.2f}x" if s and ref else ""
            out.append("| " + " | ".join([
                r["bench"], str(r["size"]), r["name"],
                fmt_time(s.median) if s else "", fmt_time(s.min) if s else "",
                (f"{100 * s.spread:.0f}%" + (" ⚠" if s.noisy else "")) if s else "",
                ratio,
                fmt_err(r["error"]),
                "" if r["bytes_per_value"] is None else f"{r['bytes_per_value']:g}",
                r["note"],
            ]) + " |")
        return "\n".join(out)

    def raw(self):
        return [dict(bench=r["bench"], size=r["size"], name=r["name"], baseline=r["baseline"],
                     times=r["stats"].times if r["stats"] else None,
                     loops=r["stats"].loops if r["stats"] else None, error=r["error"],
                     bytes_per_value=r["bytes_per_value"], note=r["note"]) for r in self.rows]


def fmt_time(t):
    for unit, scale in (("s", 1), ("ms", 1e-3), ("µs", 1e-6)):
        if t >= scale:
            return f"{t / scale:.3g} {unit}"
    return f"{t / 1e-9:.3g} ns"


def fmt_err(e):
    if e is None:
        return ""
    if isinstance(e, str):
        return e
    return "0 (exact)" if e == 0 else f"{e:.2e}"
```

```python {file=build/harness.py}
<<harness imports>>

<<harness constants>>


<<harness stats>>


<<harness measure>>


<<harness error>>


<<harness memory>>


<<harness report>>
```
