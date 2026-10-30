#!/usr/bin/env python3
"""fault_bench.py: the fault-injection test that decides the consistency option of spec v0.3 section 10.

    make && python3 test_fault.py                 the checks of the harness itself
    python3 fault_bench.py --smoke                a small run into build/smoke/ (not the measured run)
    python3 fault_bench.py                        the measured run: results/fault.md and results/fault.json

The measured run refuses to start unless ../../research/fault-rule.md contains the decision rule (RULE below)
verbatim (revised after the pre-run review), and writes the rule into results/fault.md before the first trial.

Variants (one C build each, from the M1 sources copied into src/; lib/ is not modified).  Every checking variant
runs the same check (the production audit: M1's pg_audit without the primality and pairwise-gcd parts of I1) at the
same moments: every K operations, before every refinement, and once before the readout.  They differ only in what
follows a mismatch.
  N   M1 as accepted: no production checks.
  O1  option 1: exponents only in holder factorizations (the truth), holder ids in node lists (a derived index);
      on a mismatch, rebuild the derived index from the truth and check again; halt if it still fails.
  O2  option 2: one shared record (holder, node, exponent, list position) per edge, referenced from both sides; on a
      mismatch, rebuild membership and positions from the holders' references; halt if it still fails.
  O3  N plus the check, halting on a mismatch (detection only).
  J   O1 plus a journaled refinement (F8 only; it does not compete): the check first rolls an interrupted refinement
      forward from its journal.
One fault per trial; each trial forks from the fault-free run at its moment, so every variant sees the same
workloads, moments, injector seeds and K phases.
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
RULE = """**Decision rule (revised after pre-run review; pre-registered in research/fault-rule.md on 2026-10-28, before the measured run).**

