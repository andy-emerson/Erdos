# dirbench.nim: multiplying directions (1, i, -1, -i as the counters 0..3), by five methods, timed the same way as the
# builds: each method over the same 1024 random pairs, reps calibrated to at least 50 ms, 15 reps, the median per
# operation; every result checksummed after the clock.
#   control   (s + t) mod 4, Nim's own mod
#   table     the 4 x 4 multiplication table, looked up
#   and3      (s + t) and 3
#   byte      uint8 counters added with no mask: a byte wraps at 256, a multiple of 4, so the low 2 bits stay right
#   top       the counter in a word's top 2 bits: an add's carry out of the top falls off, which is mod 4
import std/[os, strutils, random, algorithm]

const N = 1024
type Timespec {.importc: "struct timespec", header: "<time.h>".} = object
  tv_sec: clong
  tv_nsec: clong
var CLOCK_MONOTONIC {.importc, header: "<time.h>".}: cint
proc clock_gettime(id: cint; ts: var Timespec): cint {.importc, header: "<time.h>".}
proc now(): int64 =
  var t: Timespec
  discard clock_gettime(CLOCK_MONOTONIC, t)
  int64(t.tv_sec) * 1_000_000_000 + int64(t.tv_nsec)

const tbl = [[0'u8, 1, 2, 3], [1'u8, 2, 3, 0], [2'u8, 3, 0, 1], [3'u8, 0, 1, 2]]
var a8, b8, r8: array[N, uint8]
var aw, bw, rw: array[N, uint64]

template barrier() = {.emit: "__asm__ __volatile__(\"\" ::: \"memory\");".}

proc runControl(r: int) {.noinline.} =
  for it in 0 ..< r:
    for j in 0 ..< N: r8[j] = (a8[j] + b8[j]) mod 4
    barrier()
proc runTable(r: int) {.noinline.} =
  for it in 0 ..< r:
    for j in 0 ..< N: r8[j] = tbl[a8[j]][b8[j]]
    barrier()
proc runAnd3(r: int) {.noinline.} =
  for it in 0 ..< r:
    for j in 0 ..< N: r8[j] = (a8[j] + b8[j]) and 3
    barrier()
proc runByte(r: int) {.noinline.} =
  for it in 0 ..< r:
    for j in 0 ..< N: r8[j] = a8[j] + b8[j]
    barrier()
proc runTop(r: int) {.noinline.} =
  for it in 0 ..< r:
    for j in 0 ..< N: rw[j] = aw[j] + bw[j]
    barrier()

proc check(name: string) =
  for j in 0 ..< N:
    let want = (a8[j] + b8[j]) mod 4
    let got = (if name == "top": uint8(rw[j] shr 62) else: r8[j] and 3)
    doAssert got == want, name

proc main() =
  let name = paramStr(1)
  var rng = initRand(20261031)
  for j in 0 ..< N:
    a8[j] = uint8(rng.rand(3))
    b8[j] = uint8(rng.rand(3))
    aw[j] = uint64(a8[j]) shl 62
    bw[j] = uint64(b8[j]) shl 62
  let f = case name
    of "control": runControl
    of "table": runTable
    of "and3": runAnd3
    of "byte": runByte
    of "top": runTop
    else: quit "unknown method"
  f(1)
  check(name)
  var r = 1
  while true:
    let t0 = now()
    f(r)
    if now() - t0 >= 50_000_000: break
    r *= 2
  var ts: seq[float]
  for k in 0 ..< 15:
    let t0 = now()
    f(r)
    ts.add float(now() - t0) / float(r * N)
  ts.sort()
  var h = 0'u64
  for j in 0 ..< N: h = (h xor (uint64(r8[j]) + rw[j])) * 0x9E3779B97F4A7C15'u64
  echo name, " ", formatFloat(ts[7], ffDecimal, 4), " ", formatFloat((ts[14] - ts[0]) / ts[7], ffDecimal, 4), " ", toHex(h)

main()
