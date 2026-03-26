/*! \file force_profile.cpp
 * \brief Calcula el perfil de fuerza (normal o tangencial) promediado
 *        sobre todos los frames de contactos de un caso DEM.
 *
 * Uso:
 *   force_profile <dir> <case_id> <x_m> <n_bins> <y_min> <y_max> <salida>
 *                 [--qty norm tan]
 *                 [--threads N]
 *
 * Argumentos posicionales:
 *   dir      Directorio con los archivos fc_frm_*.dat
 *   case_id  Entero que identifica el caso de simulación
 *   x_m      Semi-ancho de la franja en x (incluye |x| <= x_m) para los puntos de contacto
 *   n_bins   Número de bins en y
 *   y_min    Límite inferior del rango en y
 *   y_max    Límite superior del rango en y
 *   salida   Nombre del archivo de salida
 *
 * Opciones:
 *   --qty <nombre...>  Cantidades a promediar (default: norm)
 *                      Opciones: norm tan
 *   --threads N        Número de worker threads (default: hardware_concurrency)
 *
 * Formato de salida (numpy.loadtxt compatible):
 *   # force_profile  case=<id>  x_m=<val>  frames=<N>
 *   # y_center  <qty1>  <qty2> ...
 *   <y>  <val1>  <val2> ...
 *
 * \author Manuel Carlevaro
 * \date 2026-03-04
 */

#include "dem_reader.hpp"
#include "thread_pool.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ============================================================================
// Cantidades disponibles
// ============================================================================

enum class Qty { NORM, TAN };

static const std::vector<std::pair<std::string, Qty>> QTY_MAP = {
    {"norm",    Qty::NORM},
    {"tan",     Qty::TAN},
};

static std::string qty_name(Qty q)
{
    for (const auto& [name, val] : QTY_MAP)
        if (val == q) return name;
    return "?";
}

static double extract_qty(const dem::Contact& c, Qty q)
{
    switch (q) {
    case Qty::NORM:  return c.norm;
    case Qty::TAN:   return std::abs(c.tan);
    }
    return 0.0;
}

// ============================================================================
// Estructura de acumulación por bin
// ============================================================================

struct BinAccum {
    std::vector<double> sum;   ///< Suma de la cantidad por bin
    std::vector<size_t> count; ///< Número de contactos acumulados por bin
    size_t n_bins;

    explicit BinAccum(size_t nb) : sum(nb, 0.0), count(nb, 0), n_bins(nb) {}

    /// Acumula en el bin correspondiente
    void add(size_t bin, double val)
    {
        sum[bin]   += val;
        count[bin] += 1;
    }

    /// Merge de otro acumulador en éste (para combinación thread-safe)
    void merge(const BinAccum& other)
    {
        for (size_t i = 0; i < n_bins; ++i) {
            sum[i]   += other.sum[i];
            count[i] += other.count[i];
        }
    }
};

// Acumulador global: una BinAccum por cantidad
struct GlobalAccum {
    std::vector<BinAccum> bins; ///< bins[q_idx] acumula la cantidad q_idx
    std::mutex            mtx;
    std::atomic<size_t>   frame_count{0};

    GlobalAccum(size_t n_qtys, size_t n_bins)
    {
        bins.reserve(n_qtys);
        for (size_t i = 0; i < n_qtys; ++i)
            bins.emplace_back(n_bins);
    }

    void merge(const std::vector<BinAccum>& local)
    {
        std::lock_guard lock(mtx);
        for (size_t qi = 0; qi < bins.size(); ++qi)
            bins[qi].merge(local[qi]);
    }
};

// ============================================================================
// Tarea por frame
// ============================================================================

struct FrameTask {
    fs::path              path;
    int                   case_id;
    int                   frame_id;
    double                x_m;
    double                y_min;
    double                bin_width;
    size_t                n_bins;
    std::vector<Qty>      qtys;
    GlobalAccum*          accum;

