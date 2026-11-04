#!/bin/bash
# norm 1e9..1e10 in ten chunks, one core, niced
cd /home/claude/erdos/research/goldbach
for k in 1 2 3 4 5 6 7 8 9; do
  lo=$((k*1000000000+1)); hi=$(((k+1)*1000000000))
  nice ./gg $hi EFQ 27 2000000 $lo > chunks/run_${k}e9.txt 2> chunks/run_${k}e9.err
done
echo DONE > chunks/ALLDONE
