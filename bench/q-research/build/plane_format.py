"""How often complex values fit the plane format, against a pair of axis values in the same number of bits.

P80 is a 16-bit header and a U64; 2 x Q40 is the same 80 bits as two axis values. P152 is a 24-bit header and a
U128; 2 x Q72 is 144 bits. Also reported: the bits each representation needs for the value (without headers)."""
import math
import random
from fractions import Fraction

import numpy as np

import plane as P


def datasets(n, seed=7):
    rng = random.Random(seed)

    def dec(places, bound):
        return Fraction(rng.randint(-bound * 10**places, bound * 10**places), 10**places)

    def z(places=3, bound=10**3):
        return (dec(places, bound), dec(places, bound))

    def mul(a, b):
        return (a[0] * b[0] - a[1] * b[1], a[0] * b[1] + a[1] * b[0])

    def div(a, b):
        n2 = b[0] ** 2 + b[1] ** 2
        return ((a[0] * b[0] + a[1] * b[1]) / n2, (a[1] * b[0] - a[0] * b[1]) / n2)

    def gauss(bound):
        return (Fraction(rng.randint(-bound, bound)), Fraction(rng.randint(-bound, bound)))

    out = {
        "complex decimals, 3 places, parts below 10^3": lambda: z(),
        "complex decimals, 6 places, parts below 10^6": lambda: z(6, 10**6),
        "products of two 3-place complex decimals": lambda: mul(z(), z()),
        "quotients of two 3-place complex decimals": lambda: div(z(), z()),
        "quotients of Gaussian integers below 1000": lambda: div(gauss(1000), gauss(1000)),
        "quotients of Gaussian integers below 10^6": lambda: div(gauss(10**6), gauss(10**6)),
        "products of three Gaussian-integer quotients below 100": lambda: mul(mul(div(gauss(100), gauss(100)),
                                                                                 div(gauss(100), gauss(100))),
                                                                             div(gauss(100), gauss(100))),
    }
    result = {}
    for name, gen in out.items():
        vals = []
        while len(vals) < n:
            v = gen()
            if v[1] == 0 and v[0] == 0:
                continue
            vals.append(v)
        result[name] = vals
    return result


def bits_plane(z):
    a, b, c = P.as_plane(z)
    g = math.gcd(math.gcd(abs(a), abs(b)), c)
    a, b, c = a // g, b // g, c // g
    return abs(a).bit_length() + abs(b).bit_length() + (c - 1).bit_length()


def bits_pair(z):
    t = 0
    for x in z:
        x = Fraction(x)
        if x:
            t += abs(x.numerator).bit_length() + (x.denominator - 1).bit_length()
    return t


def table(n=20000):
    rows = []
    for name, vals in datasets(n).items():
        bp = [bits_plane(z) for z in vals]
        bq = [bits_pair(z) for z in vals]
        rows.append(dict(
            values=name,
            p80=sum(P.plane_fits(*P.as_plane(z), 64) for z in vals) / n,
            q40x2=sum(P.pair_fits(z, 32) for z in vals) / n,
            p152=sum(P.plane_fits(*P.as_plane(z), 128) for z in vals) / n,
            q72x2=sum(P.pair_fits(z, 64) for z in vals) / n,
            plane_bits=float(np.median(bp)), pair_bits=float(np.median(bq))))
    return rows


if __name__ == "__main__":
    for r in table(5000):
        print(r)
