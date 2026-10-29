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
- All numbers have **one fixed width**: 64 bits in the baselines (see "Width").
- Every stored value is in **lowest terms** and is **never changed** once stored. Results are stored as new values.

This note describes the **skeletons**: the plainest correct form of each design, the schoolbook version. Optimizations are listed separately, at the end, and are kept out of the baselines.

## Shared structures

Both designs keep two lists:

| Structure | Shape | Each entry stores |
| --- | --- | --- |
| Numerators | list of $N$ | the numerator's value, and the **index** of its denominator |
| Denominators | list of $D$ | the denominator's value |

An index is a small whole number, a position in a list. The same index $k$ means the same denominator in every structure. A matrix row "belongs" to a denominator because it shares that denominator's index; the values themselves live only in the lists.

Every result must find its denominator's index: an existing one, or a new one added at the end of the list. In the concept, this is a search of the list, one entry at a time. In the implementation, a lookup table does it (described under Design 1's implementation form; Design 2 uses the same table).

**The fast path.** Two values whose denominator indices are equal add by integer addition: $\dfrac{m_x}{d} + \dfrac{m_y}{d} = \dfrac{m_x + m_y}{d}$, then reduce by $\gcd(m_x + m_y, d)$.

## Design 1: common divisors

Design 1 is described twice: first the **concept**, with as few structures as possible, then the **implementation form**, the baseline that will actually be built. The second changes how things are stored, not what is computed.

### The concept: five structures

| Structure | Shape | Each entry stores |
| --- | --- | --- |
| Numerators | list of $N$ | (value, denominator index) |
| Denominators | list of $D$ | the value; when a value is stored, the list is searched one entry at a time to see whether its denominator already exists |
| Divisors | list of $C$, sorted by value; entry 0 is $1$ | a common divisor found so far |
| Pair table | upper triangle of $D \times D$, **1 bit per cell** | 1 means "compared": this pair's gcd has been computed and recorded |
| Incidence matrix | $D \times C$ cells | empty, or the residual $d/c$ |

A filled cell in row $d$, column $c$ says "$c$ is known to divide $d$", and its content is the residual $d/c$. Every row has column 0 filled, with $d$ itself, since $1$ divides everything.

### The search, and why it is exact

To add two values over denominators $d_i$ and $d_j$, look at rows $i$ and $j$ of the incidence matrix together, and keep the columns where **both** cells are filled (an AND). Take the one with the largest column number. Its divisor $c$ is the largest known common divisor of $d_i$ and $d_j$, because the columns are sorted by value. Column 0 always qualifies, so the search always finds something.

- **If the pair bit is set, $c$ is the gcd.** *Proof:* every common divisor of $d_i$ and $d_j$ divides $\gcd(d_i, d_j)$, so none is larger. When the pair was compared, the gcd was recorded, which filled its column in both rows, and a cell is never emptied. So the gcd's column qualifies, and it is the largest that does.
- **If the pair bit is not set, $c$ starts the chain.** Both cells at $c$ are filled, so $c$ divides both, and their residuals are already stored:

$$
g = c \cdot \gcd\!\left(\frac{d_i}{c},\ \frac{d_j}{c}\right).
$$

  Then record $g$ (below) and set the pair bit.

### Recording a gcd

When $g = c \cdot g'$ has been found for the pair $(i, j)$:
1. Find $g$ in the sorted divisor list by binary search. If it is absent, insert it at its sorted position. Every row gains an empty cell at that column. Nothing else changes: the pair table holds no column numbers, so nothing is renumbered.
2. For each of rows $i$ and $j$: if its cell at $g$ is empty, fill it with $d/g$, which is the residual at $c$ divided by $g'$.
3. Set the pair bit $(i, j)$.

When $g' = 1$, $g$ is $c$ itself, already recorded in both rows: only step 3 is done, with no binary search. The residuals at $g$ are known at the end ($d/g$ from step 2, or the residuals at $c$), so the operation uses them without reading the cells again.

### Results: islands

A result's denominator, if it is new, starts as an **island**: no pair bit is set for it, and its row holds column 0 only, with the residual $d$ itself. A pair bit is set in only one place: when two **operand** denominators are compared inside an addition, a subtraction or a division. Multiplication searches the pair's rows but does not compare the pair, so it never sets the bit. So there is no "met but not compared" state. A set bit always means "compared, and the gcd is recorded in both rows". A new denominator stays an island until it is used as an operand.

The operation sometimes knows more: that $e = d_k \cdot \mu$ for an operand denominator $d_k$, with $\mu$ in hand, so every divisor known to divide $d_k$ divides $e$. The skeleton does not copy row $k$ into the new row: such a copy is not an output of the operation's arithmetic, and a production version would more likely keep a lazy link to row $k$ (see "Result reuse" under "The skeleton's boundary"). No divisor is inserted, no gcd is computed, and a denominator the lookup finds keeps its row unchanged.

### Adding two values

