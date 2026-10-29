# Note: two skeleton designs for rational classes

*A working note, not part of the paper. It replaces the earlier tripartite note: the same two designs, now described by the concrete structures a machine would store rather than as a graph. Both are proposals to be prototyped; neither is built yet.*

## Purpose and scope

Existing rational arithmetic handles two extremes well:
- **Same denominator:** add the numerators.
- **Nothing shared:** the general method, with its gcds.

Most real pairs of denominators lie between the two: they share *some* factors but not all. Both designs exploit that middle ground in two ways:
1. **Lookup.** What one pair's comparison discovered is kept, so later operations read it instead of recomputing it.
2. **One more link in the chain of gcds.** When a pair has never been compared, a common divisor already known for it shrinks the gcd that remains:

$$
\gcd(d_1, d_2) = c \cdot \gcd\!\left(\frac{d_1}{c},\ \frac{d_2}{c}\right) \quad \text{for any common divisor } c.
$$

**Scope.**
- The structures serve one **class** of rationals that the programmer opts in, such as amounts of money in one currency. Each class has its own structures, which work automatically and invisibly. When values from different classes meet, the system's ordinary rational arithmetic is used.
- All numbers have **one fixed width**. Different widths are a later question.
- Every stored value is in **lowest terms** and is **never changed** once stored. Results are stored as new values.

This note describes the **skeletons**: the plainest correct form of each design, the schoolbook version. Optimizations are listed separately, at the end, and are kept out of the baselines.

## Shared structures

Both designs start with the same three structures.

| # | Structure | Shape | Each entry stores |
| --- | --- | --- | --- |
| 1 | Numerators | list of $N$ | the numerator's value, and the **index** of its denominator |
| 2 | Denominators | list of $D$ | the denominator's value |
| 3 | Denominator lookup | table of $D$ entries | value → index, to find whether a denominator is already stored |

Structure 3 is needed because every result must find its denominator's index: an existing one, or a new one added at the end of structure 2.

An index is a small whole number, a position in a list. The same index $k$ means the same denominator in every structure. A matrix row "belongs" to a denominator because it shares that denominator's index; the values themselves live only in the lists.

**The fast path.** Two values whose denominator indices are equal add by integer addition: $\dfrac{m_x}{d} + \dfrac{m_y}{d} = \dfrac{m_x + m_y}{d}$, then reduce by $\gcd(m_x + m_y, d)$.

## Design 1: common divisors

### Structures

Four more, so seven in all.

| # | Structure | Shape | Each entry stores |
| --- | --- | --- | --- |
| 4 | Divisors | list of $C$, sorted by value; entry 0 is $1$ | a common divisor found so far |
| 5 | Pair table | upper triangle of $D \times D$ | the divisor index of the pair's gcd, or the marker "not yet" |
| 6 | Incidence | $D \times C$ bits | 1 if that divisor is known to divide that denominator |
| 7 | Residuals | one row per denominator, of varying length | the residuals $d/c$ (also called cofactors), one per set bit in the denominator's incidence row, in the same left-to-right order |

**Reading the structures.**
- *The gcd of a compared pair* $(i, j)$ is the divisor at column pair-table$[i, j]$. One lookup.
- *The residual* of denominator $i$ at column $c$ is entry number "count of set bits before column $c$ in row $i$" of residual row $i$. Counting set bits is a standard machine instruction (popcount).
- *The largest known common divisor* of an uncompared pair: combine the two incidence rows by bitwise AND, and take the highest set bit, meaning the one with the largest column number. Because the columns are sorted in increasing order, that is the largest value both are known to share. Column 0, divisor $1$, is always set, so the search always ends.

**Sizes.**
- A pair-table cell needs enough bits to name any of the $C$ columns or "not yet": $\lceil \log_2 (C + 1) \rceil$ bits.
- An incidence cell is 1 bit.
- Only the divisors, the residuals and the two value lists hold full-width numbers.

### Entering a new denominator

