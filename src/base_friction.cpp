/*! \file base_friction.cpp
 * \brief Excitación de la base vibrada y fricción base-grano.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#include "base_friction.hpp"

#include "body_data.hpp"
#include "contacts.hpp"

#include <algorithm>
#include <cmath>

MovBase base_excitation(double t, double gamma, double w,
                        const GlobalSetup &gs) {
  double y, vy, ay, rho = gs.silo.rho, phi = gs.silo.phi;
  double Aa = gamma * gs.g;
  double Av = Aa / w;
  double Ax = Av / w;
  y = -rho * Ax * std::sin(w * t) -
      (1 - rho) / 4.0 * Ax * std::sin(2 * w * t + phi);
  vy = -rho * Av * std::cos(w * t) -
       (1 - rho) / 2.0 * Av * std::cos(2 * w * t + phi);
  ay = rho * Aa * std::sin(w * t) + (1 - rho) * Aa * std::sin(2 * w * t + phi);
  return {y, vy, ay};
}

b2Vec2 karnopp(b2Vec2 v_rel, b2Vec2 F_ext, b2Vec2 a_base, double m, double dt,
               double v_tol, double mu_s, double mu_d, double N) {
  const double v_norm = v_rel.Length();
  const double v_stick = std::max(v_tol, mu_d * N * dt / m);
  if (v_norm < v_stick) {
    // Adherencia: fuerza para que v_rel = 0 al final del paso
    double fx = m * a_base.x - F_ext.x - m * v_rel.x / dt;
    double fy = m * a_base.y - F_ext.y - m * v_rel.y / dt;
    double f_norm = std::sqrt(fx * fx + fy * fy);
    double f_max = mu_s * N;
    if (f_norm > f_max) { // Ruptura: se satura en mu_s N
      fx *= f_max / f_norm;
      fy *= f_max / f_norm;
    }
    return b2Vec2(static_cast<float>(fx), static_cast<float>(fy));
  }
  // Deslizamiento
  double k = -mu_d * N / v_norm;
  return b2Vec2(static_cast<float>(k * v_rel.x),
                static_cast<float>(k * v_rel.y));
}

double pivot_friction(double w, double tau_ext, double I, double R, double dt,
                      double v_tol, double mu_s, double mu_d, double N) {
  const double arm = 2.0 / 3.0 * R; // brazo efectivo con presión uniforme
  const double tau_d = mu_d * N * arm;
  const double w_stick = std::max(v_tol / R, tau_d * dt / I);
  if (std::fabs(w) < w_stick) {
    // Adherencia: torque para que w = 0 al final del paso
    double tau = -tau_ext - I * w / dt;
    double tau_max = mu_s * N * arm;
    if (std::fabs(tau) > tau_max) tau = std::copysign(tau_max, tau);
    return tau;
  }
  return -std::copysign(tau_d, w); // Deslizamiento
}

void apply_base_friction(b2World *w, double base_vel, double base_acc,
                         double epsilon_v, double g, double dt) {
  // La base se mueve en -y con la velocidad y aceleración de base_excitation.
  const b2Vec2 base_vel_vec(0.0f, static_cast<float>(-base_vel));
  const b2Vec2 base_acc_vec(0.0f, static_cast<float>(-base_acc));
  const double inv_dt = 1.0 / dt;
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    if (b->GetType() != b2_dynamicBody) continue;
    double m = b->GetMass();
    double N = g * m;
    BodyData *bd = body_data(b);
    b2Vec2 v_rel = b->GetLinearVelocity() - base_vel_vec;
    // Resto de las fuerzas y torques sobre el grano: contactos del último Step
    b2Vec2 F_ext;
    double tau_ext;
    body_contact_wrench(b, inv_dt, &F_ext, &tau_ext);
    b2Vec2 f_roce = karnopp(v_rel, F_ext, base_acc_vec, m, dt, epsilon_v,
                            bd->fric_s, bd->fric_d, N);
    double R = b->GetFixtureList()->GetShape()->m_radius;
    double tau_roce =
        pivot_friction(b->GetAngularVelocity(), tau_ext, b->GetInertia(), R, dt,
                       epsilon_v, bd->fric_s, bd->fric_d, N);
    bd->f_base = f_roce;
    bd->tau_base = static_cast<float>(tau_roce);
    b->ApplyForceToCenter(f_roce, true);
    b->ApplyTorque(static_cast<float>(tau_roce), true);
  }
}