```text
function add(m_x over d_i, m_y over d_j)
    if i is j then
        set sum to m_x + m_y
        set h to gcd(sum, d_i)
        return store((sum / h) over (d_i / h))
    end if
    set col to the largest column filled in both row i and row j
    set r_x to cell[i, col]
    set r_y to cell[j, col]
    if pair[i, j] is not set then
        set c to divisor[col]
        set g' to gcd(r_x, r_y)                         the residuals at c
        if g' is not 1 then
            record g = c · g' for rows i and j
            set col to the column of g
            set r_x to r_x / g' and r_y to r_y / g'     the residuals at g, already known
        end if
        set pair[i, j]
    end if
    set g to divisor[col]
    set sum to m_x · r_y + m_y · r_x
    if g is 1 then
        set h to 1
    else
        set h to gcd(sum, g)
    end if
    return store((sum / h) over (r_x · (d_j / h)))
end function
```

The denominator $r_x \cdot (d_j / h)$ equals $g\, r_x r_y / h$, since $d_j = g\, r_y$; written this way it costs one multiplication, not two. `store` searches the denominator list for the result's denominator; if it is absent, appends it as a new island, with column 0 only; and appends the numerator with that index. A zero sum is stored as $0/1$.

### Example

Denominators $12$, $18$, $30$ at indices 0, 1, 2. At the start, the divisor list is $[1]$, the rows are $\{1{:}12\}$, $\{1{:}18\}$ and $\{1{:}30\}$ (written as divisor:residual, for the filled cells only), and no pair bit is set.

- **$\tfrac{1}{12} + \tfrac{5}{18}$.** Rows 0 and 1 share only column 0, so $c = 1$. The pair bit is not set, so $g' = \gcd(12, 18) = 6$ and $g = 6$. Divisor $6$ is inserted, giving the list $[1, 6]$. Row 0 becomes $\{1{:}12, 6{:}2\}$, row 1 becomes $\{1{:}18, 6{:}3\}$, and pair bit (0, 1) is set. The sum is $1 \cdot 3 + 5 \cdot 2 = 13$ over $2 \cdot 18 = 36$, and $\gcd(13, 6) = 1$. Result: $13/36$. Its denominator $36$ is new: index 3, an island with no pair bit, and row $\{1{:}36\}$.
- **$\tfrac{1}{12} + \tfrac{1}{30}$.** Row 0 has columns 0 and 1 filled, row 2 only column 0, so $c = 1$. The pair bit is not set, so $g = \gcd(12, 30) = 6$. Divisor $6$ is already listed. Row 2 becomes $\{1{:}30, 6{:}5\}$, and pair bit (0, 2) is set. The sum is $1 \cdot 5 + 1 \cdot 2 = 7$ over $2 \cdot 30 = 60$. Result: $7/60$ (index 4), an island $\{1{:}60\}$.
- **$\tfrac{1}{18} + \tfrac{1}{30}$: the chain at work.** The pair (1, 2) has never been compared, but both rows now have column 1 filled, so $c = 6$. The remaining gcd runs on the stored residuals: $g' = \gcd(3, 5) = 1$, so $g = 6$. No gcd on the full denominators was needed. Pair bit (1, 2) is set. The sum is $1 \cdot 5 + 1 \cdot 3 = 8$ over $3 \cdot 30 = 90$, and $h = \gcd(8, 6) = 2$. Result: $4/45$ (index 5), an island $\{1{:}45\}$.
- **A later $\tfrac{7}{12} + \tfrac{1}{18}$.** Pair bit (0, 1) is set, so the search's answer, $c = 6$, is the gcd. No gcd of denominators is computed. The sum is $7 \cdot 3 + 1 \cdot 2 = 23$ over $36$, and $\gcd(23, 6) = 1$. Result: $23/36$, whose denominator is found at index 3.

### The implementation form: the baseline to be built

Three plain engineering steps turn the concept into the baseline. None of them changes what is computed; they change how it is stored and found.

**1. A lookup table for denominators.** Searching the list one entry at a time costs up to $D$ checks for every stored result. The baseline adds a hash table: an array of slots, where a denominator's value picks a starting slot, and the search moves to the next slot until it finds the value or an empty slot (*linear probing*). The slots hold only denominator **indices**, never values; a check reads the value from the list.

*Example,* with 8 slots and slot = value mod 8:
- Store $\tfrac{1}{12}$, $\tfrac{5}{18}$ and $\tfrac{7}{30}$. Their denominators go to slots $12 \bmod 8 = 4$, $18 \bmod 8 = 2$ and $30 \bmod 8 = 6$, which hold indices 0, 1 and 2.
- Compute $\tfrac{13}{36}$. $36 \bmod 8 = 4$; slot 4 holds index 0, which is $12$, not $36$. Slot 5 is empty, so $36$ is new: it becomes index 3, and slot 5 holds 3.
- Reading a value goes numerator → list directly, without the lookup: index 3 is $36$.
- A later $\tfrac{7}{36}$ is found in two checks: slot 4 (index 0, $12$), then slot 5 (index 3, $36$).

