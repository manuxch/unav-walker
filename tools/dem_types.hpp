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

/// Cabecera de procedencia que el simulador (versión >= 3.0) escribe en cada
/// archivo de salida:
/// \verbatim
///   # nStep: <n> n_frame: <k> t: <t> dt: <dt> fase: <w t mod 2pi> ...
///   # git: <hash> params: <archivo> params_hash: <hash>
/// \endverbatim
struct Provenance {
    bool        present     = false; ///< true si el archivo tiene la cabecera
    long        n_step      = -1;    ///< Paso de simulación
    long        n_frame     = -1;    ///< Número de frame
    double      t           = 0.0;   ///< Tiempo de simulación
    double      dt          = 0.0;   ///< Paso temporal
    double      phase       = 0.0;   ///< Fase de la excitación, w t mod 2 pi
    std::string git;                 ///< Versión del simulador (hash de git)
    std::string params;              ///< Archivo de parámetros
    std::string params_hash;         ///< Hash del archivo de parámetros
};

/// Un punto de contacto en un archivo fc_*.dat (formato del simulador >= 3.0).
///
/// Convención: la normal (nx, ny) apunta de A a B y la tangente es
/// (ny, -nx). La fuerza sobre B es norm (nx, ny) + tan (ny, -nx); sobre A,
/// la opuesta.
struct Contact {
    int    gID_A;  ///< ID del cuerpo A (< 0: pared)
    int    gID_B;  ///< ID del cuerpo B (< 0: pared)
    double cp_x;   ///< Coordenada x del punto de contacto
    double cp_y;   ///< Coordenada y del punto de contacto
    double norm;   ///< Componente normal Fn de la fuerza sobre B
    double tan;    ///< Componente tangencial Ft de la fuerza sobre B (con signo)
    double nx;     ///< Normal unitaria, de A hacia B (x)
    double ny;     ///< Normal unitaria, de A hacia B (y)
    double xA, yA; ///< Centro de A (punto de contacto si A es pared)
    double xB, yB; ///< Centro de B (punto de contacto si B es pared)
    int    n_pc;   ///< Número de puntos del manifold del contacto
    bool   wall;   ///< true para contactos grano-pared (tipo GW)
};

/// Contenido completo de un archivo fc_frm_aa_nn.dat.
struct FCFrame {
    double time;      ///< Tiempo de simulación del frame
    int    case_id;   ///< Identificador del caso (aa)
    int    frame_id;  ///< Identificador del frame (nn)
    Provenance prov;  ///< Cabecera de procedencia

    std::vector<Contact> contacts;
};

// ============================================================================
// Tipos para archivos .sxy (tensor de estrés por grano)
// ============================================================================

/// Tensor de estrés de contacto de un grano y su estado (una línea de .sxy).
///
/// s_ij = (1 / A_grano) sum_c f_i l_j, con f la fuerza de contacto sobre el
/// grano y l = punto de contacto - centro. Índices: [0] xx, [1] xy, [2] yx,
/// [3] yy. Compresión < 0. sn es la parte debida solo a las fuerzas
/// normales; la tangencial es s - sn.
struct GrainStress {
    int    gID;    ///< ID del grano
    double s[4];   ///< Tensor de contacto total (xx, xy, yx, yy)
    double sn[4];  ///< Parte debida a las fuerzas normales
    double x, y;   ///< Centro
    double r;      ///< Radio
    double m;      ///< Masa
    double vx, vy; ///< Velocidad
    double w;      ///< Velocidad angular
    int    z_gg;   ///< Puntos de contacto activos grano-grano
    int    z_gw;   ///< Puntos de contacto activos grano-pared
};

/// Contenido completo de un archivo `<pre>_<frame>.sxy`.
struct SXYFrame {
    double     time = 0.0; ///< Tiempo de simulación (primera línea)
    Provenance prov;       ///< Cabecera de procedencia

    std::vector<GrainStress> grains;
};

} // namespace dem
