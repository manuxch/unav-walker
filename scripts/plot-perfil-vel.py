#!/usr/bin/env python3 

import numpy as np
import matplotlib.pyplot as plt
from collections import defaultdict
plt.style.use('/home/manuel/granular/unav-walker/utils/figs.mplstyle')
# plt.style.use('/home/manuel/granular/unav-sliders/unav-walker/utils/figs.mplstyle')

def leer_datos_histograma(nombre_archivo):
    """
    Lee el archivo de datos del histograma y devuelve los datos organizados
    """
    datos = []
    
    with open(nombre_archivo, 'r') as archivo:
        for linea in archivo:
            # Saltar líneas de comentario
            if linea.startswith('#'):
                continue
            
            # Dividir la línea en componentes
            partes = linea.strip().split()
            if len(partes) == 3:
                x = float(partes[0])
                y = float(partes[1])
                velocidad = partes[2]
                
                # Manejar valores NaN
                if velocidad.lower() == 'nan':
                    valor_velocidad = np.nan
                else:
                    valor_velocidad = float(velocidad)
                
                datos.append((x, y, valor_velocidad))
    
    return datos

def calcular_perfil_velocidad(datos, r=2.5):
    """
    Calcula el perfil de velocidad promedio para cada coordenada y
    considerando solo las celdas con |x| < r
    """
    # Diccionario para almacenar valores por coordenada y
    valores_por_y = defaultdict(list)
    
    # Filtrar datos dentro del rango en x y agrupar por y
    for x, y, velocidad in datos:
        if abs(x) < r and not np.isnan(velocidad):
            valores_por_y[y].append(velocidad)
    
    # Calcular promedios para cada y
    y_coords = []
    velocidades_promedio = []
    
    for y, valores in sorted(valores_por_y.items()):
        if valores:  # Solo incluir si hay valores válidos
            y_coords.append(y)
            velocidades_promedio.append(np.mean(valores))
    
    return np.array(y_coords), np.array(velocidades_promedio)

def graficar_perfil_velocidad(y_coords, velocidades_promedio, r=2.5):
    """
    Grafica el perfil de velocidad
    """
    plt.figure(figsize=(10, 8))
    
    # Crear el gráfico
    plt.plot(y_coords, velocidades_promedio * 100, 'b-', linewidth=2)
    plt.plot(y_coords, velocidades_promedio * 100, 'ro', markersize=6, alpha=0.7)
    
    # Configurar etiquetas y título
    plt.ylabel(r'Velocidad Promedio [cm/s]', fontsize=18)
    plt.xlabel(r'$y$ [cm]', fontsize=18)
    plt.title(fr'Perfil de velocidad a lo largo del eje $y$ (Promedio para $|x| < {r}$ cm)', fontsize=14)
    
    # Añadir grid
    plt.grid(True, alpha=0.3)
    
    # Añadir leyenda
    plt.legend(fontsize=10)
    
    # Ajustar layout
    plt.tight_layout()
    
    # Mostrar el gráfico
    # plt.show()
    plt.savefig('vel-profile.pdf')

def main():
    # Parámetros
    nombre_archivo = 'pressure_histogram_data.txt'
    r = 1.5  # Radio para filtrar en dirección x
    
    # Leer datos
    print("Leyendo datos del archivo...")
    datos = leer_datos_histograma(nombre_archivo)
    print(f"Se leyeron {len(datos)} puntos de datos")
    
    # Calcular perfil de velocidad
    print(f"Calculando perfil de velocidad para |x| < {r}...")
    y_coords, velocidades_promedio = calcular_perfil_velocidad(datos, r)
    
    print(f"Perfil calculado con {len(y_coords)} puntos")
    print(f"Rango de Y: {y_coords.min():.2f} a {y_coords.max():.2f}")
    print(f"Rango de velocidad: {velocidades_promedio.min():.6f} a {velocidades_promedio.max():.6f}")
    
    # Graficar resultados
    graficar_perfil_velocidad(y_coords, velocidades_promedio, r)
    
    # Opcional: Mostrar algunos valores
    print("\nPrimeros 10 valores del perfil:")
    print("Y\t\tVelocidad Promedio")
    for i in range(min(10, len(y_coords))):
        print(f"{y_coords[i]:.1f}\t\t{velocidades_promedio[i]:.6f}")

if __name__ == "__main__":
    main()
