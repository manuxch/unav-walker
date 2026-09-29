/*! \file stress_profile.cpp
 * \brief Perfil del tensor de estrés a lo largo de y, en una franja
 *        vertical centrada (por ejemplo, del ancho del orificio D), a partir
 *        de los archivos .sxy del simulador (versión >= 3.0).
 *
 * \verbatim
 * Uso:
 *   stress_profile <dir> <pre> -o <salida> --half-width W --y-max Y
 *                  --freq F [opciones]
 *
 * Argumentos:
 *   dir              Directorio con los archivos <pre>_<frame>.sxy
 *   pre              Prefijo de los archivos (preFrameFile), p. ej. frm-100
 *   -o <salida>      Archivo de salida (perfil promediado en fase)
 *   --half-width W   Semiancho de la franja: |x - x_c| <= W (W = D/2)
 *   --y-max Y        Límite superior del rango en y
 *   --freq F         Frecuencia de la excitación (Frecuencia_exitacion);
 *                    define los bloques de un período para los errores y se
 *                    verifica contra la fase de la cabecera de cada archivo
 *
 * Opciones:
 *   --y-min Y0        Límite inferior del rango en y (default 0)
 *   --dy DY           Alto de los bines (default 1, un diámetro)
 *   --x-center XC     Centro de la franja (default 0)
 *   --t-min T0        Solo frames con t >= T0
 *   --t-max T1        Solo frames con t <= T1
 *   --phase-bins P    Bines de fase de la excitación (default 20). La
 *                     velocidad media de referencia de la parte cinética se
 *                     calcula por bin de y y de fase.
 *   --phase-output A  Escribe además el perfil resuelto en fase en A
 *   --threads N       Threads (default: hardware_concurrency)
 *
 * Definiciones (ver README.md de tools/):
 *
 *   Tensor de contacto (estrés sobre los granos, índices xx xy yx yy):
 *     s = sum_p A_p s_p / sum_p A_p
 *   con s_p el tensor del grano p (columnas sxx..syy del .sxy) y A_p su área.
 *   sn es la parte debida a las fuerzas normales y st = s - sn la
 *   tangencial.
 *
 *   Parte cinética (misma normalización, se suma a s):
 *     k = - sum_p m_p v'_p v'_p / sum_p A_p,   v' = v - <v>(y, fase)
 *   con <v>(y, fase) la velocidad media (pesada por masa) en el bin de y y
 *   el bin de fase. Así la oscilación coherente con la base no se cuenta
 *   como agitación.
 *
 *   phi: fracción de área por centros = sum A_p / (A_bin N_frames). No es
 *   una fracción de empaquetamiento física (puede superar 1 en franjas
 *   angostas, porque los granos con centro en la franja cuentan su área
 *   completa); es el factor que lleva s y k al estrés medio del bin:
 *   phi (s + k).
 *
 *   Errores (e_*): desviación estándar de las medias por bloques de un
 *   período, dividida por sqrt(número de bloques).
 * \endverbatim
 *
 * \author Manuel Carlevaro
 * \date 2026-09-29
 */

#include "dem_reader.hpp"
#include "thread_pool.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
#include <numbers>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
constexpr double kPi = std::numbers::pi;
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

// ============================================================================
// Acumuladores
// ============================================================================

/// Sumas por bin de y (y de fase).
struct Acc {
    size_t n = 0;            ///< Muestras grano-frame
    double A = 0.0;          ///< sum A_p
    double As[4] = {};       ///< sum A_p s_p
    double Asn[4] = {};      ///< sum A_p sn_p
    double m = 0.0;          ///< sum m_p
    double mv[2] = {};       ///< sum m_p v_p
    double mvv[3] = {};      ///< sum m_p v_i v_j (xx, xy, yy)

