# Versión 3.2

2026.09.29

- El ejecutable pasa a llamarse `unav-walkers` (antes `silo-vib`); se
  actualizan el Makefile, las pruebas, la documentación y `scripts/submit.tpl`.
- Correcciones en los mensajes de la salida estándar: "Inserción", signos de
  interrogación y tildes ("¿...? Sí."), línea de fricción grano-grano que se
  imprimía dos veces, descripción de g (carga normal sobre la base) y
  eliminación de unidades MKS (la simulación usa unidades reducidas).
- Los archivos de salida no cambian (idénticos a la versión 3.1).

# Versión 3.1

2026.09.29

Reorganización del código, sin cambios en los resultados (las salidas son
idénticas byte a byte a las de la versión 3.0):

- Formato con clang-format (`src/.clang-format`).
- Eliminación de código muerto (funciones sin uso, ramas de polígonos).
- `siloAux.cpp` se divide en módulos por tema; nombres uniformes (tipos en
  PascalCase, funciones y variables en snake_case), sin variables globales.
  La tabla de cambios de nombre está en el mensaje del commit a73e700.
- Pruebas (`make test`) y prueba de regresión (`tests/regression.sh`).
- Documentación: `src/README.md` (modelo, parámetros, salidas, convenciones,
  validación y estilo), comentarios Doxygen y `src/Doxyfile`.

# Versión 3.0

2026.09.29

Correcciones (afectan resultados de versiones anteriores):

- Lectura estricta de parámetros: una clave desconocida, repetida o mal
  formada, un parámetro obligatorio ausente o un valor fuera de rango terminan
  el programa. Antes, una clave ausente dejaba el miembro sin inicializar; en
  particular `friccion_silo` no figuraba en `params.in` ni en `params.tpl`, y
  la fricción de las paredes quedaba en 0 o en un valor basura (incluso
  negativo, lo que produce NaN en Box2D).
- Signo de la dirección tangencial: Box2D usa t = (n.y, -n.x). La fuerza
  sobre la pared (`compute_wall_force`) y el tensor de estrés
  (`save_tensors`) usaban la opuesta; la presión de los discos no cambia, pero
  sí las componentes de corte. Ahora todas las salidas usan
  `contact_point_force`.
- Fricción con la base (Karnopp): la rama de adherencia devolvía fuerza nula
  y `fric_b_s` no se usaba. Ahora aplica la fuerza de adherencia (limitada a
  mu_s N), con una banda de adherencia max(Cero_tol, mu_d g dt).
- Amortiguación rotacional: antes w se multiplicaba por `atenuacion_rotacional`
  en cada paso (el efecto dependía de dt). Ahora es la fricción de pivoteo
  de Coulomb de un disco apoyado en la base, (2/3) mu N R con presión
  uniforme, con adherencia estática como en Karnopp y los mismos coeficientes
  fric_b_s y fric_b_d. `atenuacion_rotacional` se elimina.
- Reinyección: los granos se colocan sin superposición y con velocidad nula;
  antes conservaban la velocidad y podían superponerse con otros granos.
- Llave mal ubicada en la lectura de granos: la validación de fricción y
  restitución y la construcción de vértices de polígonos no se ejecutaban.
  Por ahora solo se admiten discos (nLados = 1).

Cambios:

- `walkers-forceWall` se integra en `silo-vib` con `fondo_medicion: T`.
- Todos los archivos de un mismo instante comparten el número de frame (pasos
  desde `t_Register`); antes cada tipo de archivo usaba su propio contador.
- Cabecera de procedencia en todas las salidas: paso, tiempo, fase de la
  excitación, dt, hash de git y hash del archivo de parámetros.
- Contactos (`fc_*.dat`): se agregan la normal y los centros de A y B; con
  ROI se guardan todos los contactos de los granos cuyo centro está en el ROI.
- Tensores (`*.sxy`): se agregan la parte normal del tensor, posición, radio,
  masa, velocidades y número de contactos activos. Las primeras 5 columnas no
  cambian.
- Nuevo chequeo de balance de impulso por grano (`check_balance_freq`).
- Nueva opción `continuous_physics` (detección continua de colisiones, TOI),
  por defecto F. Antes siempre estaba activa: los impulsos de los subpasos TOI
  no quedan en los manifolds de contacto (fuerzas registradas incompletas) y
  la simulación es ~9 veces más lenta. Con dt = 0.005 el desplazamiento por
  paso es ~1e-3, muy menor que el radio.
- `restitucion_silo` se aplica a las paredes (Box2D usa el máximo entre
  grano y pared).
- `RNG::flip(p)` ahora usa p.

# Versión 2.5 

2026.02.24 

- Selección de finalización por cantidad de partículas que salieron.
- Modificación del archivo de salida de información de contacto

# Versión 2.4 

2025.12.11

- Mejora en la muestra de fecha/hora de comienzo/finalización.

# Versión 2.3 

2025.12.10

- Corrección de guardado de frames.

# Versión 2.2 

2025.11.24 

- Agregada la opción de reinyectar o no los granos.

# Versión 2.1

2025.02.10 

- El cálculo del prefil de velocidad en el orificio de salida ahora se calcula
dividiendo por el número de veces que se registró una velocidad en cada bin.

- Corrección del cálculo del tensor de estrés (faltaba dividir por el área del grano)

# Versión 2.0

2024-12-26 

El viejo desarrollo de este código, junto con sus versiones de prueba, fue movido a 

  `/old-unav-walker`

en la PC de escritorio del IFLYSIB (iflysib38). El código que estaba alojado en 
`/src-silo` pasa a estar en `/src` y allí se implementarán los cambios a utilizar
en las simulaciones de 2025.



# Versión 1.1 de src-silo/silo-vib
2024.11.26

- Incoporación de guardado de velocidades y energías de partículas en frames.
