# The 5+2i problem: Collatz maps on the Gaussian integers

*Draft, 5 November 2026. Code, logs and Lean proofs: `research/collatz/`. Each result carries its status: proved on paper, proved in Lean 4, computer-assisted (a proof that rests on stated program runs), computed to a bound (an observation for every start up to that bound), or heuristic.*

## Abstract

A Collatz map on the Gaussian integers divides by $1+i$ when it can and otherwise sends $z$ to $\alpha z + d(z)$, with the digit $d(z)$ chosen by the class of $z$ modulo $(1+i)^k$. Such a map is a Collatz map if every nonzero orbit reaches a cycle containing a unit. Without further conditions Collatz maps are plentiful at the smallest moduli: five of the eight maps with multiplier of norm $5$ at modulus $(1+i)^2$ send every start of norm at most $10^7$ to a unit, and at modulus $(1+i)^3$ there are $305$ such maps up to conjugacy by units at $N(z) \le 10^6$. A unit cycle can have three elements and a map can have three trivial cycles, numbers that a unit group of order $4$ does not explain, and rotating or mirroring the multiplier gives an unrelated map. We then ask that the map commute with the units, $T(uz) = uT(z)$. This empties every modulus up to $(1+i)^2$ by [Paper 1], and it brings symmetries: the orbit of $uz$ is $u$ times the orbit of $z$, the trivial cycles number $1$, $2$ or $4$, and each map comes in a family of eight rotated and mirrored maps with the same norm sequences. Inside the window $2^k < N(\alpha) < 2^{k+1}$ no map that commutes with the units and has $N(\alpha) < 29$ is a Collatz map, whatever its digits; this is proved in Lean. The 5+2i map, with modulus $(1+i)^4$ and digits $\pm 1, \pm i, \pm(2+i), \pm(1-2i)$, reaches norm $29$. We conjecture that it is a Collatz map and have checked every start of norm at most $3 \cdot 10^9$. A census at moduli $(1+i)^3$ to $(1+i)^7$ places it among the other single-cycle maps by score and drift, and for the maps with many cycles it gives complete lists up to stated lengths, organized by near-coincidences of $\alpha^n$ with a unit times $(1+i)^V$.

## 1. Introduction

The $3x+1$ map sends an even integer $x$ to $x/2$ and an odd one to $3x+1$, and the Collatz conjecture says that every positive orbit reaches $1$. On all of $\mathbb{Z}$ the map has three further cycles, through $-1$, $-5$ and $-17$, because it does not commute with $-1$: $-(3x+1) = 3(-x) - 1$, so on the negative integers it acts as $3x-1$ does on the positive ones. [Paper 1] sets up maps of this kind on $\mathbb{N}$, $\mathbb{Z}$, the Gaussian integers $\mathbb{Z}[i]$ and the Eisenstein integers, and observes that commuting with the units matters more as the ring gains units. On $\mathbb{N}$ there is nothing to commute with. On $\mathbb{Z}$ the condition is already what separates $7x \pm 1$, with a single cycle up to sign, from $3x+1$. On $\mathbb{Z}[i]$, with four units, it decides which maps exist at all at a given modulus and whether the units behave alike under the map.

This paper is a closer look at $\mathbb{Z}[i]$. Section 2 studies Collatz maps that do not commute with the units. They exist at the lowest moduli, and their trivial cycles and their behaviour under rotation of the multiplier show what is lost without the criterion. Section 3 imposes the criterion and derives its consequences: rotational symmetry of orbits, families of eight maps with equal norm sequences, and a description of the unit cycles. Section 4 proves that inside the window no map that commutes with the units is a Collatz map below norm $29$, and introduces the 5+2i map, the first at that norm. Sections 5 and 6 are the census of maps that commute with the units at moduli $(1+i)^3$ to $(1+i)^7$ and the study of the maps in it with many cycles. Section 7 describes the verification and the formal proofs.

The criterion does not make a map a Collatz map and is not needed for one. What it gives is symmetry. The facts that the orbit of $uz$ is $u$ times the orbit of $z$, that a map and its rotated and mirrored maps have the same norm sequences, and that the number of trivial cycles divides $4$ all hold for maps that commute with the units and fail for some Collatz maps that do not.

### 1.1 Main results

In the order in which they are proved:

1. At modulus $(1+i)^2$ the map with $\alpha = 2-i$ and digits $i$, $1$ on the two odd classes has the unit cycle $1 \to -i \to -1 \to 1$, which has length $3$, and sends every nonzero $z$ with $N(z) \le 10^7$ into it (Theorem 1; Lean, given a program run). Four other maps of norm $5$ pass the same check, and the mirror $2+i$ fails.
2. At modulus $(1+i)^3$ there are Collatz maps that do not commute with the units for eight of the twelve multipliers, as far as computation shows, often with a single digit of the unit table changed. For $\alpha = -3$, $-3+2i$ and $-2+3i$ there is none, whatever the digit table (Theorem 2; paper). For $\alpha = 3$ the question is open.
3. A map that commutes with the units has orbits with rotational symmetry, its trivial cycles number $4/|U \cap C_1|$ with $C_1$ the cycle through $1$, its unit cycles all have the order of $S(1)$ as length, and its rotated and mirrored maps have the same norm sequences and are Collatz maps together with it (Theorem 3 and Proposition 3.4; Lean).
4. Inside the window, a map that commutes with the units and has $N(\alpha) < 29$ is not a Collatz map, for any digit table (Theorem 4; Lean).
5. The 5+2i map: Conjecture 1 says it is a Collatz map; every start with $N(z) \le 3 \cdot 10^9$ reaches $\pm 1$ or $\pm i$ (computer-assisted, with soundness in Lean).
6. The census at moduli $(1+i)^3$ to $(1+i)^7$: $2$ of $14$ maps at $(1+i)^4$, $22$ of $112$ at $(1+i)^6$ and $8$ of $46$ at $(1+i)^7$ have a single cycle up to units at $N(z) \le 10^6$, and none at $(1+i)^3$ and $(1+i)^5$ (computed).
7. For four maps at $(1+i)^7$ with many cycles, complete lists of the cycles with least norm at most $10^9$, which include every cycle up to a stated number of odd steps (Theorem 5; computer-assisted).

### 1.2 Glossary

The terms below are defined in [Paper 1]; this list fixes the notation used here. Write $\pi = 1+i$, $U = \{1, i, -1, -i\}$ and $N(a+bi) = a^2 + b^2$.

- Even and odd: $z$ is even if $\pi \mid z$, which holds exactly when $a + b$ is even. Dividing by $\pi$ halves the norm: $(a+bi)/(1+i) = ((a+b) + (b-a)i)/2$. Since $2 = -i\pi^2$, the exponent of $\pi$ in an integer is twice the exponent of $2$.
- Map: a triple $(\alpha, k, d)$ with $\alpha$ odd (the multiplier), $k \ge 0$, and a digit table $d$ on odd $z$ that depends only on $z$ modulo $\pi^k$ (the modulus) and makes $\pi^k \mid \alpha z + d(z)$. The digit set $D$ is the set of values of $d$; for $k \ge 1$ it has one odd element in each of the $2^{k-1}$ odd classes, and $d(z)$ is the element of $D$ in the class of $-\alpha z$. The map is $T(z) = z/\pi$ for even $z$ and $T(z) = \alpha z + d(z)$ for odd $z$. The accelerated map on odd $z$ is $S(z) = (\alpha z + d(z))/\pi^v$ with $v \ge k$ the exponent of $\pi$ in $\alpha z + d(z)$, and $S(z) = 0$ if $\alpha z + d(z) = 0$.
- Trivial cycle: a cycle containing a unit. Unit cycle: a cycle made only of units. A trivial cycle that is not a unit cycle contains both units and non-units.
- Collatz map: a map for which every nonzero orbit reaches a trivial cycle. This can be read off $T$ or $S$ alike.
- Commutes with the units: $T(uz) = uT(z)$ for every unit $u$; equivalently $d(uz) = u\,d(z)$, or $D$ closed under multiplication by units.
- Rotated map: $(u\alpha, k, d^u)$ with $d^u(z) = d(uz)$; same digit set. Mirrored map: $(\bar\alpha, k, \bar d)$ with $\bar d(z) = \overline{d(\bar z)}$; digit set $\bar D$.
- Window: $2^k < N(\alpha) < 2^{k+1}$. Above its lower end an odd step followed by exactly $k$ divisions multiplies the norm by $N(\alpha)/2^k > 1$; below its upper end the norm shrinks on average.
- Drift: the mean change of $\log_2 N$ per odd step if the exponents $v - k$ are independent and geometric with mean $1$, that is $\log_2 N(\alpha) - (k+1)$ bits of norm.
- Score: $1/M - 1/N$ with $M = 2^k$ and $N = N(\alpha)$. Inside the window it lies in $(0, 1/(2M))$.

Two maps $(\alpha, k, D)$ and $(\alpha, k, u^{-1}D)$ are conjugate by $z \mapsto uz$, for every map, since $u^{-1}T(uz) = \alpha z + u^{-1}d(uz)$. Counting maps "up to conjugacy" refers to this.

## 2. Collatz maps on $\mathbb{Z}[i]$ without the criterion

Odd norms are $1$ modulo $4$, and the window $(2^k, 2^{k+1})$ contains a norm of an odd element for the first time at $k = 2$, where $N(\alpha) = 5$; at $k = 1$ it would need $N(\alpha) = 3$, which is not a sum of two squares [Paper 1]. Up to units the multipliers inside the window are $2 \pm i$ at $(1+i)^2$; $3$ and $3 \pm 2i$ at $(1+i)^3$; and $4 \pm i$, $5$, $4 \pm 3i$, $5 \pm 2i$ at $(1+i)^4$. This section takes every associate separately, since without the criterion $\alpha$ and $u\alpha$ give different maps.

Two facts from [Paper 1] hold for every map. First, any map on $\mathbb{Z}[i]$ has at most $|U| = 4$ trivial cycles, since distinct cycles are disjoint and each trivial cycle contains its own unit (Lean, `trivial_cycles_le_four`). Second, the drop-below-start check is sound: if every odd $z$, in all four quadrants, with $N_0 < N(z) \le B$ has an iterate under $T$ of smaller norm, then every $z$ with $N(z) \le B$ has an iterate of norm at most $N_0$ (Lean, `drop_below_start_sound_all`). The program `gstopall` performs this check.

### 2.1 Modulus $(1+i)^2$: the $2-i$ map

Classes modulo $\pi^2 = 2i$ are classes modulo $2$, and the two odd classes are those of $1$ ($a$ odd, $b$ even) and $i$ ($a$ even, $b$ odd). The least elements of the class of $1$ are $\pm 1$, and those of the class of $i$ are $\pm i$, so there are four digit sets with digits of norm $1$, and they are the four images $uD$ of $D = \{1, i\}$. Up to conjugacy there is one such map for each of the eight associates of $2 \pm i$. None commutes with the units, since $-1 \equiv 1 \pmod{\pi^2}$ forces $d(-1) = d(1)$ [Paper 1].

All eight were run on every odd start with $N(z) \le 10^5$, with cycles taken literally (`symtest.py`, `logs/symtest_k2_B1e5.txt`; computed to $10^5$):

| $\alpha$ | trivial cycles | other cycles (odd elements) | Collatz map |
|---|---|---|---|
| $2-i$ | $1 \to -i \to -1 \to 1$ | none | yes |
| $-1-2i$ | $1 \to -1 \to -i \to 1$ | none | yes |
| $-2-i$ | $1 \to i \to -1 \to -i \to 1$ | none | yes |
| $1+2i$ | fixed points $-1$ and $-i$ | none | yes |
| $-2+i$ | fixed points $1$ and $-i$ | none | yes |
| $2+i$ | $1 \to -i \to -1 \to i \to 1$ | $5-2i, 1-6i, -5-4i, -6+3i, 7i$ | no |
| $1-2i$ | $\{1, -1\}$, $\{i, -i\}$ | $\{1+2i, -3i\}$, $\{-2-3i, 1+4i\}$ | no |
| $-1+2i$ | four fixed units | five cycles, through $-1-2i$, $2-i$, $3-2i$, $1-4i$, $4+i$ | no |

The four digit sets of each multiplier gave identical cycle counts and basins, as the conjugacy predicts. The conjugate multipliers $2-i$ and $2+i$ get opposite verdicts. For the five maps that pass, `gstopall` checked every odd $z$ in all four quadrants with $1 < N(z) \le 10^7$, $15{,}708{,}048$ starts per map, with no failure and no cycle; the longest stopping times were $203$ to $260$ steps of $T$ and the largest excursions $10.1$ to $11.4$ bits (`logs/symtest_k2_gstopall_1e7.txt`). An independent exact iteration in Python agrees to $N(z) \le 10^4$.

The first of these maps is

$$T(z) = \begin{cases} z/(1+i) & a + b \text{ even}, \\ (2-i)z + i & a \text{ odd}, b \text{ even}, \\ (2-i)z + 1 & a \text{ even}, b \text{ odd}. \end{cases}$$

Its drift is $\log_2(5/8) = -0.68$ bits per odd step, and its score is $1/4 - 1/5 = 0.05$.

*Proposition 2.1 (Lean).* The $2-i$ map is a map at modulus $\pi^2$: its digit is correct for every odd $z$ and depends only on the class. It divides by $\pi$ on even inputs, $T(z) \ne 0$ for $z \ne 0$, and $S(1) = -i$, $S(-i) = -1$, $S(-1) = 1$, $S(i) = -i$. So $1 \to -i \to -1 \to 1$ is a unit cycle, and $i$ is not periodic.

