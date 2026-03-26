/*! \file dem_reader.cpp
 * \brief Implementación de funciones para leer archivos de simulación DEM.
 *
 * \author Manuel Carlevaro
 * \date 2026-02-26
 */

#include "dem_reader.hpp"

#include <iostream>
#include <charconv>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <algorithm>

namespace dem {

// ============================================================================
// Utilidades internas
// ============================================================================

/// Extrae el frame_id numérico desde el stem de un nombre de archivo con el
/// patrón frm-{case}_{frame} o fc_frm-{case}_{frame}.
/// Devuelve -1 si no puede extraerse.
static int extract_frame_id(const std::string& stem, int case_id)
{
    const std::string prefix_fc = "fc_frm-" + std::to_string(case_id) + "_";
    const std::string prefix_fr = "frm-"    + std::to_string(case_id) + "_";

    std::string id_str;
    if (stem.starts_with(prefix_fc)) {
        id_str = stem.substr(prefix_fc.size());
    } else if (stem.starts_with(prefix_fr)) {
        id_str = stem.substr(prefix_fr.size());
    } else {
        return -1;
    }

    int val = 0;
    auto [ptr, ec] = std::from_chars(id_str.data(),
                                      id_str.data() + id_str.size(), val);
    if (ec != std::errc{}) return -1;
    return val;
}

// ============================================================================
// Utilidades públicas
// ============================================================================

std::string frame_id_to_str(int n)
{
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(6) << n;
    return oss.str();
}

std::filesystem::path xy_path(const std::filesystem::path& dir,
                               int case_id, int frame_id)
{
    return dir / ("frm-" + std::to_string(case_id)
                  + "_" + frame_id_to_str(frame_id) + ".xy");
}

std::filesystem::path ve_path(const std::filesystem::path& dir,
                               int case_id, int frame_id)
{
    return dir / ("frm-" + std::to_string(case_id)
                  + "_" + frame_id_to_str(frame_id) + ".ve");
}

std::filesystem::path fc_path(const std::filesystem::path& dir,
                               int case_id, int frame_id)
{
    return dir / ("fc_frm-" + std::to_string(case_id)
                  + "_" + frame_id_to_str(frame_id) + ".dat");
}

// ============================================================================
// Descubrimiento de frames
// ============================================================================

std::vector<int> list_frames(const std::filesystem::path& dir,
                              int case_id,
                              const std::string& ext)
{
    if (!std::filesystem::is_directory(dir)) {
        throw std::runtime_error("dem::list_frames: directorio no existe: "
                                  + dir.string());
    }

    std::vector<int> ids;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ext) continue;

