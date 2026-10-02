#!/usr/bin/env python3
"""Perfiles en y de las cantidades de vel_profile, una figura por columna.

Uso:
    plot-perfil.py [archivos ...] [--qty vy vx ...] [--etiqueta E]
                   [--titulo T] [--sin-tex]

Sin archivos, lee todos los perfil-ve-<NN>.dat del directorio actual, con
NN = D en diámetros (perfil-ve-04.dat -> D = 4 d). Las columnas se
identifican por la cabecera '# y_center vy vx ...' que escribe vel_profile.
Por defecto se hace una figura por cada columna presente en los archivos,
con una curva por D; --qty restringe a las cantidades pedidas. Un archivo
que no tiene una columna simplemente no aparece en esa figura.

El nombre de la columna fija la conversión, la etiqueta del eje, el título
y el archivo de salida, perfiles-<qty>.pdf (perfiles-<qty>-<E>.pdf con
--etiqueta E). Unidades experimentales (ver unidades.py):
    vy, vx, speed   cm/s     (vy cambia de signo: el eje y está invertido)
    w               rad/s    (cambia de signo por la misma razón)
    Eklin, Ekrot    J
Una columna con otro nombre se grafica en unidades de la simulación.

--titulo reemplaza el título de todas las figuras; '{qty}' se reemplaza por
el nombre de la cantidad.

vel_profile escribe 0 en los bines vacíos; esos bines no se grafican.
"""

import argparse
import glob
import re
import sys
from pathlib import Path

import matplotlib as mpl
import matplotlib.colors as colors
import matplotlib.pyplot as plt
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import unidades  # factores de conversión (utils/reduced_units.ods)

# Cantidad -> (conversión, etiqueta del eje, título por defecto)
CANTIDADES = {
    "vy": (unidades.vy_a_cm_s, r"$\langle v_y \rangle$ (cm/s)",
           "Perfil de velocidad vertical"),
    "vx": (unidades.vx_a_cm_s, r"$\langle v_x \rangle$ (cm/s)",
           "Perfil de velocidad horizontal"),
    "speed": (lambda v: np.asarray(v, dtype=float) * unidades.VELOCIDAD
              * unidades.CM, r"$\langle |\mathbf{v}| \rangle$ (cm/s)",
              "Perfil de rapidez"),
    "w": (unidades.w_a_rad_s, r"$\langle \omega \rangle$ (rad/s)",
          "Perfil de velocidad angular"),
    "Eklin": (unidades.energia_a_J, r"$\langle E_{k,\mathrm{lin}} \rangle$ (J)",
              "Perfil de energía cinética de traslación"),
    "Ekrot": (unidades.energia_a_J, r"$\langle E_{k,\mathrm{rot}} \rangle$ (J)",
              "Perfil de energía cinética de rotación"),
}


def d_de_archivo(f):
    """D (en diámetros) del sufijo del nombre, o None si no es un entero."""
    m = re.fullmatch(r".*-(\d+)\.dat", Path(f).name)
    return int(m.group(1)) if m else None


def columnas(f):
    """Nombres de las columnas, de la cabecera '# y_center ...'."""
    with open(f) as h:
        for linea in h:
            if not linea.startswith("#"):
                break
            campos = linea[1:].split()
            if campos and campos[0] == "y_center":
                return campos
    return None


def leer(f):
    """(nombres de columnas, datos) del archivo; None sin cabecera."""
    nombres = columnas(f)
    if nombres is None:
        print(f"aviso: {f} ignorado (sin cabecera '# y_center ...')",
              file=sys.stderr)
        return None
    return nombres, np.loadtxt(f, comments="#", ndmin=2)


def figura(qty, perfiles, args):
    """Una figura de la cantidad qty con los perfiles [(D, archivo, nombres,
    datos)] que la tienen. Devuelve el nombre del archivo guardado."""
    if qty in CANTIDADES:
        convertir, etiqueta_y, titulo = CANTIDADES[qty]
    else:
        print(f"aviso: {qty} no tiene conversión; se grafica en unidades de "
              "la simulación", file=sys.stderr)
        convertir = lambda v: v
        etiqueta_y = qty if args.sin_tex else qty.replace("_", r"\_")
        titulo = f"Perfil de {etiqueta_y}"
    if args.titulo is not None:
        titulo = args.titulo.replace("{qty}", qty)
    norm = colors.Normalize(vmin=2, vmax=20)
    cmap = mpl.colormaps['plasma']

    fig, ax = plt.subplots()
    for d, f, nombres, datos in perfiles:
        y = unidades.y_a_cm(datos[:, 0])
        v = datos[:, nombres.index(qty)]
        v = convertir(np.where(v == 0.0, np.nan, v))  # bines vacíos
        print(f"D = {d:3d} - {qty}: max = {np.nanmax(v):.4g}"
              f" - min = {np.nanmin(v):.4g}")
        ax.plot(y, v, '.-', color=cmap(norm(d)), label=fr"$D = {d} \, d$",
                alpha=0.7)
    ax.set_ylabel(etiqueta_y)
    ax.set_xlabel(r'$y$ (cm)')
    ax.set_title(titulo)
    ax.legend(ncols=2)
    fig.tight_layout()
    salida = f"perfiles-{qty}" + (f"-{args.etiqueta}" if args.etiqueta else "") + ".pdf"
    fig.savefig(salida)
    plt.close(fig)
    return salida


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("archivos", nargs="*",
                    help="salidas de vel_profile (por defecto, perfil-ve-*.dat)")
    ap.add_argument("--qty", nargs="+",
                    help="cantidades a graficar (por defecto, todas las columnas)")
    ap.add_argument("--etiqueta",
                    help="sufijo de los archivos: perfiles-<qty>-<etiqueta>.pdf")
    ap.add_argument("--titulo",
                    help="título de las figuras; {qty} se reemplaza por la cantidad")
    ap.add_argument("--sin-tex", action="store_true",
                    help="no usar LaTeX para el texto")
    args = ap.parse_args()

    if not args.sin_tex:
        plt.rcParams.update({
            'text.usetex': True,
            'font.family': 'serif',
            'font.serif': ['Linux Libertine O'],
        })
    plt.rcParams.update({
        'font.size': 14,
        'axes.titlesize': 14,
        'axes.labelsize': 14,
        'legend.fontsize': 8,
        'xtick.labelsize': 14,
        'ytick.labelsize': 14
    })

    archivos = args.archivos or sorted(glob.glob("perfil-ve-*.dat"))
    pares = []
    for f in archivos:
        d = d_de_archivo(f)
        if d is None:
            print(f"aviso: {f} ignorado (el sufijo no es D)", file=sys.stderr)
            continue
        pares.append((d, f))
    pares.sort()

    perfiles = []
    for d, f in pares:
        r = leer(f)
        if r is not None:
            perfiles.append((d, f, *r))
    # Cantidades: las pedidas, o todas las columnas en orden de aparición
    presentes = []
    for _, _, nombres, _ in perfiles:
        for q in nombres[1:]:
            if q not in presentes:
                presentes.append(q)
    qtys = args.qty or presentes
    if not qtys:
        print("No hay perfiles para graficar.", file=sys.stderr)
        return 1

    for qty in qtys:
        con_qty = [p for p in perfiles if qty in p[2]]
        if not con_qty:
            print(f"aviso: ningún archivo tiene la columna {qty}",
                  file=sys.stderr)
            continue
        print(f"Figura guardada en {figura(qty, con_qty, args)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
