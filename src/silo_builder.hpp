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
  b2Body *lid = nullptr;                         //!< Tapa o fondo de medición
  double total_grain_mass = 0.0;                 //!< Masa total de los granos

  SiloBodies() = default;
  SiloBodies(const SiloBodies &) = delete;
  SiloBodies &operator=(const SiloBodies &) = delete;
};

/*! \brief Crea las paredes, la tapa (o el fondo de medición) y los granos.
 * \param world  Mundo de Box2D
 * \param gs     Parámetros
 * \param rng    Generador de números aleatorios (posiciones y ángulos)
 * \param bodies Cuerpos creados y sus datos (salida)
 */
void build_silo(b2World *world, const GlobalSetup &gs, RNG &rng,
                SiloBodies &bodies);
