# Note: two skeleton designs for rational classes

*The joint note, as it stood for the measured run (formerly `paper/note-skeletons.md`). Since Oct 29, 2026, each design's part is also in its own folder: [Design 1](../../../08-common-divisors/research/design-note.md) and [Design 2](../../../09-coprime-base/research/design-note.md). This text keeps the side-by-side comparison. Only its links were updated.*

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

**2. Sparse incidence rows** (*revised Oct 29, 2026, approved by the author*). Each denominator's row stores only the cells that are filled: a list of entries (divisor id, residual), one per divisor known to divide $d$, **sorted by divisor value**. Divisor 1 is stored explicitly, as the first entry of every row, with residual $d$.
- **Stable ids.** A new divisor takes the next id, $C$, and keeps it for good. A second list holds the ids sorted by value, for the binary search. Inserting a divisor moves only that list: no row is shifted or renumbered.
- **The search** walks the two rows together, from their largest divisor down. If the two entries name the same divisor, it stops: that divisor is $c$, and both residuals are in the entries where it stopped. Otherwise it steps down in the row whose divisor is larger. Divisor 1 ends every walk. Each step is one scan.
- **Recording $g$** in a row inserts one entry. Since $g > c$, its place is above the entry where the walk stopped, and a binary search finds it.

*Example,* with divisors found in the order $1, 6, 15, 4$ (ids 0 to 3), so the ids sorted by value are $[0, 3, 1, 2]$, for $[1, 4, 6, 15]$:
- $30$'s row is $\{1{:}30, 6{:}5, 15{:}2\}$. $4$ does not divide $30$, so it has no entry.
- $12$'s row is $\{1{:}12, 4{:}3, 6{:}2\}$.
- The search for $12$ and $30$ starts at $6$ and $15$. $15$ is larger, so it steps down in $30$'s row to $6$. Both now name $6$, so it stops: $c = 6$, with residuals $2$ and $5$, after two steps.
- When $4$ was found, after $15$, no row changed: $4$ took id 3, and only the sorted list of ids moved.

Why: most denominators are divisible by only a few of the known divisors, so most cells are empty. A dense row of bits costs $C$ bits whatever is known; a sparse row costs 12 bytes per fact (a residual and a 4-byte id), and the search walks only the entries of the two rows.

**3. The pair table becomes a set of the pairs that met** (*revised Oct 29, 2026, approved by the author*). The concept's table has a bit for every pair, $D(D-1)/2$ bits, though most pairs never meet. The baseline instead stores only the compared pairs, in a hash set like the lookup.
- **Keys.** The pair $(i, j)$ with $i < j$ is the 64-bit key $j \cdot 2^{32} + i$. No key is 0, so 0 marks an empty slot.
- **Probing.** A key picks a starting slot by the lookup's hash, and the search moves on until it finds the key or an empty slot.
- **Growth.** The set is empty, with no table, until the first pair is compared; it then has 16 slots. It doubles, placing every key again, before it would pass half full.
- **Meaning.** It is the table's: a pair is inserted exactly when the table would have set its bit, and "is the pair bit set" becomes "is $(i, j)$ in the set". A new denominator adds nothing.

Its memory is proportional to the number of compared pairs, $P$: 8 bytes per slot, between $16P$ and $32P$ bytes once it holds a pair. The dense table is smaller only when more than about one pair in 256 of all $D(D-1)/2$ has been compared.

Neither the set nor the rows store a divisor's position by value, so inserting a new divisor shifts nothing. Only the two rows that gain the new divisor get one entry inserted. Nothing is renumbered.

*Example,* continuing Design 1's example. After $\tfrac{1}{12} + \tfrac{5}{18}$, the set holds one key, $(0, 1)$, in a table of 16 slots. After the other two additions it holds $(0, 2)$ and $(1, 2)$ as well. A later $\tfrac{7}{12} + \tfrac{1}{18}$ hashes $(0, 1)$, finds it, and so reads the search's answer as the gcd.

So the implementation form has six structures: numerators, denominators, the lookup, divisors, the pair set and the sparse incidence rows.

### Properties

- **Nothing stored is ever wrong.** Every entry is a fact about two unchanging denominators, recorded by a comparison. Any entry can be checked: $c \cdot r = d$.
- **The second gcd remains.** The design knows $g$ but not its parts, so reduction needs $\gcd(\text{sum}, g)$.
- **Growth.** The pair set grows with the number of compared pairs (in the concept, the pair table with $D^2/2$ bits). The incidence rows grow with the facts recorded, one entry per divisor known to divide a denominator (in the dense bit form, $D \cdot C$ bits).
- **Inserting a divisor shifts nothing.** It moves only the list of ids sorted by value, and adds one entry to each of the two rows that gain it. (In the dense bit form, every row shifted by one bit from that column on.)

## Design 2: the coprime base

The idea and its notation are explained in [coprime-base-notation](../../../09-coprime-base/research/coprime-base-notation.md). Like Design 1, Design 2 is described twice: first the **concept**, then the **implementation form**, the baseline that is built.

