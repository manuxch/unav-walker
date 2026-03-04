#!/usr/bin/env python3

import os
import numpy as np
import matplotlib.pyplot as plt
from glob import glob

def procesar_archivos(directorio='.'):
    """
    Lee todos los archivos .ve en el directorio y calcula la magnitud de velocidad por celda.
    
    Args:
        directorio: Directorio donde están los archivos .txt
    """
    # Obtener todos los archivos .txt
    archivos = glob(os.path.join(directorio, '*.ve'))
    
    if not archivos:
        print(f"No se encontraron archivos .txt en {directorio}")
        return None, None, None
    
    print(f"Procesando {len(archivos)} archivos...")
    
    # Determinar los límites de las coordenadas
    x_min, x_max = float('inf'), float('-inf')
    y_min, y_max = float('inf'), float('-inf')
    
    # Primera pasada: encontrar límites
    for archivo in archivos:
        try:
            with open(archivo, 'r') as f:
                for linea in f:
                    if linea.startswith('#') or not linea.strip():
                        continue
                    
                    partes = linea.split()
                    if len(partes) >= 6:
                        x = float(partes[2])
                        y = float(partes[3])
                        
                        x_min = min(x_min, x)
                        x_max = max(x_max, x)
                        y_min = min(y_min, y)
                        y_max = max(y_max, y)
        except Exception as e:
            print(f"Error leyendo {archivo}: {e}")
            continue
    
    # Asegurar valores enteros para las celdas
    x_min = np.floor(x_min)
    x_max = np.ceil(x_max)
    y_min = np.floor(y_min)
    y_max = np.ceil(y_max)
    
    print(f"Rango de coordenadas: x=[{x_min:.2f}, {x_max:.2f}], y=[{y_min:.2f}, {y_max:.2f}]")
    
    # Crear matriz para acumular velocidades y contador de partículas
    n_x = int(x_max - x_min) + 1
    n_y = int(y_max - y_min) + 1
    
    velocidad_acumulada = np.zeros((n_y, n_x))
    contador_particulas = np.zeros((n_y, n_x))
    
    # Segunda pasada: procesar datos
    # for archivo in archivos[2000:5000]:
    for archivo in archivos:
        try:
            with open(archivo, 'r') as f:
                for linea in f:
                    if linea.startswith('#') or not linea.strip():
                        continue
                    
                    partes = linea.split()
                    if len(partes) >= 6:
                        x = float(partes[2])
                        y = float(partes[3])
                        vx = float(partes[4]) * 22.1359436211787
                        vy = float(partes[5]) * 22.1359436211787
                        
                        # Calcular magnitud de velocidad
                        velocidad = np.sqrt(vx**2 + vy**2)
                        
                        # Determinar celda
                        i_x = int(np.floor(x - x_min))
                        i_y = int(np.floor(y - y_min))
                        
                        # Asegurar índices dentro de los límites
                        i_x = max(0, min(i_x, n_x - 1))
                        i_y = max(0, min(i_y, n_y - 1))
                        
                        # Acumular velocidad
                        velocidad_acumulada[i_y, i_x] += velocidad
                        contador_particulas[i_y, i_x] += 1
        except Exception as e:
            print(f"Error procesando {archivo}: {e}")
            continue
    
    # Calcular velocidad promedio por celda (evitar división por cero)
    velocidad_promedio = np.zeros_like(velocidad_acumulada)
    mascara = contador_particulas > 0
    velocidad_promedio[mascara] = velocidad_acumulada[mascara] / contador_particulas[mascara]
    
    return velocidad_promedio, x_min, y_min

def generar_mapa_calor(velocidad_promedio, x_min, y_min, titulo='Mapa de Calor de Velocidad'):
    """
    Genera y muestra el mapa de calor.
    
    Args:
        velocidad_promedio: Matriz de velocidades promedio por celda
        x_min, y_min: Límites mínimos de las coordenadas
        titulo: Título del gráfico
    """
    if velocidad_promedio is None:
        print("No hay datos para graficar")
        return
    
    # Crear figura
    fig, ax = plt.subplots(figsize=(12, 10))
    
    # Crear mapa de calor
    im = ax.imshow(velocidad_promedio, 
                   extent=[x_min, x_min + velocidad_promedio.shape[1], 
                          y_min, y_min + velocidad_promedio.shape[0]],
                   origin='lower', interpolation='none',
                   aspect='auto',
                   cmap='hot_r')  # 'hot_r' es la versión invertida (rojo más caliente)
    
    # Añadir barra de color
    cbar = plt.colorbar(im, ax=ax)
    cbar.set_label('Magnitud de Velocidad Promedio', fontsize=12)
    
    # Configurar ejes
    ax.set_xlabel('Coordenada X', fontsize=12)
    ax.set_ylabel('Coordenada Y', fontsize=12)
    ax.set_title(titulo, fontsize=14, fontweight='bold')
    
    # Añadir grid para mostrar celdas de 1x1
    ax.grid(True, alpha=0.3, which='both', linestyle='--', linewidth=0.5)
    
    # Ajustar ticks para mostrar coordenadas enteras
    x_ticks = np.arange(x_min, x_min + velocidad_promedio.shape[1] + 1, 5)
    y_ticks = np.arange(y_min, y_min + velocidad_promedio.shape[0] + 1, 5)
    ax.set_xticks(x_ticks)
    ax.set_yticks(y_ticks)
    
    # Añadir anotación con resolución
    ax.text(0.02, 0.98, f'Resolución: 1x1 celdas\nCeldas: {velocidad_promedio.shape[1]}x{velocidad_promedio.shape[0]}',
            transform=ax.transAxes, fontsize=10,
            verticalalignment='top',
            bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8))
    
    plt.tight_layout()
    plt.show()
    
    # Información estadística
    print(f"\nEstadísticas del mapa de calor:")
    print(f"  - Dimensión: {velocidad_promedio.shape[1]}x{velocidad_promedio.shape[0]} celdas")
    print(f"  - Velocidad mínima: {np.min(velocidad_promedio[velocidad_promedio > 0]):.6f}")
    print(f"  - Velocidad máxima: {np.max(velocidad_promedio):.6f}")
    print(f"  - Velocidad promedio: {np.mean(velocidad_promedio[velocidad_promedio > 0]):.6f}")
    print(f"  - Celdas con datos: {np.sum(velocidad_promedio > 0)} de {velocidad_promedio.size}")

def main():
    """
    Función principal que ejecuta el análisis.
    """
    # Especificar directorio con los archivos .txt
    # Cambia esto al directorio que contiene tus archivos
    directorio = '.'  # Directorio actual
    
    # Procesar archivos
    velocidad_promedio, x_min, y_min = procesar_archivos(directorio)
    
    if velocidad_promedio is not None:
        # Generar mapa de calor
        generar_mapa_calor(velocidad_promedio, x_min, y_min)
        
        # Opcional: Guardar los datos procesados
        np.savetxt('velocidad_promedio.csv', velocidad_promedio, delimiter=',', fmt='%.6f')
        print("\nDatos guardados en 'velocidad_promedio.csv'")

if __name__ == "__main__":
    main()
