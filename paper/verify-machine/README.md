# verify-machine

Checks for [the machine-level supplement](../supplement-machine.md).

- `division.s`: Appendix A's routines, as one file.
- `test.c`: every routine against the exact quotient, on 3 million random exact pairs each (divisors of 1 to 64 bits, about a
  quarter of them with everything below 2^32, some zero dividends); long division also on 3 million inexact pairs.
- `check.py`: the supplement's arithmetic claims: the 5-bit start, the table's entries, Newton's step doubling the
  correct bits, and the worked examples.

```
gcc -O2 test.c division.s -o test && ./test
python3 check.py
```
