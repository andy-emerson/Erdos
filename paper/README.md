# Paper

A concept paper explaining the idea behind the exact-rational work: the problem of adding rationals, lowest terms and the gcd, the common-denominator representation, and partial sharing over a coprime base. It is independent of any one specification.

**Status: active.** The current draft is [v0.6](v0.6.md).

- [v0.1](v0.1.md): sections 1–6 (problem and core idea).
- [v0.2](v0.2.md): sections 1–6, recast as an extension of the gcd method, with costs in three forms throughout. Related work, costs and limits, and evidence are to come.
- [v0.3](v0.3.md): sections 1–6 with the refined gcd method derived as a chain of gcds, an equal-input-size assumption, costs split into arithmetic and data-structure categories, appendices on multiplication, division and the gcd, and a bibliography of classical sources (some fields TBD).
- [v0.4](v0.4.md) (current draft): exact sharing of whole numbers; partial sharing deferred. Shared denominators, shared numerators, and a combined structure of whole numbers stored once, with arithmetic, structure and algorithm kept apart.
- [v0.6](v0.6.md) (in progress; build 1 of 3 is v0.5 unchanged; build 2 reorders it into one type per chapter: integers, rationals, decimals, matrices; build 3 adds the planned content).
- [v0.5](v0.5.md) (superseded by v0.6: sections 1–2 complete; 3.1–3.2 drafted): naive rational arithmetic, written from scratch: storage, the textbook formulas, and the shortcuts for stored integers that are equal, opposite or zero; then arithmetic in lowest terms: growing integers, the gcd and Euclid's algorithm, reduced storage, and the cases where a gcd is known in advance.

  **To revisit after section 3:**
  - which of section 2's special cases move to section 3, for the paper's flow;
  - whether some known-answer skips belong as early as section 1;
  - how opposite denominators are presented.

  **New chapter plan** (agreed Oct 30, 2026; supersedes the chapter numbering below where they differ). One type per chapter:
  - **1. Integers:** representation, signed and unsigned, overflow, growing integers (now 2.1), division with remainder and `mod`, gcd and Euclid with its cost, the lcm, the coprime lemma and its proofs. Moving the gcd here puts Euclid before the 2.2 whole-multiple proof, so that proof can use the gcd again; choose its cleanest form then (the earlier Euclid-based proof also assumed an unproved scaling step).
  - **2. Rationals:** storage and the sign, naive arithmetic, lowest terms using chapter 1's gcd, the special cases, the common-divisors aside, the library. Long, as the foundation.
  - **3. Decimals:** rationals whose denominators are powers of 10, built on shared denominators (now 3.1) with deferred reduction. Money is the quick first example. Common multiples (the lcm) align scales: for powers of 10 the lcm is the larger one; multiplication adds the powers. Points, raised after the decimal system is built: a decimal keeps 10ᵏ denominators and so departs from lowest terms; division is exact but leaves the type unless the divisor's reduced numerator has no prime factors but 2 and 5, in which case the result becomes a general rational, as 7 ÷ 2 leaves the integers.
  - **4. Matrices:** the same sharing idea extended to numerators (shared numerators, now 3.2, moves here): a packed list of whole numbers that cells refer to, built with a temporary worst-case hash table (3.3–3.4's index and hash material moves here as the build tool). Its trigger is matrices; its win is memory (round m2). Factored entries (primes, coprime base) are added only if the next tests earn a speed win.

  **Planned, not yet written** (agreed Oct 29, 2026):
  - **3.3:** the combination of 3.1 and 3.2, each whole number stored once, using an index (an arrival-order list). One new idea only: the combination, through its intuitive structure.
  - **3.4:** the same combination with a hash table, taught from scratch as the efficient version. Index inefficiency is explained by reasoning, not benchmarks. The win is memory; it is somewhat slower in arithmetic and lacks an obvious trigger (unlike a ledger of currency). It shows the idea can be done.
  - **Common multiples: making denominators shared (the lcm). Included (author, Oct 29, 2026); placement open, but after the shared-denominator idea of 3.1** (within 3.1, between 3.1 and 3.2, or after 3.2): ask "when do you know denominators are shared?" and answer that the lcm makes them shared. Money stays as the quick first example; a sum of many fractions shows one lcm, integer additions, one division. Its matrix use (row lcm × column lcm as every product term's denominator, so each dot product is one integer sum over $L_i \cdot M_j$) is possibly the first step of chapter 4, giving 3.1 a use there. The numerator mirror gets at most a sentence.
  - **Common divisors. Included (author, Oct 29, 2026), after the gcd idea of chapter 2, math only, no library pseudocode:** between "nothing in common" and "everything in common", dividing out first already benefits from what two numbers share; the cost is finding it. Placement depends on chapter 4: inside 2.5 (the trade-off) if chapter 4 happens, otherwise after the library as a closing aside. Phrased as an observation, not a promise.
  - **Chapter 4, only if the matrix tests pay off:**
    - 4.1 matrix arithmetic with chapter 2's rationals;
    - 4.2 the hash table of 3.4 applied to matrices (no speed gain expected, as in 3.4);
    - 4.3 factorization, with gcds as valuations;
    - 4.4 a coprime base in place of primes.
  - **Structure of the book:** chapter 1 teaches and chapter 2 builds the library from it; chapter 3 teaches and chapter 4 extends the library from it. Chapter 3's pseudocode is written as the components chapter 4 calls (as chapter 3 calls chapter 2): 3.3–3.4 as store, look up and read by handle; 3.1's columns possibly as a shared-denominator matrix step in chapter 4; 3.2's rows only if chapter 4 uses them. Pseudocode that shows an idea's evolution may stay even if unused, but no library code goes unused.
  - **Appendix B:** stops being a standalone chapter 3 library. Once chapter 4 is settled, its surviving pieces move into the appendix for the extended library. Nothing is rewritten until then.
  - **Throughout:** "win" names one side of the memory–speed trade-off that runs through the book; each section has its own win. No benchmarks in the paper; its tests come later, built for the textbook versions.

**Moved, Oct 29, 2026.** The two working notes that were here are not part of the paper and now live with the work they describe:
- the note on the two skeleton designs, formerly `note-skeletons.md`: the joint note is [`shared/partial-sharing-baseline/research/design-note.md`](../shared/partial-sharing-baseline/research/design-note.md), and each design's part is in [`08-common-divisors/research/design-note.md`](../08-common-divisors/research/design-note.md) and [`09-coprime-base/research/design-note.md`](../09-coprime-base/research/design-note.md);
- the explainer of coprime bases and the exponent-vector notation ($p^{\alpha}$, $\wedge$, $\vee$), formerly `note-coprime-base.md`: [`09-coprime-base/research/coprime-base-notation.md`](../09-coprime-base/research/coprime-base-notation.md).