Numerators point to the list, not to the lookup, because the lookup rearranges itself: when it gets half full, a larger table is made and every index is placed again, at new slots. The list never moves, so an index stays valid forever.

**2. Incidence as bits, plus a residual row.** Each incidence cell becomes 1 bit: set if $c$ is known to divide $d$. The residuals move to a **residual row** per denominator, holding them in the same left-to-right order as the row's set bits. The residual for column $c$ is at position "number of set bits before $c$", counted with one instruction per 64 columns (*popcount*).

*Example,* with divisors $[1, 4, 6, 15]$:
- $30$'s bits are 1 0 1 1 ($4$ does not divide $30$);
- its residual row is $[30, 5, 2]$;
- its residual at $15$ (column 3) is at position 2, since two bits are set before column 3: it is $2$.

Why: the search runs on every addition, and with bits it checks 64 columns per instruction, where the concept checks one cell at a time. And most denominators are divisible by only a few of the known divisors, so full-width cells would mostly be empty.

**3. The pair table, already 1 bit per cell,** is packed column by column, so that a new denominator appends its column at the end. Since it stores no column numbers, inserting a new divisor shifts only the incidence bit rows, from that column on; a divisor larger than all the others goes at the end, and nothing shifts. The residual rows keep their order, because the set bits keep theirs; only the two rows that gain the new divisor get one residual inserted. Nothing is renumbered.

So the implementation form has seven structures: numerators, denominators, the lookup, divisors, the pair table, incidence bits and residual rows.

### Properties

- **Nothing stored is ever wrong.** Every entry is a fact about two unchanging denominators, recorded by a comparison. Any entry can be checked: $c \cdot r = d$.
- **The second gcd remains.** The design knows $g$ but not its parts, so reduction needs $\gcd(\text{sum}, g)$.
- **Growth.** The pair table grows with $D^2/2$ bits. The incidence matrix grows with $D \cdot C$ bits, and the residual rows with the number of set bits.
- **Inserting a divisor is the expensive moment.** Every incidence bit row shifts by one bit from that column on.

## Design 2: the coprime base

The idea and its notation are explained in [note-coprime-base](note-coprime-base.md).

### Structures

Besides the two lists and the lookup, two more, so five in all.

Here $\nu_q(d)$ is the exponent of the piece $q$ in $d$, and $d = \prod_q q^{\nu_q(d)}$ over the pairs in its row. Most pieces do not divide a given denominator, so each row lists only those that do.

| Structure | Shape | Each entry stores |
| --- | --- | --- |
| Base | list of $C$ | a piece; no two pieces share a factor |
| Exponents | one row per denominator, of varying length | (piece index, $\nu_q(d)$) pairs, sorted by piece index, for the pieces $q$ that divide $d$ |

When a piece is split, the first new piece takes the old piece's index, and the others are appended to the end of the base. The affected rows are then re-sorted. The skeleton keeps no list of which rows use a piece: a split scans every row.

There is no pair table and no residual list. For any two denominators $d_i$ and $d_j$, and for every piece $q$:
- **gcd:** $\nu_q(\gcd(d_i, d_j)) = \min(\nu_q(d_i), \nu_q(d_j))$;
- **residuals:** $\nu_q(r_i) = \nu_q(d_i) - \min(\nu_q(d_i), \nu_q(d_j))$, and likewise for $r_j$;
- **lcm:** $\nu_q(\text{lcm}(d_i, d_j)) = \max(\nu_q(d_i), \nu_q(d_j))$.

Each is computed by walking the two sorted rows together.

**Sizes.** The values $\nu_q(d)$ are small numbers, at most the width in bits. Only the base and the two value lists hold full-width numbers.

### Entering a new denominator

Find it with the lookup; if it is new, append its value to the list. Then split it over the base. This is *factor refinement*: gcds and divisions only, never factoring. It is one scan of the base, from the first piece to the last.

```text
function enter(d)
    set rest to d
    start a new, empty exponent row for d
    set p to 0                                           the scan position
    while rest is not 1 and p is below C do              C grows when a split appends pieces
        set q to piece p
        set g to gcd(rest, q)
        if g is 1 then
            set p to p + 1
        else if g is q then
            divide rest by q as many times as it goes, counting k
            add k to ν_q in the new row
        else
            split q into pairwise coprime pieces built from g and q / g;
                the first takes q's index p, the others go at the end of the base
            for each row, including the new row, that holds the entry (q, t) do
                replace the entry (q, t) by the entries (s, t · ν_s(q)) for each new piece s
                re-sort that row by piece index
            end for
        end if
    end while
    if rest is not 1 then
        append rest to the base as a new piece
        add (rest, 1) to the new row
    end if
end function
```

The scan never goes back. After dividing out $q$, it stays at $q$, since the rest may still share part of $q$: entering $12$ when $q = 6$ leaves $2$, and the next test splits $6$. After a split it stays at $p$, which now holds the first new piece; the other new pieces are at the end of the base, and the scan reaches them later. The pieces before $p$ need no second test: each was coprime to the rest when tested, and the rest only loses factors. The scan stops as soon as the rest is $1$, and the gcd it finds is the one it uses. Splitting $q$ into pieces built from $g$ and $q/g$ is itself factor refinement on those two numbers (the explainer, §4).

