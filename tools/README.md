# Herramientas de análisis

Programas en C++20 que leen los archivos de salida de `unav-walkers`
(versión >= 3.0; ver `../src/README.md`) y calculan promedios temporales en
paralelo.

## Compilación

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Los ejecutables quedan en `build/bin/`.

## Herramientas

| Programa | Entrada | Uso |
|---|---|---|
| `stress_profile` | `.sxy` | Perfil del tensor de estrés en una franja vertical (**recomendado**) |
| `vel_profile` | `.ve` | Perfil de velocidades en una franja vertical |
| `read_demo` | `.xy`, `.ve`, `fc_*.dat` | Demostración del lector |
| `force_profile` | `fc_*.dat` | Media de Fn y \|Ft\| por punto de contacto (ver advertencia) |
| `force_map2d` | `fc_*.dat` | Mapa 2D de la media de Fn y \|Ft\| por punto de contacto (ver advertencia) |

`dem_reader` (`dem_reader.hpp`, `dem_types.hpp`) lee todos los formatos, con
la cabecera de procedencia. Rechaza los archivos de contactos y de tensores
anteriores a la versión 3.0, porque sus fuerzas tienen otra escala.

**Advertencia sobre `force_profile` y `force_map2d`.** Promedian por punto
de contacto e incluyen los contactos con las paredes y los de fuerza nula.
Leen el formato nuevo, pero para los perfiles de estrés conviene usar
`stress_profile`.

## stress_profile

```bash
build/bin/stress_profile <dir> <pre> -o perfil.dat --half-width W --y-max Y --freq F \
    [--y-min 0] [--dy 1] [--phase-bins 20] [--phase-output perfil_fase.dat] \
    [--t-min T0] [--t-max T1] [--x-center 0] [--threads N]
```

- `<pre>` es el `preFrameFile` de la simulación: se leen `<dir>/<pre>_*.sxy`.
- Se usan los granos con el centro en la franja `|x - x_c| <= W`. Para el
  ancho del orificio, `W = D/2 = radio_out_silo`.
- `--freq` es la `Frecuencia_exitacion` de la simulación. Define los
  bloques de un período para los errores y se verifica contra la fase de la
  cabecera de cada archivo.
- El programa termina con error si los archivos provienen de distintas
  versiones del simulador o de distintos archivos de parámetros.

**Definiciones.** Índices `xx xy yx yy`, con `s_ij = sum f_i l_j`.

- Tensor de contacto: `s = sum_p A_p s_p / sum_p A_p`. Es el estrés sobre los
  granos, con `A_p` el área de cada grano. Se da el total `s`, la parte
  normal `sn` y la tangencial `st = s - sn`.
- Parte cinética, con la misma normalización para que se sume a `s`:
  `k = -sum_p m_p v'v' / sum_p A_p`. Las fluctuaciones `v'` se miden respecto
  de la velocidad media en cada bin de `y` y de fase, así que la oscilación
  coherente con la base no cuenta como agitación.
- `phi = sum A_p / (A_bin N_frames)` es la fracción de área, contada por
  centros. El estrés medio del bin (medio continuo) es `phi (s + k)`.
  **`phi` no es una fracción de empaquetamiento física.** Un grano con el
  centro en la franja aporta toda su área aunque sobresalga, así que en
  franjas angostas (`D` de pocos diámetros) `phi` puede superar 1. Sí es el
  factor correcto para pasar de `s` al estrés medio del bin con asignación
  por centros.
- Signo: compresión `< 0`; la presión es `-(sxx + syy) / 2`.
- Errores `e_*`: error estándar de las medias por bloques de un período.
  Con pocos períodos (menos de ~10) son poco confiables.
- `nan` indica un bin sin granos.

**Columnas de la salida principal:**
`y n phi vx vy sxx sxy syx syy snxx snxy snyx snyy stxx stxy styx styy kxx
kxy kyy e_sxx e_sxy e_syx e_syy e_snxx e_snxy e_snyx e_snyy`.

Con `--phase-output`, se escribe además el perfil para cada bin de fase:
`fase y n n_frames phi vx vy sxx sxy syx syy snxx snxy snyx snyy kxx kxy kyy`.

**Validación.** Sobre una simulación de 149 frames, la salida coincide con
una implementación independiente en Python (numpy) con una diferencia
relativa máxima de 3e-7, que es la precisión de impresión.
