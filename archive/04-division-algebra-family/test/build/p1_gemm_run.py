"""Runs cbench/p1_gemm.c: three variants of an exact wide-entry matrix product, compared only with each other.

mmg  multimodular OpenBLAS dgemm (reduction, products, CRT all dgemms on 16-bit digits), as in gemm.c
mmgc mmg in bounded memory (primes in chunks, entries in blocks, product residues as uint32)
mmi  the same three phases on an AVX-512 IFMA kernel of our own, primes below 2^50, 52-bit digits
"""
import os
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
CB = HERE.parent / "cbench"
# OpenBLAS 0.3.34 from the scipy-openblas32 wheel (one thread), as in gemm_run.py; the system FLINT only for the
# fmpz inputs and the correctness reference
BLASDIR = "/home/claude/flintblas/shim/lib"
BLAS = BLASDIR + "/libscipy_openblas.so"
BIN = CB / "p1_gemm"
METHODS = ("mmg", "mmgc", "mmi")


def build():
    subprocess.run(["gcc", "-O3", "-march=native", "-Wall", "-o", str(BIN), str(CB / "p1_gemm.c"), "-lflint", "-lgmp",
                    "-lm", BLAS, f"-Wl,-rpath,{BLASDIR}"], check=True)


def run(method, n, bits, reps=3, edge=None, corrupt=False, outfile=None):
    """dict(median, min, reduce, product, crt, primes, peak_mb); AssertionError if the product fails its check."""
    env = dict(os.environ)
    env.pop("GEMM_EDGE", None)
    env.pop("GEMM_CORRUPT", None)
    if edge:
        env["GEMM_EDGE"] = str(edge)
    if corrupt:
        env["GEMM_CORRUPT"] = "1"
    args = ["nice", str(BIN), method, str(n), str(bits), str(reps)] + ([str(outfile)] if outfile else [])
    r = subprocess.run(args, capture_output=True, text=True, env=env)
    if r.returncode == 4:
        raise AssertionError(f"p1_gemm {method} {n} {bits}: result fails the check")
    r.check_returncode()
    v = r.stdout.split()
    return dict(median=float(v[0]), min=float(v[1]), reduce=float(v[2]), product=float(v[3]), crt=float(v[4]),
                primes=int(v[5]), peak_mb=float(v[6]), shared_mb=float(v[7]))
