## packed.nim: Packed (spec v0.4 §17, mh4 as specified): each stored matrix is its cells, two handle fields per cell
## (2k + t, the tag t at the bottom), and (l), the packed list of its distinct whole numbers; the handle k names
## entry k of (l). While a matrix is built, (h), the index table, maps whole numbers to positions in (l) (§17.3); at
## the end of the build, the pack frees (h), narrows (l) to the narrowest width that holds its widest value and the
## fields to the narrowest handle width (§17.3-17.4). mul decodes each row of its left operand once, into a row buffer
## local to one function (§17.6). The arithmetic is arith.nim's, shared with Control; an overflow marks the cell 0/0
## and the job goes on (§17.10).
##
## Structures (§17.8): stored (l); temporary (h), from a build's create to its pack; local, mul's row buffer.
## Exported as npk_* (include/m4n.h) for the driver.

{.push raises: [].}

import core, arith
when auditing:
  import std/algorithm

# ----------------------------------------------------------------------------------------- the driver's figures

type
  M2Fig {.importc: "m2_fig", header: "m2.h", bycopy.} = object
    R, K, T, W, nf: uint32
    w, wh, wh_build: cuint
    packed: cint
    bytes_cells, bytes_vals: uint64
  M2SCount {.importc: "m2_scount", header: "m2.h", bycopy.} = object
    found, added, probes, longest: uint64
  M2OCount {.importc: "m2_ocount", header: "m2.h", bycopy.} = object
    cross, cross_match, cross_reads: uint64
  M2Pack {.importc: "m2_pack", header: "m2.h", bycopy.} = object
    id: cint
    x: pointer
    W, T: uint32
    w, wh_build, wh_pack: cuint
    rewritten, bytes_list, bytes_map, bytes_table, bytes_cells_build, bytes_cells_pack: uint64

var packHook {.importc: "m2_pack_hook", header: "m2.h".}: proc (p: ptr M2Pack) {.cdecl, raises: [].}

# ----------------------------------------------------------------------------------------- the matrix

type
  Bytes = ptr UncheckedArray[byte]
  Packed = object
    cells: Bytes        ## the matrix itself: 2 rows cols handle fields of `wh` bits
    list: Bytes         ## (l): W whole numbers of `w` bits, in arrival order (2 rows cols entries of 64 during a build)
    index: Bytes        ## (h), during a build only: T slots of `whBuild` bits, each a position in (l) or empty
    rows, cols, id: int
    T, tbits: int       ## the index table's size, T_fix = 2^tbits >= 2 rows cols (§17.2)
    whBuild, wh: int    ## the handle width during the build, and now
    w: int              ## (l)'s width: 64 during the build, then narrowed
    W: int              ## whole numbers in (l)
    widest: int         ## the widest value stored so far, in bits with the sign bit (§17.4)
    nf: int             ## handle fields written
    packed: bool
    rowBytes: int       ## a result of mul: the bytes of the row buffer its build used (§17.11); else 0
    when counting:
      cnt: SkCounters
      xcnt: R2XCounters
      stores: M2SCount
      cross: M2OCount
      marks: Marks

  Decoded = object
    ## one cell as an operation reads it: the numerator's magnitude, the denominator, the sign, and whether the two
    ## fields name the same handle (one read, not two, for the read counters)
    a, b: int64
    neg, same: bool

const phi = 0x9E3779B97F4A7C15'u64       ## the hash's multiplier (v0.1 §5.3)

func size(x: Packed): int = x.rows * x.cols
func bound(x: Packed): int = 2 * x.size  ## the most whole numbers a matrix's cells name (§17.2)

# The widths. A field or slot is read zero-extended, a list entry sign-extended; every read dispatches on the width.
func fieldAt(p: Bytes, width, i: int): int {.inline.} =
  case width
  of 8: int(cast[ptr UncheckedArray[uint8]](p)[i])
  of 16: int(cast[ptr UncheckedArray[uint16]](p)[i])
  else: int(cast[ptr UncheckedArray[uint32]](p)[i])

proc setField(p: Bytes, width, i, v: int) {.inline.} =
  case width
  of 8: cast[ptr UncheckedArray[uint8]](p)[i] = uint8(v)
  of 16: cast[ptr UncheckedArray[uint16]](p)[i] = uint16(v)
  else: cast[ptr UncheckedArray[uint32]](p)[i] = uint32(v)

