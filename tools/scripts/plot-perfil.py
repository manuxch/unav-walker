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

norm = colors.Normalize(vmin=40, vmax=200)
cmap = mpl.colormaps['plasma']

files = archivos_por_D("perfil-ve-*.dat")
alfa = 0.7

for d, f in files:
    y, v_y = np.loadtxt(f, unpack=True, comments='#')
    y = unidades.y_a_cm(y)
    v_y = unidades.vy_a_cm_s(v_y)
    print(f"D = {d:3d} - max vy = {np.nanmax(v_y):.3f} - vf = {v_y[0]:.3f}")
    c = cmap(norm(d))
    plt.plot(y, v_y, '.-', color=c, label=fr"$D = {d/10} \, d$", alpha=alfa)  # vy vs y

# plt.plot(data[:, 1], data[:, 0])  # vy vs y
plt.ylabel(r'$\langle v_y \rangle$ (cm/s)');
plt.xlabel(r'$y$ (cm)')
plt.title('Perfil de velocidad vertical')
plt.legend(ncols=2)
plt.tight_layout()
plt.savefig('perfiles-vy.pdf')

