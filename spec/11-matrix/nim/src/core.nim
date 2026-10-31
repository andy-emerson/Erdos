## core.nim: what both models share (spec v0.4 §17): the build settings, the C interface the driver reads, manual
## allocation, the integer module (the gcd of §17.7, the checked arithmetic of 08), the counters of one arithmetic
## operation, and the mark of §17.10.
##
## Builds (as the C rounds): timing (nothing), counting (-d:skCount), test (-d:skCount -d:skAudit), memory (-d:m2Mem).
## Nothing here allocates through Nim's runtime: every block of a model is malloc'd, so the heap figure of the driver's
## wrapper sees exactly the model's blocks.

{.push raises: [].}

import gcd
export gcd

const
  counting* = defined(skCount)
  auditing* = defined(skAudit)
  memoryBuild* = defined(m2Mem)

when auditing and not counting:
  {.error: "the test build (skAudit) also needs skCount".}
when cpuEndian != littleEndian:
  {.error: "a cell's two fields are written in one little-endian write (§17.5)".}

# ----------------------------------------------------------------------------------------- the driver's C types

type
  SkStatus* {.size: sizeof(cint).} = enum
    skOk = 0, skEOverflow = 1, skEDivZero = 2, skEInval = 3, skEAudit = 4, skENotBuilt = 5
  Phase* = enum
    phEntry = 0, phAdd = 1, phSub = 2, phMul = 3

  # The structures the driver reads, taken from its headers so that their layout is C's. Only the fields the models
  # write are named here.
  SkCount* {.importc: "sk_count", header: "m2.h", bycopy.} = object
    gcd*, gcd_work*, div_exact*, mul*, add*: uint64
  SkCounters* {.importc: "sk_counters", header: "m2.h", bycopy.} = object
    c*: array[6, array[2, SkCount]]
  R2XCount* {.importc: "r2_xcount", header: "m2.h", bycopy.} = object
    cmp*: uint64
    cases*: array[31, uint64]
    reads*: uint64      ## K-Q3 reads: none in V, where every value is read for the cross tests (v0.1 §7.1)
  R2XCounters* {.importc: "r2_xcounters", header: "m2.h", bycopy.} = object
    c*: array[6, array[2, R2XCount]]
  SkMemItem* {.importc: "sk_mem_item", header: "m2.h", bycopy.} = object
    name*: cstring
    used*, allocated*: uint64
  SkMemory* {.importc: "sk_memory", header: "m2.h", bycopy.} = object
    n*: cuint
    item*: array[10, SkMemItem]
    used*, allocated*, values*: uint64
    used_per_rational*, allocated_per_rational*: cdouble

# r2.h's case numbers, as the counters and the per-operation record index them (static-asserted below)
type
  Case* = enum
    addZero = 0, addCross = 1, addDen = 2, addNum = 3, addGeneral = 4,
    mulOneX = 5, mulOneY = 6, mulBoth = 7, mulDen = 8, mulNum = 9, mulCrossAd = 10, mulCrossCb = 11,
    mulGeneral = 12,
    addIntBoth = 21, addIntX = 22, addIntY = 23, mulIntBoth = 24, mulIntX = 25, mulIntY = 26,
    addZeroX = 27, addZeroY = 28, mulZero = 29

{.emit: """/*INCLUDESECTION*/
#include "m2.h"
_Static_assert(R2_NCASE == 31 && R2_ADD_GEN == 4 && R2_MUL_GEN == 12 && R2_ADD_INT_BOTH == 21 && R2_MUL_INT_Y == 26 &&
               R2_ADD_ZERO_X == 27 && R2_ADD_ZERO_Y == 28 && R2_MUL_ZERO == 29, "r2.h's case numbers");
_Static_assert(SK_NPHASE == 6 && SK_NPART == 2 && SK_MEM_MAX == 10, "sk_common.h's sizes");
""".}

# the driver's globals (m1common.c, m2common.c): the per-operation record and the accounted running total
var
  percall {.importc: "m1_percall", header: "m1.h".}: ptr UncheckedArray[int16]
  percallN {.importc: "m1_percall_n", header: "m1.h".}: uint64
  percallCap {.importc: "m1_percall_cap", header: "m1.h".}: uint64
  acctLive {.importc: "m2_acct_live", header: "m2.h".}: uint64
  acctPeak {.importc: "m2_acct_peak", header: "m2.h".}: uint64

# ----------------------------------------------------------------------------------------- allocation

proc c_malloc(n: csize_t): pointer {.importc: "malloc", header: "<stdlib.h>".}
proc c_calloc(n, size: csize_t): pointer {.importc: "calloc", header: "<stdlib.h>".}
proc c_realloc(p: pointer, n: csize_t): pointer {.importc: "realloc", header: "<stdlib.h>".}
proc c_free(p: pointer) {.importc: "free", header: "<stdlib.h>".}
proc c_memset(p: pointer, c: cint, n: csize_t): pointer {.importc: "memset", header: "<string.h>", discardable.}
proc skDie*(msg: cstring) {.importc: "sk_die", header: "m2.h", noreturn.}

