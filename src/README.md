# unav-walkers

Simulación con [Box2D](https://box2d.org) 2.4.2 de la descarga de un silo
bidimensional de discos apoyados sobre una base vibrada.

## Modelo

**Geometría.** En coordenadas de la simulación, el silo ocupa `|x| <= R`,
`0 <= y <= H` y está cerrado arriba. El orificio de salida ocupa `|x| <= r`
en `y = 0` (ancho `D = 2r`), y los granos salen hacia `y < 0`.

**Plano horizontal.** Los granos son discos apoyados sobre una base
horizontal. La gravedad no actúa en el plano de la simulación: solo fija la
carga normal `N = m g` sobre la base, que entra en la fricción.

**Excitación.** La base vibra en la dirección `y` con una aceleración
bi-armónica (`base_excitation`):

    a(t) = gamma g [rho sin(w t) + (1 - rho) sin(2 w t + phi)],   w = 2 pi f

En la simulación la base se mueve con `-y(t)`. La asimetría de la
excitación produce un arrastre neto de los granos hacia el orificio.

**Fricción con la base** (`base_friction.hpp`). Usa los coeficientes
`fric_b_s` (estático) y `fric_b_d` (dinámico) de cada grano:

- *Traslación (Karnopp).* Mientras el grano está adherido a la base, la
  fuerza equilibra al resto de las fuerzas (contactos) hasta `mu_s N`.
  Cuando desliza, vale `-mu_d N v_rel/|v_rel|`.
- *Rotación (pivoteo de Coulomb).* Suponiendo presión uniforme sobre la cara
  del disco, el torque que se opone al giro es `(2/3) mu N R`, con la misma
  lógica de adherencia y deslizamiento. No depende de `dt`.

**Contactos.** Los contactos grano-grano y grano-pared los resuelve Box2D
(contactos rígidos con fricción de Coulomb y restitución). Box2D combina las
fricciones de dos cuerpos como `sqrt(f1 f2)` y las restituciones con el
máximo.

**Etapas.** Hasta `tBlock` el orificio está cerrado por una tapa
(deposición); en `tBlock` se retira. Los granos que caen por debajo de
`y = -10` se reinyectan en una posición libre al azar de la franja
`|x| <= 0.9 R`, `0.75 H <= y <= 0.95 H`, con velocidad nula (o se eliminan,
con `do_reinyection: F`).

**Fondo de medición.** Con `fondo_medicion: T` el silo queda cerrado por un
fondo de ancho completo que nunca se retira, y la fuerza de los granos sobre
el fondo se registra en cada paso.

**Limitaciones conocidas:**

- Solo se admiten discos.
- Fricción de traslación y de rotación independientes: se ignora el
  acoplamiento real de la fricción seca (efecto Contensou).
- La detección continua de colisiones (TOI) está desactivada por defecto,
  porque sus impulsos no quedan registrados en los contactos. Con
  `dt = 0.005` el desplazamiento por paso (~1e-3) es muy menor que el radio.

## Compilación y uso

Desde la raíz del repositorio (compila también las herramientas de
análisis):

```bash
cmake -S . -B build                    # configuración (Release por defecto)
cmake -S . -B build -DBOX2D_ROOT=/ruta # con otra instalación de Box2D 2.4.2
cmake --build build -j                 # ejecutables en bin/
ctest --test-dir build                 # pruebas (ver Validación)
bin/unav-walkers params.in             # corre la simulación
```

El ejecutable se enlaza estáticamente, para poder copiarlo al cluster
(`-DUNAV_STATIC=OFF` para enlace dinámico). La versión (hash de git) se
actualiza en cada compilación.

La salida estándar reproduce los parámetros leídos y el avance de la
simulación. Los archivos se escriben en `frames_<dirID>/` (ver Salidas).

## Estructura del código

| Archivo | Contenido |
|---|---|
| `main.cpp` | Bucle principal: etapas, salidas, descarga, `b2World::Step` |
| `global_setup.*` | Lectura, validación e impresión de los parámetros |
| `silo_builder.*` | Paredes, tapa o fondo de medición, y granos |
| `body_data.hpp` | Datos propios de cada cuerpo (`BodyData`) e identificadores `kGid*` |
| `base_friction.*` | Excitación de la base, Karnopp y fricción de pivoteo |
| `contacts.*` | Fuerza de contacto a partir de los impulsos de Box2D |
| `discharge.*` | Conteo de descargados, reinyección, packing fraction, perfiles en el orificio |
| `output.*` | Archivos por frame y cabecera de procedencia |
| `diagnostics.*` | Chequeo del balance de impulso por grano |
| `rng.*` | Generador de números aleatorios (`std::mt19937`) |
| `tests/` | Pruebas y regresión |

Cada encabezado documenta sus funciones en formato Doxygen. Con Doxygen
instalado, `cmake --build build --target doc` genera la documentación del
simulador y de las herramientas en `build/doc/html/`.

## Parámetros

Formato: una línea `clave: valor` por parámetro. Los comentarios empiezan con
`#` o `//` y pueden ir al final de la línea. La lectura es estricta: una
clave desconocida, repetida o mal formada, un parámetro obligatorio ausente o
un valor fuera de rango terminan el programa con un mensaje de error.
`params.in` es un ejemplo completo y comentado.

| Clave | Oblig. | Descripción |
|---|---|---|
| `rand_seed` | sí | Semilla (> 0); la simulación es reproducible |
| `altura_silo` | sí | Altura `H` |
| `radio_silo` | sí | Semiancho `R` |
| `radio_out_silo` | sí | Semiancho del orificio `r` (`0 < r < R`) |
| `restitucion_silo` | sí | Restitución de las paredes |
| `friccion_silo` | sí | Fricción de las paredes; con el valor grano-grano, la fricción grano-pared es igual a la grano-grano |
| `Amplitud_exitacion_gamma` | sí | Aceleración reducida `gamma` |
| `Frecuencia_exitacion` | sí | Frecuencia `f` del primer armónico |
| `Cero_tol` | sí | Umbral de velocidad para la adherencia a la base |
| `rho` | sí | Peso del primer armónico (`0 < rho < 1`) |
| `fase_phi` | no (0) | Fase del segundo armónico |
| `noTipoGranos` | sí | `N` tipos de granos, seguido de `N` líneas `noGranos radio nLados dens fric fric_b_s fric_b_d rest` (`nLados = 1`) |
| `timeStep` | sí | Paso temporal `dt` |
| `tMax` | sí | Tiempo máximo |
| `tBlock` | sí | Tiempo con el orificio cerrado |
| `t_Register` | sí | Comienzo de los registros |
| `maxGranosDesc` | no (0) | Detener al descargar este número de granos (0: no) |
| `pIter`, `vIter` | sí | Iteraciones de posición y velocidad de Box2D |
| `g` | sí | Gravedad (carga normal sobre la base) |
| `do_reinyection` | sí | `T`: reinyectar; `F`: eliminar los descargados |
| `fondo_medicion` | no (F) | Silo cerrado con fondo de medición |
| `continuous_physics` | no (F) | Detección continua de colisiones (TOI) |
| `dirID` | sí | Directorio de salida `frames_<dirID>/` |
| `preFrameFile` | sí | Prefijo de los archivos de frames |
| `saveFrameFreq` | sí | Frecuencia de `.xy` |
| `fluxFile`, `fluxFreq` | sí | Archivo de granos descargados (se escribe si `fluxFreq > 0`) |
| `packing_fraction_out_freq`, `pf_file` | no (0) | Packing fraction |
| `freq_perfiles`, `n_bin_perfiles` | no (0) | Perfiles de ocupación y velocidad sobre el orificio |
| `save_ve_freq` | no (0) | Frecuencia de `.ve` |
| `freq_save_contacts` | no (0) | Frecuencia de `fc_*.dat` |
| `save_tensors_freq` | no (0) | Frecuencia de `.sxy` |
| `check_balance_freq` | no (0) | Frecuencia del chequeo de balance |
| `save_roi_only` | no (F) | Guardar solo el ROI `|x| <= x_roi`, `y_min_roi <= y <= y_max_roi` |
| `x_roi`, `y_min_roi`, `y_max_roi` | con ROI | Límites del ROI |

Las frecuencias se expresan en pasos; 0 deshabilita la salida.

## Salidas

Los archivos por frame se numeran con `n_frame`, la cantidad de pasos desde
`t_Register`, así que **los archivos de un mismo instante comparten el
número**. Todos empiezan con una cabecera de procedencia:

    # nStep: <paso> n_frame: <frame> t: <t> dt: <dt> fase: <w t mod 2pi> (w t mod 2pi, rad)
    # git: <hash de git> params: <archivo> params_hash: <hash FNV-1a del archivo>

Con `save_roi_only: T`, las salidas por grano incluyen los granos cuyo
centro está en el ROI, y `fc_*.dat` incluye **todos** los contactos de esos
granos (aunque el punto de contacto quede fuera).

| Archivo | Columnas |
|---|---|
| `<pre>_<frame>.xy` | granos: `gid 1 x y radio tipo`; paredes: `gid 2 x1 y1 x2 y2 etiqueta` |
| `<pre>_<frame>.ve` | `gid tipo x y vx vy w E_kin_lin E_kin_rot` |
| `fc_<pre>_<frame>.dat` | `gid_A gid_B cp.x cp.y Fn Ft nx ny xA yA xB yB n_pc tipo` |
| `<pre>_<frame>.sxy` | `gid sxx sxy syx syy snxx snxy snyx snyy x y r m vx vy w z_gg z_gw` |
| `balance_<pre>.dat` | `t nStep n_grains rel_lin max_rel_lin rel_ang max_rel_ang n_bad rel_lin_flip rel_ang_flip` |
| `wall_force_<pre>.dat` | `t Fx Fy |F|` (fuerza de los granos sobre el fondo de medición) |
| `fluxFile` | `granos_descargados tipo t total_tipo_1 ... total` |
| `pf_file` | `t pf_bulk pf_out` |

Identificadores (`gid`): granos `>= 0`, paredes `-100`, tapa `-110`, fondo de
medición `-200`. En `fc_*.dat`, `tipo` es `GG` (grano-grano) o `GW`
(grano-pared); para una pared, `(xA, yA)` o `(xB, yB)` es el punto de
contacto.

## Convenciones

**Fuerza de contacto** (`contacts.hpp`). Es la misma convención de Box2D:

- la normal `n = (nx, ny)` apunta de A a B;
- la tangente es `t = (ny, -nx)`;
- la fuerza sobre B es `F = Fn n + Ft t`, y sobre A es `-F`;
- `Fn` y `Ft` son los impulsos del último paso divididos por `dt`.

**Tensor de estrés por grano** (`.sxy`).

    s_ij = (1 / A_grano) sum_c f_i l_j

- `f` es la fuerza de contacto sobre el grano y `l` = punto de contacto −
  centro del grano.
- `A_grano` es el área del grano: es el estrés sobre el grano, no un
  promedio de continuo.
- Compresión `< 0`; la presión es `-(sxx + syy) / 2`.
- `sn` es la parte debida solo a las fuerzas normales; la tangencial es
  `s - sn`. En discos, `sn` es simétrico y la parte tangencial tiene traza
  nula.
- Incluye los contactos con las paredes.

**Coordenadas.** El eje `y` de la simulación es opuesto al de los
experimentos. La conversión a unidades experimentales se hace en el
análisis.

## Validación

- `ctest --test-dir build` corre:
  - pruebas unitarias de la fricción con la base (Karnopp y pivoteo,
    incluida la independencia respecto de `dt`);
  - una prueba de la reinyección (sin superposiciones y con velocidad nula);
  - una prueba del lector de parámetros (cada tipo de error y un archivo
    válido).
- `tests/regression.sh <binario_ref> <binario_nuevo>` compara byte a byte
  las salidas de dos versiones en tres configuraciones. Úselo para verificar
  que un cambio de formato, organización o documentación no altera los
  resultados.
- `check_balance_freq > 0` verifica durante la simulación que, para cada
  grano, `m Δv = dt F_base + Σ J_c` e `I Δw = dt tau_base + Σ l_c × J_c`,
  con los impulsos de contacto reconstruidos. Sin TOI, los residuos
  relativos típicos son ~1e-5 (lineal) y ~1e-6 (angular). Las columnas
  `*_flip` repiten el cálculo con la tangente invertida y deben dar residuos
  grandes: esto confirma la convención de signos.

## Estilo

- Formato con `clang-format` (`.clang-format`, estilo LLVM, 80 columnas):
  `clang-format -i *.cpp *.hpp tests/*.cpp`.
- Tipos en `PascalCase`; funciones y variables en `snake_case`; constantes
  `kNombre`. Se conservan los símbolos físicos habituales (`H`, `R`, `r`,
  `N`, `I`).
- Encabezados con `#pragma once`, sin `using` y con documentación Doxygen de
  cada función pública.
- Sin variables globales: los parámetros (`const GlobalSetup &`) y el
  generador aleatorio se pasan como argumento.
- Cada versión se registra en `../changelog.md`.
