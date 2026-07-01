#!/usr/bin/env python3

import argparse
import sys
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.colors as colors
import numpy as np

plt.rcParams.update({
    'text.usetex': True,
    'font.family': 'serif',
    'font.serif': ['Linux Libertine O'],
    'font.size': 14,
    'axes.titlesize': 14,
    'axes.labelsize': 14,
    'legend.fontsize': 8,
    'xtick.labelsize': 14,
    'ytick.labelsize': 14
})

def parse_args():
    parser = argparse.ArgumentParser(description="Plot heatmaps from force_map2d output.")
    parser.add_argument('input_file', help="The .dat file produced by force_map2d")
    parser.add_argument('-o', '--output', help="Output file name (e.g. heatmap.pdf)", default=None)
    parser.add_argument('--vmax-norm', type=float, help="Max val for Normal Force colorscale", default=None)
    parser.add_argument('--vmax-tan', type=float, help="Max val for Tangential Force colorscale", default=None)
    return parser.parse_args()

def read_metadata(filename):
    meta = {}
    with open(filename, 'r') as f:
        first_line = f.readline()
        if first_line.startswith("# force_map2d"):
            parts = first_line.split()[2:] # skip "# force_map2d"
            for p in parts:
                if '=' in p:
                    k, v = p.split('=')
                    meta[k] = float(v) if '.' in v or 'e' in v else int(v)
    return meta

def main():
    args = parse_args()
    
    meta = read_metadata(args.input_file)
    if not meta:
        print("Warning: metadata header not found.")
        
    try:
        data = np.loadtxt(args.input_file, comments='#')
    except Exception as e:
        print(f"Error loading {args.input_file}: {e}")
        sys.exit(1)
        
    x = data[:, 0]
    y = data[:, 1]
    norm_force = data[:, 2]
    tan_force = data[:, 3]
    
    n_bins_x = int(meta.get('n_bins_x', len(np.unique(x))))
    n_bins_y = int(meta.get('n_bins_y', len(np.unique(y))))
    
    x_M = data[:, 0].reshape((n_bins_y, n_bins_x))
    y_M = data[:, 1].reshape((n_bins_y, n_bins_x))
    fn_M = norm_force.reshape((n_bins_y, n_bins_x))
    ft_M = tan_force.reshape((n_bins_y, n_bins_x))

    # Conversión de unidades (según scripts previos)
    cm_factor = 100.0  # asumiendo y en cm
    f_s_2_e = 0.002058 # conversión de fuerza
    
    x_M_cm = x_M * cm_factor
    y_M_cm = y_M * cm_factor
    fn_M_e = fn_M * f_s_2_e
    ft_M_e = ft_M * f_s_2_e

    # Para omitir ceros (background), usamos NaN
    fn_M_e[fn_M_e == 0] = np.nan
    ft_M_e[ft_M_e == 0] = np.nan

    xmin, xmax = x_M_cm.min(), x_M_cm.max()
    ymin, ymax = y_M_cm.min(), y_M_cm.max()
    extent = [xmin, xmax, ymin, ymax]

    fig, axes = plt.subplots(1, 2, figsize=(10, 5), sharey=True)

    cmap = mpl.colormaps['plasma']
    cmap.set_bad(color='white')

    vmax_n = args.vmax_norm if args.vmax_norm else np.nanmax(fn_M_e)
    vmax_t = args.vmax_tan if args.vmax_tan else np.nanmax(ft_M_e)

    im0 = axes[0].imshow(fn_M_e, origin='lower', extent=extent, cmap=cmap, vmax=vmax_n, aspect='equal')
    axes[0].set_title(r"Fuerza Normal $\langle f_N \rangle$ (N)")
    axes[0].set_xlabel(r"$x$ (cm)")
    axes[0].set_ylabel(r"$y$ (cm)")
    fig.colorbar(im0, ax=axes[0], fraction=0.046, pad=0.04)

    im1 = axes[1].imshow(ft_M_e, origin='lower', extent=extent, cmap=cmap, vmax=vmax_t, aspect='equal')
    axes[1].set_title(r"Fuerza Tangencial $\langle |f_T| \rangle$ (N)")
    axes[1].set_xlabel(r"$x$ (cm)")
    fig.colorbar(im1, ax=axes[1], fraction=0.046, pad=0.04)

    plt.tight_layout()
    
    out_file = args.output
    if out_file is None:
        out_file = args.input_file.replace('.dat', '.pdf')
        if out_file == args.input_file:
            out_file += '.pdf'
            
    plt.savefig(out_file, dpi=300, bbox_inches='tight')
    print(f"Mapa guardado en {out_file}")

if __name__ == '__main__':
    main()
