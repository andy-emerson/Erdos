#!/bin/sh
# build.sh: spec 13 v0.2's four builds, each linked with the driver src/bench.nim, into ../build/bin/:
#   control   Control: int64, complex as a pair
#   a         Design A
#   abf       Design A's branch-free row
#   b         Design B
# and each build's assembly (../build/asm/<b>.s, objdump of the loops). Usage, from anywhere:
#   NIM=/path/to/nim-2.2.14/bin/nim sh build.sh
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
NIM=${NIM:-nim}
B=$HERE/../build
"$NIM" -v | sed -n 1p | grep -q "Version 2.2.14" || { echo "need Nim 2.2.14"; exit 1; }
gcc --version | sed -n 1p
# spec 12's flags (C backend, danger, ARC, malloc, panics on, gcc -O2), with scalar code and gcc's if-conversion off, so
# that the machine code keeps the portable pseudocode's branches and masks: no conditional moves, no vector code
# (README, Choices).
NIMFLAGS="--hints:off -d:danger --mm:arc -d:useMalloc --panics:on --cc:gcc"
NIMC="--gcc.options.speed=-O2 -fno-strict-aliasing -fno-ident -fno-tree-vectorize -fno-if-conversion -fno-if-conversion2 -fno-ssa-phiopt"
mkdir -p "$B/bin" "$B/asm"
for b in control a abf b; do
    # shellcheck disable=SC2086
    "$NIM" c $NIMFLAGS "$NIMC" -d:bld=$b "--nimcache:$B/nimcache/$b" "-o:$B/bin/$b" "$HERE/bench.nim"
    objdump -d --no-show-raw-insn "$B/bin/$b" | awk '/^[0-9a-f]+ <.*>:$/ {p = ($2 ~ /^<loop_/)} p' > "$B/asm/$b.s"
done
# rule 6: no conditional moves (and no vector code) in the timed loops; and the branch-free row's add and subtract
# loops have no conditional branch but the loop's own (one jump back per element, one per pass)
fail=0
for b in control a abf b; do
    c=$(grep -c '\scmov' "$B/asm/$b.s" || true)
    v=$(grep -cE '\s(v?p[a-z]+|[a-z]+p[sd])\s+%xmm' "$B/asm/$b.s" || true)
    echo "$b: $c conditional moves, $v vector instructions in the loops"
    [ "$c" = 0 ] && [ "$v" = 0 ] || fail=1
done
for f in loop_int_add loop_int_sub; do
    n=$(awk -v f="<$f>:" '$2 == f {p = 1; next} /^[0-9a-f]+ <.*>:$/ {p = 0} p && /\tj[a-z]+ /' "$B/asm/abf.s" | grep -vc '\sjmp\s' || true)
    echo "abf $f: $n conditional branches (the two loops' own: 2)"
    [ "$n" = 2 ] || fail=1
done
[ $fail = 0 ] || { echo "rule 6 check FAILED"; exit 1; }
echo built
