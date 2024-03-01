#!/usr/bin/env python3

import glob
import numpy as np
from math import sqrt
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.cm as cm
from scipy.optimize import curve_fit
plt.style.use('/home/manuel/granular/walker-unav/utils/figs.mplstyle')

def f_modulacion(f, V, T):
    return V * np.cos(f - T)

def f_Teta(g, mu):
    return 2 * np.arcsin(np.pi * mu / g)

datos_files = glob.glob('vmean-phi-*.out')
datos_files.sort()

gammas = []
Tetas = []
alfa = 0.7
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12,6))
norm = mpl.colors.Normalize(vmin=0, vmax=3)
cmap = plt.cm.Reds

# bound = [(None, None), (0, np.pi/2)]
bound = [(-np.inf, 0), (np.inf, np.pi/2)]
for i, f in enumerate(datos_files):
    gama = float(f.split('-')[2][:-4]) / 100
    gammas.append(gama)
    f, vmean = np.loadtxt(f, unpack=True)
    popt, pcov = curve_fit(f_modulacion, f, vmean, bounds=bound)
    V, T = popt
    print(f'{gama = :.02f}, {V = }, {T = }')
    Tetas.append(T)
    slab = f'$\Gamma = $ {gama:.2f}'
    ax1.plot(f, vmean, '.-', label=slab, color=cmap(norm(gama)))


ax1.legend(fontsize=12)
ax1.set_xlabel(r"$\phi$")
ax1.set_ylabel(r"$v_D$ (mm/s)")
ax1.axhline(y=0, color='tab:grey')
ax1.axvline(x=np.pi, color='tab:grey')
ax1.set_xticks([0, np.pi/2, np.pi, 3*np.pi/2, 2*np.pi])
ax1.set_xticklabels([r'$0$', r'$\pi/2$', r'$\pi$', r'$3 \pi /2$', r'$2 \pi$'])

ax2.plot(gammas, Tetas, 'o')
ax2.set_xlabel(r"$\Gamma$")
ax2.set_ylabel(r"$\Theta$")
ax2.set_yticks([0, np.pi/4, np.pi/2])
ax2.set_yticklabels([r'$0$', r'$\pi/4$', r'$ \pi /2$'])

popt, pcov = curve_fit(f_Teta, gammas, Tetas, p0=(0.15))
mu = popt
print(f'{mu = }')
x_gamas = np.linspace(0.5, 3, 200)
ax2.plot(x_gamas, f_Teta(x_gamas, mu))

# f068, v068 = np.loadtxt('./gama_068/vmean-phi-068.out', unpack=True)
# f100, v100 = np.loadtxt('./gama_100/vmean-phi-100.out', unpack=True)
# f300, v300 = np.loadtxt('./gama_300/vmean-phi-300.out', unpack=True)


# alfa = 0.5
# ax1.plot(f100, v100, 'o', label=r'$\Gamma = 1.00$')
# ax1.plot(f300, v300, 'o', label=r'$\Gamma = 3.00$')

# eta0 = v0 / vrms
# etapi2 = vpi2 / vrms
# ax2.plot(vrms, eta0, 'o', color='tab:blue', label="Karnopp, $\phi = 0$", alpha=alfa)
# ax2.plot(vrms, etapi2, 'o', color='tab:orange', label="Karnopp, $\phi = \pi/2$", alpha=alfa)

# mu = 0.16
# gg = np.linspace(np.pi * mu, 3.23983, 200)
# vrms_gg = sqrt(5/32) * 9.8 * gg / omega  * 1000

# ax2.plot(vrms_gg, etaT(gg, mu, 0), color='tab:blue', label=r"$\eta_T, \phi = 0, \mu=0.16$")
# ax2.plot(vrms_gg, etaT(gg, mu, np.pi/2), color='tab:orange', label=r"$\eta_T, \phi = \pi/2, \mu=0.16$")
# ax2.set_ylim([-0.02, 0.65])
# ax2.legend()
# ax2.set_xlabel(r"$V_{\text{RMS}}$ (mm/s)")
# ax2.set_ylabel(r"$\eta$ (mm/s)")


plt.tight_layout()
plt.savefig('Fig_4_MM.pdf')


