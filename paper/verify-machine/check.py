# check.py: the arithmetic claims in the machine-level supplement.
import random

# 1.1: the clock
assert (9 + 5) % 12 == 2
# 1.2: Booleans as mod 2 arithmetic, and De Morgan
for x in (0, 1):
    assert 1 - x == (1 + x) % 2                                   # NOT
    for y in (0, 1):
        assert x & y == x * y % 2 and x ^ y == (x + y) % 2       # AND, XOR
        assert x | y == 1 - ((1 - x) & (1 - y)) == (1 + (1 + x) * (1 + y)) % 2 == (x + y + x * y) % 2   # OR
# 1.2: NAND, and every operation from NAND alone
nand = lambda x, y: 1 - (x & y)
for x in (0, 1):
    assert nand(x, x) == 1 - x
    for y in (0, 1):
        assert nand(x, y) == (1 + x * y) % 2 and nand(x, y) != (x | y) or (x, y) != (0, 0)
        assert 1 - nand(x, y) == x & y and nand(1 - x, 1 - y) == x | y
assert nand(0, 0) == 1 and (0 | 0) == 0
# 1.3: column addition: total = digit + 2 carry, digit = XOR of the three, carry = at least two; and 6 + 7
for x in (0, 1):
    for y in (0, 1):
        for c in (0, 1):
            t = x + y + c
            assert t % 2 == x ^ y ^ c and t // 2 == (1 if x + y + c >= 2 else 0)
assert 0b0110 + 0b0111 == 0b1101 == 13
# 1.3: AND, XOR examples
assert 0b1100 & 0b1010 == 0b1000 and 0b1100 ^ 0b1010 == 0b0110
# 1.3, 1.4: binary, shifts, trailing zeros
assert 0b1101 == 13 and 13 >> 2 == 3 and 13 % 4 == 1 and 12 == 2**2 * 3
# 1.5: byte examples
assert (200 + 100) % 256 == 44 and (16 * 20) % 256 == 64
# 6.3: the inverse of 3 mod 2^8, and the inexact example
assert 3 * 171 % 256 == 1 and 7 * 171 % 256 == 173 and 1197 % 256 == 173
# 6.3: the 5-bit start is right mod 2^5 for every odd o, and not mod 2^6
assert all(o * ((3 * o) ^ 2) % 32 == 1 for o in range(1, 32, 2))
assert not all(o * ((3 * o) ^ 2) % 64 == 1 for o in range(1, 64, 2))
# 6.3: Newton's step doubles the correct bits, and the step counts
M = 2**64
rng = random.Random(1)
for _ in range(100000):
    o = rng.getrandbits(64) | 1
    x, k = 1, 1
    while k < 64:
        assert o * x % 2**k == 1
        x = x * (2 - o * x) % M
        k *= 2
    assert o * x % M == 1
    x = (3 * o) ^ 2
    for _ in range(4): x = x * (2 - o * x) % M
    assert o * x % M == 1
# 6.4: the table, its index, and three steps from it
T = [pow(2 * i + 1, -1, 256) for i in range(128)]
for _ in range(100000):
    o = rng.getrandbits(64) | 1
    i = (o >> 1) & 127
    assert i == ((o % 256) - 1) // 2 and o * T[i] % 256 == 1
    x = T[i]
    for _ in range(3): x = x * (2 - o * x) % M
    assert o * x % M == 1
# 6.4: the 16-bit table's size
assert 2**15 * 2 == 64 * 1024
print("every claim holds")
