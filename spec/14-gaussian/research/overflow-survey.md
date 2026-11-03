# Integer overflow handling: survey

Scope: strategies for detecting and handling integer overflow, with attention to our design (Gaussian integer a + bi as two int32 halves in one 64-bit word, vector registers, chip-agnostic, branch-free sweep, register-held sticky flag, whole-array redo at the next word size). Web research only, November 2026. Claims marked "not found" mean a light search turned up nothing, not that nothing exists.

## 1. Strategies in use

### 1.1 Language runtimes

| System | Default on overflow | Mechanism |
|---|---|---|
| Python (CPython) | promote to bignum, per value | Every int is arbitrary precision ([PEP 237](https://peps.python.org/pep-0237/)). Since 3.11 the interpreter specializes `BINARY_OP` for small ("compact") ints and falls back when an operand is not compact ([PEP 659](https://peps.python.org/pep-0659/)). |
| Common Lisp, Scheme | fixnum to bignum, per value | Tagged fixnums; generic arithmetic checks the result and allocates a bignum on overflow ([CLHS fixnum](http://www.lispworks.com/documentation/HyperSpec/Body/t_fixnum.htm)). |
| Smalltalk (Squeak/Pharo, Self) | SmallInteger to LargeInteger | Tagged add then branch on overflow flag; SPARC's tagged trapping add was designed for this ([discussion](https://yarchive.net/comp/lisp_support.html)). |
| Erlang | small to bignum | Small integers are immediate; overflow allocates a bignum on the heap ([Erlang memory guide](https://www.erlang.org/doc/system/memory.html)). |
| Haskell | `Integer` small/big constructors | `ghc-bignum` represents `Integer` as `IS` (machine int), `IP`/`IN` (big); primitives like `addIntC#` return a carry used to switch ([ghc-bignum](https://hackage.haskell.org/package/ghc-bignum)). `Int` wraps. |
| Julia | wrap silently | Documented choice for speed ([manual, overflow behavior](https://docs.julialang.org/en/v1/manual/integers-and-floating-point-numbers/#Overflow-behavior)); `Base.Checked.checked_add` and the [SaferIntegers.jl](https://github.com/JeffreySarnoff/SaferIntegers.jl) package for checking. |
| Swift | trap | Arithmetic traps; `&+`, `&*` wrap ([Swift book, overflow operators](https://docs.swift.org/swift-book/documentation/the-swift-programming-language/advancedoperators/#Overflow-Operators)). |
| Rust | panic in debug, wrap in release | Decided in [RFC 560](https://rust-lang.github.io/rfcs/0560-integer-overflow.html); `overflow-checks` profile switch ([Cargo](https://doc.rust-lang.org/cargo/reference/profiles.html#overflow-checks)); explicit `checked_*`, `wrapping_*`, `saturating_*`, `overflowing_*` ([i32 docs](https://doc.rust-lang.org/std/primitive.i32.html#method.checked_add)). |
| Java | wrap | `Math.addExact` and friends throw ([Javadoc](https://docs.oracle.com/javase/8/docs/api/java/lang/Math.html#addExact-int-int-)); HotSpot intrinsifies them to add plus branch on overflow flag. |
| C#/.NET | wrap unless in `checked` | Lexically scoped `checked`/`unchecked` blocks and a compiler switch ([docs](https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/statements/checked-and-unchecked)). |
| C23, GCC/Clang | UB (signed) | `ckd_add`/`ckd_sub`/`ckd_mul` in `<stdckdint.h>` return an overflow bool ([cppreference](https://en.cppreference.com/w/c/numeric/ckd_add)); same as [`__builtin_add_overflow`](https://gcc.gnu.org/onlinedocs/gcc/Integer-Overflow-Builtins.html). `-ftrapv`, `-fsanitize=signed-integer-overflow`. |
| Ada (GNAT) | raise Constraint_Error | Three modes for intermediates: STRICT (check every op), MINIMIZED (compute intermediates in the widest native type, check only at assignment), ELIMINATED (compute intermediates in arbitrary precision, check only at assignment) ([GNAT UG](https://gcc.gnu.org/onlinedocs/gcc-4.8.3/gnat_ugn_unw/Overflow-Checking-Modes-in-GNAT.html)). |
| JavaScript engines | Number is double | V8 speculates int32 (Smi) from type feedback, checks overflow with a branch, and deoptimizes to the interpreter on failure; a truncation analysis drops checks where only the low 32 bits are used, e.g. `(x+y)\|0` ([Sevcik, Meurer, Neis slides](https://docs.google.com/presentation/d/1wZVIqJMODGFYggueQySdiA3tUYuHNMcyp_PndgXsO1Y/htmlpresent); [V8 deoptimization](https://chromium.googlesource.com/v8/v8.git/+/main/docs/runtime/deoptimization.md)). JavaScriptCore has a dedicated [integer range optimization phase](https://github.com/WebKit/WebKit/blob/main/Source/JavaScriptCore/dfg/DFGIntegerRangeOptimizationPhase.cpp). |

Two items in the table are not on our list. GNAT MINIMIZED/ELIMINATED is "compute wide, check narrow once at the end of an expression". V8 is "speculate narrow, deoptimize and remember", where the remembering (type feedback) means the next execution starts at the wider representation instead of failing again.

### 1.2 Array languages and dataframes

- J: integer results that overflow become floating point; extended integers only on request with the `x` suffix ([J forum thread](https://groups.google.com/a/jsoftware.com/g/forum/c/3mul3lxK8X4)). Whole array goes to float. No demotion back in that thread.
- Dyalog APL: overflow goes to 64-bit float, or to 128-bit decimal float when `⎕FR=1287` ([⎕FR](https://help.dyalog.com/latest/Content/Language/System%20Functions/fr.htm)). The interpreter also "squeezes" arrays to the smallest type (1-bit, int8, int16, int32, float) ([Dyalog course, interpreter internals](https://course.dyalog.com/Interpreter-internals/); [181⌶ unsqueezed type](https://help.dyalog.com/latest/Content/Language/I%20Beam%20Functions/Unsqueezed%20Type.htm)).
- CBQN (BQN): the closest relative of our design that I found. Dyadic arithmetic on i8/i16/i32 arrays computes each element in the next wider type and compares with the narrow cast; on the first mismatch it frees the partial result and redoes the whole operation from the untouched inputs at a wider type (float for i32), then squeezes the result back to the narrowest fitting type. See the `DOI8`/`DOI16`/`DOI32` macros and `squeeze_numNewTy` in [src/builtins/arithd.c](https://github.com/dzaima/CBQN/blob/develop/src/builtins/arithd.c). It uses an early-exit branch per element rather than a sticky flag.
- q/kdb+: wraps; the minimum value is the null `0N` and `±0W` are infinities, so overflow can land on a null or infinity without any signal ([PyKX nulls and infinities](https://code.kx.com/pykx/user-guide/fundamentals/nulls_and_infinities.html)).
- NumPy: array arithmetic wraps silently; scalar operations warn ([NumPy overflow errors](https://numpy.org/doc/stable/user/basics.types.html#overflow-errors)); NumPy 2 changed scalar promotion so Python ints no longer widen arrays ([NEP 50](https://numpy.org/neps/nep-0050-scalar-promotion.html)).
- R: integer overflow yields `NA` with a warning ([R integer](https://stat.ethz.ch/R-manual/R-devel/library/base/html/integer.html)).
- pandas/Polars: wrap like NumPy/Arrow; explicit shrinking via [`pandas.to_numeric(downcast=...)`](https://pandas.pydata.org/docs/reference/api/pandas.to_numeric.html) and [Polars `shrink_dtype`](https://docs.pola.rs/api/python/stable/reference/expressions/api/polars.Expr.shrink_dtype.html).
- Apache Arrow compute: separate wrapping (`add`) and checked (`add_checked`, errors) kernels ([pyarrow](https://arrow.apache.org/docs/python/generated/pyarrow.compute.add_checked.html)).

### 1.3 Databases

- PostgreSQL: error per operation ("integer out of range").
- SQLite: `sum()` raises "integer overflow"; `total()` always returns a float ([aggregate functions](https://www.sqlite.org/lang_aggfunc.html)). Plain integer arithmetic that overflows yields a REAL.
- DuckDB: checked operators that raise an error, plus statistics-driven elision (section 3). Sums of int32 are widened (int64/int128 result types).
- ClickHouse: wraps, C++ semantics.
- Materialize: wrapping add/subtract in auto-vectorizable loops, then a separate branch-free pass ORs overflow flags across the batch; only if set does it go back to find the offending indices. Multiplication keeps per-element checked multiply because the author saw no cheap bulk test ([PR 35464](https://github.com/MaterializeInc/materialize/pull/35464)).
- CedarDB: vectorized sum ORs the sign-bit overflow test into a `hadOverflow` vector, checked once after the loop; the test is one `vpternlogd` (imm 0x42) on AVX-512. Measured 7% over unchecked, versus 4.4x for compiler-generated per-element checks ([Sichert, Dec 2024](https://cedardb.com/blog/vectorized_overflows/)).

### 1.4 Compilers

- Check elimination by range analysis: Sol, Guillon, Pereira, Bigonha, "Dynamic Elimination of Overflow Tests in a Trace Compiler", CC 2011, removed 91.82% of overflow tests in TraceMonkey's suite ([Springer](https://rd.springer.com/chapter/10.1007/978-3-642-19861-8_2); [thesis](https://www.llp.dcc.ufmg.br/Publications/MasterThesis2011/2011-msc-MarcosRodrigoSolSouza.pdf)).
- Truncation analysis (V8 TurboFan, above): if consumers only need the result modulo 2^32, no check is needed.
- Deferred, coalesced checks: Gubner and Boncz, "Exploring Query Execution Strategies for JIT, Vectorization and SIMD", ADMS 2017, propose to "continue execution but just OR the overflow flag results of all operations together" and raise only at the end of the query or morsel; they also note SIMD lacks overflow flags so "overflow prevention is crucial for SIMD" ([PDF](https://ir.cwi.nl/pub/27565/27565.pdf)).
- Wide intermediates: GNAT MINIMIZED/ELIMINATED (above).
- Cost of naive checking: Dietz, Li, Regehr, Adve (ICSE 2012) measured IOC on SPEC CPU 2006 at a mean 44% slowdown with precondition tests and 30% with flag tests ([PDF](https://www-old.cs.utah.edu/~regehr/papers/overflow12.pdf); extended in [TOSEM 2015](https://www-old.cs.utah.edu/~regehr/papers/tosem15.pdf)).

### 1.5 Hardware

| Mechanism | ISA | Sticky? | Vector? |
|---|---|---|---|
| Trapping add (`add`, `addi`, vs `addu`) | MIPS | no, trap | no |
| `/V` trapping qualifiers | Alpha | no, trap | no |
| `INTO` trap on OF | x86-32 only, invalid in 64-bit mode ([ref](https://www.felixcloutier.com/x86/intn:into:int3:int1)) | no | no |
| Tagged add, trap on overflow or tag (`TADDccTV`) | SPARC V8 | no, trap | no |
| Program-mask fixed-point overflow interrupt | IBM S/360 onward | no | no |
| XER[SO] summary overflow | PowerPC | yes | no |
| Q flag; FPSR.QC set by saturating SIMD ops incl. 32/64-bit lanes (`SQADD`) | ARM ([SQADD](https://developer.arm.com/documentation/ddi0602/latest/SIMD-FP-Instructions/SQADD--Signed-saturating-add-)) | yes | yes |
| `vxsat` set by `vsadd`/`vssub` and narrowing clips, any SEW | RISC-V V ([vxsat](https://riscv-software-src.github.io/riscv-unified-db/manual/html/isa/isa_20240411/csrs/vxsat.html)) | yes | yes |
| Saturating add `PADDSB/PADDSW` | x86 SSE2/AVX ([ref](https://www.felixcloutier.com/x86/paddsb:paddsw)) | no flag | 8/16-bit lanes only |
| `vpternlogd` for the 3-input sign test | x86 AVX-512 ([ref](https://www.felixcloutier.com/x86/vpternlogd:vpternlogq)) | n/a | yes |
| `add.sat.s32` | NVIDIA PTX ([ref](https://docs.nvidia.com/cuda/parallel-thread-execution/index.html#integer-arithmetic-instructions-add)) | no flag | per thread |

RISC-V scalar has no flags by design; the unprivileged spec gives the branch sequences for overflow checks ([ISA manual](https://github.com/riscv/riscv-isa-manual)). The point worth noting: both ARM and RISC-V vector units already have a hardware sticky saturation flag usable for int32 and int64 lanes, but x86 has neither 32-bit saturating adds nor a sticky bit, so it cannot be the portable mechanism.

### 1.6 GPU and SIMD practice

GPU code wraps by default; there is no overflow flag in PTX, only saturating forms. SIMD practice is the software sign test from Hacker's Delight (Warren, 2nd ed., section 2-13), either per element or accumulated (CedarDB, Materialize, Gubner/Boncz). For multiply, practice is widening multiply (x86 `vpmuldq`, ARM `smull`, RVV `vwmul`) then a high-half test, or per-element scalar checks.

## 2. Prior art for a software sticky flag over SIMD lanes

Direct precedents, all from the last decade:

1. Gubner and Boncz, ADMS 2017: OR overflow results together, check at end of morsel; paired with min/max bounds to avoid checks. Proposal, with a fallback to a slower checking path when bounds exceed the native type.
2. CedarDB (Sichert, 2024): AVX-512 sign-bit test ORed into a vector, tested after the loop; 5 to 7% overhead.
3. Materialize PR 35464: wrapping vector loop, separate OR-reduction pass, slow path to locate offenders. Add and subtract only.
4. CBQN: not sticky (early exit), but has the "discard result, redo whole array from untouched inputs at wider type, then squeeze" structure.
5. The same accumulate-errors-then-test-once pattern is standard in SIMD validation code, e.g. Keiser and Lemire, "Validating UTF-8 in less than one instruction per byte" ([arXiv 2010.03090](https://arxiv.org/abs/2010.03090)), and [simdutf8](https://github.com/rusticstuff/simdutf8).

I found no published system that combines the sticky flag with a recompute into a wider multi-word representation for a compound number type like a Gaussian integer.

## 3. Range and bound tracking

Static and dynamic bound tracking is used and works well where values are bounded by construction.

- DuckDB propagates min/max statistics through `+`, `-`, `*`; when both children have stats and the bound computation itself does not overflow, it swaps the checked operator for the unchecked one at plan time (`PropagateNumericStats`, `AddPropagateStatistics`, `MultiplyPropagateStatistics` in [arithmetic.cpp](https://github.com/duckdb/duckdb/blob/main/src/function/scalar/operator/arithmetic.cpp)). This is per-column, value-range tracking, not bit-width tracking.
- Gubner and Boncz (above): use column domains to move "from overflow-checking to overflow prevention"; sums need tuple-count bounds per morsel.
- Sol et al.: 91.82% of checks removed in a JS trace JIT.
- V8 and JSC: type feedback, range typing and truncation analysis remove most Smi checks in asm.js-like code.
- Arithmetic libraries: lazy reduction in NTTs keeps values in [0, 4p) and reduces only when headroom runs out (Harvey, [arXiv 1205.2926](https://arxiv.org/abs/1205.2926)); [fiat-crypto](https://github.com/mit-plv/fiat-crypto) proves limb bounds statically so carries happen only where needed.

How well it works: excellent for add chains and bounded domains (statistics, counters, mod-p arithmetic); poor for long multiply chains and data-dependent loops, where bit-width bounds double per multiply and quickly exceed the word. Gubner and Boncz and DuckDB both keep a checked fallback. For Gaussian integers, |a|,|b| < 2^k gives product halves < 2^(2k+1), so bound tracking saves a check only for factors below about 2^15.

## 4. How often overflow happens

Hard data is thin.

- C/C++: Dietz et al. found 219 static overflow sites in 8 of 12 SPEC CINT2000 benchmarks, mostly intentional (hashing, PRNGs, bit tricks), and undefined overflows in nearly every large codebase they ran (e.g. 28 in Python, 29 in PostgreSQL). Overflow is common as a deliberate idiom, rare as an arithmetic surprise.
- JIT experience: Sevcik, Meurer and Neis describe deopt loops as a real cost when speculation fails repeatedly, which motivates remembering the failure.
- Python, Lisp, J/APL promotion frequency: not found. No published measurements of how often CPython ints leave the compact range or how often J promotes to float turned up in a light search.

For number theory workloads like ours the answer is workload-specific (norms, products and powers grow fast), so measuring promotion rate on our own benchmarks is the only reliable data.

## 5. Whole-array promotion and demotion

- Promotion of the whole array: J (to float), Dyalog (to float or decimal), CBQN (to the next int width or float), NumPy only by explicit cast.
- Automatic demotion: Dyalog squeezes "occasionally", at unspecified points; CBQN squeezes after results that went to float. Both demote as soon as the values fit, with no margin.
- Explicit demotion: pandas `downcast`, Polars `shrink_dtype`.
- Storage-level width selection: columnar stores choose bit widths per segment (bit-packing, frame of reference), recomputed on write.
- Hysteresis: not found in any array language or dataframe. The closest common analogue is container resizing (grow at full, shrink at one quarter) to avoid thrash. Our "one bit of margin" rule is a mild form; an op-count or size-ratio cooldown would be a stronger one.

## Options we have not considered

1. Compute wide, check narrow (CBQN, GNAT MINIMIZED). For Gaussian multiply, ac − bd with int32 inputs is exact in int64 lanes, so a widening multiply plus a "fits in int32" test replaces the sign-bit trick and handles multiply, which the XOR identity does not. Fits: widening multiplies exist on all three ISAs' vector units.
2. Keep the prefix instead of redoing from scratch. When the flag is checked per chunk, everything before the first overflowing chunk is correct and can be sign-extended into the wide array; only the rest is recomputed. Fits; costs a chunked check (one branch per chunk, not per element).
3. Chunked sticky checks with early exit. Testing the flag every K elements bounds wasted work on large arrays. Fits; a branch per chunk is a small cost.
4. Remember promotions (V8 type feedback). If an operation on an array promoted last time, start the next one wide. Fits as a per-array or per-call-site hint; avoids repeated fail-and-redo.
5. Hardware sticky saturation (ARM FPSR.QC, RISC-V `vxsat`) as an accelerator. Free on two of three targets, but x86 has no 32/64-bit saturating add or sticky bit, so it can only be an optional backend, not the default.
6. Value-range statistics per array or per block (DuckDB). Tracking min/max per block rather than bit-width per array allows skipping checks block by block. Fits; adds metadata and a reduction.
7. Headroom or lazy-carry scheduling (NTT lazy reduction). Allow a known number of adds before any check, using spare bits. Partly fits: we have no spare bits in full-range int32 halves, so it only helps after demotion leaves margin.
8. Truncation analysis. If a result is only used mod 2^32 (or mod a small modulus), skip checks. Fits as an optimization for explicit modular ops.
9. Scoped checking modes (C# `checked`, Rust wrapping types, GNAT modes). An explicit wrapping Gaussian type for users who want arithmetic mod 2^32. Fits as an opt-in, not the default.
10. Promote to float (J, Dyalog, SQLite). Does not fit; loses exactness.
11. Null or sentinel on overflow (kdb+, R `NA`). Does not fit; silently loses values.
12. Trap (MIPS, Swift). Does not fit; not available on vector units and not chip-agnostic.
13. Residue (multimodular) arithmetic with CRT reconstruction, as in FLINT. Avoids overflow entirely for product-heavy pipelines but cannot compare or divide cheaply. Fits only as a special-purpose path.
