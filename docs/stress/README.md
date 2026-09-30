# Cálculo del tensor de estrés

`estres.tex` describe todo el cálculo relacionado con el estrés:

- las fuerzas de contacto reconstruidas a partir de los impulsos de Box2D;
- el tensor de cada grano que escribe el simulador en los `.sxy`;
- los perfiles de `tools/stress_profile`: partes normal, tangencial y
  cinética, fracción de área y errores por bloques;
- la conversión a unidades experimentales en los gráficos.

Para compilarlo (el PDF no se guarda en el repositorio):

```bash
cd docs/stress && latexmk -pdf estres.tex && latexmk -c
```

También compila con `pdflatex`, `xelatex` o `lualatex`.
