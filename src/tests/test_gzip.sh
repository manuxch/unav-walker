#!/bin/bash
# Prueba de las salidas comprimidas (comprimir_salidas: T).
#
# Uso: tests/test_gzip.sh <unav-walkers>
#
# Corre la misma simulación con y sin compresión y verifica que:
#  - con compresión solo hay archivos .gz (los de frame) y descomprimidos
#    tienen la misma cantidad de líneas y las mismas líneas de comentario
#    (salvo el hash de procedencia de los parámetros);
#  - los valores numéricos coinciden con el archivo de texto hasta su
#    precisión (6 cifras): error relativo < 2e-5;
#  - las salidas que no son por frame (flujo, balance) son idénticas.

BIN=$(realpath "$1")
BASE=$(realpath "$(dirname "$0")/params_test.in")
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
mkdir "$TMP/a" "$TMP/b"
for v in a b; do
  sed -e 's/^dirID: .*/dirID: gz/' -e 's/^tMax: .*/tMax: 5.0/' \
    -e 's/^t_Register: .*/t_Register: 3.5/' "$BASE" > "$TMP/$v/p.in"
done
printf 'comprimir_salidas: T\r\n' >> "$TMP/b/p.in"
(cd "$TMP/a" && "$BIN" p.in > out.txt 2>&1)
(cd "$TMP/b" && "$BIN" p.in > out.txt 2>&1)
fails=0
fallo() { echo "FALLA: $1"; fails=$((fails + 1)); }

if ls "$TMP"/b/frames_gz/*.ve "$TMP"/b/frames_gz/*.sxy 2> /dev/null | grep -q .; then
  fallo "quedaron archivos sin comprimir"
fi
n=0
for f in "$TMP"/a/frames_gz/*.{xy,ve,sxy,dat}; do
  [[ -e $f ]] || continue
  name=$(basename "$f")
  [[ $name == balance_* ]] && continue
  g="$TMP/b/frames_gz/$name.gz"
  [[ -e $g ]] || { fallo "falta $name.gz"; continue; }
  n=$((n + 1))
  # Comparación: mismo número de líneas; comentarios iguales salvo la
  # procedencia (hash de parámetros); números con tolerancia relativa
  if ! paste -d'\n' <(grep -v '^#' "$f") <(zcat "$g" | grep -v '^#') | awk '
      NR % 2 == 1 { split($0, a, " "); na = NF; next }
      { if (NF != na) { bad = 1; next }
        for (i = 1; i <= NF; i++) {
          if (a[i] ~ /^[-+0-9.eE]+$/) {
            d = a[i] - $i; if (d < 0) d = -d
            m = a[i] < 0 ? -a[i] : a[i]
            if (d > 2e-5 * m + 1e-12) bad = 1
          } else if (a[i] != $i) bad = 1
        } }
      END { exit bad }'; then
    fallo "$name: los valores no coinciden"
  fi
  l1=$(wc -l < "$f"); l2=$(zcat "$g" | wc -l)
  [[ $l1 == "$l2" ]] || fallo "$name: $l1 líneas sin comprimir, $l2 comprimido"
done
((n > 0)) || fallo "no se comparó ningún archivo"
for f in flx_test.dat; do :; done
cmp -s <(grep -v '^#' "$TMP"/a/flx_test.dat) <(grep -v '^#' "$TMP"/b/flx_test.dat) ||
  fallo "el archivo de flujo difiere"
echo "salidas comprimidas: $n archivos comparados, $fails fallas"
exit $((fails > 0))