    void add(const dem::GrainStress& g) {
        const double a = kPi * g.r * g.r;
        ++n;
        A += a;
        for (int k = 0; k < 4; ++k) {
            As[k] += a * g.s[k];
            Asn[k] += a * g.sn[k];
        }
        m += g.m;
        mv[0] += g.m * g.vx;
        mv[1] += g.m * g.vy;
        mvv[0] += g.m * g.vx * g.vx;
        mvv[1] += g.m * g.vx * g.vy;
        mvv[2] += g.m * g.vy * g.vy;
    }
    void merge(const Acc& o) {
        n += o.n;
        A += o.A;
        for (int k = 0; k < 4; ++k) {
            As[k] += o.As[k];
            Asn[k] += o.Asn[k];
        }
        m += o.m;
        for (int k = 0; k < 2; ++k) mv[k] += o.mv[k];
        for (int k = 0; k < 3; ++k) mvv[k] += o.mvv[k];
    }
    /// sum m v'v' (xx, xy, yy) respecto de la media de este acumulador.
    void fluct(double out[3]) const {
        if (m <= 0.0) { out[0] = out[1] = out[2] = 0.0; return; }
        out[0] = mvv[0] - mv[0] * mv[0] / m;
        out[1] = mvv[1] - mv[0] * mv[1] / m;
        out[2] = mvv[2] - mv[1] * mv[1] / m;
    }
};

/// Sumas por bloque (período) y bin de y, para los errores.
struct BlockAcc {
    double A = 0.0;
    double As[4] = {};
    double Asn[4] = {};
};

struct Params {
    fs::path    dir;
    std::string pre;
    fs::path    output;
    fs::path    phase_output;
    double half_width = -1.0;
    double x_center = 0.0;
    double y_min = 0.0;
    double y_max = kNaN;
    double dy = 1.0;
    double freq = -1.0;
    double t_min = -std::numeric_limits<double>::infinity();
    double t_max = std::numeric_limits<double>::infinity();
    int    phase_bins = 20;
    size_t n_threads = std::thread::hardware_concurrency();
    int    n_bins = 0;
};

/// Resultados acumulados de todos los frames.
struct Global {
    std::mutex mtx;
    std::vector<std::vector<Acc>> acc;           ///< [fase][bin y]
    std::vector<size_t> frames_per_phase;        ///< frames por bin de fase
    std::map<long, std::vector<BlockAcc>> blocks; ///< [bloque][bin y]
    size_t n_frames = 0;
    double t_first = std::numeric_limits<double>::infinity();
    double t_last = -std::numeric_limits<double>::infinity();
    std::set<std::string> git, params_hash, params;
    double max_phase_err = 0.0;
    std::vector<std::string> errors;
};

// ============================================================================
// Tarea por frame
// ============================================================================

struct FrameTask {
    fs::path path;
    const Params* p;
    Global* glob;

    void operator()() const {
        dem::SXYFrame fr;
        try {
            fr = dem::read_sxy(path);
        } catch (const std::exception& e) {
            std::lock_guard lock(glob->mtx);
            glob->errors.push_back(e.what());
            return;
        }
        const double t = fr.prov.t;
        if (t < p->t_min || t > p->t_max) return;

        // Fase de la cabecera y verificación de la frecuencia
        const double phase = fr.prov.phase;
        double expected = std::fmod(2.0 * kPi * p->freq * t, 2.0 * kPi);
        double err = std::fabs(expected - phase);
        err = std::min(err, 2.0 * kPi - err);
        int ph = static_cast<int>(phase / (2.0 * kPi) * p->phase_bins);
        ph = std::clamp(ph, 0, p->phase_bins - 1);
        const long block = static_cast<long>(std::floor(t * p->freq));

        std::vector<Acc> local(p->n_bins);
        std::vector<BlockAcc> local_block(p->n_bins);
        for (const auto& g : fr.grains) {
            if (std::fabs(g.x - p->x_center) > p->half_width) continue;
            const double rel = g.y - p->y_min;
            if (rel < 0.0) continue;
            const int ib = static_cast<int>(rel / p->dy);
            if (ib >= p->n_bins) continue;
            local[ib].add(g);
            const double a = kPi * g.r * g.r;
            BlockAcc& b = local_block[ib];
            b.A += a;
            for (int k = 0; k < 4; ++k) {
                b.As[k] += a * g.s[k];
                b.Asn[k] += a * g.sn[k];
            }
        }

        std::lock_guard lock(glob->mtx);
        for (int ib = 0; ib < p->n_bins; ++ib) glob->acc[ph][ib].merge(local[ib]);
        auto& gb = glob->blocks[block];
        if (gb.empty()) gb.resize(p->n_bins);
        for (int ib = 0; ib < p->n_bins; ++ib) {
            gb[ib].A += local_block[ib].A;
            for (int k = 0; k < 4; ++k) {
                gb[ib].As[k] += local_block[ib].As[k];
                gb[ib].Asn[k] += local_block[ib].Asn[k];
            }
        }
        ++glob->frames_per_phase[ph];
        ++glob->n_frames;
        glob->t_first = std::min(glob->t_first, t);
        glob->t_last = std::max(glob->t_last, t);
        glob->git.insert(fr.prov.git);
        glob->params_hash.insert(fr.prov.params_hash);
        glob->params.insert(fr.prov.params);
        glob->max_phase_err = std::max(glob->max_phase_err, err);
    }
};

