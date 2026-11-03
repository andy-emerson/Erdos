## tcm.nim: Control (TC of the matrix round), ported from C to Nim for the Nim pilot.
##
## A line-for-line port of `spec/lib/src/tcm.c` (the matrix operations) on `tc3.c`'s arithmetic, with the m4 gcd of
## `spec/lib/vendor08/m4/sk_internal.h`, as the timing and memory builds compile them: no counters (SK_COUNT), audits
## (SK_AUDIT) or profile (SK_PROFILE). Every known-answer skip of C is kept, unconditionally, as C's KON() is 1 in those
## builds; each is marked [K-n] as in C. Same operations, same order, same special cases, same gcd.
##
## Each matrix is one array of r*c pairs (m, d) of 64-bit values in lowest terms, d >= 1, zero as (0, 1), row-major.
## Overflow is an error: every multiplication and addition is checked explicitly, as in C (`sk_add`, `sk_mul`), and an
## overflow returns skEOverflow with the step recorded. No Nim runtime is used: no ref, seq or string, no exceptions;
## memory is malloc'd and freed exactly as C does, so the heap accounting is identical.
##
## It exports C's ABI (`tcm_*`, tcm.h), so the unchanged C driver `spec/test/bench_m4.c` (IMPL_TC) links against it.

{.push raises: [].}

# ----------------------------------------------------------------------------------------- the C interface's types

type
  SkStatus {.size: sizeof(cint).} = enum
    skOk = 0, skEOverflow = 1, skEDivZero = 2, skEInval = 3, skEAudit = 4, skENotBuilt = 5
  SkPhase {.size: sizeof(cint).} = enum
    phEntry = 0, phAdd = 1, phSub = 2, phMul = 3, phDiv = 4, phRead = 5

  # the driver's structures, taken from its headers so that the layout is C's
  SkOverflow {.importc: "sk_overflow", header: "r3.h".} = object
    count: uint64
    op: SkPhase
    x, y: uint32
    step: cstring
  SkMemory {.importc: "sk_memory", header: "r3.h".} = object
  SkCounters {.importc: "sk_counters", header: "r3.h".} = object
  R2XCounters {.importc: "r2_xcounters", header: "r3.h".} = object

  SkOpctx = object      ## the operation in progress, for the overflow report (sk_opctx)
    op: SkPhase
    x, y: uint32

  Pair = object         ## a rational: numerator and denominator (tcm_pair)
    m, d: int64

  Tcm = object          ## a matrix (struct tcm_class)
    P: ptr UncheckedArray[Pair]   ## the cells: R x K pairs, row-major
    N, R, K: uint32               ## cells written so far, in the order of entry; rows; columns
    ctx: SkOpctx
    ovf: SkOverflow

const
  skNone = high(uint32)          # SK_NONE
  skLimit = high(uint32) - 1     # SK_LIMIT
  skIntMin = low(int64)          # SK_INT_MIN

# ----------------------------------------------------------------------------------------- C library and shared module

proc malloc(n: csize_t): pointer {.importc, header: "<stdlib.h>".}
proc calloc(n, size: csize_t): pointer {.importc, header: "<stdlib.h>".}
proc free(p: pointer) {.importc, header: "<stdlib.h>".}
proc memset(p: pointer, c: cint, n: csize_t): pointer {.importc, header: "<string.h>", discardable.}
proc skDie(msg: cstring) {.importc: "sk_die", cdecl, noreturn.}
proc skMemoryAdd(m: ptr SkMemory, name: cstring, used, allocated: uint64) {.importc: "sk_memory_add", cdecl.}
proc skMemoryFinish(m: ptr SkMemory, values: uint64) {.importc: "sk_memory_finish", cdecl.}

# select(c, a, b) is C's conditional expression `c ? a : b`: only the chosen operand is evaluated. Nim's own `if`
# expression is lowered to goto-structured statements, and gcc then no longer turns these selects into conditional
# moves (C's gcd loop is written for them). So it is spelled once in C, here; -d:nimIfSelect uses Nim's `if` instead.
when defined(nimIfSelect):
  template select(c: bool, a, b: untyped): untyped = (if c: a else: b)
