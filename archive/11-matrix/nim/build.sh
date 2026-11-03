#!/bin/sh
# build.sh: the Nim build of round m4 (README.md), into build/ (nothing outside this folder is written).
#   build/lib/lib{control,packed}-<build>.a   the two models, Nim -> C, gcc -O2
#   build/bin/bench-{NCTL,NPK}-<build>        each linked with the driver copy driver/bench_m4n.c
#   build/bin/tc3gate_m4                      the TC gate: tc3.c with the new gcd and §17.6's summation
# Builds: timing (nothing), counting (SK_COUNT / -d:skCount), test (SK_COUNT SK_AUDIT / -d:skCount -d:skAudit),
# memory (M2_MEM / -d:m2Mem, linked with the heap wrapper).
# Usage: NIM=/path/to/nim sh build.sh     (the pinned Nim: README.md, "Nim")
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
SPEC=$(cd "$HERE/.." && pwd)
NIM=${NIM:-nim}
B=$HERE/build
CFLAGS="-std=c11 -Wall -Wextra -Werror -O2"
INC="-I $HERE/include -I $HERE/driver -I $SPEC/lib/include -I $SPEC/lib/vendor08/m4 -I $SPEC/lib/vendor08"
# Nim: C backend, a static library with no main; danger (no bounds, range, overflow or nil checks: overflow is the
# models' own explicit checks); ARC; Nim's allocator replaced by malloc; panics on (no error-flag test after calls);
# gcc at C's -O2
NIMFLAGS="--hints:off --app:staticlib --noMain -d:danger --mm:arc -d:useMalloc --panics:on --cc:gcc"
NIMC="--gcc.options.speed=-O2 -fno-strict-aliasing -fno-ident"
WRAP="-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free"

# the pins: the Nim gcd's own (§17.7, §17.14), 08's files and pin unchanged, the m4 gcd's pin, tc3.c (the TC gate's
# base, unchanged; m1_libconf.py's sha)
(cd "$HERE/src" && sha256sum --quiet -c GCD.SHA256SUMS)
(cd "$SPEC/lib/vendor08" && sha256sum --quiet -c SHA256SUMS)
(cd "$SPEC/lib/vendor08/m4" && sha256sum --quiet -c SHA256SUMS)
echo "7956bc5f9ebc72f4e12b604ec1a5ab743d3ed75602cb075d6fb902035013494b  $SPEC/lib/src/tc3.c" | sha256sum --quiet -c
V=$("$NIM" -v); echo "$V" | sed -n 1p
mkdir -p "$B/lib" "$B/bin" "$B/obj"
for build in timing counting test memory; do
    case $build in
        timing) D=""; ND="";;
        counting) D="-DSK_COUNT"; ND="-d:skCount";;
        test) D="-DSK_COUNT -DSK_AUDIT"; ND="-d:skCount -d:skAudit";;
        memory) D="-DM2_MEM"; ND="-d:m2Mem";;
    esac
    O=$B/obj/$build
    mkdir -p "$O"
    # the driver's shared C objects: 08's sk_die and names, the per-operation record, the hooks and the total
    gcc $CFLAGS $D $INC -c "$SPEC/lib/vendor08/common.c" -o "$O/common.o"
    gcc $CFLAGS $D $INC -c "$SPEC/lib/src/m1common.c" -o "$O/m1common.o"
    gcc $CFLAGS $D $INC -c "$SPEC/lib/src/m2common.c" -o "$O/m2common.o"
    gcc $CFLAGS $D $INC -c "$SPEC/lib/src/m3common.c" -o "$O/m3common.o"
    COMMON="$O/common.o $O/m1common.o $O/m2common.o $O/m3common.o"
    L=""; [ $build = memory ] && L=$WRAP
    for m in control packed; do
        case $m in control) IMPL=NCTL;; packed) IMPL=NPK;; esac
        # shellcheck disable=SC2086
        "$NIM" c $NIMFLAGS $ND "$NIMC" "--passC:-I$HERE/include -I$SPEC/lib/include -I$SPEC/lib/vendor08" \
            "--nimcache:$B/nimcache/$m-$build" "-o:$B/lib/lib$m-$build.a" "$HERE/src/$m.nim"
        # shellcheck disable=SC2086
        gcc $CFLAGS $D -DIMPL_$IMPL $INC "$HERE/driver/bench_m4n.c" "$B/lib/lib$m-$build.a" $COMMON $L \
            -o "$B/bin/bench-$IMPL-$build"
    done
done
# the TC gate (§17.13): tc3.c unchanged, the new gcd (lib/vendor08/m4 first), the counting build
O=$B/obj/counting
gcc $CFLAGS -DSK_COUNT $INC -I "$SPEC/lib/src" "$HERE/test/tc3gate_m4.c" "$O/common.o" "$O/m1common.o" \
    -o "$B/bin/tc3gate_m4"
echo built
