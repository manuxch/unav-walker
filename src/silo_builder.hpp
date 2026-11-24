/*! \file silo_builder.hpp
 * \brief Construcción del sistema: paredes del silo, tapa o fondo, y granos.
 *
 * Geometría (coordenadas de la simulación): el silo ocupa |x| <= R,
 * 0 <= y <= H, cerrado arriba. El orificio ocupa |x| <= r en y = 0.
 *
 *  - Silo normal: las paredes son una cadena de 6 vértices con el orificio
 *    abierto, y una tapa (gid = kGidLid) lo cierra hasta t = tBlock.
 *  - fondo_medicion: las paredes son una U sin fondo y un fondo de ancho
 *    completo (gid = kGidMeasuringFloor) que nunca se remueve.
 *
 * Con paredes_moviles, el silo y la tapa son cuerpos cinemáticos que
 * acompañan el movimiento de la base (move_walls); si no, son estáticos.
 *
 * Los granos se insertan en posiciones y orientaciones al azar dentro del
 * silo; las superposiciones iniciales se resuelven en main.cpp con algunos
 * pasos de Box2D.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#pragma once

#include "body_data.hpp"
#include "global_setup.hpp"
#include "rng.hpp"

#include <box2d/box2d.h>

#include <vector>

/*! \struct SiloBodies
 * \brief Cuerpos creados y los BodyData que Box2D referencia.
 *
 * Box2D guarda punteros a los BodyData, así que un SiloBodies no se puede
 * copiar ni mover y debe vivir más que el b2World.
 */
struct SiloBodies {
  BodyData silo_data; //!< Datos de las paredes (gid = kGidSilo)
  BodyData lid_data;  //!< Datos de la tapa o del fondo de medición
  std::vector<std::vector<BodyData>> grain_data; //!< Datos por tipo y grano
  b2Body *silo = nullptr;                        //!< Paredes del silo
  b2Body *lid = nullptr;                         //!< Tapa o fondo de medición
  double total_grain_mass = 0.0;                 //!< Masa total de los granos

  SiloBodies() = default;
  SiloBodies(const SiloBodies &) = delete;
  SiloBodies &operator=(const SiloBodies &) = delete;
};

/*! \brief Con paredes_moviles, lleva las paredes y la tapa a la posición de
 * la base en el instante t + dt.
 *
 * Los cuerpos son cinemáticos: se les da la velocidad media del paso,
 * (y_base(t + dt) - y(t)) / dt, de modo que el integrador de Box2D los deja
 * exactamente en y_base(t + dt) y el solver de contactos usa esa misma
 * velocidad en la velocidad relativa grano-pared. Se llama antes de
 * b2World::Step. La base se mueve en -y: y_base = -MovBase::x.
 * \param bodies   Cuerpos del silo
 * \param y_target Posición (en y) de la base al final del paso
 * \param dt       Paso temporal
 */
void move_walls(SiloBodies &bodies, double y_target, double dt);

/*! \brief Crea las paredes, la tapa (o el fondo de medición) y los granos.
 * \param world  Mundo de Box2D
 * \param gs     Parámetros
 * \param rng    Generador de números aleatorios (posiciones y ángulos)
 * \param bodies Cuerpos creados y sus datos (salida)
 */
void build_silo(b2World *world, const GlobalSetup &gs, RNG &rng,
                SiloBodies &bodies);
