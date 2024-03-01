#!/usr/bin/bash

#gamma=(0.040 0.069 0.098 0.127 0.156 0.184 0.213 0.242 0.271 0.3)
nd=30
gamma=()
#for i in $(seq 1 ${nd}); do
    #ga=$(echo "scale=2; 0.04 +($i - 1) * 0.01" | bc)
    #gamma+=($ga)
#done
dg=$(echo "scale=5; (3.24 - 0.1) / (${nd} - 1)" | bc)
for ((i=0; i<nd;i++)); do
    gamma[$i]=$(echo "scale=5; 0.1 + ($i *${dg})" | bc)
done
echo "${gamma[@]}"

cerotol=1.0e-4

tpl=params.tpl

for i in $(seq 0 $((nd - 1))); do
    printf -v c "%02g" $i
    g=${gamma[i]}
    #echo ${g}
    ifile=p-${c}.in
    sed -e "s/__Gamma__/${g}/g;s/__ceroTol__/${cerotol}/g" ${tpl} > ${ifile}
done
