/*! \file force_map2d.cpp
 * \brief Calcula el mapa de calor bidimensional de fuerza (normal o tangencial)
 *        promediado sobre todos los frames de contactos de un caso DEM.
 *
 * \author Antigravity
 * \date 2026-04-05
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
#include <stdexcept>
#include <string>
#include <vector>
#include <limits>

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
// Acumulador de límites espaciales
// ============================================================================
struct BoundsAccum {
    std::mutex mtx;
    double x_min = std::numeric_limits<double>::max();
    double x_max = std::numeric_limits<double>::lowest();
    double y_min = std::numeric_limits<double>::max();
    double y_max = std::numeric_limits<double>::lowest();

    void merge(double lx_min, double lx_max, double ly_min, double ly_max) {
        std::lock_guard lock(mtx);
        if (lx_min < x_min) x_min = lx_min;
        if (lx_max > x_max) x_max = lx_max;
        if (ly_min < y_min) y_min = ly_min;
        if (ly_max > y_max) y_max = ly_max;
    }
};

struct BoundsTask {
    fs::path path;
    BoundsAccum* global_bounds;

    void operator()() const {
        dem::FCFrame frame;
        try {
            frame = dem::read_fc(path);
        } catch (...) { return; }

        if (frame.contacts.empty()) return;

        double lx_min = std::numeric_limits<double>::max();
        double lx_max = std::numeric_limits<double>::lowest();
        double ly_min = std::numeric_limits<double>::max();
        double ly_max = std::numeric_limits<double>::lowest();

        for (const auto& c : frame.contacts) {
            if (c.cp_x < lx_min) lx_min = c.cp_x;
            if (c.cp_x > lx_max) lx_max = c.cp_x;
            if (c.cp_y < ly_min) ly_min = c.cp_y;
            if (c.cp_y > ly_max) ly_max = c.cp_y;
        }
        global_bounds->merge(lx_min, lx_max, ly_min, ly_max);
    }
};

// ============================================================================
// Estructura de acumulación por bin (ahora 2D)
// ============================================================================

struct BinAccum {
    std::vector<double> sum;   
    std::vector<size_t> count; 
    size_t n_bins_x;
    size_t n_bins_y;

    explicit BinAccum(size_t nb_x, size_t nb_y) : 
        sum(nb_x * nb_y, 0.0), count(nb_x * nb_y, 0), 
        n_bins_x(nb_x), n_bins_y(nb_y) {}

    void add(size_t bin_x, size_t bin_y, double val)
    {
        size_t idx = bin_y * n_bins_x + bin_x;
        sum[idx]   += val;
        count[idx] += 1;
    }

    void merge(const BinAccum& other)
    {
        for (size_t i = 0; i < sum.size(); ++i) {
            sum[i]   += other.sum[i];
            count[i] += other.count[i];
        }
    }
};

struct GlobalAccum {
    std::vector<BinAccum> bins; 
    std::mutex            mtx;
    std::atomic<size_t>   frame_count{0};

    GlobalAccum(size_t n_qtys, size_t nb_x, size_t nb_y)
    {
        bins.reserve(n_qtys);
        for (size_t i = 0; i < n_qtys; ++i)
            bins.emplace_back(nb_x, nb_y);
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
    double                x_min;
    double                x_max;
    double                y_min;
    double                y_max;
    size_t                n_bins_x;
    size_t                n_bins_y;
    double                bin_width_x;
    double                bin_width_y;
    std::vector<Qty>      qtys;
    GlobalAccum*          accum;

    void operator()() const
    {
        dem::FCFrame frame;
        try {
            frame = dem::read_fc(path);
            frame.case_id  = case_id;
            frame.frame_id = frame_id;
        } catch (const std::exception& e) {
            std::cerr << "[WARN] " << path.filename() << ": " << e.what() << "\n";
            return;
        }

        std::vector<BinAccum> local;
        local.reserve(qtys.size());
        for (size_t qi = 0; qi < qtys.size(); ++qi)
            local.emplace_back(n_bins_x, n_bins_y);

        for (const auto& c : frame.contacts) {
            if (c.cp_x < x_min || c.cp_x > x_max) continue;
            if (c.cp_y < y_min || c.cp_y > y_max) continue;

            double rel_x = c.cp_x - x_min;
            double rel_y = c.cp_y - y_min;

            auto ibin_x = static_cast<size_t>(rel_x / bin_width_x);
            auto ibin_y = static_cast<size_t>(rel_y / bin_width_y);
            
            // Límite en caso que cp_x == x_max
            if (ibin_x == n_bins_x) ibin_x = n_bins_x - 1;
            if (ibin_y == n_bins_y) ibin_y = n_bins_y - 1;

            if (ibin_x >= n_bins_x || ibin_y >= n_bins_y) continue;

            for (size_t qi = 0; qi < qtys.size(); ++qi)
                local[qi].add(ibin_x, ibin_y, extract_qty(c, qtys[qi]));
        }

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
        "  " << prog << " <dir> <case_id> <n_bins_x> <n_bins_y> <salida>\n"
        "               [--xmin val] [--xmax val] [--ymin val] [--ymax val]\n"
        "               [--qty norm tan] [--threads N]\n\n"
        "Argumentos posicionales:\n"
        "  dir        Directorio con archivos fc_frm_*.dat\n"
        "  case_id    Caso de simulación\n"
        "  n_bins_x   Número de bins en x\n"
        "  n_bins_y   Número de bins en y\n"
        "  salida     Archivo de salida (.dat)\n\n"
        "Opciones:\n"
        "  --xmin, --xmax, --ymin, --ymax:\n"
        "             Límites espaciales. Si no se proveen, se calcularán\n"
        "             automáticamente revisando los frames.\n"
        "  --qty <qty...>  Cantidades: norm tan  (default: norm tan)\n"
        "  --threads N     Número de worker threads (default: hardware_concurrency)\n";
}

struct Args {
    fs::path         dir;
    int              case_id   = -1;
    size_t           n_bins_x  = 50;
    size_t           n_bins_y  = 50;
    fs::path         output;

    double           x_min     = std::numeric_limits<double>::quiet_NaN();
    double           x_max     = std::numeric_limits<double>::quiet_NaN();
    double           y_min     = std::numeric_limits<double>::quiet_NaN();
    double           y_max     = std::numeric_limits<double>::quiet_NaN();

    std::vector<Qty> qtys      = {};
    size_t           n_threads = std::thread::hardware_concurrency();
};

static Args parse_args(int argc, char* argv[])
{
    if (argc < 6) {
        print_usage(argv[0]);
        std::exit(1);
    }

    Args a;
    a.dir      = argv[1];
    a.case_id  = std::stoi(argv[2]);
    a.n_bins_x = static_cast<size_t>(std::stoul(argv[3]));
    a.n_bins_y = static_cast<size_t>(std::stoul(argv[4]));
    a.output   = argv[5];

    bool has_qty = false;
    for (int i = 6; i < argc; ++i) {
        std::string tok(argv[i]);
        if (tok == "--threads") {
            if (++i >= argc) { std::cerr << "--threads requiere argumento\n"; std::exit(1); }
            a.n_threads = static_cast<size_t>(std::stoul(argv[i]));
        } else if (tok == "--xmin") {
            if (++i >= argc) throw std::invalid_argument("--xmin requiere argumento");
            a.x_min = std::stod(argv[i]);
        } else if (tok == "--xmax") {
            if (++i >= argc) throw std::invalid_argument("--xmax requiere argumento");
            a.x_max = std::stod(argv[i]);
        } else if (tok == "--ymin") {
            if (++i >= argc) throw std::invalid_argument("--ymin requiere argumento");
            a.y_min = std::stod(argv[i]);
        } else if (tok == "--ymax") {
            if (++i >= argc) throw std::invalid_argument("--ymax requiere argumento");
            a.y_max = std::stod(argv[i]);
        } else if (tok == "--qty") {
            while (i + 1 < argc) {
                std::string next(argv[i + 1]);
                if (next.find("--") == 0) break;
                bool found = false;
                for (const auto& [name, val] : QTY_MAP) {
                    if (name == next) {
                        if (!has_qty) { a.qtys.clear(); has_qty = true; }
                        if (std::find(a.qtys.begin(), a.qtys.end(), val) == a.qtys.end())
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

    if (a.n_bins_x == 0 || a.n_bins_y == 0)
        throw std::invalid_argument("n_bins_x y n_bins_y deben ser >= 1");
    if (a.qtys.empty()) {
        a.qtys.push_back(Qty::NORM);
        a.qtys.push_back(Qty::TAN);
    }
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
    std::cout << "Caso " << a.case_id << ": " << n_total << " frames encontrados.\n";
    std::cout << "Threads: " << a.n_threads << "\n";

    // Paso opcional 1: Calcular límites de simulación si no se proveyeron
    if (std::isnan(a.x_min) || std::isnan(a.x_max) || 
        std::isnan(a.y_min) || std::isnan(a.y_max)) {
        std::cout << "Calculando límites espaciales (pre-pass)\n";
        BoundsAccum bounds;
        {
            ThreadPool pool(a.n_threads);
            std::vector<std::future<void>> futures;
            futures.reserve(n_total);
            for (int fid : frame_ids) {
                BoundsTask task{ dem::fc_path(a.dir, a.case_id, fid), &bounds };
                futures.push_back(pool.enqueue(std::move(task)));
            }
            for (auto& f : futures) f.get();
        }
        if (std::isnan(a.x_min)) a.x_min = bounds.x_min;
        if (std::isnan(a.x_max)) a.x_max = bounds.x_max;
        if (std::isnan(a.y_min)) a.y_min = bounds.y_min;
        if (std::isnan(a.y_max)) a.y_max = bounds.y_max;
        
        // Agregar un infinitesimal de padding para que el límite superior entre en el último bin
        double pad_x = (a.x_max - a.x_min) * 1e-5;
        double pad_y = (a.y_max - a.y_min) * 1e-5;
        if (pad_x == 0) pad_x = 1e-5;
        if (pad_y == 0) pad_y = 1e-5;
        if (bounds.x_max != std::numeric_limits<double>::lowest()) a.x_max += pad_x;
        if (bounds.y_max != std::numeric_limits<double>::lowest()) a.y_max += pad_y;
        if (bounds.x_min != std::numeric_limits<double>::max()) a.x_min -= pad_x;
        if (bounds.y_min != std::numeric_limits<double>::max()) a.y_min -= pad_y;
    }

    std::cout << "Límites espaciales:\n"
              << "  X: [" << a.x_min << ", " << a.x_max << "]  Bins: " << a.n_bins_x << "\n"
              << "  Y: [" << a.y_min << ", " << a.y_max << "]  Bins: " << a.n_bins_y << "\n";

    double bin_width_x = (a.x_max - a.x_min) / a.n_bins_x;
    double bin_width_y = (a.y_max - a.y_min) / a.n_bins_y;
    
    GlobalAccum accum(a.qtys.size(), a.n_bins_x, a.n_bins_y);

    {
        ThreadPool pool(a.n_threads);
        std::vector<std::future<void>> futures;
        futures.reserve(n_total);

        for (int fid : frame_ids) {
            FrameTask task{
                dem::fc_path(a.dir, a.case_id, fid),
                a.case_id, fid,
                a.x_min, a.x_max, a.y_min, a.y_max,
                a.n_bins_x, a.n_bins_y,
                bin_width_x, bin_width_y,
                a.qtys, &accum
            };
            futures.push_back(pool.enqueue(std::move(task)));
        }

        for (auto& f : futures) f.get();
    }

    size_t frames_ok = accum.frame_count.load();
    std::cout << "Procesados: " << frames_ok << " frames.\n";

    // -----------------------------------------------------------------------
    // Escribir archivo
    // -----------------------------------------------------------------------
    std::ofstream fout(a.output);
    if (!fout.is_open()) {
        std::cerr << "No se puede abrir el archivo de salida: " << a.output << "\n";
        return 1;
    }

    fout << "# force_map2d"
         << "  case=" << a.case_id
         << "  frames=" << frames_ok
         << "  x_min=" << a.x_min
         << "  x_max=" << a.x_max
         << "  y_min=" << a.y_min
         << "  y_max=" << a.y_max
         << "  n_bins_x=" << a.n_bins_x
         << "  n_bins_y=" << a.n_bins_y
         << "\n";

    fout << "# x_center  y_center";
    for (const auto& q : a.qtys) fout << "  " << qty_name(q);
    fout << "\n";

    fout << std::scientific << std::setprecision(6);
    
    for (size_t ib_y = 0; ib_y < a.n_bins_y; ++ib_y) {
        double y_center = a.y_min + (static_cast<double>(ib_y) + 0.5) * bin_width_y;
        for (size_t ib_x = 0; ib_x < a.n_bins_x; ++ib_x) {
            double x_center = a.x_min + (static_cast<double>(ib_x) + 0.5) * bin_width_x;
            fout << x_center << " " << y_center;
            
            size_t idx = ib_y * a.n_bins_x + ib_x;
            for (size_t qi = 0; qi < a.qtys.size(); ++qi) {
                size_t cnt = accum.bins[qi].count[idx];
                double mean = (cnt > 0)
                              ? accum.bins[qi].sum[idx] / static_cast<double>(cnt)
                              : std::numeric_limits<double>::quiet_NaN();
                fout << " " << mean; 
            }
            fout << "\n";
        }
        // Blank line between rows is good for gnuplot, but numpy loadtxt ignores it
        fout << "\n";
    }

    fout.close();
    std::cout << "Mapa de fuerza guardado en: " << a.output << "\n";
    return 0;
}
