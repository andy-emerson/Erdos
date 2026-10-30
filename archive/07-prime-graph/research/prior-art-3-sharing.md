# Prior art 3: finding shared factors at entry

Oct 27, 2026 · a focused pass of about 20 minutes for the entry step in [idea.md](idea.md) ("Scalability requirements"). It follows [prior-art-2.md](prior-art-2.md), which already lists Bernstein's coprime-base and smooth-part papers as related work. This pass asks a narrower question: **how fast can a new denominator x learn which live nodes share a factor with it?**

## Tags

Every claim carries one tag:
- **[read]:** the primary text or source code was fetched this pass, sometimes through a summarising fetcher;
- **[abstract]:** only the abstract, metadata or slides were seen;
- **[memory]:** prior knowledge, not re-checked this pass.

## Where we start

| Approach | Cost per entry | Locality | Measured |
|---|---|---|---|
| **Seen-prime dictionary** | Trial division; then the known large primes; then Miller–Rabin and Pollard ρ on what remains. | Local, except possibly the large-prime step (see 4). | About even on decimals. 0.88 on 32–62-bit large-prime denominators. Much faster with many distinct denominators. |
| **Control** | One gcd against each live node. | Linear in the number of nodes. | Baseline. |

On large-prime data, the likely losses are two:
- trial division that never hits;
- ρ, or a long Miller–Rabin, on a cofactor that the control only ever gcds.

The techniques below address one or the other.

---

## 1. Fast divisibility by many small primes, one word at a time

### 1a. Multiply-by-inverse divisibility test (Granlund–Montgomery / Hacker's Delight §10-17)

- **What it does.** For an odd prime p, precompute two constants:
  - `inv = p⁻¹ mod 2⁶⁴`;
  - `lim = ⌊(2⁶⁴−1)/p⌋`.

  Then p | n exactly when `n·inv mod 2⁶⁴ ≤ lim` **[memory]**. The test is one multiply and one compare, with no division. Compilers emit it for `n % C == 0`; Go's `cmd/compile/internal/ssa/magic.go` has an implementation (link from search, not opened).
- **Cost.** About 1–3 cycles per prime for a 64-bit n, against about 20–40 cycles for a hardware `%`. The primes are independent, so the loop pipelines and vectorises.
- **Incremental or batch.** Incremental, one number at a time.
- **Use for us.** This is a drop-in replacement for each `%` in the small-prime table. It needs two `uint64` constants per table prime. It is the cheapest win, and it helps on every dataset.

### 1b. Lemire, Kaser, Kurz, "Faster remainder by direct computation" (2019)

- **What it does.** A divisibility test from the fractional part of n·c, where c ≈ 2⁶⁴/d. The idea: `n·c mod 2⁶⁴ < c` means d | n **[memory for the exact form]**. The abstract reports that it is "faster than state-of-the-art Granlund-Montgomery divisibility-test functions, sometimes by more than 50%" **[abstract]**.
- **Constraints.** It targets 32-bit n with 64-bit arithmetic. The 64-bit variant needs 128-bit products and is marked experimental in the library **[read: fastmod README]**.
- **Cost.** Similar to 1a.
- **Incremental or batch.** Incremental.
- **Use for us.** An alternative to 1a for denominators below 2³². For 33–64-bit values, 1a is the simpler choice.
- **Links:**
  - <https://arxiv.org/abs/1902.01961>
  - <https://github.com/lemire/fastmod>

### 1c. Prime products packed into a limb: one remainder, then tiny checks (GMP `mpz_probab_prime_p`)

- **What it does.** GMP multiplies consecutive odd primes into one limb q, as many as fit, and computes `r = n mod q` with one `mpn_mod_1` (preinverted) pass over n. It then tests `r % p` for each small p in the batch. The fixed constant `PP`, the primes up to 53 on 64-bit limbs, gets the same treatment with a precomputed inverse **[read: gmp/mpz/pprime_p.c]**.
- **Cost.** One pass over n per *batch* of about 4–15 primes, instead of one pass per prime. The per-prime checks run on a single word, so 1a applies to them as well.
- **Incremental or batch.** Incremental.
- **Use for us.**
  - For multi-limb denominators, this cuts trial-division passes several-fold.
  - For one-limb denominators, it replaces k tests on n with one `n mod q` followed by k tests on r, which is a small gain.
  - The first `PP` step is a good first filter: n mod PP is one division, and that one residue covers all 15 primes up to 53.

### 1d. Precomputed product of primes and one gcd (Bernstein's smooth-part step, used for one number)

- **What it does.** Let z be the product of all table primes. Then:
  - `y = (z mod x)^(2^(2^e)) mod x`, with the smallest e such that 2^(2^e) ≥ x;
  - gcd(x, y) is the whole table-smooth part of x, multiplicities included.

  This is Algorithm 2.1 of the smooth-parts paper with a single x **[read]**.
