# An exact matrix product

A matrix of decimals in the vector form is an integer matrix of numerators over one denominator, so its product is an integer matrix product over the product of the denominators. Integers below $2^{53}$ are exact in float64, so the product can run on ordinary float BLAS, exactly, provided every partial sum stays below $2^{53}$.

`exact_gemm` checks that bound from the largest magnitudes. If $\max|A| \cdot \max|B| \cdot n < 2^{53}$, one float GEMM is exact. Otherwise each matrix is split into signed limbs of $b$ bits with $n \cdot 2^{2b} \le 2^{53}$, each pair of limbs is one exact GEMM, and the limb products are recombined in int64, in signed 128-bit words, or in Python integers, whichever the bound allows. This is the error-free splitting of Ozaki, Ogita, Rump and Oishi (2008), with the splitting chosen so that every product is exact.

Linear solves use the solver chain from v0.1 unchanged: numeric–symbolic refinement, then Dixon's p-adic lifting on BLAS (itself a use of the p-adic arithmetic behind quote notation), then FLINT.

```python {file=build/gemm.py}
import numpy as np
from numba import njit

FLOAT_EXACT = 2**53
U = np.uint64


class Int128Matrix:
    """A matrix of signed 128-bit integers held as a high int64 word and a low uint64 word per entry."""

    __slots__ = ("hi", "lo")

    def __init__(self, hi, lo):
        self.hi, self.lo = hi, lo

    @property
    def shape(self):
        return self.hi.shape

    def to_python(self):
        return [[(int(h) << 64) | int(l) for h, l in zip(hr, lr)] for hr, lr in zip(self.hi.tolist(), self.lo.tolist())]


def limb_bits(n):
    """Largest limb width b with n * 2**(2b) <= 2**53, so a float64 dot product of n limb products is exact."""
    return (53 - int(n).bit_length()) // 2


def magnitudes(M):
    """|M| as uint64 (two's complement negation, so -2**63 gives 2**63) and the signs."""
    mag = M.astype(np.uint64)
    neg = M < 0
    mag[neg] = ~mag[neg] + U(1)
    return mag, np.where(neg, -1, 1).astype(np.int64)


@njit
def accumulate128(P, shift, hi, lo):
    """(hi, lo) += P * 2**shift, entrywise, in signed 128-bit arithmetic (|P| < 2**53, 0 <= shift < 128)."""
    rows, cols = P.shape
    for i in range(rows):
        for j in range(cols):
            v = P[i, j]
            if shift == 0:
                vlo, vhi = U(v), np.int64(-1 if v < 0 else 0)
            elif shift < 64:
                vlo, vhi = U(v) << U(shift), v >> (64 - shift)
            else:
                vlo, vhi = U(0), v << (shift - 64)
            s = lo[i, j] + vlo
            hi[i, j] += vhi + (1 if s < vlo else 0)
            lo[i, j] = s


def exact_gemm(A, B):
    """Exact integer product A @ B of int64 matrices, computed with float64 BLAS.

    This is an error-free splitting in the spirit of Ozaki, Ogita, Rump and Oishi (2008), made exact by bounding
    every partial product. If every product, and every sum of n of them, stays below 2**53, one float GEMM is
    already exact. Otherwise each matrix is split into signed limbs of b bits, with n * 2**(2b) <= 2**53, so every
    limb product is one exact GEMM; the limbs are recombined in int64 when the result provably fits, in signed
    128-bit words (Int128Matrix) when it fits 126 bits, and in Python integers beyond that."""
    A = np.ascontiguousarray(A, dtype=np.int64)
    B = np.ascontiguousarray(B, dtype=np.int64)
    n = A.shape[1]
    if B.shape[0] != n:
        raise ValueError("inner dimensions differ")
    ma, sa = magnitudes(A)
    mb, sb = magnitudes(B)
    amax = int(ma.max()) if ma.size else 0
    bmax = int(mb.max()) if mb.size else 0
    bound = amax * bmax * n
    if bound < FLOAT_EXACT:
        return (A.astype(np.float64) @ B.astype(np.float64)).astype(np.int64)
    b = limb_bits(n)
    if b < 1:
        raise ValueError("inner dimension too large for exact float limbs")
    la = limbs(ma, sa, b, max(1, -(-amax.bit_length() // b)))
    lb = limbs(mb, sb, b, max(1, -(-bmax.bit_length() // b)))
    out_shape = (A.shape[0], B.shape[1])
    small, wide = bound < 2**62, bound < 2**126
    if small:
        C = np.zeros(out_shape, dtype=np.int64)
    elif wide:
        hi, lo = np.zeros(out_shape, dtype=np.int64), np.zeros(out_shape, dtype=np.uint64)
    else:
        C = np.zeros(out_shape, dtype=object)
    for i, x in enumerate(la):
        for j, y in enumerate(lb):
            P = (x @ y).astype(np.int64)
            if small:
                C += P << (b * (i + j))
            elif wide:
                accumulate128(P, b * (i + j), hi, lo)
            else:
                C += P.astype(object) << (b * (i + j))
    return Int128Matrix(hi, lo) if (wide and not small) else C


def limbs(mag, sign, b, k):
    """Signed limbs of a matrix in base 2**b, as float64: M = sum(limb[i] * 2**(b*i))."""
    mask = U((1 << b) - 1)
    return [(((mag >> U(b * i)) & mask).astype(np.int64) * sign).astype(np.float64) for i in range(k)]
```
