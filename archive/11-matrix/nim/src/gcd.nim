## gcd.nim: the shared gcd of spec v0.4 §17.7, in its own file under its own pin (GCD.SHA256SUMS, checked by
## build.sh; §17.7, §17.14). Both models use it, through core.nim; its values are 08's, and a gcd is counted once, by its caller,
## its inner steps not at all. The test build counts how often the remainder-first path (6a) is taken, for its tests.

{.push raises: [].}

func ctz*(x: uint64): int {.importc: "__builtin_ctzll", nodecl.}
func clz(x: uint64): int {.importc: "__builtin_clzll", nodecl.}

func mag*(v: int64): uint64 {.inline.} =
  ## |v|, for v != -2^63
  if v < 0: 0'u64 - cast[uint64](v) else: cast[uint64](v)

func bitLen*(x: uint64): int {.inline.} =
  if x == 0: 0 else: 64 - clz(x)

when defined(skAudit):
  var remainderFirst*: int   ## test build only: the times step 2 (6a) took its one remainder

func gcd*(p, q: int64): int64 {.inline.} =
  ## gcd(|p|, |q|), gcd(u, 0) = u
  var u = mag(p)
  var v = mag(q)
  if u == 0: return cast[int64](v)                      # 1. zeros first
  if v == 0: return cast[int64](u)
  let lu = bitLen(u)
  let lv = bitLen(v)
  if lu - lv > 8:                                       # 2. (6a) sizes far apart: one remainder first
    when defined(skAudit): {.cast(noSideEffect).}: remainderFirst += 1
    u = u mod v
    if u == 0: return cast[int64](v)
  elif lv - lu > 8:
    when defined(skAudit): {.cast(noSideEffect).}: remainderFirst += 1
    v = v mod u
    if v == 0: return cast[int64](u)
  let shift = ctz(u or v)                               # 3. the binary method
  u = u shr ctz(u)
  v = v shr ctz(v)
  while u != v:
    let z = ctz(u - v)                                  # 4. (6b) ctz of the raw difference
    let d = if u < v: v - u else: u - v
    u = min(u, v)
    v = d shr z
  cast[int64](u shl shift)

{.pop.}
