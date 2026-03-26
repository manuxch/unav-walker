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

norm = colors.Normalize(vmin=30, vmax=200)
cmap = mpl.colormaps['plasma']

files_N = glob.glob("perfil-fn-*.dat")
files_N.sort()
files_T = glob.glob("perfil-ft-*.dat")
files_T.sort()
d_s_2_e = 0.005 * 100 
v_s_2_e = 0.221359436211787 * 100
f_s_2_e = 0.002058

fig, ax = plt.subplots(2, 1, figsize=(8, 6), sharex=True)
# plt.title('Perfil de fuerzas normales')

alfa = 0.7
for f in files_N:
    d = int(f.split('-')[2].split('.')[0])
    y, fn = np.loadtxt(f, unpack=True, comments='#')
    y *= -d_s_2_e
    fn *= f_s_2_e
    print(f"D = {d:3d} - max F_N = {fn.max():.3e}")
    c = cmap(norm(d))
    ax[0].plot(y, fn, '.-', color=c, label=fr"$D = {d/10} \, d$", alpha=alfa)  # vy vs y

for f in files_T:
    d = int(f.split('-')[2].split('.')[0])
    y, fn = np.loadtxt(f, unpack=True, comments='#')
    y *= -d_s_2_e
    fn *= f_s_2_e
    print(f"D = {d:3d} - max F_N = {fn.max():.3e}")
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

