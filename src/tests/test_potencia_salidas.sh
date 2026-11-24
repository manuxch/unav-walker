#!/bin/bash
# Prueba de las salidas de potencia (.fz, potencia_celdas, potencia_pares).
#
# Uso: tests/test_potencia_salidas.sh <unav-walkers>
#
# Corre un silo chico (H = 8, R = 3, 25 granos, Gamma = 3) con paredes móviles,
# save_power, .fz cada 5 pasos y mapas, comprimido, y verifica que:
#  - existen los archivos y las columnas son las esperadas;
#  - en cada grano de cada .fz y en cada celda, la potencia de los contactos
#    es T_in - D + P_par (lo que recibe de otros granos, menos la disipación,
#    más el trabajo de las paredes: identidad exacta por grano), y la suma de
#    T_in sobre todos los granos es nula;
#  - la disipación y el trabajo de las paredes de las celdas y del archivo de
#    pares son iguales (se acumulan a partir de los mismos contactos);
#  - en las celdas hay 25 granos en total (promedio sobre las fases);
#  - los totales coinciden con los de energy_*.dat dentro de 15 % (ventanas
#    de promedio distintas: períodos completos frente a todos los pasos).

BIN=$(realpath "$1")
BASE=$(realpath "$(dirname "$0")/params_test.in")
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cd "$TMP" || exit 1
sed -e 's/^dirID: .*/dirID: pw/' -e 's/^tMax: .*/tMax: 45.0/' \
  -e 's/^altura_silo: .*/altura_silo: 8/' -e 's/^radio_silo: .*/radio_silo: 3/' \
  -e 's/^radio_out_silo: .*/radio_out_silo: 1.5/' \
  -e 's/^Amplitud_exitacion_gamma: .*/Amplitud_exitacion_gamma: 3.0/' \
  -e 's/^250 0.5/25 0.5/' -e 's/^t_Register: .*/t_Register: 15/' \
  -e 's/^saveFrameFreq: .*/saveFrameFreq: 0/' -e 's/^save_tensors_freq: .*/save_tensors_freq: 0/' \
  -e 's/^freq_save_contacts: .*/freq_save_contacts: 0/' -e 's/^check_balance_freq: .*/check_balance_freq: 0/' \
  -e 's/^save_ve_freq: .*/save_ve_freq: 0/' -e 's/^packing_fraction_out_freq: .*/packing_fraction_out_freq: 0/' \
  -e 's/^freq_perfiles: .*/freq_perfiles: 0/' "$BASE" > p.in
printf 'save_energy: T\r\nsave_power: T\r\nsave_fz_freq: 5\r\nsave_power_maps: T\r\nparedes_moviles: T\r\ncomprimir_salidas: T\r\nreg_cuna_h: 3\r\nceldas_x_max: 10\r\nceldas_y_min: -10\r\n' >> p.in
"$BIN" p.in > out.txt 2>&1
fails=0
fallo() { echo "FALLA: $1"; fails=$((fails + 1)); }
D=frames_pw
[[ -s $D/potencia_celdas_frm.dat.gz && -s $D/potencia_pares_frm.dat.gz ]] || fallo "faltan los mapas"
nfz=$(ls $D/frm_*.fz.gz 2> /dev/null | wc -l)
((nfz > 100)) || fallo "pocos .fz ($nfz)"

# Identidad por grano en los .fz (columnas 17-19: P_tr_gg P_tr_gw P_rot;
# 22: P_par; 23: T_in; 24-25: D_gg D_gw): P = T_in - D + P_par
bad=$(for f in $D/frm_00060*.fz.gz $D/frm_00300*.fz.gz; do zcat "$f"; done |
  awk '!/^#/ { if (NF != 25) { b++; next }
       d = $17 + $18 + $19 - $23 + $24 + $25 - $22; if (d < 0) d = -d
       s = ($17 < 0 ? -$17 : $17) + ($18 < 0 ? -$18 : $18) + ($19 < 0 ? -$19 : $19) + ($23 < 0 ? -$23 : $23) + $24 + $25
       if (d > 1e-6 * s + 1e-12) b++ } END { print b + 0 }')
