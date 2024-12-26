#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import matplotlib.colors as mcolors

# Ejemplo de datos
min_val = 0   # valor mínimo
max_val = 10  # valor máximo
values = np.array([1, 2, 5, 7, 9, 10])  # valores de la variable

# Crear un colormap
cmap = plt.get_cmap('viridis')  # Puedes cambiar 'viridis' por cualquier otro colormap

# Normalizar los valores al rango [0, 1]
norm = mcolors.Normalize(vmin=min_val, vmax=max_val)

# Mapear los valores normalizados a colores
colors = cmap(norm(values))

fig, ax = plt.subplots()
# Mostrar los colores asignados
sc = ax.scatter(values, np.ones_like(values), color=colors, s=100)
cbar = fig.colorbar(plt.cm.ScalarMappable(norm=norm, cmap=cmap), cax=ax,
                    location='left', fraction=0.15,label='Valor')

# Mostrar el gráfico
plt.show()

