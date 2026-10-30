#!/usr/bin/env python3
"""test_fault.py: checks of the fault-injection test itself (not the measured run).

    python3 test_fault.py            (after make; about half a minute)

1. Each injector performs its fault: for every variant and every class F1-F7 and F9, over many seeds and graph
   states, the driver's self-check compares the graph just before and just after the injection and confirms that
   exactly the intended change happened (F1: one list entry gone, its holder keeping the edge; F2: one stray entry
   whose holder has no edge; F3/F4/F5: one edge's node or exponent changed; F6: one reference count; F7: one atom's
   dictionary entry; F5: one list position; F9: exactly one bit of one byte) and nothing else in the index or the
   truth changed.  F8 must stop a refinement halfway; J's journal must finish it.
2. Each outcome is reached end to end through the job runner, with workloads built so that the outcome is known:
   silent wrong, detected and repaired, detected and not repaired (production check and internal diagnostic),
   crash (signal and hang), benign, plus the not-injected cases.
3. classify() on synthetic records, branch by branch.
4. The control rejects a workload whose oracle is wrong; random workloads pass the control on every variant.
"""
import os
import random
import subprocess
import sys
import types
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import workload as W  # noqa: E402

VARIANTS = ["N", "O1", "O2", "O3", "J"]


def drv(v):
    return os.path.join(HERE, "build", f"drv_{v}")


def script(v, lines, K=0, env=None):
    res = subprocess.run([drv(v), "--script", "--K", str(K)], input="\n".join(lines) + "\nEND\n",
                         capture_output=True, text=True, env=env, timeout=60)
    return res.returncode, res.stdout.splitlines()


def job(v, cmds, exps, trials, K=0, workend=None, env=None):
    w = types.SimpleNamespace(cmds=cmds, exps=exps, workend=len(cmds) if workend is None else workend)
    res = subprocess.run([drv(v)], input=W.job_text(w, K, trials), capture_output=True, text=True, env=env,
                         timeout=120)
    lines = res.stdout.splitlines()
    return lines[0] if lines else res.stderr, [W.parse_record(l) for l in lines[1:] if l.startswith("T\t")]


# A graph with shared multi-atom nodes, prime nodes, large primes, values, arrays with D and exceptions.
SETUP = ["V 0 1 30", "V 1 7 30", "V 2 1 45", "V 3 5 12", "V 4 3 1000003", "V 5 1 6", "V 6 11 2147483647",
         "AF 0 6 1 10 3 100 7 1000 1 3 2 7 5 1000003", "AF 1 6 1 2 1 4 1 8 1 16 3 7 1 5", "B + 7 0 2", "B * 8 3 5",
         "AO + 2 0 1", "V 9 13 210", "AV 3 4 0 1 2 3"]


