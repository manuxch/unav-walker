#!/usr/bin/env python3

import numpy as np
from math import sqrt
import matplotlib.pyplot as plt
plt.style.use('../utils/figs.mplstyle')


def Theta(g, mu):
    '''g = Gamma: aceleración reducida.'''
    return 2 * np.arcsin(np.pi * mu / g)

def etaT(g, mu, fi):
    return sqrt(2/5) * np.cos(fi - Theta(g, mu))

g0, v0 = np.loadtxt('../test-04/mean_vel_0.dat', usecols=(0, 1), unpack=True)
gpi2, vpi2 = np.loadtxt('../test-05/mean_vel_pi2.dat', usecols=(0, 1), unpack=True)
omega = 125.66

vrms = sqrt(5/32) * 9.8 * g0 / omega  * 1000

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12,6))

alfa = 0.5
ax1.plot(vrms, v0, 'o', label="Karnopp, $\phi = 0$", alpha=alfa)
ax1.plot(vrms, vpi2, 'o', label="Karnopp, $\phi = \pi/2$", alpha=alfa)
ax1.legend()
ax1.set_xlabel(r"$V_{\text{RMS}}$ (mm/s)")
ax1.set_ylabel(r"$v_D$ (mm/s)")

eta0 = v0 / vrms
etapi2 = vpi2 / vrms
ax2.plot(vrms, eta0, 'o', color='tab:blue', label="Karnopp, $\phi = 0$", alpha=alfa)
ax2.plot(vrms, etapi2, 'o', color='tab:orange', label="Karnopp, $\phi = \pi/2$", alpha=alfa)

mu = 0.16
gg = np.linspace(np.pi * mu, 3.23983, 200)
vrms_gg = sqrt(5/32) * 9.8 * gg / omega  * 1000

ax2.plot(vrms_gg, etaT(gg, mu, 0), color='tab:blue', label=r"$\eta_T, \phi = 0, \mu=0.16$")
ax2.plot(vrms_gg, etaT(gg, mu, np.pi/2), color='tab:orange', label=r"$\eta_T, \phi = \pi/2, \mu=0.16$")
ax2.set_ylim([-0.02, 0.65])
ax2.legend()
ax2.set_xlabel(r"$V_{\text{RMS}}$ (mm/s)")
ax2.set_ylabel(r"$\eta$ (mm/s)")


plt.tight_layout()
plt.savefig('Fig_2_MM.pdf')


