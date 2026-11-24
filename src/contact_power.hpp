/*! \file contact_power.hpp
 * \brief Potencia exacta de los contactos, por contacto, por grano y por paso.
 *
 * En cada b2World::Step, Box2D integra primero las fuerzas aplicadas (la
 * base) y lleva las velocidades a v' y w'; después resuelve los contactos
 * con impulsos y llega a v'' y w''. Si J_c es el impulso total del contacto c
 * sobre el grano i (b2ManifoldPoint::normalImpulse y tangentImpulse, con
 * la convención de contacts.hpp) y l_c el brazo de palanca,
 *
 *     m (v'' - v') = sum_c J_c,        I (w'' - w') = sum_c (l_c x J_c)_z,
 *
 * y entonces el cambio de energía cinética de los contactos es, exactamente,
 *
 *     dK_i = sum_c J_c . u_c,    u_c = vm + wm z x l_c,
 *     vm = (v' + v'') / 2,   wm = (w' + w'') / 2.
 *
 * (la velocidad media del paso en el punto de contacto). Con la fuerza
 * media f = J / dt, para un contacto entre los granos i y j (f_ij: fuerza
 * que j ejerce sobre i),
 *
 *     P_i = f_ij . u_i = T_{j->i} - D_ij / 2,
 *     T_{j->i} = f_ij . (u_i + u_j) / 2,    D_ij = -f_ij . (u_i - u_j),
 *
 * donde T es la energía que pasa de j a i y D la que disipa el contacto.
 * Con una pared de velocidad u_w (nula si es fija), la fuerza sobre el
 * grano f cumple P_i = -D + W_pared, con D = -f . (u_i - u_w) la disipación
 * y W_pared = f . u_w el trabajo de la pared sobre el grano (una fuente).
 *
 * Los brazos de palanca y la normal son los que usa el solver: se calculan
 * con las posiciones de los cuerpos al inicio del paso (before_step), no con
 * las finales. Las velocidades y las potencias son del sistema del
 * laboratorio.
 *
 * Uso en el bucle (después de la descarga y la reinyección, como
 * EnergyBudget):
 *
 *     power.before_step(&world);
 *     world.Step(...);
 *     power.after_step(&world, dt);
 *
 * Después de after_step, records(), grains() y total() describen el paso.
 * Energías y potencias en unidades reducidas (m g d y m g sqrt(g d)).
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#pragma once

#include <box2d/box2d.h>

#include <vector>

/*! \struct ContactRecord
 * \brief Un punto de contacto de un paso (ver la descripción del módulo). */
struct ContactRecord {
  const b2Body *a = nullptr; //!< Cuerpo A (la normal apunta de A a B)
  const b2Body *b = nullptr; //!< Cuerpo B
  bool a_grain = false;      //!< A es un grano
  bool b_grain = false;      //!< B es un grano
  b2Vec2 point;              //!< Punto de contacto (posiciones al inicio)
  double fn = 0.0, ft = 0.0; //!< Fuerza normal y tangencial sobre B
  double fx = 0.0, fy = 0.0; //!< Fuerza sobre B (sobre A es la opuesta)
  b2Vec2 ua, ub;             //!< Velocidad media del paso en el punto de A y B
  double t_ab = 0.0;  //!< T_{A->B} = f . (ua + ub) / 2 (energía de A a B)
  double d = 0.0;     //!< Disipación del contacto, -f . (ub - ua)
  double w_wall = 0.0; //!< Grano-pared: trabajo de la pared sobre el grano
  double p_tr_a = 0.0, p_rot_a = 0.0; //!< Potencia recibida por A (granos)
  double p_tr_b = 0.0, p_rot_b = 0.0; //!< Potencia recibida por B (granos)
};

/*! \struct GrainPower
 * \brief Magnitudes de un grano en un paso, sumadas sobre sus contactos. */
struct GrainPower {
  const b2Body *body = nullptr;
  b2Vec2 vm;                 //!< Velocidad media del paso
  double wm = 0.0;           //!< Velocidad angular media del paso
  double c_gg[2] = {0, 0};   //!< Fuerza de contacto con otros granos
  double c_gw[2] = {0, 0};   //!< Fuerza de contacto con paredes
  double tau = 0.0;          //!< Torque de contacto (respecto del centro)
  double p_tr_gg = 0.0;      //!< Potencia traslacional de los contactos gg
  double p_tr_gw = 0.0;      //!< Potencia traslacional de los contactos gw
  double p_rot = 0.0;        //!< Potencia rotacional de los contactos
  double dk_cont = 0.0;      //!< Cambio de K de los contactos, medido
  double k_scale = 0.0;      //!< K(v') + K(v''), escala del residuo
};

/*! \struct PowerStep
 * \brief Totales del paso, en energía (potencia por dt). */
struct PowerStep {
  double w_par = 0.0;      //!< Trabajo de las paredes sobre los granos
  double d_gg = 0.0;       //!< Disipación en contactos grano-grano
  double d_gp = 0.0;       //!< Disipación en contactos grano-pared
  double w_cont_tr = 0.0;  //!< Trabajo traslacional de los contactos
  double w_cont_rot = 0.0; //!< Trabajo rotacional de los contactos
  /// Trabajo total de los contactos: -(d_gg + d_gp) + w_par.
  double w_cont() const { return w_cont_tr + w_cont_rot; }
};

/*! \class ContactPower
 * \brief Calcula la potencia exacta de los contactos de cada paso. */
class ContactPower {
public:
  /*! \brief Guarda el estado de los cuerpos antes de b2World::Step.
   * La fuerza y el torque de la base se leen de BodyData::f_base y
   * tau_base (como EnergyBudget).
   * \param w Mundo
   */
  void before_step(b2World *w);

  /// Calcula los contactos y las potencias del paso (de duración \p dt),
  /// tras b2World::Step.
  void after_step(b2World *w, double dt);

  const std::vector<ContactRecord> &records() const { return records_; }
  const std::vector<GrainPower> &grains() const { return grains_; }
  const PowerStep &total() const { return total_; }
  /// Mayor residuo relativo por grano del último paso, |dK medido - dK por
  /// contactos| / (K(v') + K(v'')): el redondeo float del solver (~1e-6).
  double max_residual() const { return max_residual_; }

private:
  struct WallState {
    const b2Body *body;
    b2Transform xf;
    b2Vec2 v;
  };
  struct GrainState {
    b2Body *body;
    b2Transform xf;
    b2Vec2 v0;
    float w0;
    b2Vec2 force;
    float torque;
  };
  double dt_ = 0.0;
  std::vector<GrainState> grain_state_;
  std::vector<WallState> wall_state_;
  std::vector<GrainPower> grains_;
  std::vector<ContactRecord> records_;
  PowerStep total_;
  double max_residual_ = 0.0;
};
