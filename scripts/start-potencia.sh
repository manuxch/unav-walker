#!/usr/bin/bash
# Lanza la tanda de la validación de la potencia de contacto (SLURM, un job
# array con una simulación por línea de la tabla).
#
# Uso: start-potencia.sh [-n]      (-n: solo genera los p-<i>.in y submit.sh)
#
# Necesita en el directorio de trabajo: params-potencia.tpl,
# submit-potencia.tpl y el binario unav-walkers. Por cada caso genera
# p-<i>.in a partir de la plantilla.
#
# Parámetros del vídeo (propuesta 2026-12): Gamma = 9.9 / 9.81, rho = 0.495,
# phi = 1.79, mu_s = mu_d = 0.28; silo de 37.2 d (R = 18.6). La corrida
# puente (pot-090-p) usa la base de la tanda de producción PRL-energia:
# Gamma = 1.7, rho = 0.5, phi = 1.63, mu_s = 0.14, mu_d = 0.13.
#
# tMax es una cota provisional: la simulación termina sola tras
# periodos_registro períodos de registro. Ajustar con la corrida piloto.
#
# pre       prefijo de los archivos (pot-<D en décimas>-<f|m|p>)
# radio     semiancho del orificio, r = D / 2
# paredes   T = móviles, F = fijas
# N0, H     granos iniciales y altura del silo
casos='
# pre       radio  paredes N0    H    Gamma      rho   phi   mus  mud  tMax
pot-053-f   2.65   F       2500  80   1.0091743  0.495 1.79  0.28 0.28 30000
pot-053-m   2.65   T       2500  80   1.0091743  0.495 1.79  0.28 0.28 30000
pot-090-f   4.5    F       2500  80   1.0091743  0.495 1.79  0.28 0.28 20000
pot-090-m   4.5    T       2500  80   1.0091743  0.495 1.79  0.28 0.28 20000
pot-120-f   6.0    F       2500  80   1.0091743  0.495 1.79  0.28 0.28 20000
pot-120-m   6.0    T       2500  80   1.0091743  0.495 1.79  0.28 0.28 20000
pot-230-f   11.5   F       3300  105  1.0091743  0.495 1.79  0.28 0.28 20000
pot-230-m   11.5   T       3300  105  1.0091743  0.495 1.79  0.28 0.28 20000
pot-090-p   4.5    F       2500  80   1.7        0.5   1.63  0.14 0.13 20000
'

solo_generar=0
[[ ${1:-} == -n ]] && solo_generar=1

cont=0
while read -r pre radio paredes n0 h gamma rho phi mus mud tmax; do
  [[ -z $pre || $pre == \#* ]] && continue
  echo "${cont} ${pre} r=${radio} paredes=${paredes} N0=${n0} H=${h}"
  sed -e "s/__pre__/${pre}/g;s/__radio__/${radio}/g;s/__paredes__/${paredes}/g" \
    -e "s/__N0__/${n0}/g;s/__H__/${h}/g;s/__Gamma__/${gamma}/g;s/__rho__/${rho}/g" \
    -e "s/__phi__/${phi}/g;s/__mus__/${mus}/g;s/__mud__/${mud}/g;s/__tMax__/${tmax}/g" \
    params-potencia.tpl >"p-${cont}.in"
  cont=$((cont + 1))
done <<<"$casos"
cont=$((cont - 1))
sed -e "s/__N__/${cont}/g" submit-potencia.tpl >submit.sh
((solo_generar)) || sbatch submit.sh
