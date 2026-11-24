#!/usr/bin/bash
# Piloto de la validación de la potencia de contacto: D = 9 d, paredes
# fijas, parámetros del vídeo (caso pot-090-f de start-potencia.sh), con
# registro ralo, hasta que se descargan todos los granos.
#
# Uso: start-piloto.sh [-n]      (-n: solo genera p-0.in y submit.sh)
#
# Necesita en el directorio de trabajo: params-potencia.tpl,
# submit-potencia.tpl y unav-walkers.
#
# Qué mide: el caudal en función de los granos que quedan en el silo
# (flx-pot-090-f.dat), el avance del frente de granos aguas abajo (.ve cada
# período) y el trabajo de las paredes y la disipación por período
# (energy_*.dat con save_power). Con eso se fijan N0, tMax y el instante en
# que quedan 2000 granos para los 9 casos de producción.
#
# Respecto de la producción: el registro empieza en t_Register (después de
# la deposición) en lugar de con 2000 granos; sin .sxy, fc_ ni .fz ni
# mapas; .ve cada período (500 pasos); termina al descargar los 2500 granos.

# Parámetros del caso pot-090-f (los de start-potencia.sh)
pre=pot-090-f
sed -e "s/__pre__/${pre}/g;s/__radio__/4.5/g;s/__paredes__/F/g" \
  -e "s/__N0__/2500/g;s/__H__/80/g;s/__Gamma__/1.0091743/g;s/__rho__/0.495/g" \
  -e "s/__phi__/1.79/g;s/__mus__/0.28/g;s/__mud__/0.28/g;s/__tMax__/60000/g" \
  params-potencia.tpl |
  sed -e 's/^granos_registro: .*/granos_registro: 0\r/' \
    -e 's/^periodos_registro: .*/periodos_registro: 0\r/' \
    -e 's/^maxGranosDesc: [0-9]*/maxGranosDesc: 2500/' \
    -e 's/^save_tensors_freq: .*/save_tensors_freq: 0\r/' \
    -e 's/^freq_save_contacts: .*/freq_save_contacts: 0\r/' \
    -e 's/^save_ve_freq: .*/save_ve_freq: 500\r/' \
    -e 's/^save_fz_freq: .*/save_fz_freq: 0\r/' \
    -e 's/^save_power_maps: .*/save_power_maps: F\r/' \
    -e 's/^dirID: .*/dirID: piloto\r/' >p-0.in
sed -e "s/__N__/0/g" -e 's/unav-potencia/unav-piloto/' submit-potencia.tpl >submit.sh
echo "p-0.in generado (${pre}); submit.sh"
[[ ${1:-} == -n ]] || sbatch submit.sh
