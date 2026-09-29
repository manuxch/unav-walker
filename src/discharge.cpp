/*! \file discharge.cpp
 * \brief Descarga por el orificio: conteo, reinyección y medidas en la salida.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#include "discharge.hpp"

#include "body_data.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>

int count_discharged(b2World *w, std::vector<int> &suma_tipo, int paso,
                     std::ofstream &flux_file, const GlobalSetup &gs) {
  int total_desc = 0; // Granos descargados desde el comienzo
  for (int n : suma_tipo)
    total_desc += n;
  const double y_min = 0.0;
  int n_desc = 0; // Granos descargados en esta llamada
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    BodyData *bd = body_data(b);
    if (!bd->is_grain || !bd->is_in) continue;
    b2Vec2 p = b->GetPosition();
    double radio = b->GetFixtureList()->GetShape()->m_radius;
    if (p.y > y_min - radio) continue;
    bd->is_in = false;
    n_desc++;
    total_desc++;
    suma_tipo[bd->tipo]++;
    flux_file << total_desc << " " << bd->tipo << " ";
    flux_file << std::setprecision(8) << paso * gs.dt << " ";
    for (int n : suma_tipo)
      flux_file << n << " ";
    flux_file << total_desc << std::endl;
  }
  flux_file.flush();
  return n_desc;
}

namespace {

/*! Detecta si un disco de prueba se superpone con algún fixture del mundo. */
class OverlapQuery : public b2QueryCallback {
public:
  b2CircleShape probe;
  b2Transform xf;
  const b2Body *self = nullptr;
  bool hit = false;
  bool ReportFixture(b2Fixture *f) override {
    if (f->GetBody() == self) return true;
    const b2Shape *s = f->GetShape();
    for (int32 ch = 0; ch < s->GetChildCount(); ++ch) {
      if (b2TestOverlap(&probe, 0, s, ch, xf, f->GetBody()->GetTransform())) {
        hit = true;
        return false;
      }
    }
    return true;
  }
};

/*! Busca al azar una posición libre para un disco de radio \p radius en la
 * franja de reinyección. */
bool find_free_spot(b2World *w, const b2Body *self, float radius,
                    const GlobalSetup &gs, RNG &rng, b2Vec2 *pos) {
  const int max_tries = 100;
  const float gap = 1.02f; // pequeña separación para evitar contactos iniciales
  for (int k = 0; k < max_tries; ++k) {
    b2Vec2 p(
        static_cast<float>(rng.get_double(-0.9 * gs.silo.R, 0.9 * gs.silo.R)),
        static_cast<float>(rng.get_double(0.75 * gs.silo.H, 0.95 * gs.silo.H)));
    OverlapQuery q;
    q.probe.m_radius = gap * radius;
    q.xf.Set(p, 0.0f);
    q.self = self;
    b2AABB aabb;
    aabb.lowerBound = p - b2Vec2(gap * radius, gap * radius);
    aabb.upperBound = p + b2Vec2(gap * radius, gap * radius);
    w->QueryAABB(&q, aabb);
    if (!q.hit) {
      *pos = p;
      return true;
    }
  }
  return false;
}

} // namespace

void reinject_grains(b2World *w, const GlobalSetup &gs, RNG &rng,
                     bool reinject) {
  const double y_elim = -10.0; // altura por debajo de la cual se reinyecta
  static unsigned long n_fail = 0;

  b2Body *b = w->GetBodyList();
  while (b) {
    // Guardar el siguiente cuerpo ANTES de una posible eliminación
    b2Body *next_body = b->GetNext();
    BodyData *bd = body_data(b);
    if (b->GetType() != b2_dynamicBody || bd->is_in) {
      b = next_body;
      continue;
    }

    b2Vec2 pos = b->GetPosition();
    if (std::isnan(pos.x) || std::isnan(pos.y)) {
      std::cout << "ERROR: Grano " << bd->gid
                << " tiene posición inválida (NaN)" << std::endl;
      w->DestroyBody(b);
      b = next_body;
      continue;
    }
    if (pos.y > y_elim) {
      b = next_body;
      continue;
    }

    if (reinject) {
      b2Vec2 new_pos;
      float radius = b->GetFixtureList()->GetShape()->m_radius;
      if (find_free_spot(w, b, radius, gs, rng, &new_pos)) {
        b->SetTransform(new_pos, b->GetAngle());
        b->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
        b->SetAngularVelocity(0.0f);
        b->SetAwake(true);
        bd->is_in = true;
      } else if (++n_fail % 1000 == 1) {
        std::cout << "# AVISO: sin lugar libre para reinyectar (fallos "
                     "acumulados: "
                  << n_fail << "); se reintenta en el paso siguiente."
                  << std::endl;
      }
    } else {
      w->DestroyBody(b);
    }
    b = next_body;
  }
}

