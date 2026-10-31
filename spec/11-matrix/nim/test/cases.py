"""The untimed tests of spec v0.4 §17.12 that need crafted matrices (the gcd's are test/gcdtest.nim), as workloads in
the driver's format, for check.py to run through both models in every build against the simulator:

    python3 cases.py OUTDIR        then    python3 check.py OUTDIR

Each case asserts, on the simulator's prediction, that it tests what it names (a full table, a list width, a pair of
handle widths, a mark of the stated kind at the stated cell), so that check.py's equality with the simulator covers
it. The cases:
  fill-1, fill-2        a build at 100% fill, W = T_fix, in every matrix (n = 1, 2)
  width-8/16/32/64      (l) narrowed to 8, 16, 32 and 64 bits (inputs and result)
  fields-8-8 ...        the fields at every pair of build and packed handle widths: (8, 8), (16, 8), (16, 16),
                        (32, 8), (32, 16), (32, 32)
  ovf-add, ovf-add-min  an overflow in addition (2^63; and -2^63, which overflows as in 08)
  ovf-prod-first/later  an overflow in a product, as the first term and as a later term
  ovf-sum-first/later   an overflow in a sum of products, at the first addition and at a later one
  ovf-num-h             an overflow in addition's case num with h != 1, at a (t/h): both divisions are made first,
                        as tc3.c makes them (4 exact divisions and the multiplication counted)
  ovf-den               an overflow in a denominator (case num with g = 1, b d)
  mark-pos, mark-sop    a marked operand in a later step, on the left and on the right, and in the same term as an
                        overflow it would have caused
  mul-n1                n = 1 multiplication (no addition)
"""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[1] / "test"))
import m2_common as C  # noqa: E402
import sim  # noqa: E402

P62, P40, P31 = 2 ** 62, 2 ** 40, 2 ** 31


def ones(n, v=(1, 2)):
    return [v] * (n * n)


def distinct(n, start):
    """n^2 cells naming 2 n^2 distinct whole numbers: (k + 1)/k for consecutive k from start"""
    return [(start + 2 * i + 1, start + 2 * i) for i in range(n * n)]