- **Cost.**
  - One remainder of z by x. z is fixed: primes below 2¹⁶ give about 1,500 limbs.
  - About e ≤ 6 modular squarings, then one gcd.
  - In return, there are no per-prime branches.
- **Incremental or batch.** Usable one number at a time with a precomputed z. It pays off mainly in batch, through the remainder tree.
- **Use for us.**
  - For one-limb x, a scan of 1,500 limbs (a few µs) probably loses to trial division with 1a.
  - Its real use for us is the same trick with z = **the product of live large nodes** (section 3).
  - Note also that Franke et al.'s "divide out gcd(z, x) repeatedly" loop can take many iterations, while repeated squaring followed by a single gcd avoids it **[read]**.
- **Link:** <https://nymity.ch/anomalous-tor-keys/bibliography/pdf/Bernstein04.pdf> (2004 draft)

### 1e. SIMD trial division

- **What it does.** Runs 1a across 4–16 primes per instruction. Examples:
  - 16- or 32-bit lanes for small primes;
  - AVX-512 `vpmullq` for 64-bit lanes.

  yafu has SSE2, SSE4.1 and AVX2 trial-division and resieve kernels (`tdiv_medprimes_32k_avx2`) for factor-base primes below 2¹⁶ **[read: header only]**. They serve sieving, where many n are tested against many p, not a single n.
- **Cost.** About 4–8× fewer instructions than scalar 1a for a long table.
- **Incremental or batch.** Incremental in n, vectorised over p.
- **Use for us.** Only worth it if profiling shows the small-prime loop still dominates after 1a.
- **Link:** <https://github.com/bbuhrow/yafu>

---

## 2. Bernstein's coprime-base algorithms: the incremental forms

**"Factoring into coprimes in essentially linear time,"** *J. Algorithms* 54 (2005). Section titles and the two algorithms below were read from the 2004-04-04 draft PDF **[read]**.

### 2a. split(a, P), Algorithm 15.3

- **What it does.** Given a coprime set P, it returns ppi(a, p) for each p ∈ P. Here ppi(a, p) is the part of a made of primes that divide p. The steps:
  1. compute b = ppi(a, prod P);
  2. halve P;
  3. recurse on both halves.

  Theorem 15.1 gives the decomposition: b = ppo(b, prod P) · ∏ₚ ppi(b, p).
- **Cost.** Essentially linear in lg a + lg(prod P), up to log factors (Theorem 15.4).
- **Incremental or batch.** One a at a time against all of P. Its cost scales with **prod P**, that is, with the size of the graph.
- **Use for us.** This is the theory behind section 3: a product tree over the nodes, descended only along branches where the gcd is nontrivial.

### 2b. Extending a coprime base by one element, Algorithm 16.2

- **What it does.** Computes cb(P ∪ {b}):
  1. take x = prod P;
  2. split b into a, the part over primes of x, and r, the part coprime to x;
  3. emit r as a new element;
  4. run split(a, P);
  5. run the two-element coprime base (Algorithm 13.2) on each pair (p, ppi(a, p)) that is not 1.

  The paper contrasts this with Bach–Driscoll–Shallit refinement, which goes element by element and is quadratic.
- **Cost.** Essentially linear in lg(b·prod P) (Theorem 16.3).
- **Incremental or batch.** **This is the incremental coprime-base insert.** It is exactly our entry step, done for the whole graph at once.
- **Use for us.** It is the principled form of "control, but fast". It still violates "no cost grows with the graph", since it is linear in the total size of the nodes. Section 3 is the practical version of it.
- **Software.**
  - `dcba-concept-20040407.cc`, a proof of concept with no stated licence **[read: listing]**.
  - FLINT `fmpz_factor_refine` produces pairwise-coprime bases **[read: docs]**. The algorithm it uses was not checked.
- **Links:**
  - <https://cr.yp.to/coprimes.html>
  - <https://cr.yp.to/lineartime/dcba-20040404.pdf>

### 2c. arithmoi `Coprimes.insert`, for contrast

- **What it does.** Walks the existing list with one gcd per element (`doPair`), and stops early once x has been reduced to 1 **[read: source]**.
- **Use for us.** This is our control. It confirms that no library seen so far does better than linear gcds for a single insert.
- **Link:** <https://hackage.haskell.org/package/arithmoi-0.8.0.0/docs/src/Math.NumberTheory.Euclidean.Coprimes.html>

---

## 3. Product of live nodes: one remainder, then a gcd

