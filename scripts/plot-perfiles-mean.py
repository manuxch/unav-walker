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

fline = ''
f0 = files[0]
file = open(f0, 'r')
for _ in range(10):
    fline = file.readline().rstrip()
r_out = float(fline.split()[-1])

fig, ax1 = plt.subplots()
ax1.axvline(x=r_out, color='tab:gray')
ax1.axvline(x=-r_out, color='tab:gray')
ax1.set_xlabel(r'$x$ [d]')
ax1.set_ylabel(r'$\phi$', color='tab:red')
ax1.tick_params(axis='y', labelcolor='tab:red')
ax2 = ax1.twinx()
ax2.set_ylabel(r'$v_y$ [cm/s]', color='tab:blue')
ax2.tick_params(axis='y', labelcolor='tab:blue')
alfa_v = 0.2
alfa_pf = 0.2

v_scale = 0.542217668 * 100 # Paso a unidades experimentales y a cm/s
pf_scale = 0.9069 # escaleo por lo que corresponde a pf bidimensional
r, pf, v = np.loadtxt(files[0], usecols=(0, 1, 2), skiprows=3, unpack=True)

v = v * v_scale
pf = pf * pf_scale

for ifile in files[1:]:
    r_, pf_, v_ = np.loadtxt(ifile, usecols=(0, 1, 2), skiprows=3, unpack=True)
    v_ = v_ * v_scale
    pf += pf_ * pf_scale
    v += v_
    ax1.plot(r_, pf_scale * pf_, '.', color='tab:red', alpha=alfa_pf)
    ax2.plot(r_, v_, '.', color='tab:blue', alpha=alfa_v)

pf = pf / (len(files))
v = v / (len(files))
ax1.plot(r, pf, color='tab:red')
ax2.plot(r, v, color='tab:blue')
# tit = f"No. of simulations: {len(files)}"
# plt.legend(loc='best', title=tit)
stitle = rf'D = ${2 * r_out:.1f} d$'
fig.suptitle(stitle)
fig.tight_layout()
fname = f'perfiles-out-{2 * r_out:.1f}-D'
plt.savefig(fname + '.pdf')
with open(f'perfiles-out-{2 * r_out:.1f}-D.dat', 'w') as file:
    file.write("# r pf v_y\n")
    for i in range(r.size):
        file.write(f'{r[i]:.3f} {pf[i]:.3f} {100 * v[i]:.3f}\n')
file.close()
