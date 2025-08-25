#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import glob
plt.style.use('/home/manuel/granular/unav-walker/utils/figs.mplstyle')

files = glob.glob('*.dat')
print(len(files))
files.sort()
v_s_2_e = 0.542217668
t_s_2_e = 0.055328334

ts = []
vels = []
n_start = 400
for f in files[5:]:
    fin = open(f, 'r')
    data = fin.readlines()
    fin.close()
    print(data)
    ts.append(float(data[0].split()[12]) * t_s_2_e)
    vels.append(float(data[1].split()[5]) * v_s_2_e * 100)

ats = np.array(ts)
avels = np.array(vels)
print(avels[n_start:].mean(), avels[n_start:].std())
plt.plot(ats, avels)
plt.plot(ats[n_start:], avels[n_start:])
plt.xlabel('$t$ [s]')
plt.ylabel(r'$v_y$ [cm/s]')
plt.title(r'$\mu_d = 0.13$')
plt.tight_layout()
plt.savefig('vy-t.pdf')



