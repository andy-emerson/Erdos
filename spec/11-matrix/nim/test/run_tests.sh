#!/bin/sh
# run_tests.sh: every check of the Nim build (README.md, "Verification"), after build.sh. Nothing timed.
#   1. gcdtest     the gcd's tests (§17.12), test build
#   2. alloc       the structures, block by block (§17.8), timing and memory builds, each model
#   3. cases       the crafted tests of §17.12 through both models in every build, against the simulator
#   4. v0.4        the §17.9 workloads (gen.py), jobs 1, 4, 5, 8, 9, 12 of each op, against the simulator
#   5. m3          m3's 45 workloads, against the simulator and m3's exact references
#   6. ccompare    the Nim build against the C build on m3's workloads
# Usage: NIM=/path/to/nim WORK=/scratch/dir sh test/run_tests.sh   (WORK receives the generated workloads)
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/.." && pwd)
SPEC=$(cd "$ROOT/.." && pwd)
NIM=${NIM:-nim}
WORK=${WORK:?set WORK to a scratch directory}
B=$ROOT/build
INC="-I $ROOT/include -I $SPEC/lib/include -I $SPEC/lib/vendor08/m4 -I $SPEC/lib/vendor08"
WRAP="-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free"

"$NIM" c --hints:off -d:danger --mm:arc -d:useMalloc --panics:on -d:skCount -d:skAudit \
    "--passC:-I$ROOT/include -I$SPEC/lib/include -I$SPEC/lib/vendor08" "--passL:$B/obj/test/common.o" \
    "--nimcache:$B/nimcache/gcdtest" "-o:$B/bin/gcdtest" "$HERE/gcdtest.nim"
"$B/bin/gcdtest"

for b in timing memory; do
    D=""; [ $b = memory ] && D=-DM2_MEM
    O=$B/obj/$b
    for m in packed control; do
        X=""; [ $m = control ] && X=-DALLOC_CONTROL
        # shellcheck disable=SC2086
        gcc -std=c11 -Wall -Wextra -Werror -O2 $D $X $INC "$HERE/alloc.c" "$B/lib/lib$m-$b.a" \
            "$O/common.o" "$O/m1common.o" "$O/m2common.o" $WRAP -o "$B/bin/alloc-$m-$b"
        printf '%s %s: ' $m $b
        "$B/bin/alloc-$m-$b"
    done
done

rm -rf "$WORK/cases"
python3 "$HERE/cases.py" "$WORK/cases" > /dev/null
python3 "$HERE/check.py" "$WORK/cases" --jobs 2 --out "$WORK/check-cases.json" | tail -1

for op in add mul sop pos; do
    [ -f "$WORK/v04/$op/gen.json" ] || python3 "$HERE/gen.py" "$WORK/v04/$op" --op $op > /dev/null
done
python3 "$HERE/check.py" "$WORK/v04/add" "$WORK/v04/mul" "$WORK/v04/sop" "$WORK/v04/pos" --jobs 2 \
    --out "$WORK/check-v04.json" | tail -1

M3=$SPEC/test/build/m3
python3 "$HERE/check.py" "$M3/work-add" "$M3/work-mul" "$M3/work-sop" "$M3/work-pos" --jobs 2 \
    --out "$WORK/check-m3.json" | tail -1
python3 "$HERE/ccompare.py" --out "$WORK/ccompare.json" | tail -1
echo "all checks ran"