Splitting a piece rewrites the exponent rows of the denominators that used it. It never touches a stored value.

### Adding two values

```text
function add(m_x over d_i, m_y over d_j)
    if i is j then
        set sum to m_x + m_y
        set h to gcd(sum, d_i)
        return store((sum / h) over (d_i / h))
    end if
    set r_x to 1, set r_y to 1, set "equal" to empty, R to empty and L to empty
    walk rows i and j together; for each piece q with a = ν_q(d_i) and b = ν_q(d_j) do
        multiply r_x by q^(a − min(a, b))
        multiply r_y by q^(b − min(a, b))
        if a equals b then
            add (q, a, q^a) to the list "equal"         a is not 0: the walk visits only pieces of a row
        else
            add (q, max(a, b)) to R                       the result's valuation, already known
        end if
    end walk
    set sum to m_x · r_y + m_y · r_x
    set den to d_i · r_y
    for each (q, a, p) in "equal" do                     p = q^a, formed once in the walk
        set h to gcd(sum, p)
        if h is not 1 then
            divide sum by h, and den by h
        end if
        add (q, a, p, h) to L                            the power test waits for the lookup
    end for
    return store(sum over den), with R and L
end function

function finish(R, L)                                    only for a new denominator, after the lookup
    set u to 1
    for each (q, a, p, h) in L do
        if h is 1 then
            add (q, a) to R                              h = q^0: no division
        else if h is q^k for some k then                 divide h by q while the remainder is 0
            add (q, a − k) to R if a − k > 0
        else
            multiply u by p / h                          a leftover: q is composite
        end if
    end for
    return (R, u)
end function
```

Only the pieces $q$ with $\nu_q(d_i) = \nu_q(d_j) > 0$ can cancel (the explainer, §6, gives the reason). When there are none, no gcd at all is computed. A zero sum is stored as $0/1$, as in Design 1. $R$ and $L$ carry what the walk knows about the result's denominator (next section). Whether each cancellation $h$ is a power of its piece matters only for building a new row, so `store` looks the denominator up first, and runs `finish` only if it is new; $h = 1$ is $q^0$ and costs no division.

### Results: reusing known valuations

**What the operation knows.** A new result's denominator must be entered over the base, but most of its valuations are already known from the walk that computed it:
- **addition and subtraction:** $\nu_q(e) = \max(\nu_q(d_i), \nu_q(d_j))$ where the two differ, and $\nu_q(d_i) - k$ where they are equal and the cancellation is $q^k$;
- **multiplication:** the two rows added, less the cancellation; with equal indices, row $i$ doubled;
- **division:** $\nu_q(d_i) - \min(\nu_q(d_i), \nu_q(d_j))$ for the part $d_i / g_2$; only $|m_y| / g_1$, from the old numerator, is unknown.

The one thing the walk cannot know is what remains of a composite piece $q$ when its cancellation $h$ is not a power of $q$: the **leftover** $q^a / h$. (Base $[6]$: $\gcd(2, 6) = 2$ is not a power of $6$, and the leftover is $3$.)

**In the implementation,** the operation passes the known valuations as a row $R$ and the unknown part as a number $u$ (the product of the leftovers, or $|m_y|/g_1$ in division, or $1$). In addition and multiplication, the power tests that complete $R$ and give $u$ wait until the lookup has failed to find the denominator (`finish` above). A new denominator is entered by the scan above, started from $R$ with `rest` set to $u$. When $u = 1$ no gcd is computed at all. When the lookup finds the denominator, $R$ and $u$ are discarded. This reuse is a direct output of the operation's own arithmetic, which a production coprime-base implementation would keep; Design 1 has no counterpart, and its new rows hold column 0 only.

*Example:* $\tfrac{1}{12} + \tfrac{5}{18}$ over the base $(2, 3)$. The walk sees $\nu_2 = 2, 1$ and $\nu_3 = 1, 2$, unequal at both, so $R = \{2{:}2, 3{:}2\}$ and $u = 1$: the row of $36$ is $R$, with no gcd. In $\tfrac{1}{18} + \tfrac{1}{30}$, piece $2$ has equal valuations $1, 1$ and cancels by $h = 2 = 2^1$, so $R = \{3{:}2, 5{:}1\}$ and $u = 1$: the row of $45$.

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

## Subtraction, multiplication and division

Throughout, $x = m_x/d_i$ and $y = m_y/d_j$ are stored values in lowest terms. The **control** is plain reduced rationals with the usual methods.

### Subtraction

$x - y = x + (-y)$: negate $m_y$ and add. Negating changes no denominator, so the structures are used exactly as for addition, in both designs.

*Example:* $\tfrac{1}{12} - \tfrac{5}{18}$ takes the same steps as $\tfrac{1}{12} + \tfrac{5}{18}$, with $-5$: the sum is $1 \cdot 3 + (-5) \cdot 2 = -7$, over $36$. Result: $-7/36$.

