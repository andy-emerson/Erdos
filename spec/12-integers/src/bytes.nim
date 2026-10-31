# bytes.nim: the Byte encoding build (spec §1.5). A value is its byte length L as an unsigned LEB128 varint, then L
# magnitude bytes, little-endian, in one malloc'd block. Every operation decodes both operands to Growing's words, runs
# Growing's routine and encodes the result: 2 decodes and 1 encode.
import bits, alloc, growing

type Val* = ptr UncheckedArray[uint8]
const wordBuild* = false
const WB = W div 8

proc release*(v: Val) {.inline.} = xfree(v)

proc decode(v: Val): Big {.inline.} =
  var L = 0
  var sh = 0
  var i = 0
  while true:
    let c = v[i]
    inc i
    L = L or (int(c and 0x7F) shl sh)
    sh += 7
    if (c and 0x80) == 0: break
  let n = (L + WB - 1) div WB
  result = cast[Big](xalloc((n + 1) * sizeof(Word)))
  result[0] = Word(n)
  for k in 1 .. n: result[k] = 0
  for k in 0 ..< L:
    result[1 + k div WB] = result[1 + k div WB] or (Word(v[i + k]) shl (8 * (k mod WB)))

proc encode(x: Big): Val {.inline.} =
  let n = x.len
  let L = (if n == 0: 0 else: (n - 1) * WB + (W - clz(x[n]) + 7) div 8)
  var vl = 1
  var t = L shr 7
  while t != 0:
    inc vl
    t = t shr 7
  result = cast[Val](xalloc(vl + L))
  var u = L
  for i in 0 ..< vl:
    result[i] = uint8(u and 0x7F) or (if i < vl - 1: 0x80'u8 else: 0'u8)
    u = u shr 7
  for k in 0 ..< L:
    result[vl + k] = uint8((x[1 + k div WB] shr (8 * (k mod WB))) and 0xFF)

const scap = 66      # round 2: operands decoded into stack buffers up to 65 words (the jobs need at most 32)

proc decodeInto(v: Val; sbuf: var array[scap, Word]): Big {.inline.} =
  var L = 0
  var sh = 0
  var i = 0
  while true:
    let c = v[i]
    inc i
    L = L or (int(c and 0x7F) shl sh)
    sh += 7
    if (c and 0x80) == 0: break
  let n = (L + WB - 1) div WB
  if n + 1 <= scap:
    result = cast[Big](addr sbuf[0])
    acctLocal((n + 1) * sizeof(Word))
  else:
    result = cast[Big](xalloc((n + 1) * sizeof(Word)))
  result[0] = Word(n)
  for k in 1 .. n: result[k] = 0
  for k in 0 ..< L:
    result[1 + k div WB] = result[1 + k div WB] or (Word(v[i + k]) shl (8 * (k mod WB)))

proc releaseDecoded(x: Big; sbuf: var array[scap, Word]) {.inline.} =
  if cast[pointer](x) == cast[pointer](addr sbuf[0]): acctLocalEnd((x.len + 1) * sizeof(Word))
  else: release(x)

template lift(name, routine: untyped) =
  proc name*(a, b: Val): Val =
    var sa {.noinit.}: array[scap, Word]
    var sb {.noinit.}: array[scap, Word]
    let x = decodeInto(a, sa)
    let y = decodeInto(b, sb)
    let z = routine(x, y)
    result = encode(z)
    releaseDecoded(x, sa)
    releaseDecoded(y, sb)
    release(z)

lift(opAdd, growing.opAdd)
lift(opSub, growing.opSub)
lift(opAbsdiff, growing.opAbsdiff)
lift(opAbsdiff2, growing.opAbsdiff2)
lift(opMul, growing.opMul)
lift(opDivexact, growing.opDivexact)
lift(opGcd, growing.opGcd)

proc fromWords*(w: openArray[uint64]): Val =
  let x = growing.fromWords(w)
  result = encode(x)
  release(x)

proc byteLen*(v: Val): (int, int) =
  ## (varint bytes, L)
  var L = 0
  var sh = 0
  var i = 0
  while true:
    let c = v[i]
    inc i
    L = L or (int(c and 0x7F) shl sh)
    sh += 7
    if (c and 0x80) == 0: break
  (i, L)
