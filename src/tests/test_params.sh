#!/bin/bash
# Pruebas del lector de parámetros (global_setup.cpp).
#
# Uso: tests/test_params.sh <silo-vib>
#
# Cada caso modifica tests/params_test.in y verifica que el programa termine
# con el mensaje de error esperado. El último caso verifica que el archivo
# sin modificar se acepta y la simulación corre.

BIN=$(realpath "$1")
BASE=$(realpath "$(dirname "$0")/params_test.in")
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cd "$TMP" || exit 1
fails=0

# caso <descripción> <mensaje esperado> <comando sed sobre el archivo base>
caso() {
  sed -e "$3" "$BASE" > p.in
  out=$(timeout 10 "$BIN" p.in 2>&1 | grep -m1 "ERROR")
  if [[ "$out" == *"$2"* ]]; then
    printf "%-38s OK\n" "$1"
  else
    printf "%-38s FALLA (salida: %s)\n" "$1" "$out"
    fails=$((fails + 1))
  fi
}

caso "falta un parámetro obligatorio" "falta el parámetro obligatorio 'friccion_silo:'" '/^friccion_silo:/d'
caso "clave desconocida" "clave desconocida 'tMaxx:'" 's/^tMax:/tMaxx:/'
caso "clave repetida" "clave repetida 'g:'" '$a g: 2.0'
caso "valor no numérico" "valor no numérico para g:" 's/^g: 1.0/g: 1.0x/'
caso "valor lógico inválido" "valor lógico inválido para do_reinyection:" 's/^do_reinyection: T/do_reinyection: yes/'
caso "polígonos no admitidos" "solo se admiten discos" 's/^250 0.5 1 /250 0.5 3 /'
caso "línea de grano incompleta" "línea de tipo de grano 1 mal formada" 's/^250 0.5 1 .*/250 0.5 1 1.27/'
caso "valor fuera de rango" "radio_out_silo debe cumplir" 's/^radio_out_silo: .*/radio_out_silo: 20/'
caso "ROI incompleto" "se requieren x_roi" 's/^save_roi_only: F/save_roi_only: T/;/^x_roi:/d'
caso "parámetro obsoleto" "atenuacion_rotacional ya no se usa" '$a atenuacion_rotacional: 0.95'

# El archivo base se acepta y una corrida corta termina bien.
sed -e 's/^tMax: .*/tMax: 0.05/;s/^tBlock: .*/tBlock: 0.02/;s/^t_Register: .*/t_Register: 0.0/' "$BASE" > ok.in
if timeout 60 "$BIN" ok.in 2>&1 | grep -q "# Simulación finalizada."; then
  printf "%-38s OK\n" "archivo válido: la simulación corre"
else
  printf "%-38s FALLA\n" "archivo válido: la simulación corre"
  fails=$((fails + 1))
fi

if ((fails)); then echo "HAY FALLAS"; exit 1; fi
echo "Todas las pruebas pasan"