*Revised Oct 29, 2026, approved by the author:* Design 2 now refines **on demand, and only locally**. The first form refined every new denominator against the whole base as soon as it was stored, which cost about $C$ gcds per new denominator (see "Why on demand", below).

### The concept: a known part and a remainder

Besides the two lists and the lookup, three more structures, so six in all.

| Structure | Shape | Each entry stores |
| --- | --- | --- |
| Base | list of $C$ | a piece; no two pieces share a factor |
| Exponents | one row per denominator, of varying length | (piece index, $\nu_q$) pairs, sorted by piece index: the **known part** of $d$ |
| Remainders | list of $D$ | the **remainder** $u$ of $d$: the part not yet needed |

Every denominator is split into two parts, $d = K \cdot u$:
- the **known part** $K = \prod_q q^{\nu_q(K)}$, over the pairs in its row;
- the **remainder** $u$, which no operation has needed yet.

A newly entered denominator is all remainder: its row is empty and $u = d$. The remainder may share factors with any piece, even one of its own row, so $\nu_q(K)$ equals $\nu_q(d)$ only where $u$ is coprime to $q$.

**Principle: no work in advance, no work in the background; each operation computes only what it needs.**
- **Entry** looks the denominator up and, if it is new, appends it with an empty row and $u = d$. It computes no gcd against the base.
- **Two denominators meet** when an addition, a subtraction or a division uses them. Each remainder is then refined against the other row's pieces and against the other remainder. Whenever this pulls a piece into one row from its remainder, the other remainder is tested against that piece too, repeating until nothing new is added. The meeting ends with:
  - **(M1)** $u_i$ coprime to every piece of row $j$;
  - **(M2)** $u_j$ coprime to every piece of row $i$;
  - **(M3)** $\gcd(u_i, u_j) = 1$.
- **Multiplication** needs no meeting. It cancels against each piece, then against $u$.
- **The base stays pairwise coprime.** When a piece splits, the first new piece takes the old piece's index, and the others are appended to the end of the base. The rows that use the piece are rewritten. The skeleton keeps no list of which rows use a piece: a split scans every row.

There is no pair table and no residual list. After a meeting, for every piece $q$:
- **gcd:** $\nu_q(\gcd(d_i, d_j)) = \min(\nu_q(K_i), \nu_q(K_j))$;
- **residuals:** $r_i = u_i \prod_q q^{\nu_q(K_i) - \min(\nu_q(K_i), \nu_q(K_j))}$, and likewise for $r_j$.

Each is computed by walking the two sorted rows together.

**Why the meeting is enough.** Take a prime $p$ that divides both denominators. It cannot divide $u_i$: by M3 it would not divide $u_j$, so it would divide a piece of row $j$, which M1 rules out. Likewise it cannot divide $u_j$. So $p$ lies in a piece of each row, and since no two pieces share a factor, it is the same piece $q$, held by both rows. So the shared part of $d_i$ and $d_j$ is made of shared pieces, at their known valuations, and the remainders and the unshared pieces are coprime to the other denominator.

**Sizes.** The values $\nu_q$ are small numbers, at most the width in bits. Only the base, the remainders and the two value lists hold full-width numbers.

### Meeting two denominators

```text
function meet(i, j)                                      i is not j
    if u_i is 1 and u_j is 1 then
        return
    end if
    set queue A, the pieces to test u_i against, to the pieces of row j
    set queue B, the pieces to test u_j against, to the pieces of row i
    repeat
        while a queue has a piece not yet taken do
            take the next piece q, from A while A has one, otherwise from B
            reduce that side's remainder against q, unless it is 1
        end while
        if u_i is 1 or u_j is 1 then
            return
        end if
        set g to gcd(u_i, u_j)
        if g is 1 then
            return
        end if
        absorb(g)
    end repeat
end function

function reduce(side k, q)                               refine u_k against the piece q
    while u_k is not 1 do
        set h to gcd(u_k, q)
        if h is 1 then
            return
        else if h is q then
            divide u_k by q as many times as it goes, counting n
            add n to ν_q in row k
            if row k did not hold q before then
                add q to the other side's queue          the extra refinement
            end if
        else
            split q into pairwise coprime pieces built from h and q / h;
                the first takes q's index, the others go at the end of the base
            rewrite every row that holds q, and add the new pieces to every queue that holds q
        end if
    end while
end function

function absorb(g)                                       the remainders share g: make it pieces
    set rest to g
    set p to 0                                           the scan position
    while rest is not 1 and p is below C do              C grows when a split appends pieces
        set q to piece p
        set h to gcd(rest, q)
        if h is 1 then
            set p to p + 1
        else if h is q then
            divide rest by q as many times as it goes
            add q to both queues
        else
            split q as in reduce                         the scan stays at p
        end if
    end while
    if rest is not 1 then
        append rest to the base as a new piece, and add it to both queues
    end if
end function
```

**The extra refinement.** $u_j$ is first tested against the pieces row $i$ held when the meeting began. But a meeting can pull a piece into row $i$ from $u_i$, and $u_j$ must then be tested against that piece as well; likewise the other way round. Without this step, the meeting can end with M1 or M2 false. The counterexample is in the example below.

