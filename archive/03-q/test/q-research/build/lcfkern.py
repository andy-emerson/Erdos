"""Compiled 128-bit order-preserving keys for Q72 values (delta-coded continued fractions), and the sorting
methods they are compared with."""
import numpy as np
from numba import njit

U = np.uint64
ZERO, ONE = U(0), U(1)
ALL = ~U(0)


@njit(inline="always")
def bitlen(x):
    n = 0
    while x:
        x >>= ONE
        n += 1
    return n


@njit(inline="always")
def put(hi, lo, pos, bits, n):
    """Write the low n bits of `bits` (n <= 64) at bit position pos (0 = most significant of 128)."""
    if n == 0:
        return hi, lo
    v = bits & (ALL >> U(64 - n)) if n < 64 else bits
    end = pos + n                     # bits occupy [pos, end)
    if end <= 64:
        hi |= v << U(64 - end)
    elif pos >= 64:
        lo |= v << U(128 - end)
    else:
        k = end - 64                  # bits that spill into lo
        hi |= v >> U(k)
        lo |= v << U(64 - k)
    return hi, lo


@njit(inline="always")
def put_delta(hi, lo, pos, a, a_is_2_64, flip):
    """Delta code of a (or of 2**64 when a_is_2_64): lexibinary of L = bits(a), then the low L - 1 bits."""
    L = 65 if a_is_2_64 else bitlen(a)
    k = bitlen(U(L)) - 1
    n = k + 1 + k + (L - 1)
    word_l = (ALL >> U(64 - k)) << U(k + 1) if k else ZERO      # k ones, a zero, then ...
    word_l |= U(L) & ((ONE << U(k)) - ONE)                      # ... the low k bits of L
    nl = 2 * k + 1
    if flip:
        word_l = ~word_l
    hi, lo = put(hi, lo, pos, word_l, nl)
    pos += nl
    m = L - 1
    if m:
        low = ZERO if a_is_2_64 else a          # low L - 1 bits of a (2**64 has none set)
        if flip:
            low = ~low
        hi, lo = put(hi, lo, pos, low, m)
        pos += m
    return hi, lo, pos


@njit
def magnitude_key(p, q):
    """127-bit key (in a 128-bit pair, top bit clear) of p/q >= 0; the key orders values exactly."""
    hi, lo = ZERO, ZERO
    pos = 1                                    # bit 0 is reserved for the sign
    a = p // q
    r = p - a * q
    a0_max = a == ALL                          # a0 + 1 = 2**64
    hi, lo, pos = put_delta(hi, lo, pos, a + ONE if not a0_max else ZERO, a0_max, False)
    idx = 1
    x, y = q, r
    while y:
        a = x // y
        x, y = y, x - a * y
        hi, lo, pos = put_delta(hi, lo, pos, a, False, idx % 2 == 1)
        idx += 1
    if idx % 2 == 0:                           # the infinite terminator: ones at an even index, zeros at odd
        rest = 128 - pos
        if rest > 0:
            if rest >= 64:
                hi, lo = put(hi, lo, pos, ALL, 64)
                hi, lo = put(hi, lo, pos + 64, ALL, rest - 64)
            else:
                hi, lo = put(hi, lo, pos, ALL, rest)
    return hi, lo


@njit
def keys(neg, p, q, khi, klo):
    """Signed keys: sign bit 1 for values >= 0; a negative value takes the complement of its magnitude key."""
    for i in range(len(p)):
        hi, lo = magnitude_key(p[i], q[i])
        if neg[i] and p[i] != ZERO:
            hi, lo = (~hi) & (ALL >> ONE), ~lo
        else:
            hi |= ONE << U(63)
        khi[i], klo[i] = hi, lo


@njit
def mul128(a, b):
    a0, a1 = a & U(0xFFFFFFFF), a >> U(32)
    b0, b1 = b & U(0xFFFFFFFF), b >> U(32)
    p00, p01, p10, p11 = a0 * b0, a0 * b1, a1 * b0, a1 * b1
    mid = (p00 >> U(32)) + (p01 & U(0xFFFFFFFF)) + (p10 & U(0xFFFFFFFF))
    return p11 + (p01 >> U(32)) + (p10 >> U(32)) + (mid >> U(32)), (p00 & U(0xFFFFFFFF)) | (mid << U(32))


@njit(inline="always")
def less(na, pa, qa, nb, pb, qb):
    """Exact a < b for signed rationals, by cross-multiplication in 128 bits."""
    za, zb = pa == ZERO, pb == ZERO
    sa = 0 if za else (-1 if na else 1)
    sb = 0 if zb else (-1 if nb else 1)
    if sa != sb:
        return sa < sb
    if sa == 0:
        return False
    h1, l1 = mul128(pa, qb)
    h2, l2 = mul128(pb, qa)
    mag_less = h1 < h2 or (h1 == h2 and l1 < l2)
    mag_greater = h1 > h2 or (h1 == h2 and l1 > l2)
    return mag_less if sa > 0 else mag_greater


@njit
def merge_sort_exact(neg, p, q):
    """Indices that sort the values, by a merge sort whose comparisons are exact cross-multiplications."""
    n = len(p)
    idx = np.arange(n)
    tmp = np.empty(n, dtype=np.int64)
    width = 1
    while width < n:
        for lo in range(0, n, 2 * width):
            mid, hi = min(lo + width, n), min(lo + 2 * width, n)
            i, j, k = lo, mid, lo
            while i < mid and j < hi:
                a, b = idx[i], idx[j]
                if less(neg[b], p[b], q[b], neg[a], p[a], q[a]):
                    tmp[k] = b
                    j += 1
                else:
                    tmp[k] = a
                    i += 1
                k += 1
            while i < mid:
                tmp[k] = idx[i]; i += 1; k += 1
            while j < hi:
                tmp[k] = idx[j]; j += 1; k += 1
        idx, tmp = tmp, idx
        width *= 2
    return idx


@njit
def fix_ties(order, f, neg, p, q):
    """After sorting by float64 approximations (exact order wherever the floats differ, since rounding is
    monotonic), sort each run of equal floats exactly. Returns the number of runs fixed."""
    n = len(order)
    runs = 0
    i = 0
    while i < n:
        j = i + 1
        while j < n and f[order[j]] == f[order[i]]:
            j += 1
        if j - i > 1:
            runs += 1
            for a in range(i + 1, j):          # insertion sort within the run
                x = order[a]
                b = a - 1
                while b >= i and less(neg[x], p[x], q[x], neg[order[b]], p[order[b]], q[order[b]]):
                    order[b + 1] = order[b]
                    b -= 1
                order[b + 1] = x
        i = j
    return runs


@njit
def merge_sort_keys(kh, kl):
    """The same merge sort as merge_sort_exact, comparing 128-bit keys instead of cross-multiplying."""
    n = len(kh)
    idx = np.arange(n)
    tmp = np.empty(n, dtype=np.int64)
    width = 1
    while width < n:
        for lo in range(0, n, 2 * width):
            mid, hi = min(lo + width, n), min(lo + 2 * width, n)
            i, j, k = lo, mid, lo
            while i < mid and j < hi:
                a, b = idx[i], idx[j]
                if kh[b] < kh[a] or (kh[b] == kh[a] and kl[b] < kl[a]):
                    tmp[k] = b
                    j += 1
                else:
                    tmp[k] = a
                    i += 1
                k += 1
            while i < mid:
                tmp[k] = idx[i]; i += 1; k += 1
            while j < hi:
                tmp[k] = idx[j]; j += 1; k += 1
        idx, tmp = tmp, idx
        width *= 2
    return idx
