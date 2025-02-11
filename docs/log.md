# 2024.12.26

Este es la primera entrada del nuevo log en formato markdown.

Hice un reordenamiento del código para dejar el repositorio con el programa que voy
a utilizar en las simulaciones 2025. La imagen del repositorio, con sus códigos de prueba,
pasaron a `old-unav-walker` en la pc de escritorio del IFLYSIB.

A partir de aquí implementaré los cambios en la versión 2.0 del programa.

# 2024.02.10

Retomo las actividades en 2025. Lo primero es recalcular el perfil de velocidades considerando
solo la cantidad de veces que cada bin registró una velocidad, para el cálculo del perfil
promedio (en vez de dividir por todos los frames). 

Esto dio origen a la versión 2.1 del código.

Además hice un jupyter-notebook (vd-gamma-phi.ipynb) en `/doc/` del proyecto, para calcular 
en qué valor de $\phi$ obtengo la misma velocidad de drift $v_d$ para dos valores de 
$\Gamma$ distintos: 1 y 3. El resultado dio $\phi$ = 1.6329057179736992.