def cases():
    out = {}
    out["fill-1"] = ("add", 1, [[(2, 3)], [(5, 7)]])
    out["fill-2"] = ("add", 2, [[(2, 3), (5, 7), (11, 13), (17, 19)], [(23, 29), (31, 37), (41, 43), (47, 53)]])
    for w, base in ((8, 3), (16, 1000), (32, 1 << 20), (64, 1 << 40)):
        out[f"width-{w}"] = ("add", 2, [[(base + 1, 1), (base, 1), (1, 2), (base, 3)],
                                        [(base, 1), (1, 1), (3, 2), (1, 3)]])
    out["fields-8-8"] = ("add", 2, [ones(2), ones(2, (1, 3))])
    out["fields-16-8"] = ("add", 16, [ones(16), ones(16, (1, 3))])
    out["fields-16-16"] = ("add", 16, [distinct(16, 2), distinct(16, 1000)])
    out["fields-32-8"] = ("add", 129, [ones(129), ones(129, (1, 3))])
    out["fields-32-16"] = ("add", 129, [[(i % 500 + 2, 1) for i in range(129 * 129)], ones(129, (1, 3))])
    out["fields-32-32"] = ("add", 129, [distinct(129, 2), distinct(129, 100000)])
    a = [(1, 2), (3, 4), (5, 6), (7, 8)]
    out["ovf-add"] = ("add", 2, [[(P62, 1)] + a[1:], [(P62, 1)] + a[1:]])
    out["ovf-add-min"] = ("add", 2, [[(-P62, 1)] + a[1:], [(-P62, 1)] + a[1:]])
    big = 2 ** 61 + 5                               # coprime to 6 and 10; 4 (2^61 + 5) > 2^63
    out["ovf-num-h"] = ("add", 2, [[(big, 6)] + a[1:], [(big, 10)] + a[1:]])
    out["ovf-den"] = ("add", 2, [[(1, P31 * 2 - 1)] + a[1:], [(1, P31 * 2 + 1)] + a[1:]])
    b = [(1, 2), (1, 3), (1, 5), (2, 3), (3, 5), (1, 7), (5, 7), (2, 9), (4, 9)]
    A = list(b); B = list(b)
    A[0], B[0] = (P40, 1), (P40, 1)                 # C[0][0]: the first product overflows
    out["ovf-prod-first"] = ("mul", 3, [A, B])
    A = list(b); B = list(b)
    A[1], B[3] = (P40, 1), (P40, 1)                 # C[0][0]: the second product (t = 1) overflows
    out["ovf-prod-later"] = ("mul", 3, [A, B])
    A = list(b); B = list(b)
    A[0], B[0], A[1], B[3] = (P31, 1), (P31, 1), (P31, 1), (P31, 1)   # 2^62 + 2^62 at t = 1
    out["ovf-sum-first"] = ("mul", 3, [A, B])
    A = list(b); B = list(b)
    A[0], B[0], A[1], B[3], A[2], B[6] = (P31, 1), (P31 // 2, 1), (P31, 1), (P31 // 2, 1), (P31, 1), (P31, 1)
    out["ovf-sum-later"] = ("mul", 3, [A, B])        # 2^61 + 2^61 fits at t = 1; + 2^62 overflows at t = 2
    # pos: S = A + B, U = C + D, R = S U (n = 3). S[0] and U[0], U[1] overflow (new); S[6] = 2^40 + 1 is not marked.
    A, Bm, Cm, D = list(b), list(b), list(b), list(b)
    A[0], Bm[0] = (P62, 1), (P62, 1)                # S[0][0] marked: R[0][j]'s first term, left
    Cm[1], D[1] = (P62, 1), (P62, 1)                # U[0][1] marked: R[i][1]'s first term, right
    Cm[0], D[0] = (P62, 1), (P62, 1)                # U[0][0] marked: R[2][0]'s first term, right, beside S[2][0]
    A[6], Bm[6] = (P40, 1), (1, 1)                  # S[2][0] = 2^40 + 1, whose product would overflow
    out["mark-pos"] = ("pos", 3, [A, Bm, Cm, D])
    # sop: P = A B, Q = C D, R = P + Q (n = 3). P[0][0] and Q[1][1] overflow; R[0][0] left, R[1][1] right.
    A, Bm, Cm, D = list(b), list(b), list(b), list(b)
    A[0], Bm[0] = (P40, 1), (P40, 1)
    Cm[3], D[1] = (P40, 1), (P40, 1)                # Q[1][1]'s first term C[1][0] D[0][1]
    out["mark-sop"] = ("sop", 3, [A, Bm, Cm, D])
    out["mul-n1"] = ("mul", 1, [[(3, 4)], [(5, 6)]])
    return out


def expect(name, op, n, p):
    """the simulator's prediction tests what the case names"""
    S, K, V = p["stores"], p["counts"], p["values"]
    mats = list(sim.INPUTS[op]) + [s[3] for s in sim.STEPS[op]]
    if name.startswith("fill"):
        for x in mats:
            assert S[x]["W"] == S[x]["T"], (name, x, S[x]["W"], S[x]["T"])
    elif name.startswith("width"):
        w = int(name.split("-")[1])
        assert S["A"]["w"] == w and S["C"]["w"] == w, (name, S["A"]["w"], S["C"]["w"])
    elif name.startswith("fields"):
        whb, whp = (int(v) for v in name.split("-")[1:])
        assert (S["A"]["whb"], S["A"]["whp"]) == (whb, whp), (name, S["A"]["whb"], S["A"]["whp"])
    elif name == "ovf-num-h":
        assert V["C"][0] == sim.MARK and K["C"].marks[:2] == [1, 0], (name, K["C"].marks)
        r = p["records"][0]
        assert sim.CASE_NAMES[r[0]] == "add_num" and r[1:] == (2, 8, 4, 1, 1), r   # gcd(6, 10), gcd(8, 2): work 4 + 4
    elif name in ("ovf-add", "ovf-add-min", "ovf-den"):
        assert V["C"][0] == sim.MARK and K["C"].marks[:2] == [1, 0], (name, K["C"].marks)
    elif name.startswith("ovf-"):
        assert V["C"][0] == sim.MARK and K["C"].marks[:2] == [1, 0], (name, K["C"].marks)
        stopped = {"ovf-prod-first": 0, "ovf-prod-later": 1, "ovf-sum-first": 1, "ovf-sum-later": 2}[name]
        # C[0][0] stops at term `stopped`: its tests are 2 per term reached
        assert K["C"].marks[2] == 2 * (n ** 3) - 2 * (n - 1 - stopped), (name, K["C"].marks)
    elif name == "mark-pos":
        assert K["S"].marks[:2] == [1, 0] and K["U"].marks[:2] == [2, 0], (K["S"].marks, K["U"].marks)
        R = V["R"]
        assert all(R[j] == sim.MARK for j in range(3)) and all(R[3 * i + 1] == sim.MARK for i in range(3))
        assert R[6] == sim.MARK and K["R"].marks[0] == 0, K["R"].marks
    elif name == "mark-sop":
        assert K["P"].marks[:2] == [1, 0] and K["Q"].marks[:2] == [1, 0]
        assert V["R"][0] == sim.MARK and V["R"][4] == sim.MARK and K["R"].marks[:2] == [0, 2], K["R"].marks
    elif name == "mul-n1":
        assert len(p["records"]) == 1 and sim.CASE_NAMES[p["records"][0][0]].startswith("mul")


def main():
    out = Path(sys.argv[1])
    out.mkdir(parents=True, exist_ok=True)
    for i, (name, (op, n, mats)) in enumerate(cases().items(), 1):
        C.write_workload(out / f"job-{i:02d}.wl", op, n, i, mats)
        _, _, _, inputs = C.read_workload(out / f"job-{i:02d}.wl")
        expect(name, op, n, sim.job(op, n, inputs))
        print(f"job-{i:02d} {name}: {op} n={n}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
