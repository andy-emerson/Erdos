#!/bin/sh
# build.sh: the five builds of spec v0.1, each linked with the driver src/bench.nim, into build/bin/.
#   <b>-j1    timing, job 1: scalar (-fno-tree-vectorize)        <b>       timing, jobs 2-16
#   <b>-mem   the memory build (-d:mem: accounted bytes)          <b>-w32   Growing and Byte encoding at W = 32 (tests)
# for b in control pow2 even growing bytes. Usage: NIM=/path/to/nim-2.2.14/bin/nim sh build.sh
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
NIM=${NIM:-nim}
B=$HERE/build
"$NIM" -v | sed -n 1p | grep -q "Version 2.2.14" || { echo "need Nim 2.2.14 (README.md)"; exit 1; }
gcc --version | sed -n 1p
# the 11-matrix flags: C backend, danger (no runtime checks), ARC, malloc, panics on, gcc -O2
NIMFLAGS="--hints:off -d:danger --mm:arc -d:useMalloc --panics:on --cc:gcc"
NIMC="--gcc.options.speed=-O2 -fno-strict-aliasing -fno-ident"
mkdir -p "$B/bin"
one() {  # name, build, extra flags...
    name=$1; b=$2; shift 2
    # shellcheck disable=SC2086
    "$NIM" c $NIMFLAGS "$NIMC" -d:bld=$b "$@" "--nimcache:$B/nimcache/$name" "-o:$B/bin/$name" "$HERE/src/bench.nim"
}
for b in control pow2 even growing bytes; do
    one "$b-j1" $b "--passC:-fno-tree-vectorize"
    one "$b" $b
    one "$b-mem" $b -d:mem
done
for b in growing bytes; do one "$b-w32" $b -d:W=32; done
echo built
