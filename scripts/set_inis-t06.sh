#!/usr/bin/bash

nd=30
gamma=2.00

phi=()
df=$(echo "scale=5; (6.283185307 - 0.0) / (${nd} - 1)" | bc)
for ((i=0; i<nd;i++)); do
    phi[$i]=$(echo "scale=5; 0.0 + ($i *${df})" | bc)
done
echo "${phi[@]}"

cerotol=1.0e-4

tpl=params.tpl

for i in $(seq 0 $((nd - 1))); do
    printf -v c "%02g" $i
    phii=${phi[i]}
    ifile=p-${c}.in
    sed -e "s/__Gamma__/${gamma}/g;s/__ceroTol__/${cerotol}/g;s/__phPhi__/${phii}/g" ${tpl} > ${ifile}
done