class Injectors(unittest.TestCase):
    def test_each_injector_performs_its_fault(self):
        r = random.Random(5)
        for v in VARIANTS:
            for cls in (1, 2, 3, 4, 5, 6, 7, 9):
                for trial in range(40):
                    k = r.randrange(4, len(SETUP) + 1)
                    seed = r.randrange(1, 2**31)
                    rc, out = script(v, SETUP[:k] + [f"ZC {cls} {seed}"])
                    self.assertEqual(rc, 0, (v, cls, out[-1:]))
                    line = out[-1]
                    self.assertTrue(line.startswith("zc 1 "), (v, cls, seed, k, line))
                    if cls in (1, 2, 5, 6, 7):  # index faults: the production audit must see them
                        self.assertIn("audit=fail", line, (v, cls, line))

    def test_f8_stops_a_refinement_halfway(self):
        for v in VARIANTS:
            # one holder on node 30: the split of 30 by 45 stops before it (0 of 1)
            # (N, O3: the graph is left inconsistent; O1, O2: the next refinement in the same entry verifies first
            # and rebuilds, so the record shows a repair)
            rc, out = script(v, ["V 0 1 30", "Z8 1", "V 1 1 45", "ZL", "AU", "REC", "P 0"])
            if v == "O3":  # the next refinement of the same entry is audited first: halt
                self.assertEqual(rc, 42, (v, out))
                continue
            self.assertEqual(out[3], "zl cls=8 pos=0 old=1", (v, out))
            self.check_f8_effect(v, out[4], out[5], out[6])
            # four holders: it stops after 2 of 4
            rc, out = script(v, ["V 0 1 30", "V 1 7 30", "V 2 11 30", "V 3 13 30", "Z8 1", "V 4 1 45", "ZL", "AU",
                                 "REC", "P 2"])
            self.assertEqual(out[6], "zl cls=8 pos=2 old=4", (v, out))
            self.check_f8_effect(v, out[7], out[8], out[9], "= 11 30")  # 11/30 was in the half left behind
            # not armed: the same refinement completes and the graph is consistent
            rc, out = script(v, ["V 0 1 30", "V 1 7 30", "V 2 11 30", "V 3 13 30", "V 4 1 45", "AU"])
            self.assertEqual(out[-1], "au ok", (v, out))

    def check_f8_effect(self, v, au, rec, p0, right="= 1 30"):
        if v in ("O1", "O2", "J"):
            self.assertIn("nrepair=1", rec, (v, rec))
            self.assertEqual(p0 == right, v == "J", (v, p0))  # only J's roll-forward keeps the value right
            self.assertEqual("nroll=1" in rec, v == "J", (v, rec))
        else:
            self.assertNotEqual(au, "au ok", (v, au))
            self.assertIn("nrepair=0", rec, (v, rec))

    def test_no_injection_leaves_graph_consistent(self):
        for v in VARIANTS:
            rc, out = script(v, SETUP + ["AU"])
            self.assertEqual(out[-1], "au ok", (v, out))


def X(s):
    return "X " + s


