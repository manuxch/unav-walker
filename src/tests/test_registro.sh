#!/bin/bash
# Prueba de la eliminación de granos y del comienzo del registro por llenado.
#
# Uso: tests/test_registro.sh <unav-walkers>
#
# Corre, en un silo chico (H = 8, R = 3, 25 granos, Gamma = 3) donde la
# descarga es rápida, una descarga sin reinyección con y_eliminacion = -4,
# x_eliminacion = 2,
# granos_registro = 20 y periodos_registro = 2, y verifica que:
#  - el registro comienza al inicio de un período con <= 20 granos en el silo;
#  - la simulación termina tras 2 períodos registrados y el balance de energía
#    tiene exactamente esos períodos completos;
#  - ningún grano descargado queda por debajo de y = -4 ni más allá de |x| = 2.

BIN=$(realpath "$1")
BASE=$(realpath "$(dirname "$0")/params_test.in")
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cd "$TMP" || exit 1
sed -e 's/^dirID: .*/dirID: reg/' -e 's/^tMax: .*/tMax: 150.0/' \
  -e 's/^altura_silo: .*/altura_silo: 8/' -e 's/^radio_silo: .*/radio_silo: 3/' \
  -e 's/^radio_out_silo: .*/radio_out_silo: 1.5/' \
  -e 's/^Amplitud_exitacion_gamma: .*/Amplitud_exitacion_gamma: 3.0/' \
  -e 's/^250 0.5/25 0.5/' -e 's/^t_Register: .*/t_Register: 3.5/' \
  -e 's/^do_reinyection: .*/do_reinyection: F/' \
  -e 's/^saveFrameFreq: .*/saveFrameFreq: 0/' -e 's/^save_tensors_freq: .*/save_tensors_freq: 0/' \
  -e 's/^freq_save_contacts: .*/freq_save_contacts: 0/' -e 's/^check_balance_freq: .*/check_balance_freq: 0/' \
  -e 's/^save_ve_freq: .*/save_ve_freq: 25/' -e 's/^packing_fraction_out_freq: .*/packing_fraction_out_freq: 0/' \
  -e 's/^freq_perfiles: .*/freq_perfiles: 0/' "$BASE" > p.in
printf 'save_energy: T\r\ny_eliminacion: -4\r\nx_eliminacion: 2\r\ngranos_registro: 20\r\nperiodos_registro: 2\r\n' >> p.in
"$BIN" p.in > out.txt 2>&1
fails=0
fallo() { echo "FALLA: $1"; fails=$((fails + 1)); }

linea=$(grep -m1 "Comienzo del registro" out.txt)
[[ -n $linea ]] || fallo "no hubo comienzo de registro"
en_silo=$(sed -E 's/.*, ([0-9]+) granos en el silo.*/\1/' <<< "$linea")
((en_silo <= 20)) || fallo "granos en el silo al comenzar: $en_silo > 20"
grep -q "2 períodos registrados" out.txt || fallo "no terminó por períodos de registro"
# Períodos completos en el balance de energía
n_per=$(grep -vc '^#' frames_reg/energy_frm.dat)
((n_per == 2)) || fallo "el balance tiene $n_per períodos"
# Primer período: comienza en el primer paso con f t >= entero (dt f = 0.00226)
t0=$(grep -v '^#' frames_reg/energy_frm.dat | head -n 1 | awk '{print $2}')
awk -v t="$t0" 'BEGIN { x = t * 0.45175395; d = x - int(x + 0.5); if (d < 0) d = -d; exit d > 0.0025 }' ||
  fallo "el registro no empieza al inicio de un período (t_ini = $t0)"
# Ningún grano descargado más allá de los límites
awk 'FNR > 0 && !/^#/ && !/^ #/ { if ($4 < -4.05) bad = 1; if ($4 < -0.6 && ($3 > 2.05 || $3 < -2.05)) bad = 1 }
     END { exit bad }' frames_reg/frm_*.ve || fallo "hay granos descargados fuera de la zona"
echo "registro por llenado: comienzo con $en_silo granos, $n_per períodos, $fails fallas"
exit $((fails > 0))