*Proof.* Modulo $2$, $(2-i)z + d \equiv iz + d$, which is $i(a+1) - b \equiv 0$ when $a$ is odd, $b$ even and $d = i$, and $ia - b + 1 \equiv 0$ when $a$ is even, $b$ odd and $d = 1$. For the units, $(2-i) + i = 2 = \pi^2(-i)$, $(2-i)(-i) + 1 = -2i = \pi^2(-1)$, $-(2-i) + i = -2+2i = \pi^3$, and $(2-i)i + 1 = 2+2i = \pi^3(-i)$. The orbit of $i$ under $S$ is $i, -i, -1, 1, -i, \ldots$ and never returns to $i$. For odd $z$, $(2-i)z + d(z) = 0$ would give $5N(z) = N(d(z)) = 1$. The Lean theorems are `digit2_class`, `digit2_correct`, `E2_T_div`, `E2_cycle`, `E2_I_not_periodic` and `E2_T_ne_zero`, and `E2_not_symmetric` records that the map does not commute with the units. $\square$

*Theorem 1 (Lean, given the run).* If every odd $z$ with $1 < N(z) \le B$ has an iterate under $T$ of smaller norm, then every nonzero $z$ with $N(z) \le B$ reaches $1$, $-i$ or $-1$ under the $2-i$ map. The `gstopall` run establishes the hypothesis for $B = 10^7$.

*Proof.* By the soundness of the drop-below-start check with $N_0 = 1$, some iterate has norm at most $1$, so it is $0$ or a unit. It is not $0$ by Proposition 2.1. If it is $i$, then $T^4(i) = -i$, through $i \to 2+2i \to 2 \to 1-i \to -i$. This is `E2_sound`. $\square$

The unit cycle of the $2-i$ map has $3$ elements. Under a map that commutes with the units every unit cycle has as many elements as the order of a unit, $1$, $2$ or $4$, and every unit lies on one (Section 3.3). Here neither holds: the length $3$ is not the order of any unit, and the unit $i$ lies off the cycle and falls into it. Orbits do not enter the cycle evenly. Of the $141{,}324$ odd starts with $N(z) \le 90{,}000$, other than the three cycle elements and $i$, $76{,}511$ enter it at $1$, $40{,}130$ at $-i$ and $24{,}683$ at $-1$, and $17{,}979$ pass through $i$ on the way (`entry2mi.py`; computed).

The mirrored map of the $2-i$ map has multiplier $2+i$ and digit set $\bar D = \{1, -i\} = -i\{i, 1\}$, so it is conjugate to the map $2+i$ of the table, which is not a Collatz map. One step of the two maps is related by $N(S_{\bar\alpha}(z)) = N(S_\alpha(\bar z))$ for every map (Proposition 3.2 below), but the iterates are not, because the unit that conjugation introduces at each division is absorbed only by a map that commutes with the units.

### 2.2 Modulus $(1+i)^3$

At modulus $\pi^3$ the four odd classes each contain exactly one unit, since two distinct units differ by an element of norm $2$ or $4$ and a nonzero multiple of $\pi^3$ has norm at least $8$. A digit table is therefore a list $(d_1, d_i, d_{-1}, d_{-i})$, where $d_u$ is the digit of the class of $u$ and lies in the class of $-\alpha u$. The unit table $D = U$ commutes with the units. The multipliers inside the window are the twelve associates of $3$, $3+2i$ and $3-2i$, and Theorem 4 will show that none of them gives a Collatz map with a digit set closed under units. The question here is whether other tables do.

The search (`m3search.c`, `m3search.py`; `research/collatz/modulus3-search.md`) took, for each class, the six elements of least norm (norms $1, 5, 5, 9, 13, 13$), so $1296$ tables per multiplier, and removed digits that force failure on their own. Two kinds force it. A digit $-\alpha z$ with $z$ odd sends $z$ to $0$. A digit of the form $(\pi^v - \alpha)z$ with $v \ge 3$ and $z$ an odd non-unit makes $z$ a fixed point, by the following lemma, which is the case $\varepsilon = 1$ of Lemma 4.1 and needs no criterion.

*Lemma 2.2 (Lean).* Let $(\alpha, k, d)$ be a map, $v \ge k$, and $z$ odd with $\delta = (\pi^v - \alpha)z \in D$. Then $S(z) = z$. If $z$ is not a unit, the map is not a Collatz map.

*Proof.* The element $\delta$ is congruent to $-\alpha z$ modulo $\pi^k$, so it is the digit of $z$, and $\alpha z + \delta = \pi^v z$ with $z$ odd. A fixed point of norm $N(z) > 1$ is a cycle without a unit. (Lean: `Expr.fixed_point`, `Expr.fixed_point_not_rule`.) $\square$

*Corollary 2.3 (Lean).* If $\pi^v - \alpha = \eta$ is a unit for some $v \ge k \ge 1$, every digit of a Collatz map with multiplier $\alpha$ at modulus $\pi^k$ is a unit.

*Proof.* A digit $\delta$ equals $(\pi^v - \alpha)\eta^{-1}\delta$, and $\eta^{-1}\delta$ is odd; if $\delta$ is not a unit, Lemma 2.2 applies to $z = \eta^{-1}\delta$. (Lean: `Expr.digits_units`.) $\square$

After pruning, $5628$ tables remained, and `m3search` ran each on every odd start with $N(z) \le 10^4$ in all four quadrants. Of these, $1212$ passed, and an independent run through `gcz` with exact re-iteration of every cycle agreed. Each passing table then went through `gstopall` to $N(z) \le 10^6$ ($1{,}570{,}740$ starts each), and $1204$ passed again; up to conjugacy they are $305$ maps. The eight tables that passed at $10^4$ and failed at $10^6$ all have $\alpha = 3-2i$ and fall into two conjugacy classes, each with a nontrivial cycle of $187$ odd steps whose least element has norm about $57{,}000$ (verified in exact arithmetic). All counts are computed, to the bounds stated:

| $\alpha$ | $N(\alpha)$ | pass at $10^4$ | pass at $10^6$ | maps up to conjugacy | tables with $1$, $2$, $3$ trivial cycles | tables with a unit cycle |
|---|---|---|---|---|---|---|
| $3$ | 9 | 0 | 0 | 0 | | |
| $3i$ | 9 | 240 | 240 | 61 | 172, 52, 16 | 136 |
| $-3$ | 9 | 0 | 0 | 0 | | |
| $-3i$ | 9 | 48 | 48 | 13 | 46, 2, 0 | 0 |
| $3+2i$ | 13 | 72 | 72 | 18 | 72, 0, 0 | 24 |
| $-2+3i$ | 13 | 0 | 0 | 0 | | |
| $-3-2i$ | 13 | 124 | 124 | 31 | 32, 60, 32 | 120 |
| $2-3i$ | 13 | 84 | 84 | 21 | 80, 4, 0 | 60 |
| $3-2i$ | 13 | 176 | 168 | 42 | 148, 20, 0 | 92 |
| $2+3i$ | 13 | 272 | 272 | 69 | 268, 4, 0 | 144 |
| $-3+2i$ | 13 | 0 | 0 | 0 | | |
| $-2-3i$ | 13 | 196 | 196 | 50 | 74, 74, 48 | 172 |
| total | | 1212 | 1204 | 305 | | |

None of the five tables per multiplier that commute with the units passes. One example can be checked by hand. For $\alpha = 3i$ take the digits $2-i, -1, -i, 1$ on the classes of $1, i, -1, -i$. Then

$$3i \cdot 1 + (2-i) = 2+2i = \pi^3(-i), \quad 3i(-i) + 1 = 4 = \pi^4(-1), \quad 3i(-1) - i = -4i = \pi^4 i, \quad 3i \cdot i - 1 = -4 = \pi^4,$$

so $1 \to -i \to -1 \to i \to 1$ is a unit cycle through all four units. The table differs from the unit table $(-i, 1, i, -1)$ only in the class of $1$, and every odd $z$ with $N(z) \le 10^7$ drops below its norm under this map, with longest stopping time $155$ steps of $T$ (`logs/m3search_reps_gstopall_1e7.txt`; computed to $10^7$).

How many digits must change? Counting the tables that pass at $10^6$ by their number of non-unit digits, a single changed digit suffices for six of the eight multipliers that have Collatz maps: $4$ such tables for $-3i$, $16$ for $3i$, $8$ for $-2-3i$, $4$ for $2-3i$ (with a digit of norm above $5$), $16$ for $2+3i$ and $8$ for $3-2i$. For $-3-2i$ and $3+2i$ there are none, and at least two digits must change. One representative per multiplier, the passing table with the least total digit norm, was run through `gstopall` to $N(z) \le 10^7$ with no failure.

Trivial cycles need not be unit cycles. For $\alpha = -3i$ no passing table has a unit cycle. The representative with digits $2-3i, 1, i, -1$ has a single trivial cycle of ten elements,

$$1 \to -2+i \to 1-2i \to 1+2i \to -2-i \to 2-i \to -1+2i \to i \to -1 \to -i \to 1,$$

which contains all four units and six of the eight elements of norm $5$; every orbit with $N(z) \le 10^7$ falls into it. A map can also have both kinds: for $\alpha = 3i$ with digits $-2-i, -3-2i, -i, 1$ the unit $1$ is fixed and a second trivial cycle runs through $-1$, $i$, $-i$ and three elements of norm $5$. Tables with three trivial cycles occur for $3i$, $-3-2i$ and $-2-3i$; three does not divide $4$.

For three multipliers no digit table works.

*Theorem 2 (paper).* For $\alpha \in \{-3, -3+2i, -2+3i\}$ there is no Collatz map at modulus $\pi^3$, whatever the digit table.

*Proof.* With $\pi^3 = -2+2i$ and $\pi^4 = -4$, we have $\pi^4 - (-3) = -1$, $\pi^3 - (-3+2i) = 1$ and $\pi^3 - (-2+3i) = -i$, all units. By Corollary 2.3 every digit of a Collatz map is a unit, and since the units lie in distinct classes the table is the unit table. For that table the following literal cycles contain no unit, as direct multiplication confirms. For $\alpha = -3$: $2+i \to 1+2i \to -1+2i \to -2+i \to -2-i \to -1-2i \to 1-2i \to 2-i \to 2+i$, each step with exponent $3$, the eight elements of norm $5$. For $\alpha = -2+3i$: $3 \to 4-i \to 5-2i \to -2-3i \to -3 \to -4+i \to -5+2i \to 2+3i \to 3$, with exponents $3, 3, 5, 4, 3, 3, 5, 4$. For $\alpha = -3+2i$ the cycle through $3$ has sixteen elements, the unit multiples of $3$, $4+i$, $5+2i$ and $2-3i$, of norms $9, 17, 29, 13$. $\square$

For the other nine multipliers $N(\pi^v - \alpha) \ge 5$ for every $v \ge 3$, so Lemma 2.2 removes only digits that are multiples of $\pi^v - \alpha$ by an odd non-unit, and none among the six smallest. For $\alpha = 3$ no table with these digits passes. A wider search, with all $39$ elements of norm at most $100$ in each class, left $10{,}866{,}920$ tables after pruning, of which $1{,}049{,}464$ passed at $10^4$, and in a random sample of $30$ passing tables per multiplier $244$ of $270$ held at $10^6$ (computed). For $\alpha = 3$, $6868$ tables passed at $10^4$, about half a percent, and $19$ of the $30$ sampled failed by $10^6$. No pruning applies to $\alpha = 3$ (Lemma 2.2 removes no digit of norm at most $100$), so a proof that it has no Collatz map would need an argument about longer cycles that depend on the whole table. Whether $\alpha = 3$ admits a Collatz map at modulus $\pi^3$ is open.

### 2.3 Modulus $(1+i)^4$

Classes modulo $\pi^4 = -4$ are classes modulo $4$. Of the eight odd classes, four (those of the unit multiples of $1+2i$) have two elements of least norm $5$: $1+2i$ and $1-2i$, $2+i$ and $-2+i$, and their unit images. So each multiplier has $16$ digit sets with digits of least norm, and they fall into $6$ conjugacy classes; two of the $16$ commute with the units. For the $28$ associates of the seven multipliers inside the window this gives $168$ maps up to conjugacy, all run at $N(z) \le 10^5$ (`logs/symtest_k4_B1e5.txt`; computed). Of these, $35$ pass. Eight commute with the units, the 5+2i map and its mirror in four associates each (Section 4). The other $27$ do not: $16$ have norm $17$ (associates $\pm 1 \pm 4i$ and $4 \pm i$), and $11$ have norm $29$. No map of norm $25$ passes.

The norm-$17$ multipliers have two cycles up to units with either digit set closed under units (Section 5), and a tie-break that does not commute with the units removes the second. For $\alpha = 1+4i$ and $\alpha = 4+i$ with $D = U \cup \{1-2i, -2+i, -2-i, -1-2i\}$, `gstopall` found no failure to $N(z) \le 10^6$ (computed). Four associates, $-4 \pm i$, $-3-4i$ and $-4-3i$, can never pass: for each, $\pi^v - \alpha$ is a unit for some $v \ge 4$, so Corollary 2.3 forces unit digits, and no unit lies in the classes of the unit multiples of $1+2i$.

