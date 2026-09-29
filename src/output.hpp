/*! \file output.hpp
 * \brief Archivos de salida por frame y cabecera de procedencia.
 *
 * Todos los archivos se escriben en `frames_<dirID>/` y se numeran con
 * n_frame, la cantidad de pasos transcurridos desde t_Register: los archivos
 * de un mismo instante comparten el número de frame. Cada archivo lleva una
 * cabecera de procedencia (provenance_header). Los formatos de columnas
 * están descritos en README.md.
 *
 * Con save_roi_only, las salidas por grano incluyen solo los granos cuyo
 * centro está en el ROI, y fc_*.dat incluye todos los contactos de esos
 * granos.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#pragma once

#include "global_setup.hpp"

#include <box2d/box2d.h>

#include <cstdint>
#include <string>

/*! Entero con 6 dígitos y ceros a la izquierda (numeración de frames). */
std::string frame_id_string(int num);

/*! Fecha y hora local, "AAAA-MM-DD hh:mm:ss". */
std::string local_time_string();

/*! \brief Líneas de comentario con la procedencia de un archivo de salida:
 * paso, frame, tiempo, dt, fase de la excitación (w t mod 2 pi), versión del
 * código (hash de git) y archivo y hash de parámetros.
 * \return Líneas que empiezan con '#', terminadas en '\\n'
 */
std::string provenance_header(const GlobalSetup &gs, double t, uint32_t n_step,
                              int n_frame);

/*! \brief Guarda la configuración (`<pre>_<frame>.xy`): granos
 * "gid 1 x y radio tipo" y paredes "gid 2 x1 y1 x2 y2 etiqueta".
 */
void save_frame(b2World *w, int n_frame, int n_step, const GlobalSetup &gs);

/*! \brief Guarda velocidades y energías cinéticas (`<pre>_<frame>.ve`):
 * "gid tipo x y vx vy w E_kin_lin E_kin_rot".
 */
void save_velocities(int n_frame, double t, uint32_t n_step, b2World *w,
                     const GlobalSetup &gs);

/*! \brief Guarda las fuerzas de contacto (`fc_<pre>_<frame>.dat`), una línea
 * por punto de contacto:
 * "gid_A gid_B cp.x cp.y Fn Ft nx ny xA yA xB yB n_pc tipo".
 * Fn y Ft son las componentes de la fuerza sobre B (convención en
 * contacts.hpp).
 */
void save_contacts(b2World *w, double t, uint32_t n_step, int n_frame,
                   const GlobalSetup &gs);

/*! \brief Guarda el tensor de estrés de contacto de cada grano
 * (`<pre>_<frame>.sxy`),
 *
 *     s_ij = (1 / A_grano) sum_c f_i l_j,
 *
 * con f la fuerza sobre el grano y l = punto de contacto - centro del grano,
 * su parte debida solo a las fuerzas normales, la posición, la velocidad y
 * el número de contactos activos. Actualiza las presiones mínima y máxima
 * (-tr(s) / 2) registradas.
 */
void save_stress(b2World *w, int n_frame, const GlobalSetup &gs, double *p_min,
                 double *p_max, double t, uint32_t n_step);
