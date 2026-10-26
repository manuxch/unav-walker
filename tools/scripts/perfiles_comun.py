"""Lectura común de los perfiles en y de vel_profile y force_profile.

Los archivos se llaman <algo>-<NN>.dat, con NN = D en diámetros
(perfil-ve-04.dat -> D = 4 d), y tienen una cabecera de columnas
'# y_center <qty1> <qty2> ...' (force_profile agrega al final la columna n).
"""

import re
import sys
from pathlib import Path

import numpy as np


def d_de_archivo(f):
    """D (en diámetros) del sufijo del nombre, o None si no es un entero."""
    m = re.fullmatch(r".*-(\d+)\.dat", Path(f).name)
    return int(m.group(1)) if m else None


def columnas(f):
    """Nombres de las columnas, de la cabecera '# y_center ...'; None si no
    la hay."""
    with open(f) as h:
        for linea in h:
            if not linea.startswith("#"):
                break
            campos = linea[1:].split()
            if campos and campos[0] == "y_center":
                return campos
    return None


def leer_perfiles(archivos):
    """Lista ordenada por D de (D, archivo, nombres, datos).

    Saltea con un aviso los archivos cuyo sufijo no es D o que no tienen la
    cabecera de columnas.
    """
    perfiles = []
    for f in archivos:
        d = d_de_archivo(f)
        if d is None:
            print(f"aviso: {f} ignorado (el sufijo no es D)", file=sys.stderr)
            continue
        nombres = columnas(f)
        if nombres is None:
            print(f"aviso: {f} ignorado (sin cabecera '# y_center ...')",
                  file=sys.stderr)
            continue
        perfiles.append((d, f, nombres, np.loadtxt(f, comments="#", ndmin=2)))
    perfiles.sort(key=lambda p: (p[0], p[1]))
    return perfiles