class Outcomes(unittest.TestCase):
    """End to end through the job runner: a trial's record and its classification."""

    def one(self, v, cmds, exps, cls, moment, K=0, env=None, workend=None):
        ctl, recs = job(v, cmds, exps, [(0, cls, moment, 1, 0)], K=K, env=env, workend=workend)
        self.assertTrue(ctl.startswith("CONTROL\tok"), (v, ctl))
        self.assertEqual(len(recs), 1)
        return W.classify(recs[0]), recs[0]

    def test_silent_wrong(self):
        # N: 1/6 with its exponent on node 6 raised (F4, the only target), then 1/6 + 1/6 is computed wrong
        cmds = ["V 0 1 6", "V 1 1 6", "B + 2 0 1", "P 2"]
        exps = [X("= 1 6"), X("= 1 6"), X("= 1 3"), X("= 1 3")]
        (o, sub), d = self.one("N", cmds, exps, 4, 1)
        self.assertEqual(o, "silent_wrong", d)
        self.assertEqual(d["mismatch_cmd"], 2)

    def test_silent_wrong_after_repair(self):
        # O1, F8: the split of 30 by 45 stops before 1/30's update; the rebuild treats the half-updated
        # factorization as the truth, and 1/30 silently becomes 1/10
        cmds = ["V 0 1 30", "V 1 1 45", "P 0", "P 1"]
        exps = [X("= 1 30"), X("= 1 45"), X("= 1 30"), X("= 1 45")]
        for v in ("O1", "O2"):
            (o, sub), d = self.one(v, cmds, exps, 8, 0, K=1)
            self.assertEqual((o, sub), ("silent_wrong", "after a repair"), d)
            self.assertEqual(d["got"], "= 1 10")
        # J: the journal rolls the refinement forward; O3: its pre-refinement audit halts
        (o, sub), d = self.one("J", cmds, exps, 8, 0, K=1)
        self.assertEqual(o, "detected_repaired", d)
        self.assertEqual(d["nroll"], 1)
        (o, sub), d = self.one("O3", cmds, exps, 8, 0, K=1000)
        self.assertEqual((o, sub), ("detected_not_repaired", "production check"), d)

    def test_detected_and_repaired(self):
        # O1/O2, F1 (one of two holders of node 6 dropped from its list), check after every operation
        cmds = ["V 0 1 6", "V 1 5 6", "B + 2 0 1", "P 0", "P 1", "P 2"]
        exps = [X("= 1 6"), X("= 5 6"), X("= 1 1"), X("= 1 6"), X("= 5 6"), X("= 1 1")]
        for v in ("O1", "O2"):
            (o, sub), d = self.one(v, cmds, exps, 1, 2, K=1)
            self.assertEqual(o, "detected_repaired", (v, d))
            self.assertEqual(d["latent"], 0)

    def test_detected_not_repaired_by_audit(self):
        cmds = ["V 0 1 6", "V 1 5 6", "B + 2 0 1", "P 0", "P 1", "P 2"]
        exps = [X("= 1 6"), X("= 5 6"), X("= 1 1"), X("= 1 6"), X("= 5 6"), X("= 1 1")]
        (o, sub), d = self.one("O3", cmds, exps, 1, 2, K=1)
        self.assertEqual((o, sub), ("detected_not_repaired", "production check"), d)
        self.assertEqual(d["exit"], W.EXIT_HALT)

    def test_detected_not_repaired_by_internal_check(self):
        # N, F2: a stray holder in node 6's list; releasing the last listed holder moves the stray one, and unlink
        # finds that it has no edge (the library's own diagnostic)
        cmds = ["V 0 1 6", "V 1 5 6", "I 2 7", "R 0", "R 1", "P 2"]
        exps = [X("= 1 6"), X("= 5 6"), X("= 7 1"), X("ok"), X("ok"), X("= 7 1")]
        seen = set()
        for seed in range(1, 40):
            ctl, recs = job("N", cmds, exps, [(0, 2, 3, seed, 0)])
            o, sub = W.classify(recs[0])
            seen.add(o)
            if o == "detected_not_repaired":
                self.assertTrue(sub.startswith("internal diagnostic"), sub)
                self.assertEqual(recs[0]["sig"], W.SIGABRT)
                break
        self.assertIn("detected_not_repaired", seen)

    def test_crash_signal_and_hang(self):
        # the driver's handling of a child killed by a signal and of a hang (timeout), seen through the control run
        # (a control that crashes is reported with its record); classified as a trial would be
        for cmd, env, want in (("SEGV", None, ("crash", "signal 11")),
                               ("HANG", dict(os.environ, FAULT_TIMEOUT="1"), ("crash", "hang (timeout)"))):
            w = types.SimpleNamespace(cmds=["V 0 1 6", cmd, "P 0"], exps=[X("= 1 6"), "I", X("= 1 6")], workend=3)
            res = subprocess.run([drv("N")], input=W.job_text(w, 0, [(0, 6, 1, 1, 0)]), capture_output=True,
                                 text=True, env=env, timeout=60)
            line = res.stdout.splitlines()[0]
            self.assertTrue(line.startswith("CONTROL\tfail"), line)
            d = W.parse_record(line[len("CONTROL\tfail"):])
            d["injected"] = 1
            self.assertEqual(W.classify(d), want, d)

    def test_crash_end_to_end(self):
        # a real crash from a fault: F6 lowers node 6's count to 0 (seeded), the release of 1/6 frees the node while
        # 5/6 still uses it; later entries reuse it.  Over seeds, at least one signal-terminated trial must appear,
        # and every signal is classified as a crash.
        cmds = ["V 0 1 6", "V 1 5 6", "R 0", "V 2 1 35", "V 3 1 77", "B + 4 1 2", "B * 5 1 3", "P 1", "P 2", "P 3",
                "P 4", "P 5"]
        exps = [X("= 1 6"), X("= 5 6"), X("ok"), X("= 1 35"), X("= 1 77"), X("= 181 210"), X("= 5 462"),
                X("= 5 6"), X("= 1 35"), X("= 1 77"), X("= 181 210"), X("= 5 462")]
        outs = {}
        for v in VARIANTS:
            ctl, recs = job(v, cmds, exps, [(i, c, 2, i + 1, 0) for i, c in enumerate([6, 1, 9] * 30)])
            self.assertTrue(ctl.startswith("CONTROL\tok"), ctl)
            for d in recs:
                o, sub = W.classify(d)
                outs.setdefault(v, set()).add(o)
                if d["sig"]:
                    self.assertIn(o, ("crash", "detected_not_repaired"))
        self.assertIn("crash", outs["N"])

    def test_benign(self):
        cmds = ["V 0 1 6", "P 0"]
        exps = [X("= 1 6"), X("= 1 6")]
        (o, sub), d = self.one("N", cmds, exps, 6, 1)
        self.assertEqual(o, "benign", d)
        self.assertEqual(sub, "latent inconsistency left")

    def test_not_injected(self):
        cmds = ["V 0 1 6", "P 0"]
        exps = [X("= 1 6"), X("= 1 6")]
        ctl, recs = job("N", cmds, exps, [(0, 8, 5, 1, 0)])
        self.assertEqual(W.classify(recs[0]), (None, "no refinement in the workload"))
        # F5 applies to every variant now; on N a wrong position in 1/6's edge is benign but left latent
        for v in VARIANTS:
            ctl, recs = job(v, cmds, exps, [(1, 5, 1, 1, 0)])
            self.assertEqual(recs[0]["injected"], 1, v)
        ctl, recs = job("N", cmds, exps, [(1, 5, 1, 1, 0)])
        self.assertEqual(W.classify(recs[0]), ("benign", "latent inconsistency left"))
        self.assertTrue(W.undetected(*W.classify(recs[0])))

    def test_final_check_before_readout(self):
        # F1 early, a K too large to fire and a phase at its end: only the check before the readout can see it
        cmds = ["V 0 1 6", "V 1 5 6", "I 2 3", "P 0", "P 1", "P 2"]
        exps = [X("= 1 6"), X("= 5 6"), X("= 3 1"), X("= 1 6"), X("= 5 6"), X("= 3 1")]
        got = {}
        for v in VARIANTS:
            ctl, recs = job(v, cmds, exps, [(0, 1, 2, 1, 999)], K=1000 if v != "N" else 0, workend=3)
            got[v] = W.classify(recs[0])
        self.assertEqual(got["N"], ("benign", "latent inconsistency left"))
        self.assertEqual(got["O3"], ("detected_not_repaired", "production check"))
        for v in ("O1", "O2", "J"):
            self.assertEqual(got[v], ("detected_repaired", "consistent"), v)

    def test_phase_controls_the_next_check(self):
        # O3 at K=3 with the fault before command 1: phase 0 checks after the first operation (halt before the
        # wrong sum is printed), a late phase lets the sum escape first
        cmds = ["V 0 1 6", "V 1 1 6", "B + 2 0 1", "P 2"]
        exps = [X("= 1 6"), X("= 1 6"), X("= 1 3"), X("= 1 3")]
        ctl, recs = job("O3", cmds, exps, [(0, 4, 1, 1, 0), (1, 4, 1, 1, 2)], K=3)
        got = {d["id"]: W.classify(d)[0] for d in recs}
        self.assertEqual(got[0], "detected_not_repaired")
        self.assertEqual(got[1], "silent_wrong")


