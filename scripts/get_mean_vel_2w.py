#!/usr/bin/env python3

import argparse
import numpy as np
# import matplotlib.pyplot as plt
# plt.style.use('/home/manuel/granular/walker-unav/utils/figs.mplstyle')
from scipy.optimize import curve_fit
# import glob
# from utiles import obtener_fase
from math import sqrt


def f_lineal(t, A, B):
    return A * t + B


parser = argparse.ArgumentParser()
parser.add_argument("file", metavar='N', type=str)
args = parser.parse_args()
print(args)
ifile = args.file

skip_datos = 5000
omega = 125.66
v_med = []
fases = []

t, x0, x1 = np.loadtxt(ifile, unpack=True, skiprows=skip_datos)
popt0, _ = curve_fit(f_lineal, t, x0)
A, B = popt0
v_med.append(A)
popt1, _ = curve_fit(f_lineal, t, x1)
A, B = popt1
v_med.append(A)

print(v_med)

# print("# gamma v_med")
# for i, f in enumerate(datos_files):
    # fase = obtener_fase(f)
    # if not fase:
        # print("Error en la lectura de fase.")
        # quit()
    # fases.append(fase)
    # t, y, x = np.loadtxt(f, usecols=(0, 1, 2), unpack=True, skiprows=skip_datos)
    # # t = t[-n_dat:]º
    # # x = x[-n_dat:]
    # popt, pcov = curve_fit(f_lineal, t, x)
    # A, B = popt
    # v_med.append(A)
    # s = f"{fase:.5f} {abs(A):.5f}"
    # print(s)

# gammas = np.array(gammas)
# vrms = sqrt(5/32) * 9.8 * gammas / omega  * 1000
# plt.plot(vrms, v_med, 'o')
# # plt.plot(gammas, v_med, 'o')
# plt.xlabel(r"$V_{\text{RMS}}$ (mm/s)")
# # plt.xlabel(r"$\Gamma$ (g)")
# plt.ylabel(r"$v_D$ (mm/s)")
# plt.tight_layout()
# plt.savefig('Fig_2_vmed.pdf')
