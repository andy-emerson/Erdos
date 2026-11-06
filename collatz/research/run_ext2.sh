#!/bin/bash
# Verification extensions for the further Gaussian maps (cycle-bounds.md, Section 7).
# Sequential, 2 workers each, checkpointed by gstop_ext.py (rerun to resume).  Logs in logs/ext2/.
set -e
cd "$(dirname "$0")"
mkdir -p logs/ext2
X="python3 gstop_ext.py"
# (1+i)^7 single-cycle maps, from 10^9 (G45 covers norm <= 10^9)
$X k7_a-12_3.rule 1e9 4.6e9 5e8 2 logs/ext2/k7_a-12_3.stop
$X k7_a-11_4.rule 1e9 1.1e10 5e8 2 logs/ext2/k7_a-11_4.stop
$X k7_a-12_7.rule 1e9 1.6e10 5e8 2 logs/ext2/k7_a-12_7.stop
# (1+i)^6 single-cycle maps, one per mirror pair, from norm 1
$X k6_a-10-1i_C.rule 1 1.6e8 4e7 2 logs/ext2/k6_a-10-1i_C.stop
$X k6_a-11-2i_B.rule 1 3e7 1e6 2 logs/ext2/k6_a-11-2i_B.stop
$X k6_a-11-2i_D.rule 1 3e7 1e6 2 logs/ext2/k6_a-11-2i_D.stop
$X k6_a-9-2i_D.rule 1 4.2e9 3e8 2 logs/ext2/k6_a-9-2i_D.stop
for s in A C; do $X k6_a-8-3i_$s.rule 1 1.2e10 5e8 2 logs/ext2/k6_a-8-3i_$s.stop; done
for s in A B; do $X k6_a-10-3i_$s.rule 1 8.2e9 5e8 2 logs/ext2/k6_a-10-3i_$s.stop; done
for s in A B C; do $X k6_a-7-4i_$s.rule 1 3.3e10 1e9 2 logs/ext2/k6_a-7-4i_$s.stop; done
# E_2 maps at (1+i)^2, all quadrants (V2), from norm 1
for a in 2-1i -1-2i -2-1i 1+2i -2+1i; do $X E2_a$a.rule 1 1e10 5e8 2 logs/ext2/E2_a$a.stop ./gstopf 10000000 all; done
# cone map 2+i on Z[i]/U, from 10^5 (stage 1 of fundamental-domain.md classifies norm <= 10^5)
$X - 1e5 6.8e10 2e9 2 logs/ext2/cone_2+i.stop ./gstopcone 2 1 0 -1 -1 0 {lo} {hi}
echo ALL DONE
