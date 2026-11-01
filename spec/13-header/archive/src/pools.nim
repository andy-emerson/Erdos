# pools.nim: spec 13 §1 Magnitudes, Result class and Pools, shared by A and B.
# A magnitude is a 5-bit class c and a word: class 0, the word is the magnitude; class 1..31, the word is a slot index
# into pool c, whose slots are 2^c words, lowest first. A pool's free list is linked through freed slots' first words
# and ended by all ones. Allocation pops the free list, else takes the next slot, else doubles the pool (one slot at
# first use). Result slots are allocated before any pointer is taken: growth moves pointers, not indices.
import routines, alloc

type Pool* = object
  mem*: Words        # the slots
  cap*: Word         # slots held
  next*: Word        # the next slot never taken
  head*: Word        # the free list (all ones: empty)

const EndMark* = not 0'u64

var pools*: array[32, Pool]   # pools[1 .. 31]
for c in 0 .. 31: pools[c].head = EndMark     # every free list empty at first

proc cls*(n: int): int {.inline.} =
  ## the smallest class holding n words (class 0 holds one word); above 31 is an error
  var c = 0
  while (1 shl c) < n: inc c
  if c > 31: specError("class above 31")
  c

proc grow(c: int) {.noinline.} =
  ## double pool c (one slot at first use); the old block is copied and freed
  let p = addr pools[c]
  if p.next == EndMark: specError("pool full (2^W - 1 slots)")
  let nc = (if p.cap == 0: 1'u64 else: 2'u64 * p.cap)
  let size = 1 shl c
  let m = cast[Words](xalloc(int(nc) * size * sizeof(Word)))
  for k in 0 ..< int(p.cap) * size: m[k] = p.mem[k]
  if p.mem != nil: xfree(p.mem)
  p.mem = m
  p.cap = nc

proc slotAlloc*(c: int): Word {.inline.} =
  let p = addr pools[c]
  if p.head != EndMark:                  # pop the free list
    result = p.head
    p.head = p.mem[int(result shl c)]
  else:
    if p.next == p.cap: grow(c)
    result = p.next
    p.next = p.next + 1

proc slotRelease*(c: int; i: Word) {.inline.} =
  let p = addr pools[c]
  p.mem[int(i shl c)] = p.head
  p.head = i

template slotPtr*(c: int; i: Word): Words = cast[Words](addr pools[c].mem[int(i shl c)])

proc partLen*(c: int; w: Word): int {.inline.} =
  ## the real length of a magnitude: scan its slot down to the top nonzero word (the upper half, when canonical)
  if c == 0: return int(w != 0)
  norm(slotPtr(c, w), 1 shl c)

template opnd*(c: int; w: Word; loc: var Word): Words =
  ## a pointer to a magnitude's words: the inline word (copied to a local) or the slot. Taken after allocation.
  (if c == 0: cast[Words](addr loc) else: slotPtr(c, w))

proc partCopy*(c: int; w: Word): (int, Word) {.inline.} =
  ## §1 Operations: a pool number's slot copied into a new slot; an inline number copies its word
  if c == 0: return (0, w)
  let j = slotAlloc(c)
  let s = slotPtr(c, w)
  let d = slotPtr(c, j)
  for k in 0 ..< (1 shl c): d[k] = s[k]
  (c, j)

proc partMake*(m: openArray[Word]): (int, Word) =
  ## (class, word) for a magnitude given as words, lowest first, top nonzero (outside the operations)
  let n = m.len
  let c = cls(n)
  if c == 0: return (0, (if n == 1: m[0] else: 0'u64))
  let i = slotAlloc(c)
  let p = slotPtr(c, i)
  for k in 0 ..< (1 shl c): p[k] = (if k < n: m[k] else: 0'u64)
  (c, i)

proc partWords*(c: int; w: Word): seq[Word] =
  ## the magnitude's words at its real length (outside the operations)
  if c == 0:
    if w != 0: result.add w
    return
  let p = slotPtr(c, w)
  for k in 0 ..< partLen(c, w): result.add p[k]

proc slotWords*(c: int; w: Word): seq[Word] =
  ## the whole slot (outside the operations: the check)
  let p = slotPtr(c, w)
  for k in 0 ..< (1 shl c): result.add p[k]

# ------------------------------------------------------------------------------------------------ result slots
type Res* = object
  c*: int         # the class of the bound
  i*: Word        # its slot
  loc*: Word      # class 0: the one-word local

proc resNew*(bound: int): Res {.inline.} =
  ## §1 Result class: the smallest class holding the bound; class 0 is a one-word local
  result.c = cls(bound)
  result.loc = 0
  if result.c != 0: result.i = slotAlloc(result.c)

template resDst*(r: Res): Words =
  (if r.c == 0: cast[Words](addr r.loc) else: slotPtr(r.c, r.i))

proc moveDown(r: Res; n, t: int): Word {.noinline.} =
  ## the result fits class t (1 .. c - 1): a slot there, the words copied and zero-filled, the predicted slot released
  let j = slotAlloc(t)
  let s = slotPtr(r.c, r.i)
  let d = slotPtr(t, j)
  for k in 0 ..< n: d[k] = s[k]
  for k in n ..< (1 shl t): d[k] = 0
  slotRelease(r.c, r.i)
  j

proc finish*(r: var Res; n: int): (int, Word) {.inline.} =
  ## the result of length n: the rest of its slot zeroed (§1 Routines); it stays, or moves down to the smallest class
  ## holding it (class 0: the inline word), and the slot is released. Pointers are taken afresh here.
  if r.c == 0: return (0, r.loc)
  let p = slotPtr(r.c, r.i)
  for k in n ..< (1 shl r.c): p[k] = 0
  let t = cls(n)
  if t == r.c: return (r.c, r.i)
  if t == 0:
    let w = p[0]                     # 0 when n = 0, since the rest was zeroed
    slotRelease(r.c, r.i)
    return (0, w)
  (t, moveDown(r, n, t))

# ------------------------------------------------------------------------------------------------ checks (outside the clock)
proc poolsCheck*(): string =
  ## every slot taken is on its pool's free list exactly once; "" when so
  for c in 1 .. 31:
    let p = addr pools[c]
    var seen = newSeq[bool](int(p.next))
    var i = p.head
    var cnt = 0
    while i != EndMark:
      if i >= p.next: return "pool " & $c & ": free list holds slot " & $i & ", never taken"
      if seen[int(i)]: return "pool " & $c & ": slot " & $i & " twice on the free list"
      seen[int(i)] = true
      inc cnt
      i = p.mem[int(i shl c)]
    if Word(cnt) != p.next: return "pool " & $c & ": " & $(int(p.next) - cnt) & " of " & $p.next & " slots not free"
  ""

proc poolBytes*(): int =
  ## the pools' capacity in bytes (memory: the slack beside the slots in use)
  for c in 1 .. 31: result += int(pools[c].cap) * (1 shl c) * sizeof(Word)
