#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import matplotlib
import matplotlib as mpl
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
from matplotlib.cm import ScalarMappable
from matplotlib.patches import Circle, Polygon
from matplotlib.collections import PatchCollection
from matplotlib import collections  as mc
import numpy as np
import argparse
import glob#, os
import sys
from tqdm import tqdm

###
# VER http://jakevdp.github.io/blog/2012/08/18/matplotlib-animation-tutorial/
###

parser = argparse.ArgumentParser(description='Programa para graficar frames.')
parser.add_argument('-f','--pfile',help='Input data file', required=True, action="store")
parser.add_argument('-s','--skip', help='Skip data frames', type=int, required=False, action="store", default=1)
parser.add_argument('-g','--grid', help='Show grid', type=bool, required=False, action="store", default=False)
parser.add_argument('-y','--ymin', help='Min y-scale', type=float, required=False, action="store", default=-5.0)
parser.add_argument('-Y','--ymax', help='Max y-scale', type=float, required=False, action="store", default=20.0)

args =  parser.parse_args()
preName = args.pfile + '_'
skp_frm = args.skip
show_grid = args.grid
scl_y_min = args.ymin
scl_y_max = args.ymax

fileFrames = []
fileStresses = []
# os.chdir('.')
for file in glob.glob(preName + '*.xy'):
    fileFrames.append(file)
for file in glob.glob(preName + '*.sxy'):
    fileStresses.append(file)
print('Archivos de frames: ', len(fileFrames))
print('Archivos de stress: ', len(fileStresses))
fileFrames.sort()
fileStresses.sort()


frm_idxs = {}
min_stress = 0.0
max_stress = 0.0
for file in fileFrames:
    frm_id = file.split('_')[-1][:-3]
    has_stress = any(preName + f"{frm_id}.sxy" in fss for fss in fileStresses)
    if has_stress:
        file_stress = preName + f"{frm_id}.sxy"
        print(file, file_stress)
        gid, sxx, syy = np.loadtxt(file_stress, usecols=(0,1,4), unpack=True)
        press = -0.5 *(sxx + syy)
        frm_idxs[frm_id] = (file, file_stress, press.min(), press.max())
        if press.min() < min_stress:
            min_stress = press.min()
        if press.max() > max_stress:
            max_stress = press.max()
    else:
        frm_idxs[frm_id] = (file, None, 0.0, 0.0)

# for frm_id, value in frm_idxs.items():
    # print(frm_id, value)
# print("min stress: ", min_stress, max_stress)

params = {'backend': 'pdf',
        'interactive': False,
        'lines.linewidth': 2.0,
        'axes.labelsize': 10,
        'font.size': 10,
        'legend.fontsize': 10,
        'xtick.labelsize': 15,
        'ytick.labelsize': 15,
        'figure.subplot.left':0.01,
        'figure.subplot.right':0.99,
        'figure.subplot.top':0.95,
        'figure.subplot.bottom':0.05,
        'text.usetex': True}
matplotlib.rcParams.update(params)
cmap = plt.get_cmap('OrRd') # Crear un colormap
# cmap = plt.get_cmap('inferno') # Crear un colormap
# norm = mcolors.LogNorm(vmin=1e-5, vmax=max_stress) # Normalizar los valores a [0, 1]
# norm = mcolors.Normalize(vmin=min_stress, vmax=max_stress) # Normalizar los valores a [0, 1]
norm = mcolors.PowerNorm(gamma=0.5, vmin=min_stress, vmax=max_stress) # Normalizar los valores a [0, 1]

nActualFile = 0
# for frm_id, value in frm_idxs.items():
for frm_id, value in tqdm(frm_idxs.items()):
    fig = plt.figure(figsize=(10,10))
    fin = open(value[0],'r')
    data = fin.readlines()
    fin.close()
    n_grains = 0
    for linea in data[1:]:
        if int(linea.split()[0]) >= 0:
            n_grains += 1
    gcolors = np.zeros(n_grains)
    sgid = np.arange(n_grains, dtype='int')[::-1]
    if value[1]:
        sgid, sxx, syy = np.loadtxt(value[1], usecols=(0, 1, 4), unpack=True)
        sgid = sgid.astype(int)
        press = -0.5 * (sxx + syy)

    maxY = []
    yMin = yMax = xMin = xMax = 0.0
    maxYfrm = 0.0
    ax=fig.add_subplot(111, aspect='equal')
    patches = []
    caja = []
    lines = []
    colorG = []
    fout = preName + f'{nActualFile:06d}.png'
    time = float(data[0].split()[2])
    for linea in data[1:]:
        l = linea.split()
        gid = int(l[0])
        if gid < 0:
            bverts = []
            nv = int(l[1])
            for i in range(nv):
                bverts.append((float(l[2+2*i]), float(l[2+2*i+1])))
                if bverts[i][0] < xMin:
                    xMin = bverts[i][0]
                if bverts[i][0] > xMax:
                    xMax = bverts[i][0]
                if bverts[i][1] < yMin:
                    yMin = bverts[i][1]
                if bverts[i][1] > yMax:
                    yMax = bverts[i][1]
            for i in range(nv - 1):
                lines.append([bverts[i], bverts[i+1]])
            continue
        nvert = int(l[1])
        conts = []
        verts = []
        if nvert > 1:
            for i in range(nvert):
                verts.append([float(l[2+2*i]), float(l[2+2*i+1])])
            patches.append(Polygon(verts, closed=True, fill=True))
            colorG.append(press[sgid[gid]])
        else:
            radio = float(l[4])
            patches.append(Circle((float(l[2]),float(l[3])), radio, fill=True, edgecolor='tab:grey'))
            if float(l[3]) > maxYfrm:
                maxYfrm = float(l[3])
            colorG.append(press[sgid[gid]])
    maxY.append(maxYfrm)
    # cmap = plt.get_cmap('Set1')
    # colors = cmap(colorG)
    p = PatchCollection(patches, color=cmap(norm(colorG)), edgecolor='tab:grey', alpha=0.9)
    lc = mc.LineCollection(lines, color='black', linewidths=1)
    ax.xaxis.set_ticks_position('both')
    ax.yaxis.set_ticks_position('both')
    ax.grid(visible=show_grid, which='both')
    #  ax.axes.get_xaxis().set_visible(False)
    #  ax.axes.get_yaxis().set_visible(False)

    ax.add_collection(p)
    ax.add_collection(lc)
    alturaSilo = yMax - yMin
    plt.xlim([1.1 * xMin, 1.1 * xMax])
    plt.ylim([scl_y_min,  scl_y_max])
    #  plt.ylim([yMin - 0.3 * alturaSilo,  1.1 * alturaSilo])
    # plt.ylim([yMin - 0.3 * alturaSilo, yMax + 0.15 * alturaSilo])
    # cb = plt.colorbar(p, cmap='OrRd')
    fig.colorbar(mpl.cm.ScalarMappable(norm=mpl.colors.PowerNorm(gamma=0.5, vmin=min_stress, vmax=max_stress), cmap='OrRd'),
                 ax=ax, orientation='vertical', label='Pressure')
    # fig.colorbar(mpl.cm.ScalarMappable(norm=mpl.colors.Normalize(min_stress, max_stress), cmap='OrRd'),
                 # ax=ax, orientation='vertical', label='Pressure')
    plt.tight_layout()
    plt.savefig(fout)
    #  plt.cla()
    nActualFile += 1
    plt.close(fig)
