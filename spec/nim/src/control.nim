## control.nim: Control (spec v0.4 §17.0, §17.13), the textbook model: each matrix is its r c cells, pairs (m, d) of
## 64-bit integers in lowest terms, d >= 1, zero as 0/1, row-major. No other structure (§17.8): no temporary, no local.
## The arithmetic is arith.nim's, shared with Packed. mul sums each cell from its first product (§17.6); an overflow
## marks the cell 0/0 and the job goes on (§17.10).
##
## Exported as nctl_* (include/m4n.h) for the driver.

{.push raises: [].}

import core, arith

type
  Pair = object
    m, d: int64
  Control = object
    cells: ptr UncheckedArray[Pair]       ## the matrix itself: rows x cols pairs
    rows, cols, entered: int
    when counting:
      cnt: SkCounters
      xcnt: R2XCounters
      marks: Marks

func size(x: Control): int = x.rows * x.cols

func operand(p: Pair): Operand {.inline.} = Operand(a: int64(mag(p.m)), b: p.d, neg: p.m < 0)
func pair(r: Ratio): Pair {.inline.} = Pair(m: r.n, d: r.e)

template counted(x: var Control, phase: Phase, t: Tally, record = true) =
  when counting: commit(x.cnt, x.xcnt, phase, t, record)

template marked(x: var Control, field: untyped) =
  when counting: x.marks.field += 1

proc checkOperand(p: Pair) {.inline.} =
  ## test build (v0.1 §4, as §17.10 amends it): an operand that is not the mark is d >= 1, in lowest terms,
  ## m != -2^63. Called after the mark tests, which catch the mark first.
  when auditing:
    if p.d < 1 or p.m == int64Min or gcd(p.m, p.d) != 1: skDie("an operand pair is not valid")

# ----------------------------------------------------------------------------------------- create, enter

proc nctl_create(rows, cols: uint32): ptr Control {.exportc, dynlib.} =
  if rows == 0 or cols == 0: skDie("Control: bad dimensions")
  result = cast[ptr Control](allocZeroed(sizeof(Control)))
  result.rows = int(rows)
  result.cols = int(cols)
  result.cells = cast[ptr UncheckedArray[Pair]](allocBytes(16 * result[].size))

proc nctl_destroy(x: ptr Control) {.exportc, dynlib.} =
  if x == nil: return
  freeBytes(x.cells)
  freeBytes(x)

proc nctl_enter(x: ptr Control, m, d: int64): SkStatus {.exportc, dynlib.} =
  ## the next cell, in row-major order, reduced gcd-first
  if x.entered >= x[].size: return skEInval
  var t: Tally
  var r: Ratio
  result = reduce(m, d, r, t)
  if result != skOk: return
  x[].counted(phEntry, t, record = false)
  x.cells[x.entered] = pair(r)
  x.entered += 1

# ----------------------------------------------------------------------------------------- the operations

proc nctl_madd(a, b, c: ptr Control): SkStatus {.exportc, dynlib.} =
  ## c = a + b, cell by cell in row order (v0.1 §6.1)
  if a.rows != b.rows or a.cols != b.cols or c.rows != a.rows or c.cols != a.cols or c.entered != 0 or
      a.entered != a[].size or b.entered != b[].size: return skEInval
  for i in 0 ..< c[].size:
    let x = a.cells[i]
    let y = b.cells[i]
    let xMarked = markTest(c.marks, x.d)                # both tested, the left first (§17.10 item 2)
    let yMarked = markTest(c.marks, y.d)
    if xMarked or yMarked:
      c[].marked(passed)
      c.cells[i] = pair(mark)
      continue
    checkOperand(x)
    checkOperand(y)
    var t: Tally
    var s: Ratio
    if add(x.operand, y.operand, s, t):
      c.cells[i] = pair(s)
    else:
      c[].marked(made)
      c.cells[i] = pair(mark)
    c[].counted(phAdd, t)
  c.entered = c[].size
  skOk

proc cell(a, b, c: var Control, i, j: int): Ratio =
  ## one product cell, summed from its first product (§17.6), term by term until a mark or an overflow (§17.10 item 4)
  let n = a.cols
  var s: Ratio
  for k in 0 ..< n:
    let x = a.cells[i * n + k]
    let y = b.cells[k * b.cols + j]
    let xMarked = markTest(c.marks, x.d)
    let yMarked = markTest(c.marks, y.d)
    if xMarked or yMarked:
      c.marked(passed)
      return mark
    checkOperand(x)
    checkOperand(y)
    var t: Tally
    var p: Ratio
    let fits = multiply(x.operand, y.operand, p, t)
    c.counted(phMul, t)
    if not fits:
      c.marked(made)
      return mark
    if k == 0:
      s = p
    else:
      var u: Tally
      let fits = add(s.operand, p.operand, s, u)
      c.counted(phAdd, u)
      if not fits:
        c.marked(made)
        return mark
  s

proc nctl_mmul(a, b, c: ptr Control): SkStatus {.exportc, dynlib.} =
  ## c = a b, cell by cell in row order
  if a.cols != b.rows or c.rows != a.rows or c.cols != b.cols or c.entered != 0 or
      a.entered != a[].size or b.entered != b[].size: return skEInval
  for i in 0 ..< c.rows:
    for j in 0 ..< c.cols:
      c.cells[i * c.cols + j] = pair(cell(a[], b[], c[], i, j))
  c.entered = c[].size
  skOk

# ----------------------------------------------------------------------------------------- read-out, reports

proc nctl_read(x: ptr Control, cell: uint32, m, d: var int64): SkStatus {.exportc, dynlib.} =
  if int(cell) >= x.entered: return skEInval
  m = x.cells[cell].m
  d = x.cells[cell].d
  skOk

proc nctl_memory(x: ptr Control, rep: var SkMemory) {.exportc, dynlib.} =
  ## the accounted bytes: the cells only (16 per cell)
  zeroMem(addr rep, sizeof(rep))
  let bytes = uint64(16 * x[].size)
  rep.n = 1
  rep.item[0] = SkMemItem(name: "cells", used: bytes, allocated: bytes)
  rep.used = bytes
  rep.allocated = bytes
  rep.values = uint64(x[].size)
  rep.used_per_rational = 16.0
  rep.allocated_per_rational = 16.0

proc nctl_counters(x: ptr Control, k: ptr SkCounters, xk: ptr R2XCounters): SkStatus {.exportc, dynlib.} =
  when counting:
    if k != nil: k[] = x.cnt
    if xk != nil: xk[] = x.xcnt
    skOk
  else:
    skENotBuilt

proc nctl_marks(x: ptr Control, m: ptr Marks): SkStatus {.exportc, dynlib.} =
  when counting:
    m[] = x.marks
    skOk
  else:
    skENotBuilt

proc nctl_audit(x: ptr Control, why: var cstring): SkStatus {.exportc, dynlib.} =
  ## test build: every cell entered is a valid pair (d >= 1, lowest terms, zero as 0/1, m != -2^63) or the mark
  when auditing:
    for i in 0 ..< x.entered:
      let p = x.cells[i]
      if p.d == 0:
        if p.m != 0:
          why = "a cell with d = 0 is not the mark 0/0"
          return skEAudit
      elif p.d < 0 or p.m == int64Min:
        why = "a cell's pair is out of range"
        return skEAudit
      elif gcd(p.m, p.d) != 1:
        why = "a cell is not in lowest terms (or zero is not 0/1)"
        return skEAudit
    skOk
  else:
    skENotBuilt

{.pop.}
