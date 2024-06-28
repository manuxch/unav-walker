#!/usr/bin/env python3

import argparse
import glob
import numpy as np
from scipy.stats import linregress
import matplotlib.pyplot as plt
plt.style.use('/home/manuel/granular/unav-walker/utils/figs.mplstyle')

parser = argparse.ArgumentParser(description="Gráfico del pf bulk y out en múltiples archivos.")
parser.add_argument("-f", "--in_file", help="Prefijo de archivos de entrada", required=True)
parser.add_argument("-t", "--t_factor",
            help="Escala de tiempo (sim -> exp [s]). Default: valor experimental",
            type=float, default=0.055328334, required=False)

args = parser.parse_args()
preFile = args.in_file
tf = args.t_factor
pf_max_2d = 0.9069 # = pi / (2 sqrt(3) ) valor ideal de Wikipedia

files = glob.glob(preFile + "*")
files.sort()
print("Archivos a analizar: ", len(files))

alfa_b = 0.4
alfa_o = 0.2
fig, ax1 = plt.subplots()
for ifile in files:
    t, pf_bulk, pf_out = np.loadtxt(ifile, usecols=(0, 1, 2), unpack=True)
    t = t * tf
    pf_out = pf_out * pf_max_2d
    ax1.plot(t, pf_out, color='tab:blue', alpha=alfa_o)
    ax1.plot(t, pf_bulk, color='tab:red', alpha=alfa_b)

ax1.plot(np.NaN, np.NaN, color='tab:blue', label='Out')
ax1.plot(np.NaN, np.NaN, color='tab:red', label='Bulk')
tit = f"No. of simulations: {len(files)}"
plt.legend(loc='best', title=tit)
ax1.set_xlabel(r'$t$ [s]')
ax1.set_ylabel(r'$\phi$')
plt.grid(visible=True, which='both')
fig.tight_layout()
plt.savefig('pf-t-multFiles.pdf')
