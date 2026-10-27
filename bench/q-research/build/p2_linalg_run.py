"""Runs cbench/p2_linalg.c: exact linear algebra over the rationals, the format against GMP and FLINT.

Operations (W = the bits a result entry needs, in WIDTHS):
  mv     y = A x, n = 512                 spmv   CSR, n = 10^4, n/100 nonzeros per row
  mm     C = A B, n = 128, 256            solve  A x = b, n = 32, 64, entries of W/8 bits
Contenders: ours (the vector form; mm is p1_gemm.c's mmi, solve is multimodular Cramer on IFMA), gmp_mpz, gmp_mpq,
flint_fmpz, flint_fmpq (FLINT 3.0.1 built with BLAS). See the header of p2_linalg.c.
"""
import os
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
CB = HERE.parent / "cbench"
SRC = CB / "p2_linalg.c"
BIN = CB / "p2_linalg"
# OpenBLAS from the scipy-openblas wheel (one thread) and FLINT 3.0.1 configured with BLAS. FLINT_ROOT selects the
# FLINT install; by default the one built with CFLAGS='-O3 -march=native' (inst-native), else the generic build (inst).
FB = Path("/home/claude/flintblas")
BLASDIR = str(FB / "shim" / "lib")
BLAS = BLASDIR + "/libscipy_openblas.so"


def flint_root():
    if os.environ.get("FLINT_ROOT"):
        return Path(os.environ["FLINT_ROOT"])
    native = FB / "inst-native"
    return native if (native / "lib" / "libflint.so").exists() else FB / "inst"


FLINTB = flint_root()
OPS = ("mv", "mm", "spmv", "solve")
CONTENDERS = ("ours", "gmp_mpz", "gmp_mpq", "flint_fmpz", "flint_fmpq")
WIDTHS = (128, 256, 512, 1024, 2048)
SIZES = {"mv": (512,), "mm": (128, 256), "spmv": (10**4,), "solve": (32, 64)}
# FLINT's candidate routines per (contender, op), the names P2_METHOD takes (see p2_linalg.c above fz_setup)
METHODS = {
    ("flint_fmpz", "mv"): ("mat", "dot", "vec"),
    ("flint_fmpz", "mm"): ("mul", "blas", "multi_mod", "fft"),
    ("flint_fmpz", "spmv"): ("addmul",),
    ("flint_fmpz", "solve"): ("solve", "dixon", "multi_mod", "fflu", "q_solve", "q_dixon", "q_multi_mod"),
    ("flint_fmpq", "mv"): ("mul", "cleared", "direct", "vec"),
    ("flint_fmpq", "mm"): ("mul", "cleared", "direct"),
    ("flint_fmpq", "spmv"): ("addmul",),
    ("flint_fmpq", "solve"): ("solve", "dixon", "multi_mod", "fraction_free"),
}


def build(force=False):
    """Rebuilds when the sources are newer or the binary was linked against another FLINT (recorded beside it)."""
    deps = [SRC, CB / "p1_gemm.c"]
    stamp = BIN.with_suffix(".flint")
    same = stamp.exists() and stamp.read_text() == str(FLINTB)
    if force or not same or not BIN.exists() or BIN.stat().st_mtime < max(d.stat().st_mtime for d in deps):
        subprocess.run(["gcc", "-O3", "-march=native", "-Wall", "-Wno-unused-function", f"-I{FLINTB}/include",
                        "-o", str(BIN), str(SRC), f"-L{FLINTB}/lib", "-lflint", "-lgmp", "-lm", BLAS,
                        f"-Wl,-rpath,{FLINTB}/lib:{BLASDIR}"], check=True)
        stamp.write_text(str(FLINTB))


def run(op, contender, n, W, reps=3, edge=None, corrupt=False, k=None, drop=False, method=None, rt=False, outfile=None,
        timeout=None):
    """dict(median, min, peak_mb, primes, method); AssertionError if the result fails its check. method: FLINT's
    routine for the flint contenders (METHODS), None for the default."""
    env = {x: y for x, y in os.environ.items() if not x.startswith("P2_")}
    if edge:
        env["P2_EDGE"] = str(edge)
    if corrupt:
        env["P2_CORRUPT"] = "1"
    if k:
        env["P2_K"] = str(k)
    if drop:
        env["P2_DROP"] = "1"
    if method:
        env["P2_METHOD"] = method
    if rt:
        env["P2_RT"] = "1"
    args = ["nice", str(BIN), op, contender, str(n), str(W), str(reps)] + ([str(outfile)] if outfile else [])
    r = subprocess.run(args, capture_output=True, text=True, env=env, timeout=timeout)
    if r.returncode == 4:
        raise AssertionError(f"p2_linalg {op} {contender} n={n} W={W}: result fails the check ({r.stderr.strip()})")
    if r.returncode:
        raise RuntimeError(f"p2_linalg {op} {contender} n={n} W={W}: exit {r.returncode}: {r.stderr.strip()}")
    v = r.stdout.split()
    return dict(median=float(v[0]), min=float(v[1]), peak_mb=float(v[2]), primes=int(v[3]),
                method=None if v[4] == "-" else v[4])


def read_out(path):
    """(op, n, W, k, dA, dB, A, B, result) from OUTFILE; A is a dense row-major list, or (rowptr, cols, vals)
    for spmv; result a list of (num, den)."""
    lines = Path(path).read_text().split("\n")
    op, n, W, k = lines[0].split()
    n, W, k = int(n), int(W), int(k)
    dA, dB = map(int, lines[1].split())
    pos = 2

    def take(c):
        nonlocal pos
        v = [int(x) for x in lines[pos:pos + c]]
        pos += c
        return v
    if op == "spmv":
        rp = take(n + 1)
        cols = take(rp[-1])
        A = (rp, cols, take(rp[-1]))
    else:
        A = take(n * n)
    B = take(n * n if op == "mm" else n)
    cnt = n * n if op == "mm" else n
    res = [tuple(map(int, ln.split())) for ln in lines[pos:pos + cnt]]
    return op, n, W, k, dA, dB, A, B, res
