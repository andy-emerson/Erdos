# Checks for Appendices A and B of v0.5

These checks cover Appendices A and B of the paper: [Appendix A](../v0.5.md#appendix-a-the-library-in-python), "The library in Python", and [Appendix B](../v0.5.md#appendix-b-the-section-3-library-in-python), "The section 3 library in Python". They are kept outside the paper. Run them from the repository root with one command:

```sh
python3 paper/verify/verify.py
```

The run takes about 30 seconds and ends with `ALL CHECKS PASSED`, or with `FAILED` and the failing checks; the exit status is 0 or 1. Use `--no-control` to skip the comparison with 10-interning's control, which then needs no compiler and no numpy. Use `--paper FILE` to check another copy of the paper. **Nothing is timed.**

Requirements: Python 3 (tested with 3.13). The control comparison also needs `gcc` and numpy, because 10-interning's own generator imports numpy.

## What is checked

1. **The code is the paper's.** `verify.py` reads the one ` ```python ` block under the heading "Appendix A. The library in Python" in `v0.5.md` and runs it. Nothing is copied, so if the paper changes, the checks run on the new code. The block must define `rational`, `numerator`, `denominator`, `mod`, `gcd`, `reduce`, `add`, `subtract`, `multiply` and `divide`, in that order, and nothing else.
2. **The translation is line for line.** Each function's last pseudocode definition before the appendix is its final form, and the paper's own pseudocode is parsed for it. That pseudocode is translated mechanically with the substitutions the appendix lists (`·` to `*`, `−` to `-`, `=` to `==` in conditions, `AND`/`OR`, `/` to `//`, `ERROR` to `raise ZeroDivisionError`, and `b′`, `s₊`, `k₁` and the rest to `b_prime`, `s_plus`, `k1` and the rest). The result must equal the appendix function as a syntax tree and in its number of lines. `rational`, `numerator` and `denominator` (section 1.2) are defined only in prose, so their three short forms are fixed in the harness. `mod` (section 2.3) has pseudocode and is checked like the other functions.
3. **Every function against `fractions.Fraction`.**
   - `mod` is checked on every pair with 0 ≤ u ≤ 60 and 1 ≤ v ≤ 30, and on random pairs: the result is in 0..v−1 and differs from u by a multiple of v.
   - `gcd` is checked against `math.gcd` on every pair in −30..30 except (0, 0), and on random and common-factor pairs up to 200 bits.
   - `reduce` is checked on every pair in −30..30 with a nonzero denominator, and on random pairs.
   - The four operations are checked on every pair of stored values with |numerator| ≤ 12 and denominator ≤ 12, which covers zero, ±1 and every sign combination. They are also checked on random values up to 200 bits, and on pairs built to hit each coincidence the special cases test for: integer operands (denominator 1), shared denominators, equal or opposite numerators, cross matches, reciprocals and common factors.
   - Every result must be a pair of integers with a positive denominator and gcd 1, and must equal the exact value.
   - Division by zero must raise `ZeroDivisionError` for every dividend tried.
   - No operation may call `reduce`, as section 2.9 says.
   - **Special cases are ordered by cost (section 1.6).** For every pair with |numerator| ≤ 15 and denominator ≤ 15 (any positive denominator for section 1's `add` and `multiply`, reduced values for section 2's), the case reached gives a result no dearer than any other case that holds, and among cases whose results cost the same it has the cheapest test. Each special case is a top-level `if` on a, b, c and d; the order is read from the paper's pseudocode, and the harness knows only each case's cost: multiplications in section 1, gcds in section 2, and a test against constants cheaper than a comparison of the operands.
4. **Branch coverage.** Every `if` of the appendix is traced (`sys.settrace`), both its true and its false outcome, and so is every term of each top-level `or`. Every outcome and every term must be reached, and every statement must run. The report gives the counts by line of the paper. It also lists the outcomes reached only by direct calls, never from inside the four operations. These are `gcd`'s `v < 0` (no operation passes `gcd` a negative second argument) and `reduce`'s `b < 0` and `g == 1` (no operation calls `reduce`).
5. **The worked computation of section 2.9** is reproduced exactly:
   - every call of `reduce`, `gcd`, `subtract`, `add`, `divide` and `multiply`, in order and nesting, with its arguments and result, including each step of every gcd chain;
   - the condition and outcome of every `if` each function evaluates, matching the paper's step tables;
   - the intermediate values (`g`, `b_prime`, `d_prime`, `t`, `h`, `p`, `q`, `k3`, …);
   - and that `reduce` and each operation run exactly once.
6. **Against 10-interning's textbook control (TC), correctness only** (`control.py`):
   - Round 2's own generator (`10-interning/test/r2_gen.py`) writes its small (toy) workloads for the eight jobs round 2 runs (1–4 and 8–11, 360 to 840 calls each) into a temporary directory.
   - TC is built there from `10-interning/lib/src/tc.c` with round 2's own compiler commands (`r2_libconf.build_commands`, counting build only). It is run once per job in its `count` mode, the untimed mode round 2's self-tests use.
   - The paper's library runs the same calls: an entry `p, q` is `reduce(rational(p, q))`, and each operation is applied to the stored values of earlier calls.
   - Every call's value must equal TC's read-out, integer for integer, and the generator's exact reference.
   - Nothing is written outside the temporary directory.

7. **Appendix B, the columns and rows of section 3** (`section3.py`, run by `verify.py`; Appendix A's checks above are unchanged):
   - The one ` ```python ` block under "Appendix B. The section 3 library in Python" is read and run on top of Appendix A's functions, which it uses and does not repeat. It must be the import of `namedtuple`, `empty = None`, the two kinds of pair `Column` and `Row`, and then define `prepend`, `first`, `rest`, `shared`, `entries`, `column`, `row`, `join`, `take`, `tally`, `sum_` and `product`, in that order.
   - Each function is the line-for-line translation of its pseudocode in sections 3.1 and 3.2, with Appendix A's substitutions, `ERROR` as `raise ValueError`, and `pair` as `Column` in section 3.1 and `Row` in section 3.2; `sum` is `sum_`, since `sum` is a Python built-in. `join` and `take` are defined for a column (3.1) and for a row (3.2), and each is one Python function: the column definition under `if isinstance(g, Column):`, then the row definition, with the column or row named `g`. The five list and pair primitives, defined in prose, have fixed short forms.
   - Against `fractions.Fraction`: joining every small stored value, and random ones, to columns over 1..60, 100, 360, 3600, 2^64 and random denominators (joined exactly when the denominator divides the column's, refused with `ValueError` otherwise, and the old column left unchanged); sums with signs and raw entries, of empty columns and of columns whose entries cancel (both 0/1), with one `reduce` at most, one `tally` call per entry plus one for the empty list, and a sum at most log2 N bits longer than its largest entry; taking out every entry in −200..200 over many denominators, reduced, with `reduce` skipped exactly for 0 and for D = 1; rows for every numerator in −12..12 and random ones (0 refused), joined only by values with that numerator, taken out with no gcd, and every product of two entries reduced with no gcd or `reduce`, equal to section 2.7's `multiply`, staying in the row only for the numerator 1; and raw entries placed directly on the caller's promise of section 3.2 (b > 0, and gcd(a, b) = 1 unless a = ±1), whose products are reduced with no gcd.
   - Every example and claim of section 3: 1/2 as 50/100, 1/3 and 2/5 refused, that a reduced a/b is u/D only when b divides D, the column of cents (4 additions and 1 gcd, against 4 + 2 + 2 + 2 = 10 gcds in section 2's library, both giving 13/5), 1/36, 9/20, the row of −1, 1/2 ÷ 1/4 = 2, and that scaling a denominator to a shared numerator breaks lowest terms unless c = a, which is no scaling at all (c = −a fails only on the sign).
   - Branch coverage of Appendix B: every `if` outcome, including both kinds in `join` and `take`, and every statement must be reached.

## Limits

- Python's integers are unbounded, so the fixed-width storage of section 1 (and TC's 64-bit overflow checks) is not modelled.
- `gcd` is recursive, as in the paper, so Python's default recursion limit (1000) bounds it. That allows inputs of several hundred bits even in the worst case, well beyond what is tested here.
- Outside the paper's interface, `rational(p, 0)` is not rejected (the paper requires q ≠ 0 and does not check it), and `gcd(0, 0)` returns 0. Neither is tested.
- `tally` is recursive, as in the paper, so Python's recursion limit bounds the length of a list it tallies; the columns tested have at most a few hundred entries.