At norm $29$ all $128$ literal tables with digits of least norm were run for the eight multipliers $u(5 \pm 2i)$ (`variants29.py`, `logs/variants29_1e5.txt`; computed to $10^5$, the same at $2 \cdot 10^4$). Of these, $48$ pass, $8$ commuting with the units and $40$ not. By multiplier:

| $\alpha$ | $5+2i$ | $-2+5i$ | $-5-2i$ | $2-5i$ | $5-2i$ | $2+5i$ | $-5+2i$ | $-2-5i$ |
|---|---|---|---|---|---|---|---|---|
| passing tables (of 16) | 13 | 7 | 1 | 11 | 1 | 9 | 1 | 5 |

The rotated map $(u\alpha, k, d^u)$ has the same digit set as the original, so each column of this table is the rotated image of the first column (or of the fifth) applied to all sixteen tie-breaks at once. A digit set that passes with $5+2i$ may fail with $-5-2i$: $-5-2i$, $5-2i$ and $-5+2i$ pass only with the table that commutes with the units. The table also shows mirroring failing, since the rotations of $5+2i$ pass $32$ times and those of $5-2i$ only $16$.

The trivial cycles of the non-commuting maps of norm $29$ include several that a map commuting with the units cannot have:

1. unit cycles of length $3$: for $\alpha = 2+5i$ with digits $1-2i, -2+i, -2-i, -1-2i$ on the tied classes, $1 \to -i \to i \to 1$; with $1+2i, 2+i, 2-i, -1+2i$, $-1 \to i \to -i \to -1$; and others, such as $1 \to -1 \to i \to 1$;
2. a lone unit $2$-cycle $\{i, -i\}$ as the only trivial cycle, so that $1$ and $-1$ are not periodic;
3. three fixed units, for $\alpha = -2-5i$ with $1-2i, -2+i, 2-i, -1-2i$ (fixed $1$, $i$, $-1$).

A random sample of $80$ tables with non-minimal digits (norm at most $25$ in their classes) and random associates found $12$ that pass at $10^5$, at norms $17$, $25$ and $29$; norm $25$, where no table of least digits passes, has passing maps with $\alpha = -3+4i$, $3+4i$ and $4-3i$ (`logs/symtest_k4_nonmin_sample.txt`; computed).

## 3. Maps that commute with the units

### 3.1 The criterion and what it empties

A map that commutes with the units satisfies $d(uz) = u\,d(z)$, so its digit set is closed under multiplication by units. By [Paper 1] this is the same as $T(uz) = uT(z)$ for all $z$, and it implies $S(uz) = uS(z)$ and $S^n(uz) = uS^n(z)$, since multiplying by a unit does not change the exponent of $\pi$.

[Paper 1] proves, for a Dedekind domain with a prime $P$ and a finite unit group $U$, that a map commuting with $U$ exists at modulus $P^k$ for some odd multiplier exactly when it exists for every odd multiplier, exactly when no unit $u \ne 1$ is congruent to $1$ modulo $P^k$, and exactly when $U$ acts freely on the odd classes. For $\mathbb{Z}[i]$ the largest exponent of $\pi$ in some $u - 1$ is $2$, attained at $u = -1$ since $-2 = i\pi^2$, so maps that commute with the units exist exactly from modulus $\pi^3$ on. The case that matters here has a two-line proof.

*Proposition 3.1 (Lean).* For $k \le 2$ no map at modulus $\pi^k$ commutes with the units. At modulus $1$ the only digit set closed under units is $\{0\}$, so the map sends odd $z$ to $\alpha z$.

*Proof.* Since $-1 - 1 = i\pi^2$, $z$ and $-z$ lie in the same class modulo $\pi^k$ for $1 \le k \le 2$, so $d(-z) = d(z)$, and commuting with $-1$ gives $d(z) = -d(z)$, hence $d(z) = 0$, which is even. At $k = 0$ there is one class and one digit. (Lean: `prop1`, with the key fact `pi_sq_dvd_two_mul`.) $\square$

At modulus $\pi^2$ even a weaker form fails: $S(-z)$ cannot always be an associate of $S(z)$. Since $z$ and $-z$ share a digit $d$, that would make $(\alpha z + d)/(\alpha z - d)$ a unit times a power of $\pi$, and for large $|z|$ this ratio lies within $1/2$ of $1$ without being $1$, while no other unit times a power of $\pi$ does.

So the criterion removes all the maps of Section 2.1, and with them the smallest norm, $5$, at which Collatz maps exist. From modulus $\pi^3$ on the units lie in distinct classes, their differences having exponent at most $2$, and they divide the $2^{k-1}$ odd classes into $2^{k-3}$ orbits of four. A digit set closed under units is the set of unit multiples of one digit chosen in one class of each orbit.

### 3.2 Rotational symmetry and the family of eight maps

Under the criterion the orbit of $uz$ is $u$ times the orbit of $z$. The map acts on classes of associates, one representative per class suffices for a check (the one with $a > 0$, $b \ge 0$), and cycles are counted up to units. Rotated and mirrored maps are where the criterion does further work. Two one-step facts hold for every map.

*Proposition 3.2 (Lean).* For every map and every unit $u$:

1. $S_{u\alpha}(z) = S_\alpha(uz)$ for all $z$, where $S_{u\alpha}$ is the rotated map;
2. for odd $z$ with $w = \alpha\bar z + d(\bar z) \ne 0$ and $v$ the exponent of $\pi$ in $w$, $S_{\bar\alpha}(z) = (-i)^v\,\overline{S_\alpha(\bar z)}$, where $S_{\bar\alpha}$ is the mirrored map; and in all cases $N(S_{\bar\alpha}(z)) = N(S_\alpha(\bar z))$.

*Proof.* (1) For odd $z$, $u\alpha z + d(uz) = \alpha(uz) + d(uz)$, and $uz$ is odd; for even $z$ both sides are $0$. (2) Write $w = \pi^v y$ with $y = S_\alpha(\bar z)$ odd. Then $\bar\alpha z + \bar d(z) = \bar w = \bar\pi^v \bar y = \pi^v(-i)^v\bar y$, since $\bar\pi = -i\pi$, and $(-i)^v\bar y$ is odd. (Lean: `Expr.rotate_S`, `Expr.mirror_S`, `Expr.mirror_norm`.) $\square$

The unit $(-i)^v$ comes from $\overline{z/\pi} = i\,\bar z/\pi$ at each division. For the second step of an orbit the mirrored map is applied to $(-i)^v\overline{S_\alpha(\bar z)}$ rather than to $\overline{S_\alpha(\bar z)}$, and a map that does not commute with the units treats the two differently. In the same way the rotated map applies $S_\alpha$ to $u S_{u\alpha}(z)$ at the second step, and $S_\alpha(uy)$ need not be related to $S_\alpha(y)$. That is why the columns of the norm-$29$ table in Section 2.3 differ. To pass from one step to whole orbits we need the following transfer.

*Lemma 3.3 (Lean).* Let $g$ commute with the units, let $f$ be any map, and suppose that for every $z$ there is a unit $\varepsilon$ with $g(z) = \varepsilon f(z)$. If every odd orbit of $f$ reaches a trivial cycle, so does every odd orbit of $g$.

*Proof.* By induction $g^n(z) = \delta_n f^n(z)$ with $\delta_n$ a unit, since $g(\delta_n f^n(z)) = \delta_n g(f^n(z))$. Let $y = f^n(z)$ satisfy $f^p(y) = y$ with $f^j(y)$ a unit, and put $x = g^n(z) = \delta y$. Then $g^p(x) = \delta' x$ for a unit $\delta'$, so $g^{4p}(x) = \delta'^4 x = x$, and $g^j(x)$ is a unit times $f^j(y)$, hence a unit. (Lean: `trivial_transfer`.) $\square$

*Theorem 3 (Lean).* Let $(\alpha, k, d)$ commute with the units, and let $u$ be a unit.

1. The rotated map commutes with the units, $S_{u\alpha}(z) = uS_\alpha(z)$, and $S_{u\alpha}^n(z) = u^n S_\alpha^n(z)$. So the norm sequence of every $z$ is the same under both maps, and one is a Collatz map exactly when the other is.
2. The mirrored map commutes with the units, and
$$S_{\bar\alpha}^n(z) = (-i)^{V_n(\bar z)}\,\overline{S_\alpha^n(\bar z)}, \qquad V_n(y) = \sum_{j=0}^{n-1} v\left(\alpha S_\alpha^j(y) + d(S_\alpha^j(y))\right),$$
so the norm sequence of $z$ under the mirrored map is that of $\bar z$ under the original, and one is a Collatz map exactly when the other is.

*Proof.* (1) $d^u(vz) = d(uvz) = v\,d^u(z)$, and by Proposition 3.2, $S_{u\alpha}(z) = S_\alpha(uz) = uS_\alpha(z)$. Induction gives $S_{u\alpha}^{n+1}(z) = uS_\alpha(u^nS_\alpha^n(z)) = u^{n+1}S_\alpha^{n+1}(z)$. Lemma 3.3 with $\varepsilon = u$ and $\varepsilon = u^{-1}$ gives the equivalence. (2) $\bar d(uz) = \overline{d(\bar u\bar z)} = \overline{\bar u\,d(\bar z)} = u\,\bar d(z)$. For the formula, Proposition 3.2 gives $S_{\bar\alpha}(z) = (-i)^{v_0}\overline{S_\alpha(\bar z)}$, and since the mirrored map commutes with the units, the induction hypothesis applied at $\overline{S_\alpha(\bar z)}$ gives the step from $n$ to $n+1$. For the equivalence, $h(z) = \overline{S_\alpha(\bar z)}$ commutes with the units and is a Collatz map exactly when $S_\alpha$ is, since conjugation preserves parity, periodicity and units, and $S_{\bar\alpha} = \varepsilon(z)h(z)$ with a unit $\varepsilon(z)$, so Lemma 3.3 applies both ways. (Lean: `Expr.rotate_symmetric`, `rotate_S_mul`, `rotate_iter`, `rotate_norm`, `rotate_rule`, `Expr.mirror_symmetric`, `mirror_iter`, `mirror_norm_iter`, `mirror_rule`.) $\square$

So every map that commutes with the units belongs to a family of eight, its four rotated maps and the four rotations of its mirrored map, all with the same norm sequences up to $z \mapsto \bar z$ and all Collatz maps or none. If $D$ is closed under conjugation, the mirrored map has the same digit set. In the census we count maps up to rotation and, where stated, up to mirroring. For the family of the 5+2i map (Section 4.2) the script `mirror29.py` compared the first $60$ norms of every odd $z$ with $|a|, |b| \le 60$ under each of the eight maps against those of $z$ or $\bar z$ under the 5+2i map: $58{,}560$ comparisons and no mismatch (computed).

### 3.3 Unit cycles and the number of trivial cycles

*Proposition 3.4 (Lean).* Let $S$ commute with the units, and put $w = S(1)$.

1. If $w$ is a unit, then $S^n(u) = uw^n$ for every unit $u$. Every unit lies on a unit cycle, every unit cycle has length $\text{ord}(w)$, which is $1$, $2$ or $4$, and there are $4/\text{ord}(w)$ unit cycles. If $w$ is not a unit, there is no unit cycle.
2. If $1$ is not periodic, no unit is, and there is no trivial cycle. If $1$ lies on the cycle $C_1$, the trivial cycles are the sets $uC_1$, and $U \cap C_1$ is a subgroup of $U$, the stabilizer of $C_1$. Their number is $4/|U \cap C_1|$: four if $U \cap C_1 = \{1\}$, two if it is $\{\pm 1\}$, one if it is $U$.

*Proof.* (1) $S(u) = uS(1) = uw$, and by induction $S^n(u) = uw^n$; if $w$ is a unit then $uw^n = u$ exactly when $w^n = 1$, and if not, $N(uw) = N(w) \ne 1$. (2) $S^p(u) = u$ exactly when $S^p(1) = 1$, and the cycle of $u$ is $uC_1$. A trivial cycle contains a unit $u$, so it is $uC_1$. If $uC_1 = C_1$ then $u \in C_1$, and conversely if $u \in C_1$ then $uC_1$ and $C_1$ are cycles through $u$ and coincide. The set $U \cap C_1$ contains $1$ and is closed under products, since $u, u' \in C_1$ gives $uu' \in uC_1 = C_1$, so it is a subgroup of the cyclic group $U$, and $uC_1 = u'C_1$ exactly when $u^{-1}u'$ lies in it. (Lean: `unit_cycles`, `no_unit_cycle`, `unit_periodic_iff`, `unit_cycle_eq`, `stab_iff`, `stab_mul`, `trivial_cycle_count`.) $\square$

The general bound of [Paper 1], at most four trivial cycles, thus becomes a divisor of $4$ under the criterion, and the cycle counts of Section 2 show that both parts need it: the $2-i$ map has a unit cycle of length $3$ with $S(1)$ a unit, and maps of modulus $(1+i)^3$ and $(1+i)^4$ have three trivial cycles. The number depends on the associate of $\alpha$, since by Theorem 3 rotating by $u$ replaces $S(1) = w$ by $uw$. Table 4.2 shows all three counts within the family of the 5+2i map, and the below-window map $\alpha = 1+2i$ at $(1+i)^3$ with $D = U$ has $S(1) = -i$ and one unit cycle through all four units. Under the criterion a trivial cycle that contains both units and non-units occurs only when $S(1)$ is not a unit. In the maps of the census at moduli $(1+i)^3$ to $(1+i)^5$ and in the single-cycle maps at $(1+i)^7$ the cycle through $1$ has one odd step, so $S(1)$ is a unit there.

