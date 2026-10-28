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

- **Partition 3 holds common divisors found so far:** one node per distinct divisor value, including an explicit node $1$.
- **An edge from denominator $d$ to divisor $c$** means "$c$ divides $d$". Its weight is the residual $d / c$.
- **Every denominator is linked to node $1$,** with residual $d$ itself. That residual need not be stored, since it is the denominator, but the edge exists conceptually. So every pair of denominators shares at least one divisor node.
- **Each divisor value is stored once,** however many denominators it divides, and each residual once per (denominator, divisor) pair.

### Filling it

The machine compares pairs of denominators by gcd:
- **eagerly,** in the background, whenever resources are free;
- **lazily,** on demand, when an operation needs a pair that has not been compared yet. That pair goes first.

When a comparison of $d_1$ and $d_2$ finds $g = \gcd(d_1, d_2)$, node $g$ is found or created, both denominators are linked to it with their residuals $d_1/g$ and $d_2/g$, and the comparison is recorded. When $g = 1$, both are already linked to node $1$, and only the comparison is recorded.

**Improvement.** When a new divisor node $g$ is created, it can also be linked to every other denominator it divides. That is a cheap divisibility test per denominator, and it makes later lookups more likely to find the true gcd already recorded.

### Recording which pairs have been compared

The largest divisor node two denominators share is a common divisor, but not necessarily the greatest: unless the pair has been compared, a larger one may exist. Since every pair shares node $1$, the shared nodes alone cannot tell a compared pair from one never compared. The design therefore records comparisons explicitly, as edges *within* partition 2. The denominator partition becomes a complete graph whose edges say whether a pair has been compared. Two variants:

- **Variant A, Boolean edges: the clearest way to understand the design.** Each pair's edge is one bit: compared or not. To find the gcd of a compared pair, find the *largest* divisor node the two share, which is at least node $1$; keeping each denominator's divisor list sorted, largest first, makes this quick.
  - *Memory:* one bit per pair, $N(N-1)/2$ bits for $N$ denominators. If comparisons are made in a fixed order (each new denominator against all earlier ones), most of this can be replaced by one number per denominator: "compared with every denominator before position $j$". Only out-of-order comparisons forced by operations need explicit storage.
  - *Lookup:* a search over shared divisor nodes, then a fetch of the divisor's value from partition 3.
- **Variant B, pointer edges: the expected implementation.** Each compared pair's edge holds a pointer to its gcd node. A pointer to node $1$ means "compared, coprime"; no pointer means "not compared". The gcd and both residuals are found directly, with no search.
  - *Why it is expected:* the divisor's value must be fetched from partition 3 either way. A pointer goes there in one step, while variant A first searches the shared divisor nodes. So the lookup is a sunk cost, and the pointer removes the extra steps. It is cheap but not free: one memory access.
  - *Memory:* one fixed-size pointer per compared pair: more than A, but never the divisor values themselves, which stay stored once in partition 3.

A third variant, storing the gcd *value* on each pair edge and dropping partition 3, was considered and rejected. It duplicates possibly large numbers once per pair, and either stores residuals per pair or recomputes them by two divisions at every use.

### The chain of gcds when a pair has not been compared

Even when a pair has never been compared, the largest divisor node the two already share, $c$ (at least $1$), is a known common divisor, though not confirmed as the greatest. The rest of the gcd lies in the residuals:

$$
\gcd(d_1, d_2) = c \cdot \gcd\!\left(\frac{d_1}{c},\ \frac{d_2}{c}\right).
$$

So the remaining gcd, $g' = \gcd(d_1/c,\ d_2/c)$, runs on the smaller residuals, which are already stored on the edges to $c$. The result $g = c \cdot g'$ is then recorded: node $g$ is created if new, its residual edges are added, and the pair's comparison record is set.

The full chain, from cheapest to most expensive:
1. **Same denominator:** integer addition.
2. **Compared:** follow the record to $g$.
3. **Not compared:** take the largest shared divisor $c$, then a gcd on the residuals.
4. **Nothing beyond $1$ shared:** the same step with $c = 1$, which is the refined method's first gcd on the full denominators.

### Adding two values

