/*! \file base_friction.hpp
 * \brief Excitación de la base vibrada y fricción base-grano.
 *
 * Los granos (discos) están apoyados sobre una base horizontal que vibra en
 * la dirección del eje y del silo. La gravedad no actúa en el plano de la
 * simulación: solo determina la carga normal N = m g sobre la base, que
 * entra en la fricción. La base ejerce sobre cada grano
 *
 *  - una fuerza de fricción de Karnopp (karnopp), según la velocidad del
 *    grano relativa a la base, y
 *  - un torque de fricción de pivoteo (pivot_friction), que se opone a la
 *    rotación del disco sobre la base.
 *
 * Ambos modelos usan los coeficientes fric_b_s (estático) y fric_b_d
 * (dinámico) del grano y tienen una fase de adherencia, en la que la
 * fricción equilibra al resto de las fuerzas hasta el límite estático.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#pragma once

#include "global_setup.hpp"

#include <box2d/box2d.h>

/*! \struct MovBase
 * \brief Posición, velocidad y aceleración de la base en un instante.
 */
struct MovBase {
  double x; //!< Posición
  double v; //!< Velocidad
  double a; //!< Aceleración
};

/*! \brief Excitación bi-armónica de la base.
 *
 * Aceleración a(t) = A [rho sin(w t) + (1 - rho) sin(2 w t + phi)], con
 * A = gamma g; la velocidad y la posición son sus primitivas con media nula.
 * \param t     Tiempo
 * \param gamma Aceleración reducida (Gamma)
 * \param w     Frecuencia angular del primer armónico (rad / tiempo)
 * \param gs    Parámetros (usa silo.rho, silo.phi y g)
 */
MovBase base_excitation(double t, double gamma, double w,
                        const GlobalSetup &gs);

/*! \brief Fuerza de fricción de la base sobre un grano (modelo de Karnopp).
 *
 * Adherencia: si |v_rel| < v_stick, la fuerza es la necesaria para que el
 * grano se mueva con la base al final del paso,
 *
 *     F_stick = m a_base - F_ext - m v_rel / dt,
 *
 * limitada en módulo a mu_s N. Deslizamiento: F = -mu_d N v_rel / |v_rel|.
 *
 * La banda de adherencia es v_stick = max(v_tol, mu_d N dt / m): en un paso
 * la fricción dinámica cambia la velocidad en mu_d N dt / m, de modo que una
 * banda más angosta nunca se alcanzaría y el grano oscilaría alrededor de
 * v_rel = 0.
 * \param v_rel  Velocidad del grano relativa a la base
 * \param F_ext  Resto de las fuerzas sobre el grano (contactos)
 * \param a_base Aceleración de la base
 * \param m      Masa del grano
 * \param dt     Paso temporal
 * \param v_tol  Umbral de velocidad para la adherencia (Cero_tol)
 * \param mu_s   Coeficiente de fricción estática
 * \param mu_d   Coeficiente de fricción dinámica
 * \param N      Carga normal sobre la base (m g)
 */
b2Vec2 karnopp(b2Vec2 v_rel, b2Vec2 F_ext, b2Vec2 a_base, double m, double dt,
               double v_tol, double mu_s, double mu_d, double N);

/*! \brief Torque de fricción de pivoteo de un disco apoyado sobre la base.
 *
 * Con presión de contacto uniforme sobre la cara del disco de radio R, el
 * torque de Coulomb que se opone al giro es (2/3) mu N R. Como en karnopp:
 * adherencia si |w| < w_stick, con el torque necesario para que w = 0 al
 * final del paso, tau = -tau_ext - I w / dt, limitado a (2/3) mu_s N R;
 * deslizamiento: tau = -(2/3) mu_d N R sign(w). La banda de adherencia es
 * w_stick = max(v_tol / R, (2/3) mu_d N R dt / I). La base no rota, así que
 * w es la velocidad angular relativa.
 *
 * Simplificación: la traslación y la rotación se tratan por separado (en la
 * fricción seca real están acopladas, efecto Contensou).
 * \param w       Velocidad angular del grano
 * \param tau_ext Resto de los torques sobre el grano (contactos)
 * \param I       Momento de inercia respecto del centro
 * \param R       Radio del disco
 * \param dt      Paso temporal
 * \param v_tol   Umbral de velocidad del borde (w R) para la adherencia
 * \param mu_s    Coeficiente de fricción estática
 * \param mu_d    Coeficiente de fricción dinámica
 * \param N       Carga normal sobre la base (m g)
 */
double pivot_friction(double w, double tau_ext, double I, double R, double dt,
                      double v_tol, double mu_s, double mu_d, double N);

/*! \brief Aplica la fricción de la base (fuerza y torque) a cada grano.
 *
 * La base se mueve en -y con la velocidad y la aceleración de
 * base_excitation. Los contactos del último Step se usan como fuerzas y
 * torques externos. La fuerza y el torque aplicados quedan en
 * BodyData::f_base y BodyData::tau_base.
 * \param w         Mundo
 * \param base_vel  Velocidad de la base (MovBase::v)
 * \param base_acc  Aceleración de la base (MovBase::a)
 * \param epsilon_v Umbral de velocidad para la adherencia (Cero_tol)
 * \param g         Gravedad (carga normal m g)
 * \param dt        Paso temporal
 */
void apply_base_friction(b2World *w, double base_vel, double base_acc,
                         double epsilon_v, double g, double dt);
