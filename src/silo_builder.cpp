/*! \file silo_builder.cpp
 * \brief Construcción del sistema: paredes del silo, tapa o fondo, y granos.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#include "silo_builder.hpp"

#include <algorithm>
#include <iostream>

using std::cout;
using std::endl;

namespace {

/*! Paredes del silo: una cadena de segmentos en cada orientación, porque las
 * cadenas de Box2D colisionan de un solo lado. */
void create_walls(b2World *world, const GlobalSetup &gs, BodyData *data) {
  b2BodyDef bd;
  bd.position.Set(0.0f, 0.0f);
  bd.type = b2_staticBody;
  data->is_grain = false;
  data->gid = kGidSilo;
  bd.userData.pointer = reinterpret_cast<uintptr_t>(data);
  b2Body *silo = world->CreateBody(&bd);
  const Contenedor &s = gs.silo;
  std::vector<b2Vec2> wall_poly;
  if (gs.fondo_medicion) { // U sin fondo
    wall_poly = {b2Vec2(-s.R, 0.0f), b2Vec2(-s.R, s.H), b2Vec2(s.R, s.H),
                 b2Vec2(s.R, 0.0f)};
  } else { // Con el orificio [-r, r] abierto
    wall_poly = {b2Vec2(-s.r, 0.0f), b2Vec2(-s.R, 0.0f), b2Vec2(-s.R, s.H),
                 b2Vec2(s.R, s.H),   b2Vec2(s.R, 0.0f),  b2Vec2(s.r, 0.0f)};
  }
  const int n_wall = static_cast<int>(wall_poly.size());
  for (int side = 0; side < 2; ++side) {
    std::vector<b2Vec2> verts(wall_poly);
    if (side == 1) std::reverse(verts.begin(), verts.end());
    b2ChainShape chain;
    chain.CreateChain(verts.data(), n_wall, verts.front(), verts.back());
    b2FixtureDef fix;
    fix.shape = &chain;
    fix.density = 0.0f;
    fix.friction = s.fric;
    fix.restitution = s.rest;
    silo->CreateFixture(&fix);
  }
  cout << "#\t- Silo creado." << endl;
}

/*! Tapa del orificio (gid = kGidLid) o fondo de medición de ancho completo
 * (gid = kGidMeasuringFloor), como un segmento de dos lados en y = 0. */
b2Body *create_lid(b2World *world, const GlobalSetup &gs, BodyData *data) {
  b2BodyDef bd;
  bd.position.Set(0.0f, 0.0f);
  bd.type = b2_staticBody;
  data->is_grain = false;
  data->gid = gs.fondo_medicion ? kGidMeasuringFloor : kGidLid;
  bd.userData.pointer = reinterpret_cast<uintptr_t>(data);
  b2Body *lid = world->CreateBody(&bd);
  const float x_lid =
      static_cast<float>(gs.fondo_medicion ? gs.silo.R : gs.silo.r);
  b2EdgeShape edge;
  edge.SetTwoSided(b2Vec2(-x_lid, 0.0f), b2Vec2(x_lid, 0.0f));
  b2FixtureDef fix;
  fix.shape = &edge;
  fix.density = 0.0f;
  fix.friction = gs.silo.fric;
  fix.restitution = gs.silo.rest;
  lid->CreateFixture(&fix);
  if (gs.fondo_medicion)
    cout << "#\t- Fondo de medición creado (gID=-200)." << endl;
  else
    cout << "#\t- Tapa del orificio creada (gID=-110)." << endl;
  return lid;
}

/*! Granos (discos) en posiciones y ángulos al azar dentro del silo. */
void create_grains(b2World *world, const GlobalSetup &gs, RNG &rng,
                   SiloBodies &bodies) {
  // Región de inserción, con márgenes respecto de las paredes. Se conservan
  // los tipos float del código original para reproducir las posiciones.
  float max_radio = 0.0f;
  for (const TipoGrano &tg : gs.granos) {
    if (tg.radio > max_radio) max_radio = tg.radio;
  }
  const float y_inf = 2.7f * max_radio;
  const float y_sup = gs.silo.H - 2.1f * max_radio;
  const float x_izq = -gs.silo.R + 2.1f * max_radio;
  const float x_der = gs.silo.R - 2.1f * max_radio;

  int next_gid = 0;
  cout << "#\t- Insertando granos..." << endl;
  bodies.grain_data.resize(gs.granos.size());
  for (size_t i = 0; i < gs.granos.size(); i++) {
    const TipoGrano &tg = gs.granos[i];
    std::vector<BodyData> &data = bodies.grain_data[i];
    data.resize(tg.n_granos); // no se redimensiona después: punteros estables
    for (int j = 0; j < tg.n_granos; j++) {
      float x = rng.get_double(x_izq, x_der);
      float y = rng.get_double(y_inf, y_sup);
      data[j].tipo = static_cast<int>(i);
      data[j].is_grain = true;
      data[j].is_in = true;
      data[j].fric_d = tg.fric_d;
      data[j].fric_s = tg.fric_s;
      data[j].gid = next_gid++;
      b2BodyDef bd;
      bd.type = b2_dynamicBody;
      bd.allowSleep = true;
      bd.bullet = gs.continuous_physics;
      bd.position.Set(x, y);
      bd.angle = rng.get_double(-b2_pi, b2_pi);
      bd.userData.pointer = reinterpret_cast<uintptr_t>(&data[j]);
      b2Body *grain = world->CreateBody(&bd);
      b2CircleShape circle;
      circle.m_radius = tg.radio;
      b2FixtureDef fix;
      fix.shape = &circle;
      fix.density = tg.dens;
      fix.friction = tg.fric;
      fix.restitution = tg.rest;
      grain->CreateFixture(&fix);
      bodies.total_grain_mass += grain->GetMass();
      if (j == 0) {
        cout << "#\t- Grano de tipo " << i << " creado con masa "
             << grain->GetMass() << " kg." << endl;
      }
    }
  }
  cout << "#\t- Insersión de granos finalizada." << endl;
  cout << "#\t- Masa total de granos = " << bodies.total_grain_mass << " kg."
       << endl;
}

} // namespace

void build_silo(b2World *world, const GlobalSetup &gs, RNG &rng,
                SiloBodies &bodies) {
  create_walls(world, gs, &bodies.silo_data);
  bodies.lid = create_lid(world, gs, &bodies.lid_data);
  create_grains(world, gs, rng, bodies);
}
