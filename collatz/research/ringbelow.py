"""Below the |N| window in Z[sqrt2]: the norm form of W1 fails with two real places.

Map at modulus pi^2 = 2 (pi = sqrt2, q = 2, k = 2) with multiplier alpha = 3 + 2 sqrt2 = eps^2,
|N(alpha)| = 1 < q^k = 4, but |sigma_1(alpha)| = 5.83 > |pi|^2 = 2.  Digits: for each odd class
mod 2 the element of least a^2 + 2b^2 in the class of -alpha z (the smallest digits).  Odd starts
a + b sqrt2 with a odd, |a|, |b| <= 30; an orbit counts as escaped when a coordinate passes 10^40,
as periodic when a value repeats.  Used in general-theory.md Section 6.4.
Output: logs/rings_Zr2_below.txt.
"""
import random, sys

ALPHA = (3, 2)


def mul(x, y):
    return (x[0] * y[0] + 2 * x[1] * y[1], x[0] * y[1] + x[1] * y[0])


def is_odd(z):
    return z[0] % 2 != 0


def div_pi(z):  # (a + b r)/r = b + (a/2) r
    a, b = z
    assert a % 2 == 0
    return (b, a // 2)


def digit(z):
    w = mul(ALPHA, z)
    best = None
    for a in range(-3, 4, 2):
        for b in range(-3, 4):
            if (w[0] + a) % 2 == 0 and (w[1] + b) % 2 == 0:
                s = a * a + 2 * b * b
                if best is None or s < best[0]:
                    best = (s, (a, b))
    return best[1]


def S(z):
    d = digit(z)
    w = mul(ALPHA, z)
    w = (w[0] + d[0], w[1] + d[1])
    if w == (0, 0):
        return w
    while not is_odd(w):
        w = div_pi(w)
    return w


def main():
    random.seed(1)
    esc = per = zero = 0
    starts = 200
    for _ in range(starts):
        z = (random.randrange(-30, 31) | 1, random.randrange(-30, 31))
        seen = set()
        status = "cap"
        for _n in range(2000):
            if z == (0, 0):
                status = "zero"
                break
            if z in seen:
                status = "periodic"
                break
            seen.add(z)
            z = S(z)
            if max(abs(z[0]), abs(z[1])) > 10 ** 40:
                status = "escaped"
                break
        esc += status == "escaped"
        per += status == "periodic"
        zero += status == "zero"
    print(f"Z[sqrt2], alpha = 3+2sqrt2, modulus 2, smallest digits, {starts} odd starts |a|,|b| <= 30")
    print(f"escaped past 10^40: {esc}; periodic: {per}; reached 0: {zero}; capped: {starts - esc - per - zero}")


if __name__ == "__main__":
    main()
