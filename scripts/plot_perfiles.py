#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt
import argparse

plt.style.use('/home/manuel/granular/unav-walker/utils/figs.mplstyle')

parser = argparse.ArgumentParser(description='Programa para graficar perfiles de pf y velocidad vertical.')
parser.add_argument('-f','--pfile',help='Input data file', required=True, action="store")
args =  parser.parse_args()
ifile = args.pfile

r, pf, v = np.loadtxt(ifile, usecols=(0, 1, 2), skiprows=3, unpack=True)
print(r.size)

alfa = 0.5
fig, ax1 = plt.subplots()
color = 'tab:red'
ax1.axvline(x=2.5, color='tab:gray')
ax1.axvline(x=-2.5, color='tab:gray')
ax1.set_xlabel(r'$x$ [d]')
ax1.set_ylabel(r'$\phi$', color=color)
ax1.plot(r, pf, '-o', color=color, alpha=alfa)
ax1.tick_params(axis='y', labelcolor=color)

ax2 = ax1.twinx()
color = 'tab:blue'
ax2.set_ylabel(r'$v_y$ [cm/s]', color=color)
ax2.plot(r, 100*v, '-x', color=color, alpha=alfa)
ax2.tick_params(axis='y', labelcolor=color)

fig.tight_layout()
plt.savefig('perfiles.pdf')
