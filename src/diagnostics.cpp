/*! \file diagnostics.cpp
 * \brief Chequeo del balance de impulso de cada grano.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#include "diagnostics.hpp"

#include "body_data.hpp"
#include "contacts.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>

void record_pre_step(b2World *w) {
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    if (b->GetType() != b2_dynamicBody) continue;
    BodyData *bd = body_data(b);
    bd->v_prev = b->GetLinearVelocity();
    bd->w_prev = b->GetAngularVelocity();
  }
}

void check_force_balance(b2World *w, const GlobalSetup &gs, double t,
                         uint32_t n_step, std::ofstream &fout) {
  const double dt = gs.dt;
  // Acumuladores: [0] tangente de Box2D, [1] tangente invertida (control)
  double res_lin[2] = {0, 0}, ref_lin[2] = {0, 0};
  double res_ang[2] = {0, 0}, ref_ang[2] = {0, 0};
  double max_rel_lin = 0.0, max_rel_ang = 0.0;
  int n_grains = 0, n_bad = 0;
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    if (b->GetType() != b2_dynamicBody) continue;
    const BodyData *bd = body_data(b);
    if (!bd->is_grain) continue;
    const double m = b->GetMass();
    const double I = b->GetInertia();
    const b2Vec2 center = b->GetWorldCenter();
    const b2Vec2 dv = b->GetLinearVelocity() - bd->v_prev;
    const double dw = b->GetAngularVelocity() - bd->w_prev;
    for (int k = 0; k < 2; ++k) {
      const double tsgn = (k == 0) ? 1.0 : -1.0;
      // Impulso y momento angular de contacto, y sus sumas en módulo (escala)
      double Jx = 0.0, Jy = 0.0, Lz = 0.0, sum_J = 0.0, sum_L = 0.0;
      for (b2ContactEdge *ce = b->GetContactList(); ce; ce = ce->next) {
        b2Contact *c = ce->contact;
        if (!c->IsTouching()) continue;
        b2WorldManifold wm;
        c->GetWorldManifold(&wm);
        double sgn = (c->GetFixtureB()->GetBody() == b) ? 1.0 : -1.0;
        for (int i = 0; i < c->GetManifold()->pointCount; ++i) {
          ContactPointForce cpf = contact_point_force(c, wm, i, 1.0);
          double jx =
              sgn * (cpf.fn * cpf.normal.x + tsgn * cpf.ft * cpf.tangent.x);
          double jy =
              sgn * (cpf.fn * cpf.normal.y + tsgn * cpf.ft * cpf.tangent.y);
          b2Vec2 l = cpf.point - center;
          double lz = l.x * jy - l.y * jx;
          Jx += jx;
          Jy += jy;
          Lz += lz;
          sum_J += std::sqrt(jx * jx + jy * jy);
          sum_L += std::fabs(lz);
        }
      }
      double rx = m * dv.x - dt * bd->f_base.x - Jx;
      double ry = m * dv.y - dt * bd->f_base.y - Jy;
      double rl = std::sqrt(rx * rx + ry * ry);
      double refl = m * dv.Length() + dt * bd->f_base.Length() + sum_J;
      double ra = std::fabs(I * dw - dt * bd->tau_base - Lz);
      double refa = std::fabs(I * dw) + dt * std::fabs(bd->tau_base) + sum_L;
      res_lin[k] += rl;
      ref_lin[k] += refl;
      res_ang[k] += ra;
      ref_ang[k] += refa;
      if (k == 0) {
        // Piso de la referencia: impulso típico de la fricción con la base
        // (evita residuos relativos espurios en granos casi sin fuerzas).
        const double r = b->GetFixtureList()->GetShape()->m_radius;
        const double j0 = dt * bd->fric_d * gs.g * m;
        double rel_l = rl / std::max(refl, j0);
        double rel_a = ra / std::max(refa, r * j0);
        max_rel_lin = std::max(max_rel_lin, rel_l);
        max_rel_ang = std::max(max_rel_ang, rel_a);
        if (rel_l > 1e-2 || rel_a > 1e-2) ++n_bad;
      }
    }
    ++n_grains;
  }
  auto ratio = [](double a, double b) { return (b > 0) ? a / b : 0.0; };
  fout << std::setprecision(10) << t << " " << n_step << " " << n_grains
       << std::scientific << std::setprecision(4) << " "
       << ratio(res_lin[0], ref_lin[0]) << " " << max_rel_lin << " "
       << ratio(res_ang[0], ref_ang[0]) << " " << max_rel_ang << " " << n_bad
       << " " << ratio(res_lin[1], ref_lin[1]) << " "
       << ratio(res_ang[1], ref_ang[1]) << std::defaultfloat << "\n";
}
