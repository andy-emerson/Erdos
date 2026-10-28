#!/usr/bin/env python3
"""fault_bench.py: the fault-injection test that decides the consistency option of spec v0.3 section 10.

    make && python3 test_fault.py                 the checks of the harness itself
    python3 fault_bench.py --smoke                a small run into build/smoke/ (not the measured run)
    python3 fault_bench.py                        the measured run: results/fault.md and results/fault.json

The measured run refuses to start unless ../../research/fault-rule.md contains the decision rule (RULE below)
verbatim, and writes the rule into results/fault.md before the first trial.

Variants (one C build each, from the M1 sources copied into src/; lib/ is not modified):
  N   M1 as accepted: no production checks (its audit exists but never runs).
  O1  option 1: exponents only in holder factorizations (the truth); node lists hold holder ids (a derived index).
      verify = the production audit (I1-I4 without the primality and pairwise-gcd parts of I1); on a mismatch,
      rebuild the derived index (lists, counts, positions, dictionary, node constants, cached p^a and D) from the
      truth, audit again, and halt if it still fails.  Runs every K operations and before every refinement.
  O2  option 2: one shared record (holder, node, exponent, list position) per edge, referenced from the holder's
      factorization and the node's list.  Same verification; repair rebuilds membership from the holders' references.
  O3  N plus the same audit every K operations; halt on a mismatch, no repair.
Faults F1-F9 as in the test plan (see the report).  One fault per trial; each trial forks from the fault-free run at
the fault's moment, so every variant sees the same workloads, moments, injector seeds and K phases.
"""
import argparse
import json
import math
import os
import random
import subprocess
import sys
import time
from multiprocessing import Pool

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import workload as W  # noqa: E402

RULE_FILE = os.path.normpath(os.path.join(HERE, "..", "..", "research", "fault-rule.md"))
RULE = """**Decision rule (pre-registered in research/fault-rule.md on 2026-10-28, before the measured run).**

0. **Trials.** Every variant runs the same trials: the same workloads (2,000 random commands on single values and
   arrays, then a readout of every live value and array), and for each trial the same fault class, moment, injector
   seed and K phase.  N runs once (it has no checks); O1, O2 and O3 run at K = 100, 1,000 and 10,000.  There are
   2,000 trials per class per variant and K (100 workloads, 20 trials per class each).  Each trial ends in one
   outcome: detected and repaired, detected and not repaired, silent wrong, crash, or benign.  A wrong result
   returned as success, or a result where the oracle has an error, is silent wrong; an error status where the oracle
   has a result is reported separately as a spurious error and is not counted as silent wrong.  A trial whose fault
   found no target is excluded and counted.
1. **Primary.** S_bug(v, K) is the number of silent-wrong outcomes over the bug classes F1, F2, F5, F6, F7 and F8.
   F5 counts 0 on N, O1 and O3, which store no node-side exponent, so the fault cannot occur there (shown as n/a).
   A variant's primary value is its minimum of S_bug over K (N: its single value).  Two counts are **tied** when the
   95% interval of their paired difference (bootstrap over workloads, 10,000 resamples, seed 20261028) contains 0.
   The primary winners are the variants whose primary value is tied with the lowest one.  A winner's admissible Ks
   are those at which its S_bug is tied with its own primary value.
2. **Secondary**, among the primary winners: S_hw(v, K) is the number of silent-wrong outcomes over F9 (a single
   bit flip).  A variant's secondary value is its minimum of S_hw over its admissible Ks; the winners are those
   tied with the lowest.  The admissible Ks shrink to those whose S_hw is tied with the variant's secondary value.
3. **Tertiary**, among the secondary winners: a variant's overhead at K is the geometric mean, over the two normal
   workloads, of its time divided by N's time (median of 5 runs each).  Its tertiary value is its lowest overhead
   over its admissible Ks.  The variant with the lowest tertiary value is the decision; values within 5% of each
   other are a tie, which is reported as a tie with no verdict forced.  Memory is reported and does not decide.
4. **Residual risk.** F3 and F4 corrupt the single source of truth itself (option 1's holder factorization;
   option 2's shared edge record), so no cross-check against derived data can tell which side is right: no variant
   can correctly repair them, and a rebuild from that truth enshrines the corruption.  They are not counted in 1-3
   and are reported separately.  A per-holder checksum over the factorization and the numerator, written by every
   update and checked on read or audit, would catch them."""

