"""axes.py: the four-direction check of spec 13 §4, value only (no headers' classes, words or pools), against Gaussian
integers held as (re, im) pairs of Python integers.

A: a number is (d, m), value m * i^d, with §2's direction arithmetic on the header's direction bits (8 bits, D = 0xC0);
zero has no axis: an add with a zero operand returns the other (a subtract, the second negated).
B: a number is four magnitudes (m0, m1, m2, m3) on 1, i, -1, -i; a product of slots j and k lands in slot (j + k) mod 4.
Collapse per axis (§4): |m0 - m2| to slot 0 or 2 by its borrow, and |m1 - m3| to slot 1 or 3."""
import random

D = 0xC0
UNIT = [(1, 0), (0, 1), (-1, 0), (0, -1)]


class AxisError(Exception):
    pass


def gmul(x, y):
    return x[0] * y[0] - x[1] * y[1], x[0] * y[1] + x[1] * y[0]


def gscale(m, u):
    return m * u[0], m * u[1]


# ------------------------------------------------------------------------------------------------ A
def a_val(x):
    d, m = x
    return gscale(m, UNIT[d])


def a_mk(d, m):
    return (0, 0) if m == 0 else (d, m)


def a_hd(x):
    return x[0] << 6


def a_neg(x):
    h = a_hd(x) ^ 0x80
    return a_mk(h >> 6 & 3, x[1])


def a_add(x, y, flip=0):
    """flip = 0x80 is subtract: the add rules with h2 xor 0x80 in place of h2"""
    if y[1] == 0:
        return x
    if x[1] == 0:
        return a_neg(y) if flip else y
    h1, h2 = a_hd(x) & D, (a_hd(y) ^ flip) & D
    if h1 == h2:
        return a_mk(h1 >> 6, x[1] + y[1])
    if h1 ^ h2 == 0x80:
        b = int(x[1] < y[1])
        return a_mk(((h1 + (b << 7)) & 0xFF) >> 6, abs(x[1] - y[1]))
    raise AxisError("sum across axes")


def a_sub(x, y):
    return a_add(x, y, 0x80)


def a_mul(x, y):
    return a_mk((((a_hd(x) & D) + (a_hd(y) & D)) & 0xFF) >> 6, x[1] * y[1])


def a_div(x, y):
    assert x[1] % y[1] == 0
    return a_mk((((a_hd(x) & D) - (a_hd(y) & D)) & 0xFF) >> 6, x[1] // y[1])


def a_canonical(x):
    return x[1] != 0 or x == (0, 0)


# ------------------------------------------------------------------------------------------------ B
def b_val(x):
    re = im = 0
    for k in range(4):
        r, i = gscale(x[k], UNIT[k])
        re += r
        im += i
    return re, im


def b_neg(x):
    return tuple(x[(k + 2) % 4] for k in range(4))


def b_add(x, y):
    return tuple(x[k] + y[k] for k in range(4))


def b_sub(x, y):
    """slot k of y is added to slot k + 2: the crossed parts"""
    return tuple(x[k] + y[(k + 2) % 4] for k in range(4))


def b_mul(x, y):
    r = [0, 0, 0, 0]
    for j in range(4):
        for k in range(4):
            r[(j + k) % 4] += x[j] * y[k]
    return tuple(r)


def b_collapse(x):
    r = [0, 0, 0, 0]
    for lo in (0, 1):
        p, n = x[lo], x[lo + 2]
        b = int(p < n)
        r[lo + 2 * b] = abs(p - n)
    return tuple(r)


def b_collapsed(x):
    return x[0] * x[2] == 0 and x[1] * x[3] == 0


# ------------------------------------------------------------------------------------------------ the check
def run(nrand, seed):
    rng = random.Random(seed)
    out = {}

    def rec(key, ok, case):
        c = out.setdefault(key, {"n": 0, "fail": 0, "example": None})
        c["n"] += 1
        if not ok:
            c["fail"] += 1
            if c["example"] is None:
                c["example"] = case

    # A: every pair with magnitudes below 2^5, all four directions; then random magnitudes up to 32 words of 4 bits
    small = [(0, 0)] + [(d, m) for d in range(4) for m in range(1, 32)]
    big = lambda: a_mk(rng.randrange(4), 0 if rng.random() < 1 / 16 else rng.getrandbits(rng.randint(1, 128)))  # noqa
    for xs in (small, None):
        pairs = ([(x, y) for x in xs for y in xs] if xs else
                 [(big(), big()) for _ in range(nrand)])
        for x, y in pairs:
            if xs is None and rng.random() < 1 / 16:
                y = a_mk(rng.randrange(4), x[1])
            vx, vy = a_val(x), a_val(y)
            for name, f, ref in (("A mul", a_mul, gmul(vx, vy)),):
                r = f(x, y)
                rec(name, a_val(r) == ref and a_canonical(r), (x, y, ref, r))
            r = a_neg(x)
            rec("A neg", a_val(r) == (-vx[0], -vx[1]) and a_canonical(r), (x, r))
            same_axis = x[1] == 0 or y[1] == 0 or (x[0] - y[0]) % 2 == 0
            for name, f, ref in (("A add", a_add, (vx[0] + vy[0], vx[1] + vy[1])),
                                 ("A sub", a_sub, (vx[0] - vy[0], vx[1] - vy[1]))):
                try:
                    r = f(x, y)
                    rec(name, same_axis and a_val(r) == ref and a_canonical(r), (x, y, ref, r))
                except AxisError:
                    rec(name + " (cross-axis error)", not same_axis, (x, y, "no error", "error"))
            if y[1]:                                # division: a = b * q, q from x
                a = a_mul(y, x)
                r = a_div(a, y)
                rec("A div", a_val(r) == vx and a_canonical(r), (a, y, vx, r))

    # B: every pair of four-slot values with each slot below 4; then random slots up to 32 words of 4 bits
    small = [(a, b, c, d) for a in range(4) for b in range(4) for c in range(4) for d in range(4)]
    part = lambda: 0 if rng.random() < 1 / 16 else rng.getrandbits(rng.randint(1, 128))  # noqa: E731
    for xs in (small, None):
        pairs = ([(x, y) for x in xs for y in xs] if xs else
                 [(tuple(part() for _ in range(4)), tuple(part() for _ in range(4))) for _ in range(nrand)])
        for x, y in pairs:
            vx, vy = b_val(x), b_val(y)
            for name, f, ref in (("B add", b_add, (vx[0] + vy[0], vx[1] + vy[1])),
                                 ("B sub", b_sub, (vx[0] - vy[0], vx[1] - vy[1])),
                                 ("B mul", b_mul, gmul(vx, vy))):
                r = f(x, y)
                c = b_collapse(r)
                rec(name, b_val(r) == ref, (x, y, ref, r))
                rec("B collapse", b_val(c) == ref and b_collapsed(c), (r, ref, c))
            r = b_neg(x)
            rec("B neg", b_val(r) == (-vx[0], -vx[1]), (x, r))
            c = b_collapse(x)
            rec("B collapse", b_val(c) == vx and b_collapsed(c), (x, vx, c))
    return out


if __name__ == "__main__":
    import json
    print(json.dumps(run(1000, 1), indent=1, default=str))
