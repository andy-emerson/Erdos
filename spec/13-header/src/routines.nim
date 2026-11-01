# routines.nim: spec 13 §1 Routines, shared by Control, A and B (rule 3). Growing's schoolbook routines (spec 12
# src/growing.nim), refactored once to write into a given destination: each reads its operands at their real length,
# writes at most its bound, and returns the result's length. W = 64 is fixed.
#
# Portable pseudocode (algorithm design rule 6): carries and borrows are values (s < x), never a flag; the only
# product is the 64 x 64 -> 128 one (x86-64 MUL, ARM64 MUL + UMULH, RISC-V MUL + MULHU); no divide instruction; no
# conditional move (build.sh also turns gcc's if-conversion off). ctz is gcc's builtin, as in Growing (README, Choices).
import alloc

type
  Word* = uint64
  Words* = ptr UncheckedArray[Word]

const W* = 64

proc ctzBuiltin(x: uint64): cint {.importc: "__builtin_ctzll", nodecl.}
template ctz*(x: Word): int = int(ctzBuiltin(x))

proc specError*(msg: cstring) {.noreturn, noinline.} =
  ## an error the spec names (sum across axes, compare across axes, class above 31, gcd(0, 0), a full pool)
  proc fputs(s: cstring; f: File): cint {.importc, header: "<stdio.h>".}
  discard fputs("spec error: ", stderr)
  discard fputs(msg, stderr)
  discard fputs("\n", stderr)
  quit(3)

proc imax*(a, b: int): int {.inline.} =
  ## the larger length, as a branch (gcc's MAX_EXPR would become a conditional move)
  if a >= b: result = a
  else: result = b

proc imin*(a, b: int): int {.inline.} =
  if a <= b: result = a
  else: result = b

template at*(x: Words; i: int): Words = cast[Words](addr x[i])

# the W x W -> 2W product: low word returned, high word in hi
proc mulw*(a, b: Word; hi: var Word): Word {.inline.} =
  var lo, h: Word
  {.emit: ["unsigned __int128 p_ = (unsigned __int128)", a, " * ", b, "; ", lo, " = (NU64)p_; ", h,
           " = (NU64)(p_ >> 64);"].}
  hi = h
  lo

proc norm*(x: Words; n: int): int {.inline.} =
  ## the real length: n less the zero words on top
  var n = n
  while n > 0 and x[n - 1] == 0: dec n
  n

# ------------------------------------------------------------------------------------------------ add
proc rAdd*(r, x: Words; nx: int; y: Words; ny: int): int =
  ## r = x + y: one loop over the shorter operand's words, then the longer's tail carries alone. Writes max + 1 words.
  var x = x
  var y = y
  var lx = nx
  var ly = ny
  if lx < ly:
    swap x, y
    swap lx, ly
  var c: Word = 0
  for i in 0 ..< ly:
    let s = x[i] + c
    let c1 = Word(s < c)
    let t = s + y[i]
    r[i] = t
    c = c1 + Word(t < s)
  for i in ly ..< lx:
    let t = x[i] + c
    r[i] = t
    c = Word(t < c)
  r[lx] = c
  norm(r, lx + 1)

# ------------------------------------------------------------------------------------------------ multiply-accumulate
proc rMac*(r: Words; nr: int; x: Words; nx: int; y: Words; ny: int): int =
  ## r += x * y. r holds nr words; words nr .. imax(nr, nx + ny) - 1 are set to 0 first, and a carry past them is
  ## written only when nonzero (§1: imax(n_r, n1 + n2), plus a carry word only when nonzero). Returns the length.
  let top = imax(nr, nx + ny)
  for k in nr ..< top: r[k] = 0
  var lim = top
  for i in 0 ..< nx:
    let xi = x[i]
    var c: Word = 0
    for j in 0 ..< ny:
      var hi: Word
      var lo = mulw(xi, y[j], hi)
      lo += c
      hi += Word(lo < c)
      let t = r[i + j]
      lo += t
      hi += Word(lo < t)
      r[i + j] = lo
      c = hi
    var k = i + ny
    while c != 0:
      if k == lim:
        r[k] = c                   # the carry word, only when nonzero
        inc lim
        c = 0
      else:
        let t = r[k] + c
        c = Word(t < c)
        r[k] = t
        inc k
  norm(r, lim)

# ------------------------------------------------------------------------------------------------ absolute difference
proc rSubInto*(r, x: Words; la: int; y: Words; lb: int): Word {.inline.} =
  ## r = x - y over imax(la, lb) words; returns the final borrow (1 when x < y). Common words, then the tail.
  var br: Word = 0
  let m = imin(la, lb)
  for i in 0 ..< m:
    let p = x[i]
    let q = y[i]
    let d = p - q
    r[i] = d - br
    br = Word(p < q) + Word(d < br)
  if la >= lb:
    for i in m ..< la:
      let p = x[i]
      r[i] = p - br
      br = Word(p < br)
  else:
    for i in m ..< lb:
      let q = y[i]
      let d = 0 - q
      r[i] = d - br
      br = Word(q != 0) + Word(d < br)
  br

proc rAbsdiff*(r, x: Words; nx: int; y: Words; ny: int; br: var Word): int =
  ## |x - y| over max words, and its borrow b (1 when x < y). The notes' tree: no borrow, the difference stands; a
  ## borrow, it is negated on the clock (NOT, then + 1, word by word). Returns the length.
  let n = imax(nx, ny)
  let b = rSubInto(r, x, nx, y, ny)
  br = b
  if b != 0:
    var c: Word = 1
    for i in 0 ..< n:
      let t = (not r[i]) + c
      c = Word(t < c)
      r[i] = t
  norm(r, n)

