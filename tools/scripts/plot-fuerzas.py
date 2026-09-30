#!/usr/bin/env python3

import glob
import re
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.colors as colors
import numpy as np
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import unidades  # factores de conversión (utils/reduced_units.ods)
def archivos_por_D(patron):
    """Pares (10 D, archivo) de los archivos que cumplen el patrón glob.

    El sufijo numérico del nombre es 10 D (perfil-fn-30.dat -> D = 3.0 d).
    Los archivos con otro sufijo se saltean con un aviso.
    """
    pares = []
    for f in sorted(glob.glob(patron)):
        m = re.fullmatch(r'.*-(\d+)\.dat', f)
        if m is None:
            print(f"aviso: {f} ignorado (el sufijo no es 10 D)", file=sys.stderr)
            continue
        pares.append((int(m.group(1)), f))
    return pares

plt.rcParams.update({
    'text.usetex': True,
    'font.family': 'serif',
    'font.serif': ['Linux Libertine O'],
    'font.size': 14,
    'axes.titlesize': 14,
    'axes.labelsize': 14,
    'legend.fontsize': 8,
    'xtick.labelsize': 14,
    'ytick.labelsize': 14
})

norm = colors.Normalize(vmin=30, vmax=200)
cmap = mpl.colormaps['plasma']

files_N = archivos_por_D("perfil-fn-*.dat")
files_T = archivos_por_D("perfil-ft-*.dat")

fig, ax = plt.subplots(2, 1, figsize=(8, 6), sharex=True)
# plt.title('Perfil de fuerzas normales')

alfa = 0.7
for d, f in files_N:
    y, fn = np.loadtxt(f, unpack=True, comments='#', usecols=(0, 1))
    y = unidades.y_a_cm(y)
    fn = unidades.fuerza_a_N(fn)
    print(f"D = {d:3d} - max F_N = {np.nanmax(fn):.3e}")
    c = cmap(norm(d))
    ax[0].plot(y, fn, '.-', color=c, label=fr"$D = {d/10} \, d$", alpha=alfa)  # vy vs y

for d, f in files_T:
    y, fn = np.loadtxt(f, unpack=True, comments='#', usecols=(0, 1))
    y = unidades.y_a_cm(y)
    fn = unidades.fuerza_a_N(fn)
    print(f"D = {d:3d} - max F_T = {np.nanmax(fn):.3e}")
    c = cmap(norm(d))
    ax[1].plot(y, fn, '.-', color=c, label=fr"$D = {d/10} \, d$", alpha=alfa)  # vy vs y

# plt.plot(data[:, 1], data[:, 0])  # vy vs y
ax[0].set_ylabel(r'$\langle f_N \rangle$ (N)');
ax[1].set_ylabel(r'$\langle |f_T| \rangle$ (N)');
ax[1].set_xlabel(r'$y$ (cm)')
ax[0].legend(ncols=2)
# ax[1].legend()
plt.tight_layout()
plt.savefig('perfiles-fn-ft.pdf')

