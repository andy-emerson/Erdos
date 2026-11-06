# Collatz maps at modulus $(1+i)^3$ whose digit tables do not commute with the units

Working notes. Nothing here is in the papers. Definitions are those of `theorems.md`, Section 1, and `symmetry-test.md`, Section 2. Scripts: `m3search.c` (binary `m3search`), `m3search.py`, and the existing `symtest.py`, `gcz` and `gstopall`. Logs are in `logs/m3search_*`.

## 1. Summary

1. Collatz maps whose digit tables do not commute with the units exist at modulus $\pi^3$, $\pi = 1+i$, inside the window $8 < N(\alpha) < 16$, as far as computation can show. With the six smallest digits in each class, $1212$ of the $5628$ tables that survive the fixed-point lemma pass at $N(z) \le 10^4$. Of these, $1204$ also pass `gstopall` to $N(z) \le 10^6$. Up to the conjugacy $z \mapsto uz$ that is $305$ maps, spread over $8$ of the $12$ multipliers.
2. For six of the eight multipliers some passing table differs from the unit table $D = U$ in a single digit. For $-3-2i$ and $3+2i$ at least two digits must change. For $\alpha = 3i$ the table that sends the classes of $1, i, -1, -i$ to the digits $2-i, -1, -i, 1$ has one trivial cycle, $1 \to -i \to -1 \to i \to 1$, which is a unit cycle through all four units. Every odd $z$ with $N(z) \le 10^7$ drops below its norm, with largest stopping time $155$.
3. Proven: for $\alpha = -3$, $-3+2i$ and $-2+3i$ there is no Collatz map at modulus $\pi^3$ at all, whatever the digit table (Section 4). For $\alpha = 3$ no table with the six smallest digits passes, and the passing tables with larger digits are fragile.
4. Score $1/8 - 1/N(\alpha)$: $1/72 \approx 0.0139$ for the associates of $3$ (norm $9$), and $5/104 \approx 0.0481$ for the associates of $3 \pm 2i$ (norm $13$).
5. Extending the digits to norm at most $100$ gives $10{,}866{,}920$ tables after pruning, of which $1{,}049{,}464$ pass at $N(z) \le 10^4$. A random sample of $270$ passing tables (30 per multiplier) was run to $10^6$, and $244$ held.

Nothing here proves that any map is a Collatz map. As with $3x+1$, the evidence is a finite check, and a proof is out of reach. What is proven is the negative statement of item 3.

## 2. Setting

A map at modulus $\pi^3$ has an odd multiplier $\alpha$ and a digit table $d$ indexed by the class of $z$ modulo $\pi^3$, with $\pi^3 \mid \alpha z + d(z)$. The odd classes modulo $\pi^3$ are four, and each contains exactly one unit, since two distinct units differ by an element of norm $2$ or $4$ and a nonzero multiple of $\pi^3$ has norm at least $8$. So the table is a list $(d_1, d_i, d_{-1}, d_{-i})$, where $d_u$ is the digit of the class of $u$ and lies in the class of $-\alpha u$. The accelerated map is $S(z) = (\alpha z + d(z))/\pi^v$ with $v$ maximal, and the map is a Collatz map if every nonzero orbit reaches a cycle containing a unit. Cycles are literal. The table commutes with the units exactly when $d_{u u'} = u\,d_{u'}$ for all units, that is, when the digit set is $d_1 U$.

The multipliers in the window are the associates of $3$ (norm $9$) and of $3 + 2i$ and $3 - 2i$ (norm $13$), $12$ values in all. Without the commuting condition associates and conjugates are different maps. The only conjugacy left is $z \mapsto uz$, which sends the table $d$ to $d'(z) = u^{-1}d(uz)$ and keeps $\alpha$ (`symmetry-test.md`, Section 2). The search treats the $12$ multipliers separately and counts tables both literally and up to this conjugacy.

## 3. Method

