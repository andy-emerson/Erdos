#!/bin/bash
# Second batch (cycle-bounds.md, Section 7): finish the cone run, then the cheap further jumps.
set -e
cd "$(dirname "$0")"
X="python3 gstop_ext.py"
$X - 1e5 6.8e10 2e9 2 logs/ext2/cone_2+i.stop ./gstopcone 2 1 0 -1 -1 0 {lo} {hi}
$X - 6.8e10 1.7e11 2e9 2 logs/ext2/cone_2+i.stop ./gstopcone 2 1 0 -1 -1 0 {lo} {hi}
$X E2_a-2-1i.rule 1e10 1.4e10 5e8 2 logs/ext2/E2_a-2-1i.stop ./gstopf 10000000 all
$X E2_a1+2i.rule 1e10 1.4e10 5e8 2 logs/ext2/E2_a1+2i.stop ./gstopf 10000000 all
$X E2_a-1-2i.rule 1e10 1.7e10 5e8 2 logs/ext2/E2_a-1-2i.stop ./gstopf 10000000 all
$X E2_a-2+1i.rule 1e10 1.9e10 5e8 2 logs/ext2/E2_a-2+1i.stop ./gstopf 10000000 all
$X k7_a-11_8.rule 1e9 9.3e10 1e9 2 logs/ext2/k7_a-11_8.stop
echo ALL DONE
