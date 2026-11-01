#!/bin/sh
# build.sh: spec 13 round 2's four builds, each linked with the driver src/bench.nim, into ../build/bin/:
#   control   Control: spec 12's Growing with A's header list
#   a         Design A
#   abf       Design A's branch-free row (-d:branchfree)
#   b         Design B
# and <b>-mem, the memory build (-d:mem: Control's blocks accounted). Usage: NIM=/path/to/nim-2.2.14/bin/nim sh build.sh
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
NIM=${NIM:-nim}
B=$HERE/../build
"$NIM" -v | sed -n 1p | grep -q "Version 2.2.14" || { echo "need Nim 2.2.14 (../../11-matrix/nim/README.md)"; exit 1; }
gcc --version | sed -n 1p
# spec 12's flags (C backend, danger, ARC, malloc, panics on, gcc -O2), with scalar code everywhere and gcc's
# if-conversion off, so that the machine code keeps the portable pseudocode's branches and masks (no conditional moves,
# no vector code); README, Choices.
NIMFLAGS="--hints:off -d:danger --mm:arc -d:useMalloc --panics:on --cc:gcc"
NIMC="--gcc.options.speed=-O2 -fno-strict-aliasing -fno-ident -fno-tree-vectorize -fno-if-conversion -fno-if-conversion2 -fno-ssa-phiopt"
mkdir -p "$B/bin"
one() {  # name, build, extra flags...
    name=$1; b=$2; shift 2
    # shellcheck disable=SC2086
    "$NIM" c $NIMFLAGS "$NIMC" -d:bld=$b "$@" "--nimcache:$B/nimcache/$name" "-o:$B/bin/$name" "$HERE/bench.nim"
}
flags() { case $1 in abf) echo "-d:branchfree" ;; *) echo "" ;; esac; }
for b in control a abf b; do
    f=$(flags $b)
    # shellcheck disable=SC2086
    one "$b" $b $f
    # shellcheck disable=SC2086
    one "$b-mem" $b -d:mem $f
done
# no conditional moves anywhere in the builds' own code (rule 6): count them in the timing binaries
for b in control a abf b; do
    n=$(objdump -d --no-show-raw-insn "$B/bin/$b" | awk '/^[0-9a-f]+ <.*>:$/ {f=$2} /\tcmov/ && f ~ /(op|rAdd|rMac|rSub|rAbs|rCmp|rDiv|rGcd|slot|grow|finish|moveDown|partLen|loop_)/ {c++} END {print c+0}')
    echo "$b: $n conditional moves in the builds' functions"
done
echo built
