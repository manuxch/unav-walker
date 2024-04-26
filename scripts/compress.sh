#!/usr/bin/bash

#for i in $(seq 0 19); do
for i in $(seq 0 9); do
    printf -v c "%02g" $i
    wdir=C-${c}
    cd ${wdir}
    7z a flx-${c}.7z flx_${c}_*.dat
    rm flx_${c}_*.dat
    7z a fms-${c}.7z fms_*.out
    rm fms_*.out
    7z a params-${c}.7z p-*.in
    rm p-*.in
    cd frames_mix-${c}
    7z a frames-${c}.7z *.xy
    rm *.xy
    7z a q6-ve-${c}.7z *.dat
    rm *.dat
    cd ../..
done