void save_packing_fraction(b2World *w, const GlobalSetup &gs, double t,
                           std::ofstream &fout) {
  const double y_inf = gs.silo.R;
  const double y_sup = 2 * gs.silo.R;
  double pf_out = 0.0;
  double pf_bulk = 0.0;
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    if (b->GetType() != b2_dynamicBody) continue;
    b2Vec2 pos = b->GetPosition();
    double radio = b->GetFixtureList()->GetShape()->m_radius;
    if (pos.y - radio > y_sup) continue; // Arriba de y_sup
    if (pos.y + radio > y_inf) {         // Intersecta la franja del bulk
      pf_bulk += clipped_circle_area(y_inf, y_sup, pos.y, radio);
    }
    if (std::abs(pos.x) > gs.silo.r) continue; // Centro fuera del orificio
    if (std::abs(pos.y) > radio) continue;     // No corta la recta y = 0
    pf_out += 2.0 * std::sqrt(radio * radio - pos.y * pos.y);
  }
  pf_out /= 2.0 * gs.silo.r;
  pf_bulk /= 2.0 * gs.silo.R * (y_sup - y_inf);
  fout << t << " " << pf_bulk << " " << pf_out << std::endl;
}

double clipped_circle_area(double y_inf, double y_sup, double y, double r) {
  constexpr double pi = std::numbers::pi;
  double h, a;
  if (std::abs(y - y_sup) < r) {
    h = r - std::abs(y - y_sup);
  } else if (std::abs(y - y_inf) < r) {
    h = r - std::abs(y - y_inf);
  } else {
    a = pi * r * r;
    return a;
  }
  // Área del segmento circular de altura h
  a = r * r * std::acos(1.0 - h / r) -
      (r - h) * std::sqrt(r * r - (r - h) * (r - h));
  if (y > y_sup || y < y_inf) {
    return a;
  }
  return pi * r * r - a;
}

void update_outlet_profiles(b2World *w, double *vel_0, size_t *pf_0,
                            size_t *bin_count, int n_bins, double r_out) {
  const double delta_r = 2.0 * r_out / n_bins;
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    if (b->GetType() != b2_dynamicBody) continue;
    b2Vec2 pos = b->GetPosition();
    if (std::abs(pos.x) > r_out) continue;
    double radio = b->GetFixtureList()->GetShape()->m_radius;
    if (std::abs(pos.y) > radio) continue;
    // Cuerda del disco sobre la recta y = 0
    double half_chord = std::sqrt(radio * radio - pos.y * pos.y);
    double x_inf = pos.x - half_chord;
    double x_sup = pos.x + half_chord;
    int i_inf = std::floor((x_inf + r_out) / delta_r);
    int i_sup = std::floor((x_sup + r_out) / delta_r);
    // Un grano superpuesto con el borde del orificio puede exceder el rango
    i_inf = std::max(i_inf, 0);
    i_sup = std::min(i_sup, n_bins - 1);
    b2Vec2 vel = b->GetLinearVelocity();
    for (int i = i_inf; i <= i_sup; ++i) {
      vel_0[i] += vel.y;
      pf_0[i] += 1;
      bin_count[i] += 1;
    }
  }
}
