# Note: Design 1, common divisors

*A working note, not part of the paper. Design 1's part of the [joint note](../../shared/partial-sharing-baseline/research/design-note.md) on the two skeleton designs, extracted on Oct 29, 2026, when the designs became separate specifications. The text is the joint note's, with the other design and the side-by-side comparison left out; the comparison stays in the joint note. Status: **concluded**, see [findings](findings.md).*

## Purpose and scope

Existing rational arithmetic handles two extremes well:
- **Same denominator:** add the numerators.
- **Nothing shared:** the general method, with its gcds.

Most real pairs of denominators lie between the two: they share *some* factors but not all. Design 1 exploits that middle ground (as Design 2 does, by other means: [`09-coprime-base`](../../09-coprime-base/research/design-note.md)) in two ways:
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

**Same denominator.** When $i$ is $j$, both cross gcds are $1$ ($\gcd(m_x, d_i) = 1$ by lowest terms), so $\tfrac{m_x}{d} \cdot \tfrac{m_y}{d} = \tfrac{m_x m_y}{d^2}$ with no gcd at all. This is true of plain rationals too; in the designs, equal indices detect it with one comparison. The denominator $d^2$ is still looked up (in Design 1, if new, it is an island), and it overflows the width sooner.

**A zero factor.** If $x = 0$, stored as $0/1$, then $g_1 = \gcd(0, d_j) = d_j$ and $g_2 = \gcd(m_y, 1) = 1$, so the formula gives $0/1$ with no special case (and likewise for $y = 0$).

*Examples,* after the additions above:
- $\tfrac{5}{18} \cdot \tfrac{7}{30}$. Control: $g_1 = \gcd(5, 30) = 5$, $g_2 = \gcd(7, 18) = 1$. Design 1: $c = 6$, with residuals $3$ and $5$, so $g_1 = \gcd(5, 5) = 5$ and $g_2 = \gcd(7, 3) = 1$. Result: $\tfrac{7}{2^2 \cdot 3^3} = \tfrac{7}{108}$.
- $\tfrac{5}{12} \cdot \tfrac{7}{12} = \tfrac{35}{144}$, with no gcd.

### Division

$x / y = x \cdot (1/y)$, and $y = 0$ is an error. The reciprocal of $m_y/d_j$ is $d_j / m_y$, with the sign of $m_y$ moved to the numerator: $\pm d_j / |m_y|$. The cross gcds of this product are:
- $g_1 = \gcd(m_x, |m_y|)$, between two numerators: neither design helps;
- $g_2 = \gcd(d_i, d_j)$, between two denominators: exactly what the designs are for. Design 1 finds it by the search, a lookup if the pair bit is set, otherwise by the chain, then records it and sets the bit (two operand denominators were compared).

The result is $\pm (m_x/g_1)(d_j/g_2)$ over $(d_i/g_2)(|m_y|/g_1)$. Its denominator contains part of the old numerator, which the structures have never seen, so it must be entered: looked up in Design 1, an island if new.

**A zero dividend.** If $x = 0$, stored as $0/1$, then $g_1 = \gcd(0, |m_y|) = |m_y|$ and $g_2 = 1$, so the result is $0/1$ with no special case.

When $i$ is $j$, $g_2 = d_i$ and the denominators cancel completely: the result is $m_x/m_y$ reduced by $\gcd(m_x, m_y)$, with the sign moved to the numerator.

*Example:* $\tfrac{5}{12} \div \left(-\tfrac{7}{18}\right) = -\tfrac{5 \cdot 18}{12 \cdot 7}$. Here $g_1 = \gcd(5, 7) = 1$, and $g_2 = \gcd(12, 18) = 6$: Design 1 reads it from the search, since pair bit (0, 1) is set. Result: $-\tfrac{5 \cdot 3}{2 \cdot 7} = -\tfrac{15}{14}$. The denominator $14$ is new. In Design 1 it is an island, $\{1{:}14\}$.

## Width

