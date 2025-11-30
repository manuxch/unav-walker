# Perfil de velocidades
[2025.11.23]

Corrí una simulación de prueba en el cluster del GMG. Traje los archivos de salida al directorio 

    /granular/unav-sliders/vel-profile
    
de la PC de casa.

Extraje las coordenadas y las velocidades. Hice videos de la dinámica y también analicé con el programa `granular_cmap_render` la magnitud de la velocidad.

Procesé el histograma de salida para obtener una figura, usando el script

    `plot-perfil-vel.py`
    
usando como input el histograma que está en el subdirectorio `renders` producido por `granular_cmap_render`.
    
Todos los resultados están en la carpeta 

    /unav-sliders/unav-walker/resultados
    
\TODO Tengo que emprolijar el análisis y los resultados en un solo directorio.

# Velocidades de partícula única
2025.11.17

Para ver si está bien parametrizado el sistema, ajusté los valores de las unidades reducidas a partir de los valores experimentales (eso está en `unav-walkers/utils/reduced_units.ods`).

Hice una simulación con pocas partículas he hice el seguimiento de su posición y velocidad. Como solo corrí unas 10 partículas, éstas no colisionan entre si.

Al hacer el seguimiento de las que inicialmente están lejos del "base", todas las velocidades de drift dieron iguales. Las figuras `vy-t-fit.pdf`, `vy-t-colision.pdf` y `vy-t-base.pdf` muestran distintos períodos del segimiento de la partícula 1: durante el desplazamiento libre (que ajusta la velocidad de drift), al colisionar con la base, y cuando queda rebotando contra la misma.

El ajuste obtenido de la velocidad de drift es similar al que midieron experimentalmente: 1 cm/s.

El script que realiza el análisis está en 

    /unav-walkers/scripts/anal-vel-single-particle.py
    

