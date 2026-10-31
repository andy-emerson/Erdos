# Nim pilot: Control ported from C to Nim

**Nim pilot: Control ported from C to Nim** · Oct 30, 2026 · pilot; nothing timed, nothing committed. Instruction counts are callgrind Ir, one run at a time. Peaks are the driver's accounted and heap figures.

**Verdict: go, with conditions** (above).

### 1. What was built

**Files:**
- `tcm.nim`: a line-for-line port of `tcm.c` on `tc3.c`'s arithmetic, with the m4 gcd, as the timing and memory builds compile them.
  - It exports C's ABI (`tcm_create`, `enter`, `madd`, `mmul`, `add`, `sub`, `mul`, `div`, `read`, `memory`, `overflow`, …).
  - It is linked with the **unchanged** `bench_m4.c` (`IMPL_TC`) and the same shared objects, so the driver, its accounting and its heap wrapper are the same code for both models.
- `build.sh`: builds C with `m4_libconf.py`'s exact command lines.

**Port rules kept:**
- Same operations, order and special cases, with `[K-n]` marks as in C.
- No `std/rationals` and no `std/math`. The gcd is C's, ported: the m4 remainder-first case and the ctz of the raw difference.
- Overflow is explicit, as `sk_add`/`sk_mul` do it: `__builtin_*_overflow`, or a result equal to −2^63.
  - It is written as `c.add(x, y, "step")` and `c.mul(…)` templates that return `skEOverflow` with the step recorded.
  - Checked on crafted workloads: the same status, step, operation, cells and count as C, including a sum of exactly −2^63.
- Inline hints mirror C's: `{.inline.}` where C has `static inline`.

**Flags:**
- `--app:staticlib --noMain -d:danger --mm:arc -d:useMalloc --panics:on`.
- gcc at C's `-O2` through `--gcc.options.speed`. Nim always adds `-fno-strict-aliasing`.

**Memory: ARC, with manual malloc for the matrix.**
- The cells are `ptr UncheckedArray[Pair]` from `malloc`, and the 64-byte class struct comes from `calloc`, as in C.
- This is what keeps the heap figure (the sum of `malloc_usable_size` behind `--wrap=malloc`) identical.
- A `seq` would add a header to the block and, without `-d:useMalloc`, use Nim's allocator. (The `seq` variant was not measured.)
- ARC emits nothing, because nothing in the port is managed.

**Not ported:**
- The `SK_COUNT`/`SK_AUDIT` builds. `tcm_counters` and `tcm_audit` return `SK_ENOTBUILT`.
- Imported from C rather than ported: `sk_die`, `sk_memory_add` and `sk_memory_finish`, and the driver's struct types (via `r3.h`).

**Workloads:** m3 add and mul jobs 1–12, all 24. They match `gen.json`'s sha256, and `m3_gen.py` regenerates jobs 1 and 10 of each op byte for byte.

**Nim:** 2.2.13 nightly, git `af53f1f1545382155204d3800e9716976b588ca8`, tarball sha256 `e11da6625f161fc9cc5f6ad9b92bc2d0ed68720933809eb607986f2f1b041540`.

### 2. Results

**Values:** identical on 24 jobs (43,008 cells). C, every Nim build, and the exact reference match byte for byte.

**Instruction counts:** the ratio table above.

**Peaks: Nim/C = 1.000 everywhere.**

| n | build peak, accounted | build peak, heap | op peak, accounted | op peak, heap |
|---|---|---|---|---|
| 16 | 4,096 / 4,096 | 4,176 / 4,176 | 12,288 / 12,288 | 12,528 / 12,528 |
| 32 | 16,384 / 16,384 | 16,464 / 16,464 | 49,152 / 49,152 | 49,392 / 49,392 |
| 64 | 65,536 / 65,536 | 65,616 / 65,616 | 196,608 / 196,608 | 196,848 / 196,848 |

These are the same for add and mul and for every ρ. Per-job figures are in `results/mem.csv`.

**Per-job Ir:** the C op Ir at n = 16/32/64 is about 57k / 220k / 875k for add and 2.1M / 16.8M / 135M for mul. Nim's op ratio is 0.972–0.974 for add and 0.998–0.999 for mul at every n and ρ. The full table is in `results/ir.csv`.

**Method:**
- `bench time <wl> <ro> 1 1` under callgrind, with inclusive Ir of the API functions.
- build = (enter + ⅔ create) / 2; op = madd or mmul + ⅓ create. This is DECISIONS-m4's method.

### 3. Side by side: the gcd (C `sk_gcd_raw` → Nim `gcd`)

