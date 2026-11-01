# The comparison pseudocode of the notes, line for line, against true values.
EQUAL, FIRST, SECOND, NOT_ORDERED = "equal", "first", "second", "not ordered"
def compare(m1, d1, m2, d2):
    t = m1 | m2
    if t == 0: return EQUAL                              # both zero
    if m1 == 0:                                          # first zero: the second's direction decides
        return SECOND if (d2 & 2) == 0 else FIRST
    if m2 == 0:                                          # second zero: the first's direction decides
        return FIRST if (d1 & 2) == 0 else SECOND
    t = d1 ^ d2
    if (t & 1) == 1: return NOT_ORDERED                  # different axes
    if t == 2:                                           # opposite: the positive one is larger
        return FIRST if (d1 & 2) == 0 else SECOND
    if m1 == m2: return EQUAL                            # same direction
    if m1 < m2:                                          # second magnitude larger
        return SECOND if (d1 & 2) == 0 else FIRST
    return FIRST if (d1 & 2) == 0 else SECOND            # first magnitude larger

def value(m, d): return m * [1, 1j, -1, -1j][d]
n = 0
for m1 in range(6):
  for d1 in range(4):
    for m2 in range(6):
      for d2 in range(4):
        a, b = value(m1, d1), value(m2, d2)
        got = compare(m1, d1, m2, d2)
        # reference: same axis (or a zero) -> order along that axis; else not ordered
        if a == b: want = EQUAL
        elif a.imag == 0 and b.imag == 0: want = FIRST if a.real > b.real else SECOND
        elif a.real == 0 and b.real == 0: want = FIRST if a.imag > b.imag else SECOND
        else: want = NOT_ORDERED
        assert got == want, (m1, d1, m2, d2, got, want)
        n += 1
# XOR of directions: 0 same, 2 opposite, odd = different axes
for d1 in range(4):
  for d2 in range(4):
    t = d1 ^ d2; u = (d1 - d2) % 4
    assert (t == 0) == (u == 0) and (t == 2) == (u == 2) and (t & 1) == (u & 1)
print("comparison pseudocode matches true values on", n, "pairs; XOR agrees with subtraction mod 4")