1. Candidates. For each multiplier and each class, the digits are the elements of the required class in increasing norm. The first run takes the six smallest, which are the elements of norm at most $13$ (norms $1, 5, 5, 9, 13, 13$), so $6^4 = 1296$ tables per multiplier and $15552$ in all. The second run takes all $39$ elements of norm at most $100$ per class, $39^4 = 2{,}313{,}441$ tables per multiplier. Both candidate sets are closed under the conjugacy.
2. Pruning. A digit is removed if it forces failure on its own. The first test is Lemma 5.1 of `theorems.md` with $\varepsilon = 1$, the only case that applies to tables that do not commute with the units: if $d = (\pi^v - \alpha) z$ with $v \ge 3$ and $z$ odd and not a unit, then $S(z) = z$ is a nontrivial fixed point. The second is a digit $d = -\alpha z$ with $z$ odd, which gives $\alpha z + d = 0$ and sends the orbit of $z$ to $0$. In both cases $z$ automatically lies in the class whose digit is $d$. This removes $-\alpha u$ from each class, and for the three multipliers of Section 4 it removes every digit except the unit.
3. Test. `m3search` runs every remaining table on all odd $z$ with $N(z) \le 10^4$ (all four quadrants, $15{,}700$ starts) in increasing norm. Each orbit is followed under $S$ in 128-bit arithmetic until it meets a point already known to reach a trivial cycle, closes a literal cycle (Brent), reaches $0$, or passes $10^5$ odd steps or $2^{100}$. A table passes if every start reaches a cycle containing a unit. With reduction on, only the least table in each conjugacy orbit is run, and the orbit size is used to recover literal counts. The reduced and unreduced runs of the six-smallest search give identical literal counts, and the four conjugates of a table always agree, as Section 2 predicts.
4. Cross-check. All $1212$ passing tables of the six-smallest run and $410$ random failing ones were run again through `symtest.gauss_run`, which uses `gcz` and re-iterates every cycle in exact Python arithmetic. There were no disagreements.
5. Confirmation. Each passing table of the six-smallest run was run through `gstopall` from $N(z) > 1$ to $N(z) \le 10^6$ ($1{,}570{,}740$ starts per map). By Theorem 6.1 of `theorems.md`, if every start drops below its norm, then every orbit with $N(z) \le 10^6$ reaches norm at most $1$. Pruning has excluded the digits that lead to $0$, so such an orbit reaches a unit, and the units reach a trivial cycle by the run at $10^4$. One representative per multiplier was also run to $10^7$.

## 4. The three multipliers with no Collatz map

*Proposition 4.1.* For $\alpha \in \{-3, -3+2i, -2+3i\}$ there is no Collatz map at modulus $\pi^3$, whatever the digit table.

*Proof.* In each case $\pi^v - \alpha$ is a unit for some $v \ge 3$: $\pi^4 + 3 = -1$, $\pi^3 - (-3+2i) = 1$ and $\pi^3 - (-2+3i) = -i$, using $\pi^3 = -2+2i$ and $\pi^4 = -4$. This is Corollary 5.2 of `theorems.md` with $\varepsilon = 1$, which needs no commuting assumption. So in a Collatz map every digit is a unit. The units lie in distinct classes modulo $\pi^3$ (Section 2), so the digit set is $U$, which is closed under the units, and the table commutes with them. Theorem 5.4 of `theorems.md` excludes this. $\square$

The pruning step finds the same thing: for these three multipliers only the unit survives in each class, at both candidate bounds, and the single remaining table has a nontrivial cycle through $z$ of norm $5$ or $9$.

For the other nine multipliers $N(\pi^v - \alpha) \ge 5$ for every $v \ge 3$, so Lemma 5.1 removes only the digits that are multiples of $\pi^v - \alpha$ with odd non-unit quotient. Among the six smallest digits it removes none. Among the digits of norm at most $100$ it removes $8$ to $44$ per multiplier, and none for $3$, $2-3i$, $3-2i$ and $3+2i$.

