"""Measured run of the storage-granularity study (cbench/p1_storage.c): pow2 / limb / byte storage of the vector form,
compared with each other, plus a null control (limb against a second, identical limb run).

Design
- B values: within limb-count buckets L in 2, 4, 8, 16, 24, 32, B = 64(L-1) + 16, 32, 48 (18 values, evenly spread
  inside each bucket rather than just past a boundary), and the exact boundaries 128 .. 2048 as a separate table
  (there pow2, limb and byte store the same bytes per value, so they are A/A points for storage).
- Ops add, mul, sort, scan, widen at two regimes: n small enough to stay in L2 and n = 10^5 (beyond L2).
- Every process: one warm-up run, one calibration run choosing an inner count so each timed sample is at least
  MIN seconds, then REPS samples (median). Results are checked exactly in every process (the output text against
  the expected text from Python integers; padding and neighbour bytes by the C).
- ROUNDS rounds; within a round every variant runs once per (B, op), in an order rotated each round; ratios to limb
  are paired by round. Summaries are geometric means with 95% bootstrap intervals over rounds (2000 resamples).
  The null control's interval shows the noise band; a ratio is marked * when its interval excludes 1 and lies
  outside the null control's interval.
- Chain x <- x b + c 1000^(k+1) at three growth rates (|b| < 10^2, 10^4, 10^8), K in 8 .. 128, policies pow2, limb,
  byte, byte with slack 1.25 / 1.5 / 2.0, limb with slack 1.5; time, re-store time (timed in the C), re-stores,
  bytes allocated / copied / peak, final stride. A dense replay of K = 1 .. 128 (from the same rules, no timing)
  gives the final bytes per value and re-stores for every K.
- The analytic memory table over B = 1 .. 2048.

usage: python3 storage_bench.py [--quick] [--rounds R] [--out-prefix results/storage]
--quick: small n, 2 rounds, short samples; about a minute; its intervals are not meaningful (2 rounds).
"""
import argparse
import ctypes
import ctypes.util
import json
import os
import platform
import subprocess
import time
from pathlib import Path

import numpy as np

import p1_storage_run as P

HERE = Path(__file__).resolve().parent
RESULTS = HERE.parent / "results"
BUCKETS = (2, 4, 8, 16, 24, 32)
BUCKET_B = [64 * (L - 1) + o for L in BUCKETS for o in (16, 32, 48)]
BOUNDARY_B = [128, 256, 512, 1024, 2048]
OPS = P.OPS
VARS = ("pow2", "limb", "byte", "limb2")             # limb2: the null control (limb run again)
CHAIN_POLICIES = (("pow2", 150), ("limb", 150), ("byte", 150), ("slack", 125), ("slack", 150), ("slack", 200),
                  ("lslack", 150))
KS = (8, 16, 32, 64, 128)


def plabel(pol, slack):
    return f"byte+slack{slack / 100:g}" if pol == "slack" else f"limb+slack{slack / 100:g}" if pol == "lslack" else pol


def config(quick, rounds):
    if quick:
        return dict(regimes={"in-cache": 1000, "beyond-L2": 30000}, rounds=rounds or 2, min_sample=0.001, reps=1,
                    chain_n=10000, chain_reps=1, boot=500)
    return dict(regimes={"in-cache": 3000, "beyond-L2": 100000}, rounds=rounds or 12, min_sample=0.002, reps=3,
                chain_n=100000, chain_reps=1, boot=2000)