```text
function add(m_x / d_x, m_y / d_y)
    if d_x is d_y then
        return (m_x + m_y) / d_x
    end if
    if the pair d_x and d_y is compared then
        set g to the largest divisor node shared by d_x and d_y
    else
        set c to the largest divisor node shared by d_x and d_y
        set g' to gcd(d_x / c, d_y / c)
        set g to c · g'
        if node g does not exist then
            create node g
        end if
        if d_x is not linked to g then
            link d_x to g with residual d_x / g
        end if
        if d_y is not linked to g then
            link d_y to g with residual d_y / g
        end if
        mark the pair d_x and d_y as compared
    end if
    set r_x to the residual of d_x at g
    set r_y to the residual of d_y at g
    set sum to m_x · r_y + m_y · r_x
    if g is 1 then
        return sum / (r_x · r_y)
    end if
    set h to gcd(sum, g)
    return (sum / h) / (r_x · r_y · (g / h))
end function
```

Variant B replaces the comparison check and, for a compared pair, the search for the largest shared divisor with one pointer lookup, and records a comparison by setting the pointer.

The last steps are the refined method without its first gcd:

$$
\frac{m_x}{d_x} + \frac{m_y}{d_y} = \frac{m_x r_y + m_y r_x}{g\, r_x\, r_y},
\qquad \text{then reduce by } h = \gcd(m_x r_y + m_y r_x,\ g).
$$

When $g = 1$, the result is already in lowest terms, and no second gcd is computed.

### Example

Denominators $12$, $18$, $30$. All three are linked to node $1$, with implicit residuals $12$, $18$ and $30$.
- **A first comparison:** $\gcd(12, 18) = 6$ creates node $6$, with residuals $12 \to 2$ and $18 \to 3$. In variant B, the pair holds a pointer to node $6$.
- **The improvement:** $6$ divides $30$, so $30$ is linked to node $6$ with residual $5$. The pairs $(12, 30)$ and $(18, 30)$ are still not compared.

To add $\tfrac{1}{12} + \tfrac{5}{18}$: the pair is compared, $g = 6$, and the residuals are $2$ and $3$. The sum is $(1 \cdot 3 + 5 \cdot 2) / (6 \cdot 2 \cdot 3) = 13/36$. The remaining gcd, $\gcd(13, 6) = 1$, confirms lowest terms.

To add $\tfrac{1}{12} + \tfrac{1}{30}$: the pair is not compared, but the two share node $6$, so $c = 6$. The gcd on the residuals is $\gcd(2, 5) = 1$, so $g = 6 \cdot 1 = 6$; the node and its edges already exist, and only the comparison is recorded. The sum is $(1 \cdot 5 + 1 \cdot 2) / (6 \cdot 2 \cdot 5) = 7/60$, and $\gcd(7, 6) = 1$ confirms lowest terms.

### Properties

- **Consistency.** Every entry in partition 3, and every comparison edge, is a fact about two immutable denominators. Once true, it stays true. Nothing ever needs repair, and any entry can be checked cheaply: $g \cdot r = d$.
- **The second gcd remains.** Design 1 knows the gcd $g$ but not its parts, so the reduction still needs $\gcd(\text{numerator}, g)$, as in the refined method.
- **Growth.** The comparison record grows with the number of pairs, quadratic at worst. Divisor nodes and residual edges grow with what is actually shared; the edges to node $1$ need no storage.

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

### Structure and behaviour

| | Design 1: common divisors | Design 2: coprime base |
| --- | --- | --- |
| Partition 3 holds | node $1$ and gcds found between pairs | pairwise coprime pieces of all denominators |
| Denominator-to-partition-3 edge weight | residual $d/c$ | exponent of the piece in $d$ |
| Pair record needed | yes (variant A: bits; variant B: pointers, expected in practice; a pointer to node $1$ means coprime, no pointer means not compared) | no |
| Gcd of a pair | looked up: the largest shared node (A) or via the pointer (B); on first meeting, computed once as $c \cdot \gcd(d_1/c,\ d_2/c)$, $c$ the largest node already shared (at least $1$) | read off as $p^{\alpha \wedge \beta}$ for any pair, never computed per pair |
| Second gcd (reduction) | against all of $g$ | only against pieces shared at equal power, often none |
| What changes over time | new nodes and edges added; nothing ever modified | pieces refined; denominators' vectors updated (never values) |
| Consistency | entries never become wrong | vectors can change but are always checkable: $p^{\alpha} = d$ |
| Growth | pairs compared (quadratic at worst) | base size plus denominator–piece edges |
| Cost concentrated in | gcds per pair, at first meeting or in the background | refinement per new denominator |