// ============================================================================
// Argumentos
// ============================================================================

static void print_usage(const char* prog) {
    std::cout <<
        "Uso:\n"
        "  " << prog << " <dir> <pre> -o <salida> --half-width W --y-max Y "
        "--freq F\n"
        "      [--y-min Y0] [--dy DY] [--x-center XC] [--t-min T0] "
        "[--t-max T1]\n"
        "      [--phase-bins P] [--phase-output archivo] [--threads N]\n\n"
        "  dir   Directorio con los archivos <pre>_<frame>.sxy\n"
        "  pre   Prefijo de los archivos (preFrameFile)\n"
        "Ver la cabecera de stress_profile.cpp para las definiciones.\n";
}

static Params parse_args(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        std::exit(1);
    }
    Params p;
    p.dir = argv[1];
    p.pre = argv[2];
    for (int i = 3; i < argc; ++i) {
        std::string tok(argv[i]);
        auto next = [&]() -> std::string {
            if (++i >= argc)
                throw std::invalid_argument(tok + " requiere un argumento");
            return argv[i];
        };
        if (tok == "-o") p.output = next();
        else if (tok == "--half-width") p.half_width = std::stod(next());
        else if (tok == "--x-center") p.x_center = std::stod(next());
        else if (tok == "--y-min") p.y_min = std::stod(next());
        else if (tok == "--y-max") p.y_max = std::stod(next());
        else if (tok == "--dy") p.dy = std::stod(next());
        else if (tok == "--freq") p.freq = std::stod(next());
        else if (tok == "--t-min") p.t_min = std::stod(next());
        else if (tok == "--t-max") p.t_max = std::stod(next());
        else if (tok == "--phase-bins") p.phase_bins = std::stoi(next());
        else if (tok == "--phase-output") p.phase_output = next();
        else if (tok == "--threads") p.n_threads = std::stoul(next());
        else throw std::invalid_argument("argumento desconocido: " + tok);
    }
    if (p.output.empty()) throw std::invalid_argument("falta -o <salida>");
    if (p.half_width <= 0.0)
        throw std::invalid_argument("falta --half-width W (> 0)");
    if (std::isnan(p.y_max) || p.y_max <= p.y_min)
        throw std::invalid_argument("falta --y-max Y (> y_min)");
    if (p.dy <= 0.0) throw std::invalid_argument("--dy debe ser > 0");
    if (p.freq <= 0.0) throw std::invalid_argument("falta --freq F (> 0)");
    if (p.phase_bins < 1) throw std::invalid_argument("--phase-bins debe ser >= 1");
    if (p.n_threads == 0) p.n_threads = 1;
    p.n_bins = static_cast<int>(std::ceil((p.y_max - p.y_min) / p.dy - 1e-9));
    return p;
}

// ============================================================================
// Salida
// ============================================================================

static void write_value(std::ostream& out, double v) {
    if (std::isnan(v)) out << "  nan";
    else out << "  " << v;
}

