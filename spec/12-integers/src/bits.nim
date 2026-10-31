# bits.nim: gcc's count-trailing/leading-zero builtins (the compiler's, no library). Undefined at 0, as the builtins.
proc builtinCtz64(x: uint64): cint {.importc: "__builtin_ctzll", nodecl.}
proc builtinClz64(x: uint64): cint {.importc: "__builtin_clzll", nodecl.}
proc builtinCtz32(x: uint32): cint {.importc: "__builtin_ctz", nodecl.}
proc builtinClz32(x: uint32): cint {.importc: "__builtin_clz", nodecl.}

template ctz*(x: uint64): int = int(builtinCtz64(x))
template clz*(x: uint64): int = int(builtinClz64(x))
template ctz*(x: uint32): int = int(builtinCtz32(x))
template clz*(x: uint32): int = int(builtinClz32(x))
