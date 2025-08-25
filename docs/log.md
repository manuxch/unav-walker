# 2025.03.28

- Hice nuevas corridas para $\Gamma = 1, 3$, pero ahora con el coeficiente de restitución
$\nu = 0.95$, a diferencia de las simulaciones previas con $\nu = 0.5$.

La curva de presión en función de $y$ no muestra diferencia con el caso anterior. Los 
puntos son indistinguibles visualmente. Subí a la carpeta de Drive Walkers/stress 
dos archivos con las animaciones de estas nuevas simulaciones en la zona del orificio de 
salida, y actualicé la figura p-vs-y.pdf incorporando los nuevos perfiles de presión.


# 2025.03.20

- Hice una nueva corrida con $\Gamma = 1$ y otra semilla del generador de números aleatorios
para ver cómo es el estrés cuando no hay (casi) atascamientos. La nueva corrida está en:

    ~/granular/run-unav-walker/stress/G1-G3/gamma-1-2/

El resultado del perfil de presión con la altura se ve muy similar a la corrida anterior con 
$\Gamma = 1$ que estaba casi siempre atascada. En mi opinión, lo que sucede cerca del orificio
casi no afecta al estado de tensión general del sistema. El pico que aparece en el otro extremo
(opuesto a la salida) me parece que es porque al reinyectar puede haber overlap con otros granos.


# 2025.02.17

- Corregí el cálculo del tensor de estrés en siloAux.cpp, porque faltaba dividir por el área
de los granos. Incluí una función auxiliar para eso.

# 2025.02.13

- Hice corridas para $\Gamma$ = 1 y 3, con el valor de $phi$ determinado para que ambas velocidades
de drift sean iguales. Las simulaciones están en:

    ~/granular/run-unav-walker/test-vd-G1-G3/
    
Lo que se obtiene es que las velocidades medias no son iguales:
  - $\Gamma = 1$: $|v_d| = 1.14 cm/s$
  - $\Gamma = 3$: $|v_d| = 1.41 cm/s$

esto es casi un 24% de diferencia.

# 2025.02.10

Retomo las actividades en 2025. Lo primero es recalcular el perfil de velocidades considerando
solo la cantidad de veces que cada bin registró una velocidad, para el cálculo del perfil
promedio (en vez de dividir por todos los frames). 

Esto dio origen a la versión 2.1 del código.

Además hice un jupyter-notebook (vd-gamma-phi.ipynb) en `/docs/` del proyecto, para calcular 
en qué valor de $\phi$ obtengo la misma velocidad de drift $v_d$ para dos valores de 
$\Gamma$ distintos: 1 y 3. El resultado dio $\phi$ = 1.6329057179736992.

Como referencia utilizo la ec. 7 del PRL 2024

# 2025.07.06

Hice algunos cambios menores para retomar las simulaciones y comparar con los experimentos.

- Modifiqué la forma de registrar incorporando una variable `t_register` que es el tiempo a partir del cual se comienzan a registrar cosas, para sacar el período transitorio.
- Hice un script `heatmap-velocities.py` para realizar mapas de calor de velocidades, pero se puede adaptar para otras magnitudes (por ejemplo stress).

# 2024.12.26

Este es la primera entrada del nuevo log en formato markdown.

Hice un reordenamiento del código para dejar el repositorio con el programa que voy
a utilizar en las simulaciones 2025. La imagen del repositorio, con sus códigos de prueba,
pasaron a `old-unav-walker` en la pc de escritorio del IFLYSIB.

A partir de aquí implementaré los cambios en la versión 2.0 del programa.


# 2024.07.29

Terminaron las simulaciones que lancé el 2024.07.25, e hice los análisis correspondientes.
Los resultados están en 
    ~/granular/run-unav-walker/v_0
    ~/granular/run-unav-walker/igual_fric_s_d

Ambos resultados no muestran casi diferencias con el caso de referencia.

Subí las figuras y los archivos con los datos al Drive de UNAV, y les envié un email
a los Maza.


# 2024.07.25

- Lanzo una corrida en slurm con diferentes valores para el "cero" de velocidad:
(1e-4 y 1e-3).

- Lanzo una corrida en slurm para ver el efecto que tiene igualar el coeficiente 
de fricción estática igualado con el de fricción dinámica.


# 2024.07.05

- Terminó la corrida y guardo los resultados en 
    /home/manuel/granular/run-unav-walker/resultados/res-1000-g15-Q-D-bidisperso/

- Lanzo un conjunto de simulaciones pero ahora con el mismo tamaño y dos
coeficientes de fricción dinámica con la bases distintos: 0.19 y 0.13.

- Hice un análisis del flujo de descarga en función del tamaño con el script
    /home/manuel/granular/unav-walker/scripts/plot-caudales-D.py
y no se ve que haya una diferencia significativa entre ambos tamaños.
    /home/manuel/granular/run-unav-walker/resultados/res-1000-g15-Q-D-bidisperso/Q-D.pdf


# 2024.07.04

Lanzo un conjunto de simulaciones en SLURM para analizar el efecto del 
tamaño de granos. El sistema es bidisperso, con RL/Rs = 1/0.8.
    /home/mcarlevaro/unav-walker