### Multiplication

**The control.** The product in lowest terms needs two *cross* gcds, each between a numerator and the other value's denominator:

$$
\frac{m_x}{d_i} \cdot \frac{m_y}{d_j} = \frac{(m_x / g_1)(m_y / g_2)}{(d_i / g_2)(d_j / g_1)}, \qquad g_1 = \gcd(m_x, d_j), \quad g_2 = \gcd(m_y, d_i).
$$

No further gcd is needed. Numerators are not part of the structures, so neither design can look $g_1$ or $g_2$ up. What the structures can do is make the second argument of each gcd smaller, using one fact:

> $m_x$ shares no factor with $d_i$ (lowest terms), so it shares none with any divisor of $d_i$. Hence for every common divisor $c$ of $d_i$ and $d_j$: $\gcd(m_x, d_j) = \gcd(m_x, d_j / c)$. Likewise $\gcd(m_y, d_i) = \gcd(m_y, d_i / c)$.

**Design 1.** The search gives the largest known common divisor $c$ and both residuals at $c$, without comparing the pair (the pair bit is neither needed nor set). Then $g_1 = \gcd(m_x, d_j/c)$ and $g_2 = \gcd(m_y, d_i/c)$. This is a modest saving: when $c = 1$ nothing is saved, and in every case each gcd still starts by dividing a full-width numerator. The result's denominator $(d_i/g_2)(d_j/g_1)$ is looked up and, if new, enters as an island (see "Results: islands").

**Design 2.** The cross gcds are taken piece by piece, because the powers $q^{\nu_q(d)}$ in a row share no factor with one another:
- at a piece $q$ in **both** rows, nothing can cancel (by the fact above), and $\nu_q$ of the result is $\nu_q(d_i) + \nu_q(d_j)$;
- at a piece only in $d_j$, compute $\gcd(m_x, q^{\nu_q(d_j)})$; at a piece only in $d_i$, compute $\gcd(m_y, q^{\nu_q(d_i)})$. If the answer is $q^k$, $\nu_q$ of the result is the valuation minus $k$.

So the result's valuations are the two rows added, minus the cancellation, and the result needs only the lookup, not refinement. One exception: when a piece is composite, its gcd with a numerator need not be a power of it. Then the result's denominator splits that piece: the valuations known at every other piece are kept, and only the leftover is refined. (Base $[6]$, from denominators $6$ and $36$: in $\tfrac{2}{1} \cdot \tfrac{1}{6}$, $\gcd(2, 6) = 2$ is not a power of $6$; the result is $\tfrac{1}{3}$, the leftover is $3$, and refining it splits $6$ into $2$ and $3$.) The per-piece gcds for $g_1$ together have the size of one gcd against $u_j = \prod q^{\nu_q(d_j)}$ over the pieces $q$ in row $j$ only (and $u_i$ likewise for $g_2$). $u_j$ divides $d_j/g$, where $g = \gcd(d_i, d_j)$, and is smaller than it whenever a piece in both rows has a larger valuation in $d_j$. But there is one division of the numerator per piece; the saving is mainly that shared pieces are skipped.

**Same denominator.** When $i$ is $j$, both cross gcds are $1$ ($\gcd(m_x, d_i) = 1$ by lowest terms), so $\tfrac{m_x}{d} \cdot \tfrac{m_y}{d} = \tfrac{m_x m_y}{d^2}$ with no gcd at all. This is true of plain rationals too; in the designs, equal indices detect it with one comparison. The denominator $d^2$ is still looked up (in Design 1, if new, it is an island; in Design 2 its valuations are doubled, so no refinement), and it overflows the width sooner.

**A zero factor.** If $x = 0$, stored as $0/1$, then $g_1 = \gcd(0, d_j) = d_j$ and $g_2 = \gcd(m_y, 1) = 1$, so the formula gives $0/1$ with no special case (and likewise for $y = 0$).

*Examples,* after the additions above:
- $\tfrac{5}{18} \cdot \tfrac{7}{30}$. Control: $g_1 = \gcd(5, 30) = 5$, $g_2 = \gcd(7, 18) = 1$. Design 1: $c = 6$, with residuals $3$ and $5$, so $g_1 = \gcd(5, 5) = 5$ and $g_2 = \gcd(7, 3) = 1$. Design 2: pieces $2$ and $3$ are in both rows and are skipped, giving $\nu_2 = 1 + 1 = 2$ and $\nu_3 = 2 + 1 = 3$; piece $5$ is only in $30$, and $\gcd(5, 5^1) = 5$, so $\nu_5 = 1 - 1 = 0$. Result: $\tfrac{7}{2^2 \cdot 3^3} = \tfrac{7}{108}$.
- $\tfrac{5}{12} \cdot \tfrac{7}{12} = \tfrac{35}{144}$, with no gcd.

### Division