def machine():
    m = {"python": platform.python_version(), "platform": platform.platform(), "nproc": os.cpu_count()}
    try:
        m["cpu"] = next(l.split(":", 1)[1].strip() for l in open("/proc/cpuinfo") if l.startswith("model name"))
    except Exception:
        pass
    caches = {}
    for d in sorted(Path("/sys/devices/system/cpu/cpu0/cache").glob("index*")):
        try:
            lvl, typ, size = ((d / f).read_text().strip() for f in ("level", "type", "size"))
            if typ != "Instruction":
                caches[f"L{lvl}"] = size
        except Exception:
            pass
    m["caches"] = caches
    try:
        lib = ctypes.CDLL(ctypes.util.find_library("gmp"))
        m["gmp"] = ctypes.c_char_p.in_dll(lib, "__gmp_version").value.decode()
        m["gmp_lib"] = ctypes.util.find_library("gmp")
        ldd = subprocess.run(["ldd", str(P.BIN)], capture_output=True, text=True).stdout
        m["gmp_path"] = next((l.split("=>")[1].split()[0] for l in ldd.splitlines() if "libgmp" in l), None)
    except Exception:
        pass
    m["clang"] = subprocess.run(["clang", "--version"], capture_output=True, text=True).stdout.splitlines()[0]
    return m


def cache_bytes(size):
    s = size.upper()
    mult = 1024 ** 2 if s.endswith("M") else 1024 if s.endswith("K") else 1
    return int(s.rstrip("KMB")) * mult


