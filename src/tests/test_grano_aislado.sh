#!/bin/bash
# Prueba de grano_aislado: la deriva y la movilidad del modelo de Karnopp
# del simulador coinciden (dentro de 3 %) con las del modelo de Coulomb
# regularizado, para los parámetros del vídeo y los de producción.
#
# Uso: tests/test_grano_aislado.sh <grano_aislado>

BIN=$(realpath "$1")
fails=0
chequeo() { # <descripción> <opciones...>
  desc=$1; shift
  out=$("$BIN" --periodos 40 "$@")
  echo "$out" | awk -v d="$desc" '
    !/^#/ && $1 == 0 { vk = $2; vc = $3 }
    /^# tau_ef/ { tk = $(NF - 5); tc = $(NF - 3) }
    END {
      ev = (vk - vc) / vc; et = (tk - tc) / tc
      if (ev < 0) ev = -ev; if (et < 0) et = -et
      ok = (ev < 0.03 && et < 0.05 && vk < 0)
      printf "%-34s deriva %+.4f (Coulomb %+.4f)  tau %.3f (%.3f)  %s\n", d, vk, vc, tk, tc, ok ? "OK" : "FALLA"
      exit !ok
    }' || fails=$((fails + 1))
}
chequeo "parámetros del vídeo" --F -0.05,0,0.05
chequeo "parámetros de producción" --gamma 1.7 --rho 0.5 --phi 1.63 --mu_d 0.13 --mu_s 0.14 --F 0
exit $((fails > 0))
