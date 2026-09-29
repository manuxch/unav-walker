#!/usr/bin/env python3
"""Gráficos de los perfiles de estrés de stress_profile.

Uso:
    plot_stress_profile.py perfil1.dat [perfil2.dat ...] -o figura.pdf
        [--etiquetas "D = 3d" "D = 5d" ...] [--sim] [--continuo] [--sin-tex]

Cada archivo es una salida de bin/stress_profile (una curva por archivo; por
defecto la etiqueta es D = 2W, con W el semiancho de la franja). Paneles,
en función de la altura y:

    (a) presión p = -(sxx + syy)/2 del tensor de contacto
    (b) -syy (normal vertical)          (c) -sxx (normal horizontal)
    (d) sxy (línea) y syx (trazos)      (e) parte tangencial: stxy, styx
    (f) parte cinética: -kyy (línea) y -kxx (trazos)

Las bandas sombreadas son el error estándar por bloques de un período
(para p se combinan los errores de sxx y syy sin covarianza). Signos: el
estrés es negativo en compresión, por eso se grafican -sxx, -syy y p.

Por defecto las unidades son las experimentales (y en cm con el eje
invertido, estrés 2D en N/m; ver unidades.py). --sim deja las unidades de
la simulación. --continuo multiplica por phi (estrés medio del bin en lugar
del estrés sobre los granos).
"""

import argparse
import re
import sys
from pathlib import Path

import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import unidades  # noqa: E402


def leer_perfil(archivo):
    """Lee una salida de stress_profile: devuelve (columnas, semiancho W)."""
    nombres, semiancho = None, None
    with open(archivo) as f:
        for linea in f:
            if not linea.startswith("#"):
                break
            m = re.search(r"\|\s*<=\s*([-+0-9.eE]+)", linea)
            if m:
                semiancho = float(m.group(1))
            if linea.startswith("# y n phi"):
                nombres = linea[1:].split()
    if nombres is None:
        sys.exit(f"{archivo}: no es una salida de stress_profile "
                 "(falta la línea de columnas '# y n phi ...').")
    datos = np.loadtxt(archivo, comments="#", ndmin=2)
    return {n: datos[:, i] for i, n in enumerate(nombres)}, semiancho


def banda(ax, y, v, e, color):
    """Curva con banda de error (si hay errores finitos)."""
    ax.plot(y, v, "-", color=color, lw=1.5)
    if e is not None and np.isfinite(e).any():
        ax.fill_between(y, v - e, v + e, color=color, alpha=0.25, lw=0)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("perfiles", nargs="+", help="salidas de stress_profile")
    ap.add_argument("-o", "--salida", required=True,
                    help="figura (pdf, png, svg)")
    ap.add_argument("--etiquetas", nargs="+",
                    help="etiqueta de cada perfil (por defecto, D = 2W)")
    ap.add_argument("--sim", action="store_true",
                    help="unidades de la simulación")
    ap.add_argument("--continuo", action="store_true",
                    help="multiplica el estrés por phi (medio continuo)")
    ap.add_argument("--sin-tex", action="store_true",
                    help="no usar LaTeX para el texto")
    args = ap.parse_args()
    if args.etiquetas and len(args.etiquetas) != len(args.perfiles):
        sys.exit("--etiquetas debe tener una etiqueta por perfil.")

    if not args.sin_tex:
        mpl.rcParams.update({"text.usetex": True, "font.family": "serif",
                             "font.serif": ["Linux Libertine O"]})
    mpl.rcParams.update({"font.size": 11, "axes.labelsize": 11,
                         "legend.fontsize": 9})

    exp = not args.sim
    u_y = r"$y$ (cm)" if exp else r"$y / d$"
    u_s = r"(N/m)" if exp else r"($mg/d$)"

    fig, axs = plt.subplots(2, 3, figsize=(12, 6.5), sharex=True)
    ax = axs.ravel()
    titulos = [r"$p = -(\sigma_{xx}+\sigma_{yy})/2$", r"$-\sigma_{yy}$",
               r"$-\sigma_{xx}$",
               r"$\sigma_{xy}$ (---), $\sigma_{yx}$ ($--$)",
               r"tangencial: $\sigma^t_{xy}$ (---), $\sigma^t_{yx}$ ($--$)",
               r"cin\'etica: $-k_{yy}$ (---), $-k_{xx}$ ($--$)"]
    if args.sin_tex:
        titulos = [t.replace(r"\'e", "é").replace("---", "—")
                   .replace("$--$", "- -") for t in titulos]

    colores = mpl.colormaps["plasma"](np.linspace(0.0, 0.85,
                                                   len(args.perfiles)))
    for i, archivo in enumerate(args.perfiles):
        crudo, w = leer_perfil(archivo)
        d = unidades.convertir_perfil(crudo, experimentales=exp,
                                      continuo=args.continuo)
        c = colores[i]
        if args.etiquetas:
            etiqueta = args.etiquetas[i]
        elif w is not None:
            etiqueta = rf"$D = {2 * w:g}\,d$"
        else:
            etiqueta = Path(archivo).stem
        y = d["y"]
        p = -(d["sxx"] + d["syy"]) / 2
        e_p = np.hypot(d["e_sxx"], d["e_syy"]) / 2
        banda(ax[0], y, p, e_p, c)
        ax[0].lines[-1].set_label(etiqueta)
        banda(ax[1], y, -d["syy"], d["e_syy"], c)
        banda(ax[2], y, -d["sxx"], d["e_sxx"], c)
        banda(ax[3], y, d["sxy"], d["e_sxy"], c)
        ax[3].plot(y, d["syx"], "--", color=c, lw=1.2)
        ax[4].plot(y, d["stxy"], "-", color=c, lw=1.5)
        ax[4].plot(y, d["styx"], "--", color=c, lw=1.2)
        ax[5].plot(y, -d["kyy"], "-", color=c, lw=1.5)
        ax[5].plot(y, -d["kxx"], "--", color=c, lw=1.2)

    for a, t in zip(ax, titulos):
        a.set_title(t, fontsize=11)
        a.axhline(0.0, color="0.6", lw=0.6)
        a.grid(alpha=0.3)
    for a in axs[:, 0]:
        a.set_ylabel(u_s)
    for a in axs[1, :]:
        a.set_xlabel(u_y)
    ax[0].legend(ncols=2)
    medio = "medio continuo" if args.continuo else "sobre los granos"
    fig.suptitle(f"Perfiles del tensor de estrés ({medio})", fontsize=12)
    fig.tight_layout()
    fig.savefig(args.salida)
    print(f"Figura guardada en {args.salida}")


if __name__ == "__main__":
    main()
