/*! \file dem_reader.hpp
 * \brief Declaración de funciones para leer archivos de simulación DEM.
 *
 * \verbatim
 * Convenciones de nombres de archivo:
 *   frm-{case_id}_{frame_id}.xy       — posiciones
 *   frm-{case_id}_{frame_id}.ve       — velocidades y energías
 *   fc_frm-{case_id}_{frame_id}.dat   — fuerzas de contacto
 *   frm-{case_id}_{frame_id}.sxy      — tensor de estrés por grano
 * (en general <pre>_{frame_id}.<ext>, con <pre> = preFrameFile)
 *
 * donde frame_id se formatea con 6 dígitos con ceros a la izquierda.
 * \endverbatim
 *
 * \author Manuel Carlevaro
 * \date 2026-02-26
 */

#pragma once

#include "dem_types.hpp"

#include <filesystem>
#include <limits>
#include <string>
#include <vector>

namespace dem {

// ---------------------------------------------------------------------------
// Utilidades de nombre de archivo
// ---------------------------------------------------------------------------

/// Convierte un entero a string de 6 dígitos con ceros a la izquierda.
/// Ejemplo: frame_id_to_str(2) -> "000002"
std::string frame_id_to_str(int n);

/// Construye la ruta de un archivo .xy dado el directorio, caso y frame.
std::filesystem::path xy_path(const std::filesystem::path& dir,
                               int case_id, int frame_id);

/// Construye la ruta de un archivo .ve dado el directorio, caso y frame.
std::filesystem::path ve_path(const std::filesystem::path& dir,
                               int case_id, int frame_id);

/// Construye la ruta de un archivo fc_frm*.dat dado el directorio, caso y frame.
std::filesystem::path fc_path(const std::filesystem::path& dir,
                               int case_id, int frame_id);

// ---------------------------------------------------------------------------
// Descubrimiento de frames disponibles
// ---------------------------------------------------------------------------

/// Devuelve la lista ordenada de frame_ids disponibles para un caso y extensión.
/// \param dir      Directorio donde buscar los archivos.
/// \param case_id  Caso de simulación (aa).
/// \param ext      Extensión: ".xy", ".ve", o ".dat" (para fc_frm*).
/// \throws std::runtime_error si el directorio no existe.
std::vector<int> list_frames(const std::filesystem::path& dir,
                              int case_id,
                              const std::string& ext = ".xy");

// ---------------------------------------------------------------------------
// Lectores por ruta completa
// ---------------------------------------------------------------------------

/// Lee un archivo .xy de posiciones desde su ruta completa.
/// Los campos case_id y frame_id de XYFrame quedan en -1.
/// \throws std::runtime_error si el archivo no puede abrirse o el formato es inválido.
XYFrame read_xy(const std::filesystem::path& path);

/// Lee un archivo .ve de velocidades desde su ruta completa.
/// Los campos case_id y frame_id de VEFrame quedan en -1.
/// \throws std::runtime_error si el archivo no puede abrirse o el formato es inválido.
VEFrame read_ve(const std::filesystem::path& path);

/// Lee un archivo fc_frm*.dat de contactos desde su ruta completa.
/// Solo admite el formato del simulador >= 3.0 (14 columnas, fuerzas =
/// impulso / dt); los archivos anteriores tienen otra escala de fuerza y se
/// rechazan. Los campos case_id y frame_id de FCFrame quedan en -1.
/// \throws std::runtime_error si el archivo no puede abrirse o el formato es inválido.
FCFrame read_fc(const std::filesystem::path& path);

// ---------------------------------------------------------------------------
// Lectores por directorio + caso + frame (forma conveniente)
// ---------------------------------------------------------------------------

/// Lee un archivo .xy dado el directorio, caso y frame ID.
XYFrame read_xy(const std::filesystem::path& dir, int case_id, int frame_id);

/// Lee un archivo .ve dado el directorio, caso y frame ID.
VEFrame read_ve(const std::filesystem::path& dir, int case_id, int frame_id);

/// Lee un archivo fc_frm*.dat dado el directorio, caso y frame ID.
FCFrame read_fc(const std::filesystem::path& dir, int case_id, int frame_id);

// ---------------------------------------------------------------------------
// Archivos por prefijo (simulador >= 3.0)
//
// El simulador nombra los archivos <pre>_<frame>.<ext> (fc_<pre>_<frame>.dat
// para los contactos), con <pre> = preFrameFile del archivo de parámetros.
// ---------------------------------------------------------------------------

/// Ruta de un archivo `<pre>_<frame><ext>`, p. ej.
/// `frame_path(dir, "frm", 12, ".sxy")` -> `dir/frm_000012.sxy`.
std::filesystem::path frame_path(const std::filesystem::path& dir,
                                 const std::string& pre, int frame_id,
                                 const std::string& ext);

/// Frames disponibles para los archivos `<pre>_<frame><ext>`, ordenados.
/// \throws std::runtime_error si el directorio no existe.
std::vector<int> list_frames_with_prefix(const std::filesystem::path& dir,
                                         const std::string& pre,
                                         const std::string& ext);

/// Lee un archivo .sxy (tensor de estrés por grano, simulador >= 3.0).
/// \throws std::runtime_error si no puede abrirse o si el formato no es el
///         del simulador >= 3.0.
SXYFrame read_sxy(const std::filesystem::path& path);

/// Interpreta una línea de la cabecera de procedencia ("# nStep: ..." o
/// "# git: ...") y completa \p prov. Devuelve false si la línea no es de
/// procedencia.
bool parse_provenance_line(const std::string& line, Provenance& prov);

// ---------------------------------------------------------------------------
// Franja de los perfiles
//
// Los perfiles en y se calculan en una franja vertical |x| <= x_m. Por
// convención su ancho es el del orificio: x_m = r_out, ancho 2 x_m = D. El
// simulador >= 3.4 registra r_out en la cabecera de procedencia, y las
// herramientas lo usan para fijar o verificar x_m.
// ---------------------------------------------------------------------------

/// Franja resuelta y verificada.
struct Strip {
    double half_width = 0.0;  ///< Semiancho x_m
    double r_out = std::numeric_limits<double>::quiet_NaN(); ///< Semiancho del orificio (NaN si los archivos no lo registran)
    /// "orificio" (x_m = r_out), "libre" (otro ancho, pedido con
    /// --ancho-libre) o "sin_verificar" (archivos sin r_out).
    std::string kind;
};

/// Resuelve la franja a partir del argumento \p spec ("orificio" o un
/// semiancho positivo) y del r_out de los archivos.
///
/// Con "orificio", x_m = r_out. Con un número, debe coincidir con r_out
/// salvo que \p free_width sea true; si los archivos no registran r_out, se
/// acepta sin verificar. \p x_center distinto de cero solo se admite con
/// \p free_width.
/// \throws std::invalid_argument si la franja no es la del orificio y no se
///         pidió \p free_width, o si \p spec no es válido.
Strip resolve_strip(const std::string& spec, double r_out, bool free_width,
                    double x_center = 0.0);

/// Campos de la franja para la cabecera de las salidas:
/// `x_m=<x_m>  ancho=<2 x_m>  r_out=<r>  D=<2 r>  franja=<tipo>`.
std::string strip_fields(const Strip& s);

/// Línea de comentario que explica los campos de la franja.
std::string strip_comment();

} // namespace dem