else:
  {.emit: "/*INCLUDESECTION*/\n#define PILOT_SELECT(c, a, b) ((c) ? (a) : (b))".}
  func select(c: bool, a, b: int64): int64 {.importc: "PILOT_SELECT", nodecl.}
  func select(c: bool, a, b: uint64): uint64 {.importc: "PILOT_SELECT", nodecl.}
  func select(c: bool, a, b: csize_t): csize_t {.importc: "PILOT_SELECT", nodecl.}
  func select(c: bool, a, b: cint): cint {.importc: "PILOT_SELECT", nodecl.}

proc xmalloc(n: csize_t): pointer {.noinline.} =   # sk_xmalloc (in C, another translation unit)
  result = malloc(select(n != 0, n, 1))
  if result == nil: skDie("out of memory")

proc xcalloc(n, size: csize_t): pointer {.noinline.} =   # sk_xcalloc (in C, another translation unit)
  result = calloc(select(n != 0, n, 1), select(size != 0, size, 1))
  if result == nil: skDie("out of memory")

# ----------------------------------------------------------------------------------------- the integer module (m4)

# The inline hints follow C's: {.inline.} where C says `static inline`, none where it says `static`.

# The bit builtins and the overflow-reporting builtins C's sk_internal.h uses, called directly.
func ctz(x: uint64): cint {.importc: "__builtin_ctzll", nodecl.}
func clz(x: uint64): cint {.importc: "__builtin_clzll", nodecl.}
func builtinAddOverflow(a, b: int64, r: var int64): bool {.importc: "__builtin_add_overflow", nodecl.}
func builtinMulOverflow(a, b: int64, r: var int64): bool {.importc: "__builtin_mul_overflow", nodecl.}

