#!/usr/bin/env python3
"""Balance de energía de la base vibrada (save_energy: T), en función de D.

Uso:
    plot-energia.py [archivos ...] [--etiqueta E] [--reducidas] [--sin-tex]

Sin archivos, lee todos los energy_<pre>.dat del directorio actual y de
frames_*/ (los energy_fase_<pre>.dat se buscan al lado de cada uno). D es el
sufijo de <pre> en diámetros (energy_frm-04.dat -> D = 4 d) y se verifica
contra r_out de la cabecera de procedencia.

Columnas de energy_<pre>.dat (una fila por período b = floor(f t)), en m g d:
    Wb       trabajo de la base (lo que entrega el motor)
    Wlin     trabajo de la fricción de Karnopp sobre los granos
    Wpiv     trabajo del torque de pivoteo (<= 0); Dpiv = -Wpiv
    Dslide   Wb - Wlin, disipación por deslizamiento contra la base
    Wcont    trabajo de los contactos grano-grano y grano-pared (< 0)
    dK       cambio de K dentro de los pasos; dK_reiny, por descarga y
             reinyección; Delta K = dK + dK_reiny
    K        energía cinética total media del período

Con esas definiciones el balance de cada período es exacto:
    Wb   = Dslide + Wlin
    Wlin = Dpiv + (-Wcont) + (-dK_reiny) + Delta K
o sea, de lo que entrega el motor una parte se pierde en el deslizamiento
contra la base y el resto (Wlin) entra a los granos, que lo pierden por
pivoteo, en los contactos o se lo llevan al salir por el orificio.

Se usan solo los períodos completos (se descartan el primero y el último si
están cortados). Los errores son el error estándar de la media entre bloques
de --bloque períodos consecutivos (por defecto 2): la respuesta del lecho
puede alternar de un período al siguiente (período 2), y entonces los
períodos sueltos no son independientes. Un período que no completa un
bloque al final se descarta de las medias. Las series de
energia-periodos.pdf son siempre por período.

Figuras (con --etiqueta E, el sufijo -E antes de .pdf):
    energia-D.pdf        medias por período en función de D: (a) términos
                         del balance, (b) Wlin / Wb, (c) reparto de Wlin
    energia-fase.pdf     trabajo por período en cada bin de fase (Wb, Wlin,
                         Wcont) y K media, una curva por D
    energia-periodos.pdf series por período (Wb, Wlin, -Wcont, K), para
                         ver si el registro es estacionario
y la tabla energia-D.dat (medias y errores, en las unidades de las figuras).

Unidades: mJ por período (K en mJ) y tiempo en s; con --reducidas, m g d y
unidades de la simulación.
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
from perfiles_comun import d_de_archivo

COLUMNAS = ["periodo", "t_ini", "t_fin", "npasos", "N", "Wb", "Wlin", "Wpiv",
            "Wg", "Dslide", "Dpiv", "Wcont", "dK", "dK_reiny", "K"]
COLUMNAS_FASE = ["fase", "dfase"] + COLUMNAS[3:]

# Términos del balance: (clave, etiqueta, signo con el que se grafican)
TERMINOS = [
    ("Wb", r"$W_b$", 1),
    ("Dslide", r"$D_\mathrm{slide}$", 1),
    ("Wlin", r"$W_\mathrm{lin}$", 1),
    ("Wcont", r"$-W_\mathrm{cont}$", -1),
    ("Dpiv", r"$D_\mathrm{piv}$", 1),
    ("dK_reiny", r"$-\Delta K_\mathrm{reiny}$", -1),
]
# Reparto de Wlin: (clave, etiqueta, signo)
REPARTO = [
    ("Wcont", r"$-W_\mathrm{cont}$", -1),
    ("Dpiv", r"$D_\mathrm{piv}$", 1),
    ("dK_reiny", r"$-\Delta K_\mathrm{reiny}$", -1),
    ("DK", r"$\Delta K$", 1),
]


def cabecera(f):
    """Líneas de comentario del comienzo de f."""
    lineas = []
    with open(f) as h:
        for linea in h:
            if not linea.startswith("#"):
                break
            lineas.append(linea)
    return lineas


def r_out_de_cabecera(f):
    """r_out de la línea de procedencia ('# git: ... r_out: <r>'), o None."""
    for linea in cabecera(f):
        m = re.search(r"\br_out:\s*(\S+)", linea)
        if m:
            try:
                return float(m.group(1))
            except ValueError:
                return None
    return None


def verificar_columnas(f, esperadas):
    """Error si la cabecera de columnas de f no es la esperada."""
    for linea in cabecera(f):
        campos = linea[1:].split()
        if campos and campos[0] == esperadas[0]:
            if campos != esperadas:
                raise SystemExit(f"{f}: columnas inesperadas {campos}")
            return
    raise SystemExit(f"{f}: falta la cabecera '# {' '.join(esperadas)}'")


def leer(f, nombres):
    """dict columna -> array de f."""
    verificar_columnas(f, nombres)
    datos = np.loadtxt(f, comments="#", ndmin=2)
    return {k: datos[:, i] for i, k in enumerate(nombres)}


def completos(p):
    """Filtra los períodos completos (npasos cerca del máximo)."""
    ok = p["npasos"] >= 0.95 * p["npasos"].max()
    return {k: v[ok] for k, v in p.items()}


def agregar_derivadas(p):
    """Delta K del período (o del bin)."""
    p["DK"] = p["dK"] + p["dK_reiny"]
    return p


def media_error(x):
    """Media y error estándar de la media (np.nan si hay un solo valor)."""
    n = len(x)
    if n == 0:
        return np.nan, np.nan
    e = np.std(x, ddof=1) / np.sqrt(n) if n > 1 else np.nan
    return np.mean(x), e


def cociente_error(a, b):
    """Media y error de sum(a) / sum(b) entre períodos (por bloques)."""
    n = len(a)
    r = np.sum(a) / np.sum(b)
    if n < 2:
        return r, np.nan
    # Linealización del cociente: residuos a_i - r b_i
    e = np.std(a - r * b, ddof=1) / np.sqrt(n) / np.mean(b)
    return r, e


def bloques(p, n):
    """Promedios por bloque de n períodos completos consecutivos.

    Cada columna del resultado es la media de la columna en el bloque (el
    trabajo sigue siendo por período). Solo se usan bloques con n períodos
    contiguos.
    """
    per = p["periodo"].astype(int)
    idx = (per - per[0]) // n
    sel = []
    for i in np.unique(idx):
        m = np.flatnonzero(idx == i)
        if len(m) == n and per[m[-1]] - per[m[0]] == n - 1:
            sel.append(m)
    return {k: np.array([v[m].mean() for m in sel]) for k, v in p.items()}


def leer_corridas(archivos, n_bloque):
    """Lista ordenada por D de (D, archivo, períodos completos, fase|None,
    bloques de n_bloque períodos)."""
    corridas = []
    for f in archivos:
        d = d_de_archivo(f)
        if d is None:
            print(f"aviso: {f} ignorado (el sufijo no es D)", file=sys.stderr)
            continue
        r_out = r_out_de_cabecera(f)
        if r_out is None:
            print(f"aviso: {f}: la cabecera no registra r_out; no se puede "
                  "verificar D", file=sys.stderr)
        elif abs(2 * r_out - d) > 1e-6:
            print(f"aviso: {f}: el nombre dice D = {d} y la cabecera "
                  f"D = 2 r_out = {2 * r_out:g}; se usa el del nombre",
                  file=sys.stderr)
        p = agregar_derivadas(completos(leer(f, COLUMNAS)))
        if len(p["Wb"]) == 0:
            print(f"aviso: {f} ignorado (sin períodos completos)",
                  file=sys.stderr)
            continue
        ff = Path(f).with_name(Path(f).name.replace("energy_", "energy_fase_", 1))
        fase = agregar_derivadas(leer(ff, COLUMNAS_FASE)) if ff.exists() else None
        if fase is None:
            print(f"aviso: falta {ff}", file=sys.stderr)
        pb = bloques(p, n_bloque)
        if len(pb["Wb"]) < 2:
            print(f"aviso: {f}: menos de dos bloques de {n_bloque} períodos; "
                  "sin errores", file=sys.stderr)
        corridas.append((d, f, p, fase, pb))
    corridas.sort(key=lambda c: (c[0], c[1]))
    return corridas


def unidades_de(args):
    """(factor de energía, etiqueta de energía, factor de tiempo, etiqueta)."""
    if args.reducidas:
        return 1.0, r"$mgd$", 1.0, r"$\sqrt{d/g}$"
    return unidades.ENERGIA * 1e3, "mJ", unidades.TIEMPO, "s"


def figura_D(corridas, args, salida):
    """Medias por período en función de D."""
    fe, ue, _, _ = unidades_de(args)
    ds = np.array([c[0] for c in corridas])
    fig, axs = plt.subplots(3, 1, figsize=(6.4, 10), sharex=True)

    ax = axs[0]
    for (k, etiqueta, signo), mk in zip(TERMINOS, "osd^vx"):
        me = np.array([media_error(signo * c[4][k] * fe) for c in corridas])
        ax.errorbar(ds, me[:, 0], yerr=me[:, 1], fmt=mk + "-", capsize=3, alpha=0.7,
                    label=etiqueta)
    ax.set_yscale("log")
    ax.set_ylabel(f"energía por período ({ue})")
    ax.set_title("(a) Términos del balance")
    ax.legend(ncols=2)

    ax = axs[1]
    me = np.array([cociente_error(c[4]["Wlin"], c[4]["Wb"]) for c in corridas])
    ax.errorbar(ds, me[:, 0], yerr=me[:, 1], fmt="o-", capsize=3, alpha=0.7)
    ax.set_ylabel(r"$W_\mathrm{lin} / W_b$")
    ax.set_title(r"(b) Fracción de $W_b$ que entra a los granos")

    ax = axs[2]
    for (k, etiqueta, signo), mk in zip(REPARTO, "odvs"):
        me = np.array([cociente_error(signo * c[4][k], c[4]["Wlin"])
                       for c in corridas])
        ax.errorbar(ds, me[:, 0], yerr=me[:, 1], fmt=mk + "-", capsize=3, alpha=0.7,
                    label=etiqueta)
    ax.axhline(0, color="0.6", lw=0.8)
    ax.set_ylabel(r"fracción de $W_\mathrm{lin}$")
    ax.set_title(r"(c) Reparto de $W_\mathrm{lin}$")
    ax.set_xlabel(r"$D/d$")
    ax.legend(ncols=2)

    fig.tight_layout()
    fig.savefig(salida)
    plt.close(fig)


def figura_fase(corridas, args, salida):
    """Trabajo por período en cada bin de fase, una curva por D."""
    fe, ue, _, _ = unidades_de(args)
    norm = colors.Normalize(vmin=3, vmax=14)
    cmap = mpl.colormaps["plasma"]
    paneles = [("Wb", r"$W_b$", 1, True), ("Wlin", r"$W_\mathrm{lin}$", 1, True),
               ("Wcont", r"$W_\mathrm{cont}$", 1, True),
               ("K", r"$\langle K \rangle$", 1, False)]
    fig, axs = plt.subplots(2, 2, figsize=(10, 7.5), sharex=True)
    for ax, (k, etiqueta, signo, por_periodo) in zip(axs.flat, paneles):
        for d, _, _, fase, _ in corridas:
            if fase is None:
                continue
            x = fase["fase"] / (2 * np.pi)
            y = signo * fase[k] * fe
            if por_periodo:  # trabajo por período en el bin -> por unidad de fase
                y = y / (fase["dfase"] / (2 * np.pi))
            ax.plot(x, y, ".-", color=cmap(norm(d)), alpha=0.7,
                    label=fr"$D = {d} \, d$")
        ax.axhline(0, color="0.6", lw=0.8)
        unidad = (f"{ue} por período por unidad de fase" if por_periodo
                  else ue)
        ax.set_ylabel(f"{etiqueta} ({unidad})", fontsize=10)
        ax.set_xlim(0, 1)
    for ax in axs[1]:
        ax.set_xlabel(r"fase $f t \; \mathrm{mod} \; 1$")
    axs[0, 0].legend(ncols=2, fontsize=7)
    fig.suptitle("Balance de energía en fase con la excitación")
    fig.tight_layout()
    fig.savefig(salida)
    plt.close(fig)


def figura_periodos(corridas, args, salida):
    """Series por período, para verificar que el registro es estacionario."""
    fe, ue, ft, ut = unidades_de(args)
    norm = colors.Normalize(vmin=3, vmax=14)
    cmap = mpl.colormaps["plasma"]
    paneles = [("Wb", r"$W_b$", 1), ("Wlin", r"$W_\mathrm{lin}$", 1),
               ("Wcont", r"$-W_\mathrm{cont}$", -1),
               ("K", r"$\langle K \rangle$", 1)]
    fig, axs = plt.subplots(2, 2, figsize=(10, 7.5), sharex=True)
    for ax, (k, etiqueta, signo) in zip(axs.flat, paneles):
        for d, _, p, _, _ in corridas:
            t = 0.5 * (p["t_ini"] + p["t_fin"]) * ft
            ax.plot(t, signo * p[k] * fe, ".-", color=cmap(norm(d)),
                    alpha=0.7, label=fr"$D = {d} \, d$")
        unidad = ue if k == "K" else f"{ue} por período"
        ax.set_ylabel(f"{etiqueta} ({unidad})", fontsize=10)
    for ax in axs[1]:
        ax.set_xlabel(f"$t$ ({ut})")
    axs[0, 0].legend(ncols=2, fontsize=7)
    fig.suptitle("Balance de energía por período")
    fig.tight_layout()
    fig.savefig(salida)
    plt.close(fig)


def tabla_D(corridas, args, salida):
    """Medias y errores por período en función de D."""
    fe, ue, _, _ = unidades_de(args)
    claves = ["Wb", "Dslide", "Wlin", "Dpiv", "Wcont", "dK_reiny", "DK", "K"]
    cols = ["D", "nper", "nbloq", "N"]
    for k in claves:
        cols += [k, "e_" + k]
    cols += ["Wlin/Wb", "e_Wlin/Wb", "-Wcont/Wlin", "e_-Wcont/Wlin"]
    with open(salida, "w") as h:
        h.write(f"# Balance de energía por período, medias entre períodos "
                f"completos y errores estándar de la media entre bloques de "
                f"{args.bloque} períodos.\n"
                f"# Energías en {ue.strip('$')} por período (K: media, en "
                f"{ue.strip('$')}). Ver plot-energia.py.\n")
        h.write("# " + " ".join(cols) + "\n")
        for d, f, p, _, pb in corridas:
            fila = [f"{d}", f"{len(p['Wb'])}", f"{len(pb['Wb'])}",
                    f"{np.mean(pb['N']):.1f}"]
            p = pb
            for k in claves:
                m, e = media_error(p[k] * fe)
                fila += [f"{m:.6e}", f"{e:.6e}"]
            for a, b in ((p["Wlin"], p["Wb"]), (-p["Wcont"], p["Wlin"])):
                r, e = cociente_error(a, b)
                fila += [f"{r:.6e}", f"{e:.6e}"]
            h.write(" ".join(fila) + "\n")


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("archivos", nargs="*",
                    help="energy_<pre>.dat (por defecto, los de . y frames_*/)")
    ap.add_argument("--etiqueta", help="sufijo de las salidas: energia-*-<E>")
    ap.add_argument("--bloque", type=int, default=2,
                    help="períodos por bloque para medias y errores (def. 2)")
    ap.add_argument("--reducidas", action="store_true",
                    help="unidades de la simulación (m g d) en lugar de mJ y s")
    ap.add_argument("--sin-tex", action="store_true",
                    help="no usar LaTeX para el texto")
    args = ap.parse_args()

    if not args.sin_tex:
        plt.rcParams.update({
            'text.usetex': True,
            'font.family': 'serif',
            'font.serif': ['Linux Libertine O'],
        })
    plt.rcParams.update({'font.size': 12, 'legend.fontsize': 9})

    archivos = args.archivos or sorted(
        f for f in glob.glob("energy_*.dat") + glob.glob("frames_*/energy_*.dat")
        if not Path(f).name.startswith("energy_fase_"))
    if args.bloque < 1:
        ap.error("--bloque debe ser >= 1")
    corridas = leer_corridas(archivos, args.bloque)
    if not corridas:
        print("No hay balances de energía para graficar.", file=sys.stderr)
        return 1

    fe, ue, _, _ = unidades_de(args)
    for d, f, _, _, p in corridas:
        wb, e = media_error(p["Wb"] * fe)
        r, er = cociente_error(-p["Wcont"], p["Wlin"])
        print(f"D = {d:3d}: {len(p['Wb'])} bloques, Wb = {wb:.4g} ± {e:.2g} "
              f"{ue.strip('$')}, -Wcont/Wlin = {r:.3f} ± {er:.3f}  ({f})")

    suf = f"-{args.etiqueta}" if args.etiqueta else ""
    figura_D(corridas, args, f"energia-D{suf}.pdf")
    figura_fase(corridas, args, f"energia-fase{suf}.pdf")
    figura_periodos(corridas, args, f"energia-periodos{suf}.pdf")
    tabla_D(corridas, args, f"energia-D{suf}.dat")
    print(f"Guardados energia-D{suf}.pdf, energia-fase{suf}.pdf, "
          f"energia-periodos{suf}.pdf y energia-D{suf}.dat")
    return 0


if __name__ == "__main__":
    sys.exit(main())
