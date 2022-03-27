#!/bin/bash

PROG=../Debug/telem_radio
IN_FILE=/tmp/input.dat

num=$1
echo Showing Filter $num

$PROG --filter-test $num --print_filter_test_kernel > $IN_FILE
python plot.py $IN_FILE "Filter Input"> /dev/null 
rm $IN_FILE
