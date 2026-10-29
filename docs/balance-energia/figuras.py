#!/usr/bin/env python3
"""Figuras y números de balance-energia.tex a partir de una tanda analizada.

Uso:
    python3 figuras.py /ruta/a/la/tanda [--sin-tex]

La tanda es el directorio donde corrió tools/scripts/analizar-todo.sh: lee
datos/frames_walker/energy_frm-<NN>.dat, datos/flx-<NN>.dat,
datos/p-*.in y analisis/perfil-ve-<NN>.dat. Escribe en figuras/:

    energia-D.pdf, energia-fase.pdf, energia-periodos.pdf
                          (las de tools/scripts/plot-energia.py)
    alternancia.pdf       W_lin por período para cuatro D: respuesta de
                          período 2
    base.pdf              velocidad de la base y trabajo de la base en fase
y en numeros.txt los valores que cita el texto.
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import matplotlib as mpl
import matplotlib.colors as colors
import numpy as np

AQUI = Path(__file__).resolve().parent
SCRIPTS = AQUI.parent.parent / "tools" / "scripts"
sys.path.insert(0, str(SCRIPTS))
import unidades  # noqa: E402

COLS = ("periodo t_ini t_fin npasos N Wb Wlin Wpiv Wg Dslide Dpiv Wcont dK "
        "dK_reiny K").split()
COLS_FASE = ["fase", "dfase"] + COLS[3:]


def param(f, clave):
    for linea in open(f):
        c = linea.replace("\r", "").split()
        if c and c[0] == clave + ":":
            return float(c[1])
    raise KeyError(clave)


def leer(f, cols):
    a = np.loadtxt(f, comments="#", ndmin=2)
    return {k: a[:, i] for i, k in enumerate(cols)}


def completos(p):
    ok = p["npasos"] >= 0.95 * p["npasos"].max()
    return {k: v[ok] for k, v in p.items()}


def r1(x):
    """Autocorrelación entre períodos consecutivos."""
    x = x - x.mean()
    return np.sum(x[1:] * x[:-1]) / np.sum(x * x)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("tanda")
    ap.add_argument("--sin-tex", action="store_true")
    args = ap.parse_args()
    tanda = Path(args.tanda).resolve()
    datos = tanda / "datos"
    salida = AQUI / "figuras"
    salida.mkdir(exist_ok=True)

    energias = sorted((datos / "frames_walker").glob("energy_frm-*.dat"))
    cmd = [sys.executable, str(SCRIPTS / "plot-energia.py"), *map(str, energias)]
    if args.sin_tex:
        cmd.append("--sin-tex")
    subprocess.run(cmd, cwd=salida, check=True, stdout=subprocess.DEVNULL)
    (salida / "energia-D.dat").unlink(missing_ok=True)

    if not args.sin_tex:
        plt.rcParams.update({"text.usetex": True, "font.family": "serif",
                             "font.serif": ["Linux Libertine O"]})
    plt.rcParams.update({"font.size": 11, "legend.fontsize": 9})

    # Parámetros de la tanda (iguales en todas las corridas salvo el radio)
    pfile = sorted(datos.glob("p-*.in"))[0]
    f = param(pfile, "Frecuencia_exitacion")
    gamma = param(pfile, "Amplitud_exitacion_gamma")
    rho = param(pfile, "rho")
    phi = param(pfile, "fase_phi")
    T = 1.0 / f
    w = 2 * np.pi * f
    av = gamma / w  # g = 1

    # Números por D
    eJ = unidades.ENERGIA * 1e3  # m g d -> mJ
    filas = []
    corridas = {}
    for e in energias:
        d = int(re.search(r"-(\d+)\.dat$", e.name).group(1))
        pfile_d = next(p for p in datos.glob("p-*.in")
                       if f"frm-{d:02d}" in open(p).read())
        mu_d = float(open(pfile_d).read().replace("\r", "")
                     .split("noTipoGranos:")[1].split("\n")[1].split()[6])
        p = completos(leer(e, COLS))
        corridas[d] = p
        n = p["N"][0]
        fl = np.loadtxt(datos / f"flx-{d:02d}.dat", comments="#", ndmin=2)
        t = fl[:, 2]
        ndesc = np.sum((t >= p["t_ini"][0]) & (t < p["t_fin"][-1]))
        k_sal = -np.sum(p["dK_reiny"]) / ndesc if ndesc else np.nan
        # Caudal en todo el régimen de descarga
        q = (len(t) - 1) / (t.max() - t.min())
        filas.append(dict(
            D=d, nper=len(p["Wb"]),
            r1_Wb=r1(p["Wb"]), r1_Wlin=r1(p["Wlin"]), r1_K=r1(p["K"]),
            alt=np.mean(np.abs(np.diff(p["Wlin"]))) / 2 / np.mean(p["Wlin"]),
            v_desl=np.mean(p["Dslide"]) / n / (mu_d * T),
            ndesc_per=ndesc / len(p["Wb"]), K_sal=k_sal,
            v_sal=np.sqrt(2 * k_sal), caudal=q,
            Wb_quieto=n * mu_d * T,  # por <|v_b|>, abajo
        ))

    tt = np.linspace(0, T, 20001)[:-1]
    vb = -(rho * av * np.cos(w * tt) + (1 - rho) / 2 * av * np.cos(2 * w * tt + phi))
    vb_abs = np.mean(np.abs(vb))

    with open(salida / "numeros.txt", "w") as h:
        h.write(f"# tanda: {tanda}\n")
        h.write(f"# f = {f}, T = {T:.6f}, Gamma = {gamma}, rho = {rho}, "
                f"phi = {phi}\n")
        h.write(f"# amplitud de velocidad Gamma g / w = {av:.4f}; "
                f"max|v_b| = {np.abs(vb).max():.4f}; <|v_b|> = {vb_abs:.4f};"
                f" rms = {np.sqrt(np.mean(vb**2)):.4f} (sqrt(g d))\n")
        h.write("# r1_*: autocorrelación entre períodos consecutivos; alt: "
                "<|W_lin(n+1) - W_lin(n)|>/2 / <W_lin>\n")
        h.write("# v_desl = <D_slide> / (N mu_d m g T) (sqrt(g d)); "
                "Wb/Wb_quieto con Wb_quieto = N mu_d m g <|v_b|> T\n")
        h.write("# ndesc_per: granos que salen por período; K_sal: energía "
                "por grano que sale (m g d); v_sal = sqrt(2 K_sal / m); "
                "caudal: granos por unidad de tiempo\n")
        h.write("D nper r1_Wb r1_Wlin r1_K alt v_desl Wb/Wb_quieto ndesc_per "
                "K_sal v_sal caudal\n")
        for r in filas:
            p = corridas[r["D"]]
            h.write(f"{r['D']} {r['nper']} {r['r1_Wb']:+.2f} {r['r1_Wlin']:+.2f} "
                    f"{r['r1_K']:+.2f} {r['alt']:.2f} {r['v_desl']:.4f} "
                    f"{np.mean(p['Wb']) / (r['Wb_quieto'] * vb_abs):.4f} "
                    f"{r['ndesc_per']:.2f} {r['K_sal']:.5f} {r['v_sal']:.4f} "
                    f"{r['caudal']:.4f}\n")

    # Alternancia: W_lin por período para cuatro D
    norm = colors.Normalize(vmin=3, vmax=14)
    cmap = mpl.colormaps["plasma"]
    fig, axs = plt.subplots(2, 2, figsize=(9, 5.5), sharex=True, sharey=True)
    for ax, d in zip(axs.flat, (3, 7, 12, 13)):
        p = corridas[d]
        x = p["periodo"] - p["periodo"][0]
        ax.plot(x, p["Wlin"] * eJ, "o-", color=cmap(norm(d)), alpha=0.7,
                ms=4)
        ax.axhline(np.mean(p["Wlin"]) * eJ, color="0.4", lw=0.8, ls="--")
        ax.axhline(0, color="0.6", lw=0.6)
        fila = next(r for r in filas if r["D"] == d)
        ax.set_title(fr"$D = {d}\,d$: $r_1 = {fila['r1_Wlin']:+.2f}$",
                     fontsize=10)
    for ax in axs[:, 0]:
        ax.set_ylabel(r"$W_\mathrm{lin}$ (mJ por período)")
    for ax in axs[1]:
        ax.set_xlabel("período")
    fig.tight_layout()
    fig.savefig(salida / "alternancia.pdf")
    plt.close(fig)

    # Base: velocidad y trabajo de la base en fase
    fig, ax = plt.subplots(figsize=(6.4, 3.6))
    x = tt / T
    ax.plot(x, vb * unidades.VELOCIDAD * unidades.CM, color="0.2", lw=1.2,
            label=r"$v_b$ (cm/s)")
    ax.set_xlabel(r"fase $f t \; \mathrm{mod} \; 1$")
    ax.set_ylabel(r"$v_b$ (cm/s)")
    ax.axhline(0, color="0.6", lw=0.6)
    ax2 = ax.twinx()
    for d in (3, 14):
        fa = leer(datos / "frames_walker" / f"energy_fase_frm-{d:02d}.dat",
                  COLS_FASE)
        ax2.plot(fa["fase"] / (2 * np.pi),
                 fa["Wb"] * eJ / (fa["dfase"] / (2 * np.pi)), "o-",
                 color=cmap(norm(d)), alpha=0.7, ms=3,
                 label=fr"$W_b$, $D = {d}\,d$")
    # Granos quietos: F = mu_d m g sign(v_b), W_b por unidad de fase =
    # N mu_d m g |v_b| T
    n = corridas[14]["N"][0]
    ax2.plot(x, n * mu_d * np.abs(vb) * T * eJ, "--", color="0.5", lw=1,
             label=r"$N \mu_d m g |v_b| T$")
    ax2.set_ylabel(r"$W_b$ (mJ por período por unidad de fase)")
    l1, e1 = ax.get_legend_handles_labels()
    l2, e2 = ax2.get_legend_handles_labels()
    ax.legend(l1 + l2, e1 + e2, fontsize=8, ncols=4, loc="lower center",
              bbox_to_anchor=(0.5, 1.0), frameon=False)
    ax.set_xlim(0, 1)
    fig.tight_layout()
    fig.savefig(salida / "base.pdf")
    plt.close(fig)
    print(f"Figuras y números en {salida}")


if __name__ == "__main__":
    main()
