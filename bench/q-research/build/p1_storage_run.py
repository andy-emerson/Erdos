"""Runs cbench/p1_storage.c (phase one, storage granularity of the vector form) and checks every result against Python
integers: values, permutations, scan maxima, strides, and for the chain the bound sequence, re-store count and the
bytes allocated and copied (replayed here from the same rules).

usage: python3 p1_storage_run.py [--bits 65 100 ...] [--n 100000] [--reps 5] [--ops ...] [--K 16 64 128]
                                 [--slack 150] [--json out.json] [--memory]
"""
import argparse
import json
import os
import random
import subprocess
import tempfile
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
BIN = HERE.parent / "cbench" / "p1_storage"
BIN_REV = HERE.parent / "cbench" / "p1_storage_rev"          # elements processed in reverse order (tests)
BIN_BUG = HERE.parent / "cbench" / "p1_storage_bugrev"       # reverse order plus a planted spill into the neighbour
SRC = HERE.parent / "cbench" / "p1_storage.c"
VARIANTS = {"pow2": "next power-of-two width (8 * 2^k bytes), aligned",
            "limb": "ceil(B/64) limbs, aligned",
            "byte": "ceil(B/8) bytes, packed, computed in place (unaligned limbs + masked top limb)"}
EXTRA = {"bytecopy": "ceil(B/8) bytes, packed, through a copy-in/copy-out limb buffer"}
POLICIES = dict(VARIANTS, slack="packed bytes in place, re-stored at ceil(SLACK x needed bytes) when outgrown",
                lslack="limbs, re-stored at ceil(SLACK x needed limbs) when outgrown")
GROWTH = {"slow": 10**2, "mid": 10**4, "fast": 10**8}         # |b| below this: about 7, 14, 27 bits per step
OPS = ("add", "mul", "sort", "scan", "widen")
BITS = (65, 100, 128, 130, 200, 256, 260, 512, 520, 1024, 1030, 1500, 2047, 2048)
DEN = 1000


def build():
    for b, flags in ((BIN, []), (BIN_REV, ["-DP1S_REVERSE=1"]), (BIN_BUG, ["-DP1S_REVERSE=1", "-DP1S_BUG=1"])):
        if not b.exists() or b.stat().st_mtime < SRC.stat().st_mtime:
            subprocess.run(["clang", "-std=c2x", "-O3", "-march=native", "-Wall", "-Wno-invalid-feature-combination",
                            *flags, "-o", str(b), str(SRC), "-lgmp"], check=True)


def _exec(args, binary, min_sample):
    env = dict(os.environ)
    env.pop("P1S_MIN", None)
    if min_sample:
        env["P1S_MIN"] = str(min_sample)
    r = subprocess.run(["nice", str(binary)] + args, capture_output=True, text=True, env=env)
    if r.returncode == 4:
        raise AssertionError(f"p1_storage {' '.join(args[:3])}: padding or neighbour bytes corrupted")
    r.check_returncode()
    return r.stdout.split()


