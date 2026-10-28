# Note: two tripartite designs

*A working note, not part of the paper. It describes two ways of organising the stored values so that the refined gcd method's first gcd is found ahead of time. They share the same first two layers and differ in the third. Both are proposals to be prototyped and tested; neither is built yet.*

## The shared foundation: numerators and denominators

Every value enters in lowest terms, $m/d$, and is stored as two linked parts:
- **Partition 1, numerators.** One node per value, holding its integer numerator.
- **Partition 2, denominators.** One node per *distinct* reduced denominator. Many values may share one.
- **Edges between them are unweighted.** Each numerator links to exactly one denominator; a denominator may link to many numerators.

These parts are **immutable**. A value's numerator, its denominator, and the link between them are set once and never change. Nothing that happens later, in partition 3, rewrites them. So the link between a value and its denominator can never become inconsistent.

**The fast path.** Two values with the same denominator node add by integer addition:

$$
\frac{m_x}{d} + \frac{m_y}{d} = \frac{m_x + m_y}{d}.
$$

When the denominators differ, partition 3 supplies what the two denominators share, so that the refined method can skip its first gcd. The two designs differ only in what partition 3 holds.

## Design 1: a partition of common divisors

### Structure

- **Partition 3 holds common divisors found so far:** one node per distinct divisor value greater than 1.
- **An edge from denominator $d$ to divisor $c$** means "$c$ divides $d$". Its weight is the residual $d / c$.
- **Each divisor value is stored once,** however many denominators it divides, and each residual once per (denominator, divisor) pair.

### Filling it

The machine compares pairs of denominators by gcd:
- **eagerly,** in the background, whenever resources are free;
- **lazily,** on demand, when an operation needs a pair that has not been compared yet. That pair goes first.

When a comparison of $d_1$ and $d_2$ finds $g = \gcd(d_1, d_2) > 1$, node $g$ is found or created, and both denominators are linked to it with their residuals $d_1/g$ and $d_2/g$.

**Improvement.** When a new divisor node $g$ is created, it can also be linked to every other denominator it divides. That is a cheap divisibility test per denominator, and it makes later lookups more likely to find the true gcd already recorded.

### Recording which pairs have been compared

A shared divisor node shows that two denominators have *some* common divisor, but sharing no node is ambiguous: either they were compared and found coprime, or they were never compared. The design therefore records comparisons explicitly, as edges *within* partition 2. The denominator partition becomes a complete graph whose edges say whether a pair has been compared. Two variants, to be decided by testing:

- **Variant A, Boolean edges.** Each pair's edge is one bit: compared or not. To find the gcd of a compared pair, find the *largest* divisor node the two share; keeping each denominator's divisor list sorted, largest first, makes this quick. If they share none, their gcd is 1.
  - *Memory:* one bit per pair, $n(n-1)/2$ bits for $n$ denominators. If comparisons are made in a fixed order (each new denominator against all earlier ones), most of this can be replaced by one number per denominator: "compared with every denominator before position $k$". Only out-of-order comparisons forced by operations need explicit storage.
  - *Lookup:* a search over shared divisor nodes.
- **Variant B, pointer edges.** Each compared pair's edge holds a pointer to its gcd node, or a null meaning "coprime". The gcd and both residuals are found directly, with no search.
  - *Memory:* one fixed-size pointer per compared pair: more than A, but never the divisor values themselves, which stay stored once in partition 3.

A third variant, storing the gcd *value* on each pair edge and dropping partition 3, was considered and rejected. It duplicates possibly large numbers once per pair, and either stores residuals per pair or recomputes them by two divisions at every use.

### Adding two values with different denominators

1. **The comparison edge is false:** compute $g = \gcd(d_x, d_y)$, record it (node, residual edges, comparison edge), and continue with step 2.
2. **The comparison edge is true:** take $g$ and the residuals $r_x = d_x/g$ and $r_y = d_y/g$ from partition 3. If no divisor is shared, $g = 1$ and the residuals are the denominators themselves. Then apply the refined method without its first gcd:

