#!/usr/bin/env python3

import glob 
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.colors as colors
import numpy as np
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

norm = colors.Normalize(vmin=50, vmax=110)
cmap = mpl.colormaps['plasma']

files = glob.glob("perfil-ve-*.dat")
files.sort()
print(files)
d_s_2_e = 0.005 * 100 
v_s_2_e = 0.221359436211787 * 100

for f in files:
    d = int(f.split('-')[2].split('.')[0])
    y, v_y = np.loadtxt(f, unpack=True, comments='#')
    y *= -d_s_2_e
    v_y *= -v_s_2_e
    print(f"D = {d:3d} - max vy = {v_y.max():.3f} - vf = {v_y[0]:.3f}")
    c = cmap(norm(d))
    plt.plot(y, v_y, color=c, label=fr"$D = {d/10} \, d$")  # vy vs y

# plt.plot(data[:, 1], data[:, 0])  # vy vs y
plt.ylabel(r'$\langle v_y \rangle$ (cm/s)');
plt.xlabel(r'$y$ (cm)')
plt.title('Perfil de velocidad vertical')
plt.legend()
plt.tight_layout()
plt.savefig('perfiles-vy.pdf')