    void operator()() const
    {
        // Leer frame
        dem::FCFrame frame;
        try {
            frame = dem::read_fc(path);
            frame.case_id  = case_id;
            frame.frame_id = frame_id;
        } catch (const std::exception& e) {
            std::cerr << "[WARN] " << path.filename() << ": " << e.what() << "\n";
            return;
        }

        // Acumulador local (sin locks)
        std::vector<BinAccum> local;
        local.reserve(qtys.size());
        for (size_t qi = 0; qi < qtys.size(); ++qi)
            local.emplace_back(n_bins);

        for (const auto& c : frame.contacts) {
            // Filtro en x del punto de contacto
            if (std::abs(c.cp_x) > x_m) continue;

            // Calcular bin en y del punto de contacto
            double rel = c.cp_y - y_min;
            if (rel < 0.0) continue;
            auto ibin = static_cast<size_t>(rel / bin_width);
            if (ibin >= n_bins) continue;

            // Acumular cada cantidad
            for (size_t qi = 0; qi < qtys.size(); ++qi)
                local[qi].add(ibin, extract_qty(c, qtys[qi]));
        }

        // Merge al acumulador global
        accum->merge(local);
        accum->frame_count.fetch_add(1, std::memory_order_relaxed);
    }
};

// ============================================================================
// Ayuda y argumentos
// ============================================================================

static void print_usage(const char* prog)
{
    std::cout <<
        "Uso:\n"
        "  " << prog << " <dir> <case_id> <x_m> <n_bins> <y_min> <y_max> <salida>\n"
        "               [--qty norm tan] [--threads N]\n\n"
        "Argumentos posicionales:\n"
        "  dir      Directorio con archivos fc_frm_*.dat\n"
        "  case_id  Caso de simulación\n"
        "  x_m      Semi-ancho de la franja en x para los contactos  (|cp_x| <= x_m)\n"
        "  n_bins   Número de bins en y\n"
        "  y_min    Límite inferior del rango en y\n"
        "  y_max    Límite superior del rango en y\n"
        "  salida   Archivo de salida\n\n"
        "Opciones:\n"
        "  --qty <qty...>  Cantidades: norm tan  (default: norm)\n"
        "  --threads N     Número de worker threads (default: hardware_concurrency)\n\n"
        "Salida (numpy.loadtxt compatible):\n"
        "  # y_center  <qty1>  <qty2> ...\n"
        "  y1  val1  val2 ...\n";
}

struct Args {
    fs::path         dir;
    int              case_id   = -1;
    double           x_m       = 0.0;
    size_t           n_bins    = 50;
    double           y_min     = 0.0;
    double           y_max     = 1.0;
    fs::path         output;
    std::vector<Qty> qtys      = {Qty::NORM};
    size_t           n_threads = std::thread::hardware_concurrency();
};

