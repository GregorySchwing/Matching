#!/bin/bash


C=10
for deg in 2 3 5 10
do
for i in {6..8}
do
for j in {5..5}
do
let SCALE=10**$i   # sets SCALE to 10Ei.
VERTICES=$(($j*$SCALE))
filenameKece="${VERTICES}_${deg}_REG_RAND.txt"
filenameMTX="${filenameKece}.mtx"
#python generate_random_graph.py $VERTICES $deg $filenameKece matrix_market
./generate ${VERTICES} ${deg} ${filenameMTX}
#../../src/matching $filenameKece
/home/greg/Matching/build/matcher $filenameMTX 1 1 0
/home/greg/Matching/build/matcher $filenameMTX 1 2 1
/home/greg/Matching/build/matcher $filenameMTX 1 4 1
/home/greg/Matching/build/matcher $filenameMTX 1 8 1
#/home/greg/mvm/src/cpu $filenameMTX
#/home/greg/mvm/src/a.out $filenameMTX
#/home/greg/mvm/src/a.out $filenameMTX 60 48
#rm $filename
done
done
done
