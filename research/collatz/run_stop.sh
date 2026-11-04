#!/bin/bash
# usage: run_stop.sh rule start end chunk  -> appends to logs/<rule>.stop
r=$1; s=$2; e=$3; c=$4; out=logs/$(basename $r .rule).stop
x=$s; while [ $x -lt $e ]; do y=$((x+c)); [ $y -gt $e ] && y=$e; nice -n 10 ./gstop $r $x $y >> $out; x=$y; done