class Classify(unittest.TestCase):
    base = {"injected": 1, "mismatch_cmd": -1, "mismatch_kind": 0, "sig": 0, "halt": 0, "exit": 0, "completed": 1,
            "nrepair": 0, "latent": 0, "nverify": 0}

    def c(self, **kw):
        d = dict(self.base)
        d.update(kw)
        return W.classify(d)

    def test_branches(self):
        self.assertEqual(self.c()[0], "benign")
        self.assertEqual(self.c(latent=1), ("benign", "latent inconsistency left"))
        self.assertEqual(self.c(nrepair=2)[0], "detected_repaired")
        self.assertEqual(self.c(mismatch_cmd=5, mismatch_kind=1)[0], "silent_wrong")
        self.assertEqual(self.c(mismatch_cmd=5, mismatch_kind=1, nrepair=1), ("silent_wrong", "after a repair"))
        self.assertEqual(self.c(mismatch_cmd=5, mismatch_kind=2)[0], "spurious_error")
        # a wrong result that escaped before a later halt is still silent wrong (the child stops at the mismatch)
        self.assertEqual(self.c(mismatch_cmd=5, mismatch_kind=1, sig=6, halt=2)[0], "silent_wrong")
        self.assertEqual(self.c(exit=W.EXIT_HALT, halt=1, completed=0)[0], "detected_not_repaired")
        self.assertEqual(self.c(sig=6, halt=2, exit=-1, completed=0)[0], "detected_not_repaired")
        self.assertEqual(self.c(sig=6, halt=0, exit=-1, completed=0), ("crash", "abort without a diagnostic"))
        self.assertEqual(self.c(sig=11, exit=-1, completed=0), ("crash", "signal 11"))
        self.assertEqual(self.c(sig=14, exit=-1, completed=0), ("crash", "hang (timeout)"))
        self.assertEqual(self.c(injected=0)[0], None)
        self.assertEqual(self.c(injected=-1)[0], None)
        self.assertEqual(self.c(exit=3, completed=0)[0], "crash")
        self.assertEqual(self.c(sig=11, exit=-1, completed=0, in_check=1), ("crash", "inside a check (signal 11)"))
        self.assertTrue(W.undetected("crash", "signal 11") and W.undetected("silent_wrong", "x"))
        self.assertFalse(W.undetected("benign", "no trace") or W.undetected("detected_repaired", "consistent"))
        self.assertTrue(W.repair_induced("silent_wrong", "after a repair"))


