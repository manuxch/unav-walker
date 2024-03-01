#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt
plt.style.use('../utils/figs.mplstyle')
from scipy.optimize import curve_fit
import glob
from utiles import obtener_amplitud


def f_seno(t, A, B, f, phi):
    return A * np.sin(2 * np.pi * f * t + phi) + B

datos_files = glob.glob('p-*.dat')

datos_files.sort()

n_dat = 5500
amps = []
gammas = []

print("# gamma amp B fase phi")
for i, f in enumerate(datos_files):
    gamma = obtener_amplitud(f)
    if not gamma:
        print("Error en la lectura de gamma.")
        quit()
    gammas.append(gamma)
    t, y, x = np.loadtxt(f, usecols=(0, 1, 2), unpack=True)
    # t = t[-n_dat:]
    # x = x[-n_dat:]
    popt, pcov = curve_fit(f_seno, t[-n_dat:], x[-n_dat:], p0=(0.1, -.01, 20, -0.78))
    # A, fr, phi = popt
    A, B, fr, phi = popt
    amps.append(abs(A))
    s = f"{gamma:.5f} {abs(A):.5f} {B:.5f} {fr:.5f} {phi:.5f}"
    print(s)
    # plt.plot(t[-n_dat:], x[-n_dat:], '-.b')
    # plt.plot(t[-n_dat:], y[-n_dat:], '-.g')
    # plt.plot(t[-n_dat:], f_seno(t[-n_dat:], A, B, fr, phi), '-.r')
    # plt.plot(t[-n_dat:], f_seno(t[-n_dat:], A, 0, fr, 0), '-.y')
    # plt.show()

plt.plot(gammas, amps, 'o')
plt.xlabel(r"$\Gamma$ (g)")
plt.ylabel(r"$A_{w}$ (mm)")
plt.tight_layout()
plt.savefig('Fig_2_karnopp.pdf')