```
static inline sk_int sk_gcd_raw(sk_int a, sk_int b) {   func gcd(a, b: int64): int64 {.inline.} =
  sk_uint u = sk_mag(a), v = sk_mag(b);                    var u = mag(a); var v = mag(b)
  if (u == 0) return (sk_int)v;                            if u == 0: return cast[int64](v)
  if (v == 0) return (sk_int)u;                            if v == 0: return cast[int64](u)
  int lu = sk_bits_u_raw(u), lv = sk_bits_u_raw(v);        let lu = bitLen(u); let lv = bitLen(v)
  if (lu - lv > SK_GCD_SPREAD) {                           if lu - lv > gcdSpread:
    u %= v; if (u == 0) return (sk_int)v;                    u = u mod v; if u == 0: return cast[int64](v)
  } else if (lv - lu > SK_GCD_SPREAD) {                    elif lv - lu > gcdSpread:
    v %= u; if (v == 0) return (sk_int)u; }                  v = v mod u; if v == 0: return cast[int64](u)
  int s = sk_ctz_raw(u | v);                               let s = ctz(u or v)
  u >>= sk_ctz_raw(u); v >>= sk_ctz_raw(v);                u = u shr ctz(u); v = v shr ctz(v)
  while (u != v) {                                         while u != v:
    int z = sk_ctz_raw(u - v);                               let z = ctz(u - v)
    sk_uint m = u < v ? u : v;                               let m = select(u < v, u, v)
    sk_uint d = u < v ? v - u : u - v;                       let d = select(u < v, v - u, u - v)
    u = m; v = d >> z; }                                     u = m; v = d shr z
  return (sk_int)(u << s); }                               cast[int64](u shl s)
```

The arithmetic reads close to paper §2.9/§2.10. For example, add's general case is `let g = gcd(b, d)` … `let t = c.add(p, r, …)`, `let h = gcd(t, g)`, `if h == 1: … return store(t, e, dst)`.

The port follows C's case order. C's order differs from the paper's pseudocode in two places, with the same values:
- **add:** C tests x + (−x) first.
- **mul:** C tests the cross matches before integer x and integer y.

### 4. What the Nim runtime adds per operation (from the generated C and its object code)

- **Allocation:** none. The op path calls only `gcd`, `multiply` and `addsub`.
- **Reference counting / destructor hooks:** none. Only plain objects and `ptr` are used.
- **Exceptions:** by default, a thread-local error-flag test after 40 calls, costing +1.1% (add op) and +3.5% (mul op). `--panics:on` removes them all; `raises: []` alone does not.
- **Checks and stack frames:** none under `-d:danger`.
- **Zero-initialisation** of `result` and locals: emitted in the C source, then removed by gcc. `{.noinit.}` is used on the hot temporaries.
- **`shr` masking `& 63`:** free on x86-64.
- **Startup:** `NimMain` is never called and `.init_array` is unchanged. The system module is linked (+11 KB) but nothing in it runs.

### 5. Code-shape effects and remedies

1. **`if` expressions lose C's `cmov`.**
   - Nim lowers them to `if (!c) goto L; r = a; goto E; L: r = b;`. With that shape gcc doesn't use conditional moves, and the gcd loop becomes a branch. This was reproduced in plain C.
   - `tcm.c` compiles to 70 `cmov`s; the pure-Nim build has 31.
   - The result is −6–7% Ir that measures a different, branchy loop.
   - **Remedy:** `select` = `#define PILOT_SELECT(c,a,b) ((c)?(a):(b))`, emitted once and used only where C has `?:`. It is lazy like `?:` (`clz(0)` is undefined), and it gives C's exact 21-instruction loop with both `cmov`s.
   - A pure-Nim mask select also gives `cmov`, but takes 27 instructions.
   - `-d:nimIfSelect` builds the pure-Nim form.
2. **Static branch prediction.**
   - Nim's `goto BeforeRet_` returns make gcc rate the gcd calls in `addsub`/`multiply` "cold", so it keeps one out-of-line gcd. C's gcd is inlined at 14 of 16 sites.
   - Net: add op −2.7%, `madd` loop about +3 Ir per cell, mul op −0.1%, build −0.7%.
   - Remedies (not tried): force inlining, which C doesn't do either; or run every model in Nim.
3. **Linkage.** Non-exported Nim procs are `hidden`, not `static`, so dead out-of-line copies remain (e.g. `store`). They cost no Ir but steer gcc's heuristics.

### 6. For the owner

The seven items under "Needs the owner" above.