def footprint(op, var, B, n):
    """Bytes the op touches (stored magnitudes plus sign bytes; sort adds its two index arrays)."""
    v = "limb" if var == "limb2" else var
    s, sh = P.stride(v, B), P.stride(v, B // 2)
    per = {"add": 3 * s + 3, "mul": 2 * sh + s + 3, "sort": s + 1 + 8, "scan": s + 1, "widen": sh + s + 2}[op]
    return per * n


def level(nbytes, caches):
    for lv in ("L1", "L2", "L3"):
        if lv in caches and nbytes <= cache_bytes(caches[lv]):
            return lv
    return "RAM"


def boot(ratios, nboot, rng):
    """ratios: rounds x items (paired per round). Geometric mean over items and rounds, and a 95% interval from
    resampling rounds."""
    lr = np.log(np.asarray(ratios, dtype=float))
    if lr.ndim == 1:
        lr = lr[:, None]
    per_round = lr.mean(axis=1)
    est = float(np.exp(per_round.mean()))
    idx = rng.integers(0, len(per_round), size=(nboot, len(per_round)))
    bs = np.exp(per_round[idx].mean(axis=1))
    return est, float(np.percentile(bs, 2.5)), float(np.percentile(bs, 97.5))


def run_ops(cfg, log):
    out = {}
    for regime, n in cfg["regimes"].items():
        out[regime] = {}
        for B in BUCKET_B + BOUNDARY_B:
            inp = P.OpInputs(B, n, seed=B)
            rec = {op: {v: [] for v in VARS} for op in OPS}
            meta = {op: {} for op in OPS}
            for r in range(cfg["rounds"]):
                order = VARS[r % len(VARS):] + VARS[:r % len(VARS)]
                for op in OPS:
                    for v in order:
                        res = inp.run("limb" if v == "limb2" else v, op, reps=cfg["reps"], min_sample=cfg["min_sample"],
                                      fast_check=True)
                        rec[op][v].append(res["median"])
                        meta[op][v] = dict(stride=res["stride"], stride_in=res["stride_in"], inner=res["inner"])
            out[regime][B] = dict(times=rec, meta=meta)
            log(f"  {regime} n={n} B={B} done")
            del inp
    return out


def run_chain(cfg, log):
    out = {}
    for rate, bmax in P.GROWTH.items():
        ch = P.ChainInputs(cfg["chain_n"], bmax=bmax)
        rec = {K: {plabel(*p): [] for p in CHAIN_POLICIES} for K in KS}
        det = {K: {} for K in KS}
        for r in range(cfg["rounds"]):
            order = CHAIN_POLICIES[r % len(CHAIN_POLICIES):] + CHAIN_POLICIES[:r % len(CHAIN_POLICIES)]
            for K in KS:
                for pol, slack in order:
                    res = ch.run(pol, K, reps=cfg["chain_reps"], slack=slack, fast_check=True)
                    rec[K][plabel(pol, slack)].append((res["median"], res["restore_time"]))
                    det[K][plabel(pol, slack)] = {k: res[k] for k in ("bits", "stride", "restores", "alloc", "copied", "peak")}
        dense = {}
        for pol, slack in CHAIN_POLICIES:
            dense[plabel(pol, slack)] = [ch.replay(pol, K, slack) for K in range(1, 129)]
        out[rate] = dict(bmax=bmax, times=rec, det=det, dense=dense)
        log(f"  chain {rate} done")
        del ch
    return out


def fmt_ci(t):
    return f"{t[0]:.3f} [{t[1]:.3f}, {t[2]:.3f}]"


def mark(t, null):
    return "*" if (t[1] > 1 or t[2] < 1) and not (null[1] <= t[0] <= null[2]) else ""


def summarize(ops, chain, cfg, rng):
    S = {"ops": {}, "chain": {}}
    for regime, byB in ops.items():
        S["ops"][regime] = {"perB": {}, "bucket": {}, "boundary": {}}
        for B, d in byB.items():
            S["ops"][regime]["perB"][B] = {}
            for op in OPS:
                t = d["times"][op]
                limb = np.array(t["limb"])
                S["ops"][regime]["perB"][B][op] = {
                    "limb_ms": float(np.median(limb) * 1e3),
                    **{v: boot(np.array(t[v]) / limb, cfg["boot"], rng) for v in VARS if v != "limb"},
                    "stride": {v: d["meta"][op][v]["stride"] for v in VARS}}
        for group, Bs in (("bucket", BUCKET_B), ("boundary", BOUNDARY_B)):
            for opset_name, opset in [(op, (op,)) for op in OPS] + [("all ops", OPS),
                                                                    ("all but mul", tuple(o for o in OPS if o != "mul"))]:
                row = {}
                for v in VARS:
                    if v == "limb":
                        continue
                    cols = [np.array(byB[B]["times"][op][v]) / np.array(byB[B]["times"][op]["limb"])
                            for B in Bs for op in opset]
                    row[v] = boot(np.stack(cols, axis=1), cfg["boot"], rng)
                S["ops"][regime][group][opset_name] = row
    for rate, d in chain.items():
        S["chain"][rate] = {}
        for K in KS:
            limb = np.array([x[0] for x in d["times"][K]["limb"]])
            row = {}
            for p, xs in d["times"][K].items():
                t = np.array([x[0] for x in xs]); rt = np.array([x[1] for x in xs])
                row[p] = dict(ms=float(np.median(t) * 1e3), restore_ms=float(np.median(rt) * 1e3),
                              compute_ms=float(np.median(t - rt) * 1e3),
                              ratio=boot(t / limb, cfg["boot"], rng), **d["det"][K][p])
            S["chain"][rate][K] = row
        S["chain"][rate]["dense"] = {
            p: dict(mean_stride=float(np.mean([x["stride"] for x in xs])), max_peak=max(x["peak"] for x in xs),
                    restores_at_128=xs[-1]["restores"],
                    mean_copied=float(np.mean([x["copied"] for x in xs])))
            for p, xs in d["dense"].items()}
    return S


def write_md(path, S, cfg, mach, mem, elapsed, quick):
    L = []
    w = L.append
    w("# Storage granularity of the vector form: pow2 vs limb vs byte" + (" (quick run)" if quick else ""))
    w("")
    w("Generated by `build/storage_bench.py`" + (" --quick" if quick else "") + f" in {elapsed / 60:.1f} min. "
      "Compute is always on 64-bit limbs with GMP mpn; only the storage between operations differs. "
      "Every result was checked exactly against Python integers; padding and neighbour bytes were checked by the C.")
    if quick:
        w("")
        w("**Quick run: small n, 2 rounds. The bootstrap intervals are not meaningful at 2 rounds; "
          "use only to check that the harness works.**")
    w("")
    w("## Setup")
    w("")
    w(f"- rounds {cfg['rounds']} (variant order rotated each round, ratios paired within rounds), "
      f"min sample {cfg['min_sample'] * 1e3:g} ms (inner repetitions), {cfg['reps']} sample(s) per process (median)")
    w(f"- regimes: " + ", ".join(f"{k} n={v}" for k, v in cfg["regimes"].items()) + f"; chain n={cfg['chain_n']}")
    w(f"- machine: {mach.get('cpu', '?')}, {mach.get('nproc')} cores, caches {mach.get('caches')}; "
      f"GMP {mach.get('gmp', '?')} ({mach.get('gmp_path', '?')}); {mach.get('clang', '')}")
    w(f"- load average at start {mach.get('load_start')}, at end {mach.get('load_end')}")
    w("- variants: pow2 (next power-of-two width, aligned), limb (ceil(B/64) limbs, aligned), byte (ceil(B/8) "
      "bytes packed, in place: unaligned full limbs, masked top limb), limb2 = limb again (null control). "
      "Signs: one byte per element in all.")
    w("- ratios are time / limb time (below 1: faster than limb); geometric means; [95% bootstrap interval]; "
      "* = interval excludes 1 and the point lies outside the null control's interval")
    w("")
    for regime, R in S["ops"].items():
        w(f"## Ops, {regime} (n={cfg['regimes'][regime]})")
        w("")
        w("Geometric mean over the 18 bucket B values (uniform weight):")
        w("")
        w("| op | pow2/limb | byte/limb | null limb/limb |")
        w("|---|---|---|---|")
        for op, row in R["bucket"].items():
            w(f"| {op} | {fmt_ci(row['pow2'])}{mark(row['pow2'], row['limb2'])} | "
              f"{fmt_ci(row['byte'])}{mark(row['byte'], row['limb2'])} | {fmt_ci(row['limb2'])} |")
        w("")
        w("Exact boundaries (all three variants store the same bytes per value: A/A points for storage):")
        w("")
        w("| op | pow2/limb | byte/limb | null limb/limb |")
        w("|---|---|---|---|")
        for op, row in R["boundary"].items():
            w(f"| {op} | {fmt_ci(row['pow2'])} | {fmt_ci(row['byte'])} | {fmt_ci(row['limb2'])} |")
        w("")
        w("Per B (ratio [interval]; limb time per op; bytes per value pow2/limb/byte; footprint level for limb):")
        w("")
        w("| B | op | limb ms | pow2/limb | byte/limb | null | bytes p/l/b | level |")
        w("|---|---|---|---|---|---|---|---|")
        n = cfg["regimes"][regime]
        for B in BUCKET_B + BOUNDARY_B:
            for op in OPS:
                c = R["perB"][B][op]
                st = c["stride"]
                w(f"| {B} | {op} | {c['limb_ms']:.4f} | {fmt_ci(c['pow2'])}{mark(c['pow2'], c['limb2'])} | "
                  f"{fmt_ci(c['byte'])}{mark(c['byte'], c['limb2'])} | {fmt_ci(c['limb2'])} | "
                  f"{st['pow2']}/{st['limb']}/{st['byte']} | {level(footprint(op, 'limb', B, n), mach.get('caches', {}))} |")
        w("")
    w("## Growth chain")
    w("")
    w("x <- x b + c 1000^(k+1) from 3-place decimals (|x0| < 10^7, |c| < 10^6), the same proven bound for every "
      "policy. Times are medians over rounds; re-store time is timed inside the C (malloc, copy, zero fill, free); "
      "compute = total - re-store. MB are totals per run.")
    for rate, R in S["chain"].items():
        w("")
        w(f"### |b| < {P.GROWTH[rate]:g} ({rate})")
        w("")
        w("| K | policy | ms | vs limb | re-store ms | compute ms | re-stores | bits | bytes/value | alloc MB | copied MB | peak MB |")
        w("|---|---|---|---|---|---|---|---|---|---|---|---|")
        for K in KS:
            for p, c in R[K].items():
                w(f"| {K} | {p} | {c['ms']:.2f} | {fmt_ci(c['ratio'])} | {c['restore_ms']:.2f} | {c['compute_ms']:.2f} | "
                  f"{c['restores']} | {c['bits']} | {c['stride']} | {c['alloc'] / 1e6:.1f} | {c['copied'] / 1e6:.1f} | "
                  f"{c['peak'] / 1e6:.1f} |")
        w("")
        w("Dense replay over K = 1 .. 128 (from the rules, untimed):")
        w("")
        w("| policy | mean bytes/value | re-stores at K=128 | mean MB copied | max peak MB |")
        w("|---|---|---|---|---|")
        for p, c in R["dense"].items():
            w(f"| {p} | {c['mean_stride']:.1f} | {c['restores_at_128']} | {c['mean_copied'] / 1e6:.1f} | {c['max_peak'] / 1e6:.1f} |")
    w("")
    w("## Memory per value, analytic (B = 1 .. 2048, magnitude bytes; each variant adds one sign byte)")
    w("")
    w("| variant | mean bytes | mean waste | mean waste, B > 64 | mean wasted bits | worst wasted bits (at B) |")
    w("|---|---|---|---|---|---|")
    for v, m in mem.items():
        w(f"| {v} | {m['avg_bytes']:.2f} | {100 * m['avg_waste']:.1f}% | {100 * m['avg_waste_over_64']:.1f}% | "
          f"{m['avg_waste_bits']:.1f} | {m['worst_waste_bits']} ({m['worst_bits_at']}) |")
    w("")
    w("Bytes per value at the measured bucket B values: " + ", ".join(
        f"{B}: {P.stride('pow2', B)}/{P.stride('limb', B)}/{P.stride('byte', B)}" for B in BUCKET_B) + " (pow2/limb/byte).")
    w("")
    w("## Known asymmetries")
    w("")
    w("- byte's multiply with a partial top limb is mpn_mul_n on the full limbs plus two mpn_addmul_1 rows, against "
      "one mpn_mul_n for limb and pow2: an algorithmic cost of packing, not a traffic cost; mul is reported apart "
      "and 'all but mul' leaves it out. One-limb operands use the same inline 64x64 product in every variant.")
    w("- byte uses unaligned limb pointers into GMP (fine on x86-64 with this GMP build; see the GMP path above) and "
      "writes the partial top limb by read-modify-write of 8 bytes, which is only safe single-threaded.")
    w("- re-stores always malloc a new vector, copy, zero-fill and free (no realloc or growth in place).")
    Path(path).write_text("\n".join(L) + "\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--rounds", type=int)
    ap.add_argument("--out-prefix")
    ap.add_argument("--skip-chain", action="store_true")
    a = ap.parse_args()
    cfg = config(a.quick, a.rounds)
    prefix = Path(a.out_prefix) if a.out_prefix else RESULTS / ("storage-quick" if a.quick else "storage")
    P.build()
    t0 = time.time()
    log = lambda s: print(f"[{time.time() - t0:7.1f}s] {s}", flush=True)
    mach = machine()
    mach["load_start"] = os.getloadavg()
    rng = np.random.default_rng(12345)
    log(f"ops: {len(BUCKET_B) + len(BOUNDARY_B)} B x {len(OPS)} ops x {len(VARS)} variants x {cfg['rounds']} rounds "
        f"x {len(cfg['regimes'])} regimes")
    ops = run_ops(cfg, log)
    chain = {} if a.skip_chain else run_chain(cfg, log)
    mach["load_end"] = os.getloadavg()
    S = summarize(ops, chain, cfg, rng)
    mem = P.memory_table()
    elapsed = time.time() - t0
    raw = {"config": cfg, "machine": mach, "elapsed_s": elapsed, "memory": mem, "summary": S,
           "raw_ops": ops, "raw_chain": {r: {k: v for k, v in d.items() if k != "dense"} for r, d in chain.items()}}
    prefix.parent.mkdir(parents=True, exist_ok=True)
    Path(str(prefix) + ".json").write_text(json.dumps(raw, indent=1, default=str))
    write_md(str(prefix) + ".md", S, cfg, mach, mem, elapsed, a.quick)
    log(f"wrote {prefix}.md and {prefix}.json")


if __name__ == "__main__":
    main()
