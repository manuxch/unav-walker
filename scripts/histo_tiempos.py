#!/usr/bin/python3
import numpy as np
import argparse
from scipy.stats import linregress
import matplotlib.pyplot as plt
from math import log, exp


def histo(t):
    """Calcula el histograma de los intervalos de tiempo entre salidas de granos
    Params:
    -------
    t: array con el tiempo de salida del grano

    Return:
    -------
    histo: histograma con la distribució de tiempos de espera
    """
    dt = t[1:] - t[:-1]
    # histo = np.histogram(dt)
    histo = np.histogram(dt, bins='fd')
    return histo


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Cálculo del histograma de tiempos de espera.")
    parser.add_argument("-f", "--flux", help="Archivo de entrada", required=True)
    parser.add_argument("-t", "--t_factor",
                        help="Escala de tiempo (sim -> exp [s]). Default: valor experimental",
                        type=float, default=0.055328334, required=False)
    parser.add_argument("-p", "--plot", help="Muestra un gráfico del ajuste.",
                        type=bool, default=False, required=False)

    args = parser.parse_args()
    fin = args.flux
    tf = args.t_factor
    plot = args.plot
    t = np.loadtxt(fin, usecols=(2), unpack=True)
    print(f"N valores: {t.size}")
    h, b = histo(t)
    iL = 7
    iR = -1
    xbin = (b[1:] - b[:-1]) / 2 + b[:-1]
    b_fit = xbin[iL:iR]
    h_fit = h[iL:iR]
    mask = h_fit != 0
    m, i, r, pv, stderr = linregress(np.log(b_fit[mask]), np.log(h_fit[mask]))
    print(f" m = {m}, i = {i}, r = {r}, stdE = {stderr}")
    x = np.linspace(xbin[iL], xbin[iR], 100)
    y = x**m * exp(i)
    plt.plot(xbin, h, '-o')
    plt.plot(x, y, '--')
    plt.yscale('log')
    # plt.xscale('log')
    ss = f"$y = {m:.3f} \,  x + {i:.3f}$"
    ax = plt.gca()
    plt.text(0.2, 0.1, ss, horizontalalignment='center', verticalalignment='center', transform=ax.transAxes)
    plt.xlabel(r"$\delta t$ (s)")
    plt.show()