((bad == 0)) || fallo ".fz: $bad granos no cumplen P = T_in - D + P_par"

# Celdas: ib fase2 ix iy xc yc n(7) vx vy w Cgg(2) Cgw(2) tau Fb(2) taub
# P_tr_gg(19) P_tr_gw(20) P_rot(21) P_lin P_piv P_par(24) T_in(25) D_gg(26)
# D_gw(27)
read -r nb < <(zcat $D/potencia_celdas_frm.dat.gz | awk '!/^#/ { if ($1 + 1 > n) n = $1 + 1 } END { print n }')
res=$(zcat $D/potencia_celdas_frm.dat.gz | awk -v nb="$nb" '!/^#/ {
    n += $7; p += $19 + $20 + $21; d += $26 + $27; w += $24; ti += $25
    e = $19 + $20 + $21 - $25 + $26 + $27 - $24; if (e < 0) e = -e
    s = $26 + $27 + ($24 < 0 ? -$24 : $24) + ($25 < 0 ? -$25 : $25)
    if (e > 1e-6 * s + 1e-12) bad++ }
  END { printf "%d %.8e %.8e %.8e %.6f %.8e\n", bad + 0, p / nb, d / nb, w / nb, n / nb, ti / nb }')
read -r bad_c p_c d_c w_c n_c ti_c <<< "$res"
awk -v a="$ti_c" -v d="$d_c" 'BEGIN { exit (a > 1e-6 * d || -a > 1e-6 * d) }' || fallo "la suma de T_in no es nula ($ti_c)"
((bad_c == 0)) || fallo "celdas: $bad_c celdas no cumplen P = T_in - D + P_par"
awk -v n="$n_c" 'BEGIN { exit (n < 24.99 || n > 25.01) }' || fallo "granos en las celdas: $n_c (esperado 25)"
read -r d_p w_p < <(zcat $D/potencia_pares_frm.dat.gz | awk -v nb="$nb" '!/^#/ { d += $6; w += $7 } END { printf "%.8e %.8e\n", d / nb, w / nb }')
awk -v a="$d_c" -v b="$d_p" 'BEGIN { exit (a - b > 1e-6 * a || b - a > 1e-6 * a) }' || fallo "D de celdas ($d_c) y de pares ($d_p) difieren"
awk -v a="$w_c" -v b="$w_p" 'BEGIN { exit (a - b > 1e-6 * a || b - a > 1e-6 * a) }' || fallo "W_par de celdas ($w_c) y de pares ($w_p) difieren"

# Totales frente al archivo de energía (por unidad de tiempo)
read -r d_e w_e < <(grep -v '^#' $D/energy_frm.dat | awk '{ if ($4 > m) m = $4; np[NR] = $4; dd[NR] = $17 + $18; ww[NR] = $16 }
  END { for (i = 1; i <= NR; i++) if (np[i] >= 0.95 * m) { c++; D += dd[i]; W += ww[i] }
        T = 1 / 0.45175395; printf "%.8e %.8e\n", D / c / T, W / c / T }')
awk -v a="$d_c" -v b="$d_e" 'BEGIN { exit (a - b > 0.15 * b || b - a > 0.15 * b) }' || fallo "D de celdas ($d_c) y de energy ($d_e) difieren más de 15 %"
awk -v a="$w_c" -v b="$w_e" 'BEGIN { exit (a - b > 0.15 * b || b - a > 0.15 * b) }' || fallo "W_par de celdas ($w_c) y de energy ($w_e) difieren más de 15 %"
echo "salidas de potencia: $nfz .fz, $nb bines, D = $d_c, W_par = $w_c, $fails fallas"
exit $((fails > 0))
