/*! \file read_demo.cpp
 * \brief Demo de lectura de los tres tipos de archivos DEM.
 *
 * Uso:
 *   ./read_demo <directorio> <case_id> <frame_id>
 *
 * Ejemplo:
 *   ./read_demo ../../test/frames_walker 10 2
 *
 * \author Manuel Carlevaro
 * \date 2026-02-26
 */

#include "dem_reader.hpp"

#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

// Imprime un separador visual
static void sep(const std::string& title)
{
    std::cout << "\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  " << title << "\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
}

// ============================================================================
int main(int argc, char* argv[])
{
    if (argc != 4) {
        std::cerr << "Uso: " << argv[0]
                  << " <directorio> <case_id> <frame_id>\n";
        std::cerr << "Ej:  " << argv[0]
                  << " ../../test/frames_walker 10 2\n";
        return 1;
    }

    fs::path dir(argv[1]);
    int case_id  = std::stoi(argv[2]);
    int frame_id = std::stoi(argv[3]);

    // -----------------------------------------------------------------------
    // 1. Frames disponibles (para el caso dado, extensión .xy)
    // -----------------------------------------------------------------------
    sep("Frames disponibles (.xy)");
    try {
        auto ids = dem::list_frames(dir, case_id, ".xy");
        std::cout << "  Caso " << case_id << ": "
                  << ids.size() << " frames encontrados.\n";
        if (!ids.empty()) {
            std::cout << "  Primeros 5 frame IDs: ";
            for (size_t i = 0; i < std::min(ids.size(), size_t{5}); ++i)
                std::cout << ids[i] << "  ";
            std::cout << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "  ERROR: " << e.what() << "\n";
    }

    // -----------------------------------------------------------------------
    // 2. Lectura de archivo .xy
    // -----------------------------------------------------------------------
    sep("XYFrame  (posiciones)");
    try {
        auto xf = dem::read_xy(dir, case_id, frame_id);
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "  Archivo  : " << dem::xy_path(dir, case_id, frame_id) << "\n";
        std::cout << "  time     : " << xf.time   << "\n";
        std::cout << "  r_out    : " << xf.r_out  << "\n";
        std::cout << "  Círculos : " << xf.circles.size()  << "\n";
        std::cout << "  Polígonos: " << xf.polygons.size() << "\n";
        std::cout << "  Paredes  : " << xf.walls.size()    << "\n";

        if (!xf.circles.empty()) {
            const auto& c0 = xf.circles[0];
            std::cout << "  --- Primer círculo ---\n";
            std::cout << "    gID=" << c0.gID << "  tipo=" << c0.tipo
                      << "  x=" << c0.x << "  y=" << c0.y
                      << "  r=" << c0.radius << "\n";
        }
        if (!xf.polygons.empty()) {
            const auto& p0 = xf.polygons[0];
            std::cout << "  --- Primer polígono ---\n";
            std::cout << "    gID=" << p0.gID << "  n_sides=" << p0.n_sides
                      << "  tipo=" << p0.tipo << "\n";
            std::cout << "    Vértices: ";
            for (const auto& [vx, vy] : p0.vertices)
                std::cout << "(" << vx << "," << vy << ") ";
            std::cout << "\n";
        }
        if (!xf.walls.empty()) {
            const auto& w0 = xf.walls[0];
            std::cout << "  --- Primera pared ---\n";
            std::cout << "    gID=" << w0.gID
                      << "  (" << w0.x1 << "," << w0.y1 << ")"
                      << " -> (" << w0.x2 << "," << w0.y2 << ")"
                      << "  label=" << w0.label << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "  ERROR: " << e.what() << "\n";
    }

    // -----------------------------------------------------------------------
    // 3. Lectura de archivo .ve
    // -----------------------------------------------------------------------
    sep("VEFrame  (velocidades y energías)");
    try {
        auto vf = dem::read_ve(dir, case_id, frame_id);
        std::cout << "  Archivo    : " << dem::ve_path(dir, case_id, frame_id) << "\n";
        std::cout << "  time       : " << vf.time << "\n";
        std::cout << "  Partículas : " << vf.particles.size() << "\n";

        if (!vf.particles.empty()) {
            const auto& p0 = vf.particles[0];
            std::cout << "  --- Primera partícula ---\n";
            std::cout << "    gID=" << p0.gID << "  tipo=" << p0.tipo << "\n"
                      << "    pos=(" << p0.x << "," << p0.y << ")\n"
                      << "    vel=(" << p0.vx << "," << p0.vy << ")  w=" << p0.w << "\n"
                      << "    Eklin=" << p0.E_kin_lin
                      << "  Ekrot=" << p0.E_kin_rot << "\n";
        }

        // Estadísticas básicas de velocidad
        if (vf.particles.size() > 1) {
            double sum_v2 = 0.0, sum_Eklin = 0.0, sum_Ekrot = 0.0;
            for (const auto& p : vf.particles) {
                sum_v2    += p.vx * p.vx + p.vy * p.vy;
                sum_Eklin += p.E_kin_lin;
                sum_Ekrot += p.E_kin_rot;
            }
            size_t n = vf.particles.size();
            std::cout << "  --- Estadísticas ---\n";
            std::cout << "    <v^2>     = " << sum_v2 / n     << "\n";
            std::cout << "    ΣEklin    = " << sum_Eklin        << "\n";
            std::cout << "    ΣEkrot    = " << sum_Ekrot        << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "  ERROR: " << e.what() << "\n";
    }

    // -----------------------------------------------------------------------
    // 4. Lectura de archivo fc_frm*.dat
    // -----------------------------------------------------------------------
    sep("FCFrame  (fuerzas de contacto)");
    try {
        auto ff = dem::read_fc(dir, case_id, frame_id);
        std::cout << "  Archivo   : " << dem::fc_path(dir, case_id, frame_id) << "\n";
        std::cout << "  time      : " << ff.time << "\n";
        std::cout << "  Contactos : " << ff.contacts.size() << "\n";

        for (size_t i = 0; i < ff.contacts.size(); ++i) {
            const auto& c = ff.contacts[i];
            std::cout << "  [" << i << "] "
                      << "A=" << c.gID_A << "  B=" << c.gID_B
                      << "  cp=(" << c.cp_x << "," << c.cp_y << ")"
                      << "  norm=" << c.norm
                      << "  tan="  << c.tan
                      << "  n_pc=" << c.n_pc << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "  ERROR: " << e.what() << "\n";
    }

    std::cout << "\nDemo completado.\n";
    return 0;
}
