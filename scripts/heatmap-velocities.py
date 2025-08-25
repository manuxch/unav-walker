#!/usr/bin/env python3

import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
import math
import argparse
import glob#, os
from tqdm import tqdm

params = {'backend': 'pdf',
        'interactive': False,
        'lines.linewidth': 2.0,
        'axes.labelsize': 10,
        'font.size': 10,
        'legend.fontsize': 10,
        'xtick.labelsize': 10,
        'ytick.labelsize': 10,
        'figure.subplot.left':0.01,
        'figure.subplot.right':0.99,
        'figure.subplot.top':0.95,
        'figure.subplot.bottom':0.05,
        'text.usetex': True}
mpl.rcParams.update(params)

class VelocityHeatmap2D:
    def __init__(self, x_min, x_max, y_min, y_max, cell_size):
        self.x_min = x_min
        self.x_max = x_max
        self.y_min = y_min
        self.y_max = y_max
        self.cell_size = cell_size

        self.nx = int(np.ceil((x_max - x_min) / cell_size))
        self.ny = int(np.ceil((y_max - y_min) / cell_size))

        # Acumuladores por celda
        self.sum_velocities = np.zeros((self.ny, self.nx))
        self.counts = np.zeros((self.ny, self.nx), dtype=int)

    def _get_cell_indices(self, x, y):
        i = int((y - self.y_min) / self.cell_size)
        j = int((x - self.x_min) / self.cell_size)
        if 0 <= i < self.ny and 0 <= j < self.nx:
            return i, j
        else:
            return None  # Fuera de los límites

    def add_frame(self, positions, velocities):
        """
        positions: array-like de shape (N, 2) con coordenadas (x, y)
        velocities: array-like de shape (N,) con las velocidades escalares
        """
        for (x, y), v in zip(positions, velocities):
            idx = self._get_cell_indices(x, y)
            if idx:
                i, j = idx
                self.sum_velocities[i, j] += v
                self.counts[i, j] += 1

    def compute_average(self):
        with np.errstate(invalid='ignore', divide='ignore'):
            avg = self.sum_velocities / self.counts
            avg[np.isnan(avg)] = 0  # opcional: poner ceros donde no hubo datos
        return avg

    def plot(self, cmap='viridis', save_path=None, dpi=300, log_scale=False, vmin=None, vmax=None):
        avg = self.compute_average()
        extent = [self.x_min, self.x_max, self.y_min, self.y_max]
        
        plt.figure(figsize=(8, 6))
        
        if log_scale:
            # Reemplazar ceros por np.nan para que LogNorm no falle
            avg_plot = np.where(avg > 0, avg, np.nan)
            norm = LogNorm(vmin=vmin or np.nanmin(avg_plot), vmax=vmax or np.nanmax(avg_plot))
            img = plt.imshow(avg_plot, origin='lower', extent=extent,
                            interpolation='nearest', cmap=cmap, norm=norm)
        else:
            img = plt.imshow(avg, origin='lower', extent=extent,
                            interpolation='nearest', cmap=cmap, vmin=vmin, vmax=vmax)

        plt.colorbar(img, label='Velocidad promedio (m/s)')
        plt.xlabel('$x$')
        plt.ylabel('$y$')
        plt.title('Mapa de calor de velocidad promedio')
        plt.gca().set_aspect('equal')

        if save_path:
            plt.savefig(save_path, dpi=dpi, bbox_inches='tight')
            print(f"Imagen guardada en: {save_path}")
        else:
            plt.show()
        plt.close()


parser = argparse.ArgumentParser(description='Heatmap de velocidades.')
parser.add_argument('-g','--grid', help='Show grid', type=bool, required=False, action="store", default=False)
parser.add_argument('-y','--ymin', help='Min y-region', type=float, required=False, action="store", default=-5.0)
parser.add_argument('-Y','--ymax', help='Max y-region', type=float, required=False, action="store", default=10.0)
parser.add_argument('-x','--xmin', help='Min x-region', type=float, required=False, action="store", default=-7.0)
parser.add_argument('-X','--xmax', help='Max X-region', type=float, required=False, action="store", default=7.0)
parser.add_argument('-a', '--asize', help='Grid size', type=float, required=False, action="store",
                    default=1.0)

args =  parser.parse_args()
preName = 've_frm_'
show_grid = args.grid
y_min = args.ymin
y_max = args.ymax
x_min = args.xmin
x_max = args.xmax
a_size = args.asize

# Conversión de unidades de simulación a experimentales:
L_scale = 0.03
M_scale = 0.081
acel_scale = 9.8
T_scale = math.sqrt(L_scale / acel_scale)
v_scale = L_scale / T_scale
F_scale = M_scale * acel_scale


fileFrames = []
# os.chdir('.')
for file in glob.glob('frames_walker/' + preName + '*.dat'):
    fileFrames.append(file)

fileFrames.sort()
# print(fileFrames)

# Crear el objeto de acumulación
heatmap = VelocityHeatmap2D(x_min=x_min, x_max=x_max, y_min=y_min, y_max=y_max, cell_size=a_size)

for file in fileFrames:
    x, y, vx, vy = np.loadtxt(file, usecols=(2, 3, 4, 5), unpack=True)
    vx *= v_scale
    vy *= v_scale
    v = np.sqrt(vx**2 + vy**2)
    heatmap.add_frame(np.column_stack((x, y)), v)

heatmap.plot(save_path='Heatmap_vel.pdf', log_scale=False)


# Simular varios frames
# for frame in range(100):
#     # Por ejemplo, 200 puntos por frame
#     xs = np.random.uniform(0, 10, 200)
#     ys = np.random.uniform(0, 5, 200)
#     vs = np.sqrt(xs**2 + ys**2)  # una velocidad ficticia
#
#     heatmap.add_frame(np.column_stack((xs, ys)), vs)
#
# # Mostrar mapa de calor
# heatmap.plot()
#
