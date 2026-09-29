#!/bin/bash
# Prueba de regresión: compara las salidas de dos versiones del simulador.
#
# Uso: tests/regression.sh <silo-vib_referencia> <silo-vib_nuevo>
#
# Corre ambos binarios con tres configuraciones basadas en
# tests/params_test.in (silo con orificio, fondo de medición y ROI) y compara
# todos los archivos de salida y la salida estándar, byte a byte. Antes de
# comparar se normalizan las líneas que dependen de la versión (hash de git,
# número y fecha de versión) y del momento de ejecución (fecha, hora y tiempo
# transcurrido).
#
# Sirve para verificar que un cambio que no debería alterar los resultados
# (formato, reorganización, documentación) efectivamente no los altera. La
# simulación es determinista con una semilla fija.

set -u
REF=$(realpath "$1")
NEW=$(realpath "$2")
BASE=$(realpath "$(dirname "$0")/params_test.in")
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

make_params() { # <nombre> <comando sed adicional>
  sed -e "s/^dirID: .*/dirID: $1/" -e "s/^fluxFile: .*/fluxFile: flx_$1.dat/" \
      -e "s/^pf_file: .*/pf_file: pf_$1.dat/" -e "$2" "$BASE" > "$TMP/$1.in"
}
make_params orif 's/^fondo_medicion: F/fondo_medicion: F/'
make_params fondo 's/^fondo_medicion: F/fondo_medicion: T/'
make_params roi 's/^save_roi_only: F/save_roi_only: T/;s/^x_roi: .*/x_roi: 2.5/;s/^y_min_roi: .*/y_min_roi: -1.0/;s/^y_max_roi: .*/y_max_roi: 8.0/'

run() { # <binario> <directorio>
  mkdir -p "$2" && cd "$2" || exit 1
  for c in orif fondo roi; do
    cp "$TMP/$c.in" .
    "$1" "$c.in" > "$c.stdout" 2>&1 &
  done
  wait
  find . -type f -exec sed -i -E \
    's/git: [^ ]+/git: X/; s/^# Fecha y hora.*//; s/^# Tiempo transcurrido.*//; s/^# silo-vib ver.*//; s/^# 20[0-9][0-9]\.[0-9.]+$//' {} +
}

(run "$REF" "$TMP/ref")
(run "$NEW" "$TMP/new")
n=$(find "$TMP/ref" -type f | wc -l)
if diff -rq "$TMP/ref" "$TMP/new" > "$TMP/diff.txt"; then
  echo "REGRESIÓN OK: $n archivos idénticos"
else
  echo "DIFERENCIAS ($(wc -l < "$TMP/diff.txt") archivos):"
  head -20 "$TMP/diff.txt"
  exit 1
fi
