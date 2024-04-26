#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
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
# os.chdir('.')
for file in glob.glob(preName + '*.xy'):
    fileFrames.append(file)


#print fileFrames
fileFrames.sort()

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

colors = list(mcolors.TABLEAU_COLORS)
aux_c = colors[1]
colors[1] = colors[3]
colors[3] = aux_c

nTotFiles = len(fileFrames)
nActualFile = 0
colorG = []
maxY = []
yMin = yMax = xMin = xMax = 0.0
#  fig, ax = plt.subplots()
for f in tqdm(fileFrames[::skp_frm]):
    fig = plt.figure(figsize=(10,10))
    fin = open(f,'r')
    maxYfrm = 0.0
    data = fin.readlines()
    fin.close()
    ax=fig.add_subplot(111, aspect='equal')
    patches = []
    caja = []
    lines = []
    fplot = f.split('.')[0]
    fout = preName + '{:06d}.png'.format(nActualFile)
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
            colorG.append(colors[ int(l[-1]) ])
        else:
            radio = float(l[4])
            patches.append(Circle((float(l[2]),float(l[3])), radio, fill=True))
            if float(l[3]) > maxYfrm:
                maxYfrm = float(l[3])
            colorG.append(colors[ int(l[-1]) ])
    maxY.append(maxYfrm)
    # cmap = plt.get_cmap('Set1')
    # colors = cmap(colorG)
    p = PatchCollection(patches, color=colorG, alpha=0.9)
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
    plt.savefig(fout)
    #  plt.cla()
    nActualFile += 1
    plt.close(fig)
amaxY = np.array(maxY)