func mag(a: int64): uint64 {.inline.} =                     # sk_mag: |a|
  select(a < 0, 0'u64 - cast[uint64](a), cast[uint64](a))

func bitLen(x: uint64): cint {.inline.} =                   # sk_bits_u_raw
  select(x != 0, 64 - clz(x), 0)

const gcdSpread = 8   # SK_GCD_SPREAD

func gcd(a, b: int64): int64 {.inline.} =
  ## sk_gcd_raw (m4): the binary method on magnitudes, gcd(a, 0) = |a|; when the bit lengths differ by more than
  ## gcdSpread, one remainder first, gcd(u, v) = gcd(u mod v, v).
  var u = mag(a)
  var v = mag(b)
  if u == 0: return cast[int64](v)
  if v == 0: return cast[int64](u)
  let lu = bitLen(u)
  let lv = bitLen(v)
  if lu - lv > gcdSpread:          # one remainder first, when the sizes differ a lot
    u = u mod v
    if u == 0: return cast[int64](v)
  elif lv - lu > gcdSpread:
    v = v mod u
    if v == 0: return cast[int64](u)
  let s = ctz(u or v)
  u = u shr ctz(u)
  v = v shr ctz(v)
  while u != v:                    # the binary method, written so the compiler emits conditional moves
    let z = ctz(u - v)             # the raw difference: ctz(u - v) = ctz(v - u)
    let m = select(u < v, u, v)
    let d = select(u < v, v - u, u - v)
    u = m
    v = d shr z
  cast[int64](u shl s)

# Checked arithmetic (sk_add, sk_mul): true on overflow, a result outside [-2^63 + 1, 2^63 - 1].
func overflowsAdd(a, b: int64, r: var int64): bool {.inline.} =
  builtinAddOverflow(a, b, r) or r == skIntMin
func overflowsMul(a, b: int64, r: var int64): bool {.inline.} =
  builtinMulOverflow(a, b, r) or r == skIntMin

func divexact(a, b: int64): int64 {.inline.} = a div b      # sk_divexact: b divides a

func sgn(neg: bool, v: int64): int64 {.inline.} =           # r2_sgn: v, or -v when neg (a negation, K-13)
  select(neg, -v, v)

func validInput(v: int64): bool {.inline.} = v != skIntMin  # sk_valid_input

proc overflowAt(c: ptr Tcm, step: cstring): SkStatus {.inline.} =     # sk_overflow_at
  inc c.ovf.count
  c.ovf.op = c.ctx.op
  c.ovf.x = c.ctx.x
  c.ovf.y = c.ctx.y
  c.ovf.step = step
  skEOverflow

# `LET r = x + y` and `LET r = x · y`, each checked: on overflow, the operation returns skEOverflow, naming the step.
template add(c: ptr Tcm, x, y: int64, step: cstring): int64 =
  var r {.noinit.}: int64
  if overflowsAdd(x, y, r): return overflowAt(c, step)
  r
template mul(c: ptr Tcm, x, y: int64, step: cstring): int64 =
  var r {.noinit.}: int64
  if overflowsMul(x, y, r): return overflowAt(c, step)
  r

# ----------------------------------------------------------------------------------------- store (§10.5)

func store(n, e: int64, dst: var Pair): SkStatus =
  ## Write n / e (lowest terms, e >= 1) where the caller names: a cell of the matrix, or a local pair (K-14).
  dst.m = n
  dst.d = e
  skOk

# ----------------------------------------------------------------------------------------- create, entry

proc create(R, K: uint32): ptr Tcm {.exportc: "tcm_create", cdecl.} =
  ## A matrix of R x K cells, allocated from its dimensions: R*K pairs.
  if R == 0 or K == 0 or uint64(R) * uint64(K) > uint64(skLimit div 2): skDie("tcm: bad dimensions")
  let c = cast[ptr Tcm](xcalloc(1, csize_t(sizeof(Tcm))))
  c.R = R
  c.K = K
  c.P = cast[ptr UncheckedArray[Pair]](xmalloc(csize_t(sizeof(Pair)) * csize_t(R) * csize_t(K)))
  c

proc destroy(c: ptr Tcm) {.exportc: "tcm_destroy", cdecl.} =
  if c == nil: return
  free(c.P)
  free(c)

proc enter(c: ptr Tcm, m, d: int64): SkStatus {.exportc: "tcm_enter", cdecl.} =
  ## Enter m / d as the next cell of the matrix, in the order of entry (row-major).
  if not validInput(m) or not validInput(d): return skEInval
  if d == 0: return skEDivZero
  if c.N >= c.R * c.K: return skEInval    # every cell is entered
  let dst = addr c.P[c.N]
  inc c.N
  c.ctx.op = phEntry
  c.ctx.x = skNone
  c.ctx.y = skNone
  var m = m
  var d = d
  if d < 0:                               # move the sign of d to m; negation is not counted
    m = -m
    d = -d
  let g = gcd(m, d)
  if g == 1:                              # [K-1]
    return store(m, d, dst[])
  let n = divexact(m, g)
  let e = divexact(d, g)
  store(n, e, dst[])

# ----------------------------------------------------------------------------------------- operations (§10)

type
  Opnd = object   ## an operand as the pseudocode names it: its magnitudes (a, b for x; c, d for y) and its sign
    n, e: int64   # |m| and d
    neg: bool

func operand(p: Pair): Opnd {.inline.} =
  Opnd(n: select(p.m < 0, -p.m, p.m), e: p.d, neg: p.m < 0)

proc begin(c: ptr Tcm, op: SkPhase, x, y: Pair): SkStatus {.inline.} =
  ## No handle to check: the matrix loops' bounds hold by construction, and they set the operands' cells in ctx.
  c.ctx.op = op
  skOk

type
  Eq = object
    den, num, ad, cb, ox, oy, ix, iy, zx, zy: bool

func compare(X, Y: Opnd, six: bool): Eq {.inline.} =
  ## The comparisons on magnitudes, all made, in Q32's order, before a case is chosen; then the two integer tests,
  ## b = 1 and d = 1; and the two zero tests, a = 0 and c = 0.
  result.den = X.e == Y.e
  result.num = X.n == Y.n
  result.ad = X.n == Y.e
  result.cb = Y.n == X.e
  result.ox = six and X.n == X.e
  result.oy = six and Y.n == Y.e
  result.ix = X.e == 1   # x is an integer
  result.iy = Y.e == 1   # y is an integer
  result.zx = X.n == 0   # x is zero
  result.zy = Y.n == 0   # y is zero

proc addsub(c: ptr Tcm, op: SkPhase, x, y: Pair, dst: var Pair): SkStatus =
  let st = begin(c, op, x, y)
  if st != skOk: return st
  let X = operand(x)
  var Y = operand(y)
  if op == phSub: Y.neg = not Y.neg
  let (a, b, cc, d) = (X.n, X.e, Y.n, Y.e)
  let q = compare(X, Y, false)
  if q.den and q.num and X.neg != Y.neg:   # case zero: x + (-x) = 0/1  [K-2]
    return store(0, 1, dst)
  if q.zx:                                 # case zero_x: x = 0, x + y = s_y*y: no arithmetic
    return store(sgn(Y.neg, cc), d, dst)
  if q.zy:                                 # case zero_y: y = 0, x + y = x: no arithmetic
    return store(sgn(X.neg, a), b, dst)
  if q.ix and q.iy:                        # case int_both: b = d = 1, (s_x*a + s_y*c)/1: no gcd, no multiplication
    let t = c.add(sgn(X.neg, a), sgn(Y.neg, cc), "add int_both: t = s_x*a + s_y*c")
    return store(t, 1, dst)
  if q.ix:                                 # case int_x: b = 1, (s_x*a*d + s_y*c)/d, in lowest terms: no gcd
    let p = c.mul(sgn(X.neg, a), d, "add int_x: s_x*a*d")
    let t = c.add(p, sgn(Y.neg, cc), "add int_x: s_x*a*d + s_y*c")
    return store(t, d, dst)
  if q.iy:                                 # case int_y: d = 1, (s_x*a + s_y*c*b)/b, in lowest terms: no gcd
    let r = c.mul(sgn(Y.neg, cc), b, "add int_y: s_y*c*b")
    let t = c.add(sgn(X.neg, a), r, "add int_y: s_x*a + s_y*c*b")
    return store(t, b, dst)
  if q.ad or q.cb:                         # case cross: gcd(b, d) = 1
    let p = c.mul(sgn(X.neg, a), d, "add cross: s_x*a*d")
    let r = c.mul(sgn(Y.neg, cc), b, "add cross: s_y*c*b")
    let t = c.add(p, r, "add cross: s_x*a*d + s_y*c*b")
    let e = c.mul(b, d, "add cross: b*d")
    return store(t, e, dst)
  if q.den:                                # case den: gcd(b, d) = d
    let t = c.add(sgn(X.neg, a), sgn(Y.neg, cc), "add den: t = s_x*a + s_y*c")
    let h = gcd(t, d)
    if h == 1:                             # [K-3]
      return store(t, d, dst)
    let n = divexact(t, h)
    let e = divexact(d, h)
    return store(n, e, dst)
  if q.num:                                # case num: gcd(a, b*d) = 1
    let g = gcd(b, d)
    if g == 1:                             # [K-4]
      let t = c.add(sgn(X.neg, d), sgn(Y.neg, b), "add num: t = s_x*d + s_y*b")
      let n = c.mul(a, t, "add num: a*t")
      let e = c.mul(b, d, "add num: b*d")
      return store(n, e, dst)
    let bp = divexact(b, g)
    let dp = divexact(d, g)
    let t = c.add(sgn(X.neg, dp), sgn(Y.neg, bp), "add num: t = s_x*d' + s_y*b'")
    let h = gcd(t, g)
    if h == 1:                             # [K-5]
      let n = c.mul(a, t, "add num: a*t")
      let e = c.mul(bp, d, "add num: b'*d")
      return store(n, e, dst)
    let tq = divexact(t, h)
    let dq = divexact(d, h)
    let n = c.mul(a, tq, "add num: a*(t/h)")
    let e = c.mul(bp, dq, "add num: b'*(d/h)")
    return store(n, e, dst)
  # case general
  let g = gcd(b, d)
  if g == 1:                               # [K-6] the direct form is in lowest terms
    let p = c.mul(sgn(X.neg, a), d, "add general: s_x*a*d")
    let r = c.mul(sgn(Y.neg, cc), b, "add general: s_y*c*b")
    let t = c.add(p, r, "add general: s_x*a*d + s_y*c*b")
    let e = c.mul(b, d, "add general: b*d")
    return store(t, e, dst)
  let bp = divexact(b, g)
  let dp = divexact(d, g)
  let p = c.mul(sgn(X.neg, a), dp, "add general: s_x*a*d'")
  let r = c.mul(sgn(Y.neg, cc), bp, "add general: s_y*c*b'")
  let t = c.add(p, r, "add general: t = s_x*a*d' + s_y*c*b'")
  let h = gcd(t, g)
  if h == 1:                               # [K-7]
    let e = c.mul(bp, d, "add general: b'*d")
    return store(t, e, dst)
  let n = divexact(t, h)
  let dq = divexact(d, h)
  let e = c.mul(bp, dq, "add general: b'*(d/h)")
  store(n, e, dst)

proc addPairs(c: ptr Tcm, x, y: Pair, dst: ptr Pair): SkStatus {.exportc: "tcm_add", cdecl.} =
  addsub(c, phAdd, x, y, dst[])
proc subPairs(c: ptr Tcm, x, y: Pair, dst: ptr Pair): SkStatus {.exportc: "tcm_sub", cdecl.} =
  addsub(c, phSub, x, y, dst[])

proc multiply(c: ptr Tcm, x, y: Pair, dst: var Pair): SkStatus =
  let st = begin(c, phMul, x, y)
  if st != skOk: return st
  let X = operand(x)
  let Y = operand(y)
  let (a, b, cc, d) = (X.n, X.e, Y.n, Y.e)
  let q = compare(X, Y, true)
  let s = X.neg != Y.neg                   # s = s_x * s_y
  if q.zx or q.zy:                         # case zero: a zero factor, x*y = 0/1: no arithmetic
    return store(0, 1, dst)
  if q.ox:                                 # case one_x: x = +-1, x*y = s_x*y  [K-15]
    return store(sgn(s, cc), d, dst)
  if q.oy:                                 # case one_y: y = +-1, x*y = s_y*x  [K-15]
    return store(sgn(s, a), b, dst)
  if q.ix and q.iy:                        # case int_both: b = d = 1, (a*c)/1: no gcd
    let n = c.mul(a, cc, "multiply int_both: a*c")
    return store(sgn(s, n), 1, dst)
  if q.ad and q.cb:                        # case both: a = d, c = b  [K-12]
    return store(sgn(s, 1), 1, dst)
  if q.den:                                # case den: no gcd
    let n = c.mul(a, cc, "multiply den: a*c")
    let e = c.mul(d, d, "multiply den: d*d")
    return store(sgn(s, n), e, dst)
  if q.num:                                # case num: no gcd
    let n = c.mul(a, a, "multiply num: a*a")
    let e = c.mul(b, d, "multiply num: b*d")
    return store(sgn(s, n), e, dst)
  if q.ad:                                 # case cross_ad: a cancels
    let h = gcd(cc, b)
    if h == 1:                             # [K-8]
      return store(sgn(s, cc), b, dst)
    let n = divexact(cc, h)
    let e = divexact(b, h)
    return store(sgn(s, n), e, dst)
  if q.cb:                                 # case cross_cb: b cancels
    let h = gcd(a, d)
    if h == 1:                             # [K-8]
      return store(sgn(s, a), d, dst)
    let n = divexact(a, h)
    let e = divexact(d, h)
    return store(sgn(s, n), e, dst)
  if q.ix:                                 # case int_x: b = 1, (a*c)/d: one cross gcd, gcd(a, d)
    let h = gcd(a, d)
    if h == 1:                             # [K-8]
      let n = c.mul(a, cc, "multiply int_x: a*c")
      return store(sgn(s, n), d, dst)
    let ap = divexact(a, h)
    let e = divexact(d, h)
    let n = c.mul(ap, cc, "multiply int_x: a'*c")
    return store(sgn(s, n), e, dst)
  if q.iy:                                 # case int_y: d = 1, (a*c)/b: one cross gcd, gcd(c, b)
    let h = gcd(cc, b)
    if h == 1:                             # [K-8]
      let n = c.mul(a, cc, "multiply int_y: a*c")
      return store(sgn(s, n), b, dst)
    let cp = divexact(cc, h)
    let e = divexact(b, h)
    let n = c.mul(a, cp, "multiply int_y: a*c'")
    return store(sgn(s, n), e, dst)
  # case general, on magnitudes with the sign s
  let g1 = gcd(a, d)
  let g2 = gcd(cc, b)
  var ap, dp, cp, bp: int64
  if g1 == 1:                              # [K-9]
    ap = a
    dp = d
  else:
    ap = divexact(a, g1)
    dp = divexact(d, g1)
  if g2 == 1:                              # [K-9]
    cp = cc
    bp = b
  else:
    cp = divexact(cc, g2)
    bp = divexact(b, g2)
  let n = c.mul(ap, cp, "multiply general: a'*c'")
  let e = c.mul(bp, dp, "multiply general: b'*d'")
  store(sgn(s, n), e, dst)

proc mulPairs(c: ptr Tcm, x, y: Pair, dst: ptr Pair): SkStatus {.exportc: "tcm_mul", cdecl.} =
  multiply(c, x, y, dst[])

proc divide(c: ptr Tcm, x, y: Pair, dst: ptr Pair): SkStatus {.exportc: "tcm_div", cdecl.} =
  let st = begin(c, phDiv, x, y)
  if st != skOk: return st
  let X = operand(x)
  let Y = operand(y)
  let (a, b, cc, d) = (X.n, X.e, Y.n, Y.e)
  if cc == 0: return skEDivZero            # m_y = 0, before the comparisons
  let q = compare(X, Y, true)
  let s = X.neg != Y.neg                   # s = s_x * s_y
  if q.zx:                                 # case zero: a zero dividend, y != 0, x/y = 0/1: no arithmetic
    return store(0, 1, dst[])
  if q.ox:                                 # case one_x: x = +-1, x/y = s*d/c  [K-15]
    return store(sgn(s, d), cc, dst[])
  if q.oy:                                 # case one_y: y = +-1, x/y = s_y*x  [K-15]
    return store(sgn(s, a), b, dst[])
  if q.num and q.den:                      # case both: x = +-y, y != 0  [K-12]
    return store(sgn(s, 1), 1, dst[])
  if q.ad:                                 # case cross_ad: a^2 / (b*c), no gcd
    let n = c.mul(a, a, "divide cross_ad: a*a")
    let e = c.mul(b, cc, "divide cross_ad: b*c")
    return store(sgn(s, n), e, dst[])
  if q.cb:                                 # case cross_cb: a*d / b^2, no gcd; b = c
    let n = c.mul(a, d, "divide cross_cb: a*d")
    let e = c.mul(cc, cc, "divide cross_cb: c*c")
    return store(sgn(s, n), e, dst[])
  if q.num:                                # case num: a cancels
    let h = gcd(d, b)
    if h == 1:                             # [K-10]
      return store(sgn(s, d), b, dst[])
    let n = divexact(d, h)
    let e = divexact(b, h)
    return store(sgn(s, n), e, dst[])
  if q.den:                                # case den: b cancels
    let h = gcd(a, cc)
    if h == 1:                             # [K-10]
      return store(sgn(s, a), cc, dst[])
    let n = divexact(a, h)
    let e = divexact(cc, h)
    return store(sgn(s, n), e, dst[])
  # case general
  let g1 = gcd(a, cc)
  let g2 = gcd(b, d)
  var ap, cp, bp, dp: int64
  if g1 == 1:                              # [K-11]
    ap = a
    cp = cc
  else:
    ap = divexact(a, g1)
    cp = divexact(cc, g1)
  if g2 == 1:                              # [K-11]
    bp = b
    dp = d
  else:
    bp = divexact(b, g2)
    dp = divexact(d, g2)
  let n = c.mul(ap, dp, "divide general: a'*d'")
  let e = c.mul(bp, cp, "divide general: b'*c'")
  store(sgn(s, n), e, dst[])

# ----------------------------------------------------------------------------------------- read, reports

proc read(c: ptr Tcm, x: uint32, m, d: ptr int64): SkStatus {.exportc: "tcm_read", cdecl.} =
  if x >= c.N: return skEInval
  let p = c.P[x]
  m[] = p.m
  d[] = p.d
  skOk

proc size(c: ptr Tcm): uint32 {.exportc: "tcm_size", cdecl.} = c.N

proc memory(c: ptr Tcm, r: ptr SkMemory) {.exportc: "tcm_memory", cdecl.} =
  ## 16 bytes per cell, every cell counted.
  memset(r, 0, csize_t(sizeof(SkMemory)))
  let b = uint64(c.R) * uint64(c.K) * uint64(sizeof(Pair))
  skMemoryAdd(r, "pairs", b, b)
  skMemoryFinish(r, uint64(c.R) * uint64(c.K))

proc counters(c: ptr Tcm, r: ptr SkCounters, x: ptr R2XCounters): SkStatus {.exportc: "tcm_counters", cdecl.} =
  ## The counting build is not ported (the pilot measures the timing and memory builds).
  if r != nil: memset(r, 0, csize_t(sizeof(SkCounters)))
  if x != nil: memset(x, 0, csize_t(sizeof(R2XCounters)))
  skENotBuilt

proc overflow(c: ptr Tcm, r: ptr SkOverflow): SkStatus {.exportc: "tcm_overflow", cdecl.} =
  r[] = c.ovf
  if c.ovf.count != 0: skOk else: skEInval

proc audit(c: ptr Tcm, why: ptr cstring): SkStatus {.exportc: "tcm_audit", cdecl.} =
  if why != nil: why[] = "audits not built"
  skENotBuilt

proc auditChanged(c: ptr Tcm, why: ptr cstring): SkStatus {.exportc: "tcm_audit_changed", cdecl.} =
  if why != nil: why[] = "audits not built"
  skENotBuilt

# ----------------------------------------------------------------------------------------- the matrix operations (§6)

proc madd(a, b, c: ptr Tcm): SkStatus {.exportc: "tcm_madd", cdecl.} =
  ## C = A + B, elementwise, in row order: one add per cell, its result written to the cell of C at once.
  let n = c.R * c.K
  if a.R != c.R or a.K != c.K or b.R != c.R or b.K != c.K or a.N != n or b.N != n or c.N != 0:
    return skEInval
  for i in 0'u32 ..< n:
    c.ctx.x = i          # the operands' cells, for the overflow report
    c.ctx.y = i
    let st = addsub(c, phAdd, a.P[i], b.P[i], c.P[i])
    if st != skOk: return st
    c.N = i + 1
  skOk

proc mmul(a, b, c: ptr Tcm): SkStatus {.exportc: "tcm_mmul", cdecl.} =
  ## C = A B: for each cell of C in row order, s = 0/1; for k = 1..n: p = A[i][k] B[k][j], s = s + p, on local
  ## pairs; then s is written to the cell. Square matrices.
  let n = c.R
  if c.K != n or a.R != n or a.K != n or b.R != n or b.K != n or a.N != n * n or b.N != n * n or c.N != 0:
    return skEInval
  for i in 0'u32 ..< n:
    for j in 0'u32 ..< n:
      var s = Pair(m: 0, d: 1)
      var p {.noinit.}: Pair
      for k in 0'u32 ..< n:
        c.ctx.x = i * n + k
        c.ctx.y = k * n + j
        var st = multiply(c, a.P[i * n + k], b.P[k * n + j], p)
        if st != skOk: return st
        c.ctx.x = skNone   # two local pairs
        c.ctx.y = skNone
        st = addsub(c, phAdd, s, p, s)
        if st != skOk: return st
      c.P[i * n + j] = s   # store s as C[i][j]: write the pair
      c.N = i * n + j + 1
  skOk

{.pop.}