Batch gcd originates with Bernstein and was used by Heninger et al. for RSA keys. Its steps are a product tree, then a remainder tree mod X[i]², then a division and a gcd **[read: FactHacks]**. Our entry step is the case of one new number:

1. **Keep Z, the product of the live large nodes,** as a product tree whose leaves are the nodes.
2. **Test:** compute r = Z mod x, then g = gcd(r, x).
   - g = 1 means x shares nothing with any of those nodes. This is the common case for unrelated data.
   - For one-limb x, Z mod x is a single `mpn_mod_1`-type pass, at about 1–3 cycles per limb of Z. The control instead costs about 100+ cycles per node for a gcd.
3. **On a hit,** descend the tree: compute (left product) mod x, take the gcd, and recurse only where the gcd is not 1. This costs O(depth) remainders per hit, which is Algorithm 15.3 in practice.
4. **Splits leave Z unchanged,** because 30 → 6·5 replaces a leaf by a subtree with the same product. Only the new coprime part r of x multiplies in.
5. **Freed nodes can stay in Z as dead leaves.** A dead leaf only produces a false positive, which the descent detects and prunes. Z is rebuilt at idle times. So Z behaves as a **fingerprint that allows false positives but never misses a shared factor**.

- **Incremental or batch.** Incremental for each new x. The tree is maintained incrementally and rebuilt in batch.
- **Cost.**
  - Linear in the bits of Z, but with a constant 30–100× smaller than per-node gcds **[memory, estimate]**.
  - Total work still grows with the graph. To respect the rule strictly, cap Z, for example by keeping only the nodes created in the last N entries, and leave older large leftovers to the idle-time batch refinement that idea.md already plans.
- **Precedent for incremental use.** Kudelski Security ran batch gcd at scale and "test[ed] new keys incrementally" against stored product trees, with no method details **[abstract: slides]**. Other related work:
  - Zhukov and Rybakov (2013), "On solving big systems of congruences", keywords "merge gcd, gcd tree" **[abstract]**;
  - arXiv 2405.03166, a gcd-at-each-node tree for all-to-all gcd, batch only **[read, via summary]**.
- **Links:**
  - <https://facthacks.cr.yp.to/batchgcd.html>
  - <https://www.sambuz.com/doc/reaping-and-breaking-keys-at-scale-when-crypto-meets-big-presentation-897428>
  - <https://m.mathnet.ru/eng/pdma125>
  - <https://arxiv.org/abs/2405.03166>

---

## 4. Avoiding factoring at entry

This is not a single paper but a reading of the sources above together.

- **The point.** Bernstein's base needs no primes: coprime pieces are enough (dcba §1, "a substitute for prime factorization") **[read]**.
- **Applied to the dictionary.** It does not need ρ at entry either. After trial division, a cofactor c can:
  - be checked with Miller–Rabin only, or not at all;
  - be tested by section 3, without any factoring;
  - enter as a "leftover" node.

  ρ is then needed only when a gcd hit produces a composite piece that someone actually needs split. That is probably most of the 0.88 gap on large-prime data, since on that data the control never factors **[hypothesis]**.
- **Check the large-prime step.** If "exact division by the large primes already in the dictionary" loops over every large prime, then it is linear in the dictionary. In that case, replace it with the single remainder against their product from section 3, or with the tree descent.

## 5. Hashing and fingerprints

No hash scheme was found that detects a shared factor between unfactored integers. Coprimality is not preserved by hashing. The usable structures are these:

- **Prime-keyed dictionary (our design).** An inverted index that works once a prime is known.
- **Product Z (section 3).** The only "fingerprint" that reveals sharing with unknown factors. It allows false positives and never misses a shared factor.
- **Small-prime bitmask per node** (bit i set if prime i divides the node). A single AND finds the small primes two nodes share. This is useful when two values meet in an operation, not for a search at entry **[memory, folklore]**.

---

## What to try first

1. **Multiply-by-inverse tests (1a) in the small-prime loop**, with an exit once p² > the remaining cofactor, and `n mod PP` as a first filter (1c). Each change is a few lines, local and deterministic, and should help every dataset.
2. **No ρ at entry (section 4).** Run trial division, then the section-3 test, and keep composite cofactors as leftover nodes. This targets the 0.88 case directly.
3. **Product tree of live large nodes: one `Z mod x`, then a gcd, descending on a hit (section 3 and §2a).** Benchmark it against the control and the dictionary on the large-prime data. Cap Z, or confine it to recent nodes, so that it fits the no-growth rule.
4. **Batch refinement at idle times** with Bernstein smooth parts / dcba, or FLINT `fmpz_factor_refine` (§1d, §2b). This is already planned, and the libraries above can serve as reference implementations.
5. **SIMD trial division (1e).** Only if profiling still shows the small-prime loop after step 1.