$x / y = x \cdot (1/y)$, and $y = 0$ is an error. The reciprocal of $m_y/d_j$ is $d_j / m_y$, with the sign of $m_y$ moved to the numerator: $\pm d_j / |m_y|$. The cross gcds of this product are:
- $g_1 = \gcd(m_x, |m_y|)$, between two numerators: neither design helps;
- $g_2 = \gcd(d_i, d_j)$, between two denominators: exactly what the designs are for. Design 1 finds it by the search, a lookup if the pair bit is set, otherwise by the chain, then records it and sets the bit (two operand denominators were compared). Design 2 reads it from the valuations, as the min at each piece.

The result is $\pm (m_x/g_1)(d_j/g_2)$ over $(d_i/g_2)(|m_y|/g_1)$. Its denominator contains part of the old numerator, which the structures have never seen, so it must be entered: looked up in Design 1 (an island if new), and looked up and, if new, refined in Design 2. In Design 2 only $|m_y|/g_1$ is a new number to refine: the valuations of $d_i/g_2$ are already known, $\nu_q(d_i) - \min(\nu_q(d_i), \nu_q(d_j))$ at each piece. This is where division costs Design 2 more than multiplication does.

**A zero dividend.** If $x = 0$, stored as $0/1$, then $g_1 = \gcd(0, |m_y|) = |m_y|$ and $g_2 = 1$, so the result is $0/1$ with no special case.

When $i$ is $j$, $g_2 = d_i$ and the denominators cancel completely: the result is $m_x/m_y$ reduced by $\gcd(m_x, m_y)$, with the sign moved to the numerator.

*Example:* $\tfrac{5}{12} \div \left(-\tfrac{7}{18}\right) = -\tfrac{5 \cdot 18}{12 \cdot 7}$. Here $g_1 = \gcd(5, 7) = 1$, and $g_2 = \gcd(12, 18) = 6$: Design 1 reads it from the search, since pair bit (0, 1) is set; Design 2 reads it from $\min$ at pieces $2$ and $3$. Result: $-\tfrac{5 \cdot 3}{2 \cdot 7} = -\tfrac{15}{14}$. The denominator $14$ is new. In Design 1 it is an island, $\{1{:}14\}$. Design 2 reuses what its walk knows: rows $\{2{:}2, 3{:}1\}$ and $\{2{:}1, 3{:}2\}$ give $\nu_2 = 2 - 1 = 1$ and $\nu_3 = 1 - 1 = 0$ for $12/6 = 2$, so $R = \{2{:}1\}$, and only $u = |m_y|/g_1 = 7$ is new. Refining $7$ over the base $[2, 3, 5]$ finds it coprime to every piece, so $7$ becomes a new piece, and the row of $14$ is $\{2{:}1, 7{:}1\}$.

## Width

