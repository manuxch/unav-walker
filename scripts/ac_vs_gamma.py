#!/usr/bin/env python3

import numpy as np

gama = np.linspace(0.04, 0.3, 10)
a = gama * 9.8 / (125.66**2)

print(gama)
print(a)

print("$\Gamma$ & $a$ & $\Gamma$ & $a$ \\\\")
for i in range(5):
    s = f"{gama[i]:.3f} & {a[i]:.3e} & {gama[i+5]:.3f} & {a[i+5]:.3e} \\\\"
    print(s)