static Args parse_args(int argc, char* argv[])
{
    if (argc < 8) {
        print_usage(argv[0]);
        std::exit(1);
    }

    Args a;
    a.dir     = argv[1];
    a.case_id = std::stoi(argv[2]);
    a.x_m     = std::stod(argv[3]);
    a.n_bins  = static_cast<size_t>(std::stoul(argv[4]));
    a.y_min   = std::stod(argv[5]);
    a.y_max   = std::stod(argv[6]);
    a.output  = argv[7];

    bool has_qty = false;
    for (int i = 8; i < argc; ++i) {
        std::string tok(argv[i]);
        if (tok == "--threads") {
            if (++i >= argc) { std::cerr << "--threads requiere un argumento\n"; std::exit(1); }
            a.n_threads = static_cast<size_t>(std::stoul(argv[i]));
        } else if (tok == "--qty") {
            // Consume todos los tokens siguientes que estén en QTY_MAP
            while (i + 1 < argc) {
                std::string next(argv[i + 1]);
                if (next.starts_with("--")) break;
                bool found = false;
                for (const auto& [name, val] : QTY_MAP) {
                    if (name == next) {
                        if (!has_qty) { a.qtys.clear(); has_qty = true; }
                        // Evitar duplicados
                        if (std::find(a.qtys.begin(), a.qtys.end(), val)
                                == a.qtys.end())
                            a.qtys.push_back(val);
                        found = true;
                        break;
                    }
                }
                if (!found) break;
                ++i;
            }
        } else {
            std::cerr << "Argumento desconocido: " << tok << "\n";
            print_usage(argv[0]);
            std::exit(1);
        }
    }

    // Validaciones
    if (a.y_max <= a.y_min)
        throw std::invalid_argument("y_max debe ser mayor que y_min");
    if (a.n_bins == 0)
        throw std::invalid_argument("n_bins debe ser >= 1");
    if (a.x_m <= 0.0)
        throw std::invalid_argument("x_m debe ser positivo");
    if (a.qtys.empty())
        a.qtys.push_back(Qty::NORM);
    if (a.n_threads == 0) a.n_threads = 1;

    return a;
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[])
{
    Args a;
    try {
        a = parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error en argumentos: " << e.what() << "\n";
        return 1;
    }

    // Listar frames disponibles
    std::vector<int> frame_ids;
    try {
        frame_ids = dem::list_frames(a.dir, a.case_id, ".dat");
    } catch (const std::exception& e) {
        std::cerr << "Error listando frames: " << e.what() << "\n";
        return 1;
    }

    if (frame_ids.empty()) {
        std::cerr << "No se encontraron archivos .dat para el caso "
                  << a.case_id << " en " << a.dir << "\n";
        return 1;
    }

    int n_total = static_cast<int>(frame_ids.size());
    std::cout << "Caso " << a.case_id << ": " << n_total
              << " frames encontrados.\n";
    std::cout << "Threads: " << a.n_threads << "\n";
    std::cout << "Cantidades: ";
    for (const auto& q : a.qtys) std::cout << qty_name(q) << " ";
    std::cout << "\n";
    std::cout << "Franja x: [-" << a.x_m << ", +" << a.x_m << "]\n";
    std::cout << "Rango y : [" << a.y_min << ", " << a.y_max
              << "]  n_bins=" << a.n_bins << "\n";

    // Acumulador global
    double bin_width = (a.y_max - a.y_min) / static_cast<double>(a.n_bins);
    GlobalAccum accum(a.qtys.size(), a.n_bins);

    // Thread pool + dispatch
    {
        ThreadPool pool(a.n_threads);
        std::vector<std::future<void>> futures;
        futures.reserve(static_cast<size_t>(n_total));

        for (int fid : frame_ids) {
            FrameTask task{
                dem::fc_path(a.dir, a.case_id, fid),
                a.case_id, fid,
                a.x_m, a.y_min, bin_width, a.n_bins,
                a.qtys,
                &accum
            };
            futures.push_back(pool.enqueue(std::move(task)));
        }

        // Esperar a que todos los tasks terminen
        for (auto& f : futures) f.get();
    }

    size_t frames_ok = accum.frame_count.load();
    std::cout << "Procesados: " << frames_ok << " frames.\n";

    // -----------------------------------------------------------------------
    // Escribir archivo de salida
    // -----------------------------------------------------------------------
    std::ofstream fout(a.output);
    if (!fout.is_open()) {
        std::cerr << "No se puede abrir el archivo de salida: " << a.output << "\n";
        return 1;
    }

    // Metadatos como comentarios (numpy los ignora con comments='#')
    fout << "# force_profile"
         << "  case=" << a.case_id
         << "  x_m=" << a.x_m
         << "  frames=" << frames_ok
         << "  y_min=" << a.y_min
         << "  y_max=" << a.y_max
         << "  n_bins=" << a.n_bins
         << "\n";

    // Cabecera de columnas
    fout << "# y_center";
    for (const auto& q : a.qtys) fout << "  " << qty_name(q);
    fout << "\n";

    // Datos
    fout << std::scientific << std::setprecision(8);
    for (size_t ib = 0; ib < a.n_bins; ++ib) {
        double y_center = a.y_min + (static_cast<double>(ib) + 0.5) * bin_width;
        fout << y_center;
        for (size_t qi = 0; qi < a.qtys.size(); ++qi) {
            size_t cnt = accum.bins[qi].count[ib];
            double mean = (cnt > 0)
                          ? accum.bins[qi].sum[ib] / static_cast<double>(cnt)
                          : 0.0;
            fout << "  " << mean;
        }
        fout << "\n";
    }

    fout.close();
    std::cout << "Perfil guardado en: " << a.output << "\n";
    return 0;
}
