#!/bin/bash

PROG=../Debug/telem_radio
IN_FILE=/tmp/input.dat
OUT_FILE=/tmp/output.dat

num=$1
echo Running Filter $num

$PROG --filter-test $num --print-filter-test-input > $IN_FILE
$PROG --filter-test $num > $OUT_FILE
python plot.py $IN_FILE "Filter Input"> /dev/null &
python plot.py $OUT_FILE "Filter Output" 800 > /dev/null
rm $IN_FILE
rm $OUT_FILE
