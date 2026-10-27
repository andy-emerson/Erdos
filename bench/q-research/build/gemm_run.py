"""Runs cbench/gemm.c (exact wide-entry matrix products) and cbench/decode.c (header decoding cost)."""
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
CB = HERE.parent / "cbench"
# OpenBLAS 0.3.34 from the scipy-openblas32 wheel, the same library under every method; FLINT 3.0.1 configured
# with BLAS (through a shim mapping cblas_dgemm to the wheel's prefixed symbols) at FLINTB; see build_flint_blas.sh
FB = Path(__import__("os").environ.get("FLINT_BLAS_ROOT", "/home/claude/flintblas"))
BLASDIR = str(FB / "shim" / "lib")
BLAS = BLASDIR + "/libscipy_openblas.so"
FLINTB = FB / "inst"
NAMES = {"fmpz-noblas": "FLINT fmpz_mat_mul, FLINT built without BLAS (the system's)",
         "fmpz": "FLINT fmpz_mat_mul, FLINT built with BLAS",
         "blas": "FLINT fmpz_mat_mul_blas (FLINT's multimodular BLAS method)", "fmpq": "FLINT fmpq_mat_mul (reduced rationals)",
         "mm": "multimodular OpenBLAS dgemm, FLINT reduction and CRT",
         "mmg": "multimodular OpenBLAS dgemm, reduction and CRT as dgemms (uN limb layout)",
         "mmga": "the same, everything timed: setup, fmpz to limbs and back, freeing"}


def build():
    subprocess.run(["gcc", "-O3", "-march=native", "-Wall", "-o", str(CB / "gemm"), str(CB / "gemm.c"), "-lflint",
                    "-lgmp", "-lm", BLAS, f"-Wl,-rpath,{BLASDIR}"], check=True)
    subprocess.run(["gcc", "-O3", "-march=native", "-Wall", "-DFLINTBLAS", f"-I{FLINTB}/include", "-o",
                    str(CB / "gemm_fb"), str(CB / "gemm.c"), f"-L{FLINTB}/lib", "-lflint", "-lgmp", "-lm", BLAS,
                    f"-Wl,-rpath,{FLINTB}/lib:{BLASDIR}"], check=True)
    subprocess.run(["gcc", "-O3", "-march=native", "-Wall", "-o", str(CB / "decode"), str(CB / "decode.c")], check=True)


def binary_for(method):
    """(binary, method name inside it): the system FLINT for fmpz-noblas, the BLAS-enabled build otherwise."""
    return (CB / "gemm", "fmpz") if method == "fmpz-noblas" else (CB / "gemm_fb", method)


def run(method, n, bits, reps=3, edge=None):
    env = None
    if edge:
        import os
        env = dict(os.environ, GEMM_EDGE=str(edge))
    binary, m = binary_for(method)
    r = subprocess.run([str(binary), m, str(n), str(bits), str(reps)], capture_output=True, text=True, env=env)
    if r.returncode == 4:
        raise AssertionError(f"gemm {method} {n} {bits}: result differs from FLINT")
    r.check_returncode()
    v = r.stdout.split()
    out = dict(median=float(v[0]), min=float(v[1]))
    out["peak_mb"] = float(v[-1])
    if len(v) > 3:
        out.update(reduce=float(v[2]), gemm=float(v[3]), crt=float(v[4]), primes=int(v[5]))
    return out


def decode(n=10**7, reps=9):
    r = subprocess.run([str(CB / "decode"), str(n), str(reps)], capture_output=True, text=True, check=True)
    v = list(map(float, r.stdout.split()[:6]))
    return dict(fitted=v[0], uniform=v[1], runtime=v[2], fitted_cached=v[3], uniform_cached=v[4], runtime_cached=v[5])