Append its value to structure 2, and add it to structure 3. Then:
- add a pair-table column of "not yet";
- add an incidence row with only column 0 set;
- add a residual row holding one entry, $d$ itself (its residual at divisor $1$). This duplicates structure 2, but keeps every row uniform.

Nothing is compared at entry.

### Recording a gcd

When $g = \gcd(d_i, d_j)$ has been computed:
1. Find $g$ in the sorted divisor list by binary search. If it is absent, insert it in sorted position. Every incidence row then gains a 0 bit at that position, and every pair-table entry pointing at a column at or after the inserted position is renumbered.
2. For each of $d_i$ and $d_j$: if its bit at $g$ is not yet set, set it, and insert $d/g$ into its residual row at the matching position.
3. Set pair-table$[i, j]$ to $g$'s column.

### Adding two values

```text
function add(m_x over d_i, m_y over d_j)
    if i is j then
        set sum to m_x + m_y
        set h to gcd(sum, d_i)
        return store((sum / h) over (d_i / h))
    end if
    set col to pairtable[i, j]
    if col is "not yet" then
        set c to the divisor at the highest set bit of (row i and row j)
        set g' to gcd(d_i / c, d_j / c)        using the stored residuals at c
        set g to c · g'
        record g for the pair i, j             as above
        set col to the column of g
    end if
    set g to divisor[col]
    set r_x to the residual of row i at col
    set r_y to the residual of row j at col
    set sum to m_x · r_y + m_y · r_x
    if g is 1 then
        set h to 1
    else
        set h to gcd(sum, g)
    end if
    return store((sum / h) over (r_x · (d_j / h)))
end function
```

The denominator $r_x \cdot (d_j / h)$ equals $g\, r_x r_y / h$, since $d_j = g\, r_y$; written this way it costs one multiplication, not two. `store` looks the result's denominator up in structure 3, enters it if it is new, and appends the numerator with that index.

### Example

Denominators $12$, $18$, $30$ at indices 0, 1, 2. At entry, the divisor list is $[1]$, each incidence row is $[1]$, the residual rows are $[12]$, $[18]$ and $[30]$, and the pair table is all "not yet".

- **$\tfrac{1}{12} + \tfrac{5}{18}$.** The pair (0, 1) is not yet compared. Rows 0 and 1 share only column 0, so $c = 1$, and $g' = \gcd(12, 18) = 6$, so $g = 6$. Divisor $6$ is inserted, giving the list $[1, 6]$. Rows 0 and 1 become $[1, 1]$, with residual rows $[12, 2]$ and $[18, 3]$, and pair (0, 1) points at column 1. The sum is $1 \cdot 3 + 5 \cdot 2 = 13$ over $6 \cdot 2 \cdot 3 = 36$, and $\gcd(13, 6) = 1$. Result: $13/36$.
- **$\tfrac{1}{12} + \tfrac{1}{30}$.** The pair (0, 2) is not yet compared, and $c = 1$, so $g = \gcd(12, 30) = 6$. Divisor $6$ is already listed. Row 2 becomes $[1, 1]$, with residual row $[30, 5]$, and pair (0, 2) points at column 1. The sum is $1 \cdot 5 + 1 \cdot 2 = 7$ over $60$. Result: $7/60$.
- **$\tfrac{1}{18} + \tfrac{1}{30}$: the chain at work.** The pair (1, 2) has never been compared, but both rows now have column 1 set, so $c = 6$. The remaining gcd runs on the residuals: $g' = \gcd(3, 5) = 1$, so $g = 6$. No gcd on the full denominators was needed. The sum is $1 \cdot 5 + 1 \cdot 3 = 8$ over $6 \cdot 3 \cdot 5 = 90$, and $h = \gcd(8, 6) = 2$. Result: $4/45$.

