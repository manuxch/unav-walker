/*! \file diagnostics.hpp
 * \brief Chequeo del balance de impulso de cada grano.
 *
 * Verifica que las fuerzas de contacto reconstruidas (contacts.hpp) más la
 * fricción de la base expliquen el cambio de velocidad de cada grano en un
 * Step:
 *
 *     m (v - v_prev) = dt F_base + sum_c J_c,
 *     I (w - w_prev) = dt tau_base + sum_c l_c x J_c,
 *
 * con J_c los impulsos de contacto y l_c = punto de contacto - centro. Como
 * control, repite el cálculo con la tangente invertida (-t), que debe dar
 * residuos grandes. Los impulsos de los subpasos TOI (continuous_physics: T)
 * no quedan registrados en los contactos, así que con TOI el balance no
 * cierra para los granos que tuvieron colisiones nuevas.
 *
 * Uso en el bucle: record_pre_step antes de b2World::Step y
 * check_force_balance después.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#pragma once

#include "global_setup.hpp"

#include <box2d/box2d.h>

#include <cstdint>
#include <fstream>

/*! Guarda en BodyData la velocidad lineal y angular de cada grano justo antes
 * del Step. */
void record_pre_step(b2World *w);

/*! \brief Escribe una línea con los residuos del balance de impulso:
 * "t n_step n_granos rel_lin max_rel_lin rel_ang max_rel_ang n_bad
 * rel_lin_flip rel_ang_flip". Ver el significado de cada columna en la
 * cabecera de balance_*.dat (main.cpp).
 * \param w      Mundo, después del Step
 * \param gs     Parámetros
 * \param t      Tiempo al inicio del Step
 * \param n_step Paso de simulación
 * \param fout   Archivo de salida
 */
void check_force_balance(b2World *w, const GlobalSetup &gs, double t,
                         uint32_t n_step, std::ofstream &fout);