class Control(unittest.TestCase):
    def test_wrong_oracle_is_rejected(self):
        ctl, recs = job("N", ["V 0 1 6", "P 0"], [X("= 1 6"), X("= 1 7")], [(0, 4, 1, 1, 0)])
        self.assertTrue(ctl.startswith("CONTROL\tfail"), ctl)
        self.assertEqual(recs, [])

    def test_check_inside_expression_end(self):
        # regression: the measured set's workload 12 refines inside the outermost pg_expr_end, while pending arrays
        # are still deferred at depth 0; the pre-refinement check must accept that (it once halted every variant)
        import fault_bench as B
        w, _ = B.trial_specs(12, 2000, 20, B.SEED)
        for v in VARIANTS:
            res = subprocess.run([drv(v)], input=W.job_text(w, 10 if v != "N" else 0, []), capture_output=True,
                                 text=True)
            self.assertTrue(res.stdout.startswith("CONTROL\tok"), (v, res.stdout[:300]))

    def test_random_workloads_pass_the_control(self):
        for seed in (11, 12, 13):
            w = W.Workload(seed, 600)
            for v in VARIANTS:
                for K in ((0,) if v == "N" else (1, 10, 1000)):
                    res = subprocess.run([drv(v)], input=W.job_text(w, K, []), capture_output=True, text=True)
                    self.assertTrue(res.stdout.startswith("CONTROL\tok"), (seed, v, K, res.stdout[:300]))


if __name__ == "__main__":
    for v in VARIANTS:
        if not os.path.exists(drv(v)):
            sys.exit(f"test_fault.py: {drv(v)} is missing; run make first")
    unittest.main(verbosity=2)
