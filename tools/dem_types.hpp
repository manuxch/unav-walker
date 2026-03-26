/*! \file dem_types.hpp
 * \brief Tipos de datos para la lectura de archivos de simulación DEM.
 *
 * Define las estructuras que representan los datos almacenados en los tres
 * tipos de archivos de salida del simulador:
 *   - frm_aa_nn.xy   : posiciones y geometría de partículas
 *   - frm_aa_nn.ve   : velocidades y energías cinéticas
 *   - fc_frm_aa_nn.dat : fuerzas de contacto
 *
 * \author Manuel Carlevaro
 * \date 2026-02-26
 */

#pragma once

#include <string>
#include <vector>
#include <utility>    // std::pair

namespace dem {

// ============================================================================
// Tipos para archivos .xy (posiciones de partículas)
// ============================================================================

/// Partícula circular (nLados == 1 en la simulación).
struct CircleParticle {
    int    gID;     ///< ID global de la partícula
    int    tipo;    ///< Tipo de partícula (entero definido en la simulación)
    double x;       ///< Coordenada x del centro
    double y;       ///< Coordenada y del centro
    double radius;  ///< Radio
};

/// Partícula poligonal regular (nLados >= 3 en la simulación).
/// Los vértices se almacenan en coordenadas globales del mundo.
struct PolygonParticle {
    int    gID;     ///< ID global de la partícula
    int    tipo;    ///< Tipo de partícula
    int    n_sides; ///< Número de lados / vértices (3, 4, 5, ...)
    /// Vértices en orden del polígono: {(x1,y1), (x2,y2), ..., (xn,yn)}
    std::vector<std::pair<double,double>> vertices;
};

/// Segmento de pared o tapa del silo (gID negativo en la simulación).
/// El tipo de pared se identifica con la etiqueta (LINE, LID-F, LID-W).
struct WallSegment {
    int         gID;    ///< ID del cuerpo de pared (e.g. -100 o -110)
    double      x1;     ///< Extremo A: coordenada x
    double      y1;     ///< Extremo A: coordenada y
    double      x2;     ///< Extremo B: coordenada x
    double      y2;     ///< Extremo B: coordenada y
    std::string label;  ///< Etiqueta: "LINE", "LID-F" o "LID-W"
};

/// Contenido completo de un archivo frm_aa_nn.xy.
struct XYFrame {
    double time;               ///< Tiempo de simulación del frame
    double r_out;              ///< Radio del orificio de salida
    int    case_id;            ///< Identificador del caso (aa)
    int    frame_id;           ///< Identificador del frame (nn)

    std::vector<CircleParticle>  circles;   ///< Partículas circulares
    std::vector<PolygonParticle> polygons;  ///< Partículas poligonales
    std::vector<WallSegment>     walls;     ///< Segmentos de pared/tapa
};

// ============================================================================
// Tipos para archivos .ve (velocidades y energías)
// ============================================================================

/// Datos de velocidad y energía de una partícula en un frame .ve.
struct VelocityEntry {
    int    gID;        ///< ID global de la partícula
    int    tipo;       ///< Tipo de partícula
    double x;          ///< Coordenada x del centro
    double y;          ///< Coordenada y del centro
    double vx;         ///< Componente x de la velocidad lineal
    double vy;         ///< Componente y de la velocidad lineal
    double w;          ///< Velocidad angular
    double E_kin_lin;  ///< Energía cinética lineal = 0.5 * m * |v|^2
    double E_kin_rot;  ///< Energía cinética rotacional = 0.5 * I * w^2
};

/// Contenido completo de un archivo frm_aa_nn.ve.
struct VEFrame {
    double time;      ///< Tiempo de simulación del frame
    int    case_id;   ///< Identificador del caso (aa)
    int    frame_id;  ///< Identificador del frame (nn)

    std::vector<VelocityEntry> particles;
};

// ============================================================================
// Tipos para archivos fc_frm_aa_nn.dat (fuerzas de contacto)
// ============================================================================

/// Un contacto entre dos partículas en un frame fc_frm*.dat.
struct Contact {
    int    gID_A;  ///< ID de la partícula A
    int    gID_B;  ///< ID de la partícula B
    double cp_x;   ///< Coordenada x del punto de contacto
    double cp_y;   ///< Coordenada y del punto de contacto
    double norm;   ///< Módulo de la fuerza normal
    double tan;    ///< Módulo de la fuerza tangencial
    int    n_pc;   ///< Número de puntos de contacto (1 o 2, de CP1/CP2)
};

/// Contenido completo de un archivo fc_frm_aa_nn.dat.
struct FCFrame {
    double time;      ///< Tiempo de simulación del frame
    int    case_id;   ///< Identificador del caso (aa)
    int    frame_id;  ///< Identificador del frame (nn)

    std::vector<Contact> contacts;
};

} // namespace dem