`reduce` stays at $q$ after dividing by it, since the remainder may still share part of $q$: $u = 12$ against $q = 6$ leaves $2$, which then splits $6$. After a split it stays at $q$'s index, which now holds the first new piece. `absorb` is one scan of the base, which never goes back: it stays at a piece after dividing by it or splitting it, and stops as soon as the rest is $1$. Splitting $q$ into pieces built from $h$ and $q/h$ is itself factor refinement on those two numbers (the explainer, §4): gcds and divisions only, never factoring. Splitting a piece rewrites the rows that use it; it never touches a stored value.

### Adding two values

```text
function add(m_x over d_i, m_y over d_j)
    if i is j then
        set sum to m_x + m_y
        set h to gcd(sum, d_i)
        return store((sum / h) over (d_i / h)), with R empty and u = d_i / h
    end if
    meet(i, j)
    set r_x to u_i, set r_y to u_j, set "equal" to empty, R to empty and L to empty
    walk rows i and j together; for each piece q with a = ν_q(K_i) and b = ν_q(K_j) do
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
    return store(sum over den), with R, L and u = u_i · u_j
end function

function finish(R, L, u)                                 only for a new denominator, after the lookup
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

Only the pieces $q$ with $\nu_q(K_i) = \nu_q(K_j) > 0$ can cancel (the explainer, §6, gives the reason; the remainders cannot, by M1–M3). When there are none, no gcd is computed after the meeting. A zero sum is stored as $0/1$, as in Design 1. Whether each cancellation $h$ is a power of its piece matters only for building a new row, so `store` looks the denominator up first, and runs `finish` only if it is new; $h = 1$ is $q^0$ and costs no division.

### Results: reusing known valuations

**What the operation knows.** A new result's denominator is stored with the valuations the operation already computed, as its known part, and everything else in its remainder. Nothing is refined when it is stored.
- **Addition and subtraction:** $\max(\nu_q(K_i), \nu_q(K_j))$ where the two differ, and $\nu_q(K_i) - k$ where they are equal and the cancellation is $q^k$. The remainder is $u_i u_j$, times the leftovers.
- **Multiplication:** the two rows added, less the cancellation, with row $i$ doubled for equal indices. The remainder is the two remainders less their cancellations, times the leftovers.
- **Division:** the known part of $d_i$ less the gcd, $\nu_q(K_i) - \min(\nu_q(K_i), \nu_q(K_j))$. The remainder is $u_i \cdot |m_y| / g_1$, which includes the part of the old numerator.

A **leftover** is what remains of a composite piece $q$ when its cancellation $h$ is not a power of $q$: $q^a / h$. (Base $[6]$: $\gcd(2, 6) = 2$ is not a power of $6$, and the leftover is $3$.) It joins the remainder unrefined, and the piece is split only if that remainder later meets a row holding it.

Every product of remainders here divides the result's denominator, which the operation has already formed and checked, so it cannot overflow. When the lookup finds the denominator, $R$, $L$ and $u$ are discarded, and its existing row and remainder are kept unchanged. This reuse is a direct output of the operation's own arithmetic, which a production coprime-base implementation would keep. Design 1 has no counterpart: its new rows hold one entry only, (divisor 1, $d$).

### Example

The same denominators. A row is written $\{q{:}\nu_q(K), \ldots\}$ with its remainder after it.

**Entering** $12$, $18$ and $30$ computes no gcd against the base. The base is empty, and the three rows are $\{\}, u = 12$; $\{\}, u = 18$; and $\{\}, u = 30$.

**$\tfrac{1}{12} + \tfrac{5}{18}$.** First the meeting:
- Both rows are empty, so both queues are empty, and $\gcd(12, 18) = 6$.
- `absorb(6)` finds the base empty and appends $6$ as a piece, giving the base $[6]$.
- Reducing $u_{12} = 12$ against $6$ divides once, leaving $u_{12} = 2$ and row $12$ $\{6{:}1\}$. It stays at $6$: $\gcd(2, 6) = 2$ splits $6$ into $2$ and $3$. Row $12$ becomes $\{2{:}1, 3{:}1\}$, and the base $[2, 3]$. Still at index 0, now $2$, it divides out the last $2$: row $12$ is $\{2{:}2, 3{:}1\}$, $u_{12} = 1$.
- Reducing $u_{18}$ against $2$ and then $3$ gives row $18$ $\{2{:}1, 3{:}2\}$, $u_{18} = 1$.

Then the walk:
- $\nu_2$ is $2$ and $1$, and $\nu_3$ is $1$ and $2$. So $r_x = 2$, $r_y = 3$, and no piece has equal valuations.
- The sum is $1 \cdot 3 + 5 \cdot 2 = 13$ over $12 \cdot 3 = 36$, with no gcd needed.

Result: $13/36$. Its row is $R = \{2{:}2, 3{:}2\}$, with $u = 1$. Row $30$ has met nothing and is untouched: $\{\}, u = 30$.

**$\tfrac{1}{18} + \tfrac{1}{30}$.** In the meeting, $u_{18} = 1$, so only $u_{30}$ is refined, against row $18$'s pieces $2$ and $3$. That leaves row $30$ $\{2{:}1, 3{:}1\}$, $u_{30} = 5$. The meeting ends there, since $u_{18} = 1$, so $5$ never becomes a piece: nothing yet needs it.

The walk:
- $\nu_2$ is $1$ and $1$, which are equal, so $2^1$ goes on the list.
- $\nu_3$ is $2$ and $1$. So $r_x = 1 \cdot 3 = 3$ and $r_y = 5$.
- The sum is $1 \cdot 5 + 1 \cdot 3 = 8$ over $18 \cdot 5 = 90$. Checking only the piece $2$: $\gcd(8, 2) = 2$.

Result: $4/45$. Its row is $R = \{3{:}2\}$ with $u = u_{18} u_{30} = 5$. The base is still $[2, 3]$.

**The extra refinement, and why it is needed.** Three steps produce a remainder that shares its own row's piece:
1. $\tfrac{1}{2} + \tfrac{1}{6}$. The meeting absorbs $\gcd(2, 6) = 2$ as the piece $2$, so row $2$ is $\{2{:}1\}, u = 1$ and row $6$ is $\{2{:}1\}, u = 3$. Result: $2/3$, stored as $\{\}, u = 3$.
2. $\tfrac{1}{2} \div \tfrac{2}{3}$. Here $g_1 = 1$. In the meeting, $u_3 = 3$ is coprime to $2$, so nothing changes. The walk gives $R = \{2{:}1\}$, and the remainder is $u_2 \cdot 2 = 2$. Result: $3/4$, stored as $\{2{:}1\}, u = 2$.
3. $\tfrac{3}{4} + \tfrac{1}{8}$, where $8$ is entered as $\{\}, u = 8$.
   - Queue A, for $u_4$, holds row $8$'s pieces: none.
   - Queue B, for $u_8$, holds row $4$'s piece $2$. Reducing $u_8$ against $2$ gives row $8$ $\{2{:}3\}$, $u_8 = 1$.
   - Row $8$ has gained $2$, so the extra refinement puts $2$ on queue A. Reducing $u_4 = 2$ against it gives row $4$ $\{2{:}2\}$, $u_4 = 1$.
   - The walk gives $r_x = 2$ and $r_y = 1$. The sum is $3 \cdot 2 + 1 \cdot 1 = 7$, over $4 \cdot 2 = 8$.

   Result: $7/8$.

Without the extra refinement, $u_4 = 2$ is never tested against $2$, so M1 fails. The walk then misses that $2$ divides both denominators fully. It takes $r_x = u_4 \cdot 2^0 = 2$ and $r_y = 2^{3-1} = 4$, and stores $3 \cdot 4 + 1 \cdot 2 = 14$ over $4 \cdot 4 = 16$: $14/16$, not in lowest terms. The prototype found this case, and the unit tests keep it.

### The implementation form: the baseline to be built

The concept is built as it stands. These details are fixed for the build; they change how the work is ordered, not what is computed.

1. **The queues** of a meeting hold piece indices, first in, first out. Queue A is emptied before each piece is taken from queue B, and a queue never holds a piece twice. A split adds its new pieces to every queue that holds the split piece, whether that piece has been taken yet or not. A remainder of 1 is not tested.
2. **The remainders' gcd** is taken only when both queues are empty and both remainders are above 1. After an `absorb`, the queues run again.
3. **No record of meetings.** A second meeting of the same pair tests the remainders again, and finds nothing new. A record of met pairs would be a new structure, which the skeleton does not have.
4. **`store`** receives $R$, $u$, and, from addition, subtraction and multiplication with different indices, the list $L$. It runs `finish` only for a new denominator, after the lookup, and refines nothing.
5. **The remainders** are one full-width word per denominator, a sixth structure.

### Properties

- **Rows change, but are always checkable:** the product of $q^{\nu_q(K)}$ over a row, times its remainder, must equal its denominator. Pieces stay pairwise coprime.
- **No pair storage.** Nothing grows with $D^2$.
- **Entry is cheap**: a lookup and an append, with no gcd against the base.
- **A meeting costs** one gcd per piece of the other row tested against a non-unit remainder, plus a retest after each division or split, plus the gcd of the two remainders. Only when the remainders share a factor does it cost a scan of the base (`absorb`), and a split rewrites the rows that use the piece.
- **Each addition multiplies pieces** to form the residuals, where Design 1 reads stored ones.

### Why on demand

The first form refined each new denominator, entered or a result, against the whole base as soon as it was stored. Whenever part of the denominator was new to the base, that meant one scan of every piece, about $C$ gcds.

In the pilot, the base grew to 2,400–3,200 pieces, mostly private parts of single denominators that never met a partner. Design 2 spent 4–11 million gcds per mixed job, against the control's 31–47 thousand. That is 58–161 times the control's gcd work, and its time was 55–137 times the control's.

On demand, the same jobs take 31–164 thousand gcds, with bases of 3–64 pieces. The finding is recorded in [eager-d2-finding.md](../../../09-coprime-base/research/eager-d2-finding.md).

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

- then the **remainders**: $\gcd(m_y, u_i)$ and $\gcd(m_x, u_j)$, with the numerators already divided by the per-piece gcds. Taken one factor at a time like this, with the numerator divided as it goes, the gcds give $\gcd(m_y, d_i)$ exactly, even where $u_i$ shares a factor with a piece of its own row.

No meeting is needed, and no row changes. So the result's known part is the two rows added, minus the cancellation, and its remainder is the two remainders less theirs. The result needs only the lookup.

When a piece is composite, its gcd with a numerator need not be a power of it, and the **leftover** joins the result's remainder. (Base $[6]$, from the meeting of $6$ and $36$: in $\tfrac{2}{1} \cdot \tfrac{1}{6}$, $\gcd(2, 6) = 2$ is not a power of $6$. The result is $\tfrac{1}{3}$, stored with an empty row and $u = 3$, and $6$ is not split. A later $\tfrac{1}{3} + \tfrac{1}{6}$ meets row $6$ and splits $6$ into $2$ and $3$.) The per-piece gcds for $g_1$ together have the size of one gcd against $u_j = \prod q^{\nu_q(d_j)}$ over the pieces $q$ in row $j$ only (and $u_i$ likewise for $g_2$). $u_j$ divides $d_j/g$, where $g = \gcd(d_i, d_j)$, and is smaller than it whenever a piece in both rows has a larger valuation in $d_j$. But there is one division of the numerator per piece; the saving is mainly that shared pieces are skipped.

**Same denominator.** When $i$ is $j$, both cross gcds are $1$ ($\gcd(m_x, d_i) = 1$ by lowest terms), so $\tfrac{m_x}{d} \cdot \tfrac{m_y}{d} = \tfrac{m_x m_y}{d^2}$ with no gcd at all. This is true of plain rationals too; in the designs, equal indices detect it with one comparison. The denominator $d^2$ is still looked up (in Design 1, if new, it is an island; in Design 2 its valuations are doubled and its remainder squared, so no refinement), and it overflows the width sooner.

**A zero factor.** If $x = 0$, stored as $0/1$, then $g_1 = \gcd(0, d_j) = d_j$ and $g_2 = \gcd(m_y, 1) = 1$, so the formula gives $0/1$ with no special case (and likewise for $y = 0$).

*Examples,* after the additions above:
- $\tfrac{5}{18} \cdot \tfrac{7}{30}$. Control: $g_1 = \gcd(5, 30) = 5$, $g_2 = \gcd(7, 18) = 1$. Design 1: $c = 6$, with residuals $3$ and $5$, so $g_1 = \gcd(5, 5) = 5$ and $g_2 = \gcd(7, 3) = 1$. Design 2, after the additions above: pieces $2$ and $3$ are in both rows and are skipped, giving $\nu_2 = 1 + 1 = 2$ and $\nu_3 = 2 + 1 = 3$. Row $30$'s remainder is $5$, and $\gcd(5, 5) = 5$, so the result's remainder is $1$. Result: $\tfrac{7}{2^2 \cdot 3^3} = \tfrac{7}{108}$.
- $\tfrac{5}{12} \cdot \tfrac{7}{12} = \tfrac{35}{144}$, with no gcd.

### Division

$x / y = x \cdot (1/y)$, and $y = 0$ is an error. The reciprocal of $m_y/d_j$ is $d_j / m_y$, with the sign of $m_y$ moved to the numerator: $\pm d_j / |m_y|$. The cross gcds of this product are:
- $g_1 = \gcd(m_x, |m_y|)$, between two numerators: neither design helps;
- $g_2 = \gcd(d_i, d_j)$, between two denominators: exactly what the designs are for. Design 1 finds it by the search, a lookup if the pair bit is set, otherwise by the chain, then records it and sets the bit (two operand denominators were compared). Design 2 meets the two rows, then reads it from the valuations, as the min at each piece.

The result is $\pm (m_x/g_1)(d_j/g_2)$ over $(d_i/g_2)(|m_y|/g_1)$. Its denominator contains part of the old numerator, which the structures have never seen, so it must be entered: looked up in Design 1 (an island if new), and looked up in Design 2, where a new one is stored with the known valuations of $d_i/g_2$, $\nu_q(K_i) - \min(\nu_q(K_i), \nu_q(K_j))$ at each piece, and the remainder $u_i \cdot |m_y|/g_1$, unrefined. Division, like addition, costs Design 2 a meeting.

**A zero dividend.** If $x = 0$, stored as $0/1$, then $g_1 = \gcd(0, |m_y|) = |m_y|$ and $g_2 = 1$, so the result is $0/1$ with no special case.

When $i$ is $j$, $g_2 = d_i$ and the denominators cancel completely: the result is $m_x/m_y$ reduced by $\gcd(m_x, m_y)$, with the sign moved to the numerator.

*Example:* $\tfrac{5}{12} \div \left(-\tfrac{7}{18}\right) = -\tfrac{5 \cdot 18}{12 \cdot 7}$. Here $g_1 = \gcd(5, 7) = 1$, and $g_2 = \gcd(12, 18) = 6$: Design 1 reads it from the search, since pair bit (0, 1) is set; Design 2 reads it from $\min$ at pieces $2$ and $3$. Result: $-\tfrac{5 \cdot 3}{2 \cdot 7} = -\tfrac{15}{14}$. The denominator $14$ is new. In Design 1 it is an island, $\{1{:}14\}$. Design 2 reuses what its walk knows. Rows $12$ and $18$ already met, so both remainders are $1$ and the meeting computes nothing. The rows $\{2{:}2, 3{:}1\}$ and $\{2{:}1, 3{:}2\}$ give $\nu_2 = 2 - 1 = 1$ and $\nu_3 = 1 - 1 = 0$ for $12/6 = 2$, so $R = \{2{:}1\}$, and the remainder is $u = 1 \cdot 7 = 7$. The row of $14$ is $\{2{:}1\}, u = 7$. $7$ does not become a piece, and the base stays $[2, 3]$.

## Width

The baselines use 64-bit integers. The width is a single build setting, so a 32-bit run is a rebuild, not a rewrite. Overflow is detected in every operation (a product, a sum, or a result's denominator that no longer fits). A trial that overflows is reported, with where it overflowed, and excluded from the comparisons.

## Memory per rational

Each rational stores only its numerator and an index. Everything else belongs to a *denominator*, and is shared by every rational that uses it. So the cost per rational is

$$
(\text{numerator} + \text{index}) + \frac{\text{everything else}}{N}.
$$

With 8-byte values, 4-byte indices, a lookup of $S$ slots (a power of two, at most half full), $P$ compared pairs (Design 1), and $k$ entries per row on average (divisors of $d$ in Design 1, counting $1$; pieces of the known part of $d$ in Design 2, at most the pieces of $d$), in bytes:
- **Plain rationals:** $16N$.
- **A single shared denominator**, as FLINT does for rational polynomials (for vectors it is done by hand): $8N$ plus one denominator. This holds only while the common denominator, the lcm of all of them, fits the width.
- **Design 1:** $12N + 8D + 4S + 12C + 12kD$ (values, lookup, divisors with their ids sorted by value, and the sparse incidence rows at 12 bytes per (id, residual) entry), plus $8 S_P$ (the pair set: $S_P$ slots, a power of two at least $2P$, so between $16P$ and $32P$; none until a pair is compared). The concept's dense table would be $D(D-1)/16$ instead.
- **Design 2:** $12N + 16D + 4S + 8C + 8kD$ (values, lookup, remainders, base, exponent rows at 8 bytes per (index, $\nu_q$) entry).

**Case A:** $N = 1{,}000{,}000$ rationals over $D = 1{,}000$ denominators, $C = 500$, $k = 5$, $S = 2{,}048$, and $P = 5{,}000$ compared pairs (about 10 per denominator), so $S_P = 16{,}384$.

| | Bytes per rational | Of which, structures |
| --- | --- | --- |
| Plain | 16 | none |
| One shared denominator | about 8, if the lcm fits | none |
| Design 1 | about 12.21 | 0.21: pair set 131 kB, incidence rows 60 kB, the rest 22 kB |
| Design 2 | about 12.07 | 0.07: exponent rows 40 kB, remainders 8 kB, the rest 20 kB |

With many rationals per denominator, both designs cost about 12 bytes: 25% less than plain rationals. Here the concept's dense table, 62 kB, would be smaller than the set: at 8 bytes a slot, the set wins only while fewer than about one pair in 256 has been compared, and with $D = 1{,}000$ that is about 1,950 pairs. If every pair were compared, the set would take 8 MB.

**Case B:** each rational has its own denominator: $N = D = 10{,}000$, again with $C = 500$ and $k = 5$, $S = 32{,}768$, and $P = 50{,}000$ compared pairs, so $S_P = 131{,}072$.
- **Design 1:** the pair set is $8 \cdot 131{,}072$ bytes, about $1.05$ MB, which is $105$ bytes per rational. With the incidence rows ($60$), lookup ($13$), list ($8$) and the numerator and index ($12$), the total is about $198$ bytes per rational. (The dense bit rows and the residual rows took $64 + 40$ in place of $60$, for $242$.) The concept's dense table alone would be $D(D-1)/2$ bits, about $6.25$ MB, or $625$ bytes per rational, for a total of about $720$.
- **Design 2:** about $82$ bytes per rational: $12 + 8 + 13 + 40 + 8$ (the remainders), plus under 1 byte of base.

Both are worse than plain rationals here, Design 1 the more so.

**Measured on the pilot jobs** (Oct 29, 2026: jobs 3, 6, 9, 12, 15, 18 and 20, each with 10,000 entries and 20,000 operations, over 1 to 10,000 entered denominators and 5,700–30,000 in all, with results). At the end of each job, Design 1 used 22–71 bytes per rational with the sparse incidence rows. With the dense incidence bits it had used 23–117, and with the dense pair table as well, 90–1,962. Design 2 used 22–69. Of Design 1's bytes, the pair set took 1–9. The sparse rows, residuals included, took 5–33; the dense bits alone had taken 1.5–61, which was 7–53% of the total. Bytes per rational is therefore a **required test metric**.

## Side by side

| | Design 1: common divisors | Design 2: coprime base |
| --- | --- | --- |
| Structures | 7 (5 in the concept) | 6 |
| Full-width values stored | numerators, denominators, divisors, residuals | numerators, denominators, base pieces, remainders |
| Index or bit storage | lookup: $S$ index slots; pair set: one 64-bit key per compared pair, at most half full; incidence rows: one 32-bit divisor id per fact | lookup: $S$ index slots; exponent rows: one small (index, $\nu_q$) pair per piece $q$ in each known part |
| At entry of a denominator | a lookup, then append; no gcds | a lookup, then append with $u = d$; no gcds |
| A new result denominator | an island, one row entry only, (divisor 1, $d$); no gcds | the known valuations, with the rest in its remainder; no gcds |
| Gcd of a pair | always a merge of the two rows from the top; that is the gcd if the pair is in the set, otherwise one gcd on residuals, then recorded | read from the rows after a meeting refines the two remainders as far as needed; never recorded per pair |
| Residuals | stored beside their divisor ids, read where the merge stops | multiplied out from pieces on each addition |
| Reduction after adding | $\gcd(\text{sum}, g)$ on all of $g$ | only against pieces $q$ with $\nu_q(K_i) = \nu_q(K_j) > 0$, often none |
| Cross gcds in multiplication | against residuals at the largest known common divisor | per piece, only at pieces in one row, then against each remainder; no meeting |
| Expensive moment | a compare whose gcd is new: a gcd on residuals, a binary search, and an insertion into the sorted ids and into two rows | a meeting whose remainders share a factor: one scan of the base; and splitting a piece, which rewrites the rows that use it |
| Grows with | the compared pairs, and the facts recorded | the pieces the meetings needed, and one remainder per denominator |
| Bytes per rational, Case A / Case B | about 12.2 / 198 | about 12.1 / 82 |

### Computational complexity

Notation as in the paper (v0.3, §1 and §4):
- $n$ is the size of the numbers in bits;
- $\gamma$ is the size of $g$;
- $u$ is the larger size of the residuals;
- $s$ is the size of the part shared at equal valuation, the product of $q^{\nu_q(K_i)}$ over the $e$ pieces $q$ with $\nu_q(K_i) = \nu_q(K_j) > 0$ after the meeting, and $s_i$ is the size of the $i$-th such factor;
- $\pi$ is the size of the largest piece;
- $\rho$ is the number of pieces the two rows use;
- $\lambda \le n$ is the sum of $\nu_q$ over the pieces a remainder gives up in a meeting, and $\tau \le \lambda$ the number of those pieces;
- $\rho'$ is the number of pieces queued against a non-unit remainder in a meeting: at most the other row's pieces, plus those gained or created during it;
- $P$ is the number of compared pairs (Design 1).

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
| Structure per addition, every pair | a merge of the two rows, at most $k_i + k_j - 1$ steps for rows of $k_i$ and $k_j$ entries, with the residuals where it stops; one pair-set lookup, expected $O(1)$ probes | a merge of two rows, $O(\rho)$; products of pieces to form $r_x$ and $r_y$, $O\big(M(u) \log \rho\big)$ |
| Extra, pair not yet compared | $G(n - \kappa)$ on residuals ($G(n)$ when $c = 1$); $M(\kappa, \gamma - \kappa)$ for $c \cdot g'$; up to two exact divisions for residuals; a binary search, $O(\log C)$; a new divisor costs $O(C)$ to insert its id into the sorted list, with no row shifted or renumbered; placing $g$ in each of the two rows, a binary search, $O(\log k)$, and an insertion, $O(k)$; one pair-set insertion, expected $O(1)$, $O(P)$ when the set doubles | not applicable: nothing is recorded per pair |
| Meeting (addition, subtraction, division) | not applicable | none when both remainders are $1$; otherwise $\rho' + \tau$ gcds, one per queued piece plus a retest after each piece divided out, and $\lambda + \tau$ divisions: $(\rho' + \tau)\big(\text{Div}(n, \pi) + G(\pi)\big) + (\lambda + \tau)\, \text{Div}(n, \pi)$; plus $G(n)$ for $\gcd(u_i, u_j)$, and, only when that is not $1$, one scan of the base, at most $C$ gcds plus one per piece divided or created; per split, a small refinement plus a scan of all rows, $O(E)$ |
| Per new denominator | a lookup, expected $O(1)$ checks; $O(1)$ to add a row with divisor 1 only; results likewise; no gcds | the same lookup and an append, with the known row and the remainder; no gcds; for a result, power tests on the tested pieces, made only for a new denominator |
| Memory, besides the values | $8 S_P \le 32P$ bytes for the pair set, $kD$ incidence entries (a full-width residual and a 32-bit id), $C$ ids sorted by value, and $S$ index slots | about $2E$ small numbers, $D$ full-width remainders, plus $S$ index slots |

## Prepared for cluster maintenance

Clusters are a later step, not part of the skeleton. The skeleton already keeps what they will need in Design 1: the pair set's entries are exactly the edges between denominators that met in an operation, so a later check of whether a cluster has split can walk those edges, or an adjacency list built from them, to find what each denominator is connected to. (With the dense table it would have ORed rows of bits; the set has no rows to OR.) Design 2 keeps no pair record, since a meeting leaves none, so it would later need a sparse "met" record per denominator.

## The skeleton's boundary

Both baselines are built as written above, and the builders and reviewers are told the goal: **an accurate representation of the skeleton, not an optimized version for production.** Bugs and plain inefficiencies inside the skeleton are fixed. Nothing outside it is added.

**Plain inefficiencies removed** (the spec, §15.1), all reflected above:
- Design 2's `absorb` is **one scan** of the base, and `reduce` likewise stays at a piece: each stays at a piece after dividing by it or splitting it, never restarts at the first piece, and stops as soon as the rest is $1$;
- the gcd that the scan finds is the one used, not computed again;
- each power $q^a$ of a piece of equal valuation is formed once in the walk and kept for the gcd;
- in addition and multiplication, the test of whether a cancellation is a power of its piece is made after the lookup, and only for a new denominator; a cancellation of $1$ costs no division;
- in Design 1, a comparison hands back the residuals at the gcd, which it already knows, so the addition does not read them again;
- when $g' = 1$, Design 1 inserts the pair at once, with no binary search;
- a divisor inserted at the end of the list shifts no row.

**Result reuse: the author's ruling** (Oct 28, 2026; the spec, §15.2). Each design uses only the reuse its own structure gives for free, the way a production implementation of that design would naturally do it. **Design 2 keeps its reuse:** the valuations of a result's denominator come from the operation's own arithmetic, and the unknown part is kept in its remainder. **Design 1 has none:** an eager copy of an operand's row into the result's row, residuals times $\mu$, is not a direct output of its arithmetic, and a production version would more likely be lazy; so a Design 1 result enters with one row entry only, divisor 1 with residual $d$. One later improvement is deferred, outside the baseline: Design 1's lazy parent link (below). Design 2's deferred entry, deferred here before, is superseded by refinement on demand, which goes further. Tests of known divisors against a result's denominator are left out in both designs.

**Inside the skeleton:**
- one fixed integer width, 64 bits, set in one place, with overflow detected; schoolbook arithmetic;
- the same gcd and division routines in both designs and in the control;
- the denominator lookup: a hash table of indices, with linear probing, growing when half full;
- Design 1's pair set, holding only the compared pairs; sparse incidence rows of (divisor id, residual), sorted by divisor value, searched by a merge from the top;
- stable divisor ids, with sorted insertion into the list of ids by value (Design 1);
- Design 2's meetings: refining the two remainders that meet against each other's pieces and each other, with the extra refinement, and a scan of the base only when the remainders share a factor;
- result reuse in Design 2 only, as ruled above: known valuations, the rest kept in the remainder; Design 1's results enter with one row entry only, (divisor 1, $d$);
- in both designs: **no work in advance, no work in the background; each operation computes only what it needs.**

**Outside the skeleton** (possible later improvements):
- threading, GPUs and batching in phases;
- background (eager) comparisons;
- speculative refinement at entry: testing a new denominator, entered or a result, against the base before it meets anything (Design 2; the first form did this);
- a record of which pairs have met (Design 2);
- linking a new divisor to every other denominator it divides (Design 1);
- recording about a result's denominator more than the ruling allows: copying an operand's row (Design 1), divisibility tests, new divisors from the operation's quantities, or facts added to a denominator the lookup finds (either design; a meeting's refinement of its two operand rows is Design 2's own work, inside);
- Design 1's **lazy parent link** (deferred): a result row records a parent $k$ and a factor $\mu$ with $e = d_k \cdot \mu$, and the search consults the parent's row on demand;
- an index that limits which pieces a remainder is tested against (Design 2);
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
- **A variant of Design 1: the index-valued pair table.** Each cell names the column of the pair's gcd, or "not yet", instead of holding 1 bit. (With the pair set and the stable divisor ids of the sparse rows, this would be a 32-bit id stored beside each key, and inserting a divisor would renumber nothing.) A compared pair then skips the merge, though finding its residuals still needs a search of each row, $O(\log k)$. In the dense bit form, its cells needed $\lceil \log_2(C+1) \rceil$ bits, a width unknown in advance that grows with $C$, and inserting a divisor renumbered every cell pointing at or after it, $O(D^2)$.
- **Measured:** operation counts (gcds, divisions, multiplications, lookups), time, and **bytes per rational** (required), with entry cost reported separately from cost per operation, and overflowed trials counted.
- **Afterwards:** which improvements each design gains the most from. A slower baseline may have more room to improve.
