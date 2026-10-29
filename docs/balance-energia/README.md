# Balance de energía

`balance-energia.tex` describe el balance de energía de la base vibrada:

- qué mide el simulador con `save_energy: T` (`src/energy_budget.cpp`) y por
  qué el cierre es exacto;
- las salidas `energy_<pre>.dat` y `energy_fase_<pre>.dat`;
- el procesamiento con `tools/scripts/plot-energia.py` (períodos completos,
  errores por bloques de dos períodos);
- los resultados de la tanda PRL-energia (D = 3 a 14, simulador 3.4).

Las figuras de `figuras/` y los números de `figuras/numeros.txt` se
regeneran a partir de una tanda analizada con
`tools/scripts/analizar-todo.sh`:

```bash
cd docs/balance-energia && python3 figuras.py /ruta/a/la/tanda
```

Para compilarlo (el PDF no se guarda en el repositorio):

```bash
cd docs/balance-energia && latexmk -pdf balance-energia.tex && latexmk -c
```

Las tablas de resultados del texto están escritas a mano a partir de
`analisis/energia-D.dat` y de `figuras/numeros.txt`; si se rehace la tanda,
hay que actualizarlas.