func valueAt(p: Bytes, w, k: int): int64 {.inline.} =
  case w
  of 8: int64(cast[ptr UncheckedArray[int8]](p)[k])
  of 16: int64(cast[ptr UncheckedArray[int16]](p)[k])
  of 32: int64(cast[ptr UncheckedArray[int32]](p)[k])
  else: cast[ptr UncheckedArray[int64]](p)[k]

proc setValue(p: Bytes, w, k: int, v: int64) {.inline.} =
  case w
  of 8: cast[ptr UncheckedArray[int8]](p)[k] = int8(v)
  of 16: cast[ptr UncheckedArray[int16]](p)[k] = int16(v)
  of 32: cast[ptr UncheckedArray[int32]](p)[k] = int32(v)
  else: cast[ptr UncheckedArray[int64]](p)[k] = v

func buildWidth(tbits: int): int =
  ## v0.1 §5.2's fixed rule: the smallest of 8, 16, 32 with wh - 1 >= log2 T
  if tbits <= 7: 8 elif tbits <= 15: 16 else: 32

func packWidth(W: int): int =
  ## v0.2 §15.4 item 3.4: the smallest of 8, 16, 32 with 2^(wh - 1) >= W
  if W <= 128: 8 elif W <= 32768: 16 else: 32

func listWidth(bits: int): int =
  ## §17.4: the smallest of 8, 16, 32, 64 that holds a value of `bits` bits, the sign bit included
  if bits <= 8: 8 elif bits <= 16: 16 elif bits <= 32: 32 else: 64

func cellBytes(x: Packed): int = 2 * x.size * x.wh div 8
func listBytes(x: Packed): int = (if x.packed: x.W * x.w div 8 else: 8 * x.bound)
func tableBytes(x: Packed): int = x.T * x.whBuild div 8
func indexBytes(x: Packed): int = (if x.index == nil: 0 else: x.tableBytes)   ## (h)'s bytes while it exists

func value(x: Packed, k: int): int64 {.inline.} = valueAt(x.list, x.w, k)

func denominator(x: Packed, i: int): Decoded {.inline.} =
  ## cell i, read by handle as far as its denominator: its fields, its sign, whether both fields name one handle, and
  ## b. The numerator is read by `numerator`, only once the cell has passed the mark test (§17.10 item 2).
  let fn = fieldAt(x.cells, x.wh, 2 * i)
  let fd = fieldAt(x.cells, x.wh, 2 * i + 1)
  Decoded(b: x.value(fd shr 1), neg: (fn and 1) == 1, same: fn shr 1 == fd shr 1)

proc numerator(x: Packed, i: int, c: var Decoded) {.inline.} =
  ## the numerator of cell i: the denominator's value again if both fields name one handle, else read by handle
  c.a = if c.same: c.b else: x.value(fieldAt(x.cells, x.wh, 2 * i) shr 1)

func decode(x: Packed, i: int): Decoded {.inline.} =
  ## cell i, read whole by handle
  result = x.denominator(i)
  x.numerator(i, result)

func operand(c: Decoded): Operand {.inline.} = Operand(a: c.a, b: c.b, neg: c.neg)

func reads(c: Decoded): int {.inline.} =
  ## the whole numbers a by-handle read of the cell makes: one per distinct handle
  if c.same: 1 else: 2

# ----------------------------------------------------------------------------------------- counting

template counted(x: var Packed, phase: Phase, t: Tally, record = true) =
  when counting: commit(x.cnt, x.xcnt, phase, t, record)

template crossTested(x: var Packed, t: Tally, nreads: int) =
  ## an operation on two cells: its four cross tests, their matches, and the cross reads made for them (V)
  when counting:
    x.cross.cross += 4
    x.cross.cross_match += uint64(t.matches)
    x.cross.cross_reads += uint64(nreads)

template marked(x: var Packed, field: untyped) =
  when counting: x.marks.field += 1

# ----------------------------------------------------------------------------------------- building

