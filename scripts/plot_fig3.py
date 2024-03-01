#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt
plt.style.use('../utils/figs.mplstyle')

gc, phi = np.loadtxt('../test-00/amps-00_k.out', usecols=(0, 4), unpack=True)
gsc, phisc = np.loadtxt('../test-01/amps-01_sc.out', usecols=(0, 4), unpack=True)
gsc2, phisc2 = np.loadtxt('../test-02/amps-02.out', usecols=(0, 4), unpack=True)

alfa = 0.5
plt.plot(gc, np.abs(phi), 'o', label="Karnopp", alpha=alfa)
plt.plot(gsc, np.abs(phisc), 'x', label="Smooth Coulomb", alpha=alfa)
plt.plot(gsc2, np.abs(phisc2), '*', label="Smooth Coulomb - 2", alpha=alfa)
plt.legend()
plt.xlabel(r"$\Gamma$ (g)")
plt.ylabel(r"$\Delta \chi$ ")
plt.tight_layout()
plt.savefig('Fig_3_comp3.pdf')