### 3.4 Smallest digits and the criterion

The census of Section 5 uses digits of least norm in their classes. Where these are unique the criterion comes for free.

*Lemma 3.5 (paper).* If every odd class modulo $\pi^k$ has a unique element of least norm, the digit set of least elements commutes with the units.

*Proof.* The class of $-\alpha uz$ is $u$ times the class of $-\alpha z$, and $w \mapsto uw$ preserves norms, so the least element of the first is $u$ times the least element of the second. $\square$

*Proposition 3.6 (paper).* The elements of least norm in a class modulo $\Lambda = \pi^k\mathbb{Z}[i]$ lie in the closed Voronoi cell of $\Lambda$ at $0$, and two of them can be tied only on its boundary.

1. For $k = 2j+1$ the cell is the square $|x| + |y| \le 2^j$, whose boundary points satisfy $x + y \equiv 2^j \pmod 2$. For $j \ge 1$ they are even, so every odd class has a unique least element.
2. For $k = 2j$ the cell is $|x|, |y| \le 2^{j-1}$. Its odd boundary points are $\pm 2^{j-1} + iy$ with $y$ odd and the points with the roles exchanged. For $j \ge 2$ this gives $2^j$ tied classes with two least elements each, $2^{2^j}$ digit sets of least elements, and $2^{2^{j-2}}$ of them closed under units.

*Proof.* Two least elements of a class differ by a nonzero vector of $\Lambda$ and are equidistant from $0$ and from that vector, so they lie on the bisector, which is part of the boundary. For $k = 2j+1$, $\Lambda = (1+i)2^j\mathbb{Z}[i]$ and the cell is the stated square, and for $k = 2j$, $\Lambda = 2^j\mathbb{Z}[i]$. The parity statements are direct. For $j \ge 2$ the units act freely on the tied classes, so a choice closed under units is one choice for each of the $2^j/4$ orbits. $\square$

At $(1+i)^2$ the case $j = 1$ is special: the classes of $1$ and $i$ are tied, with $\pm 1$ and $\pm i$, and $-1$ swaps the two elements of each pair, so no tie-break commutes with the units, as Proposition 3.1 requires. The counts (`symtest.py ties`; checked by enumeration):

| modulus | odd classes | tied classes (norm) | digit sets of least elements | closed under units | conjugacy classes |
|---|---|---|---|---|---|
| $(1+i)^2$ | 2 | 2 (1) | 4 | 0 | 1 |
| $(1+i)^3$ | 4 | 0 | 1 | 1 | 1 |
| $(1+i)^4$ | 8 | 4 (5) | 16 | 2 | 6 |
| $(1+i)^5$ | 16 | 0 | 1 | 1 | 1 |
| $(1+i)^6$ | 32 | 8 (four of 17, four of 25) | 256 | 4 | 70 |
| $(1+i)^7$ | 64 | 0 | 1 | 1 | 1 |

The last column counts orbits under $D \mapsto uD$, by Burnside's lemma $(16+4+2+2)/4 = 6$ and $(256+16+4+4)/4 = 70$. So at odd moduli the digits of least norm already commute with the units, and the census there contains every Collatz map with such digits. At even moduli most tie-breaks do not commute, and Section 2.3 found Collatz maps among them.

The digit sets of least elements that commute with the units are these. At $(1+i)^3$, the units. At $(1+i)^4$, the units with the unit multiples of $2+i$, or with those of $1+2i$; the two are complex conjugates. At $(1+i)^5$, the unit multiples of $1$, $3$, $1+2i$, $1-2i$. At $(1+i)^6$, the unit multiples of $1$, $3$, $1 \pm 2i$, $2 \pm 3i$, with those of $1+4i$ or $1-4i$ and of $3+4i$ or $3-4i$; four sets, none closed under conjugation. Following the notes we call them A (with $1-4i$, $3-4i$), B (with $1-4i$, $3+4i$), C (with $1+4i$, $3-4i$) and D (with $1+4i$, $3+4i$); A and D are mirror images, and so are B and C. At $(1+i)^7$, the unit multiples of $1, 3, 5, 7, 1 \pm 2i, 1 \pm 4i, 1 \pm 6i, 2 \pm 3i, 2 \pm 5i, 3 \pm 4i$, a single set closed under conjugation. Each set was enumerated and checked by an independent division test.

## 4. The smallest commuting Collatz map

### 4.1 No commuting Collatz map below norm 29

The fixed-point lemma of Section 2.2 extends to a unit factor once the map commutes with the units.

*Lemma 4.1 (Lean).* Let $(\alpha, k, d)$ be a map, $v \ge k$, $\varepsilon$ a unit and $z$ odd with $\delta = (\varepsilon\pi^v - \alpha)z \in D$. Then $S(z) = \varepsilon z$. If $z$ is not a unit and the map commutes with the units or $\varepsilon = 1$, the map is not a Collatz map.

*Proof.* As in Lemma 2.2, $\delta$ is the digit of $z$ and $\alpha z + \delta = \pi^v(\varepsilon z)$. Under the criterion $S^n(z) = \varepsilon^n z$, so the cycle of $z$ lies in $\{z, iz, -z, -iz\}$, all of norm $N(z) > 1$. (Lean: `Expr.fixed_point`, `Expr.fixed_point_not_rule`.) $\square$

*Corollary 4.2 (Lean).* If $\varepsilon\pi^v - \alpha$ is a unit for some $v \ge k \ge 1$ and the map commutes with the units, a Collatz map with multiplier $\alpha$ at modulus $\pi^k$ has only unit digits. (Lean: `Expr.digits_units`.)

*Lemma 4.3 (Lean).* If $\alpha$ is odd with $2^k < N(\alpha) < 2^{k+1}$ and $N(\alpha) < 29$, then up to a unit $\alpha$ is $2 \pm i$ ($k = 2$); $3$ or $3 \pm 2i$ ($k = 3$); or $4 \pm i$, $5$ or $4 \pm 3i$ ($k = 4$).

*Proof.* $N(\alpha)$ is odd and below $32$, so $k \le 4$, and $k \le 1$ is impossible because $3$ is not a sum of two squares. The odd sums of two squares in $(4, 8)$, $(8, 16)$ and $(16, 29)$ are $5$; $9, 13$; $17, 25$. (Lean: `alpha_k3`, `alpha_k4`.) $\square$

*Theorem 4 (Lean).* A map that commutes with the units, lies inside the window and has $N(\alpha) < 29$ is not a Collatz map, whatever its digit table.

*Proof.* By Lemma 4.3, $k \in \{2, 3, 4\}$, and $k = 2$ is excluded by Proposition 3.1. By Theorem 3 we may rotate $\alpha$ to the listed representative. With $\pi^3 = -2+2i$, $\pi^4 = -4$ and $\pi^5 = -4-4i$, in each case $\eta = \varepsilon\pi^v - \alpha$ is a unit:

| $k$ | $\alpha$ | $\varepsilon$ | $v$ | $\varepsilon\pi^v$ | $\eta$ |
|---|---|---|---|---|---|
| 3 | $3$ | $-1$ | 4 | $4$ | $1$ |
| 3 | $3+2i$ | $-i$ | 3 | $2+2i$ | $-1$ |
| 3 | $3-2i$ | $-1$ | 3 | $2-2i$ | $-1$ |
| 4 | $4+i$ | $-1$ | 4 | $4$ | $-i$ |
| 4 | $4-i$ | $-1$ | 4 | $4$ | $i$ |
| 4 | $5$ | $-1$ | 4 | $4$ | $-1$ |
| 4 | $4+3i$ | $-1$ | 5 | $4+4i$ | $i$ |
| 4 | $4-3i$ | $i$ | 5 | $4-4i$ | $-i$ |

By Corollary 4.2 a Collatz map would have only unit digits. At $k = 4$ that is impossible: each $\alpha$ in the table is congruent modulo $4$ to a unit $u'$, and the digit $e$ of $z_0 = 1+2i$ would need $4 \mid u'(1+2i) + e$, that is, $1+2i \equiv -u'^{-1}e \pmod 4$; but $1+2i$ minus a unit is $2i$, $1+i$, $2+2i$ or $1+3i$, none divisible by $4$. At $k = 3$ the digit set is $U$. Writing $z \xrightarrow{e,\,v} S(z)$ for $\alpha z + e = \pi^v S(z)$, direct multiplication gives

1. $\alpha = 3$: $-2-i \xrightarrow{i,\,3} 1+2i \xrightarrow{-1,\,3} 1-2i = i(-2-i)$;
2. $\alpha = 3+2i$: $-3 \xrightarrow{-1,\,3} 1+4i \xrightarrow{-1,\,3} 5-2i \xrightarrow{1,\,5} -3+2i \xrightarrow{1,\,4} 3 = -(-3)$;
3. $\alpha = 3-2i$: $-3 \xrightarrow{-1,\,3} 4+i \xrightarrow{-i,\,3} -5-2i \xrightarrow{-1,\,5} 2-3i \xrightarrow{i,\,4} 3i = -i(-3)$.

Since the map commutes with the units, the unit multiples of the points listed form a set mapped into itself, and it contains no unit (norms $5$; $9, 17, 29, 13$; $9, 17, 29, 13$). So the orbit of $-2-i$, respectively $-3$, never reaches a trivial cycle. (Lean: `Expr.no_symmetric_rule_below_29`, with `units_mod_pi3`, `Expr.no_rule_of_cycle3`, `Expr.no_rule_k4`; the cycles are checked by `decide`.) $\square$

The theorem uses no condition on the size of the digits. Both ends of the window are needed. Below it, $\alpha = 1+2i$ at $(1+i)^3$ also has $\eta = -i\pi^3 - \alpha = 1$, so its digits are units, and with $D = U$ it is a Collatz map: $|S(z)| \le (\sqrt 5|z| + 1)/2^{3/2} < |z|$ once $|z| > 1.69$, every odd non-unit has norm at least $5$, and $S(1) = -i$ (paper; [Paper 1] treats such maps). Without the criterion, Sections 2.1 to 2.3 found Collatz maps at norms $5$, $9$, $13$, $17$ and $25$.

At norm $29$ the criterion admits Collatz maps, and even some with large digits. With digits closed under units chosen among the five smallest elements of one class per orbit, $175$ maps at $(1+i)^4$ were run at $10^5$ and three passed: the 5+2i map, its mirror, and $\alpha = -5-2i$ with $D = U \cup (5-2i)U$, whose non-unit digits have norm $29$ (`logs/symtest_G_symnonmin_k3-4.txt`; computed). The last has four fixed units, and every odd $z$ with $1 < N(z) \le 10^7$ drops below its norm, with stopping times up to $9754$ steps of $T$ and excursions up to $80.4$ bits (computed to $10^7$). At $(1+i)^3$ the same search with $40$ candidates per orbit, $120$ maps, found none, as Theorem 4 requires.

### 4.2 The 5+2i map

The 5+2i map has $\alpha = 5+2i$, $N(\alpha) = 29$ and modulus $\pi^4 = -4$, so $16 < 29 < 32$ and classes are taken modulo $4$. Its digit set is

$$D = U \cup (2+i)U = \{\pm 1, \pm i, \pm(2+i), \pm(1-2i)\},$$

the digits of least norm closed under units (Section 3.4), and the map is

$$T(z) = \begin{cases} z/(1+i) & \text{if } (1+i) \mid z, \\ (5+2i)z + d(z) & \text{otherwise,} \end{cases}$$

where $d(z)$ is the element of $D$ that makes $(5+2i)z + d(z)$ divisible by $4$:

| $z \bmod 4$ | $1$ | $i$ | $-1$ | $-i$ | $1+2i$ | $2+i$ | $-1-2i$ | $2-i$ |
|---|---|---|---|---|---|---|---|---|
| $d(z)$ | $-1+2i$ | $-2-i$ | $1-2i$ | $2+i$ | $-1$ | $-i$ | $1$ | $i$ |

The table follows from one congruence. Modulo $4$, $\alpha \equiv 1+2i$, and $(1+2i)^2 = -3+4i \equiv 1$. If $z \equiv u$ with $u$ a unit, the digit is congruent to $-(1+2i)u$, a class with no unit, whose least elements are unit multiples of $2+i$. If $z \equiv (1+2i)u$, the digit is congruent to $-u$. Multiplication by $\alpha$ swaps the two orbits of classes, so the digits alternate between units and associates of $2+i$. From $1$: $(5+2i) + (-1+2i) = 4+4i = -\pi^5$, so $S(1) = -1$ and $S(-1) = 1$. Where $3x+1$ has one digit, the 5+2i map has eight, because the window puts the multiplier $29$ at modulus $(1+i)^4$, which has $2^3$ odd classes.

*Proposition 4.4 (Lean).* The table is a correct digit table for every odd $z$, its eight representatives are exactly the odd classes modulo $4$, the map divides by $\pi$ on even inputs, it commutes with the units, and $1 \to 4+4i \to \cdots \to -1 \to \cdots \to 1$, one odd step and five divisions each way. (Lean: `pi_pow_four`, `four_dvd_class`, `tableH_reps`, `tableH_distinct`, `tableH_covers`, `digitH_correct`, `H_cycle`, `H_div`, `digitH_unit_table`, `digitH_unit`, `H_unit`.)

Orbits rise and fall as those of $3x+1$ do. Under $S$,

$$3 \to -4-i \to 4+3i \to -3+2i \to 2-3i \to -4+3i \to -2-i \to -i,$$

