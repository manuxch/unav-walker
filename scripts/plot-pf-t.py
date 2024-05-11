#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt
import argparse
plt.style.use('/home/manuel/granular/unav-walker/utils/figs.mplstyle')

parser = argparse.ArgumentParser(description='Programa para graficar pf bulk y out en el tiempo.')
parser.add_argument('-f','--pfile',help='Input data file', required=True, action="store")
args =  parser.parse_args()
ifile = args.pfile
t, pf_bulk, pf_out = np.loadtxt(ifile, usecols=(0, 1, 2), unpack=True)

alfa = 0.7
fig, ax1 = plt.subplots()
color = 'tab:red'
ax1.set_xlabel(r'$t$ [s]')
ax1.set_ylabel(r'$\phi$')
ax1.plot(0.055328334 * t, pf_bulk, color=color, label='Bulk', alpha=1.0)
ax1.plot(0.055328334 * t, pf_out, color='tab:blue', label='Out', alpha=alfa)

fig.tight_layout()
plt.savefig('pf-t.pdf')
