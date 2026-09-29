# unav-walker

Código para simulaciones de un(os) walker(s): descarga de un silo
bidimensional de discos apoyados sobre una base vibrada.

- `src/`: simulador `unav-walkers` (Box2D). Documentación en
  [src/README.md](src/README.md).
- `tools/`: análisis de los archivos de salida.
- `scripts/`: scripts de corridas y gráficos.
- `docs/`: bitácora y referencias.
- `changelog.md`: cambios por versión del simulador.

## Compilación

```bash
cmake -S . -B build          # configuración (Release por defecto)
cmake --build build -j       # simulador y herramientas, en bin/
ctest --test-dir build       # pruebas del simulador
cmake --build build --target doc   # documentación (Doxygen) en build/doc/html/
```

Requiere CMake >= 3.20, un compilador con C++20 y Box2D 2.4.2
(`-DBOX2D_ROOT=/ruta` si no está en la ruta por defecto).