proc allocBytes*(n: int): pointer =
  result = c_malloc(csize_t(max(n, 1)))
  if result == nil: skDie("out of memory")

proc allocZeroed*(n: int): pointer =
  result = c_calloc(1, csize_t(max(n, 1)))
  if result == nil: skDie("out of memory")

proc resizeBytes*(p: pointer, n: int): pointer =
  result = c_realloc(p, csize_t(max(n, 1)))
  if result == nil: skDie("out of memory")

proc freeBytes*(p: pointer) = c_free(p)

proc fillOnes*(p: pointer, n: int) = c_memset(p, 0xFF, csize_t(n))   ## every bit 1, in one step

template account*(delta: int) =
  ## the memory build's accounted running total (v0.2 §15.6): every allocation, shrink and free of a structure
  when memoryBuild:
    acctLive = uint64(int64(acctLive) + int64(delta))
    if acctLive > acctPeak: acctPeak = acctLive

# ----------------------------------------------------------------------------------------- the integer module

const int64Min* = low(int64)

func builtinAddOverflow(a, b: int64, r: var int64): bool {.importc: "__builtin_add_overflow", nodecl.}
func builtinMulOverflow(a, b: int64, r: var int64): bool {.importc: "__builtin_mul_overflow", nodecl.}

when auditing:
  # test build: 08's gcd itself, sk_gcd_raw of 08's pinned lib/vendor08/sk_internal.h, to check the new one against
  # (§17.12); include/m4n_gcd08.h refuses the m4 header
  func gcd08*(p, q: int64): int64 {.importc: "sk_gcd_raw", header: "m4n_gcd08.h".}

func overflowsAdd*(p, q: int64, r: var int64): bool {.inline.} =
  ## 08's check: true when p + q is outside [-2^63 + 1, 2^63 - 1]
  builtinAddOverflow(p, q, r) or r == int64Min

func overflowsMul*(p, q: int64, r: var int64): bool {.inline.} =
  builtinMulOverflow(p, q, r) or r == int64Min

func sgn*(neg: bool, v: int64): int64 {.inline.} =
  ## v with a sign applied: a negation, never a multiplication (K-13)
  if neg: -v else: v

# ----------------------------------------------------------------------------------------- one operation's counts

type
  Tally* = object
    ## the counts of one arithmetic operation (counting builds): its case, its core (gcds, gcd work, exact
    ## divisions, multiplications, additions), its comparisons, and its cross tests (4 of them) with their matches
    when counting:
      kase*: Case
      gcds*, work*, divs*, muls*, adds*, cmps*, matches*: int

template tally*(t: var Tally, field: untyped, n: int) =
  when counting: t.field += n

template taken*(t: var Tally, k: Case) =
  when counting: t.kase = k

proc gcdOf*(t: var Tally, p, q: int64): int64 {.inline.} =
  ## a counted gcd: once, with its work, the bits of the larger magnitude (08 v0.1 §12.1). In the test build, every
  ## value is checked against 08's gcd on the same operands (§17.12).
  when counting:
    t.gcds += 1
    t.work += bitLen(max(mag(p), mag(q)))
  result = gcd(p, q)
  when auditing:
    if result != gcd08(p, q): skDie("the new gcd differs from 08's")

proc divExact*(t: var Tally, p, q: int64): int64 {.inline.} =
  ## p / q, where q divides p: a counted exact division
  tally(t, divs, 1)
  p div q

proc commit*(k: var SkCounters, x: var R2XCounters, phase: Phase, t: Tally, record: bool) =
  ## adds one operation's counts to its matrix's counters (the core part), and to the driver's per-operation record
  when counting:
    let p = ord(phase)
    k.c[p][0].gcd += uint64(t.gcds)
    k.c[p][0].gcd_work += uint64(t.work)
    k.c[p][0].div_exact += uint64(t.divs)
    k.c[p][0].mul += uint64(t.muls)
    k.c[p][0].add += uint64(t.adds)
    if record:
      x.c[p][0].cmp += uint64(t.cmps)
      x.c[p][0].cases[ord(t.kase)] += 1
      if percall != nil:
        if percallN >= percallCap: skDie("the per-operation record's buffer is full")
        let r = cast[ptr UncheckedArray[int16]](addr percall[6 * int(percallN)])
        r[0] = int16(ord(t.kase)); r[1] = int16(t.gcds); r[2] = int16(t.work)
        r[3] = int16(t.divs); r[4] = int16(t.muls); r[5] = int16(t.adds)
        percallN += 1

# ----------------------------------------------------------------------------------------- the mark (§17.10)

type
  Marks* {.importc: "m4n_marks", header: "m4n_marks.h", bycopy.} = object
    ## the counting build's overflow record of one operation, held by its result: cells marked "new" (their own
    ## arithmetic overflowed) and "passed on" (an operand was marked), and the mark tests made
    made*, passed*, tests*: uint64

template markTest*(m: untyped, d: int64): bool =
  ## the test for the mark: d = 0 on an operand cell, counted as a "mark test" in the counting build (`m`, the
  ## result's Marks, exists only there)
  when counting: m.tests += 1
  d == 0

{.pop.}
