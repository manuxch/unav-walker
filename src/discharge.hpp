/*! \file discharge.hpp
 * \brief Descarga por el orificio: conteo, reinyección y medidas en la salida.
 *
 * El orificio ocupa |x| <= r en y = 0 y los granos salen hacia y < 0.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#pragma once

#include "global_setup.hpp"
#include "rng.hpp"

#include <box2d/box2d.h>

#include <fstream>
#include <vector>

/*! \brief Marca como descargados los granos cuyo centro pasó por debajo de
 * y = -radio y escribe una línea por grano en el archivo de flujo:
 * "granos_descargados tipo t total_tipo_1 ... total_tipo_n total".
 * \param w         Mundo
 * \param suma_tipo Descargados acumulados por tipo (se actualiza)
 * \param paso      Paso de simulación (el tiempo es paso * dt)
 * \param flux_file Archivo de flujo (si no está abierto no se escribe)
 * \param gs        Parámetros
 * \return Cantidad de granos descargados en esta llamada
 */
int count_discharged(b2World *w, std::vector<int> &suma_tipo, int paso,
                     std::ofstream &flux_file, const GlobalSetup &gs);

/*! \brief Reinyecta (o elimina) los granos descargados que cayeron por
 * debajo de y = -10.
 *
 * Con \p reinject, cada grano se coloca en una posición al azar de la franja
 * |x| <= 0.9 R, 0.75 H <= y <= 0.95 H sin superposición con otros cuerpos, y
 * con velocidad nula. Si no encuentra lugar después de 100 intentos, lo
 * reintenta en el paso siguiente. Sin \p reinject, el grano se elimina.
 * \param w        Mundo
 * \param gs       Parámetros
 * \param rng      Generador de números aleatorios
 * \param reinject true: reinyectar; false: eliminar
 */
void reinject_grains(b2World *w, const GlobalSetup &gs, RNG &rng,
                     bool reinject);

/*! \brief Escribe "t pf_bulk pf_out": fracción de área ocupada en la franja
 * R <= y <= 2 R (bulk) y fracción lineal ocupada sobre el orificio (y = 0).
 */
void save_packing_fraction(b2World *w, const GlobalSetup &gs, double t,
                           std::ofstream &fout);

/*! \brief Área de la intersección entre un círculo de radio \p r centrado
 * en altura \p y y la franja y_inf <= y <= y_sup (el círculo corta a lo sumo
 * uno de los dos bordes).
 */
double clipped_circle_area(double y_inf, double y_sup, double y, double r);

/*! \brief Acumula los perfiles de ocupación y de velocidad vy sobre el
 * orificio (y = 0), en \p n_bins bines que dividen |x| <= r_out.
 * \param w         Mundo
 * \param vel_0     Suma de vy por bin
 * \param pf_0      Cantidad de granos que ocupan cada bin
 * \param bin_count Cantidad de registros por bin
 * \param n_bins    Número de bines
 * \param r_out     Semiancho del orificio
 */
void update_outlet_profiles(b2World *w, double *vel_0, size_t *pf_0,
                            size_t *bin_count, int n_bins, double r_out);
