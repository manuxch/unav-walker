/*! \file contact_power.cpp
 * \brief Potencia exacta de los contactos (ver contact_power.hpp).
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#include "contact_power.hpp"

#include "body_data.hpp"

#include <algorithm>
#include <cmath>

namespace {

double dot(b2Vec2 a, double fx, double fy) { return a.x * fx + a.y * fy; }

/// Velocidad de un punto a distancia l del centro de un cuerpo que gira con w.
b2Vec2 point_velocity(b2Vec2 v, double w, b2Vec2 l) {
  return b2Vec2(static_cast<float>(v.x - w * l.y),
                static_cast<float>(v.y + w * l.x));
}

} // namespace

void ContactPower::before_step(b2World *w) {
  grain_state_.clear();
  wall_state_.clear();
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    BodyData *bd = body_data(b);
    if (b->GetType() == b2_dynamicBody && bd->is_grain) {
      bd->power_idx = static_cast<int>(grain_state_.size());
      grain_state_.push_back({b, b->GetTransform(), b->GetLinearVelocity(),
                              b->GetAngularVelocity(), bd->f_base,
                              bd->tau_base});
    } else {
      wall_state_.push_back({b, b->GetTransform(), b->GetLinearVelocity()});
    }
  }
}

void ContactPower::after_step(b2World *w, double dt) {
  dt_ = dt;
  total_ = PowerStep{};
  records_.clear();
  grains_.assign(grain_state_.size(), GrainPower{});
  const float h = static_cast<float>(dt_);
  const double inv_dt = 1.0 / dt_;

  // Velocidades medias del paso: v' (primera etapa, con la aritmética float
  // de b2Island::Solve) y v'' (final)
  for (size_t i = 0; i < grain_state_.size(); ++i) {
    const GrainState &s = grain_state_[i];
    const float m = s.body->GetMass(), I = s.body->GetInertia();
    const b2Vec2 v1 = s.v0 + (h * (1.0f / m)) * s.force;
    const float w1 = s.w0 + h * (1.0f / I) * s.torque;
    const b2Vec2 v2 = s.body->GetLinearVelocity();
    const float w2 = s.body->GetAngularVelocity();
    GrainPower &g = grains_[i];
    g.body = s.body;
    g.pos = s.xf.p;
    g.f_base = s.force;
    g.tau_base = s.torque;
    g.w_lin = 0.5 * m * (double(v1.x) * v1.x + double(v1.y) * v1.y) -
              0.5 * m * (double(s.v0.x) * s.v0.x + double(s.v0.y) * s.v0.y);
    g.w_piv = 0.5 * I * (double(w1) * w1 - double(s.w0) * s.w0);
    g.vm = b2Vec2(0.5f * (v1.x + v2.x), 0.5f * (v1.y + v2.y));
    g.wm = 0.5 * (double(w1) + w2);
    const double k1 = 0.5 * m * (double(v1.x) * v1.x + double(v1.y) * v1.y) +
                      0.5 * I * double(w1) * w1;
    const double k2 = 0.5 * m * (double(v2.x) * v2.x + double(v2.y) * v2.y) +
                      0.5 * I * double(w2) * w2;
    g.dk_cont = k2 - k1;
    g.k_scale = k1 + k2;
  }

  auto transform_of = [&](const b2Body *b) -> b2Transform {
    const BodyData *bd = body_data(b);
    if (bd->is_grain) return grain_state_[bd->power_idx].xf;
    for (const WallState &ws : wall_state_)
      if (ws.body == b) return ws.xf;
    return b->GetTransform();
  };
  auto wall_velocity = [&](const b2Body *b) -> b2Vec2 {
    for (const WallState &ws : wall_state_)
      if (ws.body == b) return ws.v;
    return b2Vec2_zero;
  };

  for (b2Contact *c = w->GetContactList(); c; c = c->GetNext()) {
    if (!c->IsTouching()) continue;
    const b2Fixture *fa = c->GetFixtureA(), *fb = c->GetFixtureB();
    const b2Body *ba = fa->GetBody(), *bb = fb->GetBody();
    const BodyData *da = body_data(ba), *db = body_data(bb);
    // Normal y puntos: con las transformaciones del inicio del paso, que es
    // lo que usa el solver
    b2WorldManifold wm;
    wm.Initialize(c->GetManifold(), transform_of(ba), fa->GetShape()->m_radius,
                  transform_of(bb), fb->GetShape()->m_radius);
    for (int i = 0; i < c->GetManifold()->pointCount; ++i) {
      const b2ManifoldPoint &mp = c->GetManifold()->points[i];
      ContactRecord r;
      r.a = ba;
      r.b = bb;
      r.a_grain = da->is_grain;
      r.b_grain = db->is_grain;
      r.point = wm.points[i];
      const b2Vec2 t = b2Cross(wm.normal, 1.0f); // (n.y, -n.x), como el solver
      r.fn = mp.normalImpulse * inv_dt;
      r.ft = mp.tangentImpulse * inv_dt;
      r.fx = r.fn * wm.normal.x + r.ft * t.x;
      r.fy = r.fn * wm.normal.y + r.ft * t.y;

      // Velocidad media del paso en el punto de contacto y torque (l x f)_z
      double tau_a = 0.0, tau_b = 0.0;
      if (r.a_grain) {
        const GrainPower &g = grains_[da->power_idx];
        const b2Vec2 l = r.point - transform_of(ba).p;
        r.ua = point_velocity(g.vm, g.wm, l);
        tau_a = -(l.x * r.fy - l.y * r.fx);
        r.p_tr_a = -dot(g.vm, r.fx, r.fy);
        r.p_rot_a = g.wm * tau_a;
      } else {
        r.ua = wall_velocity(ba);
      }
      if (r.b_grain) {
        const GrainPower &g = grains_[db->power_idx];
        const b2Vec2 l = r.point - transform_of(bb).p;
        r.ub = point_velocity(g.vm, g.wm, l);
        tau_b = l.x * r.fy - l.y * r.fx;
        r.p_tr_b = dot(g.vm, r.fx, r.fy);
        r.p_rot_b = g.wm * tau_b;
      } else {
        r.ub = wall_velocity(bb);
      }

      // Reparto de la energía del contacto
      if (r.a_grain && r.b_grain) {
        r.t_ab = dot(r.ua + r.ub, r.fx, r.fy) * 0.5;
        r.d = -dot(r.ub - r.ua, r.fx, r.fy);
        total_.d_gg += r.d * dt_;
        grains_[da->power_idx].t_in -= r.t_ab;
        grains_[db->power_idx].t_in += r.t_ab;
        grains_[da->power_idx].d_gg += 0.5 * r.d;
        grains_[db->power_idx].d_gg += 0.5 * r.d;
      } else {
        // Un grano G y una pared W; f_G es la fuerza sobre el grano
        const bool g_is_b = r.b_grain;
        const double sg = g_is_b ? 1.0 : -1.0;
        const b2Vec2 u_g = g_is_b ? r.ub : r.ua;
        const b2Vec2 u_w = g_is_b ? r.ua : r.ub;
        r.d = -dot(u_g - u_w, sg * r.fx, sg * r.fy);
        r.w_wall = dot(u_w, sg * r.fx, sg * r.fy);
        r.t_ab = 0.0;
        total_.d_gp += r.d * dt_;
        total_.w_par += r.w_wall * dt_;
        GrainPower &gr = grains_[(g_is_b ? db : da)->power_idx];
        gr.d_gw += r.d;
        gr.p_wall += r.w_wall;
      }

      // Sumas por grano
      auto add_grain = [&](const BodyData *d, double sg, double p_tr,
                           double p_rot, double tau, bool gg) {
        GrainPower &g = grains_[d->power_idx];
        (gg ? g.c_gg : g.c_gw)[0] += sg * r.fx;
        (gg ? g.c_gg : g.c_gw)[1] += sg * r.fy;
        g.tau += tau;
        (gg ? g.p_tr_gg : g.p_tr_gw) += p_tr;
        g.p_rot += p_rot;
      };
      const bool gg = r.a_grain && r.b_grain;
      if (r.a_grain) add_grain(da, -1.0, r.p_tr_a, r.p_rot_a, tau_a, gg);
      if (r.b_grain) add_grain(db, 1.0, r.p_tr_b, r.p_rot_b, tau_b, gg);
      records_.push_back(r);
    }
  }

  max_residual_ = 0.0;
  for (const GrainPower &g : grains_) {
    const double tr = g.p_tr_gg + g.p_tr_gw;
    total_.w_cont_tr += tr * dt_;
    total_.w_cont_rot += g.p_rot * dt_;
    const double pred = (tr + g.p_rot) * dt_;
    // El solver trabaja en float: el residuo esperado es ~1e-7 de la
    // energía cinética del grano
    if (g.k_scale > 1e-12)
      max_residual_ =
          std::max(max_residual_, std::fabs(g.dk_cont - pred) / g.k_scale);
  }
}
