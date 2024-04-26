#!/usr/bin/env python3

import argparse
import glob
import numpy as np
import flujo as fl
from scipy.stats import linregress
import matplotlib.pyplot as plt
# plt.style.use('/home/manuel/granular/unav-2024/utils/figs.mplstyle')

parser = argparse.ArgumentParser(description="Cálculo de caudal de descarga en archivos de salida.")
parser.add_argument("-f", "--flux_file", help="Prefijo de archivos de entrada", required=True)
parser.add_argument("-o", "--out", help="Nombre del archivo de resultados", required=True)
parser.add_argument("-m", "--mass",
            help="Masa de un grano (gramos) (=1: flujo de partículas). Default: valor experimental",
            type=float, default=81, required=False)
parser.add_argument("-t", "--t_factor",
            help="Escala de tiempo (sim -> exp [s]). Default: valor experimental",
            type=float, default=0.055328334, required=False)
parser.add_argument("-n", "--n_trials",
            help="Número de intentos MC (default: 5000).",
            type=int, default=5000, required=False)
parser.add_argument("-r", "--r2", help="Valor objetivo de R^2 (default: 0.9999).",
            type=float, default=0.9999, required=False)

args = parser.parse_args()
preFile = args.flux_file
oFile = args.out
m = args.mass
tf = args.t_factor
n_trials = args.n_trials
r2 = args.r2


files = glob.glob(preFile + "*")
files.sort()
print("Archivos a analizar: ", len(files))

radios = {'00': 0.6, '01': 0.786207, '02': 0.972414, '03': 1.15862, '04': 1.34483,
    '05': 1.53104, '06': 1.71724, '07': 1.90345, '08': 2.08966, '09': 2.27586,
    '10': 2.46207, '11': 2.64828, '12': 2.83448, '13': 3.02069, '14': 3.2069, '15': 3.39311,
    '16': 3.57931, '17': 3.76552, '18': 3.95173, '19': 4.13793,
    '20': 4.32414, '21': 4.51035, '22': 4.69655, '23': 4.88276,
    '24': 5.06897, '25': 5.25517, '26': 5.44138, '27': 5.62759,
    '28': 5.8138, '29': 6.00}

Q = {}
calc_failed = 0
rs = []
qr = []
for fname in files:
    f = fname.split("-")
    rlabel = f[1][:-4]
    # radio = 2 * radios[rlabel]
    with open(fname, 'r') as archivo:
        linea_1 = archivo.readline()
    radio = 2 * float(linea_1.split()[2])
    t, g = np.loadtxt(fname, usecols=(2, 4), unpack=True)
    t = t * 0.0553
    if t.size > n_trials:
        m, i, r, pv, mstd = linregress(t[:n_trials], g[:n_trials])
        print(f"lab: {rlabel}, r: {radio}, caudal: {m}, r: {r}")
        rs.append(radio)
        qr.append(m)
    else:
        m = 0
        print(f"lab: {rlabel}, r: {radio}, caudal: NaN, r: --")

    # res = fl.flujo_masico(t, g, m, tf, False, n_trials, r2)
    # if res:
        # print(f"lab: {rlabel} r: {radio}, caudal: ({res['caudal']:.3f} ± {res['cov_caudal']:.3f}) g")
        # if radio in Q:
            # Q[radio].append(res['caudal'])
        # else:
            # Q[radio] = [res['caudal']]
    # else:
        # print("No se puede calcular el flujo para " + fname)
        # calc_failed += 1

# print(f"No se pudo calcular el flujo en {calc_failed} casos ({calc_failed/len(files) * 100:.2f}%)")

fout = open(oFile, "w")
fout.write("# Radio Flujo_1 ... Flujo_n\n")
for r, q in zip(rs, qr):
    s = f"{r:.3f} {q:.3f}\n"
    fout.write(s)

plt.plot(rs, qr, 'o')
plt.xlabel("A")
plt.ylabel("Q (1/s)")
plt.show()

quit()

for a in sorted(Q.keys()):
    s = str(a)
    for f in Q[a]:
        s += " " + str(f)
    fout.write(str(s) + "\n")
