# growing.nim: the Growing build (spec §1.4). Schoolbook words, lowest first, no leading zero words (zero has none).
# Each value is one malloc'd block, a header word (the length) then the words, and is immutable. The word size W is a
# parameter (-d:W=64, the default, or -d:W=32). No special cases.
import bits, alloc

const W* {.intdefine.} = 64
when W == 64:
  type Word* = uint64
elif W == 32:
  type Word* = uint32
else:
  {.error: "W must be 64 or 32".}

type
  Big* = ptr UncheckedArray[Word]      # [0] the length n, [1..n] the words
  Words = ptr UncheckedArray[Word]     # a local buffer's words

const wordBuild* = false
type Val* = Big

template len*(x: Big): int = int(x[0])
template wd*(x: Big): Words = cast[Words](addr x[1])

proc newBig(n: int): Big {.inline.} =
  result = cast[Big](xalloc((n + 1) * sizeof(Word)))
  result[0] = Word(n)

proc norm(x: Big) {.inline.} =
  var n = x.len
  while n > 0 and x[n] == 0: dec n
  x[0] = Word(n)

proc release*(x: Big) {.inline.} = xfree(x)

# the W x W -> 2W product: gcc's unsigned __int128 at W = 64, uint64 at W = 32. Returns the low word, hi the high.
when W == 64:
  proc mulw(a, b: Word; hi: var Word): Word {.inline.} =
    var lo, h: Word
    {.emit: ["unsigned __int128 p_ = (unsigned __int128)", a, " * ", b, "; ", lo, " = (NU64)p_; ", h,
             " = (NU64)(p_ >> 64);"].}
    hi = h
    lo
else:
  proc mulw(a, b: Word; hi: var Word): Word {.inline.} =
    let p = uint64(a) * uint64(b)
    hi = Word(p shr 32)
    Word(p)

# ------------------------------------------------------------------------------------------------ add, sub, compare
proc opAdd*(a, b: Big): Big =
  ## round 2: one loop over the shorter operand's words, then the longer's tail carries alone (no per-word end test)
  var x = a.wd
  var y = b.wd
  var lx = a.len
  var ly = b.len
  if lx < ly:
    swap x, y
    swap lx, ly
  result = newBig(lx + 1)
  let r = result.wd
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
  norm(result)

proc subInto(r, x: Words; la: int; y: Words; lb: int): Word {.inline.} =
  ## r = x - y over max(la, lb) words; returns the final borrow (1 when x < y). Round 2: common words, then the tail.
  var br: Word = 0
  let m = min(la, lb)
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

proc opSub*(a, b: Big): Big =
  ## a - b over max(la, lb) words; when a < b it wraps (the borrow out is dropped).
  let n = max(a.len, b.len)
  result = newBig(n)
  discard subInto(result.wd, a.wd, a.len, b.wd, b.len)
  norm(result)

proc opAbsdiff2*(a, b: Big): Big =
  ## |a - b| with no comparison: subtract, and only if the final borrow is set, negate the result in a second pass
  ## ((r XOR m) + 1 word by word, m all ones).
  let n = max(a.len, b.len)
  result = newBig(n)
  let r = result.wd
  let br = subInto(r, a.wd, a.len, b.wd, b.len)
  let m = 0 - br
  var c = br
  for i in 0 ..< n:
    let t = (r[i] xor m) + c
    c = Word(t < c)
    r[i] = t
  norm(result)

proc cmpw(x: Words; nx: int; y: Words; ny: int): int {.inline.} =
  if nx != ny: return (if nx < ny: -1 else: 1)
  var i = nx - 1
  while i >= 0:
    if x[i] != y[i]: return (if x[i] < y[i]: -1 else: 1)
    dec i
  0

proc cmp*(a, b: Big): int = cmpw(a.wd, a.len, b.wd, b.len)

proc opAbsdiff*(a, b: Big): Big =
  if cmp(a, b) >= 0: opSub(a, b) else: opSub(b, a)

# ------------------------------------------------------------------------------------------------ multiply
proc opMul*(a, b: Big): Big =
  let la = a.len
  let lb = b.len
  result = newBig(la + lb)
  let x = a.wd
  let y = b.wd
  let r = result.wd
  for i in 0 ..< la + lb: r[i] = 0
  for i in 0 ..< la:
    var c: Word = 0
    for j in 0 ..< lb:
      var hi: Word
      var lo = mulw(x[i], y[j], hi)
      lo += c
      hi += Word(lo < c)
      lo += r[i + j]
      hi += Word(lo < r[i + j])
      r[i + j] = lo
      c = hi
    r[i + lb] = c
  norm(result)

# ------------------------------------------------------------------------------------------------ shifts (local words)
proc ctzw(x: Words): int {.inline.} =
  ## trailing zero bits of a nonzero number
  var i = 0
  while x[i] == 0: inc i
  i * W + ctz(x[i])

proc shrw(x: Words; n: var int; s: int) {.inline.} =
  ## x >>= s in place; n is the length, normalized after
  let q = s div W
  let b = s mod W
  let m = n - q
  if b == 0:
    for i in 0 ..< m: x[i] = x[i + q]
  else:
    for i in 0 ..< m:
      let hi = (if i + q + 1 < n: x[i + q + 1] shl (W - b) else: 0)
      x[i] = (x[i + q] shr b) or hi
  n = max(m, 0)
  while n > 0 and x[n - 1] == 0: dec n

