/*! \file dem_reader.hpp
 * \brief Declaración de funciones para leer archivos de simulación DEM.
 *
 * Convenciones de nombres de archivo:
 *   frm-{case_id}_{frame_id}.xy       — posiciones
 *   frm-{case_id}_{frame_id}.ve       — velocidades y energías
 *   fc_frm-{case_id}_{frame_id}.dat   — fuerzas de contacto
 *   frm-{case_id}_{frame_id}.sxy      — tensor de estrés por grano
 * (en general <pre>_{frame_id}.<ext>, con <pre> = preFrameFile)
 *
 * donde frame_id se formatea con 6 dígitos con ceros a la izquierda.
 *
 * \author Manuel Carlevaro
 * \date 2026-02-26
 */

#pragma once

#include "dem_types.hpp"

#include <filesystem>
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

/// Ruta de un archivo <pre>_<frame><ext>, p. ej. frame_path(dir, "frm", 12, ".sxy")
/// -> dir/frm_000012.sxy.
std::filesystem::path frame_path(const std::filesystem::path& dir,
                                 const std::string& pre, int frame_id,
                                 const std::string& ext);

/// Frames disponibles para los archivos <pre>_<frame><ext>, ordenados.
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

} // namespace dem
