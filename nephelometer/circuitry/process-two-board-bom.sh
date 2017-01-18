#!/bin/bash
sed 's/^"/"DET-/' < band-dsp-teensy.csv | sed 's/"DET-Part/"Part/' > band-dsp-wprefix.csv
sed 's/^"/"MOT-/' < band-motor-teensy.csv | sed 's/"MOT-Part/"Part/' > band-motor-wprefix.csv
runhaskell FixupBom.hs band-both-teensy.csv band-dsp-wprefix.csv band-motor-wprefix.csv
awk -F$'\t' '{ OFS="\t"; $1 = $1 * 3; print $0 }' < band-both-teensy-order.txt > band-both-teensy-order-x3.txt