proc store(x: var Packed, v: int64): int =
  ## §17.3 item 3: the handle of the whole number v, appended to (l) if new
  let empty = (1 shl x.whBuild) - 1
  var s = int((cast[uint64](v) * phi) shr (64 - x.tbits))
  var probes = 1
  while true:
    let k = fieldAt(x.index, x.whBuild, s)
    if k == empty:                                      # new: append v, write its position into the slot
      result = x.W
      setValue(x.list, 64, x.W, v)
      setField(x.index, x.whBuild, s, x.W)
      x.W += 1
      x.widest = max(x.widest, bitLen(uint64(v)) + 1)
      when counting: x.stores.added += 1
      break
    if valueAt(x.list, 64, k) == v:                     # found
      result = k
      when counting: x.stores.found += 1
      break
    if probes == x.T: skDie("the index table is full")  # cannot occur (§17.2)
    s = (s + 1) and (x.T - 1)
    probes += 1
  when counting:
    x.stores.probes += uint64(probes)
    x.stores.longest = max(x.stores.longest, uint64(probes))

proc storeCell(x: var Packed, i: int, r: Ratio) =
  ## §17.5: both whole numbers stored, the numerator first, then the cell's two fields written in one write
  let kn = x.store(int64(mag(r.n)))
  let kd = x.store(r.e)
  let fn = uint64(2 * kn + ord(r.n < 0))
  let fd = uint64(2 * kd)
  case x.whBuild
  of 8: cast[ptr UncheckedArray[uint16]](x.cells)[i] = uint16(fn or (fd shl 8))
  of 16: cast[ptr UncheckedArray[uint32]](x.cells)[i] = uint32(fn or (fd shl 16))
  else: cast[ptr UncheckedArray[uint64]](x.cells)[i] = fn or (fd shl 32)
  x.nf += 2

when auditing:
  proc cellValues(x: Packed): seq[(int64, int64)] =
    ## an audit copy: every cell's value (numerator, denominator)
    for i in 0 ..< x.size:
      let c = x.decode(i)
      result.add((sgn(c.neg, c.a), c.b))

  proc auditIndex(x: Packed): cstring =
    ## at the pack, before (h) is freed: every position is found at its own slot by its value's probe sequence,
    ## W slots are used, and the others hold the empty mark (v0.2 §15.7 item 5)
    let empty = (1 shl x.whBuild) - 1
    var used = 0
    for s in 0 ..< x.T:
      let k = fieldAt(x.index, x.whBuild, s)
      if k != empty:
        if k >= x.W: return "a slot of (h) holds a position past W"
        used += 1
    if used != x.W: return "(h)'s used slots are not W"
    for k in 0 ..< x.W:
      let v = valueAt(x.list, 64, k)
      var s = int((cast[uint64](v) * phi) shr (64 - x.tbits))
      while fieldAt(x.index, x.whBuild, s) != k:
        if fieldAt(x.index, x.whBuild, s) == empty: return "a position is not found by its value's probe sequence"
        s = (s + 1) and (x.T - 1)
    nil

proc pack(x: var Packed) =
  ## §17.3, at the end of the build: free (h); narrow (l) in place and shrink it; narrow the fields if the packed
  ## handle width is smaller. No map, no scan of (h), no rewrite of a handle.
  let w = listWidth(x.widest)
  let wh = packWidth(x.W)
  when counting:
    if packHook != nil:                                 # the driver's read, before (h) is freed
      var p = M2Pack(id: cint(x.id), x: addr x, W: uint32(x.W), T: uint32(x.T), w: cuint(w),
                     wh_build: cuint(x.whBuild), wh_pack: cuint(wh),
                     rewritten: uint64(if wh < x.whBuild: 2 * x.size else: 0),
                     bytes_list: uint64(x.W * w div 8), bytes_map: 0, bytes_table: uint64(x.indexBytes),
                     bytes_cells_build: uint64(x.cellBytes), bytes_cells_pack: uint64(2 * x.size * wh div 8))
      packHook(addr p)
  when auditing:
    let before = x.cellValues
    let why = x.auditIndex
    if why != nil: skDie(why)
  # 1. free (h)
  account(-x.indexBytes)
  freeBytes(x.index)
  x.index = nil
  # 2. narrow (l), entry k from bytes [8k, 8k + 8) to [k w/8, (k + 1) w/8), and shrink it to W entries
  let built = x.listBytes
  if w < 64:
    for k in 0 ..< x.W: setValue(x.list, w, k, valueAt(x.list, 64, k))
  x.w = w
  x.packed = true
  x.list = cast[Bytes](resizeBytes(x.list, x.listBytes))
  account(x.listBytes - built)
  # 3. narrow the fields in place, in one forward pass, then shrink the cells
  if wh < x.whBuild:
    let wide = x.cellBytes
    for i in 0 ..< 2 * x.size: setField(x.cells, wh, i, fieldAt(x.cells, x.whBuild, i))
    x.wh = wh
    x.cells = cast[Bytes](resizeBytes(x.cells, x.cellBytes))
    account(x.cellBytes - wide)
  when auditing:
    if x.cellValues != before: skDie("a cell's value changed in the pack")