## 5. Results with the six smallest digits

The literal counts below refer to the $625$ tables per multiplier left after pruning ($5$ digits per class). Of these, $5$ commute with the units, and none of those passes. The column "maps" counts conjugacy orbits of tables passing at $10^6$. The stopping time is in steps of $T$, from `gstopall` at $10^6$.

| $\alpha$ | $N(\alpha)$ | pass at $10^4$ | pass at $10^6$ | maps | trivial cycles (number of tables with 1, 2, 3) | tables with a unit cycle | max stopping time | max excursion (bits) |
|---|---|---|---|---|---|---|---|---|
| $3$ | 9 | 0 | 0 | 0 | | | | |
| $3i$ | 9 | 240 | 240 | 61 | 172, 52, 16 | 136 | 184 | 7.1 |
| $-3$ | 9 | 0 (1 table) | 0 | 0 | | | | |
| $-3i$ | 9 | 48 | 48 | 13 | 46, 2, 0 | 0 | 184 | 7.3 |
| $3+2i$ | 13 | 72 | 72 | 18 | 72, 0, 0 | 24 | 1810 | 36.3 |
| $-2+3i$ | 13 | 0 (1 table) | 0 | 0 | | | | |
| $-3-2i$ | 13 | 124 | 124 | 31 | 32, 60, 32 | 120 | 1838 | 37.3 |
| $2-3i$ | 13 | 84 | 84 | 21 | 80, 4, 0 | 60 | 1627 | 44.9 |
| $3-2i$ | 13 | 176 | 168 | 42 | 148, 20, 0 | 92 | 1862 | 38.2 |
| $2+3i$ | 13 | 272 | 272 | 69 | 268, 4, 0 | 144 | 1867 | 43.9 |
| $-3+2i$ | 13 | 0 (1 table) | 0 | 0 | | | | |
| $-2-3i$ | 13 | 196 | 196 | 50 | 74, 74, 48 | 172 | 2191 | 42.1 |
| total | | 1212 | 1204 | 305 | | | | |

A trivial cycle here is either a unit cycle, made only of units, or a mixed cycle that passes through some units and some non-units. "Tables with a unit cycle" counts the passing tables with at least one unit cycle. For $\alpha = -3i$ every trivial cycle is mixed. A map can have both kinds, for instance $\alpha = 3i$ with digits $(-2-i, -3-2i, -i, 1)$, which has the fixed unit $1$ and a mixed cycle through $-1$, $i$, $-i$ and three elements of norm $5$. The counts of $3$ trivial cycles show again that the divisor condition of Theorem 2.1(b) needs the table to commute with the units.

The eight tables that pass at $10^4$ but not at $10^6$ all have $\alpha = 3 - 2i$ and form two conjugacy orbits. Their representatives are $(1+2i, -i, 1, i)$ and $(-3-2i, 3i, -3, i)$ (digits for the classes of $1, i, -1, -i$). Each has a nontrivial cycle of $187$ odd steps, with least elements $31+236i$ (norm $56657$) and $-108-213i$ (norm $57033$) respectively, and largest norms of $28$ and $30$ bits. Both cycles were re-iterated in exact arithmetic and contain no unit. Starts such as $-60-177i$ and $59+224i$ fall into them without dropping below their norm.

The norm-$9$ multipliers $\pm 3i$ have short orbits (largest stopping time $184$, excursions of $7$ bits). The norm-$13$ multipliers have stopping times near $2000$ and excursions near $40$ bits, which fits the slower average contraction: the expected valuation is $4$, so the expected change of $\log_2 N$ per odd step is $\log_2(9/16) = -0.83$ against $\log_2(13/16) = -0.30$.

## 6. Representative maps

One table per multiplier, chosen to have the smallest total digit norm among the tables passing at $10^6$. The digits are listed for the classes of $1, i, -1, -i$. Each was run through `gstopall` to $N(z) \le 10^7$ ($15{,}708{,}048$ starts), with no failures and no cycles (`logs/m3search_reps_gstopall_1e7.txt`).

