# Big-integer survey: growing unsigned integers (magnitudes)

Oct 30, 2026 · Research pass, read-only on the code. Nothing here is adopted. It surveys how arbitrary-precision
**unsigned** integers are represented and computed, ranks the candidates under [`RULES.md`](../../../RULES.md), and
names the structure each one adds (RULES §4: stored / temporary / local). Sign handling is out of scope. It follows
[`overflow-survey.md`](../../11-matrix/research/overflow-survey.md), which settled *why* (results need about 90 bits on the current workload,
about 1,000 with general denominators) and argued for 64-bit limbs; this pass covers *how*. Threshold figures are quoted
from the systems' own source or tuning notes; they are machine-dependent and are given to show orders of magnitude.

**The sizes that matter here.** 90 bits = 2 limbs of 64 bits; 1,000 bits = 16 limbs. Every published crossover to a
subquadratic multiply lies at about 1,300 to 2,600 bits (§2.2). So on this workload every operation is in the schoolbook
range, and the measured weak point is expected to be gcd and allocation, not multiplication.

## 1. Representations

### 1.1 Digit base and size

| Option | What it is | Cost | Used by | For this project |
| --- | --- | --- | --- | --- |
| **Base 2^64 limbs** | each limb a full `uint64`; inner step is 64×64→128 multiply and add-with-carry | fewest limbs; needs a 128-bit product (`mulhi`) and, for Algorithm D, a 128/64 divide | GMP, FLINT, Go (`Word` = machine word), num-bigint (since 0.4), V8 (`digit_t` = pointer size), ibig, SBCL | **Recommended.** Shifts, bit length, gcd and exact division are all binary. |
| Base 2^32 limbs | 32-bit limbs, products in a native 64-bit | 2× the limbs, ~4× the inner multiplies; portable with no 128-bit type | Java `BigInteger` (`int[]`), Dart `BigInt`, Ruby (configurable `BDIGIT`) | Fallback only if a 128-bit product is unavailable. |
| Base 2^30 (or 2^15) | digits leave headroom bits in a 32-bit word, so carries are taken lazily and portable C needs no wide type | ~7% more digits than 2^32 | CPython (`PYLONG_BITS_IN_DIGIT` = 30) | No: Nim can reach a 128-bit product (below). |
| Reduced radix 2^52..2^60 in 64-bit words | headroom bits let many adds run with **no carry propagation** (redundant form, §4.1) | more limbs; one normalize pass later | crypto field code (radix 2^51 for Curve25519), SIMD bignums | Named special case for accumulation (§4.1), not the base form. |
| Base 10^k (10^9, 10^19) | decimal limbs | decimal I/O is O(n); binary ops (shift, gcd, exact division by 2^k) lose their tricks; multiply needs a division per carry | libmpdec (Python `decimal`, 10^19), some calculators | **No.** The project's decimals keep 10^k *denominators*, which are a value, not a base. Conversion is an output cost (§2.8). |

**The 128-bit primitive in Nim.** Nim has no `uint128`. Options: `{.emit.}`/`importc` of C's `unsigned __int128` (GCC and
Clang compile `a*b` to one `mul`), the `__builtin_*_overflow` intrinsics already used by `core.nim` for add-with-carry, or a
4×32-bit split in pure Nim (about 4× slower per step). A 128/64 **divide** via `__int128` calls libgcc's `__udivti3`
(tens of cycles); inline `divq` or the Möller–Granlund reciprocal (§2.4) avoid it. Exact division, binary gcd and
Lehmer's single-word steps need no 128/64 divide, which is one reason they are preferred below.

### 1.2 Order, normalization, header

| Choice | Options | Standard / recommended |
| --- | --- | --- |
| Digit order | little-endian (least significant limb first) or big-endian | **Little-endian**: carries run with the index, appending a limb is an amortized push. Everyone but Java (`mag[0]` is most significant) uses it. |
| Normalization | no leading zero limbs; zero is length 0 | **Required** (all systems). Length is then a magnitude test: `len(a) ≠ len(b)` ⇒ `a ≠ b`, and compare is O(1) unless the lengths match. |
| Header | `{len, cap, ptr}` (GMP `mpz`: `_mp_alloc`, `_mp_size`, `_mp_d`); slice `{ptr, len, cap}` (Go, Rust `Vec`); length + inline data in one object (CPython, V8, Java array) | One length is needed; capacity only if values are mutated in place. |
| Capacity policy | exact size (immutable systems); slack (Go `nat.make` adds a few words); shrink when capacity ≫ length (ibig keeps capacity within about 0.9n..1.125n) | Immutable values: exact. Mutable accumulators: slack. |

