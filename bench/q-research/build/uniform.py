"""The uniform header: every algebra uses the octonion header (4 unit bits: sign and a 3-bit axis; 4 slash bits)
over a U16 base word, and promotes by doubling the word and borrowing slash bits from its top. Then one decoder
serves every algebra, and a value of a smaller algebra is already a valid value of every larger one.

Compared here with the fitted design (family.layout with the algebra's own unit bits)."""
import family as FA

HEADER = 8
UNIT = 4            # sign + axis 0..7
SLASH = HEADER - UNIT
BASE = 16
WORDS = (16, 32, 64, 128, 256, 512, 1024, 2048)


def layout(word):
    """(unit bits, header slash bits, borrowed slash bits, data bits): the octonion layout at any word."""
    return FA.layout(8, word, HEADER)


def encode(unit, p, q, word, dim=8):
    """unit = (sign, axis) with axis < dim; the encoding does not depend on dim."""
    assert 0 <= unit[1] < dim
    return FA.encode(unit, p, q, 8, word, HEADER)


def decode(header, value, word):
    return FA.decode(header, value, 8, word, HEADER)


def valid_for(unit, dim):
    """A decoded unit belongs to an algebra of dimension dim when its axis is below dim."""
    return unit[1] < dim


def unused_unit_codes(dim):
    """Of the 16 unit codes, how many name no unit of this algebra (axes dim..7, either sign)."""
    return 2 * (8 - dim)


def compare():
    """Rows: algebra, word, fitted data bits, uniform data bits, difference, uniform's unused fraction of headers."""
    rows = []
    for name, (dim, canon) in FA.ALGEBRAS.items():
        for word in WORDS:
            fitted = FA.layout(dim, word, HEADER) if word >= canon else None
            _, _, bu, du = layout(word)
            rows.append(dict(algebra=name, dim=dim, word=word, size=HEADER + word,
                             fitted_data=fitted[3] if fitted else None,
                             fitted_borrowed=fitted[2] if fitted else None,
                             uniform_data=du, uniform_borrowed=bu,
                             cost_bits=(fitted[3] - du) if fitted else None,
                             unused_unit_codes=unused_unit_codes(dim)))
    return rows


def markdown(rows):
    out = ["| Algebra | Word | Size | Fitted: data bits (borrowed) | Uniform: data bits (borrowed) | Bits lost | "
           "Unit codes unused |", "|---|---|---|---|---|---|---|"]
    for r in rows:
        f = f"{r['fitted_data']} ({r['fitted_borrowed']})" if r["fitted_data"] is not None else "not a member"
        lost = r["cost_bits"] if r["cost_bits"] is not None else "—"
        out.append(f"| {r['algebra']} | U{r['word']} | {r['size']} | {f} | {r['uniform_data']} ({r['uniform_borrowed']}) "
                   f"| {lost} | {r['unused_unit_codes']} of 16 |")
    return "\n".join(out)


if __name__ == "__main__":
    print(markdown(compare()))
