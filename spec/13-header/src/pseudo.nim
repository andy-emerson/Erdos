# pseudo.nim: what the builds share to keep gcc to the portable pseudocode (RULES.md, algorithm design rule 6).

type Word* = uint64

proc specError*() {.noinline, noreturn.} =
  ## an error of the spec (A: a sum across axes of two nonzero numbers, an expression): exit 3. C's own output and exit,
  ## so the call raises nothing and the operations around it keep no exception check
  proc write(fd: cint; s: cstring; n: csize_t): int {.importc, header: "<unistd.h>".}
  proc exit(c: cint) {.importc, header: "<stdlib.h>", noreturn.}
  const msg = "error: sum across axes of two nonzero numbers (an expression)\n"
  discard write(2, msg, csize_t(msg.len))
  exit(3)

template opaque*(x: Word): Word =
  ## x, unchanged, hidden from gcc (no instructions; spec 12 divbench2's opaque): gcc no longer knows that x is 0 or 1,
  ## so it cannot turn a mask built from x (0 - x, AND) back into a conditional move or a branch
  var y = x
  {.emit: ["__asm__(\"\" : \"+r\"(", y, "));"].}
  y
