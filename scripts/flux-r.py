#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import glob
from scipy.stats import linregress
plt.style.use('/home/manuel/granular/unav-walker/utils/figs.mplstyle')

files = glob.glob('flx-*.dat')
files.sort()
print(len(files))
flux = {}
for file in files:
    fline = open(file).readline().rstrip()
    r = float(fline.split()[2])
    seed = int(file[-6:-4])
    if not (r in flux):
        flux[r] = []

thres = 10
for file in files:
    r = float(file.split('-')[1])
    t, q = np.loadtxt(file, usecols=(2, 3), unpack=True)
    t *= 0.055328334
    res = linregress(t[thres:], q[thres:])
    flux[r].append(res.slope)

d = []
Q = []
qmax = []
qmin = []
print('# D flux.mean flux.std flux.min flux.max')
for r, qs in flux.items():
    qsa = np.array(qs)
    print(r, qsa.mean(), qsa.std(), qsa.min(), qsa.max())
    d.append(r * 2)
    Q.append(qsa.mean())
    qmax.append(qsa.max() - qsa.mean())
    qmin.append(qsa.mean() - qsa.min())

error = [qmin, qmax]
plt.errorbar(d, Q, marker='.', yerr=error, capsize=4, linestyle='none')
plt.xlabel(r'$D$')
plt.ylabel(r'Fluxrate [1/s]')
plt.grid()
plt.tight_layout()
plt.savefig('Q-D.pdf')

