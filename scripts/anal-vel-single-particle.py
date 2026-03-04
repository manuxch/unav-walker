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
x_s_2_e = 0.005 * 100
v_s_2_e = 0.221359436211787
t_s_2_e = 0.0225876975726313
ts = []
xs = []
ys = []
xs = []
vels = []
# # Valores para obtener velocidad de drift
# n_start = 100
# n_end = 500
# Valores para ver la colisión
n_start = 00
n_end = 7600
t_start = 223.78
t_end = 225
pid = 984
for f in files[15:]:
    fin = open(f, 'r')
    data = fin.readlines()
    fin.close()
    ts.append(float(data[0].split()[2]) * t_s_2_e)
    for fila in data[2:]:
        fila = fila.split()
        if int(fila[0]) == pid:
            xs.append(float(fila[2]) * x_s_2_e)
            ys.append(float(fila[3]) * x_s_2_e)
            # vels.append(sqrt(float(fila[4])**2 + float(fila[5])**2) * v_s_2_e * 100)
            vels.append(float(fila[5]) * v_s_2_e * 100)

ats = np.array(ts)
axs = np.array(xs)
ays = np.array(ys)
axs = np.array(xs)
avels = np.array(vels)
mask = (ats >= t_start) & (ats <= t_end)
vmean = avels[mask].mean()
vstd = avels[mask].std()
# vmean = avels[n_start:n_end].mean()
print("Análisis de velocidades")
print(f"t_start: {t_start} - t_end: {t_end}")
print(f"v_mean: {vmean}, std: {vstd}")
res = linregress(ats[mask], ays[mask])
# res = linregress(ats[n_start:n_end], ys[n_start:n_end])
vel = res.slope
print("Ajuste de posición vertical")
print(f'fit vel: {vel:.5f}')
# plt.plot(ats, avels)
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8), sharex=True)

ax1.plot(ats, ays, label=fr'$y$ cm')
ax1.plot(ats, axs, label=fr'$x$ cm')
ax1.plot(ats[mask], axs[mask], label=f"pid: {pid} - x")
ax1.plot(ats[mask], ays[mask], label=f"pid: {pid} - y")
ax1.axhline(y = 0, color='tab:red', alpha=0.5)
ax1.axvline(x = t_start, color='tab:blue', alpha=0.5)
ax1.axvline(x = t_end, color='tab:blue', alpha=0.5)
ax1.set_ylabel(r'$y$ [cm]')
ax1.legend(title=f"v_fit = {vel:.5f} cm/s")
ax2.plot(ats[n_start:n_end], avels[n_start:n_end], label=fr'$\langle v_y \rangle = {vmean:.3f}$ [cm/s]')
ax2.set_xlabel('$t$ [s]')
ax2.set_ylabel(r'$v_y$ [cm/s]')
ax2.legend()
# plt.title(r'$\mu_d = 0.13$')
plt.tight_layout()
plt.show()
# plt.savefig('vy-t.pdf')