        int fid = extract_frame_id(entry.path().stem().string(), case_id);
        if (fid >= 0) ids.push_back(fid);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

// ============================================================================
// Lector de archivos .xy  (por ruta completa)
// ============================================================================

XYFrame read_xy(const std::filesystem::path& path)
{
    std::ifstream fin(path);
    if (!fin.is_open()) {
        throw std::runtime_error("dem::read_xy: no se puede abrir: "
                                  + path.string());
    }

    XYFrame frame;
    frame.case_id  = -1;
    frame.frame_id = -1;
    frame.time     = 0.0;
    frame.r_out    = 0.0;

    std::string line;
    bool header_read = false;

    while (std::getline(fin, line)) {
        if (line.empty()) continue;

        // --- Cabecera: "# time: <t> # r_out: <r>" ---
        if (!header_read && line.find("time:") != std::string::npos) {
            std::istringstream iss(line);
            std::string tok;
            while (iss >> tok) {
                if (tok == "time:")  iss >> frame.time;
                if (tok == "r_out:") iss >> frame.r_out;
            }
            header_read = true;
            continue;
        }

        if (line[0] == '#') continue;  // Otros comentarios

        // --- Líneas de datos ---
        std::istringstream iss(line);
        int gid, n_kind;
        if (!(iss >> gid >> n_kind)) continue;

        //  n_kind == 1  -> círculo:   gID 1 x y r tipo
        //  n_kind >= 3  -> polígono:  gID n x1 y1 ... xn yn tipo
        //  n_kind == 2  -> segmento:  gID 2 x1 y1 x2 y2 LABEL

        if (n_kind == 1) {
            CircleParticle cp;
            cp.gID = gid;
            int tipo;
            if (!(iss >> cp.x >> cp.y >> cp.radius >> tipo)) {
                throw std::runtime_error(
                    "dem::read_xy: formato inválido en círculo, archivo: "
                    + path.string());
            }
            cp.tipo = tipo;
            frame.circles.push_back(std::move(cp));

        } else if (n_kind >= 3) {
            PolygonParticle pp;
            pp.gID     = gid;
            pp.n_sides = n_kind;
            pp.vertices.reserve(static_cast<size_t>(n_kind));
            for (int i = 0; i < n_kind; ++i) {
                double vx, vy;
                if (!(iss >> vx >> vy)) {
                    throw std::runtime_error(
                        "dem::read_xy: faltan vértices de polígono, archivo: "
                        + path.string());
                }
                pp.vertices.emplace_back(vx, vy);
            }
            int tipo;
            if (!(iss >> tipo)) {
                throw std::runtime_error(
                    "dem::read_xy: falta tipo de polígono, archivo: "
                    + path.string());
            }
            pp.tipo = tipo;
            frame.polygons.push_back(std::move(pp));

        } else if (n_kind == 2) {
            WallSegment ws;
            ws.gID = gid;
            std::string label;
            if (!(iss >> ws.x1 >> ws.y1 >> ws.x2 >> ws.y2 >> label)) {
                throw std::runtime_error(
                    "dem::read_xy: formato inválido en segmento de pared, archivo: "
                    + path.string());
            }
            ws.label = std::move(label);
            frame.walls.push_back(std::move(ws));
        }
        // Otros valores de n_kind son ignorados silenciosamente
    }

    return frame;
}

// Variante conveniente: construye la ruta y llama a la anterior
XYFrame read_xy(const std::filesystem::path& dir, int case_id, int frame_id)
{
    XYFrame frame = read_xy(xy_path(dir, case_id, frame_id));
    frame.case_id  = case_id;
    frame.frame_id = frame_id;
    return frame;
}

// ============================================================================
// Lector de archivos .ve  (por ruta completa)
// ============================================================================

VEFrame read_ve(const std::filesystem::path& path)
{
    std::ifstream fin(path);
    if (!fin.is_open()) {
        throw std::runtime_error("dem::read_ve: no se puede abrir: "
                                  + path.string());
    }

    VEFrame frame;
    frame.case_id  = -1;
    frame.frame_id = -1;
    frame.time     = 0.0;

    std::string line;
    bool header_read = false;

    while (std::getline(fin, line)) {
        if (line.empty()) continue;

        // Cabecera: " ## sim_time: <t>"
        if (!header_read && line.find("sim_time:") != std::string::npos) {
            std::istringstream iss(line);
            std::string tok;
            while (iss >> tok) {
                if (tok == "sim_time:") iss >> frame.time;
            }
            header_read = true;
            continue;
        }

        if (line[0] == '#') continue;   // Cabecera de columnas u otros comentarios

        // Datos: gID tipo x y vx vy w E_kin_lin E_kin_rot
        std::istringstream iss(line);
        VelocityEntry ve;
        if (!(iss >> ve.gID    >> ve.tipo
                  >> ve.x      >> ve.y
                  >> ve.vx     >> ve.vy
                  >> ve.w
                  >> ve.E_kin_lin >> ve.E_kin_rot)) {
            continue;  // Línea incompleta — ignorar
        }
        frame.particles.push_back(std::move(ve));
    }

    return frame;
}

VEFrame read_ve(const std::filesystem::path& dir, int case_id, int frame_id)
{
    VEFrame frame = read_ve(ve_path(dir, case_id, frame_id));
    frame.case_id  = case_id;
    frame.frame_id = frame_id;
    return frame;
}

// ============================================================================
// Lector de archivos fc_frm*.dat  (por ruta completa)
// ============================================================================

FCFrame read_fc(const std::filesystem::path& path)
{
    std::ifstream fin(path);
    if (!fin.is_open()) {
        throw std::runtime_error("dem::read_fc: no se puede abrir: "
                                  + path.string());
    }

    FCFrame frame;
    frame.case_id  = -1;
    frame.frame_id = -1;
    frame.time     = 0.0;

    std::string line;
    bool header_read = false;

    while (std::getline(fin, line)) {
        if (line.empty()) continue;

        // Cabecera: "# Time:  <t>"
        if (!header_read && line.find("Time:") != std::string::npos) {
            std::istringstream iss(line);
            std::string tok;
            while (iss >> tok) {
                if (tok == "Time:") iss >> frame.time;
            }
            header_read = true;
            continue;
        }

        if (line[0] == '#') continue;  // Cabecera de columnas

        // Datos: gID_A gID_B cp.x cp.y norm tan CPn
        std::istringstream iss(line);
        Contact c;
        std::string n_pc_str;

        if (!(iss >> c.gID_A >> c.gID_B
                  >> c.cp_x >> c.cp_y
                  >> c.norm >> c.tan
                  >> n_pc_str)) {
            continue;
        }

        // Extraer número de CP: "CP1" -> 1, "CP2" -> 2
        c.n_pc = 0;
        if (n_pc_str.size() > 2 && n_pc_str.starts_with("CP")) {
            auto [ptr, ec] = std::from_chars(
                n_pc_str.data() + 2,
                n_pc_str.data() + n_pc_str.size(),
                c.n_pc);
            if (ec != std::errc{}) c.n_pc = 0;
        }

        frame.contacts.push_back(std::move(c));
    }

    return frame;
}

FCFrame read_fc(const std::filesystem::path& dir, int case_id, int frame_id)
{
    FCFrame frame = read_fc(fc_path(dir, case_id, frame_id));
    frame.case_id  = case_id;
    frame.frame_id = frame_id;
    return frame;
}

} // namespace dem