| $\alpha$ | digits | trivial cycles (odd elements) | unit cycle | max stopping time at $10^7$ | score |
|---|---|---|---|---|---|
| $-3i$ | $2-3i, 1, i, -1$ | one, through $-1, -i, 1, -2+i, 1-2i, 1+2i, -2-i, 2-i, -1+2i, i$ | no (mixed) | 142 | $1/72$ |
| $3i$ | $2-i, -1, -i, 1$ | one, $-1 \to i \to 1 \to -i \to -1$ | yes | 155 | $1/72$ |
| $-3-2i$ | $-1+2i, 2-i, -1, -i$ | two, the fixed units $1$ and $i$ | yes | 1556 | $5/104$ |
| $-2-3i$ | $2-i, -1, -i, 1$ | one, $-1 \to -i \to 1 \to i \to -1$ | yes | 1857 | $5/104$ |
| $2-3i$ | $2-i, 1+2i, -i, 1$ | one, $-1 \to 1 \to i \to -1$ | yes | 1476 | $5/104$ |
| $2+3i$ | $2+i, 1, i, -1$ | one, $-1 \to i \to 1 \to -1$ | yes | 1932 | $5/104$ |
| $3-2i$ | $1-2i, -i, 1, i$ | one, $-i \leftrightarrow i$ | yes | 1514 | $5/104$ |
| $3+2i$ | $1-2i, 2+i, -1-2i, i$ | one, $-1 \leftrightarrow 1$ | yes | 1575 | $5/104$ |

The tables shown differ from the unit table in one class for $\pm 3i$, $-2-3i$, $2+3i$ and $3-2i$, in two for $-3-2i$ and $2-3i$, and in three for $3+2i$. Counting all tables passing at $10^6$ by the number of non-unit digits (1, 2, 3 or 4), the tables with a single non-unit digit number $4$ for $-3i$, $16$ for $3i$, $8$ for $-2-3i$, $4$ for $2-3i$ (with a digit of norm larger than $5$), $16$ for $2+3i$ and $8$ for $3-2i$. For $-3-2i$ and $3+2i$ there are none: at least two digits of the unit table must change. In every case a small change to the unit table removes the nontrivial cycles of Theorem 5.4. The cycles of the map $\alpha = 3i$ with digits $(2-i, -1, -i, 1)$ can be checked by hand: $3i \cdot 1 + (2-i) = 2+2i = \pi^3(-i)$, $3i(-i) + 1 = 4 = \pi^4(-1)$, $3i(-1) - i = -4i = \pi^4 i$, and $3i \cdot i - 1 = -4 = \pi^4$.

The full list of the $1212$ tables, with trivial cycles, stopping times and excursions, is in `logs/m3search_n6_gstopall_1e6.txt`.

## 7. Results with digits of norm up to 100

After pruning, $10{,}866{,}920$ tables remain ($2{,}719{,}295$ conjugacy orbits). None was unresolved at the step cap. The number passing at $N(z) \le 10^4$, literal and up to conjugacy:

| $\alpha$ | tables after pruning | pass at $10^4$ | orbits passing | sample at $10^6$ |
|---|---|---|---|---|
| $3$ | 1500625 | 6868 | 1717 | 11/30 |
| $3i$ | 707281 | 177792 | 44471 | 25/30 |
| $-3i$ | 1185921 | 83600 | 20923 | 29/30 |
| $3+2i$ | 1679616 | 151348 | 37837 | 30/30 |
| $-3-2i$ | 390625 | 35032 | 8758 | 30/30 |
| $2-3i$ | 1679616 | 94604 | 23651 | 30/30 |
| $3-2i$ | 1679616 | 210108 | 52527 | 29/30 |
| $2+3i$ | 1336336 | 166250 | 41580 | 30/30 |
| $-2-3i$ | 707281 | 123862 | 30983 | 30/30 |
| $-3$, $-3+2i$, $-2+3i$ | 1 each | 0 | 0 | |
| total | 10866920 | 1049464 | 262447 | 244/270 |