with norms $9, 17, 25, 13, 13, 25, 5, 1$ and exponents $4, 4, 6, 5, 4, 7, 7$. The start $6827+4464i$ takes $2042$ odd steps, $11271$ steps of $T$, to reach a unit, with an excursion of $97.7$ bits above its starting norm.

*Table 4.2.* The family of the 5+2i map. The rotations have the digit set $D$, the rotations of the mirrored map the digit set $\bar D = U \cup (2-i)U$. By Theorem 3 all eight have the same norm sequences up to $z \mapsto \bar z$, and by Proposition 3.4 the value of $S(1)$ fixes the trivial cycles (computed by direct multiplication, and confirmed at $N(z) \le 10^5$ by `variants29.py`).

| map | $S(1)$ | trivial cycles |
|---|---|---|
| $5+2i$ | $-1$ | two: $\{1, -1\}$, $\{i, -i\}$ |
| $-2+5i$ | $-i$ | one: $1 \to -i \to -1 \to i \to 1$ |
| $-5-2i$ | $1$ | four fixed units |
| $2-5i$ | $i$ | one: $1 \to i \to -1 \to -i \to 1$ |
| $5-2i$ (mirror) | $i$ | one: $1 \to i \to -1 \to -i \to 1$ |
| $2+5i$ | $-1$ | two: $\{1, -1\}$, $\{i, -i\}$ |
| $-5+2i$ | $-i$ | one: $1 \to -i \to -1 \to i \to 1$ |
| $-2-5i$ | $1$ | four fixed units |

The mirror has $S(1) = (-i)^5\,\overline{S_{5+2i}(1)} = i$ by Proposition 3.2. All trivial cycles are unit cycles. The other digit set of least elements, $\bar D$ with $\alpha = 5+2i$, gives a different map, the mirror of $(5-2i, D)$; it has three cycles up to units (Section 5).

*Conjecture 1 (the 5+2i problem).* Every orbit of the 5+2i map on $\mathbb{Z}[i] \setminus \{0\}$ reaches $\pm 1$ or $\pm i$. Equivalently, it is a Collatz map, and up to units it has a single cycle; on $\mathbb{Z}[i]$ the cycles are $\{1, -1\}$ and $\{i, -i\}$.

By Theorem 3 the conjecture holds for one map of Table 4.2 exactly when it holds for all eight.

*Verification (computer-assisted).* The program `gcz` classified every $z$ with $N(z) \le 10^7$ and found only the cycles through $\pm 1$ and $\pm i$. Then `gstop` checked every odd $z = a+bi$ with $a > 0$, $b \ge 0$ and $10^7 < N(z) \le 3 \cdot 10^9$, $1{,}174{,}170{,}307$ starts in $30$ chunks, and each fell below its own norm, with no other cycle, in about $15$ minutes on one core (`logs/alpha5p2i_k4.stop`). The soundness of this reduction is proved in Lean.

*Theorem 4.5 (Lean).* If every odd $z$ with $a > 0$, $b \ge 0$ and $N_0 < N(z) \le B$ has an iterate under the 5+2i map of smaller norm, then every $z$ with $N(z) \le B$ has an iterate of norm at most $N_0$. (Lean: `H_sound`, which is `drop_below_start_sound` applied to the map, using `H_div` and `H_unit`.)

With $N_0 = 10^7$ and $B = 3 \cdot 10^9$, and the `gcz` classification below $10^7$, every nonzero $z$ with $N(z) \le 3 \cdot 10^9$ reaches $\pm 1$ or $\pm i$. What Lean does not check is that the two runs established their hypotheses; Section 7 lists what is trusted.

Over the range of the check the longest stopping time is $13913$ steps of $T$, at $6742+34011i$; the mean stopping time is $53.9$ steps, between $53.82$ and $53.93$ in every chunk; and the largest excursion, $\log_2$ of the peak norm before the drop divided by the starting norm, is $122.9$ bits, at $17467+5226i$. The largest coordinates met were below $2^{80}$.

### 4.3 Drift and excursions

Each odd step of the 5+2i map multiplies the norm by about $29$ and is followed by at least four divisions, each halving it, and on average five. The drift is $\log_2 29 - 5 = -0.142$ bits per odd step. For $3x+1$, measured on $x^2$ so that the sizes compare, it is $\log_2(9/16) = -0.83$ bits, so the 5+2i map contracts about six times more slowly, and its orbits can climb far before they fall.

A random-walk model fits the data (heuristic). Treat the excess divisions $v - 4$ at successive odd steps as independent geometric variables with $P(v - 4 \ge j) = 2^{-j}$; [Paper 1] shows that this is their distribution in density over odd $z$. Then $\log_2 N$ walks with drift $-0.142$ and variance $2$ per odd step. Simulating $4 \cdot 10^6$ walks gives a mean of $53.8$ steps of $T$ until the walk first falls below its start, against $53.82$ to $53.93$ measured. For the excursions, the probability of climbing $h$ bits above the start before stopping is about $Ce^{-\theta h}$, where $\theta = 0.165$ per bit solves $\mathbb{E}[e^{\theta X}] = 1$ for the change $X$ per odd step, and simulation gives $C \approx 0.36$ with the peak taken just after each multiplication. Over $n$ independent starts the record has median about $(\ln n + \ln C - \ln\ln 2)/\theta$, which for the $1.17 \cdot 10^9$ starts of the check is about $122.4$ bits, against $122.9$ observed. This agreement is closer than the model justifies: the starts are not independent, and the records of $14$ of the $30$ chunks come from orbits through one point of norm about $2^{151.2}$ (checked for five of them). The mean stopping time is the better test.

### 4.4 The backward form

Lagarias restates $3x+1$ through the inverse maps $2x$ and $(2x-1)/3$: the integers in the orbit of $4$ under the semigroup they generate form the backward tree of $1$ [Lagarias 2016]. The 5+2i map has the same form. Take the version that performs all four divisions at once, $z \mapsto (\alpha z + d(z))/\pi^4$ on odd $z$ and $z \mapsto z/\pi$ on even $z$; its orbits are subsequences of those of $T$, the skipped values are even, and the two reach a unit together. Its inverse branches are

$$w \mapsto (1+i)w \quad \text{and} \quad w \mapsto \frac{-4w - d}{5+2i}, \quad d \in D.$$

If $-4w - d$ is divisible by $5+2i$, the quotient $z$ satisfies $\alpha z + d = \pi^4 w$, which forces $z$ odd and $d$ its digit. If $w$ has $(5+2i)^j$ with $j \ge 1$ in its denominator, so does $-4w - d$, since $4$ and $d$ are prime to $5+2i$, and the second branch raises $j$; the first leaves it unchanged. So a non-integer never returns, and the Gaussian integers in the semigroup orbit are exactly the backward tree (paper).

*Conjecture 1, backward form.* The Gaussian integers in the orbit of $\{1, i\}$ under the semigroup generated by $w \mapsto (1+i)w$ and $w \mapsto (-4w-d)/(5+2i)$, $d \in D$, are all of $\mathbb{Z}[i] \setminus \{0\}$.

The set $\{1, i\}$ suffices because $-1$ and $-i$ are images of $1$ and $i$ under backward steps. The second branch multiplies the norm by $16/29 < 1$, so the orbit is not discrete, and density arguments of Erdős's kind for semigroups of expanding affine maps do not apply, as for $3x+1$. Each branch is an affine map of $\mathbb{Z}^2$, a $3 \times 3$ integer matrix, and also a $2 \times 2$ upper triangular matrix over $\mathbb{Z}[i]$. Freeness of semigroups is undecidable for $3 \times 3$ integer matrices in general [Klarner, Birget and Satterfield 1991], and open for $2 \times 2$ upper triangular integer matrices, the form of affine maps of $\mathbb{Z}$ [Cassaigne, Harju and Karhumäki 1999]; neither result settles freeness for the branches here.

## 5. The census

### 5.1 Method and summary

The census contains every map that commutes with the units, lies inside the window, and has digits of least norm, at moduli $(1+i)^3$ to $(1+i)^7$; every choice among tied digit sets is included. Maps are counted up to rotation and, in parentheses, up to rotation and mirroring; cycles are counted up to units. For each map `gcz` followed every start with $N(z) \le B$ to its cycle, and every cycle it reported was iterated again in exact arithmetic (`exact.py`), with each digit taken from the congruence instead of the table, before cycles were grouped. A cycle count is the number of cycles reached from starts with $N(z) \le B$, so it is a lower bound, except for the four maps of Theorem 5. All counts are computed.

| modulus | maps | single cycle | cycles of the other maps | checked to |
|---|---|---|---|---|
| $(1+i)^3$ | 3 (2) | 0 | 5, 4 | $10^9$ |
| $(1+i)^4$ | 14 (7) | 2 (1): the 5+2i map | 2, 2, 3, 3, 6; $\alpha = 5$: at least 35, and orbits that appear to diverge | $10^6$; the 5+2i map to $3 \cdot 10^9$ |
| $(1+i)^5$ | 11 (6) | 0 | 2, 3, 3, 4, 4, 18 | $10^6$ |
| $(1+i)^6$ | 112 (56) | 22 (11) | 2 to 10; $\alpha = 9$: 23, 25; $\alpha = 11$: 560, 568, at least 639 for one digit set; both real multipliers with orbits that appear to diverge | $10^6$ |
| $(1+i)^7$ | 46 (24) | 8 (4) | 2 to 33; $\alpha = 13$: 166; $15$: 74; $11 \pm 10i$: 766 | $10^6$; single-cycle maps and the maps of Theorem 5 to $10^9$ |

Modulus $(1+i)^4$ is the first with a single-cycle map, the 5+2i map and its mirror are the only ones there, and the moduli $(1+i)^3$ and $(1+i)^5$ have none. The fractions of single-cycle maps are $0/3$, $2/14$, $0/11$, $22/112$ and $8/46$. From $(1+i)^6$ on single-cycle maps are common, so what sets the 5+2i map apart is that it comes first: every map of smaller norm inside the window and commuting with the units fails by Theorem 4, and the other maps at its modulus have at least two cycles.

*Proposition 5.1 (computer-assisted).* Among the maps of the census at moduli $(1+i)^3$ to $(1+i)^5$, every map other than the 5+2i map and its mirror has at least two cycles up to units.

*Proof.* Tables 5.2 and 5.3 exhibit two cycles for each, and each cycle was verified in exact arithmetic. $\square$

The corrected figures in this section differ from the earlier drafts in four places: $\alpha = 5$ at $(1+i)^4$ has at least $35$ cycles (earlier $36$ or more, then $34$, before a print limit was fixed); the counts at $(1+i)^7$ for $15$ and $11 \pm 10i$ are $74$ and $766$ (the sweep to $10^6$ found $72$ and $545$); the checks of the two maps at $(1+i)^3$ above $10^6$ had $392{,}306{,}389$ starts each; and the check of the 5+2i map had $1.17 \cdot 10^9$ starts.

### 5.2 Moduli $(1+i)^3$ to $(1+i)^5$

At $(1+i)^3$ the digit set is $U$, and Theorem 4 says no map is a Collatz map; the census adds the cycles. For $\alpha = 3$ there are five cycles up to units at $N(z) \le 10^9$: $2+i \to 1+2i$ of two odd steps, which receives $92.3\%$ of the starts with $N(z) \le 10^6$; $5+2i \to 2+5i$, $5.6\%$; a mirror pair of five odd steps through $19+6i$ and $6+19i$, $1.0\%$ each; and the class of $1$, only $0.2\%$, falling to $0.07\%$ at $10^7$. For $\alpha = 3+2i$ there are four: the class of $1$ ($63.4\%$), cycles of four odd steps through $3$ ($22.9\%$), $7$ ($12.8\%$) and $8+7i$ ($0.8\%$). Both were checked by `gstop` over $10^6 < N(z) \le 10^9$ with no further cycle (`logs/G3_k3.stop`, `logs/G3p2i_k3.stop`); $\alpha = 3-2i$ is the mirror of $3+2i$.

*Table 5.2.* Moduli $(1+i)^4$ and $(1+i)^5$, digit sets of least norm, cycles up to units at $N(z) \le 10^6$, with two cycles given by a first-quadrant element and their length in odd steps. At $(1+i)^4$ the digit set is $U \cup (2+i)U$; the other gives the mirrors. At $(1+i)^5$ the maps $6-i$, $5-4i$, $6-3i$, $7-2i$, $6-5i$ are the mirrors of those listed, and $\alpha = 7$ is its own.

| modulus | $\alpha$ | cycles | two cycles (odd steps) |
|---|---|---|---|
| $(1+i)^4$ | $4+i$ | 2 | $1$ (1), $2+i$ (1) |
| | $4-i$ | 2 | $1$ (1), $2+i$ (1) |
| | $5$ | at least 35 | $1$ (1), $2+i$ (1) |
| | $4+3i$ | 3 | $1$ (1), $6+3i$ (11) |
| | $4-3i$ | 6 | $1$ (1), $4+i$ (12) |
| | $5+2i$ | 1 | $1$ (1) |
| | $5-2i$ | 3 | $1$ (1), $3+4i$ (6) |
| $(1+i)^5$ | $6+i$ | 2 | $1$ (1), $5$ (4) |
| | $5+4i$ | 4 | $1$ (1), $2+i$ (1) |
| | $6+3i$ | 3 | $1$ (1), $2+i$ (2) |
| | $7$ | 18 | $1$ (1), $2+i$ (1) |
| | $7+2i$ | 4 | $1$ (1), $2+i$ (3) |
| | $6+5i$ | 3 | $1$ (1), $2+i$ (7) |