proc shlInto(x: Words; n: int; s: int): Big =
  ## a new value x << s
  let q = s div W
  let b = s mod W
  result = newBig(n + q + 1)
  let r = result.wd
  for i in 0 ..< q: r[i] = 0
  r[n + q] = 0
  if b == 0:
    for i in 0 ..< n: r[i + q] = x[i]
  else:
    var c: Word = 0
    for i in 0 ..< n:
      r[i + q] = (x[i] shl b) or c
      c = x[i] shr (W - b)
    r[n + q] = c
  norm(result)

proc copyBig(x: Words; n: int): Big =
  result = newBig(n)
  let r = result.wd
  for i in 0 ..< n: r[i] = x[i]

# ------------------------------------------------------------------------------------------------ exact division
proc inverse(b: Word): Word {.inline.} =
  ## b^-1 mod 2^W for odd b, by Newton's iteration from b (correct to 3 bits)
  var x = b
  var bits = 3
  while bits < W:
    x = x * (2 - b * x)
    bits *= 2
  x

proc opDivexact*(a, b: Big): Big =
  ## Hensel exact division of a by b (b divides a): both shifted right by b's trailing zero bits, then each quotient
  ## word from the lowest, q_i = r_i * b^-1 mod 2^W, and q_i * b subtracted. The local structure: one buffer, a then b.
  let la = a.len
  let lb = b.len
  const cap = 128                     # round 2: scratch on the stack up to 128 words (the jobs need at most 32)
  var sbuf {.noinit.}: array[cap, Word]
  let nb = max(la + lb, 1)
  let onStack = nb <= cap
  let buf = (if onStack: cast[Words](addr sbuf[0]) else: cast[Words](xalloc(nb * sizeof(Word))))
  if onStack: acctLocal(nb * sizeof(Word))
  let x = buf
  let y = cast[Words](addr buf[la])
  for i in 0 ..< la: x[i] = a.wd[i]
  for i in 0 ..< lb: y[i] = b.wd[i]
  let s = ctzw(y)
  var nx = la
  var ny = lb
  shrw(x, nx, s)
  shrw(y, ny, s)
  let n = max(nx - ny + 1, 0)
  result = newBig(n)
  let q = result.wd
  let inv = inverse(y[0])
  for i in 0 ..< n:
    let qi = x[i] * inv
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
    var k = i + j        # the carry and the borrow, on up
    while k < nx and (c or br) != 0:
      let t = x[k]
      let d = t - c
      x[k] = d - br
      br = Word(t < c) + Word(d < br)
      c = 0
      inc k
  norm(result)
  if onStack: acctLocalEnd(nb * sizeof(Word)) else: xfree(buf)

# ------------------------------------------------------------------------------------------------ gcd
proc opGcd*(a, b: Big): Big =
  ## binary gcd over many words, in the form of std/math.gcd. The local structure: one buffer, a then b.
  let la = a.len
  let lb = b.len
  if la == 0: return copyBig(b.wd, lb)
  if lb == 0: return copyBig(a.wd, la)
  const cap = 128
  var sbuf {.noinit.}: array[cap, Word]
  let nb = la + lb
  let onStack = nb <= cap
  let buf = (if onStack: cast[Words](addr sbuf[0]) else: cast[Words](xalloc(nb * sizeof(Word))))
  if onStack: acctLocal(nb * sizeof(Word))
  var x = buf
  var y = cast[Words](addr buf[la])
  for i in 0 ..< la: x[i] = a.wd[i]
  for i in 0 ..< lb: y[i] = b.wd[i]
  var nx = la
  var ny = lb
  let shift = min(ctzw(x), ctzw(y))
  shrw(y, ny, ctzw(y))
  var fin = false
  var g: Word = 0
  while nx != 0:
    shrw(x, nx, ctzw(x))
    if nx <= 1 and ny <= 1:
      # round 2, named special case: both fit one word, so the word gcd finishes it (both odd here)
      var u = x[0]
      var w = y[0]
      while u != w:
        if u < w: swap u, w
        u -= w
        u = u shr ctz(u)
      g = u
      fin = true
      break
    if cmpw(y, ny, x, nx) > 0:
      swap x, y
      swap nx, ny
    # x -= y in place (x >= y)
    var br: Word = 0
    for i in 0 ..< nx:
      let p = x[i]
      let q = (if i < ny: y[i] else: 0)
      let d = p - q
      x[i] = d - br
      br = Word(p < q) + Word(d < br)
    while nx > 0 and x[nx - 1] == 0: dec nx
  if fin:
    var one {.noinit.}: array[1, Word]
    one[0] = g
    result = shlInto(cast[Words](addr one[0]), 1, shift)
  else:
    result = shlInto(y, ny, shift)
  if onStack: acctLocalEnd(nb * sizeof(Word)) else: xfree(buf)

# ------------------------------------------------------------------------------------------------ conversion (outside the clock)
proc fromWords*(w: openArray[uint64]): Big =
  when W == 64:
    result = newBig(w.len)
    for i in 0 ..< w.len: result.wd[i] = w[i]
  else:
    result = newBig(2 * w.len)
    for i in 0 ..< w.len:
      result.wd[2 * i] = Word(w[i] and 0xFFFF_FFFF'u64)
      result.wd[2 * i + 1] = Word(w[i] shr 32)
  norm(result)
