"""Order-preserving keys for Q values, after Kornerup and Matula's LCF (lexicographic continued fractions).

A positive rational p/q has a unique continued fraction [a0; a1, ..., an] with an >= 2 (or n = 0). Two values
compare by their first differing quotient: a larger quotient at an even index means a larger value, at an odd
index a smaller one; a finite expansion behaves as if followed by an infinite quotient. Each quotient is written
in an order-preserving, self-delimiting code, odd-indexed codes are complemented, and the infinite quotient that
ends every expansion is written as all ones (complemented at odd positions). Read as a fixed-width unsigned
integer, the key then orders values exactly as the rationals are ordered, provided every quotient's code fits.

Two codes for quotients are compared: lexibinary (k ones, a 0, then the k low bits of a; 2*bits(a) - 1 bits),
LCF's own; and a variant that writes a0 in a fixed 65 bits, because a0 can be as large as 2**64 - 1."""
import math


def cf(p, q):
    """Continued fraction of p/q (p >= 0, q >= 1) with last quotient >= 2 unless there is only one."""
    out = []
    while q:
        a, r = divmod(p, q)
        out.append(a)
        p, q = q, r
    return out


def lexibinary(a):
    """a >= 1: (bits(a) - 1) ones, a zero, then the low bits(a) - 1 bits of a."""
    k = a.bit_length() - 1
    return "1" * k + "0" + (format(a, "b")[1:] if k else "")


def delta(a):
    """a >= 1: the bit length L of a in lexibinary, then the low L - 1 bits of a. Order-preserving and
    self-delimiting; 2*bits(bits(a)) + bits(a) - 2 bits, so large quotients cost about half of lexibinary."""
    L = a.bit_length()
    return lexibinary(L) + (format(a, "b")[1:] if L > 1 else "")


def flip(s):
    return s.translate(str.maketrans("01", "10"))


def code(p, q, a0_fixed=False, qcode=None):
    """The bit string of p/q >= 0 (without the infinite terminator). qcode: lexibinary (LCF's) or delta."""
    qcode = qcode or lexibinary
    qs = cf(p, q)
    parts = [format(qs[0], "065b") if a0_fixed else qcode(qs[0] + 1)]   # a0 may be 0
    for k, a in enumerate(qs[1:], start=1):
        c = qcode(a)
        parts.append(flip(c) if k % 2 else c)
    return "".join(parts), len(qs)


def key(p, q, width, a0_fixed=False, qcode=None):
    """Fixed-width key: the code, then the infinite terminator (ones at an even index, zeros at an odd one).
    None if the code does not fit."""
    s, n = code(p, q, a0_fixed, qcode)
    if len(s) > width:
        return None
    pad = "1" if n % 2 == 0 else "0"      # the terminator sits at index n
    return int(s + pad * (width - len(s)), 2)


def signed_key(sign, p, q, width, a0_fixed=False, qcode=None):
    """Keys over all reals: one sign bit; negative values take the complement of their magnitude's key."""
    k = key(p, q, width - 1, a0_fixed, qcode)
    if k is None:
        return None
    if p == 0:
        return 1 << (width - 1)
    return (1 << (width - 1)) | k if sign > 0 else ((1 << (width - 1)) - 1 - k)