Apart from $\alpha = 5$ the counts are the same at $10^5$. For $\alpha = 5$, $10.5\%$ of the starts passed coordinates $2^{100}$ (Section 6.4). Two cycles are long: $\alpha = 5-2i$ has one of $70$ odd steps through $15+16i$, and $\alpha = 6+5i$ one of $130$ odd steps through $164+199i$. The latter map, with drift $\log_2(61/64) = -0.07$ bits, has excursions of $117.8$ bits and orbits of up to $27{,}206$ steps of $T$ below $10^6$.

Lemma 4.1 explains many of the cycles of one odd step. A digit $\delta = (\varepsilon\pi^v - \alpha)z$ gives the fixed class of $z$, a trivial cycle when $z = 1$ and a nontrivial one otherwise. For the 5+2i map, $-\pi^5 - (5+2i) = -1+2i = i(2+i)$ lies in $D$, which gives the class of $1$ with exponent $5$, and no other $z$ of this kind arises. At $(1+i)^6$, $8 \pm i$ lies at distance $1$ from $8 = i\pi^6$, so every digit gives a fixed class, at least eight per map; for $\alpha = 9$ at $(1+i)^6$ and $15$ at $(1+i)^7$ every digit gives one. Lemma 4.1 also explains a difference from $\mathbb{Z}$: there the smallest multiplier $2^k+1$ of a modulus is never a single-cycle map from modulus $8$ on [Paper 1], whereas in $\mathbb{Z}[i]$ the smallest norm $2^k + 1$ has single-cycle maps at $(1+i)^6$ ($7 \pm 4i$) and $(1+i)^7$ ($11 \pm 4i$), because what matters for Lemma 4.1 is the distance from $\alpha$ to the elements $\varepsilon\pi^v$, and $7+4i$ lies at distance $\sqrt{17}$ from $8 = i\pi^6$, where $2^k+1$ in $\mathbb{Z}$ lies at distance $1$ from $2^k$.

### 5.3 Moduli $(1+i)^6$ and $(1+i)^7$

At $(1+i)^6$ there are $28$ multipliers and the four digit sets A to D of Section 3.4, so $112$ maps, $56$ up to mirroring, none its own mirror. Of the $112$, $22$ have a single cycle at $N(z) \le 10^6$, with $\alpha = 7 \pm 4i$, $8 \pm 3i$, $9 \pm 2i$, $10 \pm i$, $10 \pm 3i$ and $11 \pm 2i$. The other $45$ maps up to mirroring have $2$ cycles ($19$ maps), $3$ ($4$), $4$ ($3$), $5$ ($5$), $6$ ($1$), $7$ ($1$), $8$ ($4$), $9$ ($2$) and $10$ ($2$), and the four real maps have $23$ and $25$ ($\alpha = 9$) and $560$ and $568$ ($\alpha = 11$, against $181$ and $211$ at $10^5$). For $\alpha = 10 \pm 5i$ and two maps with $\alpha = 11 \pm 2i$ some starts reached the cap of $50{,}000$ steps, so their counts are lower bounds. The single-cycle maps with $\alpha = 11 \pm 2i$, of norm $125$ and drift $\log_2(125/128) = -0.034$ bits, have excursions past the escape bound $2^{200}$ of `gcz`; they were checked by `gstop` from norm $1$ to $10^6$ with escape at $2^{110}$, and the $26$ starts that escaped even that were iterated in unbounded arithmetic until they fell below their starting norm, after up to $138{,}239$ steps of $T$ and excursions up to about $2^{300}$.

At $(1+i)^7$ the window $(128, 256)$ holds $18$ norms and $46$ multipliers up to units. The digit set is unique and closed under conjugation, so there are $46$ maps, $24$ up to mirroring, with $13$ and $15$ their own mirrors. Every start with $N(z) \le 10^6$ reached a cycle, with no cap and no escape; the largest excursion was $2^{153.7}$, for $14 \pm 7i$. Eight maps have a single cycle, $\alpha = 11 \pm 4i$, $12 \pm 3i$, $11 \pm 8i$ and $12 \pm 7i$, of norms $137$, $153$, $185$ and $193$, and in each the cycle is the class of $1$, one odd step. One map of each mirror pair, $\alpha = -11+4i$, $-12+3i$, $-11+8i$ and $-12+7i$, was checked by `gstop` over $10^6 < N(z) \le 10^9$ ($392{,}306{,}389$ starts each, no failure, under ten minutes in all) after all eight had been checked from norm $1$. The other $20$ maps up to mirroring have $2$ cycles ($12 \pm i$), $3$ (nine maps), $4$ ($13 \pm 2i$), $5$ ($11 \pm 6i$, $14 \pm i$), $8$ ($12 \pm 5i$), $11$ ($10 \pm 7i$), $16$ ($9 \pm 8i$), $33$ ($15 \pm 2i$), $74$ ($15$), $166$ ($13$) and $766$ ($11 \pm 10i$). Equal norms do not mean equal behaviour: $9 \pm 8i$ and $12 \pm i$ both have norm $145$, with $16$ and $2$ cycles.

### 5.4 Score, drift and the single-cycle maps

The score $1/M - 1/N$ of a map with $M = 2^k$ and $N = N(\alpha)$ and its drift $\log_2(N/M) - 1$ order maps of one modulus the same way, since both increase with $N$. Across moduli the score favours the smaller modulus through $1/M$, while the drift ignores the modulus. For the families of single-cycle maps that commute with the units, one row per multiplier up to rotation and mirroring:

| map | modulus | score | drift (bits) |
|---|---|---|---|
| $5+2i$ | $(1+i)^4$ | 0.0280 | $-0.142$ |
| $11+2i$ | $(1+i)^6$ | 0.0076 | $-0.034$ |
| $10+3i$ | $(1+i)^6$ | 0.0065 | $-0.232$ |
| $10+i$ | $(1+i)^6$ | 0.0057 | $-0.342$ |
| $9+2i$ | $(1+i)^6$ | 0.0039 | $-0.591$ |
| $12+7i$ | $(1+i)^7$ | 0.0026 | $-0.408$ |
| $11+8i$ | $(1+i)^7$ | 0.0024 | $-0.469$ |
| $8+3i$ | $(1+i)^6$ | 0.0019 | $-0.810$ |
| $12+3i$ | $(1+i)^7$ | 0.0013 | $-0.743$ |
| $11+4i$ | $(1+i)^7$ | 0.0005 | $-0.902$ |
| $7+4i$ | $(1+i)^6$ | 0.0002 | $-0.978$ |

The 5+2i map has the highest score by a factor of almost four, and only the maps with $\alpha = 11 \pm 2i$ have a weaker drift. For comparison, $3x+1$ on $\mathbb{N}$ scores $1/2 - 1/3 = 0.167$, $7x \pm 1$ on $\mathbb{Z}$ scores $0.107$, and the $2-i$ map of Section 2.1 scores $0.05$ [Paper 1].

The drift predicts the size of excursions (Section 4.3), and the single-cycle maps follow it. Over all starts with $N(z) \le 10^9$ the four maps checked at $(1+i)^7$, in order of norm $137$, $153$, $185$, $193$ (drifts $-0.90$ to $-0.41$ bits), have largest excursions of $9.7$, $14.8$, $29.2$ and $37.4$ bits, longest stopping times of $284$, $512$, $1596$ and $2243$ steps of $T$, and mean stopping times of $17.0$, $18.4$, $28.4$ and $30.6$ (`logs/k7_single_gstop_1e9.txt`). The 5+2i map over $N(z) \le 10^9$ reaches $122.9$ bits and $12{,}226$ steps, and the maps with $\alpha = 11 \pm 2i$ (drift $-0.034$) reach about $300$ bits already below $10^6$.

## 6. Maps with many cycles

### 6.1 The classes $(n, V, \varepsilon)$

Six maps of the census have many cycles: at $(1+i)^7$, $\alpha = 13$, $15$, $11 \pm 10i$ and $15 \pm 2i$, and at $(1+i)^6$, $\alpha = 9$ and $11$ with all four digit sets. Their cycles form no visible infinite family. No affine map relates two cycles except mirroring, the cycles spread over all angles, and they share no parity vector beyond the fact that most exponents equal $k$ (`famstruct.py`; computed). What organizes them is arithmetic.

Let a cycle up to units have $n$ odd steps $z_{j+1} = (\alpha z_j + d_j)/\pi^{v_j}$, total exponent $V = v_0 + \cdots + v_{n-1}$, and unit $\varepsilon$ with $S^n(z_0) = \varepsilon z_0$; we call $(n, V, \varepsilon)$ its class. With $s_j = v_j + \cdots + v_{n-1}$, $n$ steps give the cycle equation

$$z_0\left(\varepsilon\pi^V - \alpha^n\right) = \sum_{j=0}^{n-1} d_j\,\alpha^{n-1-j}\,\pi^{V - s_j}.$$

Put $r = |\alpha|^n/2^{V/2}$ and $\text{rel} = |1 - \alpha^n/(\varepsilon\pi^V)|$, so that $N(\varepsilon\pi^V - \alpha^n) = 2^V \text{rel}^2$. The right-hand side is about $2^{V/2}$ times a bounded factor, so $|z_0|$ is of order $1/\text{rel}$, and large cycles need $\alpha^n$ close to $\varepsilon\pi^V$. For real $\alpha$ this is one condition, that $V/(2n)$ approximate $\log_2\alpha$. For non-real $\alpha$ two conditions must hold together: $V/n$ close to $\log_2 N(\alpha)$, and the angle $n\arg\alpha - V\pi/4 - \arg\varepsilon$ close to $0$. The large classes sit where both hold:

| map | main class | near-coincidence | rel | cycles in class | heuristic $E$ |
|---|---|---|---|---|---|
| $13$, $(1+i)^7$ | $(10, 74, -i)$ | $13^{10} = 1.00305 \cdot 2^{37}$ | $3.05 \cdot 10^{-3}$ | 160 | 117 |
| $15$, $(1+i)^7$ | $(43, 336, -1)$ | $15^{43} = 0.9974 \cdot 2^{168}$ | $2.56 \cdot 10^{-3}$ | 32 | 29.3 |
| $11+10i$, $(1+i)^7$ | $(33, 257, i)$ | $(11+10i)^{33}$ near $i\pi^{257}$ | $6.17 \cdot 10^{-4}$ | 728 | 764 |
| $15+2i$, $(1+i)^7$ | $(6, 47, i)$ | $(15+2i)^6$ near $i\pi^{47}$ | $1.58 \cdot 10^{-2}$ | 28 | 20.5 |
| $11$, $(1+i)^6$ | $(37, 256, -1)$ | $11^{37} = 0.99929 \cdot 2^{128}$ | $7.1 \cdot 10^{-4}$ | 498 or 481 at $10^6$ | 613 |
| $9$, $(1+i)^6$ | $(6, 38, i)$ | $9^6 = 1.0136 \cdot 2^{19}$ | $1.36 \cdot 10^{-2}$ | 15 or 17 | 18.4 |

For $11+10i$ both conditions hold at $n = 33$ to about $6 \cdot 10^{-4}$, far better than the generic bound of order $n^{-1/2}$ for two simultaneous approximations, and that map has the most cycles. Smaller classes sit at multiples and sums of the resonant lengths, such as $66$ and $132$ for $11+10i$, where rel is about $m$ times the value at $33$, and $75 = 43 + 32$, $107 = 75 + 32$ for $15$. Two classes are structural: for $\alpha = 15$ at $(1+i)^7$ every digit $d$ gives a fixed class, since $-15(-d) + d = 16d$, which is $d$ times a unit times $\pi^8$, and the same holds for $\alpha = 9$ at $(1+i)^6$, with $16$ and $8$ fixed classes.

The column $E$ is a heuristic count (heuristic). The odd $\pi$-adic integers whose first $n$ exponents are $v_0, \ldots, v_{n-1}$ form $2^{(k-1)n}$ classes, on each of which $S^n$ is affine and onto, so each contains exactly one point with $S^n(z) = \varepsilon z$. If that point is a Gaussian integer with probability $1/N(\varepsilon\pi^V - \alpha^n)$, the expected number of cycles up to units in the class is

$$E(n, V, \varepsilon) = \frac{2^{(k-1)n}\binom{V - kn + n - 1}{n - 1}}{4n\,N\!\left(\varepsilon\pi^V - \alpha^n\right)}.$$

It matches the main classes to within a factor of about $1.4$. Summed over $n \le 20000$ and $|V - n\log_2 N(\alpha)| \le 3$ it gives $129$ ($13$), $70$ ($15$), $30.5$ ($15+2i$), $825$ ($11+10i$), $31$ ($9$) and $721$ ($11$), and the partial sums are constant from $n = 1000$ on ($3000$ for $11$). The sum converges because $V/n$ must stay near $\log_2 N(\alpha)$, below the mean exponent $k+1$, and such exponent vectors become exponentially rare. So the heuristic predicts finitely many cycles for each of these maps, with totals close to the counts found.

### 6.2 Best simultaneous approximations

