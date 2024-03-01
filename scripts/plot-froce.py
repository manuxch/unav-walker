#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import root

g = 9.8
m = 0.081

def karnopp(v, vc, mu_s, mu_d):
    # vret = np.empty_like(v)
    mask = np.abs(v) < vc
    vret = np.where(mask, mu_s * np.sign(v), mu_d * np.sign(v))
    # vret[mask] = mu_s * np.sign(v[mask])
    # vret[~mask] = mu_d * np.sign(v[mask])
    return vret


def sc(v, vc, mu_d):
    return mu_d * np.tanh(v/vc)

def sc_2(v, vc, vs, mu_d, mu_s):
    return mu_d * np.tanh(v/vc) + (mu_s - mu_d) * v/vs * np.exp(-(v/vs)**2)

def vb(v, B, C, E, mu):
    return mu_d * 0.081 * 9.8 * np.sin(C * np.arctan(B*v) - E*((B*v) - np.arctan(B * v)))


# res = root(fr, 0.01, args=(0.01, 0.01, 0.16))
# print(res)
# quit()


v = np.linspace(-0.1, 0.1, 5000)
mu_d = 0.16
mu_s = 0.23
vc = 0.01
vs = 0.03

B = 1.0e2
C = 1.7
E = -1.0e-4


plt.plot(v, karnopp(v, vc, mu_s, mu_d), label='Karnopp')
plt.plot(v, sc(v, vc, mu_d), label='Smooth Coulomb')
plt.plot(v, sc_2(v, vc, vs, mu_d, mu_s), label='Smooth Coulomb 2')
# plt.plot(v, vb(v, B, C, E, mu_d), label='velocity based - ec5)')
plt.xlabel(r"$v_{rel}$")
plt.ylabel(r"$f_{fric}$")
plt.legend()
plt.tight_layout()
plt.savefig('froce-3.pdf')
