#!/usr/bin/env python3

import glob
import matplotlib.pyplot as plt 
import numpy as np 
from scipy.stats import linregress
plt.style.use('/home/manuel/granular/unav-walker/utils/figs.mplstyle')

files = glob.glob("flx-*.dat")
files.sort()

sim2exp_t = 0.055328334
R_out = files[0].split('-')[1]
Q = []
std_Q = []
r_fit = []
alfa = 0.5
for file in files:
    t, q = np.loadtxt(file, usecols=(2, 3), unpack=True)
    t *= sim2exp_t
    size_fit = int(t.size / 2)
    plt.plot(t, q, alpha=alfa)
    result = linregress(t[size_fit :], q[size_fit :])
    #m, i, r, pv, mstd, isttd = linregress(t[size_fit :], q[size_fit :], alternative='greater')
    m = result.slope
    s = str(result.rvalue)
    if result.rvalue < 0.9:
        m = (q[-1] - q[0]) / (t[-1] - t[0])
        s = '*'
    Q.append(m)
    r_fit.append(s)

print("# q(1/s) r-fit")
for q, s in zip(Q, r_fit):
    print(f"{q} {s}")
plt.title(f"$R_{{out}} = {R_out} d$")
plt.xlabel("$t$ (s)") 
plt.ylabel("Walkers out")
plt.tight_layout()
plt.savefig("flujos-" + R_out + ".pdf")
