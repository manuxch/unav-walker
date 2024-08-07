#!/usr/bin/env python3

import argparse
import glob
import numpy as np
from scipy.stats import linregress
import matplotlib.pyplot as plt
plt.style.use('/home/manuel/granular/unav-walker/utils/figs.mplstyle')

parser = argparse.ArgumentParser(description="Cálculo de caudal de descarga en archivos de salida.")
parser.add_argument("-f", "--flux_file", help="Prefijo de archivos de entrada", required=True)
parser.add_argument("-m", "--mass",
            help="Masa de un grano (gramos) (=1: flujo de partículas). Default: valor experimental",
            type=float, default=1, required=False)
parser.add_argument("-t", "--t_factor",
            help="Escala de tiempo (sim -> exp [s]). Default: valor experimental",
            type=float, default=0.055328334, required=False)

args = parser.parse_args()
preFile = args.flux_file
m = args.mass
tf = args.t_factor

files = glob.glob(preFile + "*")
files.sort()
print("Archivos a analizar: ", len(files))

Q = []
calc_failed = 0
for fname in files:
    t, g = np.loadtxt(fname, usecols=(2, 4), unpack=True)
    t = t * tf
    g_max_fit = g.max() * 0.85
    g_min_fit = g.max() * 0.05
    dif_min = np.absolute(g-g_min_fit)
    idx_min = dif_min.argmin()
    dif_max = np.absolute(g-g_max_fit)
    idx_max = dif_max.argmin()
    m, i, r, pv, mstd = linregress(t[idx_min:idx_max], g[idx_min:idx_max])
    Q.append(m)
    print(f"{fname}: {m:.3f}")
    plt.plot(t, g, color='tab:gray', alpha=0.4)

units = '1/s'
y_label = 'Walkers out'
aQ = np.array(Q)
q_mean = aQ.mean()
q_std = aQ.std()
ax = plt.gca()
sout = f"Flow rate: ({q_mean:.3f} ± {q_std:.3f}) {units}"
sout += f"\nNo. of simulations: {len(files)}"
plt.text(0.55, 0.1, sout, horizontalalignment='left',
    verticalalignment='center', transform=ax.transAxes,
    fontsize='large')
plt.xlabel("t [s]")
plt.ylabel(y_label)
plt.tight_layout()
plt.savefig('flux-mean.pdf')
