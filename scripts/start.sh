#!/usr/bin/bash

cont=0

for radio in 1.5 2.0 2.5 3.0 3.5 4.0 4.5 5.0 5.5 6.0 6.5 7.0; do
  label=$(awk -v r="$radio" 'BEGIN { printf "%02d", int(2 * r + 0.5) }') # D = 2 r, dos dígitos
  echo ${cont} ${radio} ${label}
  inifile=p-${cont}.in
  sed -e "s/__radio__/${radio}/g;s/__radioS__/${label}/g" params.tpl >${inifile}
  let cont++
done
let cont--
sed -e "s/__N__/${cont}/g" submit.tpl >submit.sh
sbatch submit.sh
