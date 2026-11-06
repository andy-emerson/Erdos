"""Unbounded-arithmetic follow-up for starts that gstop or gstopf reported as FAIL (escape past
2^110 or cap): iterate the raw map T of a gcz rule file in Python integers until the norm drops
below the start's norm (PASS), the orbit returns to an associate of the start (CYCLE), or a step
cap is reached (OPEN).  Same rule semantics as gstop.c: z -> (AL z + BE) conj(DE) / N(DE).

usage: python3 gstop_big.py rulefile log-with-FAIL-lines [cap=10^8]   (one line per start)
"""
import re
import sys


def load(path):
    f = open(path)
    M = int(f.readline().split()[1])
    T = {}
    for line in f:
        ra, rb, a1, a2, b1, b2, d1, d2 = map(int, line.split())
        T[(ra, rb)] = (a1, a2, b1, b2, d1, d2, d1 * d1 + d2 * d2)
    return M, T


def check(M, T, a, b, cap):
    n0 = a * a + b * b
    assoc = {(a, b), (-b, a), (-a, -b), (b, -a)}
    x, y = a, b
    top = n0
    for s in range(1, cap + 1):
        a1, a2, b1, b2, d1, d2, dn = T[(x % M, y % M)]
        u, v = a1 * x - a2 * y + b1, a1 * y + a2 * x + b2
        p, q = u * d1 + v * d2, v * d1 - u * d2
        assert p % dn == 0 and q % dn == 0
        x, y = p // dn, q // dn
        nn = x * x + y * y
        if nn > top:
            top = nn
        if nn < n0:
            return "PASS", s, top.bit_length() - n0.bit_length()
        if nn == n0 and (x, y) in assoc:
            return "CYCLE", s, 0
    return "OPEN", cap, top.bit_length() - n0.bit_length()


def main():
    M, T = load(sys.argv[1])
    cap = int(float(sys.argv[3])) if len(sys.argv) > 3 else 10**8
    for line in open(sys.argv[2]):
        m = re.match(r"FAIL (-?\d+) (-?\d+)", line)
        if m:
            a, b = int(m.group(1)), int(m.group(2))
            r, s, e = check(M, T, a, b, cap)
            print("%s %d %d steps %d excursion_bits_about %d" % (r, a, b, s, e), flush=True)


if __name__ == "__main__":
    main()