$$
\frac{m_x}{d_x} + \frac{m_y}{d_y} = \frac{m_x r_y + m_y r_x}{g\, r_x\, r_y},
\qquad \text{then reduce by } h = \gcd(m_x r_y + m_y r_x,\ g).
$$

When $g = 1$, the result is already in lowest terms, and no gcd is computed at all.

### Example

Denominators $12$, $18$, $30$.
- **The comparisons:** $\gcd(12, 18) = 6$ gives residuals $2$ and $3$; $\gcd(12, 30) = 6$ gives $2$ and $5$; $\gcd(18, 30) = 6$ gives $3$ and $5$.
- **The result:** one divisor node, $6$, linked to all three, with residuals $12 \to 2$, $18 \to 3$, $30 \to 5$.

To add $\tfrac{1}{12} + \tfrac{5}{18}$: the pair is compared, the shared node is $6$, and the residuals are $2$ and $3$. The sum is $(1 \cdot 3 + 5 \cdot 2) / (6 \cdot 2 \cdot 3) = 13/36$. The remaining gcd, $\gcd(13, 6) = 1$, confirms lowest terms.

### Properties

- **Consistency.** Every entry in partition 3, and every comparison edge, is a fact about two immutable denominators. Once true, it stays true. Nothing ever needs repair, and any entry can be checked cheaply: $g \cdot r = d$.
- **The second gcd remains.** Design 1 knows the gcd $g$ but not its parts, so the reduction still needs $\gcd(\text{numerator}, g)$, as in the refined method.
- **Growth.** The comparison record grows with the number of pairs, quadratic at worst. Divisor nodes and residual edges grow with what is actually shared.

## Design 2: a partition of coprime pieces (the coprime base)

### The idea in one sentence

Instead of recording each pair's gcd, break the denominators into pieces that no two denominators can split differently, and record each denominator as a product of those pieces. Then *every* pair's gcd can be read off, without ever having compared that pair.

### Coprime pieces

A set of integers greater than 1 is **pairwise coprime** if no two of them share a factor. Such a set is called a **coprime base** when every denominator is a product of powers of its elements.

It is built from the denominators by gcds and divisions alone; this is called **factor refinement**:
- take two numbers that share a factor;
- replace them by their gcd and their two residuals;
- drop 1s and merge duplicates;
- repeat until no two numbers share a factor.

**Example.** From $12$, $18$, $30$:
- $\gcd(12, 18) = 6$, with residuals $2$ and $3$, gives $\{6, 2, 3, 30\}$.
- $6$ and $2$ share $2$; refining them gives $\{2, 3, 30\}$.
- $30$ shares $2$ with $2$ and $3$ with $3$; its residual is $5$.
- The base is $\{2, 3, 5\}$.

Here the pieces happen to be primes, but they need not be. If $3$ and $5$ always appeared together in the same proportion, $15$ could stay one piece. The base is only as fine as the data force it to be.

### Structure

- **Partition 3 holds the base elements,** each stored once.
- **An edge from denominator $d$ to base element $p_i$** means "$p_i$ divides $d$". Its weight is the *exponent*: how many times $p_i$ divides $d$.
- **So each denominator is described by an exponent vector** over the base: $12 = 2^2 3^1$ is $(2, 1, 0)$, $18 = 2^1 3^2$ is $(1, 2, 0)$, $30 = 2^1 3^1 5^1$ is $(1, 1, 1)$.

Because the pieces are pairwise coprime, each denominator has exactly one exponent vector.

### Reading off any pair

For two denominators with vectors $\alpha$ and $\beta$, write $\alpha \wedge \beta$ and $\alpha \vee \beta$ for the componentwise minimum and maximum, and $p^{\alpha}$ for $\prod p_i^{\alpha_i}$:

$$
\gcd = p^{\alpha \wedge \beta}, \qquad \text{residuals} = p^{\alpha - (\alpha \wedge \beta)} \text{ and } p^{\beta - (\alpha \wedge \beta)}, \qquad \text{lcm} = p^{\alpha \vee \beta}.
$$

