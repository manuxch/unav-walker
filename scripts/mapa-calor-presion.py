#!/usr/bin/env python3 

import argparse
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
import os
plt.style.use('/home/manuel/granular/unav-walker/utils/figs.mplstyle')

parser = argparse.ArgumentParser(description="Mapa de calor y perfil de presiones")
parser.add_argument("-g", "--gamma", help="Valor de Gamma", required=True)
args = parser.parse_args()
gamma = args.gamma


# Directorio donde están los archivos
directory = "."
# Tamaño del reticulado
grid_size = 1.0 * 0.03 # Configurable sims -> exp

# Diccionario para almacenar datos de presión y coordenadas
pressure_data = {}
height_pressure = []

# Diccionario para almacenar datos de presión y coordenadas
pressure_data = {}
height_pressure = []

def read_data(nnn):
    """Lee los archivos frm_nnn.sxy y frm_nnn.xy y calcula la presión."""
    sxy_file = os.path.join(directory, f"frm_{nnn}.sxy")
    xy_file = os.path.join(directory, f"frm_{nnn}.xy")
    
    if not os.path.exists(sxy_file) or not os.path.exists(xy_file):
        return None
    
    # Leer datos
    sxy = np.genfromtxt(sxy_file)
    xy = np.genfromtxt(xy_file, skip_footer=3)
    
    # Presión como el promedio de los elementos diagonales del tensor de estrés
    pressure = -(sxy[:, 1] + sxy[:, 4]) / 2  # Suponiendo sxx y syy en columna 1 y 4 (desde cero)
    
    # Coordenadas espaciales
    x, y = xy[:, 2], xy[:, 3]
    x *= 0.03
    y *= 0.03
    return x, y, pressure

# Procesar todos los archivos
for filename in os.listdir(directory):
    if filename.startswith("frm_") and filename.endswith(".sxy"):
        nnn = filename.split("_")[1].split(".")[0]
        data = read_data(nnn)
        if data is not None:
            x, y, pressure = data
            for i in range(len(x)):
                key = (x[i], y[i])
                if key not in pressure_data:
                    pressure_data[key] = []
                pressure_data[key].append(pressure[i])

# Determinar el dominio del espacio
x_vals = [k[0] for k in pressure_data.keys()]
y_vals = [k[1] for k in pressure_data.keys()]
x_min, x_max = min(x_vals), max(x_vals)
y_min, y_max = min(y_vals), max(y_vals)

# Crear la grilla
x_bins = np.arange(x_min, x_max, grid_size)
y_bins = np.arange(y_min, y_max, grid_size)
pressure_grid = np.zeros((len(x_bins), len(y_bins)))
counts = np.zeros((len(x_bins), len(y_bins)))

# Asignar valores a la grilla
for (x, y), pressures in pressure_data.items():
    avg_p = np.mean(pressures) * 0.7938 / 0.03 # unidades sims -> exp F / L
    x_idx = np.searchsorted(x_bins, x) - 1
    y_idx = np.searchsorted(y_bins, y) - 1
    if 0 <= x_idx < len(x_bins) and 0 <= y_idx < len(y_bins):
        pressure_grid[x_idx, y_idx] += avg_p
        counts[x_idx, y_idx] += 1

# Normalizar por la cantidad de partículas en cada celda
pressure_grid[counts > 0] /= counts[counts > 0]
pressure_grid[counts == 0] = np.nan  # Evitar divisiones por cero

# Crear el mapa de calor
plt.figure(figsize=(8, 6))
plt.imshow(pressure_grid.T, origin="lower", extent=[x_min, x_max, y_min, y_max], 
           cmap="Reds", aspect="equal", interpolation='bilinear')
           # norm=LogNorm())
plt.colorbar(label="Presión media (N / m)")
plt.xlabel(r"$x$ [m]")
plt.ylabel(r"$y$ [m]" )
plt.title(r"Mapa de calor de presión media, $\Gamma = $" + f"{gamma}")
# plt.show()
plt.tight_layout()
plt.savefig(f'p-heatmap-G-{gamma}.pdf')

# Calcular presión promedio en función de la altura
height_dict = {}
for y_idx, y in enumerate(y_bins):
    pressures = pressure_grid[:, y_idx][~np.isnan(pressure_grid[:, y_idx])]
    if len(pressures) > 0:
        height_dict[y] = np.mean(pressures)

heights = sorted(height_dict.keys())
avg_pressures_by_height = [height_dict[h] for h in heights]
for h, p in zip(heights, avg_pressures_by_height):
    print(h, p)

# Graficar presión en función de la altura
plt.figure(figsize=(8, 6))
plt.plot(heights, avg_pressures_by_height, "o-")
plt.xlabel(r"Altura ($y$) [m]")
plt.ylabel("Presión media [N/m]")
plt.title(r"Presión media en función de la altura, $\Gamma = $" + f"{gamma}")
plt.grid()
# plt.show()
plt.tight_layout()
plt.savefig(f'p-profile-G-{gamma}.pdf')