VARIANTS = ["N", "O1", "O2", "O3"]
KS = [100, 1000, 10000]
SEED = 20261028
BOOT = 10000
OUT_NAMES = {"detected_repaired": "detected, repaired", "detected_not_repaired": "detected, not repaired",
             "silent_wrong": "silent wrong", "crash": "crash", "benign": "benign", "spurious_error": "spurious error"}


def cells():
    return [("N", 0)] + [(v, K) for v in ("O1", "O2", "O3") for K in KS]


def trial_specs(wi, length, per, seed):
    """The trials of workload wi: identical for every variant and K."""
    w = W.Workload(seed * 1000 + wi, length)
    r = random.Random(seed * 7919 + wi)
    specs = []
    for cls in W.CLASSES:
        for j in range(per):
            moment = r.randrange(10**9) if cls == 8 else r.randrange(1, w.workend)
            specs.append(((wi * 10 + cls) * per + j, cls, moment, r.randrange(1, 2**62), r.randrange(10**9)))
    return w, specs


_wcache = {}


def run_job(args):
    v, K, wi, length, per, seed, timeout = args
    key = (wi, length, per, seed)
    if key not in _wcache:
        _wcache.clear()
        _wcache[key] = trial_specs(wi, length, per, seed)
    w, specs = _wcache[key]
    if v != "O2":
        specs = [s for s in specs if s[1] != 5]  # F5 does not apply (no node-side exponent)
    env = dict(os.environ, FAULT_TIMEOUT=str(timeout))
    t0 = time.time()
    res = subprocess.run([os.path.join(HERE, "build", f"drv_{v}")], input=W.job_text(w, K, specs),
                         capture_output=True, text=True, env=env)
    lines = res.stdout.splitlines()
    if not lines or not lines[0].startswith("CONTROL\tok"):
        return {"v": v, "K": K, "wi": wi, "error": (lines[0] if lines else res.stderr)[:2000]}
    out = []
    for l in lines[1:]:
        d = W.parse_record(l)
        o, sub = W.classify(d)
        out.append([d["cls"], o, sub, d.get("inj_region", ""), d.get("inj_desc", "")[:90], d.get("halt_msg", "")[:90],
                    d.get("repair_msg", "")[:90], d.get("got", "")[:60], d.get("want", "")[:60]])
    ctl = dict(kv.split("=") for kv in lines[0].split("\t")[2:])
    return {"v": v, "K": K, "wi": wi, "trials": out, "control": ctl, "secs": time.time() - t0}


# ------------------------------------------------------------------------------------------------- statistics

def tied(a, b, nw, seed=SEED):
    """a, b: per-workload counts (lists of length nw).  The 95% bootstrap interval of sum(a) - sum(b) contains 0."""
    d = [x - y for x, y in zip(a, b)]
    r = random.Random(seed)
    sums = []
    for _ in range(BOOT):
        s = 0
        for _ in range(nw):
            s += d[r.randrange(nw)]
        sums.append(s)
    sums.sort()
    lo, hi = sums[int(0.025 * BOOT)], sums[int(0.975 * BOOT) - 1]
    return lo <= 0 <= hi, (lo, hi)


def per_workload(results, v, K, classes, nw):
    c = [0] * nw
    for job in results:
        if job["v"] == v and job["K"] == K:
            for t in job["trials"]:
                if t[0] in classes and t[1] == "silent_wrong":
                    c[job["wi"]] += 1
    return c