(The results' denominators, $36$, $60$ and $45$, enter as new rows; that is not shown.)

### Properties

- **Nothing stored is ever wrong.** Every entry is a fact about two unchanging denominators. Any entry can be checked: $c \cdot r = d$.
- **The second gcd remains.** The design knows $g$ but not its parts, so reduction needs $\gcd(\text{sum}, g)$.
- **Growth.** The pair table grows with $D^2/2$, in small cells. The incidence matrix grows with $D \cdot C$ bits.
- **Inserting a divisor in sorted order is the expensive moment.** Every incidence row shifts, and the pair table is renumbered.

## Design 2: the coprime base

The idea and its notation are explained in [note-coprime-base](note-coprime-base.md).

### Structures

Two more, so five in all.

| # | Structure | Shape | Each entry stores |
| --- | --- | --- | --- |
| 4 | Base | list of $C$ | a piece; no two pieces share a factor |
| 5 | Exponents | one row per denominator, of varying length | (piece index, $\nu_q(d)$) pairs, sorted by piece index, for the pieces $q$ that divide $d$ |

When a piece is split, the first new piece takes the old piece's index, and the others are appended to the end of the base. The affected rows are then re-sorted. The skeleton keeps no list of which rows use a piece: a split scans every row.

Here $\nu_q(d)$ is the exponent of the piece $q$ in $d$, and $d = \prod_q q^{\nu_q(d)}$ over the pairs in its row. Most pieces do not divide a given denominator, so each row lists only those that do.

There is no pair table and no residual list. For any two denominators $d_i$ and $d_j$, and for every piece $q$:
- **gcd:** $\nu_q(\gcd(d_i, d_j)) = \min(\nu_q(d_i), \nu_q(d_j))$;
- **residuals:** $\nu_q(r_i) = \nu_q(d_i) - \min(\nu_q(d_i), \nu_q(d_j))$, and likewise for $r_j$;
- **lcm:** $\nu_q(\text{lcm}(d_i, d_j)) = \max(\nu_q(d_i), \nu_q(d_j))$.

Each is computed by walking the two sorted rows together.

**Sizes.** The values $\nu_q(d)$ are small numbers, at most the width in bits. Only the base and the two value lists hold full-width numbers.

### Entering a new denominator

Append its value to structure 2, and add it to structure 3. Then split it over the base. This is *factor refinement*: gcds and divisions only, never factoring.

```text
function enter(d)
    set rest to d
    start a new, empty exponent row for d
    repeat
        find a piece q in the base with gcd(rest, q) not 1
        if there is none then
            leave the loop
        end if
        set g to gcd(rest, q)
        if g is q then
            divide rest by q as many times as it goes, counting k
            add k to ν_q in the new row
        else
            split q into pairwise coprime pieces built from g and q / g
            for each row, including the new row, that holds q with ν_q = t do
                for each new piece s, set ν_s of that row's denominator to t · ν_s(q)
            end for
        end if
    end repeat
    if rest is not 1 then
        append rest to the base as a new piece
        add (rest, 1) to the new row
    end if
end function
```

The loop runs until `rest` shares nothing with any piece. After dividing out $q$, the rest may still share part of $q$ (entering $12$ when $q = 6$ leaves $2$). The next pass then splits $q$. Splitting $q$ into pieces built from $g$ and $q/g$ is itself factor refinement on those two numbers (the explainer, §4).

Splitting a piece rewrites the exponent rows of the denominators that used it. It never touches a stored value.

### Adding two values

```text
function add(m_x over d_i, m_y over d_j)
    if i is j then
        set sum to m_x + m_y
        set h to gcd(sum, d_i)
        return store((sum / h) over (d_i / h))
    end if
    set r_x to 1, set r_y to 1, and set "equal" to empty
    walk rows i and j together; for each piece q with a = ν_q(d_i) and b = ν_q(d_j) do
        multiply r_x by q^(a − min(a, b))
        multiply r_y by q^(b − min(a, b))
        if a equals b and a is not 0 then
            add q^a to the list "equal"
        end if
    end walk
    set sum to m_x · r_y + m_y · r_x
    set den to d_i · r_y
    for each q in "equal" do
        set h to gcd(sum, q)
        if h is not 1 then
            divide sum by h, and den by h
        end if
    end for
    return store(sum over den)
end function
```

Only the pieces $q$ with $\nu_q(d_i) = \nu_q(d_j) > 0$ can cancel (the explainer, §6, gives the reason). When there are none, no gcd at all is computed.

### Example

The same denominators. A row is written $\{q{:}\nu_q(d), \ldots\}$. Entering them:
- $12$: the base is empty, so $12$ becomes a piece. The base is $[12]$, and row $12$ is $\{12{:}1\}$.
- $18$: $\gcd(18, 12) = 6$, which splits $12$ into $2$ and $3$, with $\nu_2(12) = 2$ and $\nu_3(12) = 1$. Row $12$ becomes $\{2{:}2, 3{:}1\}$. Then $\nu_2(18) = 1$ and $\nu_3(18) = 2$, so row $18$ is $\{2{:}1, 3{:}2\}$. The base is $[2, 3]$.
- $30$: $\nu_2(30) = 1$ and $\nu_3(30) = 1$, with $5$ left over, which becomes a new piece. Row $30$ is $\{2{:}1, 3{:}1, 5{:}1\}$, and the base is $[2, 3, 5]$.

Adding:
- **$\tfrac{1}{12} + \tfrac{5}{18}$.** $\nu_2$ is $2$ and $1$, and $\nu_3$ is $1$ and $2$. So $r_x = 2$, $r_y = 3$, and no piece has equal valuations. The sum is $1 \cdot 3 + 5 \cdot 2 = 13$ over $12 \cdot 3 = 36$, with no gcd needed. Result: $13/36$.
- **$\tfrac{1}{18} + \tfrac{1}{30}$.** $\nu_2$ is $1$ and $1$, which are equal, so $2^1$ goes on the list. $\nu_3$ is $2$ and $1$, and $\nu_5$ is $0$ and $1$. So $r_x = 3$ and $r_y = 5$. The sum is $1 \cdot 5 + 1 \cdot 3 = 8$ over $18 \cdot 5 = 90$. Checking only the piece $2$: $\gcd(8, 2) = 2$. Result: $4/45$.

### Properties

- **Exponent rows change when a piece splits, but are always checkable:** the product of $q^{\nu_q(d)}$ over a row must equal its denominator $d$.
- **No pair storage.** Nothing grows with $D^2$.
- **Entry is the expensive moment.** A new denominator is compared with every piece, and a split rewrites the rows that used that piece.
- **Each addition multiplies pieces** to form the residuals, where Design 1 reads stored ones.

## Side by side

| | Design 1: common divisors | Design 2: coprime base |
| --- | --- | --- |
| Structures | 7 | 5 |
| Full-width values stored | numerators, denominators, divisors, residuals | numerators, denominators, base pieces |
| Index or bit storage | pair table: $D^2/2$ cells of $\lceil \log_2(C+1) \rceil$ bits; incidence: $D \cdot C$ bits | exponent rows: one small (index, $\nu_q(d)$) pair per piece $q$ dividing each denominator $d$ |
| At entry of a denominator | append only; no gcds | a gcd with every piece; possible splits |
| Gcd of a pair | a lookup if compared; otherwise AND plus scan, then one gcd on residuals | always read from the rows; never computed per pair |
| Residuals | stored, read by popcount | multiplied out from pieces on each addition |
| Reduction | $\gcd(\text{sum}, g)$ on all of $g$ | only against pieces $q$ with $\nu_q(d_i) = \nu_q(d_j) > 0$, often none |
| Expensive moment | inserting a new divisor, which shifts incidence rows and renumbers the pair table | splitting a piece, which rewrites the rows that use it |
| Grows with | $D^2$ (small cells) and $D \cdot C$ (bits) | the pieces actually used by each denominator |

### Computational complexity

Notation as in the paper (v0.3, §1 and §4):
- $n$ is the size of the numbers in bits;
- $\gamma$ is the size of $g$;
- $u$ is the larger size of the residuals;
- $s$ is the size of the part shared at equal valuation, the product of $q^{\nu_q(d_i)}$ over the $e$ pieces $q$ with $\nu_q(d_i) = \nu_q(d_j) > 0$, and $s_i$ is the size of the $i$-th such factor $q^{\nu_q(d_i)}$;
- $\pi$ is the size of the largest piece;
- $\rho$ is the number of pieces the two rows use;
- $\lambda \le n$ is the sum of $\nu_q(d)$ over a denominator's pieces $q$.

Also:
- $D$ is the number of denominators. To avoid a clash, the paper's division cost $D(\cdot)$ is written $\text{Div}(\cdot)$ here;
- $C$ the number of divisors (Design 1) or pieces (Design 2);
- $E$ the total number of (piece, $\nu_q(d)$) entries over all rows;
- $\kappa$ the size of $c$.

Where a cell gives three forms, they are symbolic; schoolbook; fastest.

| | Design 1 | Design 2 |
| --- | --- | --- |
| Arithmetic per addition, $g$ and residuals in hand | $3M(n, u) + \text{Div}(2n, \gamma) + G(\gamma)$, plus exact divisions by $h$; $O(nu + n\gamma)$; $O(n \log u + n \log \gamma + \gamma \log^2 \gamma)$ | $3M(n, u) + O(n) + \sum_i \big(\text{Div}(n, s_i) + G(s_i)\big)$; zero reduction when $s = 0$; $O(nu + ns)$; $O(n \log u + e\, n \log s + s \log^2 s)$ |
| Structure per addition, pair compared | one pair-table read; two popcounts over $\lceil C/64 \rceil$ words | a merge of two rows, $O(\rho)$; products of pieces to form $r_x$ and $r_y$, $O\big(M(u) \log \rho\big)$ |
| Structure per addition, pair not compared | AND plus scan, $O(C/64)$; $G(n - \kappa)$ on residuals ($G(n)$ when $c = 1$); $M(\kappa, \gamma - \kappa)$ for $c \cdot g'$; recording, with a new divisor costing $O(D \cdot C/64 + D^2)$ to shift and renumber, plus up to two exact divisions for residuals | not applicable |
| Per new denominator | $O(D + C/64)$ to extend the structures; no gcds | up to $C$ gcds, each after a division, and $\lambda$ exact divisions: $C\,\big(\text{Div}(n, \pi) + G(\pi)\big) + \lambda\, \text{Div}(n, \pi)$; per split, a small refinement plus a scan of all rows, $O(E)$ |
| Memory, besides the values | $\tfrac{D(D-1)}{2} \lceil \log_2(C+1) \rceil + D \cdot C$ bits | about $2 \times$ (total pieces used by all rows) small numbers |

## The skeleton's boundary

Both baselines are built as written above, and the builders and reviewers are told the goal: **an accurate representation of the skeleton, not an optimized version for production.** Bugs and plain inefficiencies inside the skeleton are fixed. Nothing outside it is added.

**Inside the skeleton:**
- one fixed integer width, with schoolbook arithmetic;
- the same gcd and division routines in both designs and in the control;
- the stated layouts: index lists, the packed upper triangle, bit rows with AND, popcount and highest-set-bit;
- a plain value → index table for structure 3;
- sorted insertion with immediate shifting and renumbering (Design 1);
- testing every piece at entry (Design 2);
- lazy work only: nothing is computed until an operation needs it.

**Outside the skeleton** (possible later improvements):
- threading, GPUs and batching in phases;
- background (eager) comparisons;
- linking a new divisor to every other denominator it divides (Design 1);
- an index that limits which pieces a new denominator is tested against (Design 2);
- caching residual products (Design 2);
- deferred re-sorting of divisors (Design 1);
- p-adic methods, BLAS, fast multiplication.

## To be decided by testing

- **Design 1 against Design 2** at the baseline, against a control of plain reduced rationals using the gcd-first method. Workloads include:
  - all denominators shared, partly shared at several levels, and nothing shared;
  - few denominators against many;
  - denominators that settle early against ones that keep arriving.
- **Measured:** operation counts (gcds, divisions, multiplications, lookups), time and memory, with entry cost reported separately from cost per operation.
- **Afterwards:** which improvements each design gains the most from. A slower baseline may have more room to improve.