**Nim layout.** `seq[uint64]` is 16 bytes inline (length, pointer) plus a heap block with capacity and data, and has
value semantics under ARC/ORC (copy = deep copy unless moved). A `ref object` holding a `seq` is two allocations. A single
allocation with a trailing `UncheckedArray[uint64]` (manual `alloc`, with `=destroy`/`=copy`/`=sink` hooks or a refcount)
is one block: one stored structure, as RULES §4 counts it.

### 1.3 Small values inline (the "word while it fits" idea)

| Form | What it is | Cost | Used by |
| --- | --- | --- | --- |
| **Length-as-tag pair** | `{len, word-or-pointer}`; `len ≤ 1` means the word *is* the value, `len ≥ 2` means it points to `len` limbs | 16 B per value; small values keep the **full 64 bits** (unsigned magnitudes need no sign bit); one branch on `len` per operand | ibig (`Repr = Small(Word) | Large(Buffer)`, Large has length ≥ 2), GHC `Natural = NS Word# | NB BigNat#` |
| Tagged single word | one word; a tag bit chooses immediate vs pointer | 8 B per value; immediates lose 1–2 bits (63 or 62) | Lisp fixnum/bignum (SBCL: 63-bit fixnums), FLINT `fmpz` (≤ 2^62 − 1 inline), OCaml/Zarith, V8 Smi for Numbers (not BigInt) |
| Inline N limbs (small-vector) | the object carries N limbs inline, spills to heap past N | (N+1)·8 B for every value, even small ones; no allocation up to N limbs | Boost.Multiprecision `cpp_int` (internal limb cache), Ruby `RBignum` embedded digits, LLVM `APInt` (≤ 64 bits inline) |
| Always heap, small ones cached | every value a heap object; a table of preallocated small values | an allocation per new value | CPython (−5..256 cached; 3.12 "compact" one-digit fast path), Java (`BigInteger.ZERO/ONE/TEN` constants) |