Call $n$ a record if $\text{best}(n) = \min_{V, \varepsilon} \text{rel}(n, V, \varepsilon)$ is smaller than $\text{best}(m)$ for every $m < n$. For real $\alpha$ the records are the denominators of the convergents of $\log_2|\alpha|$, apart from an occasional extra record at small $n$; for non-real $\alpha$ they are the best simultaneous approximations to the two conditions. The continued fractions $\log_2 13 = [3; 1, 2, 2, 1, 22, 23, \ldots]$, $\log_2 15 = [3; 1, 9, 1, 2, 1, 5, 1, \ldots]$ and $\log_2 11 = [3; 2, 5, 1, 1, 1, 25, \ldots]$ have denominators $1, 3, 7, 10, 227$; $1, 10, 11, 32, 43, 247, 290, 537$; and $1, 2, 11, 13, 24, 37, 949, 986$.

In every map tested the largest class sits at a record, and so does every class with at least three cycles except the multiples and sums of the main record (computed; `convergents.py`, `logs/convergents_families.txt`). Every convergent denominator of $\log_2 15$ up to $537$ except $290$ carries cycles, the last being a mirror pair of $537$ odd steps through $4081+4664i$ and $4664+4081i$. Across $54$ maps of the census with at least three cycles, rerun at $10^6$, $67$ of the $116$ classes with $n \ge 2$ sit at records and hold $1909$ of the $2104$ cycles (`convrun.py`, `logs/convergents_census.txt`). The converse fails: records without cycles are common, such as $13$ at $n = 7$ and $227$ and $9$ at $n = 5$, $47$, $53$ and $153$, mostly with small $E$. A record is close to necessary for a large class and not sufficient, and the record property follows from $E$: if $E\,\text{rel}^2$ decreases in $n$, the $n$ that maximizes $E$ is a record.

For non-real $\alpha$ the simultaneous condition is the right one. For $9+4i$ at $(1+i)^6$, $\log_2 97 = [6; 1, 1, 2, 458, \ldots]$ makes $n = 5$ an excellent approximation of the modulus (error $0.0004$), but its angle error is $0.34$ in units of $\pi/4$, and there are no cycles of five odd steps; the cycles sit at $n = 2$, with angle error $0.065$. For $12+5i$ the modulus record $n = 5$ has angle error $0.49$ and no cycles, while its seven cycles sit at $n = 2$ with angle error $0.005$ (`logs/convergents_modonly.txt`).

For $3x+1$ the link between cycle lengths and convergents of $\log_2 3$ is classical as a constraint on cycles [Steiner 1977; Simons and de Weger 2005]. The observation that the cycles of a map with many cycles concentrate at best-approximation lengths, with class sizes set by rel, was not found stated in the sources read.

### 6.3 Complete cycle lists

The cycle equation bounds one element of every cycle.

*Lemma 6.1 (paper).* Let a cycle have class $(n, V, \varepsilon)$, let $g_j = \ln|\alpha| - \frac{v_j}{2}\ln 2$, so that $\ln r = \sum_j g_j$, and let $c = \max(0, \ln r)$. Put $G_m = \min\left(m(\ln|\alpha| - \frac{k}{2}\ln 2), c\right)$ for $m < n$ and $G_n = \ln r$. Then some element $z$ of the cycle satisfies
$$|z| \le \frac{d_{\max}}{|\alpha|\,\text{rel}}\sum_{m=1}^{n} e^{G_m},$$
where $d_{\max}$ is the largest $|d|$ for $d \in D$.