# ------------------------------------------------------------------------------------------------ compare
proc rCmp*(x: Words; nx: int; y: Words; ny: int): int =
  if nx != ny: return (if nx < ny: -1 else: 1)
  var i = nx - 1
  while i >= 0:
    if x[i] != y[i]: return (if x[i] < y[i]: -1 else: 1)
    dec i
  0

# ------------------------------------------------------------------------------------------------ shifts (local words)
proc ctzw(x: Words): int {.inline.} =
  ## trailing zero bits of a nonzero number
  var i = 0
  while x[i] == 0: inc i
  i * W + ctz(x[i])

proc shrw(x: Words; n: int; s: int): int {.inline.} =
  ## x >>= s in place over n words; returns the new length
  let q = s div W
  let b = s mod W
  let m = n - q
  if b == 0:
    for i in 0 ..< m: x[i] = x[i + q]
  else:
    for i in 0 ..< m:
      let hi = (if i + q + 1 < n: x[i + q + 1] shl (W - b) else: 0)
      x[i] = (x[i + q] shr b) or hi
  norm(x, imax(m, 0))

proc inverse(b: Word): Word {.inline.} =
  ## b^-1 mod 2^W for odd b, by Newton's iteration from b (correct to 3 bits)
  var x = b
  var bits = 3
  while bits < W:
    x = x * (2 - b * x)
    bits *= 2
  x

# ------------------------------------------------------------------------------------------------ exact division
proc rDivCore*(q, buf: Words; xo, nx0, yo, ny0, n1, n2: int): int =
  ## Hensel exact division on the working buffer (the dividend at xo, nx0 words; the divisor at yo, ny0 words, nonzero,
  ## dividing it): both shifted right by the divisor's trailing zero bits, then each quotient word from the lowest,
  ## q_i = x_i * b^-1 mod 2^W, and q_i * b subtracted. Writes exactly n1 - n2 + 1 words (§1: it stops there), reading
  ## dividend words at or past its current length as 0. Returns the length.
  let x = at(buf, xo)
  let y = at(buf, yo)
  let s = ctzw(y)
  let nx = shrw(x, nx0, s)
  let ny = shrw(y, ny0, s)
  let n = imax(n1 - n2 + 1, 0)
  let inv = inverse(y[0])
  for i in 0 ..< n:
    let xi = (if i < nx: x[i] else: 0)
    let qi = xi * inv
    q[i] = qi
    var c: Word = 0      # the product's carry
    var br: Word = 0     # the subtraction's borrow
    var j = 0
    while j < ny and i + j < nx:
      var hi: Word
      var lo = mulw(qi, y[j], hi)
      lo += c
      hi += Word(lo < c)
      let t = x[i + j]
      let d = t - lo
      x[i + j] = d - br
      br = Word(t < lo) + Word(d < br)
      c = hi
      inc j
    var k = i + j       # the carry and the borrow, on up
    while k < nx and (c or br) != 0:
      let t = x[k]
      let d = t - c
      x[k] = d - br
      br = Word(t < c) + Word(d < br)
      c = 0
      inc k
  norm(q, n)

# ------------------------------------------------------------------------------------------------ gcd
proc rGcdCore*(r, buf: Words; xo0, nx0, yo0, ny0: int): int =
  ## binary gcd over many words on the working buffer (Growing's general loop; its round-2 one-word special case is
  ## not built, rule 2). The result, y << shift, is written at its own length only. Returns the length.
  var x = at(buf, xo0)
  var y = at(buf, yo0)
  var nx = nx0
  var ny = ny0
  if nx == 0:
    for i in 0 ..< ny: r[i] = y[i]
    return ny
  if ny == 0:
    for i in 0 ..< nx: r[i] = x[i]
    return nx
  let shift = imin(ctzw(x), ctzw(y))
  ny = shrw(y, ny, ctzw(y))
  while nx != 0:
    nx = shrw(x, nx, ctzw(x))
    if rCmp(y, ny, x, nx) > 0:
      swap x, y
      swap nx, ny
    var br: Word = 0          # x -= y in place (x >= y)
    for i in 0 ..< nx:
      let p = x[i]
      let q = (if i < ny: y[i] else: 0)
      let d = p - q
      x[i] = d - br
      br = Word(p < q) + Word(d < br)
    nx = norm(x, nx)
  let q = shift div W          # y << shift
  let b = shift mod W
  for i in 0 ..< q: r[i] = 0
  if b == 0:
    for i in 0 ..< ny: r[q + i] = y[i]
    return q + ny
  var c: Word = 0
  for i in 0 ..< ny:
    let w = y[i]
    r[q + i] = (w shl b) or c
    c = w shr (W - b)
  if c != 0:
    r[q + ny] = c
    return q + ny + 1
  q + ny

# ------------------------------------------------------------------------------------------------ the working buffer
const scratchCap* = 128    # Growing's: on the stack up to 128 words (the jobs need at most 48), else on the heap

template withScratch*(nwords: int; buf, body: untyped) =
  ## a function's one local structure: nwords words, on the stack when they fit
  block:
    var sbuf {.noinit.}: array[scratchCap, Word]
    let nb = imax(nwords, 1)
    let onStack = nb <= scratchCap
    let buf = (if onStack: cast[Words](addr sbuf[0]) else: cast[Words](xalloc(nb * sizeof(Word))))
    if onStack: acctLocal(nb * sizeof(Word))
    body
    if onStack: acctLocalEnd(nb * sizeof(Word)) else: xfree(buf)
