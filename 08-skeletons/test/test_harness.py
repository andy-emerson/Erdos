"""Self-tests of the harness (not of the library):  python3 test_harness.py  (or pytest)."""
import argparse
import hashlib
import random
import tempfile
from pathlib import Path

import numpy as np

import analyze as A
import common as C
import gen
import runner


def ns(**kw):
    d = dict(toy=False, stub=False, jobs="", hard_stop_s=None, resume=False, rerun=None, pilot_only=False)
    d.update(kw)
    return argparse.Namespace(**d)


def test_rule_refusal():
    with tempfile.TemporaryDirectory() as t:
        p = Path(t) / "rule.md"
        p.write_text(C.RULE_FILE.read_text() + " ")
        old = C.RULE_FILE
        C.RULE_FILE = p
        try:
            runner.Runner(ns()).preflight()
            raise AssertionError("a changed rule.md was not refused")
        except SystemExit as e:
            assert "differs" in str(e)
        finally:
            C.RULE_FILE = old
    assert C.rule_sha256() == C.RULE_SHA256, "rule.md on disk is not the fixed rule"


def test_stub_refused_for_measured_run():
    try:
        runner.Runner(ns(stub=True)).preflight()
        raise AssertionError("stub accepted for a measured run")
    except SystemExit as e:
        assert "stub" in str(e)


def test_lg_pool():
    pool, lists = gen.lg_pool(20)                   # asserts 19 + 38 = 57 elements, 872/464/43/23, 12,198
    assert sum(len(v) for v in lists.values()) == 12_198 and len(pool) == 57


def test_shared_den_bits():
    rng = random.Random(1)
    for sigma in (0.25, 0.5, 0.75):
        for _ in range(500):
            d = gen.draw_shared(rng, sigma, 20)
            assert 2 ** 19 <= d * 2 and d < 2 ** 20, d


def test_generator_deterministic():
    with tempfile.TemporaryDirectory() as t1, tempfile.TemporaryDirectory() as t2:
        gen.generate(t1, [9, 16, 19, 23], toy=True)
        gen.generate(t2, [9, 16, 19, 23], toy=True)
        for j in (9, 16, 19, 20, 23):
            for ext in ("wl", "ref"):
                h1 = hashlib.sha256((Path(t1) / f"job-{j:02d}.{ext}").read_bytes()).digest()
                h2 = hashlib.sha256((Path(t2) / f"job-{j:02d}.{ext}").read_bytes()).digest()
                assert h1 == h2


def test_twin_same_operations():
    with tempfile.TemporaryDirectory() as t:
        gen.generate(t, [19], toy=True)
        w19, w20 = C.read_workload(Path(t) / "job-19.wl"), C.read_workload(Path(t) / "job-20.wl")
        r19, r20 = C.read_pairs(Path(t) / "job-19.ref"), C.read_pairs(Path(t) / "job-20.ref")
        e19 = [(a, b) for o, a, b in zip(w19["op"], w19["a"], w19["b"]) if o == C.ENTER]
        e20 = [(a, b) for o, a, b in zip(w20["op"], w20["a"], w20["b"]) if o == C.ENTER]
        assert e19 == e20                              # same values, same batch order
        assert all(o == C.ENTER for o in w20["op"][:w20["n_in"]])
        v19 = [(r19[0][i], r19[1][i], w19["op"][i]) for i in range(w19["n"]) if w19["op"][i] != C.ENTER]
        v20 = [(r20[0][i], r20[1][i], w20["op"][i]) for i in range(w20["n"]) if w20["op"][i] != C.ENTER]
        assert v19 == v20                              # same operations, same results, same order


def test_bands():
    assert A.time_verdict(1.06, 1.2, 0.05) == "win" and A.time_verdict(1.05, 1.2, 0.05) == "tie"
    assert A.time_verdict(0.5, 0.94, 0.05) == "loss" and A.time_verdict(0.5, 0.95, 0.05) == "tie"
    assert A.fixed_verdict(0.90) == "win" and A.fixed_verdict(1.10) == "loss" and A.fixed_verdict(1.0) == "tie"


def test_bootstrap_paired():
    a = np.array([10.0 + (k % 3) for k in range(12)])
    r, lo, hi = A.boot_ratio(a, a)
    assert r == 1.0 and lo == 1.0 and hi == 1.0      # the same draw for both rows keeps the pairing


def fake_state(times, capped=(), counts=None):
    recs = []
    for (job, row), ts in times.items():
        for k, t in enumerate(ts, 1):
            recs.append(dict(job=job, row=row, kind="time", round=k, label="measured", overcap=False, total=t,
                             entry=t / 4, ops=t / 2, read=t / 4))
    for job, row in capped:
        recs.append(dict(job=job, row=row, kind="time", round=3, label="measured", overcap=True, cap_time=1.2e10))
    st = {"records": recs, "calibration": {}, "counts": counts or {}}
    return A.Data(st)


