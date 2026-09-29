/*! \file body_data.hpp
 * \brief Datos propios asociados a cada cuerpo de Box2D (granos y paredes).
 *
 * Cada b2Body guarda en su userData un puntero a un BodyData. Los BodyData
 * pertenecen a SiloBodies (silo_builder.hpp) y deben vivir mientras exista
 * el mundo de Box2D.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#pragma once

#include <box2d/box2d.h>

/*! Identificadores (gid) de los cuerpos que no son granos. Los granos tienen
 * gid >= 0. */
constexpr int kGidSilo = -100;           //!< Paredes del silo
constexpr int kGidLid = -110;            //!< Tapa del orificio
constexpr int kGidMeasuringFloor = -200; //!< Fondo de medición

/*! \struct BodyData
 * \brief Datos de un cuerpo que Box2D no almacena.
 */
struct BodyData {
  int tipo = 0;          //!< Tipo de grano (orden en el archivo de parámetros)
  bool is_grain = false; //!< true para granos, false para paredes
  bool is_in = false;    //!< true mientras el grano no salió del silo
  int gid = 0;           //!< Identificador global (ver kGid*)
  double fric_d = 0.0;   //!< Coeficiente de fricción dinámica con la base
  double fric_s = 0.0;   //!< Coeficiente de fricción estática con la base

  // Estado auxiliar para check_force_balance (diagnostics.hpp): fuerza y
  // torque de la base aplicados en el paso y velocidades previas al Step.
  b2Vec2 f_base = b2Vec2(0.0f, 0.0f); //!< Fuerza de la base (último paso)
  float tau_base = 0.0f;              //!< Torque de la base (último paso)
  b2Vec2 v_prev = b2Vec2(0.0f, 0.0f); //!< Velocidad lineal antes del Step
  float w_prev = 0.0f;                //!< Velocidad angular antes del Step
};

/*! Devuelve los datos propios de un cuerpo. */
inline BodyData *body_data(const b2Body *b) {
  return reinterpret_cast<BodyData *>(b->GetUserData().pointer);
}
