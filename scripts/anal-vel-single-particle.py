#!/usr/bin/env python3

import argparse
import matplotlib.pyplot as plt
import numpy as np
import glob
plt.style.use('/home/manuel/granular/unav-walker/utils/figs.mplstyle')

parser = argparse.ArgumentParser(
            prog='anal-vel-single-particle.py',
            description='Análisis de la velocidad de un solo walker')

parser.add_argument('gamma')
args = parser.parse_args()
gamma = args.gamma

if gamma == "1":
    files = glob.glob('gamma-1/frames_walker/ve_frm_*.dat')
else:
    files = glob.glob('gamma-3/frames_walker/ve_frm_*.dat')

print(len(files))
files.sort()
v_s_2_e = 0.542217668
t_s_2_e = 0.055328334

ts = []
vels_x = []
vels_y = []
n_start = 10000
delta_t = 5000
for f in files:
    with open(f, 'r') as fin:
        lineas = fin.readlines()
        if (len(lineas) < 2):
            raise ValueError(f"El archivo {f} no tiene al menos dos líneas")

    ts.append(float(lineas[0].split()[-1]))
    vels_x.append(float(lineas[1].split()[4]))
    vels_y.append(float(lineas[1].split()[5]))

# ats = np.array(ts)
ats = np.array(ts) * t_s_2_e
avelx = np.array(vels_x) * v_s_2_e
avely = np.array(vels_y) * v_s_2_e
vymean = avely[n_start:-10].mean()
vystd = avely[n_start:-10].std()

print(f"Gamma = {gamma} - v_y mean: {vymean}, v_y std: {vystd}")
# plt.plot(ats, avelx, label=r"$v_x$")
# plt.plot(ats, avely, '.-', label=r"$v_y$")
plt.plot(ats[n_start:n_start+delta_t], avely[n_start:n_start+delta_t], '.-',
         label=f"mean: {vymean:.4f} m/s")
# plt.plot(ats, '.')
plt.xlabel('$t$ [s]')
plt.ylabel(r'$v_y$ [m/s]')
plt.title(f'$\Gamma = {gamma}$')
plt.legend()
plt.tight_layout()
plt.savefig(f'vy-t-{gamma}.pdf')
# plt.show()



