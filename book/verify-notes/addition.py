# The addition routine of the notes (design A), line for line, on a 16-hour clock, against true sums.
M = 16
def add(m1, d1, m2, d2):
    t = d1 ^ d2
    if t != 0:
        if t & 1:                                   # different axes
            if m1 == 0: d1 = d2                     # a zero takes the other's direction ...
            elif m2 != 0: return [(m1, d1), (m2, d2)]   # expression
            t = 0                                   # ... and it is a same-direction addition
        else:                                       # opposite
            r = (m1 - m2) % M
            if m1 < m2: r = (0 - r) % M; d1 = d2
            return [(r, d1)]
    return [((m1 + m2) % M, d1)]                    # same direction
def value(m, d): return m * [1, 1j, -1, -1j][d]
n = 0
for m1 in range(8):
  for d1 in range(4):
    for m2 in range(8):
      for d2 in range(4):
        got = sum(value(m, d) for m, d in add(m1, d1, m2, d2))
        assert got == value(m1, d1) + value(m2, d2), (m1, d1, m2, d2); n += 1
print("zero-takes-the-other-direction version correct on", n, "pairs")
