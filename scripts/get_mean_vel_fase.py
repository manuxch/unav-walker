#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt
plt.style.use('/home/manuel/granular/walker-unav/utils/figs.mplstyle')
from scipy.optimize import curve_fit
import glob
from utiles import obtener_fase
from math import sqrt


def f_lineal(t, A, B):
    return A * t + B

datos_files = glob.glob('p-*.dat')

datos_files.sort()

skip_datos = 5000
omega = 125.66
v_med = []
fases = []

print("# fase v_med")
for i, f in enumerate(datos_files):
    fase = obtener_fase(f)
    if fase is None:
        print("Error en la lectura de fase.")
        quit()
    fases.append(fase)
    t, y, x = np.loadtxt(f, usecols=(0, 1, 2), unpack=True, skiprows=skip_datos)
    popt, pcov = curve_fit(f_lineal, t, x)
    A, B = popt
    v_med.append(A)
    s = f"{fase:.5f} {A:.5f}"
    print(s)

# gammas = np.array(gammas)
# vrms = sqrt(5/32) * 9.8 * gammas / omega  * 1000
# plt.plot(vrms, v_med, 'o')
# # plt.plot(gammas, v_med, 'o')
# plt.xlabel(r"$V_{\text{RMS}}$ (mm/s)")
# # plt.xlabel(r"$\Gamma$ (g)")
# plt.ylabel(r"$v_D$ (mm/s)")
# plt.tight_layout()
# plt.savefig('Fig_2_vmed.pdf')