def decide(results, ovh, nw):
    """Apply the pre-registered rule.  Returns (lines of the report, a summary dict)."""
    L, S = [], {}
    ks = {"N": [0], "O1": KS, "O2": KS, "O3": KS}
    bug = {(v, K): per_workload(results, v, K, W.BUG_CLASSES, nw) for v in VARIANTS for K in ks[v]}
    hw = {(v, K): per_workload(results, v, K, W.HW_CLASSES, nw) for v in VARIANTS for K in ks[v]}
    # 1. primary
    prim = {}
    for v in VARIANTS:
        kbest = min(ks[v], key=lambda K: (sum(bug[(v, K)]), -K))
        prim[v] = kbest
    lowest = min(VARIANTS, key=lambda v: sum(bug[(v, prim[v])]))
    L.append(f"**1. Primary** (silent wrong over F1, F2, F5, F6, F7, F8; each variant at its best K):")
    L.append("")
    winners = []
    for v in VARIANTS:
        t, iv = tied(bug[(v, prim[v])], bug[(lowest, prim[lowest])], nw) if v != lowest else (True, (0, 0))
        if t:
            winners.append(v)
        L.append(f"- {v}: {sum(bug[(v, prim[v])])} at K = {prim[v] or '-'}; against {lowest}: difference interval "
                 f"[{iv[0]}, {iv[1]}] -> {'tied with the lowest (winner)' if t else 'worse'}")
    adm = {}
    for v in winners:
        adm[v] = [K for K in ks[v] if K == prim[v] or tied(bug[(v, K)], bug[(v, prim[v])], nw)[0]]
        L.append(f"  - {v} admissible K: {', '.join(str(k) if k else '-' for k in adm[v])}")
    S["primary_winners"] = winners
    # 2. secondary
    L.append("")
    L.append("**2. Secondary** (silent wrong over F9), among the primary winners:")
    L.append("")
    sec = {v: min(adm[v], key=lambda K: (sum(hw[(v, K)]), -K)) for v in winners}
    low2 = min(winners, key=lambda v: sum(hw[(v, sec[v])]))
    win2 = []
    for v in winners:
        t, iv = tied(hw[(v, sec[v])], hw[(low2, sec[low2])], nw) if v != low2 else (True, (0, 0))
        if t:
            win2.append(v)
        L.append(f"- {v}: {sum(hw[(v, sec[v])])} at K = {sec[v] or '-'}; against {low2}: [{iv[0]}, {iv[1]}] -> "
                 f"{'winner' if t else 'worse'}")
    for v in win2:
        adm[v] = [K for K in adm[v] if K == sec[v] or tied(hw[(v, K)], hw[(v, sec[v])], nw)[0]]
        L.append(f"  - {v} admissible K: {', '.join(str(k) if k else '-' for k in adm[v])}")
    S["secondary_winners"] = win2
    # 3. tertiary
    L.append("")
    L.append("**3. Tertiary** (overhead against N, geometric mean of the two workloads), among the secondary winners:")
    L.append("")
    ter = {}
    for v in win2:
        best = None
        for K in adm[v]:
            o = overhead_of(ovh, v, K)
            if o is not None and (best is None or o < best[0]):
                best = (o, K)
        ter[v] = best
        L.append(f"- {v}: " + (f"{best[0]:.3f}x at K = {best[1] or '-'}" if best else "no overhead measured"))
    have = {v: t for v, t in ter.items() if t}
    if not have:
        S["decision"] = None
        L.append("")
        L.append("**Decision:** none (no overhead measurement).")
        return L, S
    lowv = min(have, key=lambda v: have[v][0])
    tie = [v for v in have if have[v][0] <= have[lowv][0] * 1.05]
    L.append("")
    if len(tie) == 1:
        S["decision"] = {"variant": lowv, "K": have[lowv][1]}
        L.append(f"**Decision: {lowv}** at K = {have[lowv][1] or '-'} (overhead {have[lowv][0]:.3f}x).")
    else:
        S["decision"] = {"tie": tie}
        L.append(f"**Decision: a tie** between {', '.join(tie)} (overheads within 5%); no verdict is forced.")
    return L, S


def overhead_of(ovh, v, K):
    rs = []
    for w in ("dec", "dens"):
        base = ovh.get(f"N/{w}/0")
        x = ovh.get(f"{v}/{w}/{K}")
        if not base or not x:
            return None
        rs.append(x["median"] / base["median"])
    return math.exp(sum(math.log(r) for r in rs) / len(rs))


# ------------------------------------------------------------------------------------------------- overhead

def run_overhead(reps):
    ovh = {}
    for v in VARIANTS:
        for w in ("dec", "dens"):
            for K in ([0] if v == "N" else KS):
                res = subprocess.run([os.path.join(HERE, "build", f"ovh_{v}"), w, str(K), str(reps)],
                                     capture_output=True, text=True)
                f = res.stdout.split()
                if res.returncode or len(f) < 10:
                    ovh[f"{v}/{w}/{K}"] = {"error": res.stderr[:300]}
                    continue
                ovh[f"{v}/{w}/{K}"] = {"best": float(f[4]), "median": float(f[5]), "mem_peak": int(f[6]),
                                       "checks": int(f[7]), "checksum": f[9]}
                print(f"  overhead {v} {w} K={K}: {float(f[5]):.4f} s, {int(f[6]) / 1e6:.2f} MB, {f[7]} checks",
                      flush=True)
    return ovh


# ------------------------------------------------------------------------------------------------- report