def test_verdicts_and_cap():
    rng = np.random.default_rng(0)
    base = 100 + rng.normal(0, 1, 12)
    d = fake_state({(9, "ctl"): base, (9, "ctl-null"): base * 1.001, (9, "D1"): base / 2, (9, "D2"): base * 2},
                   counts={"ctl:9": {"counts": ["ctr add core gcd_work 100"], "mem": ["mem end values 1600 1600",
                                                                                     "memN end 100"]},
                           "D1:9": {"counts": ["ctr add core gcd_work 85"], "mem": ["mem end values 8000 8000",
                                                                                  "memN end 100"]},
                           "D2:9": {"counts": ["ctr add core gcd_work 110"], "mem": ["mem end values 1700 1700",
                                                                                   "memN end 100"]}})
    h, nul, _ = A.null_h(d, 9)
    assert h == 0.05
    c1 = A.compare(d, 9, "D1", "ctl", h)
    assert c1["time"]["verdict"] == "win" and abs(c1["time"]["ratio"] - 2) < 1e-9
    assert c1["gcd"]["verdict"] == "win" and c1["mem"]["verdict"] == "loss" and c1["mem"]["flag"]
    c2 = A.compare(d, 9, "D2", "ctl", h)
    assert c2["time"]["verdict"] == "loss" and c2["gcd"]["verdict"] == "loss" and c2["mem"]["verdict"] == "tie"
    hh = A.compare(d, 9, "D1", "D2", h, h2h=True)
    assert hh["time"]["verdict"] == "win" and abs(hh["time"]["ratio"] - 4) < 1e-9
    d = fake_state({(9, "ctl"): base, (9, "ctl-null"): base, (9, "D1"): base[:2], (9, "D2"): base}, capped=[(9, "D1")])
    assert A.compare(d, 9, "D1", "ctl", 0.05)["time"]["verdict"] == "over cap"
    assert A.compare(d, 9, "D1", "D2", 0.05, h2h=True)["time"]["verdict"] == "D1 over cap"


def test_code_hashes_invalidate():
    """Gate and counting results carry code hashes; --resume and --rerun drop and redo those of other code. A row's own
    file invalidates that row only; a shared file (and the shared integer module) invalidates every row."""
    import shutil
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        lib = dict(runner.libconf.LIB)
        files = {}
        for p in list(lib["shared_files"]) + [q for v in lib["own_files"].values() for q in v]:
            files[p] = t / p.parent.name / p.name
            files[p].parent.mkdir(exist_ok=True)
            shutil.copy(p, files[p])
        lib["shared_files"] = [files[p] for p in lib["shared_files"]]
        lib["own_files"] = {k: [files[p] for p in v] for k, v in lib["own_files"].items()}
        lib["int_module"] = files[lib["int_module"]]
        old_lib = runner.libconf.LIB
        runner.libconf.LIB = lib
        try:
            def fresh(state):
                r = runner.Runner(ns(toy=True, jobs="3,9"))
                r.out = t / "out"
                r.out.mkdir(exist_ok=True)
                r.state = json_copy(state)
                r.t0 = None
                return r

            r0 = runner.Runner(ns(toy=True, jobs="3,9"))
            st = {"elapsed_before": 0, "not_constructible": [], "gates": {}, "counts": {}, "records": [],
                  "calibration": {}}
            for impl in ("ctl", "D1", "D2"):
                for job in (3, 9):
                    st["gates"][f"{impl}:{job}"] = r0.stamp_code(impl, {"impl": impl, "job": job})
                    st["counts"][f"{impl}:{job}"] = r0.stamp_code(impl, {"impl": impl, "job": job, "counts": []})
            st["records"] = [{"row": row, "job": 3} for row in ("ctl", "ctl-null", "D1", "D2")]

            r = fresh(st)                                     # unchanged code: nothing stale, no gate re-run
            assert r.drop_stale() == set() and r.gate_pairs() == []

            own = lib["own_files"]["D2"][0]                   # a row's own file: that row only
            own.write_text(own.read_text() + "\n/* fix */\n")
            r = fresh(st)
            assert r.code_int == r0.code_int and r.code["D2"] != r0.code["D2"] and r.code["D1"] == r0.code["D1"]
            assert sorted(r.gate_pairs()) == [("D2", 3), ("D2", 9)]          # --resume redoes D2's gates
            assert r.drop_stale() == {"D2"} and set(r.state["counts"]) == {"ctl:3", "ctl:9", "D1:3", "D1:9"}
            r = fresh(st)                                     # --rerun D1 also re-runs the stale D2
            assert r.plan_rerun("D1") == ["D1", "D2"]
            assert [x["row"] for x in r.state["records"]] == ["ctl", "ctl-null"] and set(r.state["gates"]) == \
                {"ctl:3", "ctl:9"}

            shared = lib["int_module"]                        # the shared integer module: every row, int hash too
            shared.write_text(shared.read_text() + "\n/* fix */\n")
            r = fresh(st)
            assert r.code_int != r0.code_int and all(r.code[i] != r0.code[i] for i in r.code)
            assert len(r.gate_pairs()) == 6 and r.drop_stale() == {"ctl", "D1", "D2"}
            assert r.state["gates"] == {} and r.state["counts"] == {}
            assert all(d["int_changed"] for d in r.state["dropped_stale"])
            r = fresh(st)
            assert r.plan_rerun("D2") == ["ctl", "D1", "D2"] and r.state["records"] == []

            shutil.copy(old_lib["int_module"], shared)        # another shared file (common.c): every row, not int
            st2 = json_copy(st)
            r1 = fresh(st2)
            for kind in ("gates", "counts"):
                for k, v in st2[kind].items():
                    r1.stamp_code(k.split(":")[0], v)
            common_c = next(p for p in lib["shared_files"] if p.name == "common.c")
            common_c.write_text(common_c.read_text() + "\n/* fix */\n")
            r = fresh(st2)
            assert r.code_int == r1.code_int and len(r.gate_pairs()) == 6
            assert r.drop_stale() == {"ctl", "D1", "D2"}
            assert not any(d["int_changed"] for d in r.state["dropped_stale"])
            unstamped = {"gates": {"D1:3": {"impl": "D1", "job": 3}}, "counts": {}}   # results with no hash: stale
            r = fresh(dict(st2, **unstamped))
            assert r.drop_stale() == {"D1"}
        finally:
            runner.libconf.LIB = old_lib


def json_copy(x):
    import json
    return json.loads(json.dumps(x))


if __name__ == "__main__":
    for name, fn in list(globals().items()):
        if name.startswith("test_"):
            fn()
            print("pass", name)
