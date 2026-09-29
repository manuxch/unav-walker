/*! \file contacts.hpp
 * \brief Fuerzas de contacto reconstruidas a partir de los impulsos de Box2D.
 *
 * Convención (la misma de b2ContactSolver en Box2D 2.4): la normal n del
 * contacto apunta del cuerpo A al cuerpo B y la tangente es
 * t = b2Cross(n, 1) = (n.y, -n.x). La fuerza sobre B en un punto de contacto
 * es
 *
 *     F = (lambda_n n + lambda_t t) / dt,
 *
 * con lambda_n, lambda_t los impulsos normal y tangencial acumulados en el
 * último Step. La fuerza sobre A es -F.
 *
 * Todas las salidas y diagnósticos del programa usan contact_point_force,
 * de modo que la convención está definida en un único lugar.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#pragma once

#include <box2d/box2d.h>

/*! \struct ContactPointForce
 * \brief Fuerza en un punto de contacto (ver la convención en contacts.hpp).
 */
struct ContactPointForce {
  b2Vec2 point;   //!< Punto de contacto (coordenadas del mundo)
  b2Vec2 normal;  //!< Normal unitaria, de A hacia B
  b2Vec2 tangent; //!< Tangente unitaria, (n.y, -n.x)
  double fn;      //!< Componente normal (impulso normal * inv_dt)
  double ft;      //!< Componente tangencial (impulso tangencial * inv_dt)
  double fx;      //!< Componente x de la fuerza sobre B
  double fy;      //!< Componente y de la fuerza sobre B
};

/*! \brief Fuerza sobre el cuerpo B en el punto \p i de un contacto.
 * \param c      Contacto
 * \param wm     Manifold del contacto en coordenadas del mundo
 * \param i      Índice del punto del manifold
 * \param inv_dt Inverso del paso temporal; con 1 devuelve impulsos
 */
ContactPointForce contact_point_force(b2Contact *c, const b2WorldManifold &wm,
                                      int i, double inv_dt);

/*! \brief Fuerza y torque (respecto del centro de masa) netos de contacto
 * sobre un cuerpo, resueltos en el último Step.
 * \param b      Cuerpo
 * \param inv_dt Inverso del paso temporal
 * \param[out] F   Fuerza neta
 * \param[out] tau Torque neto
 */
void body_contact_wrench(b2Body *b, double inv_dt, b2Vec2 *F, double *tau);

/*! \brief Fuerza total que ejercen los granos sobre el cuerpo de pared con
 * identificador \p wall_gid, resuelta en el último Step.
 * \param w        Mundo
 * \param dt       Paso temporal
 * \param wall_gid Identificador de la pared (ver kGid* en body_data.hpp)
 */
b2Vec2 wall_force(b2World *w, double dt, int wall_gid);

/*! \brief Área de un grano (disco); 1 para las paredes. */
float grain_area(const b2Body *body);
