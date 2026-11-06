#!/bin/bash
# Third batch (cycle-bounds.md, Section 7.3): the next jumps that cost minutes.
set -e
cd "$(dirname "$0")"
X="python3 gstop_ext.py"
while pgrep -f "gstop_ext.py k6_a-10-1i_C" >/dev/null; do sleep 10; done
$X k7_a-11_4.rule 1.1e10 1.4e10 5e8 2 logs/ext2/k7_a-11_4.stop
$X k7_a-12_3.rule 4.6e9 1.55e10 5e8 2 logs/ext2/k7_a-12_3.stop
$X E2_a1+2i.rule 1.4e10 1.6e10 5e8 2 logs/ext2/E2_a1+2i.stop ./gstopf 10000000 all
for s in A B C; do $X k6_a-7-4i_$s.rule 3.3e10 4.4e10 1e9 2 logs/ext2/k6_a-7-4i_$s.stop; done
for s in B D; do $X k6_a-11-2i_$s.rule 3e7 1.35e8 5e6 2 logs/ext2/k6_a-11-2i_$s.stop; done
for s in B D; do python3 gstop_big.py k6_a-11-2i_$s.rule logs/ext2/k6_a-11-2i_$s.stop 1e8 > logs/ext2/k6_a-11-2i_$s.big; done
echo ALL DONE
