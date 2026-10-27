"""The threshold test: at exactly 1024 bits, the complete small set against the complete large set.

  small  every routine with the length fixed at compile time (16 limbs)
  large  every routine with the length set at run time
Both sets may use GMP's assembly routines (which take the length as an argument either way) or hand-written C
loops (which the compiler can specialise when the length is a constant); for each operation each set uses its
faster kernel, so the sets differ only in the property under test. Representation is the same for both
(sign-magnitude limbs); the choice between representations below the threshold is a separate question.

Operations: add, multiply (512-bit inputs), dot product, exact division by 999 and by 1000, sort, bit-length scan,
widening 512 to 1024, chain steps that stay at 1024 bits, and a pipeline of them with data flowing between steps.
Two input sets: values filling the width (1012 bits), and values spread over 513-1012 bits (what the narrowest-
width rule leaves at 1024). Rounds are interleaved with the order rotated; the two sets' times are paired within
each round; ratios get a 95% bootstrap interval over rounds; a winner is called only if the interval lies wholly
outside +-5%. Every result is checked against Python integers.
"""
import json
import math
import random
import statistics
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

import numpy as np

import reuse  # noqa: F401
from run_benchmarks import metadata

import p1_arith_run as AR

HERE = Path(__file__).resolve().parent
BIN = HERE.parent / "cbench" / "p1_thresh"
SRC = HERE.parent / "cbench" / "p1_thresh.c"
W, LC = 1024, 16
OPS = ("add", "mul", "dot", "div1", "div2", "sort", "scan", "widen", "step")
NAMES = {"add": "a + b", "mul": "a b (512-bit inputs)", "dot": "dot product", "div1": "exact / 999",
         "div2": "exact / 1000", "sort": "sort", "scan": "bit-length scan", "widen": "widen 512 to 1024",
         "step": "chain steps at 1024 bits (30)", "pipe": "pipeline (mul, add, divide, add, divide, add, step, add, sort, scan)"}
KSTEPS, K0 = 30, 52
BAND = 0.05


def build():
    if not BIN.exists() or BIN.stat().st_mtime < SRC.stat().st_mtime:
        subprocess.run(["clang", "-std=c2x", "-O3", "-march=native", "-Wall", "-Wno-invalid-feature-combination",
                        "-o", str(BIN), str(SRC), "-lgmp"], check=True)