No table that commutes with the units passes, as Theorem 5.4 requires. Of the passing orbits, $162$ have size $2$ (the table commutes with $-1$ but not with $i$) and the rest size $4$. For every multiplier some failing table is first exposed only by a start of norm above $7000$, so the bound $10^4$ is too small for a reliable count. The sample shows the extent of this: at $10^6$ almost all sampled tables still pass for the norm-$13$ multipliers and for $-3i$, but $19$ of $30$ fail for $\alpha = 3$ and $5$ of $30$ for $\alpha = 3i$. The column is a sample (`logs/m3search_n100_sample_gstopall_1e6.txt`), not a count.

So $\alpha = 3$ is the weakest multiplier: no table with the six smallest digits passes, about $0.5$ percent of the larger tables pass at $10^4$, and most of those fail by $10^6$. It may admit no Collatz map at all. That is open, since the obstruction is not a fixed point (Lemma 5.1 removes no digit of norm at most $100$ for $\alpha = 3$) but longer cycles that depend on the whole table.

## 8. What is proven and what is computed

Proven: Proposition 4.1 (no Collatz map for $-3$, $-3+2i$, $-2+3i$, any digit table), from Corollary 5.2 with $\varepsilon = 1$ and Theorem 5.4 of `theorems.md`. Also the soundness of the pruning (Lemma 5.1, and the zero digit), and the hand-checked unit cycle of the map $\alpha = 3i$ in Section 6.

Computed, so these are observations up to the stated bound:

1. With the six smallest digits: $5628$ tables after pruning, $1212$ passing at $10^4$ (confirmed independently through `gcz`), $1204$ passing at $10^6$, $305$ maps up to conjugacy, none for $\alpha = 3$. One representative per multiplier passes at $10^7$.
2. Two nontrivial cycles of $187$ odd steps with least norm about $57000$ for $\alpha = 3 - 2i$, verified in exact arithmetic.
3. With digits of norm at most $100$: $10{,}866{,}920$ tables after pruning, $1{,}049{,}464$ passing at $10^4$, and $244$ of a random sample of $270$ passing at $10^6$.

The question asked was whether a map in the window at modulus $\pi^3$ whose digits do not commute with the units can be a Collatz map. On the evidence, yes, for eight of the twelve multipliers, and already with digits of norm at most $13$, usually with the unit table changed in one or two classes. An impossibility proof is therefore not to be expected. The proven part covers only the three multipliers that Corollary 5.2 reaches with $\varepsilon = 1$. For $\alpha = 3$ an impossibility proof would need more than fixed points: an argument that every table produces some longer nontrivial cycle. Nothing so far points to one.

## 9. Files

1. `m3search.c` (binary `m3search`, `gcc -O2 -o m3search m3search.c`): runs every table in a product of candidate lists on all odd $z$ with $N(z) \le B$, literal cycles, 128-bit arithmetic, optional reduction modulo $z \mapsto uz$. It checks that each digit satisfies $\pi^3 \mid \alpha u + d_u$.
2. `m3search.py`: candidate digits, the pruning (`prune maxnorm`), and the driver (`run 6 B cap tag [red]`, `run n100 B cap tag 1`).
3. Logs: `logs/m3search_n6_B1e4.txt` (six smallest digits, every table), `logs/m3search_n6_gstopall_1e6.txt` (the $1212$ passing tables, trivial cycles from `gcz` re-iterated exactly, `gstopall` to $10^6$), `logs/m3search_n40_B1e4.txt` (norm at most $40$, reduced, a timing run), `logs/m3search_n100_B1e4.txt` (norm at most $100$, reduced), `logs/m3search_n100_sample_gstopall_1e6.txt`, `logs/m3search_reps_gstopall_1e7.txt`.