For $12$ and $18$: $\alpha \wedge \beta = (1, 1, 0)$, so the gcd is $6$, the residuals are $2$ and $3$, and the lcm is $36$, all without a comparison ever having been made between $12$ and $18$.

**No pair record is needed.** There is no "not yet compared" state: once a denominator has been refined against the base, its vector is complete, and its gcd with every other denominator is known.

### Adding two values with different denominators

The refined method's first gcd is read off the vectors, as above. There is one further gain over Design 1. The pieces tell us *which part* of $g$ can cancel: only the pieces shared at *equal* power, those with $\alpha_i = \beta_i$. Where the powers differ, one term of the numerator is divisible by the piece and the other is coprime to it, so their sum cannot be. The second gcd shrinks from $\gcd(\text{numerator}, g)$ to a gcd against only those pieces, and disappears when there are none.

For $\tfrac{1}{12} + \tfrac{5}{18}$, the vectors $(2,1,0)$ and $(1,2,0)$ have no equal component, so nothing can cancel, and $13/36$ is known to be in lowest terms without any gcd.

### Adding a new denominator

A new denominator $d$ is refined against the base: its gcds with base elements are taken, and $d$ is split into pieces.
- **A piece that shares nothing with the base** becomes a new base element.
- **When $d$ shares only *part* of an existing element $q$** (for example, $q = 15$ and $d = 45 = 3^2 \cdot 5$), $q$ is refined into finer pieces, here $3$ and $5$.
- **Every denominator that used $q$** has its vector updated: an exponent $w$ on $15$ becomes $w$ on $3$ and $w$ on $5$.

Because numerators link to *denominators*, not to base elements, a refinement updates only the distinct denominators that use the element, never the values. That is the gain of the tripartite layout over the design built so far, which rewrote every value.

### Properties

- **Consistency.** Partition 3 does change: elements are refined. But every exponent vector is derived from an immutable denominator and can be checked exactly: $p^{\alpha}$ must equal $d$. A wrong vector is always detectable, and can always be recomputed from $d$.
- **Growth.** Storage grows with the base and with the edges from distinct denominators to base elements. There is no pair record, so nothing grows quadratically.
- **The cost moves to entry.** Each new denominator must be refined against the base elements it might share with. Done naively, that means all of them, which grows with the base. An index, for example a lookup of small prime pieces, or a product of the large pieces checked by one gcd, keeps it bounded. This is the main thing to engineer.

## Side by side

| | Design 1: common divisors | Design 2: coprime base |
| --- | --- | --- |
| Partition 3 holds | gcds found between pairs | pairwise coprime pieces of all denominators |
| Denominator-to-partition-3 edge weight | residual $d/c$ | exponent of the piece in $d$ |
| Pair record needed | yes (variant A: bits; variant B: pointers) | no |
| Gcd of a pair | looked up: the largest shared node (A) or via the pointer (B); computed once per pair on first meeting | read off as $p^{\alpha \wedge \beta}$ for any pair, never computed per pair |
| Second gcd (reduction) | against all of $g$ | only against pieces shared at equal power, often none |
| What changes over time | new nodes and edges added; nothing ever modified | pieces refined; denominators' vectors updated (never values) |
| Consistency | entries never become wrong | vectors can change but are always checkable: $p^{\alpha} = d$ |
| Growth | pairs compared (quadratic at worst) | base size plus denominator–piece edges |
| Cost concentrated in | gcds per pair, at first meeting or in the background | refinement per new denominator |

## To be decided by testing

- **For Design 1:** variant A against variant B (memory against lookup speed), and the value of linking new divisors to every denominator they divide.
- **Design 1 against Design 2** on the same workloads, especially mixed and partially shared denominators, and on larger numbers, where gcds are expensive.
- **For Design 2:** the index that keeps refinement of a new denominator from growing with the base.