# ----------------------------------------------------------------------------------------- create, enter

proc npk_create(rows, cols: uint32, id: cint): ptr Packed {.exportc, dynlib.} =
  ## an empty matrix, ready to be built: the cells at the build handle width, (h) filled with the empty mark in one
  ## step, and (l) allocated for 2 rows cols entries of 64 bits
  # handles of at most 32 bits: T_fix <= 2^31 (v0.1 §5.2, §11)
  if rows == 0 or cols == 0 or 2 * uint64(rows) * uint64(cols) > (1'u64 shl 31): skDie("Packed: bad dimensions")
  let x = cast[ptr Packed](allocZeroed(sizeof(Packed)))
  x.rows = int(rows)
  x.cols = int(cols)
  x.id = int(id)
  x.tbits = bitLen(uint64(x[].bound - 1))              # T_fix = 2^ceil(log2 (2 rows cols))
  x.T = 1 shl x.tbits
  x.whBuild = buildWidth(x.tbits)
  x.wh = x.whBuild
  x.w = 64
  x.cells = cast[Bytes](allocBytes(x[].cellBytes))
  x.index = cast[Bytes](allocBytes(x[].tableBytes))
  fillOnes(x.index, x[].tableBytes)
  x.list = cast[Bytes](allocBytes(x[].listBytes))
  account(x[].cellBytes + x[].indexBytes + x[].listBytes)
  x

proc npk_destroy(x: ptr Packed) {.exportc, dynlib.} =
  if x == nil: return
  account(-(x[].cellBytes + x[].indexBytes + x[].listBytes))
  freeBytes(x.cells)
  freeBytes(x.list)
  freeBytes(x.index)
  freeBytes(x)

proc npk_enter(x: ptr Packed, m, d: int64): SkStatus {.exportc, dynlib.} =
  ## the next cell, in row-major order, reduced gcd-first; the last one packs the matrix
  if x.packed: return skEInval
  var t: Tally
  var r: Ratio
  result = reduce(m, d, r, t)
  if result != skOk: return
  x[].counted(phEntry, t, record = false)
  x[].storeCell(x.nf div 2, r)
  if x.nf == x[].bound: x[].pack()

# ----------------------------------------------------------------------------------------- the operations

func ready(x: Packed): bool = x.packed

proc npk_madd(a, b, c: ptr Packed): SkStatus {.exportc, dynlib.} =
  ## c = a + b, cell by cell in row order (v0.1 §6.1), both operands read by handle; then the pack
  if a.rows != b.rows or a.cols != b.cols or c.rows != a.rows or c.cols != a.cols or
      not a[].ready or not b[].ready or c.nf != 0 or c.packed: return skEInval
  for i in 0 ..< c[].size:
    var x = a[].denominator(i)
    let xMarked = markTest(c.marks, x.b)                # both tested, the left first (§17.10 item 2)
    var y = b[].denominator(i)
    let yMarked = markTest(c.marks, y.b)
    if xMarked or yMarked:
      c[].marked(passed)
      c[].storeCell(i, mark)
      continue
    a[].numerator(i, x)                                 # a stopped cell reads no numerator
    b[].numerator(i, y)
    var t: Tally
    var s: Ratio
    let fits = add(x.operand, y.operand, s, t)
    c[].counted(phAdd, t)
    c[].crossTested(t, x.reads + y.reads)
    if fits:
      c[].storeCell(i, s)
    else:
      c[].marked(made)
      c[].storeCell(i, mark)
  c[].pack()
  skOk

proc cell(row: ptr UncheckedArray[Decoded], b, c: var Packed, j: int): Ratio =
  ## one product cell (§17.6), A's row from the row buffer and B's column by handle, summed from its first
  ## product, term by term until a mark or an overflow (§17.10 item 4)
  var s: Ratio
  for k in 0 ..< b.rows:
    let x = row[k]
    let xMarked = markTest(c.marks, x.b)
    var y = b.denominator(k * b.cols + j)
    let yMarked = markTest(c.marks, y.b)
    if xMarked or yMarked:
      c.marked(passed)
      return mark
    b.numerator(k * b.cols + j, y)                      # a stopped term reads no numerator of B
    var t: Tally
    var p: Ratio
    let fits = multiply(x.operand, y.operand, p, t)
    c.counted(phMul, t)
    c.crossTested(t, x.reads + y.reads)                 # a buffer read counts as the by-handle read it replaces
    if not fits:
      c.marked(made)
      return mark
    if k == 0:
      s = p
    else:
      var u: Tally
      let fits = add(s.operand, p.operand, s, u)        # on two temporaries: no cross test, no read
      c.counted(phAdd, u)
      if not fits:
        c.marked(made)
        return mark
  s

proc products(a, b, c: var Packed) =
  ## every cell of c = a b, in row order. The row buffer is local to this function: row i of a, decoded once,
  ## serves the row's cols cells; it is counted in the accounted total while it exists, and freed before the pack.
  let bytes = a.cols * sizeof(Decoded)
  c.rowBytes = bytes
  let row = cast[ptr UncheckedArray[Decoded]](allocBytes(bytes))
  account(bytes)
  for i in 0 ..< c.rows:
    for k in 0 ..< a.cols: row[k] = a.decode(i * a.cols + k)
    for j in 0 ..< c.cols:
      c.storeCell(i * c.cols + j, cell(row, b, c, j))
  account(-bytes)
  freeBytes(row)

proc npk_mmul(a, b, c: ptr Packed): SkStatus {.exportc, dynlib.} =
  ## c = a b; then the pack
  if a.cols != b.rows or c.rows != a.rows or c.cols != b.cols or
      not a[].ready or not b[].ready or c.nf != 0 or c.packed: return skEInval
  products(a[], b[], c[])
  c[].pack()
  skOk

# ----------------------------------------------------------------------------------------- read-out, reports

proc npk_read(x: ptr Packed, cell: uint32, m, d: var int64): SkStatus {.exportc, dynlib.} =
  if not x[].ready or int(cell) >= x[].size: return skEInval
  let c = x[].decode(int(cell))
  m = sgn(c.neg, c.a)
  d = c.b
  skOk

proc npk_memory(x: ptr Packed, rep: var SkMemory) {.exportc, dynlib.} =
  ## the accounted bytes of every structure alive: the cells, (l), and (h) during a build
  zeroMem(addr rep, sizeof(rep))
  rep.item[0] = SkMemItem(name: "cells", used: uint64(x[].cellBytes), allocated: uint64(x[].cellBytes))
  rep.item[1] = SkMemItem(name: "list", used: uint64(x[].listBytes), allocated: uint64(x[].listBytes))
  rep.n = 2
  if x.index != nil:
    rep.item[2] = SkMemItem(name: "index", used: uint64(x[].indexBytes), allocated: uint64(x[].indexBytes))
    rep.n = 3
  rep.used = uint64(x[].cellBytes + x[].listBytes + x[].indexBytes)
  rep.allocated = rep.used
  rep.values = uint64(x[].size)
  rep.used_per_rational = float(rep.used) / float(x[].size)
  rep.allocated_per_rational = rep.used_per_rational

proc npk_figures(x: ptr Packed, f: var M2Fig) {.exportc, dynlib.} =
  f = M2Fig(R: uint32(x.rows), K: uint32(x.cols), T: uint32(x.T), W: uint32(x.W), nf: uint32(x.nf),
            w: cuint(x.w), wh: cuint(x.wh), wh_build: cuint(x.whBuild), packed: cint(ord(x.packed)),
            bytes_cells: uint64(x[].cellBytes), bytes_vals: uint64(x[].listBytes))

proc npk_rowbuf(x: ptr Packed): uint64 {.exportc, dynlib.} =
  ## the bytes of the row buffer of the mul that built x (§17.11, per mul step); 0 for any other matrix
  uint64(x.rowBytes)

proc npk_counters(x: ptr Packed, k: ptr SkCounters, xk: ptr R2XCounters): SkStatus {.exportc, dynlib.} =
  when counting:
    if k != nil: k[] = x.cnt
    if xk != nil: xk[] = x.xcnt
    skOk
  else:
    skENotBuilt

proc npk_storage(x: ptr Packed, s: ptr M2SCount, o: ptr M2OCount): SkStatus {.exportc, dynlib.} =
  when counting:
    if s != nil: s[] = x.stores
    if o != nil: o[] = x.cross
    skOk
  else:
    skENotBuilt

proc npk_marks(x: ptr Packed, m: ptr Marks): SkStatus {.exportc, dynlib.} =
  when counting:
    m[] = x.marks
    skOk
  else:
    skENotBuilt

proc npk_audit(x: ptr Packed, why: var cstring): SkStatus {.exportc, dynlib.} =
  ## test build, on a packed matrix (§17.12, v0.2 §15.7 item 5): (h) freed and no other temporary; (l) at the
  ## narrowest width that holds its widest entry, its entries distinct, non-negative, each used by some cell; every
  ## handle < W, every denominator's tag 0; every cell in lowest terms with d >= 1 and zero as +0/1, or the mark +0/0
  when auditing:
    template fail(msg: string) =
      why = msg
      return skEAudit
    if not x.packed: fail("the matrix is not packed")
    if x.index != nil: fail("(h) is not freed")
    var widest = 1
    for k in 0 ..< x.W:
      let v = x[].value(k)
      if v < 0: fail("a list entry is negative")
      widest = max(widest, bitLen(uint64(v)) + 1)
    if listWidth(widest) != x.w: fail("(l) is not at the narrowest width that holds its widest entry")
    if packWidth(x.W) != x.wh: fail("the fields are not at the packed handle width")
    var used = newSeq[bool](x.W)                        # audit copies: the entries used, and the sorted list
    for i in 0 ..< 2 * x[].size:
      let f = fieldAt(x.cells, x.wh, i)
      if f shr 1 >= x.W: fail("a handle is not < W")
      if i mod 2 == 1 and (f and 1) != 0: fail("a denominator's tag is not 0")
      used[f shr 1] = true
    for k in 0 ..< x.W:
      if not used[k]: fail("a list entry is used by no cell")
    var sorted = newSeq[int64](x.W)
    for k in 0 ..< x.W: sorted[k] = x[].value(k)
    sorted.sort()
    for k in 1 ..< x.W:
      if sorted[k] == sorted[k - 1]: fail("a whole number is stored twice")
    for i in 0 ..< x[].size:
      let c = x[].decode(i)
      if c.b == 0:
        if c.a != 0 or c.neg: fail("a cell with d = 0 is not the mark +0/0")
      elif gcd(c.a, c.b) != 1 or (c.a == 0 and c.neg):
        fail("a cell is not in lowest terms (or zero is not +0/1)")
    skOk
  else:
    skENotBuilt

proc npk_t_whole(x: ptr Packed, slot: uint32, v: var int64): SkStatus {.exportc, dynlib.} =
  ## test build: during a pack, slot s of (h) as the value at its position, or -1 if empty; once packed, entry s
  when auditing:
    let s = int(slot)
    if x.index != nil:
      if s >= x.T: return skEInval
      let k = fieldAt(x.index, x.whBuild, s)
      v = if k == (1 shl x.whBuild) - 1: -1 else: valueAt(x.list, 64, k)
    else:
      if s >= x.W: return skEInval
      v = x[].value(s)
    skOk
  else:
    skENotBuilt

proc npk_t_field(x: ptr Packed, f: uint32, v: var uint32): SkStatus {.exportc, dynlib.} =
  ## test build: handle field f's raw value, 2k + t
  when auditing:
    if int(f) >= x.nf: return skEInval
    v = uint32(fieldAt(x.cells, x.wh, int(f)))
    skOk
  else:
    skENotBuilt

{.pop.}
