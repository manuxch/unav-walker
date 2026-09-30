#!/usr/bin/env bash
# Empaqueta las salidas de cada simulación en un .tar.xz, uno por archivo de
# parámetros. Correr en el directorio de las corridas, cuando terminaron
# todas.
#
# Uso:
#   empaquetar.sh [p-0.in p-1.in ...]      (por defecto, todos los p-*.in)
#
# Por cada archivo de parámetros lee dirID, preFrameFile, fluxFile y pf_file
# y arma <preFrameFile>.tar.xz (p. ej. frm-03.tar.xz) con:
#   - el archivo de parámetros;
#   - frames_<dirID>/<pre>_*       (.xy, .sxy, .ve)
#   - frames_<dirID>/fc_<pre>_*    (contactos)
#   - frames_<dirID>/wall_force_<pre>.dat, balance_<pre>.dat (si existen)
#   - fluxFile y pf_file (si existen);
#   - la salida estándar de SLURM flx_<job>_<i>.out, con i el índice de
#     p-<i>.in (si existe).
# Después de crear cada paquete verifica su integridad y que tenga todos los
# archivos. No borra nada.
#
# Variables de entorno:
#   XZ_OPT   opciones de xz (por defecto -6 -T0: nivel 6, todos los núcleos)

set -euo pipefail

export XZ_OPT=${XZ_OPT:--6 -T0}

# Valor de una clave del archivo de parámetros (tolera CRLF y comentarios)
param() {
  awk -v k="$2:" '$1 == k { print $2; exit }' "$1" | tr -d '\r'
}

if (($# == 0)); then
  shopt -s nullglob
  set -- p-*.in
  shopt -u nullglob
  if (($# == 0)); then
    echo "No hay archivos p-*.in en $(pwd)" >&2
    exit 1
  fi
fi

lista=$(mktemp)
trap 'rm -f "$lista"' EXIT

errores=0
for pfile in "$@"; do
  dir_id=$(param "$pfile" dirID)
  pre=$(param "$pfile" preFrameFile)
  flux=$(param "$pfile" fluxFile)
  pf=$(param "$pfile" pf_file)
  if [[ -z $dir_id || -z $pre ]]; then
    echo "$pfile: faltan dirID o preFrameFile; se saltea" >&2
    errores=$((errores + 1))
    continue
  fi
  frames=frames_$dir_id
  paquete=$pre.tar.xz
  if [[ -e $paquete ]]; then
    echo "$paquete ya existe; se saltea (borrarlo para rehacerlo)" >&2
    continue
  fi

  # Lista de archivos (con find: son decenas de miles, no entran en argv)
  printf '%s\n' "$pfile" >"$lista"
  find "$frames" -maxdepth 1 -type f \
    \( -name "${pre}_*" -o -name "fc_${pre}_*" \
    -o -name "wall_force_${pre}.dat" -o -name "balance_${pre}.dat" \) |
    sort >>"$lista"
  n_frames=$(($(wc -l <"$lista") - 1))
  if ((n_frames == 0)); then
    echo "$pfile: no hay salidas de $pre en $frames; se saltea" >&2
    errores=$((errores + 1))
    continue
  fi
  for f in "$flux" "$pf"; do
    [[ -n $f && -f $f ]] && printf '%s\n' "$f" >>"$lista"
  done
  if [[ $pfile =~ ^p-([0-9]+)\.in$ ]]; then
    for f in flx_*_"${BASH_REMATCH[1]}".out; do
      [[ -f $f ]] && printf '%s\n' "$f" >>"$lista"
    done
  fi

  n=$(wc -l <"$lista")

  echo "$paquete: $n archivos"
  tar -cJf "$paquete.tmp" -T "$lista"
  # Verificación: integridad de xz y cantidad de archivos
  xz -t "$paquete.tmp"
  m=$(tar -tJf "$paquete.tmp" | wc -l)
  if ((m != n)); then
    echo "$paquete: el paquete tiene $m archivos y se esperaban $n" >&2
    errores=$((errores + 1))
    continue
  fi
  mv "$paquete.tmp" "$paquete"
  echo "$paquete: $(du -h "$paquete" | cut -f1), verificado"
done

if ((errores > 0)); then
  echo "Terminó con $errores problema(s)." >&2
  exit 1
fi
