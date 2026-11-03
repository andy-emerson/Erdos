# bingcd.nim: std/math.gcd's binary gcd for unsigned integers (Nim 2.2.14, lib/pure/math.nim), copied so the flag
# builds are self-contained. With evenStep, the shared-power step runs only when bothEven (the Even flag build).
import bits

proc bingcd*(x0, y0: uint64; evenStep: static bool; bothEven = true): uint64 {.inline.} =
  var x = x0
  var y = y0
  if x == 0:
    return y
  if y == 0:
    return x
  when evenStep:
    let shift = (if bothEven: ctz(x or y) else: 0)
  else:
    let shift = ctz(x or y)
  y = y shr ctz(y)
  while x != 0:
    x = x shr ctz(x)
    if y > x:
      swap y, x
    x -= y
  y shl shift
