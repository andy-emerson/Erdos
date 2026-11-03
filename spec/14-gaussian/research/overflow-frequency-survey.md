# How often integers overflow, and how often representations bounce

Scope: published measurements of (1) how often integer values leave the machine word or a small-integer fast path in real programs, and (2) how often runtimes that switch representations switch back and forth. Context is our exact integer array type that promotes the whole array to the next word size on overflow and demotes it when every element fits with one bit of margin. Web research only, November 2026, light search. "Not found" means a light search turned up nothing, not that nothing exists. Follows section 4 of [overflow-survey.md](overflow-survey.md).

## 1. Summary

Hard frequency data exists in four places: one CPython measurement of int sizes (2015), one recent figure for SPEC CPU 2017 (about 1.5% of dynamic arithmetic instructions overflow, mostly deliberate wrapping), CPython's specialization miss ratios, and PyPy/RSqueak measurements of how often collections leave a specialized storage representation. Nothing was found that measures promotion rates in Lisp, Smalltalk, Erlang, Haskell, Julia, Ruby, APL, J, BQN, k/q, FLINT or GMP-based systems, and nothing measures values hovering near a size threshold. On hysteresis, the consistent engineering answer across V8, PyPy and HotSpot is to avoid switching back at all, or to cap the number of switches, rather than to use a margin.

## 2. Dynamic languages: how often values leave the small range

### 2.1 CPython integer sizes

Serhiy Storchaka counted the sizes of all ints created while running CPython's own test suite ([bpo-24138, msg242886](https://bugs.python.org/msg242886), May 2015). Size is the number of 30-bit digits on 64-bit builds (15-bit on 32-bit). Ints were about 13.5% of all objects created.

| Digits (64-bit) | Magnitude bound | Share | Cumulative |
|---|---|---|---|
| 0 | zero | 0.62% | 0.62% |
| 1 | below 2^30 | 75.92% | 76.54% |
| 2 | below 2^60 | 9.46% | 86.00% |
| 3 | below 2^90 | 4.43% | 90.43% |
| 4 to 6 | below 2^180 | 4.84% | 95.27% |
| 7 and up | larger | about 4.7% | 100% |

Read against our types: about 76.5% fit in 30 bits, about 86% fit in a 63-bit word, and about 14% exceed it. The test suite deliberately exercises big integers (test_long, test_math, test_decimal, hashing and id values), so the tail is heavier than in typical application code. It is still the only published size distribution I found for a real language workload, and its shape matters: the mass drops off smoothly across sizes, not as a cliff at the word boundary.

### 2.2 CPython specializing interpreter

Faster CPython's published pystats give a "miss ratio", the share of executions of a specialized instruction that deoptimized ([pystats summary, Feb 2024](https://gist.github.com/mdboom/d2b5ee67b45e0f1e9040484ca0e6fd99)): `BINARY_OP_ADD_INT` 0.0% of about 969 million executions, `BINARY_OP_SUBTRACT_INT` 0.1% of 526 million, `BINARY_OP_MULTIPLY_INT` 6.3% of 179 million. In that 3.13 development version the guards check that operands are exact ints, so misses are mostly type changes (for example int times str), not size overflow, and the specialized code itself handles non-compact ints. So this bounds overflow-driven deopts from above: for add and subtract they are essentially zero across pyperformance; multiply is the only operation with a visible miss rate. A per-run comparison ([benchmarking-public](https://github.com/faster-cpython/benchmarking-public/blob/main/results/bm-20240430-3.13.0a6%2B-04feb78-PYTHON_UOPS/bm-20240430-azure-x86_64-faster%252dcpython-unify_tier_2_for_ite-3.13.0a6%2B-04feb78-pystats-vs-base.md)) puts the whole `BINARY_OP` family at a 1.4% miss rate.

### 2.3 Other runtimes

