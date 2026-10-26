"""Lectura común de los perfiles en y de vel_profile y force_profile.

Los archivos se llaman <algo>-<NN>.dat, con NN = D en diámetros
(perfil-ve-04.dat -> D = 4 d), y tienen una cabecera de columnas
'# y_center <qty1> <qty2> ...' (force_profile agrega al final la columna n).

Las herramientas >= 3.4 registran además la franja en la cabecera
(x_m=... ancho=... r_out=... D=... franja=orificio|libre|sin_verificar).
"""

import re
import sys
from pathlib import Path

import numpy as np


def d_de_archivo(f):
    """D (en diámetros) del sufijo del nombre, o None si no es un entero."""
    m = re.fullmatch(r".*-(\d+)\.dat", Path(f).name)
    return int(m.group(1)) if m else None


def campos_franja(f):
    """Campos 'clave=valor' de la cabecera (x_m, ancho, r_out, D, franja, ...).

    Devuelve un dict de cadenas; vacío si el archivo no los tiene.
    """
    campos = {}
    with open(f) as h:
        for linea in h:
            if not linea.startswith("#"):
                break
            for tok in linea[1:].split():
                k, sep, v = tok.partition("=")
                if sep and k not in campos:
                    campos[k] = v
    return campos


def d_de_cabecera(f):
    """D (en diámetros) de la cabecera, o None si no está o no es finito."""
    try:
        d = float(campos_franja(f).get("D", "nan"))
    except ValueError:
        return None
    return d if np.isfinite(d) else None


def verificar_franja(f, d_nombre=None):
    """Avisa si la franja no es la del orificio o si el D del nombre no
    coincide con el de la cabecera."""
    campos = campos_franja(f)
    franja = campos.get("franja")
    if franja == "sin_verificar":
        print(f"aviso: {f}: no se puede verificar que la franja (ancho "
              f"{campos.get('ancho', '?')}) sea la del orificio: los datos no "
              "registran r_out", file=sys.stderr)
    elif franja is not None and franja != "orificio":
        print(f"aviso: {f}: franja={franja} (ancho {campos.get('ancho', '?')}),"
              " no la del orificio", file=sys.stderr)
    d_cab = d_de_cabecera(f)
    if d_nombre is not None and d_cab is not None and abs(d_cab - d_nombre) > 1e-6:
        print(f"aviso: {f}: el nombre dice D = {d_nombre} y la cabecera "
              f"D = {d_cab:g}; se usa el del nombre", file=sys.stderr)


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
        verificar_franja(f, d)
        perfiles.append((d, f, nombres, np.loadtxt(f, comments="#", ndmin=2)))
    perfiles.sort(key=lambda p: (p[0], p[1]))
    return perfiles
