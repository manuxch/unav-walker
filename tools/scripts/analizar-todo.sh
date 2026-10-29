#!/usr/bin/env bash
# Análisis completo de una tanda de simulaciones empaquetadas con
# scripts/empaquetar.sh (un <pre>.tar.xz por simulación, p. ej. frm-03.tar.xz).
#
# Uso:
#   analizar-todo.sh [-f] [-s] [directorio]     (por defecto, el actual)
#
#   -f   rehace todo, aunque las salidas ya existan
#   -s   no grafica (solo extrae y calcula los perfiles)
#
# En el directorio de los paquetes:
#   datos/      los paquetes extraídos (p-<i>.in, frames_<dirID>/, flx...);
#               cada paquete se extrae una sola vez
#   analisis/   perfiles y figuras:
#     perfil-ve-<NN>.dat      vel_profile: vy vx w speed Eklin Ekrot
#     perfil-f-<NN>.dat       force_profile: norm tan fmag (sin paredes,
#                             solo contactos activos)
#     estres-D<NN>.dat        stress_profile, promedio y errores por bloques
#     estres-fase-D<NN>.dat   stress_profile resuelto en fase
#     perfiles-<qty>.pdf, perfiles-fuerzas.pdf, estres.pdf,
#     energia-D.pdf, energia-fase.pdf, energia-periodos.pdf, energia-D.dat
# con NN = D en diámetros, del sufijo de <pre>.
#
# Todos los perfiles usan la franja del orificio (|x| <= r_out, de las
# cabeceras), y en y van de 0 a altura_silo con bines de un diámetro. La
# altura y la frecuencia salen del archivo de parámetros de cada paquete.
#
# Es reanudable: un paso cuya salida ya existe no se repite (salvo con -f).
# No borra nada; los datos extraídos ocupan ~2.3 GB por simulación
# (registro de 30 períodos, frames cada 5 pasos).

set -euo pipefail

aqui=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
B=$(cd "$aqui/../../bin" && pwd)
S=$aqui

forzar=0
graficar=1
while getopts "fsh" op; do
  case $op in
  f) forzar=1 ;;
  s) graficar=0 ;;
  *)
    sed -n '2,/^$/s/^# \{0,1\}//p' "${BASH_SOURCE[0]}"
    exit 0
    ;;
  esac
done
shift $((OPTIND - 1))
base=$(cd "${1:-.}" && pwd)

for b in vel_profile force_profile stress_profile; do
  [[ -x $B/$b ]] || {
    echo "Falta $B/$b: compilar las herramientas (ver tools/README.md)" >&2
    exit 1
  }
done

# Valor de una clave del archivo de parámetros (tolera CRLF y comentarios)
param() {
  awk -v k="$2:" '$1 == k { print $2; exit }' "$1" | tr -d '\r'
}

# true si hay que (re)hacer la salida $1
hacer() { ((forzar)) || [[ ! -s $1 ]]; }

shopt -s nullglob
paquetes=("$base"/*.tar.xz)
shopt -u nullglob
if ((${#paquetes[@]} == 0)); then
  echo "No hay paquetes *.tar.xz en $base" >&2
  exit 1
fi

datos=$base/datos
out=$base/analisis
mkdir -p "$datos" "$out"
energias=()

for paquete in "${paquetes[@]}"; do
  pre=$(basename "$paquete" .tar.xz)
  if [[ ! $pre =~ -([0-9]+)$ ]]; then
    echo "$paquete: el nombre no termina en -<D>; se saltea" >&2
    continue
  fi
  nn=${BASH_REMATCH[1]}
  echo "=== $pre (D = $((10#$nn)) d)"

  # 1. Extracción (una sola vez; la marca se escribe al terminar bien)
  marca=$datos/.extraido-$pre
  if ((forzar)) || [[ ! -e $marca ]]; then
    echo "  extrayendo $(basename "$paquete") ..."
    tar -xJf "$paquete" -C "$datos"
    touch "$marca"
  fi

  # 2. Parámetros: el p-*.in del paquete con este preFrameFile
  pfile=""
  for f in "$datos"/p-*.in; do
    if [[ $(param "$f" preFrameFile) == "$pre" ]]; then
      pfile=$f
      break
    fi
  done
  if [[ -z $pfile ]]; then
    echo "  no hay un p-*.in con preFrameFile: $pre; se saltea" >&2
    continue
  fi
  frames=$datos/frames_$(param "$pfile" dirID)
  H=$(param "$pfile" altura_silo)
  frec=$(param "$pfile" Frecuencia_exitacion)
  r=$(param "$pfile" radio_out_silo)
  d_par=$(awk -v r="$r" 'BEGIN { printf "%d", int(2 * r + 0.5) }')
  if ((d_par != 10#$nn)); then
    echo "  aviso: $pre dice D = $((10#$nn)) y $(basename "$pfile") tiene" \
      "radio_out_silo = $r (D = $d_par)" >&2
  fi
  nb=$(awk -v h="$H" 'BEGIN { printf "%d", h + 0.5 }') # bines de un diámetro
  echo "  $(basename "$pfile"): H = $H, f = $frec, r = $r"

  # 3. Perfiles
  s=$out/perfil-ve-$nn.dat
  if hacer "$s"; then
    echo "  vel_profile -> $(basename "$s")"
    "$B/vel_profile" "$frames" "$pre" orificio "$nb" 0 "$H" "$s" \
      --qty vy vx w speed Eklin Ekrot >"$out/log-vel-$nn.txt" 2>&1
  fi
  s=$out/perfil-f-$nn.dat
  if hacer "$s"; then
    echo "  force_profile -> $(basename "$s")"
    "$B/force_profile" "$frames" "$pre" orificio "$nb" 0 "$H" "$s" \
      --qty norm tan fmag --no-walls --active-only \
      >"$out/log-fuerza-$nn.txt" 2>&1
  fi
  s=$out/estres-D$nn.dat
  if hacer "$s"; then
    echo "  stress_profile -> $(basename "$s")"
    "$B/stress_profile" "$frames" "$pre" -o "$s" --y-max "$H" \
      --freq "$frec" --phase-output "$out/estres-fase-D$nn.dat" \
      >"$out/log-estres-$nn.txt" 2>&1
  fi

  e=$frames/energy_$pre.dat
  if [[ -s $e ]]; then
    energias+=("$e")
  else
    echo "  aviso: falta $e (¿save_energy: F?)" >&2
  fi
done

((graficar)) || exit 0

# 4. Gráficos (en analisis/, unidades experimentales)
echo "=== Gráficos en $out"
cd "$out"
python3 "$S/plot-perfil.py"
python3 "$S/plot-fuerzas.py"
shopt -s nullglob
estres=(estres-D*.dat)
shopt -u nullglob
((${#estres[@]})) && python3 "$S/plot_stress_profile.py" "${estres[@]}" -o estres.pdf
((${#energias[@]})) && python3 "$S/plot-energia.py" "${energias[@]}"
echo "Listo."
