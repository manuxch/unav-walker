#!/usr/bin/env python3
# Script para analizar la velocidad de una sola partícula.
# Este script está hecho para validar los resultados experimentales
# de Tomás
# 2025.11.29

import matplotlib.pyplot as plt
import numpy as np
import glob
from scipy.stats import linregress
from math import sqrt
# plt.style.use('../../utils/figs.mplstyle')
# plt.style.use('/home/manuel/granular/unav-walker/utils/figs.mplstyle')

files = glob.glob('*.ve')
print(len(files))
files.sort()
x_s_2_e = 0.005
v_s_2_e = 0.221359436211787
t_s_2_e = 0.0225876975726313
ts = []
xs = []
ys = []
vels = []
# # Valores para obtener velocidad de drift
# n_start = 100
# n_end = 500
# Valores para ver la colisión
n_start = 00
# n_start = 4400
# n_end = 1000 
n_end = 4499
pid = 1239
# pid = 1853
# pid = 233
for f in files:
    fin = open(f, 'r')
    data = fin.readlines()
    fin.close()
    ts.append(float(data[0].split()[2]) * t_s_2_e)
    for fila in data[2:]:
        fila = fila.split()
        if int(fila[0]) == pid:
            xs.append(float(fila[2]) * x_s_2_e)
            ys.append(float(fila[3]) * x_s_2_e)
            vels.append(sqrt(float(fila[4])**2 + float(fila[5])**2) * v_s_2_e * 100)

ats = np.array(ts)
axs = np.array(xs)
ays = np.array(ys)
avels = np.array(vels)
# vmean = avels.mean()
vmean = avels[n_start:n_end].mean()
print(vmean, avels[n_start:n_end].std())
res = linregress(ats[n_start:n_end], ys[n_start:n_end])
vel = res.slope
print(f'fit vel: {vel:.3f}')
# plt.plot(ats, avels)
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8), sharex=True)

ax1.plot(ats[n_start:n_end], ays[n_start:n_end] * 100, label=fr'$v_y = {100 * vel:.3f}$ [cm/s] (fit)')
ax1.plot(ats[n_start:n_end], axs[n_start:n_end] * 100, label=fr'$v_y = {100 * vel:.3f}$ [cm/s] (fit)')
ax1.set_ylabel(r'$y$ [cm]')
ax1.legend()
ax2.plot(ats[n_start:n_end], avels[n_start:n_end], label=fr'$\langle v_y \rangle = {vmean:.3f}$ [cm/s]')
ax2.set_xlabel('$t$ [s]')
ax2.set_ylabel(r'$v_y$ [cm/s]')
ax2.legend()
# plt.title(r'$\mu_d = 0.13$')
plt.tight_layout()
plt.show()
# plt.savefig('vy-t.pdf')