class Inputs:
    def __init__(self, n, spread, seed=0):
        rng = random.Random(zlib.crc32(f"thresh {n} {spread} {seed}".encode()))
        hf, hh = W - 12, (W - 20) // 2 - 1

        def vec(bits, k):
            v = []
            for _ in range(n):
                b = rng.randint(W // 2 + 1, bits) if spread else bits
                v.append(rng.choice((-1, 1)) * (rng.getrandbits(b) | (1 << (b - 1))))
            for i, x in enumerate((0, 1, -1, (1 << bits) - 1)):
                if i < n:
                    v[(i * (k + 3)) % n] = x if (spread or i < 3) else v[(i * (k + 3)) % n]
            return v
        self.a, self.b, self.ha, self.hb = vec(hf, 0), vec(hf, 1), vec(hh, 2) if not spread else \
            [rng.choice((-1, 1)) * rng.getrandbits(rng.randint(1, hh)) for _ in range(n)], vec(hh, 3) if not spread else \
            [rng.choice((-1, 1)) * rng.getrandbits(rng.randint(1, hh)) for _ in range(n)]
        if n > 8:
            self.a[6] = self.a[7]
        self.e1 = [999 * x for x in self.a]
        self.e2 = [1000 * x for x in self.a]
        self.x0 = [rng.choice((-1, 1)) * rng.getrandbits(rng.randint(505, 520)) for _ in range(n)]
        nrng = np.random.default_rng(zlib.crc32(f"chain {n} {seed}".encode()))
        self.cb = nrng.integers(-10**4, 10**4 + 1, size=n)
        self.cc = nrng.integers(-10**6, 10**6 + 1, size=n)
        if n > 3:
            self.cb[1], self.cc[2], self.x0[3] = 0, 0, 0
        self.n = n
        self.dir = tempfile.TemporaryDirectory()
        d = Path(self.dir.name)
        for name in ("a", "b", "e1", "e2", "x0"):
            AR._write(d / name, getattr(self, name), W)
        for name in ("ha", "hb"):
            AR._write(d / name, getattr(self, name), W // 2)
        self.cb.astype(np.int64).tofile(d / "cb.bin")
        self.cc.astype(np.int64).tofile(d / "cc.bin")
        self._chain = {}

    def chain(self, k):
        if k not in self._chain:
            x = list(self.x0)
            for s in range(k):
                cs = 1000 ** (K0 + s + 1)
                x = [xi * int(b) + int(c) * cs for xi, b, c in zip(x, self.cb, self.cc)]
            self._chain[k] = x
        return self._chain[k]

    def expected(self, op):
        a, ha, hb = self.a, self.ha, self.hb
        if op == "add":
            return [x + y for x, y in zip(a, self.b)]
        if op == "mul":
            return [x * y for x, y in zip(ha, hb)]
        if op == "dot":
            return [sum(x * y for x, y in zip(ha, hb))]
        if op in ("div1", "div2"):
            return list(a)
        if op == "sort":
            return sorted(range(self.n), key=lambda i: a[i])
        if op == "scan":
            return [max(abs(x).bit_length() for x in a)]
        if op == "widen":
            return list(ha)
        if op == "step":
            return self.chain(KSTEPS)
        if op == "pipe":
            v = [x * y + 3 * z + u + w for x, y, z, u, w in zip(ha, hb, a, self.x0, self.chain(1))]
            return v + [max(abs(x).bit_length() for x in v)] + sorted(range(self.n), key=lambda i: v[i])

    def run(self, s, kernel, op, reps, inner=1):
        out = Path(self.dir.name) / f"{s}-{op}.txt"
        env = {"P1_THRESH_L": str(LC), "P1_THRESH_K0": str(K0), "P1_THRESH_INNER": str(inner), "PATH": "/usr/bin:/bin"}
        r = subprocess.run([str(BIN), s, kernel, op, str(self.n), str(reps), self.dir.name, str(out), str(KSTEPS)],
                           capture_output=True, text=True, env=env)
        r.check_returncode()
        med, mn = map(float, r.stdout.split())
        return med, [int(x) for x in out.read_text().split()]


def boot(ratios, rng, B=4000):
    """Geometric mean of ratios over rounds, with a 95% bootstrap interval (rounds resampled). ratios: list (rounds)
    or array rounds x ops (equal weights over ops)."""
    logs = np.log(np.array(ratios, dtype=float))
    idx = rng.integers(0, logs.shape[0], size=(B, logs.shape[0]))
    m = logs[idx].mean(axis=tuple(range(1, logs.ndim + 1)))
    return float(np.exp(logs.mean())), float(np.exp(np.quantile(m, 0.025))), float(np.exp(np.quantile(m, 0.975)))


def side(g, lo, hi, band):
    """'large' or 'small' if the interval lies wholly outside the band on that side, else 'tie'."""
    if lo > 1 + band:
        return "large"
    if hi < 1 - band:
        return "small"
    return "tie"


# The decision rule, fixed before the measured run:
#   For each regime (n = 100000 out of cache, n = 3000 in cache) and each input set (values filling the width,
#   values spread over 513-1012 bits) there are two statistics: the equal-weight geometric mean of the per-operation
#   ratios, and the pipeline. Each gets a verdict: a side wins only if its 95% interval lies wholly beyond the band,
#   where the band is the larger of 5% and the null control's spread (the same set timed twice).
#   1024 belongs to a side if that side wins at least half of the eight verdicts and the other side wins none.
#   Otherwise the result is a tie: both sets are equally good at 1024, and the threshold can be stated either way.
#   (For the phase-two runs a tie is resolved by the out-of-cache point estimate; this is reported as a tie.)
def measure(I, rounds, reps, inner, rng):
    """Pilot: two rounds over every (set, kernel) choose each set's kernel per operation. Measured: rounds over the
    chosen kernels with the order rotated, plus the null control (the small set's chosen kernel timed twice)."""
    want = {op: I.expected(op) for op in OPS + ("pipe",)}
    combos = [(s, k) for s in ("small", "large") for k in ("c", "cn", "asm")]
    pilot = {(op, s, k): [] for op in OPS for s, k in combos}
    for r in range(2):
        for op in OPS:
            for s, k in combos[r:] + combos[:r]:
                med, got = I.run(s, k, op, reps, inner.get(op, 1))
                assert got == want[op], (op, s, k)
                pilot[(op, s, k)].append(med)
    best = {(op, s): min(("c", "cn", "asm"), key=lambda k: statistics.median(pilot[(op, s, k)])) for op in OPS for s in ("small", "large")}
    runs = [("small", "a"), ("large", "a"), ("small", "b")]          # b: the null control
    t = {(op, s, x): [] for op in OPS + ("pipe",) for s, x in runs}
    for r in range(rounds):
        order = runs[r % 3:] + runs[:r % 3]
        for op in OPS + ("pipe",):
            for s, x in order:
                k = best[(op, s)] if op != "pipe" else ",".join(f"{o}={best[(o, s)]}" for o in OPS)
                med, got = I.run(s, k, op, reps, inner.get(op, 1))
                assert got == want[op], (op, s, x)
                t[(op, s, x)].append(med)
    return best, t


def main():
    quick = "--quick" in sys.argv
    build()
    regimes = ([("in cache", 600)] if quick else [("out of cache", 100000), ("in cache", 3000)])
    rounds, reps = (3, 3) if quick else (12, 5)
    rng = np.random.default_rng(1)
    report, raw, verdicts = [], {}, []
    meta = metadata()
    fmt = lambda s: f"{s * 1e6:.3g} µs" if s < 1e-3 else f"{s * 1e3:.3g} ms"
    for rlabel, n in regimes:
        for spread in (False, True):
            label = f"{rlabel}, n = {n}, " + ("values spread over 513-1012 bits" if spread else "values filling the width (1012 bits)")
            I = Inputs(n, spread, seed=3)
            inner = {}
            if n < 10000:                                        # make each timed sample at least ~2 ms
                for op in OPS:
                    if op in ("step",):
                        continue
                    med, _ = I.run("large", "asm", op, 3, 1)
                    inner[op] = max(1, int(math.ceil(0.002 / max(med, 1e-9))))
            best, t = measure(I, rounds, reps, inner, rng)
            null = [a / b for a, b in zip(t[("pipe", "small", "a")], t[("pipe", "small", "b")])]
            null_ops = np.array([[a / b for a, b in zip(t[(op, "small", "a")], t[(op, "small", "b")])] for op in OPS]).T
            ng, nlo, nhi = boot(null_ops, rng)
            band = max(BAND, abs(1 - nlo), abs(nhi - 1))
            rows, per = [], []
            for op in OPS:
                ratios = [a / b for a, b in zip(t[(op, "small", "a")], t[(op, "large", "a")])]
                per.append(ratios)
                g, lo, hi = boot(ratios, rng)
                rows.append(dict(op=op, small=statistics.median(t[(op, "small", "a")]), large=statistics.median(t[(op, "large", "a")]),
                                 ks=best[(op, "small")], kl=best[(op, "large")], ratio=g, lo=lo, hi=hi, verdict=side(g, lo, hi, band)))
            gm, glo, ghi = boot(np.array(per).T, rng)
            pr = [a / b for a, b in zip(t[("pipe", "small", "a")], t[("pipe", "large", "a")])]
            gp, lop, hip = boot(pr, rng)
            vg, vp = side(gm, glo, ghi, band), side(gp, lop, hip, band)
            verdicts += [vg, vp]
            raw[label] = dict(n=n, inner=inner, best={f"{k[0]} {k[1]}": v for k, v in best.items()},
                              times={f"{k[0]} {k[1]} {k[2]}": v for k, v in t.items()}, rows=rows,
                              geomean=[gm, glo, ghi], pipe=[gp, lop, hip], null=[ng, nlo, nhi], band=band, verdicts=[vg, vp])
            kname = {"c": "C", "cn": "C, no unrolling", "asm": "GMP"}
            report += [f"### {label}", "",
                       f"Ratio = small set / large set time (above 1: the large set is faster): geometric mean over {rounds} "
                       f"rounds, 95% bootstrap interval. Band for a verdict: ±{band:.1%} (the larger of 5% and the null "
                       f"control, the small set timed twice: {ng:.3f}, {nlo:.3f}–{nhi:.3f}). Kernels were chosen in a "
                       "separate pilot." + (f" Each timed sample repeats the operation (inner counts: {inner})." if inner else ""), "",
                       "| Operation | Small set (kernel) | Large set (kernel) | Ratio | 95% interval | Verdict |", "|---|---|---|---|---|---|"]
            for r in rows:
                report.append(f"| {NAMES[r['op']]} | {fmt(r['small'])} ({kname[r['ks']]}) | {fmt(r['large'])} ({kname[r['kl']]}) | "
                              f"{r['ratio']:.3f} | {r['lo']:.3f}–{r['hi']:.3f} | {r['verdict']} |")
            report += [f"| **all operations, equal weights** | | | {gm:.3f} | {glo:.3f}–{ghi:.3f} | **{vg}** |",
                       f"| **{NAMES['pipe']}** | {fmt(statistics.median(t[('pipe', 'small', 'a')]))} | "
                       f"{fmt(statistics.median(t[('pipe', 'large', 'a')]))} | {gp:.3f} | {lop:.3f}–{hip:.3f} | **{vp}** |", ""]
            print(f"  {label} done", flush=True)
    ns, nl = verdicts.count("small"), verdicts.count("large")
    decision = "small" if ns * 2 >= len(verdicts) and nl == 0 else "large" if nl * 2 >= len(verdicts) and ns == 0 else "tie"
    first = [v for k, v in raw.items() if k.startswith(regimes[0][0])]
    point = float(np.exp(np.mean([math.log(v["geomean"][0]) for v in first] + [math.log(v["pipe"][0]) for v in first])))
    rep_at_1024 = "lc" if decision == "small" or (decision == "tie" and point <= 1) else "lr"
    rule = ("The rule, fixed before the run: each regime and input set gives two verdicts (the equal-weight mean and the "
            "pipeline); a side wins a verdict only if its interval lies wholly beyond the band; 1024 belongs to a side if "
            "that side wins at least half of the verdicts and the other none; otherwise it is a tie.")
    concl = {"small": "1024 is the last size for the small routines (compile-time lengths).",
             "large": "1024 is the first size for the large routines (run-time lengths).",
             "tie": f"A tie: at 1024 the two sets perform alike, so the threshold can be stated either way. For the phase-two "
                    f"runs the out-of-cache point estimate ({point:.3f}) puts 1024 with the "
                    f"{'small' if rep_at_1024 == 'lc' else 'large'} routines."}[decision]
    text = ("# The threshold test: 1024 bits, the complete small set against the complete large set\n\n" +
            "\n".join(f"- **{k}**: {v}" for k, v in meta.items()) + "\n\n" + "\n".join(report) +
            f"\n## Decision\n\n{rule}\n\nVerdicts: {', '.join(verdicts)}. **{concl}**\n")
    out = HERE.parent / "results"
    suffix = "-quick" if quick else ""
    (out / f"threshold{suffix}.md").write_text(text)
    (out / f"threshold{suffix}.json").write_text(json.dumps(dict(rounds=rounds, reps=reps, inputs=raw,
        decision=dict(verdicts=verdicts, point=point, decision=decision, rep_at_1024=rep_at_1024)), indent=1, default=str))
    print(text)


if __name__ == "__main__":
    main()
