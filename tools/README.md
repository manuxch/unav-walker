# Herramientas de análisis y visualización

Programas en C++20 y scripts en Python para analizar las salidas del
simulador `unav-walkers` (versión >= 3.0). El formato de las salidas está
descrito en [`../src/README.md`](../src/README.md).

- **Programas en C++** (`bin/`): leen los archivos por frame de una
  simulación, promedian en paralelo sobre todos los frames y escriben
  perfiles o mapas en texto, en **unidades de la simulación**.
- **Scripts en Python** (`tools/scripts/`): grafican esas salidas en
  **unidades experimentales**, con los factores de un único módulo,
  `unidades.py`.

## Contenido

| Herramienta | Entrada | Salida | Para qué |
|---|---|---|---|
| `bin/read_demo` | `.xy`, `.ve`, `fc_*.dat` de un frame | texto en pantalla | Inspeccionar un frame y verificar el lector |
| `bin/vel_profile` | `<pre>_*.ve` | perfil en `y` | Velocidades y energías medias en una franja vertical |
| `bin/force_profile` | `fc_<pre>_*.dat` | perfil en `y` | Fuerzas de contacto medias en una franja vertical |
| `bin/force_map2d` | `fc_<pre>_*.dat` | mapa `(x, y)` | Fuerzas de contacto medias en todo el silo |
| `bin/stress_profile` | `<pre>_*.sxy` | perfil en `y` (y por fase) | Tensor de estrés (contacto y cinético) en una franja vertical |
| `scripts/plot-perfil.py` | salidas de `vel_profile` | `perfiles-vy.pdf` | Perfiles de `v_y` para varios `D` |
| `scripts/plot-fuerzas.py` | salidas de `force_profile` | `perfiles-fn-ft.pdf` | Perfiles de `Fn` y `\|Ft\|` para varios `D` |
| `scripts/plot-mapa-fuerzas.py` | salida de `force_map2d` | figura | Mapas de `Fn` y `\|Ft\|` |
| `scripts/plot_stress_profile.py` | salidas de `stress_profile` | figura | Perfiles del tensor de estrés para varios `D` |
| `scripts/unidades.py` | — | — | Conversión a unidades experimentales |
| `dem_reader` (biblioteca) | todos los formatos | — | Lectura de archivos para programas nuevos |

La visualización de la dinámica (imágenes y videos de los frames `.xy`) se
hace con `granular_render`, un repositorio aparte.

## Requisitos y compilación

- CMake >= 3.20 y un compilador con C++20 (GCC >= 11).
- Python 3 con numpy y matplotlib.
- LaTeX y la fuente Linux Libertine O para los gráficos. Los scripts
  anteriores la requieren siempre; `plot_stress_profile.py` admite
  `--sin-tex`.

Desde la raíz del repositorio (compila también el simulador):

```bash
cmake -S . -B build
cmake --build build -j
```

Los ejecutables quedan en `bin/`, en la raíz del repositorio.

## Convenciones comunes

**Archivos.** Todas las herramientas reciben el directorio de salida de una
simulación (`frames_<dirID>`) y el prefijo `<pre>`, que es el
`preFrameFile` del archivo de parámetros. Leen los archivos
`<pre>_<frame>.<ext>`, y `fc_<pre>_<frame>.dat` para los contactos.

| `preFrameFile` | Archivos que se leen |
|---|---|
| `frm` | `frm_*.xy`, `frm_*.ve`, `frm_*.sxy`, `fc_frm_*.dat` |
| `frm-100` (corridas anteriores) | `frm-100_*.xy`, ..., `fc_frm-100_*.dat` |

Los demás archivos del directorio (`balance_*.dat`, `wall_force_*.dat`) se
ignoran.

**Promedios.** Los programas promedian sobre **todos los frames** del
directorio. Para restringir el intervalo de tiempo con `force_profile`,
`vel_profile` o `force_map2d`, hay que ajustar `t_Register` en la
simulación o mover los archivos. `stress_profile` tiene `--t-min` y
`--t-max`.