/// Cabecera común de los archivos de salida.
static void write_header(std::ostream& out, const Params& p, const Global& g,
                         const std::string& title) {
    auto join = [](const std::set<std::string>& s) {
        std::string r;
        for (const auto& x : s) r += (r.empty() ? "" : ",") + x;
        return r;
    };
    out << "# " << title << "\n";
    out << "# dir=" << p.dir.string() << "  pre=" << p.pre
        << "  frames=" << g.n_frames << "  t=[" << g.t_first << ", "
        << g.t_last << "]  bloques=" << g.blocks.size() << "\n";
    out << "# git=" << join(g.git) << "  params=" << join(g.params)
        << "  params_hash=" << join(g.params_hash) << "\n";
    out << "# franja |x - " << p.x_center << "| <= " << p.half_width
        << "  y=[" << p.y_min << ", " << p.y_max << ")  dy=" << p.dy
        << "  freq=" << p.freq << "  phase_bins=" << p.phase_bins << "\n";
    out << "# s = sum A_p s_p / sum A_p (compresión < 0); st = s - sn; "
           "k = -sum m v'v' / sum A_p (v' respecto de <v>(y, fase));\n"
           "# phi = sum A_p / (A_bin N_frames); e_* = error estándar por "
           "bloques de un período; nan: sin datos.\n";
}

static void write_profile(const Params& p, Global& g) {
    std::ofstream out(p.output);
    if (!out) throw std::runtime_error("no se puede abrir " + p.output.string());
    write_header(out, p, g, "stress_profile (promedio en fase)");
    out << "# y n phi vx vy sxx sxy syx syy snxx snxy snyx snyy "
           "stxx stxy styx styy kxx kxy kyy "
           "e_sxx e_sxy e_syx e_syy e_snxx e_snxy e_snyx e_snyy\n";
    out << std::scientific << std::setprecision(6);
    const double a_bin = 2.0 * p.half_width * p.dy;
    for (int ib = 0; ib < p.n_bins; ++ib) {
        Acc tot;
        double fl[3] = {0.0, 0.0, 0.0}; // sum m v'v' respecto de <v>(y, fase)
        for (int ph = 0; ph < p.phase_bins; ++ph) {
            const Acc& a = g.acc[ph][ib];
            tot.merge(a);
            double f[3];
            a.fluct(f);
            for (int k = 0; k < 3; ++k) fl[k] += f[k];
        }
        const double y = p.y_min + (ib + 0.5) * p.dy;
        out << y << "  " << tot.n;
        const bool ok = tot.A > 0.0;
        write_value(out, tot.A / (a_bin * g.n_frames));
        write_value(out, ok ? tot.mv[0] / tot.m : kNaN);
        write_value(out, ok ? tot.mv[1] / tot.m : kNaN);
        for (int k = 0; k < 4; ++k) write_value(out, ok ? tot.As[k] / tot.A : kNaN);
        for (int k = 0; k < 4; ++k) write_value(out, ok ? tot.Asn[k] / tot.A : kNaN);
        for (int k = 0; k < 4; ++k)
            write_value(out, ok ? (tot.As[k] - tot.Asn[k]) / tot.A : kNaN);
        for (int k = 0; k < 3; ++k) write_value(out, ok ? -fl[k] / tot.A : kNaN);
        // Errores por bloques de un período
        for (int part = 0; part < 2; ++part) {
            for (int k = 0; k < 4; ++k) {
                std::vector<double> means;
                for (const auto& [blk, vb] : g.blocks) {
                    if (vb[ib].A > 0.0)
                        means.push_back((part == 0 ? vb[ib].As[k] : vb[ib].Asn[k])
                                        / vb[ib].A);
                }
                double e = kNaN;
                if (means.size() >= 2) {
                    double mu = 0.0;
                    for (double v : means) mu += v;
                    mu /= means.size();
                    double var = 0.0;
                    for (double v : means) var += (v - mu) * (v - mu);
                    var /= (means.size() - 1);
                    e = std::sqrt(var / means.size());
                }
                write_value(out, e);
            }
        }
        out << "\n";
    }
}

