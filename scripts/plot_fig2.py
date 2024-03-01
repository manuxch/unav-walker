#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt
plt.style.use('../utils/figs.mplstyle')

gc, ac = np.loadtxt('../test-00/amps-00_k.out', usecols=(0, 1), unpack=True)
gsc, asc = np.loadtxt('../test-01/amps-01_sc.out', usecols=(0, 1), unpack=True)
gsc2, asc2 = np.loadtxt('../test-02/amps-02.out', usecols=(0, 1), unpack=True)

alfa = 0.5
plt.plot(gc, ac, 'o', label="Karnopp", alpha=alfa)
plt.plot(gsc, asc, 'x', label="Smooth Coulomb", alpha=alfa)
plt.plot(gsc2, asc2, '*', label="Smooth Coulomb - 2", alpha=alfa)
plt.legend()
plt.xlabel(r"$\Gamma$ (g)")
plt.ylabel(r"$A_{w}$ (mm)")
plt.tight_layout()
plt.savefig('Fig_2_comp3.pdf')


