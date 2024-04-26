#!/usr/bin/python3
import numpy as np
import argparse
from scipy.stats import linregress
import matplotlib.pyplot as plt
from math import log, exp
import glob


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
    parser.add_argument("-d", "--dirs", nargs='+', help="Directorios de entrada", required=True)
    parser.add_argument("-t", "--t_factor",
                        help="Escala de tiempo (sim -> exp [s]). Default: valor experimental",
                        type=float, default=0.055328334, required=False)
    parser.add_argument("-p", "--plot", help="Muestra un gráfico del ajuste.",
                        type=bool, default=False, required=False)

    args = parser.parse_args()
    idirs = args.dirs
    tf = args.t_factor
    plot = args.plot
    datos = {}
    for d in idirs:
        print(d)
        files = glob.glob(d + '/flx-*')
        for f in files:
            with open(f, 'r') as archivo:
                linea_1 = archivo.readline()
            D = 2 * float(linea_1.split()[2])
            t, g = np.loadtxt(f, usecols=(2, 4), unpack=True)
            t = t * 0.0553
            if t.size > 50:
                h, b = histo(t)
                color = plt.cm.viridis((D - 1.2) / (12 - 1.2))
                xbin = (b[1:] - b[:-1]) / 2 + b[:-1]
                plt.plot(xbin, h, color=color)


    plt.yscale('log')
    plt.xscale('log')
    # plt.xlim([0, 10])
    plt.xlabel(r"$\delta t$ (s)")
    norm = plt.Normalize(1.2, 12)
    sm = plt.cm.ScalarMappable(cmap=plt.cm.viridis, norm=norm)
    sm.set_array([])
    cbar = plt.colorbar(sm)
    cbar.set_label(r'$D$')
    plt.savefig('histo-dt-D.pdf')