def table(head, rows):
    out = ["| " + " | ".join(head) + " |", "|" + "|".join(" --- " for _ in head) + "|"]
    out += ["| " + " | ".join(str(c) for c in r) + " |" for r in rows]
    return out


def report(cfg, results, ovh, wall, path_md):
    nw = cfg["workloads"]
    L = []
    counts, notinj, subs, regions = {}, {}, {}, {}
    for job in results:
        for t in job["trials"]:
            cell = (job["v"], job["K"])
            if t[1] is None:
                notinj[(cell, t[0])] = notinj.get((cell, t[0]), 0) + 1
                continue
            counts.setdefault(cell, {}).setdefault(t[0], {}).setdefault(t[1], 0)
            counts[cell][t[0]][t[1]] += 1
            if t[1] == "silent_wrong":
                subs.setdefault(cell, {}).setdefault(t[2], 0)
                subs[cell][t[2]] += 1
            if t[0] == 9:
                rg = regions.setdefault(cell, {}).setdefault(t[3] or "?", {})
                rg[t[1]] = rg.get(t[1], 0) + 1
    L.append("## Results")
    L.append("")
    L.append(f"{sum(len(j['trials']) for j in results):,} trials in {wall / 60:.1f} min; {nw} workloads of "
             f"{cfg['length']:,} commands; {cfg['per']} trials per class per workload.")
    L.append("")
    L.append("### Silent wrong by class (the outcome that matters)")
    L.append("")
    head = ["variant", "K"] + [f"F{c}" for c in W.CLASSES] + ["bug classes (F1 F2 F5-F8)", "F9", "F3+F4 (residual)"]
    rows = []
    for cell in cells():
        c = counts.get(cell, {})
        sw = lambda k: c.get(k, {}).get("silent_wrong", 0)  # noqa: E731
        r = [cell[0], cell[1] or "-"]
        for k in W.CLASSES:
            r.append("n/a" if (k == 5 and cell[0] != "O2") else sw(k))
        r += [sum(sw(k) for k in W.BUG_CLASSES), sw(9), sw(3) + sw(4)]
        rows.append(r)
    L += table(head, rows)
    L.append("")
    L.append("### All outcomes, per variant and K")
    L.append("")
    for cell in cells():
        L.append(f"**{cell[0]}{'' if not cell[1] else f', K = {cell[1]:,}'}**")
        L.append("")
        rows = []
        for k in W.CLASSES:
            c = counts.get(cell, {}).get(k, {})
            if k == 5 and cell[0] != "O2":
                rows.append([W.CLASS_NAMES[k]] + ["n/a"] * (len(W.OUTCOMES) + 1))
                continue
            rows.append([W.CLASS_NAMES[k]] + [c.get(o, 0) for o in W.OUTCOMES] + [notinj.get((cell, k), 0)])
        L += table(["class"] + [OUT_NAMES[o] for o in W.OUTCOMES] + ["not injected"], rows)
        L.append("")
    L.append("### How silent wrong arose")
    L.append("")
    allsub = sorted({s for c in subs.values() for s in c})
    L += table(["variant", "K"] + allsub, [[c[0], c[1] or "-"] + [subs.get(c, {}).get(s, 0) for s in allsub]
                                          for c in cells()])
    L.append("")
    L.append("('no check fired first': no verification ran between the fault and the wrong result; 'checks passed': "
             "verifications ran but did not see the fault; 'after a repair': a rebuild treated corrupted data as the "
             "truth.)")
    L.append("")
    L.append("### F9 by region (silent wrong / trials)")
    L.append("")
    allr = sorted({r for c in regions.values() for r in c})
    rows = []
    for cell in cells():
        rg = regions.get(cell, {})
        rows.append([cell[0], cell[1] or "-"] + [f"{rg.get(r, {}).get('silent_wrong', 0)}/{sum(rg.get(r, {}).values())}"
                                                  for r in allr])
    L += table(["variant", "K"] + allr, rows)
    L.append("")
    L.append("### Residual risk: F3 and F4 (corruptions of the truth)")
    L.append("")
    L.append("These classes corrupt the single source of truth itself, so no cross-check can tell which side is "
             "right; they are not counted in the decision.  Detection, where it happens, is incidental: a derived "
             "copy (the cached p^a, a list position, the edge order) happens to disagree.  O1 and O2 then rebuild "
             "from the corrupted truth.  A per-holder checksum over the factorization and the numerator would catch "
             "them.")
    L.append("")
    rows = []
    for cell in cells():
        for k in (3, 4):
            c = counts.get(cell, {}).get(k, {})
            rows.append([cell[0], cell[1] or "-", f"F{k}"] + [c.get(o, 0) for o in W.OUTCOMES])
    L += table(["variant", "K", "class"] + [OUT_NAMES[o] for o in W.OUTCOMES], rows)
    L.append("")
    L.append("### Overhead (no fault)")
    L.append("")
    rows = []
    for v in VARIANTS:
        for K in ([0] if v == "N" else KS):
            r = [v, K or "-"]
            for w in ("dec", "dens"):
                x, b = ovh.get(f"{v}/{w}/{K}", {}), ovh.get(f"N/{w}/0", {})
                if "median" in x and "median" in b:
                    r += [f"{x['median']:.4f}", f"{x['median'] / b['median']:.2f}x", f"{x['mem_peak'] / 1e6:.2f}",
                          x["checks"]]
                else:
                    r += ["-"] * 4
            o = overhead_of(ovh, v, K)
            r.append(f"{o:.3f}x" if o else "-")
            rows.append(r)
    L += table(["variant", "K", "dec s", "dec vs N", "dec MB", "dec checks", "dens s", "dens vs N", "dens MB",
                "dens checks", "geo. mean vs N"], rows)
    L.append("")
    sums = {f"{w}": {ovh.get(f"{v}/{w}/{K}", {}).get("checksum") for v in VARIANTS for K in ([0] if v == "N" else KS)}
            for w in ("dec", "dens")}
    L.append("Result checksums equal across variants: " + ", ".join(f"{w} {'yes' if len(s) == 1 else 'NO'}"
                                                                    for w, s in sums.items()) + ".")
    L.append("")
    L.append("## Decision (by the pre-registered rule)")
    L.append("")
    dl, ds = decide(results, ovh, nw)
    L += dl
    L.append("")
    with open(path_md, "a") as f:
        f.write("\n".join(L) + "\n")
    return counts, ds


