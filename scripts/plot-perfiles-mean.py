#!/usr/bin/env python3

import argparse
import glob
import numpy as np
from scipy.stats import linregress
import matplotlib.pyplot as plt
plt.style.use('/home/manuel/granular/unav-walker/utils/figs.mplstyle')

parser = argparse.ArgumentParser(description='Programa para graficar perfiles de pf y velocidad vertical de múltiples archivos.')
parser.add_argument("-f", "--in_file", help="Prefijo de archivos de entrada", required=True)

args = parser.parse_args()
preFile = args.in_file

files = glob.glob(preFile + "*")
files.sort()
print("Archivos a analizar: ", len(files))

fig, ax1 = plt.subplots()
ax1.axvline(x=2.5, color='tab:gray')
ax1.axvline(x=-2.5, color='tab:gray')
ax1.set_xlabel(r'$x$ [d]')
ax1.set_ylabel(r'$\phi$', color='tab:red')
ax1.tick_params(axis='y', labelcolor='tab:red')
ax2 = ax1.twinx()
ax2.set_ylabel(r'$v_y$ [cm/s]', color='tab:blue')
ax2.tick_params(axis='y', labelcolor='tab:blue')
alfa_v = 0.2
alfa_pf = 0.2

r, pf, v = np.loadtxt(files[0], usecols=(0, 1, 2), skiprows=3, unpack=True)

for ifile in files[1:]:
    r_, pf_, v_ = np.loadtxt(ifile, usecols=(0, 1, 2), skiprows=3, unpack=True)
    pf += pf_
    v += v_
    ax1.plot(r_, pf_, '.', color='tab:red', alpha=alfa_pf)
    ax2.plot(r_, 100*v_, '.', color='tab:blue', alpha=alfa_v)

pf = pf / (len(files))
v = v / (len(files))
ax1.plot(r, pf, color='tab:red')
ax2.plot(r, 100*v, color='tab:blue')
# tit = f"No. of simulations: {len(files)}"
# plt.legend(loc='best', title=tit)
fig.tight_layout()
plt.savefig('perfiles-out-multFiles.pdf')