**Unidades y coordenadas.** Los programas escriben en unidades de la
simulación: longitudes en diámetros `d`, fuerzas en `m g`, estrés 2D en
`m g / d`. El origen está en el centro del orificio y `y` crece hacia
arriba, dentro del silo. Los scripts convierten a unidades experimentales
e invierten el eje `y` (ver [Unidades](#unidades-scriptsunidadespy)).

**Formato de salida.** Texto compatible con `numpy.loadtxt`: las líneas de
cabecera empiezan con `#` y la última de ellas nombra las columnas.

**Franja central.** Para el perfil sobre el orificio se usa una franja de
semiancho `D/2 = radio_out_silo`. Según la herramienta, la selección es por
el punto de contacto (`force_profile`) o por el centro del grano
(`vel_profile`, `stress_profile`).

## Pipeline de uso

Ejemplo completo para una corrida con `D = 3 d` (`radio_out_silo: 1.5`),
`H = 30`, frecuencia `f = 0.45175395` y los archivos en `frames_walker/`
con prefijo `frm`.

### 1. Simulación

Estas son las salidas que usan las herramientas. Las frecuencias se dan en
pasos; con `dt = 0.005` un período tiene ~443 pasos:

```text
t_Register: 867.8          // tMax - 10 períodos: promedio sobre ~10 oscilaciones
saveFrameFreq: 20          // .xy   -> granular_render, read_demo
save_ve_freq: 20           // .ve   -> vel_profile
freq_save_contacts: 20     // fc_   -> force_profile, force_map2d
save_tensors_freq: 20      // .sxy  -> stress_profile
check_balance_freq: 100    // control de la simulación (balance_frm.dat)
save_roi_only: F           // o T con un ROI que contenga la franja (ver nota)
```

```bash
cd corrida_D3/
/ruta/a/unav-walker/bin/unav-walkers params.in > salida.log
```

Con 20 pasos entre frames hay ~22 muestras por período, unos 220 frames en
10 períodos.

> **ROI.** Si se usa `save_roi_only: T` para ahorrar espacio, el ROI tiene
> que contener la franja de análisis. Para `stress_profile` y
> `vel_profile` alcanza con `x_roi >= D/2`. Para `force_profile` hace
> falta `x_roi >= D/2 + 0.5`: se guardan los contactos de los granos con
> centro en el ROI, y un punto de contacto puede estar hasta un radio
> fuera del centro.

### 2. Controles rápidos

```bash
B=/ruta/a/unav-walker/bin
$B/read_demo frames_walker frm 20          # resumen del frame 20: granos, paredes, contactos
tail -3 frames_walker/balance_frm.dat      # rel_lin ~1e-5 y rel_ang ~1e-6 si todo está bien
```

La dinámica se visualiza renderizando los `.xy` con `granular_render`.

### 3. Perfiles y mapas (unidades de la simulación)

```bash
# Velocidad vertical media en la franja |x| <= D/2, bines de un diámetro
$B/vel_profile frames_walker frm 1.5 30 0 30 perfil-ve-30.dat

# Fuerzas de contacto (Fn y |Ft| en archivos separados, para plot-fuerzas.py),
# sin contactos con paredes ni contactos inactivos
$B/force_profile frames_walker frm 1.5 30 0 30 perfil-fn-30.dat --qty norm --no-walls --active-only
$B/force_profile frames_walker frm 1.5 30 0 30 perfil-ft-30.dat --qty tan  --no-walls --active-only

# Todas las cantidades de fuerza en un solo archivo
$B/force_profile frames_walker frm 1.5 30 0 30 fuerzas-D30.dat --qty norm tan fmag fx fy --no-walls --active-only

# Tensor de estrés en la franja, promediado y resuelto en fase
$B/stress_profile frames_walker frm -o estres-D30.dat --half-width 1.5 --y-max 30 \
    --freq 0.45175395 --phase-output estres-fase-D30.dat

# Mapa 2D de fuerzas de contacto en todo el silo (bines de un diámetro)
$B/force_map2d frames_walker frm 20 30 mapa-D30.dat --xmin -10 --xmax 10 --ymin 0 --ymax 30 \
    --no-walls --active-only
```

### 4. Gráficos (unidades experimentales)

```bash
S=/ruta/a/unav-walker/tools/scripts
python3 $S/plot-perfil.py                     # lee perfil-ve-*.dat  -> perfiles-vy.pdf
python3 $S/plot-fuerzas.py                    # lee perfil-fn-*.dat y perfil-ft-*.dat -> perfiles-fn-ft.pdf
python3 $S/plot-mapa-fuerzas.py mapa-D30.dat -o mapa-D30.pdf
python3 $S/plot_stress_profile.py estres-D*.dat -o estres.pdf
```

### 5. Varios anchos de orificio

`plot-perfil.py` y `plot-fuerzas.py` grafican todos los archivos del
directorio actual y toman `D` del nombre, en décimas de diámetro:
`perfil-fn-30.dat` corresponde a `D = 3.0 d` y `perfil-fn-40.dat` a
`D = 4.0 d` (no `-04`, que sería `D = 0.4 d`). Los archivos cuyo sufijo no
es un número entero se ignoran con un aviso. Un lazo típico, con una
corrida por `D` en `corrida_D<D>/`:

```bash
for D in 2 3 5 8 12 18; do
  W=$(awk "BEGIN {print $D / 2}")            # semiancho de la franja
  dir=corrida_D$D/frames_walker
  tag=$((D * 10))
  $B/vel_profile   $dir frm $W 30 0 30 perfil-ve-$tag.dat
  $B/force_profile $dir frm $W 30 0 30 perfil-fn-$tag.dat --qty norm --no-walls --active-only
  $B/force_profile $dir frm $W 30 0 30 perfil-ft-$tag.dat --qty tan  --no-walls --active-only
  $B/stress_profile $dir frm -o estres-D$tag.dat --half-width $W --y-max 30 --freq 0.45175395
done
python3 $S/plot-perfil.py && python3 $S/plot-fuerzas.py
python3 $S/plot_stress_profile.py estres-D*.dat -o estres.pdf
```

`plot_stress_profile.py` toma la etiqueta `D = 2W` de la cabecera de cada
archivo, así que no depende del nombre.

---

## Programas en C++

### read_demo

Muestra el contenido de un frame: cantidad de frames disponibles, y para el
frame pedido, granos y paredes (`.xy`), velocidades y energías (`.ve`) y
contactos (`fc_*.dat`). Sirve para verificar que el directorio y el prefijo
son correctos. Si falta algún tipo de archivo (por ejemplo `.ve`, cuando
`save_ve_freq: 0`), lo informa y sigue con los demás.

```bash
bin/read_demo <dir> <pre> <frame_id>
bin/read_demo frames_walker frm 20
```

### vel_profile

Perfil vertical de velocidades y energías en la franja `|x| <= x_m`,
seleccionando los granos por su centro.

```bash
bin/vel_profile <dir> <pre> <x_m> <n_bins> <y_min> <y_max> <salida> \
    [--qty vy vx w speed Eklin Ekrot] [--threads N]
```

| Argumento | Descripción |
|---|---|
| `x_m` | Semiancho de la franja (`D/2` para el orificio) |
| `n_bins`, `y_min`, `y_max` | Bines de igual alto en `[y_min, y_max)` |
| `--qty` | `vy` (por defecto), `vx`, `w` (velocidad angular), `speed` (`\|v\|`), `Eklin`, `Ekrot` (energías cinéticas) |

Salida: `y_center qty1 qty2 ...`. Cada valor es la media sobre todos los
pares grano-frame del bin. **Los bines vacíos valen 0.** `plot-perfil.py`
espera exactamente dos columnas, así que para usarlo hay que correr con la
cantidad por defecto (`vy`).

### force_profile

Perfil vertical de fuerzas de contacto en la franja `|cp_x| <= x_m`,
seleccionando por el **punto de contacto**.

```bash
bin/force_profile <dir> <pre> <x_m> <n_bins> <y_min> <y_max> <salida> \
    [--qty norm tan fmag fx fy] [--no-walls] [--active-only] [--threads N]
```

| Opción | Descripción |
|---|---|
| `--qty norm` | `Fn`, componente normal (por defecto) |
| `--qty tan` | `\|Ft\|`, módulo de la componente tangencial |
| `--qty fmag` | `\|F\| = sqrt(Fn² + Ft²)` |
| `--qty fx`, `fy` | `\|F_x\|`, `\|F_y\|`: componentes cartesianas en valor absoluto (su signo depende de cuál de los dos cuerpos se considere) |
| `--no-walls` | Excluye los contactos grano-pared |
| `--active-only` | Excluye los contactos con `Fn = 0` |

Salida: `y_center qty1 qty2 ... n`. `n` es la cantidad de puntos de
contacto del bin sumada sobre los frames. Cada valor es la media sobre esos
puntos, todos con el mismo peso, y los bines vacíos valen `nan`. La
cabecera registra el prefijo, la franja, los bines y los filtros.

- **Filtros.** Box2D registra como "en contacto" pares que no transmiten
  fuerza (`Fn = 0`), y la franja incluye contactos con el fondo y las
  paredes. En una corrida de prueba, esos contactos eran el 23 % del total
  de la franja. Por eso conviene usar explícitamente `--no-walls` y
  `--active-only`, o no usarlos, según lo que se quiera medir.
- **Media por contacto.** Es una fuerza media por contacto, no una presión:
  no tiene en cuenta cuántos contactos hay por unidad de área. Para eso
  está `stress_profile`.

### force_map2d

Mapa de fuerzas de contacto medias en bines `(x, y)` de todo el silo, por
punto de contacto.

```bash
bin/force_map2d <dir> <pre> <n_bins_x> <n_bins_y> <salida> \
    [--xmin v --xmax v --ymin v --ymax v] [--qty norm tan fmag fx fy] \
    [--no-walls] [--active-only] [--threads N]
```

- Sin `--xmin`, `--xmax`, `--ymin` y `--ymax`, los límites se calculan
  recorriendo todos los contactos. Conviene fijarlos, por ejemplo
  `--xmin -R --xmax R --ymin 0 --ymax H`, para comparar mapas entre
  corridas.
- Cantidades y filtros como en `force_profile`; por defecto, `norm tan`.
- Salida: `x_center y_center qty1 qty2 ...`, una fila de `x` por bloque,
  con bloques separados por una línea en blanco (formato de gnuplot).
  Los bines vacíos valen `nan`.
- `plot-mapa-fuerzas.py` usa las columnas 3 y 4 como `Fn` y `|Ft|`, así que
  para usarlo hay que dejar la selección por defecto (`--qty norm tan`).

### stress_profile

Perfil vertical del **tensor de estrés** en la franja `|x - x_c| <= W`,
seleccionando los granos por su centro, a partir de los `.sxy`. El
simulador ya calcula el tensor de cada grano.

```bash
bin/stress_profile <dir> <pre> -o <salida> --half-width W --y-max Y --freq F \
    [--y-min 0] [--dy 1] [--x-center 0] [--t-min T0] [--t-max T1] \
    [--phase-bins 20] [--phase-output <archivo>] [--threads N]
```

| Argumento | Descripción |
|---|---|
| `--half-width W` | Semiancho de la franja (`D/2` para el orificio) |
| `--y-max Y`, `--y-min`, `--dy` | Rango y alto de los bines (por defecto, un diámetro) |
| `--freq F` | `Frecuencia_exitacion` de la simulación: define los bloques de un período para los errores y se verifica contra la fase de las cabeceras |
| `--t-min`, `--t-max` | Intervalo de tiempo a promediar |
| `--phase-bins P` | Bines de fase de la excitación (por defecto 20); ver la parte cinética |
| `--phase-output` | Escribe además el perfil resuelto en fase |

**Definiciones.** Índices `xx xy yx yy`, con `s_ij = sum f_i l_j`; el signo
es negativo en compresión. La deducción completa, con las propiedades de
las partes normal y tangencial, está en `docs/stress/estres.tex`.

- Tensor de contacto: `s = sum_p A_p s_p / sum_p A_p`, promediado sobre el
  área de los granos. Es el estrés sobre los granos. Se da el total `s`,
  la parte normal `sn` y la tangencial `st = s - sn`.
- Parte cinética, con la misma normalización para que se sume a `s`:
  `k = -sum_p m_p v'v' / sum_p A_p`. Las fluctuaciones `v'` se miden
  respecto de la velocidad media en cada bin de `y` y de fase, así que la
  oscilación coherente con la base no cuenta como agitación. Con
  `--phase-bins 1`, la referencia es la media temporal.
- `phi = sum A_p / (A_bin N_frames)` es la fracción de área, contada por
  centros. El estrés medio del bin es `phi (s + k)`.
  **`phi` no es una fracción de empaquetamiento física:** un grano con el
  centro en la franja aporta toda su área aunque sobresalga, así que en
  franjas angostas `phi` puede superar 1.
- Errores `e_*`: error estándar de las medias por bloques de un período.
  Con menos de ~10 períodos son poco confiables. Para `k`, las
  fluctuaciones de cada bloque se miden respecto de la misma media global
  por bin de `y` y de fase.
- En bines casi vacíos (un grano por celda de `y` y fase), `v' = 0` y `k`
  da cero: no es agitación nula, es falta de datos.

**Salida principal:** `y n phi vx vy sxx sxy syx syy snxx snxy snyx snyy
stxx stxy styx styy kxx kxy kyy e_sxx e_sxy e_syx e_syy e_snxx e_snxy e_snyx
e_snyy e_kxx e_kxy e_kyy`, con `n` = pares grano-frame y `nan` en los bines vacíos.

**Salida por fase** (`--phase-output`): `fase y n n_frames phi vx vy sxx sxy
syx syy snxx snxy snyx snyy kxx kxy kyy`, un bloque por bin de fase.

**Controles.** El programa termina con un mensaje de error si:

- los archivos provienen de distintas versiones del simulador o de distintos
  archivos de parámetros;
- la fase registrada en las cabeceras no coincide con `--freq`;
- algún archivo tiene el formato anterior a la versión 3.0.

---

## Scripts de gráficos

Todos convierten a unidades experimentales con `unidades.py`: `y` en cm,
con el eje invertido y el origen en el orificio; velocidades en cm/s;
fuerzas en N; estrés 2D en N/m.

### plot-perfil.py

```bash
python3 tools/scripts/plot-perfil.py
```

Lee todos los `perfil-ve-<NN>.dat`, con `NN` = 10 D, del directorio
actual (salidas de `vel_profile` con la cantidad `vy`) y grafica `<v_y>`
(cm/s) en función de `y` (cm), una curva por `D`, en `perfiles-vy.pdf`. La escala de colores
está fijada para `D` entre 4 y 20 diámetros.

### plot-fuerzas.py

```bash
python3 tools/scripts/plot-fuerzas.py
```

Lee todos los `perfil-fn-<NN>.dat`, con `NN` = 10 D (salidas de
`force_profile --qty norm`), y `perfil-ft-<NN>.dat` (`--qty tan`) del
directorio actual. Grafica `<f_N>` y `<|f_T|>` (N) en función de `y` (cm),
en dos paneles, en `perfiles-fn-ft.pdf`. Usa las dos primeras columnas de cada archivo, así
que la columna `n` no molesta.

### plot-mapa-fuerzas.py

```bash
python3 tools/scripts/plot-mapa-fuerzas.py mapa.dat [-o mapa.pdf] [--vmax-norm V] [--vmax-tan V]
```

Mapas de `<f_N>` y `<|f_T|>` (N) de una salida de `force_map2d` con
`--qty norm tan`. La imagen conserva la orientación del silo, con el
orificio abajo; las etiquetas están en cm experimentales. Los bines vacíos
quedan en blanco. `--vmax-*` fija el máximo de la escala de colores, para
comparar mapas. Sin `-o`, la figura se llama como el archivo de entrada,
con `.pdf`.

### plot_stress_profile.py

```bash
python3 tools/scripts/plot_stress_profile.py estres-D30.dat estres-D50.dat -o estres.pdf \
    [--etiquetas "D = 3d" "D = 5d"] [--sim] [--continuo] [--sin-tex]
```

Una curva por archivo de `stress_profile`. La etiqueta por defecto,
`D = 2W`, se toma de la cabecera. Seis paneles en función de `y`:

- (a) presión `p = -(sxx + syy)/2`;
- (b) `-syy` y (c) `-sxx`;
- (d) `sxy` y `syx`;
- (e) parte tangencial `stxy` y `styx`;
- (f) parte cinética `-kyy` y `-kxx`, con sus errores.

Las bandas son el error estándar por bloques. Opciones:

- `--sim`: unidades de la simulación.
- `--continuo`: multiplica el estrés por `phi` (estrés medio del bin).
- `--sin-tex`: sin LaTeX.

### Unidades: `scripts/unidades.py`

Único lugar con los factores de conversión (de `utils/reduced_units.ods`):

| Magnitud | Experimento | Simulación |
|---|---|---|
| Longitud (diámetro `d`) | 0.005 m | 1 |
| Masa | 2.10e-4 kg | 1 |
| Tiempo `sqrt(d/g)` | 0.0225877 s | 1 |
| Velocidad | 0.221359 m/s | 1 |
| Fuerza `m g` | 0.002058 N | 1 |
| Estrés 2D `F/L` | 0.4116 N/m | 1 |

Funciones:

- `x_a_cm(x)` y `y_a_cm(y)`; esta última invierte el eje.
- `vx_a_cm_s(vx)` y `vy_a_cm_s(vy)`; esta última invierte el signo.
- `fuerza_a_N(f)`.
- `convertir_perfil(datos, experimentales=True, continuo=False)`, para las
  salidas de `stress_profile`. Invierte `y`, así que cambian de signo `vy`
  y las componentes `xy` y `yx`; los errores no cambian de signo.

Para usarlo desde un script nuevo:

```python
import sys
sys.path.insert(0, "/ruta/a/unav-walker/tools/scripts")
import unidades
y_cm = unidades.y_a_cm(y)
```

Los scripts de `../scripts/` con `t = 0.0553 s` o longitud `0.03 m`
corresponden a otro montaje (d = 3 cm) y no usan este módulo.

---

## Biblioteca `dem_reader` (para programas nuevos)

`dem_reader.hpp` y `dem_types.hpp` leen todos los formatos del simulador.
Para un programa nuevo, basta con agregarlo a `tools/CMakeLists.txt` y
enlazarlo con `dem_reader`.

| Función | Devuelve |
|---|---|
| `list_frames_with_prefix(dir, pre, ext)` | Números de frame de `<pre>_*.<ext>`, ordenados (para contactos, `pre = "fc_" + pre`) |
| `frame_path(dir, pre, frame, ext)` | Ruta `dir/<pre>_<frame 6 dígitos><ext>` |
| `read_xy(path)` | `XYFrame`: tiempo, discos, polígonos y segmentos de pared |
| `read_ve(path)` | `VEFrame`: posición, velocidad y energías de cada grano |
| `read_fc(path)` | `FCFrame`: contactos (`Contact`: IDs, punto, `norm`, `tan`, normal, centros, `wall`) y cabecera de procedencia (`prov`) |
| `read_sxy(path)` | `SXYFrame`: tensor por grano (`GrainStress`) y cabecera de procedencia |

`read_fc` y `read_sxy` rechazan los formatos anteriores a la versión 3.0.
La convención de la fuerza de contacto: sobre B vale `F = norm (nx, ny) +
tan (ny, -nx)`, y sobre A es `-F`. `thread_pool.hpp` es el pool de threads
que usan los programas para procesar los frames en paralelo.

## Validación

- `force_profile` coincide con un cálculo independiente en Python, con y
  sin filtros y en todas las cantidades, con una diferencia relativa
  máxima de 4e-9 en 149 frames.
- `stress_profile` coincide con una implementación independiente en Python
  con una diferencia de 3e-7, que es la precisión de impresión.
- `read_fc` y `read_sxy` se probaron con archivos del formato nuevo y del
  anterior (que rechazan).
