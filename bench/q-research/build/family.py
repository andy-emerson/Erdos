"""The division-algebra family: unit tables, the one-byte header, and promotion by borrowed slash bits.

Units. The signed basis units of the reals, complex numbers, quaternions and octonions are +-e_x for
x < 1, 2, 4, 8. Their products come from the Cayley-Dickson construction, (a, b)(c, d) = (ac - d*b, da + bc*),
applied to basis elements, so each algebra's table is the restriction of the next one's: a unit code that names a
complex unit names the same quaternion and octonion unit.

Header. A unit code is a sign bit and an axis index (0 bits for the reals, 1 for complex, 2 for quaternions, 3 for
octonions). The one-byte member of each algebra puts the code in the top of the byte and the slash in the rest:
reals 1 + 7 (U128), complex 2 + 6 (U64), quaternions 3 + 5 (U32), octonions 4 + 4 (U16).

Promotion. A wider word W = 2^k needs k slash bits; the header holds h of them and the remaining k - h are borrowed
from the top of the word, so that, read as one bit string, the value is: unit code, header slash bits, borrowed
slash bits, numerator, denominator - 1. The numerator is the top s + 1 of the word's remaining data bits."""
from fractions import Fraction
from functools import lru_cache


# ---------------------------------------------------------------- unit tables by Cayley-Dickson

@lru_cache(maxsize=None)
def basis_mul(x, y, dim):
    """e_x e_y = sign * e_z in the algebra of dimension dim (1, 2, 4, 8); returns (sign, z)."""
    if dim == 1:
        return 1, 0
    h = dim // 2
    # e_x = (a, b) with a = e_x if x < h else 0, b = e_{x-h} if x >= h else 0; the same for e_y = (c, d)
    xa, ya = x < h, y < h
    if xa and ya:                      # (a, 0)(c, 0) = (ac, 0)
        return basis_mul(x, y, h)
    if xa and not ya:                  # (a, 0)(0, d) = (0, da)
        s, z = basis_mul(y - h, x, h)
        return s, z + h
    if not xa and ya:                  # (0, b)(c, 0) = (0, b c*)
        s, z = basis_mul(x - h, y, h)
        s *= conj_sign(y)
        return s, z + h
    s, z = basis_mul(y - h, x - h, h)  # (0, b)(0, d) = (-d* b, 0)
    return -s * conj_sign(y - h), z


def conj_sign(x):
    """e_x* = +e_0 for x = 0, -e_x otherwise."""
    return 1 if x == 0 else -1


def unit_mul(u, v, dim):
    """Signed units as (sign, axis): (s1 e_x)(s2 e_y) = s1 s2 sign e_z."""
    (s1, x), (s2, y) = u, v
    s, z = basis_mul(x, y, dim)
    return s1 * s2 * s, z


def units(dim):
    return [(s, x) for x in range(dim) for s in (1, -1)]


# ---------------------------------------------------------------- the header and promotion

ALGEBRAS = {"real": (1, 128), "complex": (2, 64), "quaternion": (4, 32), "octonion": (8, 16)}


def unit_bits(dim):
    return 1 + (dim.bit_length() - 1)


def layout(dim, word, header_bits=8):
    """(unit bits, slash bits in the header, slash bits borrowed from the word, data bits)."""
    U = unit_bits(dim)
    h = header_bits - U
    k = word.bit_length() - 1                      # a 2^k-bit word needs k slash bits
    b = max(0, k - h)
    return U, h, b, word - b


def fits(p, q, dim, word, header_bits=8):
    _, _, _, D = layout(dim, word, header_bits)
    return p.bit_length() + (q - 1).bit_length() <= D


def encode(unit, p, q, dim, word, header_bits=8):
    """(header, word value) of sign * e_axis * p/q in canonical form; None if it does not fit.
    unit = (sign, axis); p >= 0 and q >= 1 coprime; zero is (header 0, word 0)."""
    U, h, b, D = layout(dim, word, header_bits)
    if p == 0:
        return 0, 0
    if not fits(p, q, dim, word, header_bits):
        return None
    sign, axis = unit
    code = ((1 if sign < 0 else 0) << (U - 1)) | axis
    s = p.bit_length() - 1                          # slash: the numerator's top s + 1 data bits
    s_head, s_borrow = s >> b, s & ((1 << b) - 1)
    header = (code << h) | s_head
    data = (p << (D - (s + 1))) | (q - 1)
    return header, (s_borrow << D) | data


def decode(header, value, dim, word, header_bits=8):
    """Every pattern decodes: (unit, p, q); zero is ((1, 0), 0, 1)."""
    U, h, b, D = layout(dim, word, header_bits)
    code = header >> h
    sign = -1 if (code >> (U - 1)) & 1 else 1
    axis = code & ((1 << (U - 1)) - 1)
    s = ((header & ((1 << h) - 1)) << b) | (value >> D)
    data = value & ((1 << D) - 1)
    s = min(s, D - 1)                               # a slash past the data bits reads as the whole field
    width = D - (s + 1)
    p = data >> width
    q = (data & ((1 << width) - 1)) + 1
    if p == 0:
        return (1, 0), 0, 1
    return (sign, axis), p, q


def value_of(unit, p, q):
    """A signed unit times p/q as (sign * p/q, axis)."""
    return Fraction(unit[0] * p, q), unit[1]
