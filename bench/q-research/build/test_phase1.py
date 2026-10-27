"""Correctness tests for phase one (run: python3 runtests.py test_phase1). The benchmark also checks every timed
result; these tests add small and edge sizes and make sure the checks can fail."""
import random

import p1_arith_run as AR
import p1_layout_run as LR
import p1_widen_run as WR


def test_arith_every_rep_op_width_small_sizes():
    AR.build()
    for n in (1, 2, 9, 257):
        for W in AR.WIDTHS:
            I = AR.WidthInputs(W, n, seed=n)
            for op in AR.OPS:
                want = I.expected(op)
                for rep in AR.REPS:
                    if op == "widen" and rep == "mpz":
                        continue
                    assert I.run(rep, op, 1)[2] == want, (n, W, op, rep)


def test_arith_chain_and_cap():
    C = AR.ChainInputs(129)                                   # includes 0, -2^62, b = 0, c = 0
    for K in (1, 4, 17, 40, 60):
        want = C.expected(K)
        for rep in AR.REPS:
            assert C.run(rep, K, 1)[3] == want, (K, rep)
    for rep in AR.REPS:
        assert C.run(rep, 400, 1)[3] is None, rep


def test_arith_checks_can_fail():
    """A wrong expectation must be detected: compare against a perturbed reference."""
    I = AR.WidthInputs(256, 50, seed=3)
    for op in ("add", "mul", "div2", "sort"):
        want = I.expected(op)
        bad = list(want)
        bad[7] = bad[7] + 1 if op != "sort" else bad[8]
        for rep in AR.REPS:
            assert I.run(rep, op, 1)[2] != bad, (op, rep)


def test_layouts_every_op_storage():
    LR.build()
    for name, L in LR.LAYOUTS.items():
        for op in ("dec", "enc", "mul", "promo", "move"):
            if op == "promo" and L["word"] == 128:
                continue
            if op == "move" and L["order"] == "bot":
                continue
            I = LR.Inputs(name, 600, op, seed=5)
            sts = LR.storages(name) if op in ("dec", "enc", "mul") else (("soa",) if name.startswith("t") else ("flat",))
            for st in sts:
                med, mn, fails, bpv, lines = I.run(st, 1)
                assert I.check(st, lines, fails), (name, op, st)


def test_layout_checks_can_fail():
    I = LR.Inputs("puni_top_64", 300, "enc", seed=1)
    med, mn, fails, bpv, lines = I.run("flat", 1)
    h, v = lines[5].split()
    lines[5] = f"{h} {int(v, 16) ^ 1:032x}"
    assert not I.check("flat", lines, fails)


def test_layout_reference_round_trip():
    """The reference coder round-trips every value that fits, in every layout; values that do not fit are refused."""
    rng = random.Random(2)
    for L in list(LR.LAYOUTS.values()) + list(LR.QUAT.values()):
        for _ in range(300):
            s, a, p, q = LR.rand_value(rng, L["D"], rng.randint(1, L["D"]), dim=2 ** (L["U"] - 1))
            enc = LR.G.ref_encode(L, s, a, p, q)
            assert enc is not None
            assert LR.G.ref_decode(L, *enc) == ((s, a, p, q) if p else (1, 0, 0, 1))
        assert LR.G.ref_encode(L, 1, 0, 2 ** L["D"], 1) is None


def test_widen_rules():
    WR.build()
    for share in WR.SHARES:
        for n in (1, 7, 1000):
            I = WR.Inputs(n, share, seed=n)
            for op in ("sq", "sumsq"):
                want = I.expected(op)
                for rule in WR.RULES:
                    assert I.run(rule, op, 1)[3] == want, (share, n, op, rule)