- PyPy: the int/long split exists and the JIT emits overflow-checked machine ops, but no frequency data found.
- Lisp: Johnson, "Trap Architectures for Lisp Systems" ([UCB/CSD-88-470](https://www2.eecs.berkeley.edu/Pubs/TechRpts/1988/5733.html), 1988) reports that "small integer (fixnum) arithmetic dominates most programs" and that other number types can turn up on almost any operation; I did not get the per-benchmark figures from the PDF. Watt, "Look Before You Leap" ([arXiv 2606.05466](https://arxiv.org/html/2606.05466), 2026) re-measures tagging costs but uses synthetic values 1 to 10 and gives no overflow rates.
- Erlang: the OTP team designed the `rand` generators around 58-bit values so that intermediates never become bignums, which they say are "almost an order of magnitude slower" than immediates, and avoided 59-bit because that would need an overflow check before every add ([Erlang blog](https://www.erlang.org/blog/faster-rand/)). Evidence of cost, not of frequency.
- Julia: in the overflow-checking debates, the consensus position (stevengj and others) is that Int64 overflow is rare for counts and sizes and common mainly in number theory and combinatorics; one benchmark showed checked `sum` over Int32 about 5.7x slower, mostly from lost vectorization ([Discourse 3338](https://discourse.julialang.org/t/a-plea-for-int-overflow-checking-as-the-default/3338)). No measured rates.
- Smalltalk, Haskell (`IS` versus `IP`/`IN`), Ruby (Fixnum/Bignum, YJIT side exits), LuaJIT, V8 Smi to HeapNumber: no frequency data found. LuaJIT's [lj_opt_narrow.c](https://github.com/LuaJIT/LuaJIT/blob/9512d5c1aced61e13e7be2d3208ec7ae3516b458/src/lj_opt_narrow.c) argues that replacing every FP op with an integer op plus overflow check is "counter-productive" and narrows only where demanded, which is a design judgment, not a measurement.

## 3. Statically typed code: overflow in C, C++ and Rust

- Zhang, Ling, Kasad, Sweeney, Gupta, Garfinkel, Taram, "An Integer Overflow Endgame" ([USENIX Security 2026](https://mktrm.github.io/files/int_overflow_usenix26.pdf)): SPEC CPU 2017 "contains many instances of signed and unsigned integer overflows, around 1.5% of all dynamic arithmetic instructions". They exclude the known overflowing sites from checking, which implies these are concentrated in a few deliberate sites (hashing, PRNGs), consistent with Dietz et al. Check costs: UBSan 14.9% geomean on SPECint 2017 x86 (13.9% ARM, 8.1% RISC-V), Rust's built-in checks 3.5%, their compiler scheme 4.4% (11.7% with SIMD checks), proposed ISA support under 2%.
- Emmerich et al., Rust network driver ([arXiv 1909.06344](https://arxiv.org/pdf/1909.06344)): enabling overflow checks cost 0.8% at batch size 8 and nothing measurable at larger batches; the branches are always predicted.
- CBQN: checked narrow-integer arithmetic costs "around 10% overhead relative to an unchecked operation" for unfused vector primitives, more for multiply ([BQN implementation notes](https://mlochbaum.github.io/BQN/implementation/primitive/types.html)).
- Fuzzing: the OSS-Fuzz study by Ding and Le Goues ([arXiv 2103.11518](https://arxiv.org/abs/2103.11518)) does not break out integer overflow in its abstract; not pursued.

So in general-purpose code, unintended overflow is rare per operation, deliberate wrapping is about 1.5% of arithmetic and lives at a few sites, and a check that almost never fires costs a few percent when it does not block vectorization.

## 4. JIT speculation failures on integer overflow

- JavaScriptCore [bug 85752](https://bugs.webkit.org/show_bug.cgi?id=85752) (2012): int32 multiply whose result is truncated with `|0` speculated no-overflow, and that speculation "will typically fail", giving an OSR exit every time. The fix stopped speculating unless one operand is known small (below 2^17 in the patch). Multiply is again the operation where narrow speculation fails, matching 2.2.
- HotSpot JDK-8346989, cited in "Understanding and Finding JIT Compiler Performance Bugs" ([arXiv 2603.06551](https://arxiv.org/pdf/2603.06551)): with `Math.multiplyExact`, when overflowing arguments keep arriving, HotSpot "continuously hits a deoptimization point and fails to self-correct the speculation". Speculation bugs were 55 of 191 JIT performance bugs studied.
- Sol et al. ([SBLP 2010](https://www.llp.dcc.ufmg.br/Publications/Art2010/SBLP2010_rsol.pdf)) counted 3,958 overflow tests in TraceMonkey traces over 253 scripts but report how many they removed, not how many fired.

## 5. Array languages, computer algebra, databases

- CBQN promotes the whole array and squeezes results back to the narrowest type; Dyalog APL squeezes arrays "to the narrowest possible type" only "when a compaction occurs", that is at workspace garbage collection, or when `⎕SIZE` is called ([dfns type notes](https://dfns.dyalog.com/n_type.htm)). Demotion in Dyalog is therefore lazy and infrequent, which acts as time-based hysteresis. No frequency data for J, APL, BQN or k/q promotion was found.
- FLINT `fmpz` keeps values up to 2^62 − 1 inline and promotes to an `mpz` beyond that; "A value that fits inline is always stored inline", so demotion is eager and per value, with no margin ([FLINT fmpz](https://flintlib.org/doc/fmpz/index.html)). Per-value demotion costs one compare, so FLINT has no reason for hysteresis; per-array demotion costs a scan, which is the difference from our case. No published rate of leaving the fast path.
- Databases: no statistics on column overflow or type widening found, only incident reports, for example int4 keys exhausted at Buildkite ([incident](https://buildkitestatus.com/incidents/63zxhjsysszc)) and others ([Crunchy Data](https://www.crunchydata.com/blog/the-integer-at-the-end-of-the-universe-integer-overflow-in-postgres)). These are monotone counters that cross once and never come back.

## 6. Representation switching and thrashing

| System | Switch to wider | Switch back | Measured |
|---|---|---|---|
| V8 elements kinds | on first non-Smi, -0, NaN | never ("only possible to transition downwards through the lattice"; emptying does not reset) ([V8 blog](https://v8.dev/blog/elements-kinds)) | no |
| PyPy storage strategies | on first foreign element | only to empty when cleared | under 10% of specialized collections dehomogenise, typically holding one element at the time ([Bolz, Diekmann, Tratt 2013](https://tratt.net/laurie/research/pubs/html/bolz_diekmann_tratt__storage_strategies_for_collections_in_dynamically_typed_languages/)) |
| RSqueak strategies | as above | to empty only | 97% of objects reaching SmallIntegerOrNil stay there, 90% for FloatOrNil ([Pape et al. 2015](https://www.hpi.uni-potsdam.de/swa/publications/media/PapeFelgentreffHirschfeldGulenkoBolz_2015_LanguageIndependentStorageStrategiesForTracingJitBasedVirtualMachines_AuthorsVersion.pdf)) |
| HotSpot C2 | uncommon trap, recompile | stops speculating after trap limits (PerMethodTrapLimit 100, recompilation cutoff about 200) | a 2025 bug gave "thousands of deoptimizations per second" when the limits interacted badly ([JDK-8374307 review](https://mail.openjdk.org/pipermail/hotspot-compiler-dev/2025-December/101193.html)) |
| V8 Crankshaft | deopt | optimization disabled after `kMaxOptCount` reoptimizations ([compiler.cc](https://android.googlesource.com/platform/external/v8.git/+/afcf6d0da905bea0fdfb8b2c48629777819d4370/src/compiler.cc)) | no |
| Dyalog APL | on overflow | squeeze at GC only | no |
| FLINT fmpz | per value | immediately, per value | no |

Two lessons. First, the systems that measured found that switching to the wide form is uncommon and happens early, so they made it one way and accepted the occasional cost. Second, every JIT that allows switching back has needed a counter-based cap, because repeated fail-and-retry loops happen in practice and are expensive. None uses a value-based margin like ours; the margin is our own addition.

## 7. Values hovering near the threshold

No direct measurement found. What the data and the arithmetic suggest:

- The CPython distribution in 2.1 is smooth across sizes, so there is no special pile-up just under the word limit in general code.
- Whole-array demotion is already asymmetric. Promotion fires when any one element overflows; demotion needs every element to fit. For N independent values spread up to a bound near the threshold, the chance all fit in one bit less is about 2^-N, so large arrays rarely demote by accident. Bouncing is mainly a small-array problem, or a problem of arrays whose maximum is driven by a single element.
- The operations that move a maximum back and forth are additive: add then subtract, accumulate then reduce. One add or subtract grows magnitude by at most one bit. A one-bit margin therefore guarantees that after a demotion the next add or subtract cannot overflow, which removes the simplest bounce (x + y overflows, then − y returns under the line, then + y again).
- Multiply grows by many bits, so no small margin stops a multiply from promoting. But multiply chains grow monotonically; they promote once and stay. The multiply-then-reduce pattern (mod m with m near 2^31) is the one realistic bouncing loop a margin does not catch, and its reduced values sit uniformly in [0, m), which for any sizeable array makes all-fit-in-30-bits almost impossible, so it would stay wide.
- Number theory workloads (norms, powers, products) are the workloads Julia's community and JSC's experience single out as overflow prone, and they trend upward rather than oscillating.

## What the data says for our design

1. Overflow is rare per operation in general code and concentrated in multiply. Add and subtract specializations in CPython essentially never miss across pyperformance; multiply misses at about 6%, and both JSC and HotSpot have had multiply-specific speculation loops. Our design should expect promotions mostly from multiply and from long accumulations, and keep the narrow fast path as the default.
2. For our own workloads (Gaussian norms and products) the external data does not transfer. About a quarter of ints in CPython's test suite already exceed 30 bits and 14% exceed 60, which is a warning that exact-integer workloads leave small words often. Measure promotion counts per operation on our benchmarks.
3. When promotion happens it tends to be permanent (PyPy under 10% dehomogenise, RSqueak 97% stay, monotone database counters, growing number-theoretic values). That supports making demotion cheap and infrequent rather than eager: check it only when it is free (fused into a reduction we already do) or at natural boundaries, as Dyalog does at compaction.
4. The one-bit margin looks justified and sufficient for its specific purpose. It is the smallest margin that makes a single add or subtract after demotion overflow-free, which kills the add/subtract bounce, the only bounce mechanism I can construct for large arrays. A larger margin buys nothing against multiply, which jumps many bits anyway. For large arrays the all-elements-must-fit rule already provides most of the hysteresis, so the margin matters mainly for small arrays and for arrays whose maximum is one outlier.
5. The margin does not cover every loop. Every JIT that switches back also keeps a counter cap, because value-based rules alone did not prevent fail-and-retry storms. A cheap safeguard is a per-array or per-call-site count: after K promotions of the same array, stop demoting it (V8 and PyPy simply never demote). This is optional; I found no evidence it is needed for exact integer arrays, only that runtimes eventually needed it for speculation in general.
