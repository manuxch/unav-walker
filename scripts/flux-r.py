#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import glob
from scipy.stats import linregress
plt.style.use('/home/manuel/granular/unav-walker/utils/figs.mplstyle')

files = glob.glob('*/flx-*.dat')
files.sort()
flux_L = {}
flux_S = {}
for file in files:
    direc, fname = file.split('/')
    fline = open(file).readline().rstrip()
    r = float(fline.split()[2])
    seed = int(fname[-6:-4])
    if not (r in flux_L):
        flux_L[r] = []
        flux_S[r] = []

thres = 0
for dir_file in files:
    direc, file = dir_file.split('/')
    r = float(file.split('-')[1])
    t, qL, qS = np.loadtxt(dir_file, usecols=(2, 3, 4), unpack=True)
    t *= 0.055328334
    resL = linregress(t[thres:], qL[thres:])
    resS = linregress(t[thres:], qS[thres:])
    flux_L[r].append(resL.slope)
    flux_S[r].append(resS.slope)

d = []
QL = []
QS = []
qmaxL = []
qminL = []
qmaxS = []
qminS = []
print('# D flux_H.mean flux_H.std flux_H.min flux_H.max flux_L.mean flux_L.mean flux_L.min flux_L.max')
# print('# D flux_L.mean flux_L.std flux_L.min flux_L.max flux_S.mean flux_S.mean flux_S.min flux_S.max')
for r in flux_L.keys():
    qsLa = np.array(flux_L[r])
    qsSa = np.array(flux_S[r])
    dO = 2 * r
    d.append(dO)
    QL.append(qsLa.mean())
    qmaxL.append(qsLa.max())
    qminL.append(qsLa.min())
    QS.append(qsSa.mean())
    qmaxS.append(qsLa.max())
    qminS.append(qsLa.min())
    sout = f"{dO:.1f} "
    sout += f"{qsLa.mean():.3f} {qsLa.std():.3f} {qsLa.min():.3f} {qsLa.max():.3f} "
    sout += f"{qsSa.mean():.3f} {qsSa.std():.3f} {qsSa.min():.3f} {qsSa.max():.3f} "
    print(sout)

error_L = [np.array(QL) - np.array(qminL), np.array(qmaxL) - np.array(QL)]
error_S = [np.array(QS) - np.array(qminS), np.array(qmaxS) - np.array(QS)]
alfa = 0.7
plt.errorbar(d, QL, marker='.', yerr=error_L, capsize=4, linestyle='none', label=r'$\mu_s = 0.19$', alpha=alfa)
plt.errorbar(d, QS, marker='.', yerr=error_L, capsize=4, linestyle='none', label=r'$\mu_s = 0.13$', alpha=alfa)
# plt.errorbar(d, QL, marker='.', yerr=error_L, capsize=4, linestyle='none', label=r'$d_L = 1.0$', alpha=alfa)
# plt.errorbar(d, QS, marker='.', yerr=error_L, capsize=4, linestyle='none', label=r'$d_S = 0.8$', alpha=alfa)
plt.xlabel(r'$D$')
plt.ylabel(r'Fluxrate [1/s]')
plt.grid()
plt.legend()
plt.tight_layout()
plt.savefig('Q-D.pdf')