*Proof.* Starting the cycle at any element, the cycle equation gives $|z_0|\,\text{rel} \le |\alpha|^{-1}\sum_{m=1}^{n}|d_{n-m}|\,e^{G'_m}$, where $G'_m$ is the sum of the last $m$ of the $g_j$. Start at an index where the prefix sums of the $g_j$ are least. Then each suffix sum is the total minus a nonnegative amount or a sum that wraps around and is at most $0$, so $G'_m \le c$. Since every $v_j \ge k$, also $G'_m \le m(\ln|\alpha| - \frac{k}{2}\ln 2)$, and $G'_n = \ln r$. $\square$

*Lemma 6.2 (paper).* If $|V - n\log_2 N(\alpha)| \ge 3$, the bound of Lemma 6.1 is at most $1.55\,d_{\max}n/|\alpha|$.

*Proof.* Then $r \ge 2^{3/2}$ or $r \le 2^{-3/2}$, $\text{rel} \ge |1 - r|$, and the sum is at most $n\max(1, r)$. $\square$

So for each $n$ only six values of $V$ and four units need evaluating. `gcz` to a bound $B$ finds every cycle with an element of norm at most $B$, and `gstop` over $(B, 10^9]$ records every cycle whose least element has norm in that range. Choosing $B$ at least the bound of Lemma 6.1 for every $n$ up to some length gives:

*Theorem 5 (computer-assisted).* For the maps at $(1+i)^7$ with the digit set of least elements:

1. $\alpha = 13$ has exactly $166$ cycles up to units with least norm at most $10^9$, including every cycle with at most $216$ odd steps; $160$ of them have $10$ odd steps.
2. $\alpha = 15$ has exactly $74$, including every cycle with at most $246$ odd steps; $32$ have $43$ odd steps.
3. $\alpha = 11 \pm 10i$ has exactly $766$, including every cycle with at most $2000$ odd steps; $728$ have $33$ odd steps, $29$ have $66$, $2$ have $99$ and $3$ have $132$.
4. $\alpha = 15 \pm 2i$ has exactly $33$, including every cycle with at most $1498$ odd steps; $28$ have $6$ odd steps.

*Proof.* Lemmas 6.1 and 6.2 bound the least norm of every cycle up to the stated length by $10^9$ (by $1.3 \cdot 10^7$, $1.65 \cdot 10^7$, $10^7$ and $2.85 \cdot 10^7$ for shorter lengths). `gcz` ran to these four bounds, and `gstop` from them to $10^9$: $387{,}593{,}990$ starts for $13$ with no cycle; $388{,}772{,}062$ for $11+10i$ with one cycle, already known, least element $2318+2599i$; $386{,}219{,}564$ for $15$, where eight starts reached the step cap and, iterated in exact arithmetic, fall into the $537$-step pair without dropping below their norm; and $381{,}507{,}118$ for $15+2i$ with no cycle (`logs/families_gstop_k7_*`). The bound was evaluated in double precision and used with a margin of at least $1.4$ wherever it decides coverage, and rel was confirmed in exact arithmetic for the main classes. Every cycle in the lists was checked in exact arithmetic by `famverify.py`: each digit lies in $D$ and is the one the map picks, each element is odd, the cycle equation holds, and the elements are pairwise non-associate. $\square$

The count of cycles with least norm at most $B$ saturates: for $11+10i$ it is $2, 3, 6, 66, 395, 765, 766$ at $B = 10^2, \ldots, 10^8$, and for $13$ it is $166$ from $10^6$ on. For $\alpha = 11$ at $(1+i)^6$ there is no complete list. A `gstop` pass over least norms in $(10^6, 3 \cdot 10^6]$ for digit set A found $172$ cycles, $101$ already known, so that map has at least $639$, and so does its mirror. The bound of Lemma 6.1 for $n = 37$ is about $5.6 \cdot 10^8$, out of reach because many starts fall into cycles above their own norm and run to the cap. The heuristic predicts a few cycles of $949$ and $986$ odd steps ($E = 2.9$ and $5.2$), at the next convergents of $\log_2 11$, near norm $10^{13}$.

Whether any of these maps has finitely many cycles is open. A proof would need an effective lower bound for $|\varepsilon\pi^V - \alpha^n|/2^{V/2}$, a linear form in two logarithms for real $\alpha$, or in a logarithm and an angle for non-real $\alpha$; with a bound of Baker type, Lemma 6.1 would turn each check into a finite one, as Simons and de Weger do for $3x+1$ after Steiner [Steiner 1977; Simons and de Weger 2005].

### 6.4 Divergence along the invariant lines

Orbits that appear to diverge occur only for the real multipliers $\alpha = 5$ at $(1+i)^4$ and $9$, $11$ at $(1+i)^6$. For $\alpha = 5$, $176$ of about $1500$ random starts with norm up to $1.8 \cdot 10^7$ appeared to diverge. At $(1+i)^6$, of about $150$ random starts with $|x|, |y| \le 1000$ per digit set, $13$ to $29$ escaped past norm $2^{120}$ for $\alpha = 9$ and $55$ to $69$ for $\alpha = 11$ (`logs/families_k6_escape.txt`; computed). Every escaped orbit ended on one of the eight lines

$$L: \quad x = \pm 1, \quad y = \pm 1, \quad x - y = \pm 1, \quad x + y = \pm 1,$$

most on the diagonal ones, with exponents $k$, $k+1$ and $k+2$ only.

*Proposition 6.3 (computer-assisted).* For $\alpha = 3$ at $(1+i)^3$, $5$ at $(1+i)^4$ (both digit sets), $7$ at $(1+i)^5$, $9$ and $11$ at $(1+i)^6$ (all four digit sets), and $13$ and $15$ at $(1+i)^7$, $S$ maps $L$ into itself, and every odd step from $L$ has exponent $k$, $k+1$ or $k+2$.

*Proof.* A point of $L$ is $u + tw$ with $u$ a unit, $w$ an associate of $1$ or $1+i$, and $t$ an integer. Since $\alpha$ is real, $\alpha z + d = \alpha tw + (\alpha u + d)$, and the coordinate of $\alpha z + d$ transverse to $w$ is that of $\alpha u + d$. The digit depends only on $t$ modulo a power of $2$, so whether $S(z)$ lies in $L$, and with which exponent, depends only on $t$ modulo a fixed power of $2$ as long as the exponent stays bounded. `famline1d.py` tested every $t$ in a window of $2^{16}$ (and $2^{18}$) consecutive integers from $10^{12}$ on two of the lines; the other six are their images under units. Every point mapped into $L$ with exponent $k$, $k+1$ or $k+2$, so the exponent is bounded on every residue class and the window covers a full period. $\square$

On $L$ the map is one-dimensional, and $\log_2|z|$ changes by $\frac{1}{2}(\log_2 N(\alpha) - v)$ per odd step. An odd exponent turns a line of axis type into one of diagonal type and back, so the exponents define a two-state chain. Its stationary distribution, measured over a full period, gives a drift along $L$ (heuristic; it assumes the residues of $t$ are equidistributed along an orbit):

| $\alpha$, modulus | mean $v$ | $\log_2 N(\alpha)$ | drift along $L$ (bits of $\lvert z \rvert$) | observed |
|---|---|---|---|---|
| $3$, $(1+i)^3$ | 3.600 | 3.170 | $-0.215$ | converges |
| $5$, $(1+i)^4$ | 4.400 | 4.644 | $+0.122$ | diverges |
| $7$, $(1+i)^5$ | 5.778 | 5.615 | $-0.082$ | converges |
| $9$, $(1+i)^6$ | 6.222 | 6.340 | $+0.059$ | diverges |
| $11$, $(1+i)^6$ | 6.545 | 6.919 | $+0.187$ | diverges |
| $13$, $(1+i)^7$ | 7.684 | 7.401 | $-0.142$ | converges |
| $15$, $(1+i)^7$ | 7.882 | 7.814 | $-0.034$ | converges |

The values do not depend on the digit set, and the sign matches the observed behaviour in every case. For $\alpha = 15$ a uniform average over $L$ would give $+0.021$; the chain spends only $0.47$ of its time on lines of axis type, where exponents are larger, and the drift is negative. Exact iteration agrees: $300$ starts on $L$ for $\alpha = 15$, with $t$ up to $2 \cdot 10^9$, all returned to cycles, and $200$ starts on $L$ for $\alpha = 9$ all escaped (`famlinerun.py`). The lines carry the divergence and not the cycles: for $\alpha = 11$ at $(1+i)^6$ none of the cycles of $37$ odd steps meets $L$. The picture resembles Kionke's divergent cones [Kionke 2017] and Matthews's example on $\mathbb{Z}[\sqrt 2]$ with the real multiplier $3$, which has three invariant one-dimensional sets and apparently divergent orbits [Matthews 2010, Example 10.1]. An earlier explanation, that divergence occurs when $\alpha/\pi^k$ is real up to a unit, is replaced by this one: it holds for $5$, $9$ and $11$ but does not explain why $3$, $7$, $13$ and $15$, whose lines are also invariant, converge.

## 7. Verification and formal proof

### 7.1 Programs

`gcz.c` classifies every $z$ with $N(z) \le B$. It stores one entry per lattice point, follows each orbit until it meets a classified point, closes a cycle (found with Brent's method), escapes past coordinates $2^{100}$ or reaches a step cap, and fills in the result along the path. Coordinates are 128-bit integers, and a map is given as a table over residues of $(a \bmod M, b \bmod M)$ with $M = 4$, $8$ or $16$.

`gstop.c` is the drop-below-start check for maps that commute with the units: for each odd $z$ with $a > 0$, $b \ge 0$ and $N_0 < N(z) \le B$ it iterates $T$ until the norm falls below $N(z)$, and reports any orbit that returns to an associate of its start, reaches the cap, or passes coordinates $2^{110}$. Norms are compared exactly while below $10^{36}$; above that the iteration continues without comparing, so the program cannot report a false drop. `gstopall.c` does the same over all four quadrants for maps that need not commute with the units, counting a cycle only on a literal return. `m3search.c` runs every table in a product of candidate lists at modulus $\pi^3$ and checks that each digit satisfies the congruence.

`exact.py` iterates any map in unbounded integer arithmetic and takes each digit from the congruence, so it does not share the tables of the C programs; it re-derives every cycle reported here and agrees with `gcz` and `gstop` on the extreme orbits. `famverify.py` checks the cycle lists of Theorem 5 as described there.

### 7.2 What Lean proves

The development in `research/collatz/lean/` uses core Lean 4 (toolchain `v4.35.0`) without Mathlib, with no `sorry`, `admit`, `native_decide` or `bv_decide`, so every `decide` proof is checked by the kernel. `CheckAxioms.lean` prints the axioms of every main theorem: at most `propext`, `Classical.choice` and `Quot.sound`. A clean build takes about $12$ seconds. The Lean names keep older words: `Expr` is a map, `Expr.IsRule` a Collatz map, and `Symmetric` means commuting with the units. Proved in Lean:

1. the definitions of map, $T$, $S$, periodic point, trivial cycle and Collatz map, and Lemmas 2.2, 4.1 and Corollaries 2.3, 4.2;
2. the bound of four trivial cycles, Proposition 3.4, Proposition 3.2, Lemma 3.3 and Theorem 3;
3. Proposition 3.1, Lemma 4.3 and Theorem 4;
4. Proposition 2.1 and Theorem 1 for the $2-i$ map, with `drop_below_start_sound_all`;
5. Proposition 4.4 and Theorem 4.5 for the 5+2i map, with `drop_below_start_sound`.

Not formalized: the step from the window to $N(\alpha) = 3$ at modulus $1+i$, the contraction below the window, Theorem 2 (whose ingredients, Corollary 2.3 and the explicit cycles, are elementary), Propositions 3.6 and 6.3, Lemma 6.1, and the census.

### 7.3 What is trusted

Theorem 1 and the bound $3 \cdot 10^9$ for the 5+2i map are theorems given the runs: that `gstopall` and `gstop` established the hypotheses of `E2_sound` and `H_sound` for the stated bounds, and that `gcz` found every cycle reached from $N(z) \le 10^7$ for the 5+2i map. These rest on the C programs, their 128-bit arithmetic, the compiler, and the agreement of the rule file `alpha5p2i_k4.rule` with the Lean digit table, which was checked entry by entry. Theorem 5 rests in addition on the double-precision evaluation of Lemma 6.1 with the margin stated. The census counts, the counts at modulus $\pi^3$ and the drift and excursion figures are computed to the bounds stated and are not proofs of anything beyond those bounds.

## 8. Open questions

1. *The 5+2i problem.* Prove or refute Conjecture 1. A second cycle with $n$ odd steps and total exponent $V$ satisfies $\prod_j(\alpha + d_j/z_j) = \varepsilon\pi^V$, which for large $z_j$ forces $2^V$ close to $29^n$ and the angle of $(5+2i)^n$ close to $V\pi/4$ modulo $\pi/2$. The angle condition has no analogue for $3x+1$. Bounds of Steiner and Simons and de Weger type might be adapted to both conditions together to exclude cycles with few odd steps.
2. *The other single-cycle maps.* The same question for the $22$ maps at $(1+i)^6$ and $8$ at $(1+i)^7$, and whether single-cycle maps remain a minority at $(1+i)^8$, with window $(256, 512)$.
3. *Finiteness of cycles.* Whether the maps of Theorem 5, and $\alpha = 11$ at $(1+i)^6$, have finitely many cycles; this needs a lower bound for $|\varepsilon\pi^V - \alpha^n|$.
4. *Divergence.* Prove that some orbit on $L$ grows without bound for $\alpha = 5$, $9$ or $11$.
5. *The multiplier $3$ without the criterion.* Whether $\alpha = 3$ at modulus $\pi^3$ admits any Collatz map.
6. *A density theorem.* Terras and Everett showed that almost every positive integer has a finite stopping time under $3x+1$ because the parity pattern of the first $n$ steps depends only on $x$ modulo $2^n$ [Terras 1976; Everett 1977]. The same holds here modulo powers of $\pi$, and we expect finite stopping time on a set of density one whenever $N(\alpha) < 2^{k+1}$; this has not been worked out. It would not rule out divergence, since Kionke's examples combine density-one stopping with positive-density divergence [Kionke 2017]. Siegel and O'Dwyer prove almost bounded orbits on a set of logarithmic density one for Hydra maps on rings of integers under a dimension constraint [Siegel and O'Dwyer 2026]; whether the 5+2i map or an iterate of it meets their hypotheses is open, and a direct encoding modulo $(1+i)^4$ appears, on a rough estimate, to fail the constraint.
7. *The $2$-adic extension.* The digit depends only on $z$ modulo $4$, so the 5+2i map extends continuously to $\mathbb{Z}_2[i]$. For $3x+1$ the extension preserves Haar measure and is conjugate to the shift [Lagarias 1985; Bernstein 1994]; whether the analogues hold here is open.
8. *Larger bounds.* At about $1.2 \cdot 10^6$ starts per second per core, checking Conjecture 1 to $10^{11}$ needs about nine core-hours.

## 9. Miscellaneous

*The multiplier $3$.* The Gaussian multiplier $3$ is inert, so multiplication by $3$ preserves the real line. On the axes, up to units, the map $\alpha = 3$ at $(1+i)^3$ with $D = U$ is the accelerated $3x+1$ map applied to whichever of $\pm x$ is $1$ modulo $4$: a non-real unit gives exponent $1$, so the digit is $\pm 1$, and an integer divisible by $\pi^3$ is divisible by $4$. Hence $|S(x)| \le (3|x|+1)/4 < |x|$ for $|x| > 1$, every real orbit decreases to a unit, and the four other cycles of Section 5.2 lie off the axes (paper). No map with $\alpha = 3$ both agrees with $3x+1$ on the positive integers and lies inside the window: if $\pi^k$ divides $3n+1$ for every odd $n \ge 1$, it divides $10 = -i\pi^2(2+i)(2-i)$, so $k \le 2$ and $9 > 2^{k+1}$ (paper; the step $k \le 2$ in Lean as `prop5_2`, `prop5_2'`). A map whose multiplier or modulus depends on the class, of the kind Matthews studies [Matthews 2010], could still restrict to $3x+1$; none has been tested, and a single-cycle example would contain the Collatz conjecture.

*Joseph's map.* Joseph's extension of $3x+1$ to $\mathbb{Z}_2[i]$ divides by $2$ and sends $z$ to $(3z + \beta)/2$ on the classes $\beta \in \{1, i, 1+i\}$ modulo $2$ [Joseph 1998], and Kucinski found all $77$ of its cycles of period at most $400$ on $\mathbb{Z}[i]$ [Kucinski 2000]. If the classes are visited evenly, the mean change of $\ln N$ per step is $\frac{3}{4}\ln 9 - \ln 4 = +0.26$. In our runs $98.2\%$ of the starts with $N(z) \le 10^5$ had not reached a cycle after $20000$ steps or passed coordinates $2^{100}$, and the rest fell into $67$ cycles (computed).

*Modulus $1+i$.* The map of the CSU Channel Islands REU sends $z$ to $z/(1+i)$ or $(2+i)z + u$ with a fixed unit $u$ [Alvarado 2025]. It lies at modulus $1+i$ with $N(\alpha) = 5$ above the window $(2, 4)$, so its drift is $\log_2 5 - 2 > 0$, which fits the report that about $5.6\%$ of starts with $|a|, |b| \le 100$ reached a unit.

*Sweeps with free digits.* With each digit chosen freely among odd elements of norm at most $5$, a single nonzero cycle appeared at $N(z) \le 10^5$ in $68$ of $288$ maps at $(1+i)^2$, $108$ of $972$ at $(1+i)^3$ and $76$ of $448$ at $(1+i)^4$ (`sweep_gen.py`; computed). These counts predate the criterion of triviality and count cycles differently, but they show that a single cycle alone does not pick out a map.

*Modulus $(1+i)^6$ without the criterion.* A sample of $6$ random conjugacy classes of non-commuting tie-breaks for each of the $112$ associates, $672$ maps, found $146$ passing at $10^5$, at norms $65$, $73$, $85$, $101$, $109$ and $117$; every sampled map with $\alpha = 9$ or $11$ had escaping starts, and the norm-$125$ maps were inconclusive because of the $2^{100}$ cutoff (`logs/symtest_k6_sample_B1e5.txt`; computed). One sampled map has three trivial cycles.

*The Eisenstein integers.* The next ring, $\mathbb{Z}[\omega]$, with six units and the inert prime $2$, has its first commuting Collatz candidate at the first modulus that allows the criterion, $\alpha = 5+2\omega$ at modulus $4$; it is treated in [Paper 1].

## 10. Code and data

Everything is in `research/collatz/`. C programs: `gcz.c`, `gstop.c`, `gstopall.c`, `m3search.c`. Rule files: `alpha5p2i_k4.rule` (the 5+2i map), `G3_k3.rule`, `G3p2i_k3.rule`, `k7_a-11_4.rule`, `k7_a-12_3.rule`, `k7_a-11_8.rule`, `k7_a-12_7.rule`. Python: `rules.py`, `exact.py`, `sweep_eq2.py`, `sweep_gen.py`, `symtest.py`, `m3search.py`, `variants29.py`, `mirror29.py`, `entry2mi.py`, `below_window.py`, `joseph.py`, `bigrun.py`, `run_stop.sh`, and for Section 6 `families.py`, `famtheory.py`, `famcover.py`, `famheur.py`, `famstruct.py`, `famverify.py`, `famlines.py`, `famline1d.py`, `famlinerun.py`, `famescape.py`, `convergents.py`, `convrun.py`. Logs are in `logs/`, among them `alpha5p2i_k4.stop`, `G3_k3.stop`, `G3p2i_k3.stop`, `k345_1e6_rerun.txt`, `k6_1e6.txt`, `k7_1e6.txt`, `k7_single_gstop_1e9.txt`, `symtest_*`, `m3search_*`, `variants29_1e5.txt` and `families_*`. The working notes are `theorems.md`, `symmetry-test.md`, `modulus3-search.md`, `families.md`, `convergents.md` and `score.md`. The outputs of `convrun.py` and the enlarged `gcz` used for Theorem 5 (cycle limit raised to $65536$) were kept only in a scratchpad and are not reproducible from the repository as it stands. The Lean proofs are built with `lake build` in `lean/`, and their axioms are printed by `lake env lean CheckAxioms.lean`.

## References

- [Paper 1] Collatz maps across rings of integers, companion paper, in preparation (`papers/collatz-rings.md`).
- [Alvarado 2025] A. Alvarado, The Collatz conjecture over the Gaussian integers, talk slides, University of Colima, 2 April 2025, on work of a 2017 REU at CSU Channel Islands. https://fefoaranda.com/wp-content/uploads/2025/04/colima-alvarado-2025.pdf [slides only; the paper was not found].
- [Bernstein 1994] D. J. Bernstein, A non-iterative 2-adic statement of the 3N+1 conjecture, Proceedings of the American Mathematical Society 121 (1994), 405–408 [not checked against the publication].
- [Cassaigne, Harju and Karhumäki 1999] J. Cassaigne, T. Harju and J. Karhumäki, On the undecidability of freeness of matrix semigroups, International Journal of Algebra and Computation 9(3–4) (1999), 295–305 [cited through Lagarias 2016; not seen].
- [Everett 1977] C. J. Everett, Iteration of the number-theoretic function f(2n) = n, f(2n+1) = 3n+2, Advances in Mathematics 25 (1977), 42–45 [not checked against the publication].
- [Joseph 1998] J. A. Joseph, A chaotic extension of the 3x+1 function to Z_2[i], Fibonacci Quarterly 36(4) (1998), 309–316.
- [Kionke 2017] S. Kionke, A geometric approach to divergent points of higher dimensional Collatz mappings, arXiv:1511.05893; Monatshefte für Mathematik 182 (2017) [journal details from a secondary source].
- [Klarner, Birget and Satterfield 1991] D. A. Klarner, J.-C. Birget and W. Satterfield, On the undecidability of the freeness of integer matrix semigroups, International Journal of Algebra and Computation 1(2) (1991), 223–226 [cited through Lagarias 2016; not seen].
- [Kucinski 2000] G. I. Kucinski, Cycles for the 3x+1 map on the Gaussian integers, preprint, University of Scranton, May 2000. https://monks.scranton.edu/files/pubs/kucinski.pdf
- [Lagarias 1985] J. C. Lagarias, The 3x+1 problem and its generalizations, American Mathematical Monthly 92 (1985), 3–23.
- [Lagarias 2016] J. C. Lagarias, Erdős, Klarner, and the 3x+1 problem, American Mathematical Monthly 123(8) (2016), 753–776.
- [Matthews 2010] K. R. Matthews, Generalized 3x+1 mappings: Markov chains and ergodic theory, in J. C. Lagarias (ed.), The Ultimate Challenge: The 3x+1 Problem, American Mathematical Society, 2010, 79–103.
- [Siegel and O'Dwyer 2026] M. Siegel and R. O'Dwyer, Almost-boundedness of Collatz-type maps on number rings via Archimedean methods, arXiv:2609.33018 (2026).
- [Simons and de Weger 2005] J. Simons and B. de Weger, Theoretical and computational bounds for m-cycles of the 3n+1 problem, Acta Arithmetica 117 (2005), 51–70 [not checked against the publication].
- [Steiner 1977] R. P. Steiner, A theorem on the Syracuse problem, Proceedings of the 7th Manitoba Conference on Numerical Mathematics (1977), 553–559 [not checked against the publication].
- [Terras 1976] R. Terras, A stopping time problem on the positive integers, Acta Arithmetica 30 (1976), 241–252 [not checked against the publication].