def header(cfg):
    H = [f"# Fault injection: the consistency option (spec v0.3 §10)", "",
         f"{time.strftime('%b %d, %Y')} · `test/fault/fault_bench.py`"
         f"{' (SMOKE RUN: not the measured run)' if cfg['smoke'] else ''}", "",
         "## Rule", "", RULE, "",
         "## Setup", "",
         "- **Variants** (one C build each from the M1 sources, copied into `test/fault/src/`): **N** M1 with no "
         "production checks; **O1** option 1 with `verify` (the production audit) and `rebuild` of the derived index "
         "from the holder factorizations, every K operations and before every refinement; **O2** option 2, one shared "
         "edge record (holder, node, exponent, list position) referenced from both sides, the same verification, "
         "membership repaired by rebuild; **O3** N plus the audit every K operations, halting on a mismatch.  The "
         "production audit is M1's `pg_audit` without the primality and pairwise-gcd parts of I1.  An operation is a "
         "public call that mutates or reduces (M1's `DEBUG_AUDIT` points).",
         "- **Faults**, one per trial, before a random command of the workload (F8: a random refinement of the "
         "workload): F1 drop one entry of a node's list (count and length both lowered, as a buggy unlink would); "
         "F2 append a stray holder (O2: a stray record) to a node's list; F3 a wrong node id in a holder's "
         "factorization; F4 a wrong exponent there (+-1 or +2); F5 a wrong exponent in a node-side record (O2 only; "
         "the others store none); F6 a reference count +-1 or +-2; F7 an atom's dictionary entry to another live "
         "node; F8 a refinement's copy-and-loop stopped halfway, the program continuing; F9 one bit flipped, uniform "
         "over the bytes of the graph's live content (nodes, atoms, lists, holders, factorizations or records, "
         "dictionary entries, numerators, array metadata).",
         "- **Oracle**: Python `fractions`, every value and array element; after the workload, every live value and "
         "array is read and compared.  The trial stops at the first output that differs.",
         "- **K phase**: each trial's next periodic check comes after a uniformly random 1..K operations.",
         "- **Control**: each job first runs its workload without a fault; it must match the oracle everywhere, with "
         "no repair and no halt (so verification has no false positives).", ""]
    return H


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--smoke", action="store_true", help="a small run into build/smoke/ (not the measured run)")
    ap.add_argument("--workloads", type=int, default=100)
    ap.add_argument("--per", type=int, default=20, help="trials per class per workload")
    ap.add_argument("--length", type=int, default=2000)
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 2)
    ap.add_argument("--reps", type=int, default=5, help="overhead runs per configuration")
    ap.add_argument("--timeout", type=int, default=10, help="seconds before a trial counts as a hang")
    ap.add_argument("--no-overhead", action="store_true")
    a = ap.parse_args()
    if a.smoke:
        a.workloads, a.per, a.length, a.reps = min(a.workloads, 6), min(a.per, 4), min(a.length, 600), 1
        outdir = os.path.join(HERE, "build", "smoke")
    else:
        outdir = os.path.join(HERE, "results")
        try:
            rule_text = open(RULE_FILE).read()
        except OSError:
            sys.exit(f"fault_bench.py: {RULE_FILE} is missing; the rule must be pre-registered there first")
        if RULE not in rule_text:
            sys.exit(f"fault_bench.py: {RULE_FILE} does not contain the decision rule verbatim; refusing to run")
        if (a.workloads, a.per, a.length) != (100, 20, 2000):
            sys.exit("fault_bench.py: the measured run uses the pre-registered sizes (100 workloads, 20 per class, "
                     "2,000 commands)")
    for v in VARIANTS:
        for b in ("drv", "ovh"):
            if not os.path.exists(os.path.join(HERE, "build", f"{b}_{v}")):
                sys.exit(f"fault_bench.py: build/{b}_{v} is missing; run make first")
    os.makedirs(outdir, exist_ok=True)
    md, js = os.path.join(outdir, "fault.md"), os.path.join(outdir, "fault.json")
    cfg = {"workloads": a.workloads, "per": a.per, "length": a.length, "seed": SEED, "smoke": a.smoke,
           "timeout": a.timeout, "reps": a.reps, "ks": KS, "bootstrap": BOOT}
    with open(md, "w") as f:  # the rule goes into the report before the first trial
        f.write("\n".join(header(cfg)) + "\n")
    t0 = time.time()
    jobs = [(v, K, wi, a.length, a.per, SEED, a.timeout) for wi in range(a.workloads) for v, K in cells()]
    results, errors = [], []
    with Pool(a.jobs) as pool:
        for i, r in enumerate(pool.imap_unordered(run_job, jobs, chunksize=1)):
            if "error" in r:
                errors.append(r)
                print(f"fault_bench.py: CONTROL FAILED {r['v']} K={r['K']} workload {r['wi']}: {r['error'][:300]}",
                      flush=True)
            else:
                results.append(r)
            if (i + 1) % 20 == 0 or i + 1 == len(jobs):
                el = time.time() - t0
                print(f"  {i + 1}/{len(jobs)} jobs, {el / 60:.1f} min, eta {el / (i + 1) * (len(jobs) - i - 1) / 60:.1f}"
                      f" min", flush=True)
    if errors:
        with open(md, "a") as f:
            f.write(f"\n**{len(errors)} jobs failed their control; the run is invalid.**\n")
            for e in errors[:20]:
                f.write(f"- {e['v']} K={e['K']} workload {e['wi']}: `{e['error'][:300]}`\n")
        json.dump({"config": cfg, "errors": errors}, open(js, "w"))
        sys.exit(1)
    wall = time.time() - t0
    ovh = {} if a.no_overhead else run_overhead(a.reps)
    counts, decision = report(cfg, results, ovh, wall, md)
    with open(md, "a") as f:
        f.write("## Caveats\n\n"
                "- Moments are uniform over the workload's commands, so the fault lands in whatever graph state the "
                "workload has then (64 value slots, 16 array slots); faults on values overwritten before any read are "
                "benign by construction.\n"
                f"- A workload has {a.length:,} commands (about two-thirds of them counted operations), so at K = "
                "10,000 most trials see no periodic check before the end: K is judged against this program length.\n"
                "- F9 is uniform over live content bytes, not over all allocated memory (free capacity, empty "
                "dictionary slots and the entry caches are excluded), and O2's records add bytes, so its F9 targets "
                "differ in mix from the others'.\n"
                "- Single-threaded, one graph; faults are injected between operations (F8 inside one); a trial stops "
                "at its first wrong output.\n")
    json.dump({"config": cfg, "counts": {f"{v}/{K}": c for (v, K), c in counts.items()}, "overhead": ovh,
               "decision": decision, "jobs": results}, open(js, "w"))
    print(f"fault_bench.py: wrote {md} and {js} ({wall / 60:.1f} min of trials)")


if __name__ == "__main__":
    main()