### Computational complexity

Notation as in v0.3, §1 and §4: $n$ is the size of the numbers in bits; $\gamma$ the size of $g$; $u$ the larger size of the residuals (cofactors); $s$ the size of the part shared at equal power, made of $e$ base elements of sizes $s_i$; $\pi$ the size of the largest base element; $\rho$ the number of base elements the two values use (v0.3's $r$); $\tau$ the number of base elements a new denominator is tested against (the whole base, $k$, without an index); $\lambda \le n$ the sum of a denominator's exponents. New here: $N$ is the number of distinct denominators; $\delta_x$, $\delta_y$ are the numbers of divisor nodes linked to $d_x$ and $d_y$; $\kappa$ is the size of the largest shared divisor $c$; $\nu$ is the number of denominators that use a refined piece. Where a cell gives three forms, they are symbolic; schoolbook; fastest.

| | Design 1, variant A | Design 1, variant B | Design 2 |
| --- | --- | --- | --- |
| Arithmetic per addition, $g$ and residuals in hand | refined method without its first gcd or residual divisions: $3M(n, u) + D(2n, \gamma) + G(\gamma)$, plus exact divisions by $h$; $O(nu + n\gamma)$; $O(n \log u + n \log \gamma + \gamma \log^2 \gamma)$ | as A | $2M(n, u) + O(n) + \sum_i \big(D(n, s_i) + G(s_i)\big)$, plus exact divisions; zero reduction when $s = 0$; $O(nu + ns)$; $O(n \log u + e\, n \log s + s \log^2 s)$ |
| Structure per addition, pair already known | comparison-bit check plus search of shared divisor nodes: $O(\delta_x + \delta_y)$ | one pointer lookup: $O(1)$ | read both exponent vectors: $O(\rho)$, plus $O\big(M(u) \log \rho\big)$ to form the cofactors if not kept, plus $G(\pi) + D(\pi) + O(\nu)$ per refinement triggered |
| Structure per addition, pair not yet compared | search for $c$, $O(\delta_x + \delta_y)$; $G(n - \kappa)$ for $g'$ on the residuals ($G(n)$ when $c = 1$); $M(\kappa, \gamma - \kappa)$ for $g = c \cdot g'$; record: up to $2D(n, \gamma)$ for new residuals, $O(1)$ for the bit; linking a new $g$ to the other denominators it divides, up to $(N - 2)\,D(n, \gamma)$ | as A, recording a pointer instead of a bit | not applicable: every pair is always known |
| One-time, per new denominator | none required; background comparisons cost up to $(N - 1)\,G(n)$ | as A | refinement against the base: $(\tau + \lambda)\,D(n, \pi) + \tau\, G(\pi)$, plus $G(\pi) + D(\pi) + O(\nu)$ per refinement; $O\big((\tau + \lambda)\, n \pi\big)$, plus $O(\pi^2 + \nu)$ per refinement; $O\big((\tau + \lambda)\, n \log \pi + \tau\, \pi \log^2 \pi\big)$, plus $O(\pi \log^2 \pi + \nu)$ per refinement |
| Memory | at most $N(N-1)/2$ bits (one number per denominator with a fixed comparison order, plus out-of-order pairs), plus divisor nodes and residual edges (each up to $n$ bits) | one pointer per compared pair, at most $N(N-1)/2$, plus the same | base elements plus denominator–piece edges (small exponents); no per-pair storage |
| Total background work to know every pair | $N(N-1)/2$ gcds: up to $\tfrac{N(N-1)}{2}\, G(n)$ | as A | each denominator refined once: $N$ times the one-time row; essentially linear in $Nn$ with a batch coprime base [Bernstein] |

## To be decided by testing

- **For Design 1:** variant A against variant B (memory against lookup speed), and the value of linking new divisors to every denominator they divide.
- **Design 1 against Design 2** on the same workloads, especially mixed and partially shared denominators, and on larger numbers, where gcds are expensive.
- **For Design 2:** the index that keeps refinement of a new denominator from growing with the base.
