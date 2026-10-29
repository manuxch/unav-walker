#!/usr/bin/env python3
"""Perfiles en y de las fuerzas de contacto de force_profile, un panel por
cantidad y una curva por D.

Uso:
    plot-fuerzas.py [archivos ...] [--qty norm tan ...] [--etiqueta E]
                    [--titulo T] [--sin-tex]

Sin archivos, lee todos los perfil-f*-<NN>.dat del directorio actual
(perfil-fn-04.dat, perfil-ft-04.dat, perfil-fm-04.dat, ...), con NN = D en
diámetros. Las columnas se identifican por la cabecera
'# y_center <qty1> <qty2> ... n' que escribe force_profile, así que un
archivo puede tener una o varias cantidades, y las de un mismo D pueden
estar repartidas en varios archivos. La columna n (número de contactos) no
se grafica.

Por defecto hay un panel por cada cantidad presente, en este orden:
    norm   <f_N>     componente normal
    tan    <|f_T|>   módulo de la componente tangencial
    fmag   <|f|>     módulo de la fuerza
    fx     <|f_x|>   módulo de la componente x
    fy     <|f_y|>   módulo de la componente y
--qty elige y ordena los paneles. Todas en N; fx y fy son valores
absolutos, así que no cambian de signo al invertir el eje y.

La figura se guarda en perfiles-fuerzas.pdf (perfiles-fuerzas-<E>.pdf con
--etiqueta E). Los bines vacíos (nan) no se grafican.
"""

import argparse
import glob
import sys
from pathlib import Path

import matplotlib as mpl
import matplotlib.colors as colors
import matplotlib.pyplot as plt
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import unidades  # factores de conversión (utils/reduced_units.ods)
from perfiles_comun import leer_perfiles

# Cantidad -> etiqueta del eje (en el orden de los paneles)
CANTIDADES = {
    "norm": r"$\langle f_N \rangle$ (N)",
    "tan": r"$\langle |f_T| \rangle$ (N)",
    "fmag": r"$\langle |\mathbf{f}| \rangle$ (N)",
    "fx": r"$\langle |f_x| \rangle$ (N)",
    "fy": r"$\langle |f_y| \rangle$ (N)",
}


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("archivos", nargs="*",
                    help="salidas de force_profile (por defecto, perfil-f*-*.dat)")
    ap.add_argument("--qty", nargs="+",
                    help="cantidades a graficar, en orden (por defecto, todas)")
    ap.add_argument("--etiqueta",
                    help="sufijo del archivo: perfiles-fuerzas-<etiqueta>.pdf")
    ap.add_argument("--titulo", help="título de la figura")
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

    archivos = args.archivos or sorted(glob.glob("perfil-f*-*.dat"))
    perfiles = leer_perfiles(archivos)

    # Por cantidad, una curva por D: (D, archivo, y, valores)
    curvas = {}
    for d, f, nombres, datos in perfiles:
        for i, q in enumerate(nombres[1:], start=1):
            if q == "n":
                continue
            lista = curvas.setdefault(q, [])
            previa = next((c for c in lista if c[0] == d), None)
            if previa is not None:
                print(f"aviso: {q} de D = {d} está en {previa[1]} y en {f}; "
                      f"se usa {previa[1]}", file=sys.stderr)
                continue
            lista.append((d, f, datos[:, 0], datos[:, i]))

    if args.qty:
        qtys = []
        for q in args.qty:
            if q in curvas:
                qtys.append(q)
            else:
                print(f"aviso: ningún archivo tiene la columna {q}",
                      file=sys.stderr)
    else:
        qtys = [q for q in CANTIDADES if q in curvas]
        qtys += [q for q in curvas if q not in CANTIDADES]
    if not qtys:
        print("No hay perfiles para graficar.", file=sys.stderr)
        return 1

    norm = colors.Normalize(vmin=3, vmax=14)
    cmap = mpl.colormaps['plasma']
    fig, axs = plt.subplots(len(qtys), 1, figsize=(8, 2.6 * len(qtys) + 0.6),
                            sharex=True, squeeze=False)
    axs = axs[:, 0]
    for ax, q in zip(axs, qtys):
        if q in CANTIDADES:
            convertir, etiqueta = unidades.fuerza_a_N, CANTIDADES[q]
        else:
            print(f"aviso: {q} no tiene conversión; se grafica en unidades de "
                  "la simulación", file=sys.stderr)
            convertir = lambda v: v
            etiqueta = q if args.sin_tex else q.replace("_", r"\_")
        for d, f, y, v in curvas[q]:
            v = convertir(v)
            print(f"D = {d:3d} - {q}: max = {np.nanmax(v):.4g}")
            ax.plot(unidades.y_a_cm(y), v, '.-', color=cmap(norm(d)),
                    label=fr"$D = {d} \, d$", alpha=0.7)
        ax.axvline(0.0, color="0.5", lw=0.8, ls=":")  # orificio
        ax.set_ylabel(etiqueta)
    axs[0].legend(ncols=2)
    axs[-1].set_xlabel(r'$y$ (cm)')
    if args.titulo:
        axs[0].set_title(args.titulo)
    fig.tight_layout()
    salida = "perfiles-fuerzas" + (f"-{args.etiqueta}" if args.etiqueta else "") + ".pdf"
    fig.savefig(salida)
    print(f"Figura guardada en {salida}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
