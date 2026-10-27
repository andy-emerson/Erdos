"""Runs cbench/p2_elem.c (phase two, element-by-element: the format against GMP and FLINT); every result is checked
against Python integers, or against exact decimal strings for dec_out."""
import subprocess
from pathlib import Path

import p1_arith_run as AR

HERE = Path(__file__).resolve().parent
FORCE = False
BIN = HERE.parent / "cbench" / "p2_elem"
SRC = HERE.parent / "cbench" / "p2_elem.c"
LIBS = {"mpz": "GMP mpz_t over a shared denominator", "mpq": "GMP mpq_t per element",
        "fmpz": "FLINT fmpz over a shared denominator", "fmpq": "FLINT fmpq per element"}
OPS = ("add", "mul", "dot", "div1", "div2", "sort", "scan", "dec_in", "dec_out")


def flint_root():
    """The FLINT every phase-two binary links: FLINT_ROOT, else the native rebuild, else the BLAS build."""
    import os
    for r in (os.environ.get("FLINT_ROOT"), "/home/claude/flintblas/inst-native", "/home/claude/flintblas/inst"):
        if r and Path(r, "lib").exists():
            return r


def build():
    deps = [SRC, HERE.parent / "cbench" / "p1_arith.c"]
    if FORCE or not BIN.exists() or BIN.stat().st_mtime < max(d.stat().st_mtime for d in deps):
        root = flint_root()
        subprocess.run(["clang", "-std=c2x", "-O3", "-march=native", "-Wall", "-Wno-invalid-feature-combination",
                        f"-I{root}/include", "-o", str(BIN), str(SRC), f"-L{root}/lib", "-lflint", "-lgmp",
                        f"-Wl,-rpath,{root}/lib"], check=True)


def dec(v):
    s = "-" if v < 0 else ""
    m = abs(v)
    return f"{s}{m // 1000}.{m % 1000:03d}"


class WidthInputs(AR.WidthInputs):
    def __init__(self, W, n, seed=0):
        super().__init__(W, n, seed)
        (Path(self.dir.name) / "dec.txt").write_text("\n".join(dec(x) for x in self.a) + "\n")

    def expected(self, op):
        if op == "dec_in":
            return list(self.a)
        if op == "dec_out":
            return [dec(x) for x in self.a]
        return super().expected(op)

    def run(self, rep, op, reps=5):
        out = Path(self.dir.name) / f"{rep}-{op}.txt"
        r = subprocess.run([str(BIN), rep, op, str(self.W), str(self.n), str(reps), self.dir.name, str(out)],
                           capture_output=True, text=True)
        if r.returncode == 2 and "not defined" in r.stderr:
            return None, None, None
        r.check_returncode()
        med, mn, w = r.stdout.split()
        text = out.read_text().split()
        return float(med), float(mn), (text if op == "dec_out" else [int(x) for x in text])


class ChainInputs(AR.ChainInputs):
    def run(self, rep, K, reps=3):
        out = Path(self.dir.name) / f"{rep}-chain-{K}.txt"
        r = subprocess.run([str(BIN), rep, "chain", "64", str(self.n), str(reps), self.dir.name, str(out), str(K),
                            str(self.den)], capture_output=True, text=True)
        if r.returncode == 3:
            return None, None, None, None
        r.check_returncode()
        med, mn, w = r.stdout.split()
        return float(med), float(mn), int(w), [int(x) for x in out.read_text().split()]
