#!/usr/bin/env bash

# Definir los límites y el número de valores deseados
inicio=0.60
fin=6.0
n_valores=30

# Calcular el espaciado entre los valores
paso=$(awk "BEGIN {print ($fin - $inicio) / ($n_valores - 1)}")

# Inicializar la lista vacía
lista=()

# Generar la lista de valores
for ((i = 0; i < $n_valores; i++)); do
    valor=$(awk "BEGIN {print $inicio + $i * $paso}")
    lista+=($valor)
done

# Imprimir la lista
#echo "${lista[@]}"

tpl=params.tpl
cont=0
for i in $(seq 0 $((n_valores-1))); do
    printf -v c "%02g" $i
    rO=${lista[i]}  
    echo ${c} ${rO}
    ifile=p-${cont}.in
    sed -e"s/__rOut__/${rO}/g;s/__C__/${c}/g" ${tpl} > ${ifile}
    let cont++
done
let cont--
sed -e "s/__N__/${cont}/g" submit.tpl > submit.sh
sbatch submit.sh
