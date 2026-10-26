# Complex columns

Both vector forms carry complex values. In the turn-byte layout each element has its own turn, so one vector can hold a mix of real and imaginary elements; in the signed layout one vector is one axis. `CxCol` is either one vector, or a real-part and an imaginary-part vector. Products of one-part columns add turns; sums widen to two parts when a real meets an imaginary value, and narrow back when no element has both parts. The same code serves both layouts.

```python {file=build/cxcol.py}
from q72 import CrossAxis


class CxCol:
    """Complex column over either layout. 'one': one column whose elements may be real or imaginary;
    'two': a real-part column and an imaginary-part column, both on the real axis."""

    __slots__ = ("kind", "a", "b")

    def __init__(self, kind, a, b=None):
        self.kind, self.a, self.b = kind, a, b

    @staticmethod
    def real(col):
        return CxCol("one", col)

    def __len__(self):
        return len(self.a)

    def widen(self):
        if self.kind == "two":
            return self
        return CxCol("two", self.a.real_part(), self.a.imag_part())

    def narrow(self):
        """Back to one part when no element has both parts nonzero."""
        if self.kind == "one":
            return self
        try:
            return CxCol("one", self.a + self.b.rotate(1))
        except CrossAxis:
            return self

    def rotate(self, k):
        k %= 4
        if self.kind == "one":
            return CxCol("one", self.a.rotate(k))
        re, im = self.a, self.b
        if k == 0:
            return self
        if k == 1:
            return CxCol("two", im.rotate(2), re)
        if k == 2:
            return CxCol("two", re.rotate(2), im.rotate(2))
        return CxCol("two", im, re.rotate(2))

    def __add__(self, o):
        if self.kind == "one" and o.kind == "one":
            try:
                return CxCol("one", self.a + o.a)
            except CrossAxis:
                pass
        x, y = self.widen(), o.widen()
        return CxCol("two", x.a + y.a, x.b + y.b).narrow()

    def __sub__(self, o):
        return self + o.rotate(2)

    def __mul__(self, o):
        if self.kind == "one" and o.kind == "one":
            return CxCol("one", self.a * o.a)
        x, y = self.widen(), o.widen()
        return CxCol("two", x.a * y.a - x.b * y.b, x.a * y.b + x.b * y.a).narrow()

    def abs2(self):
        if self.kind == "one":
            return self.a * self.a.conj()
        return self.a * self.a + self.b * self.b

    def total2(self):
        if self.kind == "one":
            return self.a.total2()
        return self.a.total(), self.b.total()

    def values(self):
        if self.kind == "one":
            return [v.parts() for v in self.a.values()]
        return [(r.fraction(), i.fraction()) for r, i in zip(self.a.values(), self.b.values())]
```
