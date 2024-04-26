#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
r1, q1 = np.loadtxt("caudales-2.dat", unpack=True)
r2, q2 = np.loadtxt("caudales-4.dat", unpack=True)

plt.plot(r1, q1, '-o', label=r"$t_s = 9.22$ min")
plt.plot(r2, q2, '-s', label=r"$t_s = 46$ min")
plt.xlabel("A (d)")
plt.ylabel("Q (1/s)")
plt.legend(title=r'$\mu = 0.16$')
plt.xlim([0, 13])
plt.savefig('caudales-ext.pdf')