**Canonical demotion.** All systems with an inline form keep "small ⇔ fits" (Lisp normalizes bignum results to fixnums;
GHC, ibig, FLINT's `_fmpz_demote_val`). It makes equality a representation test, makes hashing well-defined, and costs one
length check per big result. Inline-2 would hold the current workload's 90-bit values with no allocation, but doubles
the size of every small value; the length-as-tag pair plus a fast 2-limb path is the usual compromise.

### 1.4 Mutability and ownership

| Model | What it is | Systems | Trade |
| --- | --- | --- | --- |
| **Immutable values** | every result is a new object; sharing is a pointer copy | Java, CPython, V8/JS, Dart, GHC | simple, safe to share and intern; an allocation per result |
| Mutable destination | `op(dest, a, b)`; dest's buffer reused; aliasing (`dest == a`) allowed | GMP `mpz_*`, Go `z.Add(x, y)`, Julia `Base.GMP.MPZ.add!` | no allocation in loops; aliasing rules must be handled per operation |
| Caller-owned raw arrays | functions on `(ptr, len)` with the result area and scratch supplied by the caller | GMP `mpn_*`, V8 `bigint/` (`RWDigits`), FLINT `_fmpz_*` | the kernel layer under both models above; no allocation inside kernels |
| Move-reuse | operators consume an owned operand and reuse its buffer | num-bigint (`impl Add<BigUint> for BigUint`), Nim `sink` parameters | immutable-looking API, mutable cost when the operand is dead |

RULES' lesson "immutable stored values have paid off" fits the first row for stored values, with the third row
(kernels on caller-supplied limbs) underneath so that one operation can carve all intermediates from one temporary.

## 2. Algorithms per operation

n = limbs; M(n) = cost of an n-limb multiply. Crossovers in 64-bit limbs unless noted.

### 2.1 Add, subtract, shift, compare

| Operation | Algorithm | Cost | Notes |
| --- | --- | --- | --- |
| add | carry chain, then copy the longer operand's tail while the carry propagates | O(max(m, n)) | result ≤ max + 1 limbs; GMP `mpn_add_n` + `mpn_add_1` |
| subtract (a ≥ b) | borrow chain, then renormalize (strip high zeros) | O(m) | unsigned: `a < b` is the caller's error (RULES Optimization §2: expose it) or returns a flag; the sign layer decides order by compare first |
| compare | lengths, then limbs from the top | O(1) if lengths differ, else up to O(n) | needs normalization |
| shift left / right | word shift (move) + bit shift (one pass with `clz`-sized carry) | O(n) | bit length = 64·(len − 1) + (64 − clz(top)) |
| add/sub one limb | carry stops early | O(1) amortized | named special case |

### 2.2 Multiply and square

| Algorithm | Cost | Typical crossover (from the systems' tuning) | Structure |
| --- | --- | --- | --- |
| **Schoolbook** (row by row, `addmul_1` inner kernel: `r += a·b_j` with carry) | m·n limb multiplies | baseline | result area m + n limbs, no scratch |
| **Schoolbook squaring** (off-diagonal products once, double by a shift, add the squares) | ~n²/2 multiplies | GMP uses it at all sizes below Toom-2; Go `basicSqrThreshold` = 12 words | none extra |
| Karatsuba (Toom-2) | O(n^1.585) | GMP `MUL_TOOM22` ≈ 27 limbs (K8 file); Go 40 words; V8 34 digits; CPython 70 × 30-bit; Java 80 × 32-bit. **≈ 1,300–2,600 bits** | one temporary scratch of ~2n limbs (+ O(log n) for recursion) |
| Toom-3 / Toom-4 / unbalanced Toom (32, 42, …) | O(n^1.465), O(n^1.404) | GMP Toom-3 ≈ 81, Toom-4 ≈ 234 limbs; V8 193; Java 240 × 32-bit | scratch, as Karatsuba |
| FFT: Schönhage–Strassen (mod 2^N + 1) or 3-prime NTT + CRT | O(n log n log log n) | GMP ≈ 7,500 limbs (`MUL_FFT_THRESHOLD`); V8 1,500 digits | one temporary transform buffer |
| Harvey–van der Hoeven | O(n log n) | theoretical only | — |
| **Fused multiply-add** (`addmul`, `submul`) | same as multiply, no separate add pass | — | removes the product temporary; the natural kernel for dot products |

At 2–16 limbs none of the subquadratic rows pays. Unbalanced products (big × 1 limb: `mul_1`) are a named special case.

### 2.3 Exact division (the division rationals need)

| Algorithm | What it is | Cost | Notes |
| --- | --- | --- | --- |
| **Jebelean / Hensel (2-adic) exact division** | precompute `inv = d₀⁻¹ mod 2^64` by Newton (≈ 5 multiplies from a small table or seed); each quotient limb is `q = r₀·inv`, subtract `q·d`, shift by a limb, from the low end | O((m − n + 1)·n), only multiplies; no normalization, no correction | GMP `mpz_divexact`, `mpn_bdiv`; FLINT. Strip common factors of 2 first (d odd). |
| Bidirectional exact division (Krandick–Jebelean) | high half by ordinary division, low half by Hensel; they meet in the middle | about half the work | special case at larger sizes |
| Exact divide by one limb | one multiply per limb | O(m) | GMP `mpn_divexact_1`; **named special case** (gcds are often 1 limb) |
| Divide-and-conquer Hensel division | block version using fast multiply | O(M(n) log n) | GMP `DC_BDIV_QR` ≈ 52 limbs |

### 2.4 General division (quotient and remainder)

| Algorithm | Cost | Crossover | Notes |
| --- | --- | --- | --- |
| Divide by one limb | O(m), one 128/64 step per limb | — | with a precomputed reciprocal (Möller–Granlund 2-by-1) the step is two multiplies, no hardware divide |
| **Knuth Algorithm D** | O((m − n + 1)·n) | baseline | normalize (shift so the divisor's top bit is set), estimate each quotient limb from the top 2–3 limbs, at most 2 corrections; Möller–Granlund 3-by-2 reciprocal removes the hardware divide. Needs a normalized copy of divisor and remainder: **one local** if both live in one block |
| Burnikel–Ziegler (recursive) | ≈ 2·K(n) | Java 80 × 32-bit, V8 57, Go `divRecursiveThreshold` 40, GMP `DC_DIV_QR` ≈ 56 limbs | needs Karatsuba to pay |
| Newton reciprocal + Barrett (GMP "mu" division) | O(M(n)) | GMP `MU_DIV_QR` ≈ 1,900 limbs; V8 Newton 50 | |

Control does not need a remainder (overflow-survey §0). Algorithm D is needed only for Euclid-based gcd and decimal output.

### 2.5 gcd

| Algorithm | Cost | Crossover | Notes |
| --- | --- | --- | --- |
| Euclid (remainders) | O(n²) with Algorithm D, but each step is a full division | — | the textbook form |
| **Binary (Stein), multi-limb** | ~ n·64 iterations of O(n) shift/subtract: O(n²) with a large constant | baseline | needs no division; the current 64-bit gcd extended. Weak when sizes differ greatly |
| **Lehmer** (and the "hgcd2" 2-limb variant) | O(n²), about one full-size pass per ~32 quotient steps: several times faster than binary at 2+ limbs | GMP uses it below `GCD_DC` ≈ 555 limbs | works on the top 1–2 limbs to build a 2×2 cofactor matrix, applies it once; one local (the two working copies) |
| Accelerated / k-ary (Sorenson, Jebelean, Weber) | O(n²), different constant | — | right-shift k-ary is the binary analog of Lehmer |
| Half-gcd (Schönhage; Möller 2008), binary recursive (Stehlé–Zimmermann) | O(M(n) log n) | GMP `HGCD` ≈ 125, `GCD_DC` ≈ 555 limbs (~35,000 bits) | far above this workload |
| **Mixed size** (one operand 1 limb) | one big-mod-small pass, O(n), then the 64-bit gcd | — | **named special case**; GMP `mpn_gcd_1`. Common in rationals (Henrici's g is small) |
| Extended gcd / cofactors | Lehmer and half-gcd carry the cofactor matrix | — | for inverses and rational reconstruction later |

### 2.6 Conversion to and from decimal

| Direction | Schoolbook | Fast | Crossover |
| --- | --- | --- | --- |
| to decimal | repeated divide by 10^19 (one-limb division), emit 19 digits per step: O(n²) | divide-and-conquer by precomputed 10^(19·2^k) (Schönhage): O(M(n) log n) | GMP `GET_STR_DC` ≈ 12 limbs; Java 20 × 32-bit; V8 43 |
| from decimal | Horner in chunks of 19 digits (`mul_1` + `add_1`): O(n²) | split in halves, combine with a multiply by a power of 10 | GMP `SET_STR_DC` ≈ 248 limbs; V8 300 |

CPython ≥ 3.12 added subquadratic `int`↔`str` (in `_pylong.py`) after quadratic conversion was used for denial of
service. Output is off the hot path here.

## 3. How real systems do it

| System | Limb | Small form | Mutability / memory | Algorithms |
| --- | --- | --- | --- | --- |
| GMP `mpn` / `mpz` | 64-bit, little-endian | none (always a heap block; `mpz` header 16 B, sign in `_mp_size`) | `mpz` mutable destination with `realloc`; `mpn` caller-owned arrays; scratch via `TMP_ALLOC` (stack below a size, heap above) | everything above, assembly kernels per CPU, tuned thresholds (`gmp-mparam.h`) |
| CPython `int` | 30-bit digits | small-int cache −5..256; 3.12+ "compact" one-digit fast path | immutable, refcounted, length in the object | schoolbook, Karatsuba (70 digits); no Toom/FFT in C; subquadratic str conversion in Python |
| Java `BigInteger` | 32-bit `int[]`, **big-endian**, separate `signum` | none | immutable; a new array per result | Karatsuba 80, Toom-3 240, squaring thresholds 128/216, Burnikel–Ziegler 80, Schönhage conversion 20 (ints) |
| Go `math/big` | machine word, little-endian `nat []Word` | none | mutable receiver `z.Op(x, y)`, aliasing allowed, `nat.make` reuses capacity | Karatsuba 40, Karatsuba squaring 80, recursive division 40 (calibrated across five CPUs in 2025); Lehmer gcd |
| Rust num-bigint | `u64` `Vec`, little-endian | none | owned values, move-reuse of buffers | schoolbook, Karatsuba, Toom-3; Lehmer-less (binary/Euclid) gcd |
| Rust ibig / dashu | word | `Small(Word)` / `Large(Buffer ≥ 2 words)`, canonical | owned; compact capacity | Karatsuba, Toom-3, divide-and-conquer division, Lehmer gcd |
| GHC `ghc-bignum` | word | `Natural = NS Word# \| NB BigNat#`; `Integer = IS \| IP \| IN`; canonical | immutable `ByteArray#` on the GC heap | backends: native Haskell, GMP, or FFI |
| Lisp (SBCL), Scheme (Chez, Racket) | 64-bit (SBCL) | tagged fixnum (63-bit SBCL); results normalized to fixnum when they fit | immutable, GC | schoolbook + Karatsuba typical |
| Julia `BigInt` | GMP `mpz` | none (`Int` is fixed and checked) | wraps GMP with finalizers; in-place via `Base.GMP.MPZ` | GMP's |
| JavaScript BigInt (V8) | pointer-size digits | none (Smi is for Number only) | immutable heap objects; kernels in `src/bigint/` on caller arrays | Karatsuba 34, Toom-3 193, FFT 1,500, Burnikel 57, Newton 50 digits; fast to-string 43 |
| Dart `BigInt` | 32-bit digits (VM) | `int` is a separate 64-bit type | immutable | schoolbook, Karatsuba-free in the core library |
| Ruby `Integer` | `BDIGIT` 32/64-bit | Fixnum immediates (merged into `Integer` in 2.4); short bignums embedded in the object | immutable | own Karatsuba/Toom-3; optional GMP for large mul/div/conversion |
| FLINT `fmpz` | GMP limbs | inline `slong` ≤ 2^62 − 1, tag bit means pointer to a pooled `mpz` | mutable destination; `mpz` pool; canonical demotion | GMP's, plus matrix kernels that accumulate dot products in 2–3-limb sums before normalizing |

## 4. Less usual proposals

| Proposal | What it is | Cost | Structure | Pays when | For this project |
| --- | --- | --- | --- | --- | --- |
| **4.1 Redundant / carry-save** | hold a value as limbs with headroom (reduced radix) or as a pair whose sum is the value; adds skip carry propagation; one normalize at the end | more limbs; a normalize pass before any compare/gcd | none if the accumulator is a fixed handful of words; else a local | long sums of products: **dot products** (Σ aᵢbᵢ into a 3-limb accumulator with one carry word, FLINT-style); hardware multipliers | Named special case inside matrix multiply: accumulate 128-bit products into a 192-bit sum, normalize once per cell |
| 4.2 Residue number system (multi-modular) | value as residues mod k coprime word primes; add/mul per residue, in parallel | O(k) per add/mul; **compare, divide, gcd, overflow detection need CRT** (O(k²)) | a residue vector (stored or temporary) | many multiplies, few comparisons, a proven size bound; large matrices with multi-limb entries | overflow-survey §4 covers it; not at 2–16 limbs, and gcd-heavy rationals defeat it |
| 4.3 Variable-length byte encodings (LEB128, protobuf varint, SQLite varint, prefix varint) | 7 bits per byte + continuation bit (or a length prefix) | encode/decode O(n); no arithmetic in this form | the encoded bytes (a stored structure if kept) | storage, serialization, hashing, a packed list of mixed-size integers | possibly for a matrix's packed list (mixed small/big entries inline, no pointers); not for arithmetic |
| 4.4 Lazy / expression integers | keep an expression (DAG) and evaluate when a value is needed; or fuse at compile time (Boost.Multiprecision expression templates turn `a = b*c + d` into one fused op) | evaluation deferred, not avoided; a DAG node per op if lazy at run time | DAG = a stored structure; expression templates add none | when results are discarded or fusible (`addmul`); for exact reals (constructive reals) | the fused-op half is useful (`addmul`, `submul`, Nim templates); run-time laziness is not |
| 4.5 Interning / sharing | hash-cons equal big values so each exists once; or share immutable values by pointer + refcount | a hash and lookup per created value; a global table | a global table (needs discussion: belongs to no one object) | many duplicate big values; equality becomes a pointer test | sharing by refcount is free with immutable values; a global intern table is not obviously worth it |
| 4.6 Arena / pooled allocation | bump-allocate all intermediates of one operation from one block, free at once (GMP `TMP_ALLOC`, V8 scratch); free lists by size class (CPython freelists, FLINT `mpz` pool) | O(1) allocate, no per-object free; arena sized from operand lengths | **one temporary per operation** (the arena) | many short-lived intermediates; 2–16-limb values where malloc dominates the arithmetic | Fits RULES §4 directly: the same arena serves add, multiply, gcd and exact division (RULES §3) |

## 5. Ranking

**Candidates in one table** (sizes: 2–16 limbs now).

| Candidate | What it does | Cost | Structure | Pays when | Combines with |
| --- | --- | --- | --- | --- | --- |
| 64-bit limbs, little-endian, normalized | the base form | ⌈bits/64⌉ limbs | stored: the limb array | always | everything |
| Length-as-tag small form + canonical demotion | 64-bit word inline until it overflows, back when it fits | 16 B per value; one branch per operand, one length check per result | none extra (fixed data) for small; the limb array for big | most values fit 64 bits | all operations; 2-limb fast path |
| Schoolbook add/sub/compare/shift | carry chains | O(n) | none (result area) | always | arena |
| Schoolbook multiply + `addmul_1` kernel, squaring | row-by-row | mn; n²/2 for squares | none | below ~25–40 limbs | fused dot product, arena |
| Karatsuba / Toom / FFT | subquadratic multiply | n^1.585 / n^1.465 / n log n | temporary scratch | above ~1,500–2,500 bits / ~5,000 bits / ~500,000 bits | Burnikel–Ziegler, half-gcd, fast conversion |
| Jebelean exact division (+ 1-limb case) | quotient known to be exact, from the low end | (m − n + 1)n multiplies, no divide | none | every reduction to lowest terms | gcd |
| Algorithm D (+ 1-limb case) | quotient and remainder | (m − n + 1)n, needs 128/64 divide or reciprocal | one local (normalized copies in one block) | Euclid, decimal output | Möller–Granlund reciprocal |
| Binary gcd, multi-limb | gcd with shifts/subtracts | ~64n² limb ops | one local | baseline only | mixed-size case |
| Lehmer gcd | gcd with word-sized cofactor steps | O(n²), several × fewer full passes | one local | 2+ limbs, gcd-heavy (rationals) | exact division, cofactors |
| Mixed-size gcd | big mod 1 limb, then 64-bit gcd | O(n) | none | one operand 1 limb | every gcd |
| Half-gcd | subquadratic gcd | O(M(n) log n) | temporary | above ~30,000 bits | fast multiply |
| Carry-save dot-product accumulator | sum products without propagating carries | 1 normalize per sum | fixed handful (data) | matrix multiply inner loop | fused `addmul` |
| Arena per operation | one block for all intermediates | O(1) per carve | one temporary | malloc dominates at small n | everything |
| RNS / multi-modular | arithmetic on residues | k per op, k² to reconstruct | temporary residue vectors | large n, multi-limb entries, few gcds | CRT, cleared denominators |
| Varint encoding | compact storage | O(n) encode/decode | stored bytes | packed storage, hashing | small form |
| Interning | one copy per value | hash per creation | global table | many duplicates | immutable values |

**Recommended baseline (schoolbook, RULES Optimization §1).** 64-bit limbs, little-endian, normalized; one heap block per
big value holding its length and limbs (one stored structure); immutable stored values; kernels on caller-supplied limb
areas. Operations: add, subtract (a ≥ b, else an exposed error), compare, shifts, schoolbook multiply on an `addmul_1`
kernel, Jebelean exact division, multi-limb binary gcd, schoolbook one-limb division for decimal output. The 128-bit
product through `__int128` via `{.emit.}` or the builtins `core.nim` already uses. No inline small form yet: it is the
first measured optimization, so the baseline is the control it must match value for value.

**Next improvements, in the expected order** (each to be confirmed by measurement):
1. **Small form with canonical demotion** (length-as-tag pair, full 64-bit word inline; promotion at the existing
   overflow checks). Keeps today's speed on values that fit.
2. **Arena per operation** for intermediates (one temporary), if allocation shows in the profile.
3. **Named special cases:** one-limb operand in multiply, exact division and gcd (mixed-size gcd); 2-limb fast paths
   (the 90-bit workload).
4. **Lehmer gcd** (2-limb hgcd2 form) once multi-limb gcd dominates, which is expected for rationals.
5. **Fused dot-product accumulation** (carry-save into 3 limbs, normalize once per cell) in matrix multiply.
6. Schoolbook squaring; Algorithm D with a precomputed reciprocal if Euclid or output needs it.
7. Karatsuba only if values pass ~25–40 limbs (≈ 1,600–2,600 bits); Toom, FFT, Burnikel–Ziegler, half-gcd, fast decimal
   conversion are not indicated at the project's sizes.

## Sources

- D. E. Knuth, *TAOCP* Vol. 2, 3rd ed., §4.3.1 (multiple precision, Algorithm D), §4.3.3 (fast multiplication),
  §4.4 (radix conversion), §4.5.2 (binary gcd, Lehmer).
- R. Brent, P. Zimmermann, *Modern Computer Arithmetic* (2010), ch. 1–2. https://arxiv.org/abs/1004.4710
- T. Jebelean, "An algorithm for exact division", *J. Symbolic Computation* 15 (1993); W. Krandick, T. Jebelean,
  "Bidirectional exact integer division", *JSC* 21 (1996).
- N. Möller, T. Granlund, "Improved division by invariant integers", *IEEE Trans. Computers* 60 (2011).
- N. Möller, "On Schönhage's algorithm and subquadratic integer gcd computation", *Math. Comp.* 77 (2008).
- C. Burnikel, J. Ziegler, "Fast recursive division", MPI-I-98-1-022 (1998).
- D. Harvey, J. van der Hoeven, "Integer multiplication in time O(n log n)", *Annals of Math.* 193 (2021).
- GMP manual (Algorithms, Internals): https://gmplib.org/manual/ ; x86_64 thresholds:
  https://third-party-mirror.googlesource.com/gmp/+/refs/heads/master/mpn/x86_64/gmp-mparam.h
- Go `math/big` threshold calibration: https://tip.golang.org/src/math/big/calibrate ;
  https://go.googlesource.com/go/+/8bf5bf51738a198902e03bbec7e6ff220f4cb002/src/math/big/natdiv.go
- OpenJDK, JDK-8022181 "Tune algorithm crossover thresholds in BigInteger":
  https://mail.openjdk.org/pipermail/core-libs-dev/2013-December/023668.html ; https://bugs.openjdk.org/browse/JDK-8022181
- V8 `src/bigint/bigint-internal.h` (thresholds):
  https://chromium.googlesource.com/v8/v8/+/6d4ba583dff27894f143d54ae6da3b185fbe2803/src/bigint/bigint-internal.h ;
  "Adding BigInts to V8": https://v8.dev/blog/bigint
- ibig `UBig` representation: https://docs.rs/ibig/0.3.6/x86_64-pc-windows-msvc/src/ibig/ubig.rs.html ; dashu-int:
  https://docs.rs/crate/dashu-int/latest/source/src/ibig.rs
- ghc-bignum: https://hackage-content.haskell.org/package/ghc-bignum
- CPython `Include/cpython/longintrepr.h`, `Objects/longobject.c` (30-bit digits, `KARATSUBA_CUTOFF`), `Lib/_pylong.py`.
- FLINT `fmpz` documentation (inline small values, promote/demote): https://flintlib.org/doc/fmpz.html
- Boost.Multiprecision (`cpp_int`, expression templates): https://www.boost.org/doc/libs/release/libs/multiprecision/
- DWARF/LEB128 (DWARF v5 §7.6); Protocol Buffers varint encoding: https://protobuf.dev/programming-guides/encoding/
