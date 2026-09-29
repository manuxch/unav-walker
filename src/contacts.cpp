/*! \file contacts.cpp
 * \brief Fuerzas de contacto reconstruidas a partir de los impulsos de Box2D.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#include "contacts.hpp"

#include "body_data.hpp"

#include <cmath>

ContactPointForce contact_point_force(b2Contact *c, const b2WorldManifold &wm,
                                      int i, double inv_dt) {
  const b2ManifoldPoint &mp = c->GetManifold()->points[i];
  ContactPointForce f;
  f.point = wm.points[i];
  f.normal = wm.normal;
  f.tangent = b2Cross(wm.normal, 1.0f); // (n.y, -n.x), como b2ContactSolver
  f.fn = mp.normalImpulse * inv_dt;
  f.ft = mp.tangentImpulse * inv_dt;
  f.fx = f.fn * f.normal.x + f.ft * f.tangent.x;
  f.fy = f.fn * f.normal.y + f.ft * f.tangent.y;
  return f;
}

void body_contact_wrench(b2Body *b, double inv_dt, b2Vec2 *F, double *tau) {
  double fx = 0.0, fy = 0.0, tz = 0.0;
  const b2Vec2 center = b->GetWorldCenter();
  for (b2ContactEdge *ce = b->GetContactList(); ce; ce = ce->next) {
    b2Contact *c = ce->contact;
    if (!c->IsTouching()) continue;
    b2WorldManifold wm;
    c->GetWorldManifold(&wm);
    // contact_point_force da la fuerza sobre B; sobre A es la opuesta.
    double sgn = (c->GetFixtureB()->GetBody() == b) ? 1.0 : -1.0;
    for (int i = 0; i < c->GetManifold()->pointCount; ++i) {
      ContactPointForce cpf = contact_point_force(c, wm, i, inv_dt);
      b2Vec2 l = cpf.point - center;
      fx += sgn * cpf.fx;
      fy += sgn * cpf.fy;
      tz += sgn * (l.x * cpf.fy - l.y * cpf.fx);
    }
  }
  *F = b2Vec2(static_cast<float>(fx), static_cast<float>(fy));
  *tau = tz;
}

b2Vec2 wall_force(b2World *w, double dt, int wall_gid) {
  double fx = 0.0, fy = 0.0;
  const double inv_dt = 1.0 / dt;
  for (b2Contact *c = w->GetContactList(); c; c = c->GetNext()) {
    if (!c->IsTouching()) continue;
    bool a_is_wall = body_data(c->GetFixtureA()->GetBody())->gid == wall_gid;
    bool b_is_wall = body_data(c->GetFixtureB()->GetBody())->gid == wall_gid;
    if (!a_is_wall && !b_is_wall) continue;
    b2WorldManifold wm;
    c->GetWorldManifold(&wm);
    // contact_point_force da la fuerza sobre B; sobre A es la opuesta.
    double sgn = a_is_wall ? -1.0 : 1.0;
    for (int i = 0; i < c->GetManifold()->pointCount; ++i) {
      ContactPointForce cpf = contact_point_force(c, wm, i, inv_dt);
      fx += sgn * cpf.fx;
      fy += sgn * cpf.fy;
    }
  }
  return b2Vec2(static_cast<float>(fx), static_cast<float>(fy));
}

float grain_area(const b2Body *body) {
  if (!body_data(body)->is_grain) return 1.0f; // área no aplicable a paredes
  float radio = body->GetFixtureList()->GetShape()->m_radius;
  float area = M_PI * radio * radio;
  return area;
}
