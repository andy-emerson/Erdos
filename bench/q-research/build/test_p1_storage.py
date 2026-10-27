"""Tests for cbench/p1_storage.c (run: python3 runtests.py test_p1_storage)."""
import p1_storage_run as P

_built = []
ALL = list(P.VARIANTS) + list(P.EXTRA)
CHAIN_POLICIES = list(P.POLICIES) + list(P.EXTRA)
BS = (2, 3, 8, 63, 64, 65, 100, 127, 128, 129, 130, 200, 256, 257, 260, 520, 1030, 1500, 2047, 2048)


def _build():
    if not _built:
        P.build()
        _built.append(1)


def test_inputs_hold_the_edges():
    for B in (2, 65, 2047):
        inp = P.OpInputs(B, 9)
        top = (1 << B) - 1
        assert {0, 1, -1, top, -top} <= set(inp.s)
        assert {0, 1, -1, (1 << (B - 1)) - 1} <= set(inp.a)
        assert {0, (1 << inp.BH) - 1, -((1 << inp.BH) - 1)} <= set(inp.ha)
        assert len(set(inp.s)) < len(inp.s)                     # a tie for the stable sort


def test_ops_exact_against_python_integers():
    _build()
    for B in BS:
        for n in (1, 2, 9, 37):
            inp = P.OpInputs(B, n, seed=n)
            for op in P.OPS:
                for var in ALL:
                    inp.run(var, op, reps=1)                      # raises on any wrong value or stride
    for B in (65, 130, 2047):
        inp = P.OpInputs(B, 700, seed=5)
        for op in P.OPS:
            for var in ALL:
                inp.run(var, op, reps=2)


def test_chain_exact_against_python_integers():
    _build()
    for n in (1, 3, 9, 50):
        ch = P.ChainInputs(n, seed=n)
        for K in (0, 1, 2, 5, 20, 61, 140):
            for pol in CHAIN_POLICIES:
                for slack in ((125, 150, 300) if pol in ("slack", "lslack") else (150,)):
                    r = ch.run(pol, K, reps=1, slack=slack)
                    assert r["stride"] == P.stride(pol, r["bits"], r["stride"], slack)


def test_chain_restore_counts_differ_as_designed():
    _build()
    ch = P.ChainInputs(20)
    r = {p: ch.run(p, 120, reps=1) for p in P.POLICIES}
    assert r["pow2"]["restores"] < r["limb"]["restores"] < r["byte"]["restores"]
    assert r["slack"]["restores"] < r["byte"]["restores"] and r["lslack"]["restores"] < r["limb"]["restores"]
    assert len({x["bits"] for x in r.values()}) == 1            # the same bound for every policy


def test_corruption_is_detected():
    _build()
    for B in (2, 130):
        for n in (1, 9):
            inp = P.OpInputs(B, n)
            for var in ALL:
                for op in P.OPS:
                    if op == "sort" and n == 1:
                        continue
                    inp.run(var, op, reps=1)
                    try:
                        inp.run(var, op, reps=1, corrupt=True)
                    except AssertionError:
                        continue
                    raise AssertionError(f"{var} {op} B={B} n={n}: a corrupted expectation passed")
    ch = P.ChainInputs(5)
    for pol in CHAIN_POLICIES:
        try:
            ch.run(pol, 10, reps=1, corrupt=True)
        except AssertionError:
            continue
        raise AssertionError(f"chain {pol}: a corrupted expectation passed")


def test_memory_table():
    for B in range(1, 2049):
        assert P.stride("byte", B) <= P.stride("limb", B) <= P.stride("pow2", B)
        assert 8 * P.stride("byte", B) - B < 8 and 8 * P.stride("limb", B) - B < 64
    m = P.memory_table()
    assert m["byte"]["worst_waste_bits"] == 7 and m["limb"]["worst_waste_bits"] == 63


def test_growth_rates_exact():
    _build()
    for name, bmax in P.GROWTH.items():
        ch = P.ChainInputs(7, seed=3, bmax=bmax)
        for K in (1, 9, 64, 128):
            for pol in CHAIN_POLICIES:
                ch.run(pol, K, reps=1)


def test_reverse_order_exact():
    """Every op, variant and chain policy, processing elements last to first: a write that spills into the next
    element's bytes is then not repaired by that element's own later write, and the C checks all padding after."""
    _build()
    for B in (2, 8, 63, 65, 100, 129, 130, 257, 1030, 2047):
        for n in (1, 2, 11):
            inp = P.OpInputs(B, n, seed=B)
            for op in P.OPS:
                for var in ALL:
                    inp.run(var, op, reps=1, binary=P.BIN_REV)
    ch = P.ChainInputs(9)
    for K in (1, 7, 40):
        for pol in CHAIN_POLICIES:
            ch.run(pol, K, reps=1, binary=P.BIN_REV)


def test_neighbour_spill_is_detected():
    """The planted spill (one byte of the next element flipped after each write) is caught for every writing op,
    variant and policy; sort and scan only read, and their inputs' padding is checked after the run."""
    _build()
    for B in (8, 65, 130, 1030):
        inp = P.OpInputs(B, 6)
        for var in ALL:
            for op in P.OPS:
                if op in ("sort", "scan"):
                    inp.run(var, op, reps=1, binary=P.BIN_BUG)          # no writes: must still pass
                    continue
                try:
                    inp.run(var, op, reps=1, binary=P.BIN_BUG)
                except AssertionError:
                    continue
                raise AssertionError(f"{var} {op} B={B}: a spill into the neighbour passed")
    ch = P.ChainInputs(6)
    for pol in CHAIN_POLICIES:
        try:
            ch.run(pol, 5, reps=1, binary=P.BIN_BUG)
        except AssertionError:
            continue
        raise AssertionError(f"chain {pol}: a spill into the neighbour passed")


def test_inner_count_keeps_results_exact():
    _build()
    inp = P.OpInputs(130, 50)
    for var in ALL:
        for op in P.OPS:
            r = inp.run(var, op, reps=2, min_sample=0.0005, fast_check=True)
            assert r["inner"] >= 1
    ch = P.ChainInputs(30)
    for pol in CHAIN_POLICIES:
        r = ch.run(pol, 20, reps=2, min_sample=0.0005, fast_check=True)
        assert r["inner"] > 1 and 0 <= r["restore_time"] <= r["median"]