The baselines use 64-bit integers. The width is a single build setting, so a 32-bit run is a rebuild, not a rewrite. Overflow is detected in every operation (a product, a sum, or a result's denominator that no longer fits). A trial that overflows is reported, with where it overflowed, and excluded from the comparisons.

## Memory per rational

Each rational stores only its numerator and an index. Everything else belongs to a *denominator*, and is shared by every rational that uses it. So the cost per rational is

$$
(\text{numerator} + \text{index}) + \frac{\text{everything else}}{N}.
$$

With 8-byte values, 4-byte indices, a lookup of $S$ slots (a power of two, at most half full), and $k$ entries per row on average (divisors of $d$ in Design 1, counting $1$; pieces of $d$ in Design 2), in bytes:
- **Plain rationals:** $16N$.
- **A single shared denominator**, as FLINT does for rational polynomials (for vectors it is done by hand): $8N$ plus one denominator. This holds only while the common denominator, the lcm of all of them, fits the width.
- **Design 1:** $12N + 8D + 4S + 8C + 8kD$ (values, lookup, divisors, residual rows), plus $D \cdot \lceil C/64 \rceil \cdot 8$ (incidence bit rows, padded to whole words), plus $D(D-1)/16$ (pair table).
- **Design 2:** $12N + 8D + 4S + 8C + 8kD$ (values, lookup, base, exponent rows at 8 bytes per (index, $\nu_q(d)$) entry).

**Case A:** $N = 1{,}000{,}000$ rationals over $D = 1{,}000$ denominators, $C = 500$, $k = 5$, $S = 2{,}048$.

| | Bytes per rational | Of which, structures |
| --- | --- | --- |
| Plain | 16 | none |
| One shared denominator | about 8, if the lcm fits | none |
| Design 1 | about 12.19 | 0.19: pair table 62 kB, incidence 64 kB, residuals 40 kB, the rest 20 kB |
| Design 2 | about 12.06 | 0.06: exponent rows 40 kB, the rest 20 kB |

With many rationals per denominator, both designs cost about 12 bytes: 25% less than plain rationals.

**Case B:** each rational has its own denominator: $N = D = 10{,}000$, again with $C = 500$ and $k = 5$, and $S = 32{,}768$.
- **Design 1:** the pair table alone is $D(D-1)/2$ bits, about $6.25$ MB, which is $625$ bytes per rational. (The full $D \times D$ square would be $12.5$ MB; the upper triangle is half of it.) With incidence ($64$), residuals ($40$), lookup ($13$), list ($8$) and the numerator and index ($12$), the total is about $760$ bytes per rational.
- **Design 2:** about $74$ bytes per rational: $12 + 8 + 13 + 40$, plus under 1 byte of base.

Both are worse than plain rationals here, Design 1 by far. Bytes per rational is therefore a **required test metric**.

## Side by side

| | Design 1: common divisors | Design 2: coprime base |
| --- | --- | --- |
| Structures | 7 (5 in the concept) | 5 |
| Full-width values stored | numerators, denominators, divisors, residuals | numerators, denominators, base pieces |
| Index or bit storage | lookup: $S$ index slots; pair table: $D^2/2$ bits; incidence: $D \cdot C$ bits | lookup: $S$ index slots; exponent rows: one small (index, $\nu_q(d)$) pair per piece $q$ dividing each denominator $d$ |
| At entry of a denominator | a lookup, then append; no gcds | a lookup, then a gcd with every piece in one scan; possible splits |
| A new result denominator | an island, column 0 only; no gcds | the known valuations, with only the unknown part scanned; no gcds when nothing is unknown |
| Gcd of a pair | always AND plus scan; that is the gcd if the pair bit is set, otherwise one gcd on residuals, then recorded | always read from the rows; never computed per pair |
| Residuals | stored, read by popcount | multiplied out from pieces on each addition |
| Reduction after adding | $\gcd(\text{sum}, g)$ on all of $g$ | only against pieces $q$ with $\nu_q(d_i) = \nu_q(d_j) > 0$, often none |
| Cross gcds in multiplication | against residuals at the largest known common divisor | per piece, only at pieces in one row |
| Expensive moment | inserting a new divisor, which shifts every incidence bit row | splitting a piece, which rewrites the rows that use it |
| Grows with | $D^2/2$ bits and $D \cdot C$ bits | the pieces actually used by each denominator |
| Bytes per rational, Case A / Case B | about 12.2 / 760 | about 12.1 / 74 |

### Computational complexity

Notation as in the paper (v0.3, §1 and §4):
- $n$ is the size of the numbers in bits;
- $\gamma$ is the size of $g$;
- $u$ is the larger size of the residuals;
- $s$ is the size of the part shared at equal valuation, the product of $q^{\nu_q(d_i)}$ over the $e$ pieces $q$ with $\nu_q(d_i) = \nu_q(d_j) > 0$, and $s_i$ is the size of the $i$-th such factor $q^{\nu_q(d_i)}$;
- $\pi$ is the size of the largest piece;
- $\rho$ is the number of pieces the two rows use;
- $\lambda \le n$ is the sum of $\nu_q(d)$ over a denominator's pieces $q$, and $\tau \le \lambda$ the number of those pieces.

Also:
- $D$ is the number of denominators. To avoid a clash, the paper's division cost $D(\cdot)$ is written $\text{Div}(\cdot)$ here;
- $C$ the number of divisors (Design 1) or pieces (Design 2);
- $E$ the total number of (piece, $\nu_q(d)$) entries over all rows;
- $\kappa$ the size of $c$;
- $S$ the number of lookup slots, and $k$ the average number of entries per row.

Where a cell gives three forms, they are symbolic; schoolbook; fastest.

| | Design 1 | Design 2 |
| --- | --- | --- |
| Arithmetic per addition, $g$ and residuals in hand | $3M(n, u) + \text{Div}(2n, \gamma) + G(\gamma)$, plus exact divisions by $h$; $O(nu + n\gamma)$; $O(n \log u + n \log \gamma + \gamma \log^2 \gamma)$ | $3M(n, u) + O(n) + \sum_i \big(\text{Div}(n, s_i) + G(s_i)\big)$; zero reduction when $s = 0$; $O(nu + ns)$; $O(n \log u + e\, n \log s + s \log^2 s)$ |
| Structure per addition, every pair | AND plus scan, $O(C/64)$; two popcounts, $O(C/64)$ | a merge of two rows, $O(\rho)$; products of pieces to form $r_x$ and $r_y$, $O\big(M(u) \log \rho\big)$ |
| Extra, pair not yet compared | $G(n - \kappa)$ on residuals ($G(n)$ when $c = 1$); $M(\kappa, \gamma - \kappa)$ for $c \cdot g'$; up to two exact divisions for residuals; a binary search, $O(\log C)$; a new divisor costs $O(D \cdot C/64)$ to shift the incidence rows, with nothing renumbered | not applicable |
| Per new denominator | a lookup, expected $O(1)$ checks; $O(C/64 + D/64)$ to add an incidence row and a pair-table column; results likewise; no gcds | the same lookup; one scan of the base, at most $C + \tau$ gcds plus one per piece created by a split, since after dividing by a piece the scan tests it a second time; and $\lambda + \tau$ divisions, $\lambda$ that succeed and one that fails per piece divided: $(C + \tau)\big(\text{Div}(n, \pi) + G(\pi)\big) + (\lambda + \tau)\, \text{Div}(n, \pi)$; for a result, only the unknown part $u$ is scanned, and none when $u = 1$, after power tests on the tested pieces, made only for a new denominator; per split, a small refinement plus a scan of all rows, $O(E)$ |
| Memory, besides the values | $\tfrac{D(D-1)}{2} + D \cdot C$ bits, plus $kD$ full-width residuals and $S$ index slots | about $2E$ small numbers, plus $S$ index slots |

## Prepared for cluster maintenance

Clusters are a later step, not part of the skeleton. The skeleton already keeps what they will need in Design 1: the pair table's set bits are exactly the edges between denominators that met in an operation, so a later check of whether a cluster has split can OR rows of bits together to find what each denominator is connected to. Design 2 keeps no pair record, so it would later need a sparse "met" record per denominator.

## The skeleton's boundary

Both baselines are built as written above, and the builders and reviewers are told the goal: **an accurate representation of the skeleton, not an optimized version for production.** Bugs and plain inefficiencies inside the skeleton are fixed. Nothing outside it is added.

**Plain inefficiencies removed** (the spec, §15.1), all reflected above:
- Design 2's entry is **one scan** of the base: it stays at a piece after dividing by it or splitting it, never restarts at the first piece, and stops as soon as the rest is $1$;
- the gcd that the scan finds is the one used, not computed again;
- each power $q^a$ of a piece of equal valuation is formed once in the walk and kept for the gcd;
- in addition and multiplication, the test of whether a cancellation is a power of its piece is made after the lookup, and only for a new denominator; a cancellation of $1$ costs no division;
- in Design 1, a comparison hands back the residuals at the gcd, which it already knows, so the addition does not read them again;
- when $g' = 1$, Design 1 sets the pair bit at once, with no binary search;
- a divisor inserted at the end of the list shifts no row.

**Result reuse: the author's ruling** (Oct 28, 2026; the spec, §15.2). Each design uses only the reuse its own structure gives for free, the way a production implementation of that design would naturally do it. **Design 2 keeps its reuse:** the valuations of a result's denominator come from the operation's own arithmetic, and only the unknown part is refined. **Design 1 has none:** an eager copy of an operand's row into the result's row, residuals times $\mu$, is not a direct output of its arithmetic, and a production version would more likely be lazy; so a Design 1 result enters with column 0 only, residual $d$. Two later improvements are deferred, outside the baseline: Design 1's lazy parent link, and Design 2's deferred entry (below). Tests of known divisors against a result's denominator are left out in both designs.

**Inside the skeleton:**
- one fixed integer width, 64 bits, set in one place, with overflow detected; schoolbook arithmetic;
- the same gcd and division routines in both designs and in the control;
- the denominator lookup: a hash table of indices, with linear probing, growing when half full;
- Design 1's 1-bit pair table, packed column by column; 1-bit incidence rows with AND, highest set bit and popcount; residual rows;
- sorted insertion of divisors with immediate shifting (Design 1);
- testing every piece at entry (Design 2);
- result reuse in Design 2 only, as ruled above: known valuations, refining only the unknown part; Design 1's results enter with column 0 only;
- lazy work only: nothing is computed until an operation needs it.

**Outside the skeleton** (possible later improvements):
- threading, GPUs and batching in phases;
- background (eager) comparisons;
- linking a new divisor to every other denominator it divides (Design 1);
- recording about a result's denominator more than the ruling allows: copying an operand's row (Design 1), divisibility tests, new divisors from the operation's quantities, or facts added to an existing denominator's row (either design);
- Design 1's **lazy parent link** (deferred): a result row records a parent $k$ and a factor $\mu$ with $e = d_k \cdot \mu$, and the search consults the parent's row on demand;
- Design 2's **deferred entry** (deferred): a result's denominator is stored bare, and its row refined only when it is first used;
- an index that limits which pieces a new denominator is tested against (Design 2);
- caching residual products (Design 2);
- deferred re-sorting of divisors (Design 1);
- cluster maintenance;
- p-adic methods, BLAS, fast multiplication.

## To be decided by testing

- **Design 1 against Design 2** at the baseline, against a control of plain reduced rationals using the gcd-first method. Workloads include:
  - all denominators shared, partly shared at several levels, and nothing shared;
  - few denominators against many, including one denominator per rational (Case B);
  - denominators that settle early against ones that keep arriving;
  - a mix of all four operations, not only addition.
- **A variant of Design 1: the index-valued pair table.** Each cell names the column of the pair's gcd, or "not yet", instead of holding 1 bit. A compared pair then skips the search, though reading its residuals by popcount still costs $O(C/64)$. Its cells need $\lceil \log_2(C+1) \rceil$ bits, a width unknown in advance that grows with $C$, and inserting a divisor renumbers every cell pointing at or after it, $O(D^2)$.
- **Measured:** operation counts (gcds, divisions, multiplications, lookups), time, and **bytes per rational** (required), with entry cost reported separately from cost per operation, and overflowed trials counted.
- **Afterwards:** which improvements each design gains the most from. A slower baseline may have more room to improve.
