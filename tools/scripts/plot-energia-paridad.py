#!/usr/bin/env python3
"""Balance de energía separado en períodos pares e impares.

Uso:
    plot-energia-paridad.py [archivos ...] [--bines B] [--etiqueta E]
                            [--reducidas] [--sin-tex]

Sin archivos, lee los energy_<pre>.dat del directorio actual y de
frames_*/. La respuesta del lecho alterna de un período al siguiente
(período 2; ver docs/balance-energia), así que se separan los períodos
completos según la paridad de b = floor(f t):

  - por período, de energy_<pre>.dat: medias de Wb, Wlin, -Wcont y K en los
    períodos pares y en los impares (errores: error estándar de la media
    entre los períodos de cada paridad);
  - en fase, de los <pre>_*.ve que están junto a energy_<pre>.dat: la
    energía cinética total <K> en función de la fase, por paridad. Cada
    frame se asigna al período por su t (t_ini <= t < t_fin del archivo de
    energía) y al bin por la fase de su cabecera.

El archivo de fase del simulador (energy_fase_<pre>.dat) suma todos los
períodos, así que los trabajos (Wb, Wlin, Wcont) en fase no se pueden
separar por paridad con las salidas actuales; <K> sí, porque los .ve tienen
la velocidad de cada grano.

Qué paridad es la "alta" es arbitrario (depende de cuándo empieza el
registro) y puede cambiar dentro de una corrida si la alternancia salta un
período. Por eso las figuras no comparan pares con impares sino la
paridad de W_lin alto (la de mayor W_lin medio) con la de W_lin bajo, y
dicen cuál es cada una. La tabla da las medias por paridad, la paridad
alta y la coherencia: la fracción de pares de períodos consecutivos en los
que el de paridad alta tiene más W_lin.

Salidas (con --etiqueta E, sufijo -E):
    energia-paridad-fase.pdf   <K> en fase en los períodos de W_lin alto
                               (línea) y bajo (trazos), un panel por D
    energia-paridad-D.pdf      medias en los períodos de W_lin alto y bajo
                               en función de D
    energia-paridad.dat        tabla de las medias por paridad
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
import unidades  # noqa: E402
from perfiles_comun import d_de_archivo  # noqa: E402

COLUMNAS = ["periodo", "t_ini", "t_fin", "npasos", "N", "Wb", "Wlin", "Wpiv",
            "Wg", "Dslide", "Dpiv", "Wcont", "dK", "dK_reiny", "K"]
CANT = [("Wb", r"$W_b$", 1), ("Wlin", r"$W_\mathrm{lin}$", 1),
        ("Wcont", r"$-W_\mathrm{cont}$", -1), ("K", r"$\langle K \rangle$", 1)]


def leer_periodos(f):
    datos = np.loadtxt(f, comments="#", ndmin=2)
    p = {k: datos[:, i] for i, k in enumerate(COLUMNAS)}
    ok = p["npasos"] >= 0.95 * p["npasos"].max()
    return {k: v[ok] for k, v in p.items()}


def cabecera_ve(f):
    """(t, fase) de la cabecera de un .ve."""
    with open(f) as h:
        for linea in h:
            if not linea.lstrip().startswith("#"):  # la 1.a línea es ' ## ...'
                break
            c = linea.split()
            if "t:" in c and "fase:" in c:
                return float(c[c.index("t:") + 1]), float(c[c.index("fase:") + 1])
    raise ValueError(f"{f}: sin 't:' y 'fase:' en la cabecera")


def k_en_fase(directorio, pre, p, bines):
    """<K> por (paridad, bin de fase) de los .ve; y conteo de frames."""
    suma = np.zeros((2, bines))
    n = np.zeros((2, bines))
    archivos = sorted(Path(directorio).glob(f"{pre}_*.ve"))
    for f in archivos:
        t, fase = cabecera_ve(f)
        i = np.searchsorted(p["t_fin"], t, side="right")
        if i >= len(p["t_fin"]) or t < p["t_ini"][i]:
            continue  # fuera de los períodos completos
        par = int(p["periodo"][i]) % 2
        ib = min(int(bines * fase / (2 * np.pi)), bines - 1)
        e = np.loadtxt(f, comments="#", usecols=(7, 8), ndmin=2)
        suma[par, ib] += e.sum()
        n[par, ib] += 1
    with np.errstate(invalid="ignore"):
        return suma / n, n, len(archivos)


def media_error(x):
    if len(x) < 2:
        return np.mean(x), np.nan
    return np.mean(x), np.std(x, ddof=1) / np.sqrt(len(x))


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("archivos", nargs="*",
                    help="energy_<pre>.dat (por defecto, los de . y frames_*/)")
    ap.add_argument("--bines", type=int, default=20,
                    help="bines de fase para <K> (por defecto 20)")
    ap.add_argument("--etiqueta", help="sufijo de las salidas")
    ap.add_argument("--reducidas", action="store_true",
                    help="unidades de la simulación (m g d) en lugar de mJ")
    ap.add_argument("--sin-tex", action="store_true")
    args = ap.parse_args()

    if not args.sin_tex:
        plt.rcParams.update({"text.usetex": True, "font.family": "serif",
                             "font.serif": ["Linux Libertine O"]})
    plt.rcParams.update({"font.size": 11, "legend.fontsize": 9})
    fe, ue = (1.0, "$mgd$") if args.reducidas else (unidades.ENERGIA * 1e3, "mJ")

    archivos = args.archivos or sorted(
        f for f in glob.glob("energy_*.dat") + glob.glob("frames_*/energy_*.dat")
        if not Path(f).name.startswith("energy_fase_"))
    corridas = []
    for f in archivos:
        d = d_de_archivo(f)
        if d is None:
            print(f"aviso: {f} ignorado (el sufijo no es D)", file=sys.stderr)
            continue
        pre = Path(f).name[len("energy_"):-len(".dat")]
        p = leer_periodos(f)
        kf, n, nve = k_en_fase(Path(f).parent, pre, p, args.bines)
        if nve == 0:
            print(f"aviso: no hay {pre}_*.ve junto a {f}; sin <K> en fase",
                  file=sys.stderr)
        par = p["periodo"].astype(int) % 2
        # Paridad "alta": la de mayor W_lin medio. Coherencia: fracción de
        # pares consecutivos (n, n+1) en que el de paridad alta tiene más W_lin
        alta = int(np.mean(p["Wlin"][par == 1]) > np.mean(p["Wlin"][par == 0]))
        w = p["Wlin"]
        cons = np.diff(p["periodo"]) == 1
        signo = np.where(par[:-1] == alta, 1, -1) * np.sign(w[:-1] - w[1:])
        coher = np.mean(signo[cons] > 0) if cons.any() else np.nan
        corridas.append(dict(d=d, f=f, p=p, par=par, kf=kf, n=n, nve=nve,
                             alta=alta, coher=coher))
        print(f"D = {d:3d}: {np.sum(par == 0)} pares, {np.sum(par == 1)} "
              f"impares, {nve} .ve; Wlin alto en los "
              f"{'impares' if alta else 'pares'} (coherencia {coher:.2f})")
    if not corridas:
        print("No hay balances de energía.", file=sys.stderr)
        return 1
    corridas.sort(key=lambda c: c["d"])
    suf = f"-{args.etiqueta}" if args.etiqueta else ""

    # Tabla
    with open(f"energia-paridad{suf}.dat", "w") as h:
        h.write("# Balance por período separado por paridad de b = floor(f t),"
                f" energías en {ue.strip('$')} por período (K: media).\n"
                "# alta: paridad con más W_lin (0 par, 1 impar); coher: "
                "fracción de períodos consecutivos que respetan esa paridad.\n")
        cols = ["D", "n_par", "n_impar", "alta", "coher"]
        for k, _, _ in CANT:
            cols += [f"{k}_par", f"e_{k}_par", f"{k}_impar", f"e_{k}_impar"]
        h.write("# " + " ".join(cols) + "\n")
        for c in corridas:
            p, par = c["p"], c["par"]
            fila = [str(c["d"]), str(np.sum(par == 0)), str(np.sum(par == 1)),
                    str(c["alta"]), f"{c['coher']:.3f}"]
            for k, _, s in CANT:
                for q in (0, 1):
                    m, e = media_error(s * p[k][par == q] * fe)
                    fila += [f"{m:.6e}", f"{e:.6e}"]
            h.write(" ".join(fila) + "\n")

    # Medias por paridad en función de D
    ds = np.array([c["d"] for c in corridas])
    fig, axs = plt.subplots(2, 2, figsize=(10, 7), sharex=True)
    for ax, (k, etiqueta, s) in zip(axs.flat, CANT):
        for alto, mk, nombre in ((True, "o-", "alto"), (False, "s--", "bajo")):
            me = np.array([media_error(
                s * c["p"][k][c["par"] == (c["alta"] if alto else 1 - c["alta"])]
                * fe) for c in corridas])
            ax.errorbar(ds, me[:, 0], yerr=me[:, 1], fmt=mk, capsize=3,
                        alpha=0.7,
                        label=fr"períodos de $W_\mathrm{{lin}}$ {nombre}")
        unidad = ue if k == "K" else f"{ue} por período"
        ax.set_ylabel(f"{etiqueta} ({unidad})")
        ax.axhline(0, color="0.6", lw=0.6)
    for ax in axs[1]:
        ax.set_xlabel(r"$D/d$")
    axs[0, 0].legend()
    fig.suptitle("Balance por período: paridad de $W_\\mathrm{lin}$ alto y "
                 "de $W_\\mathrm{lin}$ bajo")
    fig.tight_layout()
    fig.savefig(f"energia-paridad-D{suf}.pdf")
    plt.close(fig)

    # <K> en fase por paridad, un panel por D
    con_ve = [c for c in corridas if c["nve"] > 0]
    if con_ve:
        norm = colors.Normalize(vmin=3, vmax=14)
        cmap = mpl.colormaps["plasma"]
        ncol = 4
        nfil = int(np.ceil(len(con_ve) / ncol))
        fig, axs = plt.subplots(nfil, ncol, figsize=(12, 2.6 * nfil + 0.6),
                                sharex=True, sharey=True, squeeze=False)
        x = (np.arange(args.bines) + 0.5) / args.bines
        for ax, c in zip(axs.flat, con_ve):
            col = cmap(norm(c["d"]))
            a = c["alta"]
            ax.plot(x, c["kf"][a] * fe, "o-", color=col, alpha=0.7, ms=3,
                    label=r"$W_\mathrm{lin}$ alto")
            ax.plot(x, c["kf"][1 - a] * fe, "s--", color=col, alpha=0.7, ms=3,
                    label=r"$W_\mathrm{lin}$ bajo")
            alta = "impares" if a else "pares"
            ax.set_title(fr"$D = {c['d']}\,d$ (alto: {alta}, "
                         fr"coherencia {c['coher']:.2f})", fontsize=10)
        for ax in axs.flat[len(con_ve):]:
            ax.set_visible(False)
        for ax in axs[:, 0]:
            ax.set_ylabel(fr"$\langle K \rangle$ ({ue})")
        for ax in axs[-1]:
            ax.set_xlabel(r"fase $f t \; \mathrm{mod} \; 1$")
        axs[0, 0].legend()
        fig.suptitle("Energía cinética en fase, en los períodos de "
                     r"$W_\mathrm{lin}$ alto y bajo")
        fig.tight_layout()
        fig.savefig(f"energia-paridad-fase{suf}.pdf")
        plt.close(fig)

    print(f"Guardados energia-paridad-D{suf}.pdf, energia-paridad-fase{suf}.pdf"
          f" y energia-paridad{suf}.dat")
    return 0


if __name__ == "__main__":
    sys.exit(main())
