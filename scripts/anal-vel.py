#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import glob
from scipy.stats import linregress
plt.style.use('/home/manuel/granular/unav-walker/utils/figs.mplstyle')

files = glob.glob('*.ve')
print(len(files))
files.sort()
x_s_2_e = 0.005
v_s_2_e = 0.221359436211787
t_s_2_e = 0.000245

ts = []
ys = []
vels = []
n_start = 100
n_end = 1500
for f in files[5:]:
    fin = open(f, 'r')
    data = fin.readlines()
    fin.close()
    ts.append(float(data[0].split()[12]) * t_s_2_e)
    ys.append(float(data[3].split()[3]) * x_s_2_e)
    vels.append(float(data[3].split()[5]) * v_s_2_e * 100)

ats = np.array(ts)
ays = np.array(ys)
avels = np.array(vels)
vmean = avels[n_start:n_end].mean()
print(vmean, avels[n_start:n_end].std())
res = linregress(ats[n_start:n_end], ys[n_start:n_end])
vel = res.slope
print(f'fit vel: {vel:.3f}')
# plt.plot(ats, avels)
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8), sharex=True)

ax1.plot(ats[n_start:n_end], ays[n_start:n_end], label=fr'$v_y = {vel:.3f}$ [m/s]')
ax1.set_ylabel(r'$y$ [cm]')
ax1.legend()
ax2.plot(ats[n_start:n_end], avels[n_start:n_end], label=fr'$<v_y> = {vmean:.3f}$ [cm/s]')
ax2.set_xlabel('$t$ [s]')
ax2.set_ylabel(r'$v_y$ [cm/s]')
ax2.legend()
# plt.title(r'$\mu_d = 0.13$')
plt.tight_layout()
plt.savefig('vy-t.pdf')