def limbs(bits):
    return max(1, -(-bits // 64))


def stride(var, bits, cur=0, slack=150):
    """Stored magnitude bytes per value for that many bits (cur: the current stride, for the chain); as in the C."""
    if var == "pow2":
        q = 1
        while q < limbs(bits):
            q *= 2
        s = 8 * q
    elif var == "limb":
        s = 8 * limbs(bits)
    elif var == "lslack":
        need = limbs(bits)
        if 8 * need <= cur:
            return cur
        s = 8 * max(need, -(-need * slack // 100))
    else:
        need = max(1, -(-bits // 8))
        if var == "slack":
            if need <= cur:
                return cur
            s = max(need, -(-need * slack // 100))
        else:
            s = need
    return max(s, cur)


def _write(path, vals, bits):
    nb = 8 * limbs(bits)
    for v in vals:
        assert abs(v).bit_length() <= bits
    Path(str(path) + ".mag").write_bytes(b"".join(abs(v).to_bytes(nb, "little") for v in vals))
    Path(str(path) + ".sgn").write_bytes(bytes(1 if v < 0 else 0 for v in vals))


def _read(path):
    return Path(path).read_text().split()


def _hex(tokens):
    return [int(x, 16) for x in tokens]


class OpInputs:
    """Vectors at B bits: a, b with B - 1 bits (their sum needs B), s with B bits; ha, hb with floor(B/2) bits (their
    product needs B). Each holds 0, +-1 and the largest magnitude +-(2^bits - 1); s has a tie (stable sort)."""

    def __init__(self, B, n, seed=0):
        assert B >= 2
        rng = random.Random(seed * 7919 + B)
        BH = B // 2

        def vec(bits, k):
            v = [rng.choice((-1, 1)) * rng.getrandbits(bits) for _ in range(n)]
            top, used = (1 << bits) - 1, set()
            for i, x in enumerate((0, 1, -1, top, -top)):
                if i < n:
                    v[(5 * k + i) % n] = x                   # distinct positions, a different place per vector
                    used.add((5 * k + i) % n)
            return v, [i for i in range(n) if i not in used]
        (self.a, _), (self.b, _), (self.s, free) = vec(B - 1, 0), vec(B - 1, 1), vec(B, 2)
        (self.ha, _), (self.hb, _) = vec(BH, 3), vec(BH, 4)
        if len(free) >= 2:
            self.s[free[-1]] = self.s[free[0]]           # a tie between plain values, for the stable sort
        self.B, self.BH, self.n = B, BH, n
        self.dir = tempfile.TemporaryDirectory()
        for name, bits in (("a", B), ("b", B), ("s", B), ("ha", BH), ("hb", BH)):
            _write(Path(self.dir.name) / name, getattr(self, name), bits)

    def expected(self, op):
        if op == "add":
            return [x + y for x, y in zip(self.a, self.b)]
        if op == "mul":
            return [x * y for x, y in zip(self.ha, self.hb)]
        if op == "sort":
            return sorted(range(self.n), key=lambda i: self.s[i])
        if op == "scan":
            return [max(abs(x).bit_length() for x in self.s)]
        if op == "widen":
            return list(self.ha)

    def expected_text(self, op):
        """The exact OUTFILE text the C must write (fast check for repeated runs)."""
        key = "_txt_" + op
        if not hasattr(self, key):
            e = self.expected(op)
            fmt = (lambda v: str(v)) if op in ("sort", "scan") else (lambda v: format(v, "x"))
            setattr(self, key, "".join(fmt(v) + "\n" for v in e))
        return getattr(self, key)

    def run(self, var, op, reps=5, corrupt=False, binary=BIN, min_sample=0, fast_check=False):
        """dict(median, min, stride, stride_in, inner); AssertionError if any result, stride or padding is wrong."""
        out = Path(self.dir.name) / f"{var}-{op}.txt"
        med, mn, so, si, inner = _exec([var, op, str(self.B), str(self.n), str(reps), self.dir.name, str(out)],
                                       binary, min_sample)
        sin_bits = self.BH if op in ("mul", "widen") else self.B
        assert int(so) == stride(var, self.B) and int(si) == stride(var, sin_bits), (var, op, self.B, so, si)
        res = dict(median=float(med), min=float(mn), stride=int(so), stride_in=int(si), inner=int(inner))
        if fast_check and not corrupt:
            assert out.read_text() == self.expected_text(op), f"p1_storage {var} {op} B={self.B} n={self.n}: wrong result"
            return res
        tok = _read(out)
        got = [int(x) for x in tok] if op in ("sort", "scan") else _hex(tok)
        exp = self.expected(op)
        if corrupt:
            exp = list(exp)
            if op == "sort" and self.n > 1:
                exp[0], exp[-1] = exp[-1], exp[0]
            else:
                exp[-1] += 1
        assert got == exp, f"p1_storage {var} {op} B={self.B} n={self.n}: wrong result"
        return res


class ChainInputs:
    """x <- x b + c 1000^(k+1): x0 3-place decimals below 10^4 (|x0| < 10^7), |b| < bmax (10^4 by default; GROWTH
    sets the growth rate), |c| < 10^6; with extremes 0 and +-(bound - 1) in each."""

    def __init__(self, n, den=DEN, seed=101, extremes=True, bmax=10**4):
        rng = np.random.default_rng(seed)
        self.x0 = rng.integers(-10**7 + 1, 10**7, size=n)
        self.cb = rng.integers(-bmax + 1, bmax, size=n)
        self.cc = rng.integers(-10**6 + 1, 10**6, size=n)
        if extremes:
            for arr, lim, off in ((self.x0, 10**7 - 1, 0), (self.cb, bmax - 1, 3), (self.cc, 10**6 - 1, 6)):
                for j, v in enumerate((0, lim, -lim)):
                    if off + j < n:
                        arr[off + j] = v
        self.n, self.den = n, den
        self.dir = tempfile.TemporaryDirectory()
        for name in ("x0", "cb", "cc"):
            getattr(self, name).astype(np.int64).tofile(Path(self.dir.name) / f"{name}.bin")
        self._cache = {0: [int(v) for v in self.x0]}

    def expected(self, K):
        k0 = max(k for k in self._cache if isinstance(k, int) and k <= K)
        x = self._cache[k0]
        cs = self.den ** (k0 + 1)
        b, c = [int(v) for v in self.cb], [int(v) for v in self.cc]
        for _ in range(k0, K):
            x = [xi * bi + ci * cs for xi, bi, ci in zip(x, b, c)]
            cs *= self.den
        self._cache[K] = x
        return x

    def replay(self, policy, K, slack=150):
        """The bound sequence and the storage accounting the C must report, from the same rules."""
        bx = max(1, max(abs(int(v)).bit_length() for v in self.x0))
        bb = max(abs(int(v)).bit_length() for v in self.cb)
        bc = max(abs(int(v)).bit_length() for v in self.cc)
        cur = stride(policy, bx, 0, slack)
        acc = dict(restores=0, alloc=self.n * cur, copied=0, peak=self.n * cur)
        pw = 1
        for _ in range(K):
            pw *= self.den
            bx = max(bx + bb, bc + pw.bit_length()) + 1
            ns = stride(policy, bx, cur, slack)
            if ns != cur:
                acc["restores"] += 1
                acc["alloc"] += self.n * ns
                acc["copied"] += self.n * cur
                acc["peak"] = max(acc["peak"], self.n * (cur + ns))
                cur = ns
        return dict(acc, bits=bx, stride=cur)

    def expected_text(self, K):
        key = ("txt", K)
        if key not in self._cache:
            self._cache[key] = "".join(format(v, "x") + "\n" for v in self.expected(K))
        return self._cache[key]

    def run(self, policy, K, reps=3, slack=150, corrupt=False, binary=BIN, min_sample=0, fast_check=False):
        out = Path(self.dir.name) / f"{policy}-chain-{K}.txt"
        v = _exec([policy, "chain", "0", str(self.n), str(reps), self.dir.name, str(out), str(K), str(self.den),
                   str(slack)], binary, min_sample)
        res = dict(median=float(v[0]), min=float(v[1]), bits=int(v[2]), stride=int(v[3]), restores=int(v[4]),
                   alloc=int(v[5]), copied=int(v[6]), peak=int(v[7]), restore_time=float(v[8]), inner=int(v[9]))
        if fast_check and not corrupt:
            assert out.read_text() == self.expected_text(K), f"p1_storage chain {policy} K={K} n={self.n}: wrong result"
        else:
            exp = self.expected(K)
            if corrupt:
                exp = exp[:-1] + [exp[-1] + 1]
            assert _hex(_read(out)) == exp, f"p1_storage chain {policy} K={K} n={self.n}: wrong result"
        rp = self.replay(policy, K, slack)
        for key in ("bits", "stride", "restores", "alloc", "copied", "peak"):
            assert res[key] == rp[key], (policy, K, key, res[key], rp[key])
        assert max(abs(x).bit_length() for x in self.expected(K)) <= res["bits"] <= 8 * res["stride"]
        return res


def memory_table(maxB=2048):
    """Magnitude bytes per value over B = 1 .. maxB (each variant also has the one sign byte): average bytes, average
    and worst waste (stored bits - B, as a share of the stored bits) and where the worst falls."""
    out = {}
    for var in VARIANTS:
        s = [stride(var, B) for B in range(1, maxB + 1)]
        waste = [(8 * x - B) / (8 * x) for B, x in zip(range(1, maxB + 1), s)]
        wb = [8 * x - B for B, x in zip(range(1, maxB + 1), s)]
        out[var] = dict(avg_bytes=sum(s) / len(s), avg_waste=sum(waste) / len(waste),
                        avg_waste_bits=sum(wb) / len(wb), worst_waste=max(waste),
                        worst_at=1 + waste.index(max(waste)), worst_waste_bits=max(wb),
                        worst_bits_at=1 + wb.index(max(wb)),
                        avg_waste_over_64=sum(w for B, w in zip(range(1, maxB + 1), waste) if B > 64) / (maxB - 64))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bits", type=int, nargs="+", default=list(BITS))
    ap.add_argument("--n", type=int, default=100000)
    ap.add_argument("--reps", type=int, default=5)
    ap.add_argument("--ops", nargs="+", default=list(OPS) + ["chain"])
    ap.add_argument("--variants", nargs="+", default=list(VARIANTS), help="also: bytecopy")
    ap.add_argument("--K", type=int, nargs="+", default=[16, 64, 128])
    ap.add_argument("--slack", type=int, default=150)
    ap.add_argument("--json")
    ap.add_argument("--memory", action="store_true", help="print the analytic memory table")
    a = ap.parse_args()
    build()
    res = {"n": a.n, "reps": a.reps, "slack": a.slack, "ops": [], "chain": [], "memory": memory_table()}
    if a.memory:
        for var, m in res["memory"].items():
            print(f"{var:5s} avg {m['avg_bytes']:7.2f} B/value  avg waste {100 * m['avg_waste']:5.1f}% "
                  f"(B>64: {100 * m['avg_waste_over_64']:5.1f}%, {m['avg_waste_bits']:6.1f} bits)  "
                  f"worst {100 * m['worst_waste']:5.1f}% at B={m['worst_at']}, {m['worst_waste_bits']} bits at B={m['worst_bits_at']}")
    for B in a.bits:
        ops = [o for o in a.ops if o != "chain"]
        if not ops:
            continue
        inp = OpInputs(B, a.n)
        for op in ops:
            row = {"B": B, "op": op}
            for var in a.variants:
                r = inp.run(var, op, a.reps)
                row[var] = r
            res["ops"].append(row)
            print(f"B={B:5d} {op:6s} " + "  ".join(f"{v}:{row[v]['median'] * 1e3:8.3f}ms/{row[v]['stride']:4d}B"
                                                    for v in a.variants), flush=True)
        del inp
    if "chain" in a.ops:
        ch = ChainInputs(a.n)
        for K in a.K:
            for pol in list(a.variants) + (["slack"] if "byte" in a.variants else []):
                r = ch.run(pol, K, a.reps, a.slack)
                res["chain"].append(dict(r, K=K, policy=pol))
                print(f"chain K={K:4d} {pol:5s} {r['median'] * 1e3:9.3f}ms bits={r['bits']} stride={r['stride']}B "
                      f"restores={r['restores']} alloc={r['alloc'] / 1e6:.1f}MB copied={r['copied'] / 1e6:.1f}MB "
                      f"peak={r['peak'] / 1e6:.1f}MB", flush=True)
    if a.json:
        Path(a.json).write_text(json.dumps(res, indent=1))


if __name__ == "__main__":
    main()