static void write_phase_profile(const Params& p, Global& g) {
    std::ofstream out(p.phase_output);
    if (!out)
        throw std::runtime_error("no se puede abrir " + p.phase_output.string());
    write_header(out, p, g, "stress_profile (resuelto en fase)");
    out << "# fase y n n_frames phi vx vy sxx sxy syx syy snxx snxy snyx snyy "
           "kxx kxy kyy\n";
    out << "# fase: centro del bin de fase (rad). Bloques separados por una "
           "línea en blanco (gnuplot).\n";
    out << std::scientific << std::setprecision(6);
    const double a_bin = 2.0 * p.half_width * p.dy;
    for (int ph = 0; ph < p.phase_bins; ++ph) {
        const double phase = (ph + 0.5) * 2.0 * kPi / p.phase_bins;
        const size_t nf = g.frames_per_phase[ph];
        for (int ib = 0; ib < p.n_bins; ++ib) {
            const Acc& a = g.acc[ph][ib];
            const bool ok = a.A > 0.0;
            double f[3];
            a.fluct(f);
            out << phase << "  " << p.y_min + (ib + 0.5) * p.dy << "  " << a.n
                << "  " << nf;
            write_value(out, nf ? a.A / (a_bin * nf) : kNaN);
            write_value(out, ok ? a.mv[0] / a.m : kNaN);
            write_value(out, ok ? a.mv[1] / a.m : kNaN);
            for (int k = 0; k < 4; ++k) write_value(out, ok ? a.As[k] / a.A : kNaN);
            for (int k = 0; k < 4; ++k) write_value(out, ok ? a.Asn[k] / a.A : kNaN);
            for (int k = 0; k < 3; ++k) write_value(out, ok ? -f[k] / a.A : kNaN);
            out << "\n";
        }
        out << "\n";
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    Params p;
    try {
        p = parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error en argumentos: " << e.what() << "\n";
        return 1;
    }

    std::vector<int> ids;
    try {
        ids = dem::list_frames_with_prefix(p.dir, p.pre, ".sxy");
    } catch (const std::exception& e) {
        std::cerr << "Error listando frames: " << e.what() << "\n";
        return 1;
    }
    if (ids.empty()) {
        std::cerr << "No hay archivos " << p.pre << "_*.sxy en " << p.dir << "\n";
        return 1;
    }
    std::cout << "Archivos .sxy: " << ids.size() << "  threads: " << p.n_threads
              << "  bines en y: " << p.n_bins << "\n";

    Global g;
    g.acc.assign(p.phase_bins, std::vector<Acc>(p.n_bins));
    g.frames_per_phase.assign(p.phase_bins, 0);
    {
        ThreadPool pool(p.n_threads);
        std::vector<std::future<void>> futures;
        futures.reserve(ids.size());
        for (int fid : ids) {
            futures.push_back(pool.enqueue(
                FrameTask{dem::frame_path(p.dir, p.pre, fid, ".sxy"), &p, &g}));
        }
        for (auto& f : futures) f.get();
    }

    if (!g.errors.empty()) {
        std::cerr << "Error leyendo " << g.errors.size() << " archivos; el "
                  << "primero: " << g.errors.front() << "\n";
        return 1;
    }
    if (g.n_frames == 0) {
        std::cerr << "Ningún frame en el rango de tiempo pedido.\n";
        return 1;
    }
    if (g.git.size() > 1 || g.params_hash.size() > 1) {
        std::cerr << "Error: los archivos provienen de versiones del simulador "
                     "o de archivos de parámetros distintos.\n";
        return 1;
    }
    if (g.max_phase_err > 1e-4) {
        std::cerr << "Error: la fase de las cabeceras no coincide con --freq "
                  << p.freq << " (diferencia máxima " << g.max_phase_err
                  << " rad).\n";
        return 1;
    }
    std::cout << "Frames usados: " << g.n_frames << "  t = [" << g.t_first
              << ", " << g.t_last << "]  períodos (bloques): " << g.blocks.size()
              << "\n";
    try {
        write_profile(p, g);
        std::cout << "Perfil guardado en " << p.output << "\n";
        if (!p.phase_output.empty()) {
            write_phase_profile(p, g);
            std::cout << "Perfil por fase guardado en " << p.phase_output << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
