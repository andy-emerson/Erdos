# control.nim: Control (spec §1.1). Nim's uint64 with its own operators and std/math.gcd. Overflow wraps.
import std/math

type Val* = uint64
const wordBuild* = true

proc fromWords*(w: openArray[uint64]): Val = (if w.len > 0: w[0] else: 0'u64)   # the value mod 2^64

proc opAdd*(a, b: Val): Val {.inline.} = a + b
proc opSub*(a, b: Val): Val {.inline.} = a - b
proc opAbsdiff*(a, b: Val): Val {.inline.} = (if a >= b: a - b else: b - a)
proc opMul*(a, b: Val): Val {.inline.} = a * b
proc opDivexact*(a, b: Val): Val {.inline.} = a div b
proc opGcd*(a, b: Val): Val {.inline.} = gcd(a, b)
