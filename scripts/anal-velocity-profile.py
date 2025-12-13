#!/usr/bin/env python3
'''
Script para analizar las salidas .ve de la simulación de los walkers
y hacer el pewrfil de velocidad sobre una franja central de ancho fijo.
'''

import numpy as np
import matplotlib.pyplot as plt
import glob
import os

def calcular_perfil_velocidad(xr, n_bins=50, y_min=None, y_max=None):
    """
    Calcula el perfil de magnitud de velocidad a lo largo del eje Y
    
    Parameters:
    -----------
    xr : float
        Ancho de la franja en X (-xr, xr)
    n_bins : int
        Número de bins para el perfil en Y
    y_min, y_max : float
        Límites del eje Y (si None, se determinan automáticamente)
    """
    
    # Lista para almacenar todos los datos
    todas_posiciones_y = []
    todas_magnitudes_velocidad = []
    
    # Encontrar todos los archivos txt en el directorio actual
    archivos_txt = glob.glob("*.ve")
    
    if not archivos_txt:
        print("No se encontraron archivos .txt en el directorio actual")
        return
    
    print(f"Procesando {len(archivos_txt)} archivos...")
    
    for archivo in archivos_txt:
        # nro_frame = int(archivo[-9:].split('.')[0])
        # if nro_frame < 500:
        #     continue
        try:
            # Leer el archivo, saltando la primera línea si es un comentario
            datos = np.loadtxt(archivo, comments='#')
            
            # Verificar que el archivo tiene datos
            if datos.size == 0:
                continue
                
            # Extraer columnas relevantes
            # Columna 1: ID, Columna 2: type, Columna 3: x, Columna 4: y, 
            # Columna 5: vx, Columna 6: vy
            x_pos = datos[:, 2] # Columna 3: posición x en cm
            y_pos = datos[:, 3] # Columna 4: posición y en cm
            vx = datos[:, 4] * 22.1359436211787    # Columna 5: velocidad x en cm/s
            vy = datos[:, 5] * 22.1359436211787    # Columna 6: velocidad y en cm/s
            
            # Filtrar partículas dentro de la franja en X
            mascara_franja = (x_pos >= -xr) & (x_pos <= xr)
            
            y_filtrado = y_pos[mascara_franja]
            vx_filtrado = vx[mascara_franja]
            vy_filtrado = vy[mascara_franja]
            
            # Calcular magnitud de velocidad
            magnitud_vel = np.sqrt(vx_filtrado**2 + vy_filtrado**2)
            
            # Agregar a las listas generales
            todas_posiciones_y.extend(y_filtrado)
            todas_magnitudes_velocidad.extend(magnitud_vel)
            
        except Exception as e:
            print(f"Error al procesar {archivo}: {e}")
            continue
    
    # Convertir a arrays numpy
    todas_posiciones_y = np.array(todas_posiciones_y)
    todas_magnitudes_velocidad = np.array(todas_magnitudes_velocidad)
    
    if len(todas_posiciones_y) == 0:
        print("No se encontraron partículas dentro de la franja especificada")
        return
    
    # Determinar límites de Y si no se especifican
    if y_min is None:
        y_min = np.min(todas_posiciones_y)
    if y_max is None:
        y_max = np.max(todas_posiciones_y)
    
    # Crear bins para el perfil
    bins_y = np.linspace(y_min, y_max, n_bins + 1)
    centros_bins = (bins_y[:-1] + bins_y[1:]) / 2
    
    # Calcular promedio de velocidad en cada bin
    promedio_velocidad = np.zeros(n_bins)
    desviacion_velocidad = np.zeros(n_bins)
    conteo_partículas = np.zeros(n_bins)
    
    for i in range(n_bins):
        mascara_bin = (todas_posiciones_y >= bins_y[i]) & (todas_posiciones_y < bins_y[i+1])
        if np.sum(mascara_bin) > 0:
            velocidades_bin = todas_magnitudes_velocidad[mascara_bin]
            promedio_velocidad[i] = np.mean(velocidades_bin)
            desviacion_velocidad[i] = np.std(velocidades_bin)
            conteo_partículas[i] = len(velocidades_bin)
        else:
            promedio_velocidad[i] = np.nan
            desviacion_velocidad[i] = np.nan
    
    # Guardar resultados en archivo
    nombre_archivo_salida = f"perfil_velocidad_xr{xr}.dat"
    with open(nombre_archivo_salida, 'w') as f:
        f.write("# Y_center Velocidad_promedio Desviacion_estandar Conteo_particulas\n")
        for i in range(n_bins):
            if not np.isnan(promedio_velocidad[i]):
                f.write(f"{centros_bins[i]:.6f} {promedio_velocidad[i]:.6f} "
                       f"{desviacion_velocidad[i]:.6f} {conteo_partículas[i]:.0f}\n")
    
    # Graficar resultados
    plt.figure(figsize=(10, 6))
    
    # Perfil de velocidad promedio
    plt.subplot(2, 1, 1)
    mascara_valida = ~np.isnan(promedio_velocidad)
    plt.plot(centros_bins[mascara_valida], promedio_velocidad[mascara_valida], 
             'b-', linewidth=2, label='Velocidad promedio')
    plt.fill_between(centros_bins[mascara_valida], 
                    promedio_velocidad[mascara_valida] - desviacion_velocidad[mascara_valida],
                    promedio_velocidad[mascara_valida] + desviacion_velocidad[mascara_valida],
                    alpha=0.3, label='±1 desviación estándar')
    plt.ylabel('Magnitud de velocidad')
    plt.title(f'Perfil de velocidad - Franja X: [-{xr}, {xr}]')
    plt.legend()
    plt.grid(True, alpha=0.3)
    
    # Conteo de partículas por bin
    plt.subplot(2, 1, 2)
    plt.bar(centros_bins, conteo_partículas, width=(y_max-y_min)/n_bins*0.8, 
            alpha=0.7, color='red')
    plt.xlabel('Posición Y')
    plt.ylabel('Número de partículas')
    plt.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(f'perfil_velocidad_xr{xr}.png', dpi=300, bbox_inches='tight')
    plt.show()
    
    # Estadísticas generales
    print(f"\n--- Estadísticas para franja X = [-{xr}, {xr}] ---")
    print(f"Total de partículas analizadas: {len(todas_posiciones_y)}")
    print(f"Rango Y: [{y_min:.3f}, {y_max:.3f}]")
    print(f"Velocidad promedio global: {np.mean(todas_magnitudes_velocidad):.6f}")
    print(f"Desviación estándar global: {np.std(todas_magnitudes_velocidad):.6f}")
    print(f"Resultados guardados en: {nombre_archivo_salida}")

def main():
    """
    Función principal - modifica los parámetros aquí
    """
    # PARÁMETROS A MODIFICAR
    xr = 1.5        # Ancho de la franja en X (-xr, xr)
    n_bins = 50      # Número de bins para el perfil
    y_min = None     # Límite inferior de Y (None = automático)
    y_max = None     # Límite superior de Y (None = automático)
    
    calcular_perfil_velocidad(xr, n_bins, y_min, y_max)

if __name__ == "__main__":
    main()
