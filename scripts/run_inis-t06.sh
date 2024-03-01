#!/bin/bash

exe=$1
# Iterar sobre los archivos .in en el directorio actual
for archivo_in in ./*.in; do
  if [ -e "$archivo_in" ]; then
    # Obtener el nombre base del archivo sin la extensión .in
    nombre_base=$(basename "$archivo_in" .in)

    # Crear el nombre del archivo de salida .dat
    archivo_dat="${nombre_base}.dat"

    # Ejecutar el programa run y redirigir la salida a archivo-*.dat
    ../src/${1} "$archivo_in" > "$archivo_dat"

    # Verificar si la ejecución fue exitosa
    if [ $? -eq 0 ]; then
      echo "Ejecución exitosa para $archivo_in. Resultados en $archivo_dat"
    else
      echo "Error al ejecutar run para $archivo_in."
    fi
  fi
done