0. **Trials.** Variants N, O1, O2 and O3 compete; J (O1 plus a journaled refinement) runs F8 only and does not
   compete.  Every variant runs the same trials: 100 workloads of 2,000 random commands on single values and arrays,
   then a readout of every live value and array, and for each trial the same class, moment, injector seed and K
   phase.  N runs once (it has no checks); the others run at K = 10, 100 and 1,000.  Every checking variant runs the
   same check (the production audit) every K operations, before every refinement and once before the readout, so
   every trial meets at least one check; they differ only in what follows a mismatch.  There are 2,000 trials per
   class per variant and K (20 per class per workload).  Classes: the **index faults** F1 (a list entry dropped),
   F2 (a stray list entry), F5 (a wrong list position: M1's position array, O2's record position), F6 (a reference
   count) and F7 (a dictionary entry); the **truth corruptions** F3 (a wrong node in a factorization), F4 (a wrong
   exponent in a factorization; on O2, the shared record's exponent) and F8 (a refinement stopped halfway); and the
   **hardware fault** F9 (one bit flipped in the graph's live memory).
1. **Outcomes.** Detected and repaired (all results right); detected and not repaired (a production check halted,
   or the library's own diagnostic aborted); silent wrong (a wrong result returned as success, or a result where the
   oracle has an error); crash (a signal, an abort without a diagnostic, or a hang); benign (all results right),
   split by whether the graph is left inconsistent.  **Undetected** = silent wrong + crash + benign with a latent
   inconsistency.  **Repair-induced** = silent wrong after a repair.  An error status where the oracle has a result
   is a spurious error, reported separately.  A fault that found no target is excluded and counted.
2. **Gate.** R(v, K) is the repair-induced count over all classes.  (v, K) is penalised if the 95% interval of
   R(v, K) (bootstrap over workloads, 10,000 resamples, seed 20261028) lies above 0.  A variant's eligible Ks are its
   unpenalised ones; a variant with none ranks after every variant with one, and if no variant has one, every K is
   eligible.
3. **Primary.** U(v, K) is the undetected count over the index faults F1, F2, F5, F6 and F7.  A variant's primary
   value is its minimum over its eligible Ks (N: its single value).  Two counts are **tied** when the 95% interval of
   their paired difference (bootstrap over workloads, as in 2) contains 0.  The winners are the eligible variants
   whose primary value is tied with the lowest; a winner's admissible Ks are its eligible Ks tied with its own value.
4. **First tie-break.** D(v, K) is the detected count (repaired + not repaired) over the same index faults.  Among
   the winners, a variant's value is its maximum over its admissible Ks; the winners are those tied with the highest,
   and the admissible Ks shrink to those tied with the variant's value.
5. **Second tie-break: overhead**, only between variants tied on both 3 and 4, each at its largest admissible K
   (fixed before any timing is read; N has no K).  Overhead is measured on two workloads separately (a decimal array
   expression; many distinct denominators), in 7 rounds that run every configuration once in a rotated order.  X is
   compared with Y per workload by the median over rounds of T(X) / T(Y), with a 95% bootstrap interval over rounds.
   X is cheaper than Y if the interval lies below 1/1.1 on at least one workload and above 1.1 on none.  A variant
   cheaper than every other remaining one is the decision; otherwise it is a tie, reported with no verdict forced.
   Memory is reported and does not decide.
6. **Reported, not deciding:** the truth-corruption class (undetected, detected and repair-induced counts; its
   repair-induced count enters the gate); F9 per memory region, not pooled, because the byte surfaces differ between
   variants; F8 in its own section, with J against O1; spurious errors; crashes inside a check.
7. **Residual risk.** F3 and F4 corrupt the single source of truth itself, so no cross-check against derived data
   can tell which side is right: detection is incidental (a cached p^a or a list position happens to disagree), and
   a rebuild from that truth enshrines the corruption.  A per-holder checksum over the factorization and the
   numerator, written by every update and checked on read or audit, would catch them.  F8 shows that a refinement
   must be atomic (a journal, or an equivalent)."""

VARIANTS = ["N", "O1", "O2", "O3"]
KS = [10, 100, 1000]
SEED = 20261028
BOOT = 10000
ROUNDS = 7
BAND = 1.1
OUT_NAMES = {"detected_repaired": "detected, repaired", "detected_not_repaired": "detected, not repaired",
             "silent_wrong": "silent wrong", "crash": "crash", "benign": "benign", "spurious_error": "spurious error"}


def ks_of(v):
    return [0] if v == "N" else KS


def cells():
    return [("N", 0)] + [(v, K) for v in ("O1", "O2", "O3") for K in KS]


def all_cells():
    return cells() + [("J", K) for K in KS]


def kname(K):
    return f"{K:,}" if K else "-"


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
    if v == "J":
        specs = [s for s in specs if s[1] == 8]
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

def boot_interval(d, nw, seed=SEED):
    """95% bootstrap interval of sum(d) over workloads (d: per-workload values)."""
    r = random.Random(seed)
    sums = []
    for _ in range(BOOT):
        s = 0
        for _ in range(nw):
            s += d[r.randrange(nw)]
        sums.append(s)
    sums.sort()
    return sums[int(0.025 * BOOT)], sums[int(0.975 * BOOT) - 1]


def tied(a, b, nw):
    lo, hi = boot_interval([x - y for x, y in zip(a, b)], nw)
    return lo <= 0 <= hi, (lo, hi)


def per_workload(results, v, K, classes, nw, pred):
    c = [0] * nw
    for job in results:
        if job["v"] == v and job["K"] == K:
            for t in job["trials"]:
                if t[0] in classes and t[1] is not None and pred(t[1], t[2]):
                    c[job["wi"]] += 1
    return c


def ratio_interval(xs, ys, seed=SEED):
    """median over rounds of x/y (paired by round) and its 95% bootstrap interval"""
    rs = sorted(x / y for x, y in zip(xs, ys))
    n = len(rs)
    med = lambda v: (v[(len(v) - 1) // 2] + v[len(v) // 2]) / 2  # noqa: E731
    r = random.Random(seed)
    bs = sorted(med(sorted(rs[r.randrange(n)] for _ in range(n))) for _ in range(BOOT))
    return med(rs), (bs[int(0.025 * BOOT)], bs[int(0.975 * BOOT) - 1])


def cheaper(ovh, x, y):
    """(x is cheaper than y by the rule, per-workload details)"""
    det, below, above = {}, False, False
    for w in ("dec", "dens"):
        tx, ty = ovh.get(f"{x[0]}/{w}/{x[1]}", {}).get("times"), ovh.get(f"{y[0]}/{w}/{y[1]}", {}).get("times")
        if not tx or not ty:
            return None, det
        m, (lo, hi) = ratio_interval(tx, ty)
        det[w] = (m, lo, hi)
        below |= hi < 1 / BAND
        above |= lo > BAND
    return below and not above, det


def decide(results, ovh, nw):
    L, S = [], {}
    idx, allc = W.INDEX_CLASSES, W.CLASSES
    U = {(v, K): per_workload(results, v, K, idx, nw, W.undetected) for v in VARIANTS for K in ks_of(v)}
    D = {(v, K): per_workload(results, v, K, idx, nw, lambda o, s: W.detected(o)) for v in VARIANTS for K in ks_of(v)}
    R = {(v, K): per_workload(results, v, K, allc, nw, W.repair_induced) for v in VARIANTS for K in ks_of(v)}
    # 2. the gate
    L += ["**2. Gate** (repair-induced silent wrong, all classes; penalised if its 95% interval lies above 0):", ""]
    elig = {}
    for v in VARIANTS:
        elig[v] = []
        for K in ks_of(v):
            lo, hi = boot_interval(R[(v, K)], nw)
            pen = lo > 0
            if not pen:
                elig[v].append(K)
            L.append(f"- {v} K = {kname(K)}: {sum(R[(v, K)])} [{lo}, {hi}] -> {'penalised' if pen else 'passes'}")
    cands = [v for v in VARIANTS if elig[v]]
    if not cands:
        cands = list(VARIANTS)
        elig = {v: ks_of(v) for v in VARIANTS}
        L.append("- every variant is penalised at every K: all compete")
    else:
        pen = [v for v in VARIANTS if v not in cands]
        if pen:
            L.append(f"- ranked after the others (no eligible K): {', '.join(pen)}")
    S["gate_eligible"] = {v: elig[v] for v in VARIANTS}
    # 3. primary
    L += ["", "**3. Primary** (undetected over the index faults F1, F2, F5, F6, F7; each variant at its best eligible K):",
          ""]
    best = {v: min(elig[v], key=lambda K: (sum(U[(v, K)]), -K)) for v in cands}
    low = min(cands, key=lambda v: sum(U[(v, best[v])]))
    win, adm = [], {}
    for v in cands:
        t, iv = (True, (0, 0)) if v == low else tied(U[(v, best[v])], U[(low, best[low])], nw)
        if t:
            win.append(v)
        L.append(f"- {v}: {sum(U[(v, best[v])])} at K = {kname(best[v])}; difference with {low} [{iv[0]}, {iv[1]}]"
                 f" -> {'winner' if t else 'worse'}")
    for v in win:
        adm[v] = [K for K in elig[v] if K == best[v] or tied(U[(v, K)], U[(v, best[v])], nw)[0]]
        L.append(f"  - {v} admissible K: {', '.join(kname(k) for k in adm[v])}")
    S["primary_winners"] = win
    # 4. detected count
    L += ["", "**4. First tie-break** (detected over the index faults; higher is better):", ""]
    bd = {v: max(adm[v], key=lambda K: (sum(D[(v, K)]), K)) for v in win}
    high = max(win, key=lambda v: sum(D[(v, bd[v])]))
    win2 = []
    for v in win:
        t, iv = (True, (0, 0)) if v == high else tied(D[(v, bd[v])], D[(high, bd[high])], nw)
        if t:
            win2.append(v)
        L.append(f"- {v}: {sum(D[(v, bd[v])])} at K = {kname(bd[v])}; difference with {high} [{iv[0]}, {iv[1]}] -> "
                 f"{'winner' if t else 'worse'}")
    for v in win2:
        adm[v] = [K for K in adm[v] if K == bd[v] or tied(D[(v, K)], D[(v, bd[v])], nw)[0]]
        L.append(f"  - {v} admissible K: {', '.join(kname(k) for k in adm[v])}")
    S["tiebreak1_winners"] = win2
    L.append("")
    if len(win2) == 1:
        S["decision"] = {"variant": win2[0], "K": adm[win2[0]]}
        L.append(f"**Decision: {win2[0]}** (admissible K: {', '.join(kname(k) for k in adm[win2[0]])}); overhead "
                 f"does not enter.")
        return L, S
    # 5. overhead
    at = {v: (v, max(adm[v])) for v in win2}
    L += [f"**5. Second tie-break** (overhead, each at its largest admissible K: "
          f"{', '.join(f'{v} K = {kname(at[v][1])}' for v in win2)}):", ""]
    beats = {}
    for x in win2:
        beats[x] = True
        for y in win2:
            if x == y:
                continue
            c, det = cheaper(ovh, at[x], at[y])
            L.append(f"- {x} vs {y}: " + "; ".join(f"{w} {m:.3f} [{lo:.3f}, {hi:.3f}]" for w, (m, lo, hi) in det.items())
                     + f" -> {'cheaper' if c else 'not cheaper' if c is not None else 'not measured'}")
            beats[x] = beats[x] and bool(c)
    dec = [v for v in win2 if beats[v]]
    L.append("")
    if len(dec) == 1:
        S["decision"] = {"variant": dec[0], "K": at[dec[0]][1]}
        L.append(f"**Decision: {dec[0]}** at K = {kname(at[dec[0]][1])}.")
    else:
        S["decision"] = {"tie": win2}
        L.append(f"**Decision: a tie** between {', '.join(win2)}; no verdict is forced.")
    return L, S


# ------------------------------------------------------------------------------------------------- overhead

def run_overhead(rounds, scale):
    configs = [(v, w, K) for w in ("dec", "dens") for v in VARIANTS for K in ks_of(v)]
    ovh = {f"{v}/{w}/{K}": {"times": []} for v, w, K in configs}
    for rd in range(rounds):
        order = configs[rd % len(configs):] + configs[:rd % len(configs)]  # rotated each round
        for v, w, K in order:
            res = subprocess.run([os.path.join(HERE, "build", f"ovh_{v}"), w, str(K), "1", str(scale)],
                                 capture_output=True, text=True)
            f = res.stdout.split()
            o = ovh[f"{v}/{w}/{K}"]
            if res.returncode or len(f) < 10:
                o["error"] = res.stderr[:300]
                continue
            o["times"].append(float(f[4]))
            o["mem_peak"], o["checks"], o["checksum"] = int(f[6]), int(f[7]), f[9]
        print(f"  overhead round {rd + 1}/{rounds} done", flush=True)
    for o in ovh.values():
        ts = sorted(o["times"])
        if ts:
            o["median"] = (ts[(len(ts) - 1) // 2] + ts[len(ts) // 2]) / 2
    return ovh


# ------------------------------------------------------------------------------------------------- report

def table(head, rows):
    out = ["| " + " | ".join(head) + " |", "|" + "|".join(" --- " for _ in head) + "|"]
    out += ["| " + " | ".join(str(c) for c in r) + " |" for r in rows]
    return out


def tally(results):
    T = {}
    for job in results:
        cell = (job["v"], job["K"])
        for t in job["trials"]:
            T.setdefault(cell, {}).setdefault(t[0], []).append((t[1], t[2], t[3]))
    return T


def row_counts(ts):
    c = {o: 0 for o in W.OUTCOMES}
    c.update(none=0, undet=0, rep_ind=0, latent=0, in_check=0, internal=0)
    for o, sub, _ in ts:
        if o is None:
            c["none"] += 1
            continue
        c[o] += 1
        c["undet"] += W.undetected(o, sub)
        c["rep_ind"] += W.repair_induced(o, sub)
        c["latent"] += o == "benign" and sub == "latent inconsistency left"
        c["in_check"] += o == "crash" and sub.startswith("inside a check")
        c["internal"] += o == "detected_not_repaired" and sub.startswith("internal diagnostic")
    return c


def outcome_table(T, cell_list, classes):
    head = ["variant", "K", "class", "undetected", "silent wrong (after a repair)", "silent wrong (other)",
            "crash (inside a check)", "benign, latent", "benign, clean", "detected, repaired",
            "detected, not repaired (internal diagnostic)", "spurious error", "not injected"]
    rows = []
    for cell in cell_list:
        for k in classes:
            ts = T.get(cell, {}).get(k)
            if ts is None:
                continue
            c = row_counts(ts)
            rows.append([cell[0], kname(cell[1]), f"F{k}", c["undet"], c["rep_ind"], c["silent_wrong"] - c["rep_ind"],
                         f"{c['crash']} ({c['in_check']})", c["latent"], c["benign"] - c["latent"],
                         c["detected_repaired"], f"{c['detected_not_repaired']} ({c['internal']})",
                         c["spurious_error"], c["none"]])
    return table(head, rows)


def summary_table(T, cell_list):
    head = ["variant", "K", "index: undetected", "index: detected", "truth: undetected", "truth: repair-induced",
            "all: repair-induced", "F9: undetected"]
    rows = []
    for cell in cell_list:
        def agg(classes, key):
            return sum(row_counts(T.get(cell, {}).get(k, []))[key] for k in classes)
        det = sum(row_counts(T.get(cell, {}).get(k, []))["detected_repaired"] +
                  row_counts(T.get(cell, {}).get(k, []))["detected_not_repaired"] for k in W.INDEX_CLASSES)
        rows.append([cell[0], kname(cell[1]), agg(W.INDEX_CLASSES, "undet"), det, agg(W.TRUTH_CLASSES, "undet"),
                     agg(W.TRUTH_CLASSES, "rep_ind"), agg(W.CLASSES, "rep_ind"), agg([9], "undet")])
    return table(head, rows)


def report(cfg, results, ovh, wall, path_md):
    nw = cfg["workloads"]
    T = tally(results)
    L = ["## Results", "",
         f"{sum(len(j['trials']) for j in results):,} trials in {wall / 60:.1f} min; {nw} workloads of "
         f"{cfg['length']:,} commands; {cfg['per']} trials per class per workload.", "",
         "### Summary", ""]
    L += summary_table(T, cells())
    L += ["", "### Index faults (decide)", ""]
    L += outcome_table(T, cells(), W.INDEX_CLASSES)
    L += ["", "### Truth corruptions F3, F4 (scored; repair-induced enters the gate)", "",
          "F3 and F4 corrupt the single source of truth itself, so no cross-check can tell which side is right. "
          "Detection is incidental (a cached p^a, a list position or the edge order happens to disagree), and a "
          "rebuild from that truth enshrines the corruption (the repair-induced column).  A per-holder checksum over "
          "the factorization and the numerator would catch them.", ""]
    L += outcome_table(T, cells(), [3, 4])
    L += ["", "### F8: an interrupted refinement (its own section; not in the index totals)", "",
          "The half-update is in the truth: the split node's atoms have changed and only part of its holders were "
          "rewritten (graph.c, split_out).  A rebuild from the factorizations cannot recover it, so refinement must "
          "be atomic.  J is O1 plus a journal written before the atoms change (N, P, Q, R, gQ, gR); its check first "
          "rolls an interrupted refinement forward (a holder that already has its P edge is done).  J does not "
          "compete in the decision.", ""]
    L += outcome_table(T, [c for c in all_cells() if c[0] in ("N", "O1", "O3", "J")] +
                       [c for c in cells() if c[0] == "O2"], [8])
    L += ["", "### F9 by memory region (undetected / trials; not pooled, the byte surfaces differ)", ""]
    regs = {}
    for cell in cells():
        for o, sub, rg in T.get(cell, {}).get(9, []):
            if o is None:
                continue
            x = regs.setdefault(rg or "?", {}).setdefault(cell, [0, 0])
            x[0] += W.undetected(o, sub)
            x[1] += 1
    allr = sorted(regs)
    L += table(["variant", "K"] + allr, [[c[0], kname(c[1])] + [
        f"{regs[r][c][0]}/{regs[r][c][1]}" if c in regs[r] else "-" for r in allr] for c in cells()])
    L += ["", "Full F9 outcomes:", ""]
    L += outcome_table(T, cells(), [9])
    L += ["", "### Overhead (no fault; median of the rounds, ratio to N with its 95% interval)", ""]
    rows = []
    for w in ("dec", "dens"):
        for v in VARIANTS:
            for K in ks_of(v):
                o, b = ovh.get(f"{v}/{w}/{K}", {}), ovh.get(f"N/{w}/0", {})
                if not o.get("times") or not b.get("times"):
                    rows.append([w, v, kname(K)] + ["-"] * 4)
                    continue
                m, (lo, hi) = ratio_interval(o["times"], b["times"])
                rows.append([w, v, kname(K), f"{o['median']:.4f}", f"{m:.3f} [{lo:.3f}, {hi:.3f}]",
                             f"{o['mem_peak'] / 1e6:.2f}", o["checks"]])
    L += table(["workload", "variant", "K", "median s", "vs N", "peak MB", "checks per run"], rows)
    sums = {w: {ovh.get(f"{v}/{w}/{K}", {}).get("checksum") for v in VARIANTS for K in ks_of(v)} for w in ("dec", "dens")}
    L += ["", "Result checksums equal across variants: " +
          ", ".join(f"{w} {'yes' if len(s) == 1 else 'NO'}" for w, s in sums.items()) + ".", "",
          "## Decision (by the pre-registered rule)", ""]
    dl, ds = decide(results, ovh, nw)
    L += dl + [""]
    with open(path_md, "a") as f:
        f.write("\n".join(L) + "\n")
    return T, ds


def header(cfg):
    return [f"# Fault injection: the consistency option (spec v0.3 §10)", "",
            f"{time.strftime('%b %d, %Y')} · `test/fault/fault_bench.py`"
            f"{' (SMOKE RUN: not the measured run)' if cfg['smoke'] else ''}", "",
            "## Rule", "", RULE, "",
            "## Setup", "",
            "- **Variants** (one C build each from the M1 sources, copied into `test/fault/src/`): N, O1, O2, O3 and J "
            "as in the rule.  The check is M1's `pg_audit` without the primality and pairwise-gcd parts of I1, run "
            "every K operations (a public call that mutates or reduces: M1's `DEBUG_AUDIT` points), before every "
            "refinement and before the readout.",
            "- **Faults**, one per trial, before a random command of the workload (F8: a random refinement of it): "
            "F1 drop one list entry (count and length both lowered, as a buggy unlink would); F2 append a stray holder "
            "(O2: a stray record); F3 a wrong node id in a factorization; F4 a wrong exponent (+-1 or +2; on O2 the "
            "shared record); F5 a wrong list position (another position in the list, or one past a list of one); "
            "F6 a reference count +-1 or +-2; F7 an atom's dictionary entry to another live node; F8 a refinement's "
            "copy-and-loop stopped halfway, the program continuing; F9 one bit, uniform over the bytes of the graph's "
            "live content (nodes, atoms, lists, holders, factorizations or records, dictionary entries, numerators, "
            "array metadata).",
            "- **Oracle**: Python `fractions`; after the workload, every live value and array is read and compared. "
            "A trial stops at its first wrong output.  Benign trials are audited once more afterwards (not a "
            "production check) to find latent inconsistencies.",
            "- **K phase**: each trial's next periodic check comes after a uniformly random 1..K operations.",
            "- **Control**: each job first runs its workload without a fault; it must match the oracle everywhere, "
            "with no repair and no halt (so the check has no false positives).", ""]


CAVEATS = """## Caveats and disclosures

- **Crashes inside the check.** The audit follows pointers and indices that the fault may have corrupted, and can
  crash itself; such crashes count as crashes (undetected) for every checking variant, and are shown in the
  "(inside a check)" column.  They inflate the crash counts of O1, O2 and O3 relative to N.
- **N's detections** come only from M1's own internal consistency checks (`pgi_die` diagnostics such as "unlink: a
  list names a holder without the edge"); they are counted as detected, not repaired, and shown in the "(internal
  diagnostic)" column.  The other variants get the same credit when those checks fire first.
- Moments are uniform over the workload's commands, so a fault lands in whatever state the workload has then
  (64 value slots, 16 array slots); faults in values overwritten before any read end benign (clean or latent).
- F9 is uniform over the live content bytes, not over all allocated memory (free capacity, empty dictionary slots
  and the entry caches are excluded); O2's records add bytes, so its regions differ in size and mix.
- Single-threaded, one graph; faults are injected between operations (F8 inside one).
"""


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--smoke", action="store_true", help="a small run into build/smoke/ (not the measured run)")
    ap.add_argument("--workloads", type=int, default=100)
    ap.add_argument("--per", type=int, default=20, help="trials per class per workload")
    ap.add_argument("--length", type=int, default=2000)
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 2)
    ap.add_argument("--rounds", type=int, default=ROUNDS, help="overhead rounds")
    ap.add_argument("--timeout", type=int, default=10, help="seconds before a trial counts as a hang")
    ap.add_argument("--no-overhead", action="store_true")
    a = ap.parse_args()
    scale = 1
    if a.smoke:
        a.workloads, a.per, a.length, a.rounds = min(a.workloads, 6), min(a.per, 4), min(a.length, 600), 2
        scale = 20
        outdir = os.path.join(HERE, "build", "smoke")
    else:
        outdir = os.path.join(HERE, "results")
        try:
            rule_text = open(RULE_FILE).read()
        except OSError:
            sys.exit(f"fault_bench.py: {RULE_FILE} is missing; the rule must be pre-registered there first")
        if RULE not in rule_text:
            sys.exit(f"fault_bench.py: {RULE_FILE} does not contain the decision rule verbatim; refusing to run")
        if (a.workloads, a.per, a.length, a.rounds) != (100, 20, 2000, ROUNDS) or a.no_overhead:
            sys.exit("fault_bench.py: the measured run uses the pre-registered sizes (100 workloads, 20 per class, "
                     "2,000 commands, 7 overhead rounds)")
    for v in VARIANTS + ["J"]:
        if not os.path.exists(os.path.join(HERE, "build", f"drv_{v}")):
            sys.exit(f"fault_bench.py: build/drv_{v} is missing; run make first")
    os.makedirs(outdir, exist_ok=True)
    md, js = os.path.join(outdir, "fault.md"), os.path.join(outdir, "fault.json")
    cfg = {"workloads": a.workloads, "per": a.per, "length": a.length, "seed": SEED, "smoke": a.smoke,
           "timeout": a.timeout, "rounds": a.rounds, "scale": scale, "ks": KS, "bootstrap": BOOT}
    with open(md, "w") as f:  # the rule goes into the report before the first trial
        f.write("\n".join(header(cfg)) + "\n")
    t0 = time.time()
    jobs = [(v, K, wi, a.length, a.per, SEED, a.timeout) for wi in range(a.workloads) for v, K in all_cells()]
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
    t1 = time.time()
    ovh = {} if a.no_overhead else run_overhead(a.rounds, scale)
    cfg["overhead_minutes"] = (time.time() - t1) / 60
    T, decision = report(cfg, results, ovh, wall, md)
    with open(md, "a") as f:
        f.write(CAVEATS)
    json.dump({"config": cfg, "overhead": ovh, "decision": decision,
               "counts": {f"{v}/{K}/F{k}": row_counts(ts) for (v, K), byc in T.items() for k, ts in byc.items()},
               "jobs": results}, open(js, "w"))
    print(f"fault_bench.py: wrote {md} and {js} ({wall / 60:.1f} min of trials, "
          f"{cfg['overhead_minutes']:.1f} min of overhead)")


if __name__ == "__main__":
    main()