The baselines use 64-bit integers. The width is a single build setting, so a 32-bit run is a rebuild, not a rewrite. Overflow is detected in every operation (a product, a sum, or a result's denominator that no longer fits). A trial that overflows is reported, with where it overflowed, and excluded from the comparisons.

## Memory per rational

Each rational stores only its numerator and an index. Everything else belongs to a *denominator*, and is shared by every rational that uses it. So the cost per rational is

$$
(\text{numerator} + \text{index}) + \frac{\text{everything else}}{N}.
$$

With 8-byte values, 4-byte indices, a lookup of $S$ slots (a power of two, at most half full), $P$ compared pairs (Design 1), and $k$ entries per row on average (divisors of $d$, counting $1$), in bytes:
- **Plain rationals:** $16N$.
- **A single shared denominator**, as FLINT does for rational polynomials (for vectors it is done by hand): $8N$ plus one denominator. This holds only while the common denominator, the lcm of all of them, fits the width.
- **Design 1:** $12N + 8D + 4S + 12C + 12kD$ (values, lookup, divisors with their ids sorted by value, and the sparse incidence rows at 12 bytes per (id, residual) entry), plus $8 S_P$ (the pair set: $S_P$ slots, a power of two at least $2P$, so between $16P$ and $32P$; none until a pair is compared). The concept's dense table would be $D(D-1)/16$ instead.

**Case A:** $N = 1{,}000{,}000$ rationals over $D = 1{,}000$ denominators, $C = 500$, $k = 5$, $S = 2{,}048$, and $P = 5{,}000$ compared pairs (about 10 per denominator), so $S_P = 16{,}384$.

| | Bytes per rational | Of which, structures |
| --- | --- | --- |
| Plain | 16 | none |
| One shared denominator | about 8, if the lcm fits | none |
| Design 1 | about 12.21 | 0.21: pair set 131 kB, incidence rows 60 kB, the rest 22 kB |

With many rationals per denominator, Design 1 costs about 12 bytes: 25% less than plain rationals. Here the concept's dense table, 62 kB, would be smaller than the set: at 8 bytes a slot, the set wins only while fewer than about one pair in 256 has been compared, and with $D = 1{,}000$ that is about 1,950 pairs. If every pair were compared, the set would take 8 MB.

**Case B:** each rational has its own denominator: $N = D = 10{,}000$, again with $C = 500$ and $k = 5$, $S = 32{,}768$, and $P = 50{,}000$ compared pairs, so $S_P = 131{,}072$.
- **Design 1:** the pair set is $8 \cdot 131{,}072$ bytes, about $1.05$ MB, which is $105$ bytes per rational. With the incidence rows ($60$), lookup ($13$), list ($8$) and the numerator and index ($12$), the total is about $198$ bytes per rational. (The dense bit rows and the residual rows took $64 + 40$ in place of $60$, for $242$.) The concept's dense table alone would be $D(D-1)/2$ bits, about $6.25$ MB, or $625$ bytes per rational, for a total of about $720$.

Design 1 is worse than plain rationals here (Design 2, at about 82 bytes, less so).

**Measured on the pilot jobs** (Oct 29, 2026: jobs 3, 6, 9, 12, 15, 18 and 20, each with 10,000 entries and 20,000 operations, over 1 to 10,000 entered denominators and 5,700–30,000 in all, with results). At the end of each job, Design 1 used 22–71 bytes per rational with the sparse incidence rows. With the dense incidence bits it had used 23–117, and with the dense pair table as well, 90–1,962. Of Design 1's bytes, the pair set took 1–9. The sparse rows, residuals included, took 5–33; the dense bits alone had taken 1.5–61, which was 7–53% of the total. Bytes per rational is therefore a **required test metric**.

## At a glance

|  | Design 1: common divisors |
| --- | --- |
| Structures | 7 (5 in the concept) |
| Full-width values stored | numerators, denominators, divisors, residuals |
| Index or bit storage | lookup: $S$ index slots; pair set: one 64-bit key per compared pair, at most half full; incidence rows: one 32-bit divisor id per fact |
| At entry of a denominator | a lookup, then append; no gcds |
| A new result denominator | an island, one row entry only, (divisor 1, $d$); no gcds |
| Gcd of a pair | always a merge of the two rows from the top; that is the gcd if the pair is in the set, otherwise one gcd on residuals, then recorded |
| Residuals | stored beside their divisor ids, read where the merge stops |
| Reduction after adding | $\gcd(\text{sum}, g)$ on all of $g$ |
| Cross gcds in multiplication | against residuals at the largest known common divisor |
| Expensive moment | a compare whose gcd is new: a gcd on residuals, a binary search, and an insertion into the sorted ids and into two rows |
| Grows with | the compared pairs, and the facts recorded |
| Bytes per rational, Case A / Case B | about 12.2 / 198 |

### Computational complexity

Notation as in the paper (v0.3, §1 and §4):
- $n$ is the size of the numbers in bits;
- $\gamma$ is the size of $g$;
- $u$ is the larger size of the residuals;
- $P$ is the number of compared pairs.

Also:
- $D$ is the number of denominators. To avoid a clash, the paper's division cost $D(\cdot)$ is written $\text{Div}(\cdot)$ here;
- $C$ the number of divisors;
- $\kappa$ the size of $c$;
- $S$ the number of lookup slots, and $k$ the average number of entries per row.

Where a cell gives three forms, they are symbolic; schoolbook; fastest.

|  | Design 1 |
| --- | --- |
| Arithmetic per addition, $g$ and residuals in hand | $3M(n, u) + \text{Div}(2n, \gamma) + G(\gamma)$, plus exact divisions by $h$; $O(nu + n\gamma)$; $O(n \log u + n \log \gamma + \gamma \log^2 \gamma)$ |
| Structure per addition, every pair | a merge of the two rows, at most $k_i + k_j - 1$ steps for rows of $k_i$ and $k_j$ entries, with the residuals where it stops; one pair-set lookup, expected $O(1)$ probes |
| Extra, pair not yet compared | $G(n - \kappa)$ on residuals ($G(n)$ when $c = 1$); $M(\kappa, \gamma - \kappa)$ for $c \cdot g'$; up to two exact divisions for residuals; a binary search, $O(\log C)$; a new divisor costs $O(C)$ to insert its id into the sorted list, with no row shifted or renumbered; placing $g$ in each of the two rows, a binary search, $O(\log k)$, and an insertion, $O(k)$; one pair-set insertion, expected $O(1)$, $O(P)$ when the set doubles |
| Per new denominator | a lookup, expected $O(1)$ checks; $O(1)$ to add a row with divisor 1 only; results likewise; no gcds |
| Memory, besides the values | $8 S_P \le 32P$ bytes for the pair set, $kD$ incidence entries (a full-width residual and a 32-bit id), $C$ ids sorted by value, and $S$ index slots |

## Prepared for cluster maintenance

Clusters are a later step, not part of the skeleton. The skeleton already keeps what they will need in Design 1: the pair set's entries are exactly the edges between denominators that met in an operation, so a later check of whether a cluster has split can walk those edges, or an adjacency list built from them, to find what each denominator is connected to. (With the dense table it would have ORed rows of bits; the set has no rows to OR.)

## The skeleton's boundary

The baseline is built as written above, and the builders and reviewers are told the goal: **an accurate representation of the skeleton, not an optimized version for production.** Bugs and plain inefficiencies inside the skeleton are fixed. Nothing outside it is added.

**Plain inefficiencies removed** (the spec, §15.1), all reflected above:
- in Design 1, a comparison hands back the residuals at the gcd, which it already knows, so the addition does not read them again;
- when $g' = 1$, Design 1 inserts the pair at once, with no binary search;
- a divisor inserted at the end of the list shifts no row.

**Result reuse: the author's ruling** (Oct 28, 2026; the spec, §15.2). Each design uses only the reuse its own structure gives for free, the way a production implementation of that design would naturally do it. **Design 1 has none:** an eager copy of an operand's row into the result's row, residuals times $\mu$, is not a direct output of its arithmetic, and a production version would more likely be lazy; so a Design 1 result enters with one row entry only, divisor 1 with residual $d$. One later improvement is deferred, outside the baseline: Design 1's lazy parent link (below). Tests of known divisors against a result's denominator are left out.

**Inside the skeleton:**
- one fixed integer width, 64 bits, set in one place, with overflow detected; schoolbook arithmetic;
- the same gcd and division routines as Design 2 and the control;
- the denominator lookup: a hash table of indices, with linear probing, growing when half full;
- Design 1's pair set, holding only the compared pairs; sparse incidence rows of (divisor id, residual), sorted by divisor value, searched by a merge from the top;
- stable divisor ids, with sorted insertion into the list of ids by value (Design 1);
- no result reuse, as ruled above: Design 1's results enter with one row entry only, (divisor 1, $d$);
- **no work in advance, no work in the background; each operation computes only what it needs.**

**Outside the skeleton** (possible later improvements):
- threading, GPUs and batching in phases;
- background (eager) comparisons;
- linking a new divisor to every other denominator it divides (Design 1);
- recording about a result's denominator more than the ruling allows: copying an operand's row, divisibility tests, new divisors from the operation's quantities, or facts added to a denominator the lookup finds;
- Design 1's **lazy parent link** (deferred): a result row records a parent $k$ and a factor $\mu$ with $e = d_k \cdot \mu$, and the search consults the parent's row on demand;
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
- **Afterwards:** which improvements the design gains the most from. A slower baseline may have more room to improve.


*Done, Oct 29, 2026:* measured against the control and against Design 2 in the [partial-sharing baseline](../../shared/partial-sharing-baseline/) ([results](../../shared/partial-sharing-baseline/test/results/results.md)); what was learned is in [findings](findings.md).
