/*! \file test_reinjection.cpp
 * \brief Prueba de reinject_grains (discharge.hpp): los granos reinyectados
 * no se superponen con otros cuerpos, quedan con velocidad nula y dentro de
 * la franja de reinyección.
 *
 * Uso: `test_reinjection archivo_de_parámetros` (H = 30, R = 10).
 */

#include "body_data.hpp"
#include "discharge.hpp"
#include "global_setup.hpp"
#include "rng.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::printf("Uso: %s <archivo_de_parámetros>\n", argv[0]);
    return 2;
  }
  const GlobalSetup gs(argv[1]);
  RNG rng(42);
  b2World world(b2Vec2(0, 0));
  std::vector<BodyData> data(400); // no se redimensiona: punteros estables
  int n = 0;
  auto make = [&](float x, float y, bool in) {
    b2BodyDef bd;
    bd.type = b2_dynamicBody;
    bd.position.Set(x, y);
    data[n].is_grain = true;
    data[n].is_in = in;
    data[n].gid = n;
    bd.userData.pointer = reinterpret_cast<uintptr_t>(&data[n]);
    ++n;
    b2Body *b = world.CreateBody(&bd);
    b2CircleShape c;
    c.m_radius = 0.5f;
    b2FixtureDef fd;
    fd.shape = &c;
    fd.density = 1;
    b->CreateFixture(&fd);
    return b;
  };

  // Franja de reinyección: |x| <= 0.9 R, 0.75 H <= y <= 0.95 H.
  const float x_max = 0.9 * gs.silo.R;
  const float y_min = 0.75 * gs.silo.H, y_max = 0.95 * gs.silo.H;
  // Se la puebla con 50 granos al azar, sin superposición.
  int placed = 0;
  for (int k = 0; k < 20000 && placed < 50; ++k) {
    float x = rng.get_double(-x_max - 0.5, x_max + 0.5);
    float y = rng.get_double(y_min - 0.5, y_max + 0.5);
    bool free = true;
    for (b2Body *b = world.GetBodyList(); b; b = b->GetNext())
      if ((b->GetPosition() - b2Vec2(x, y)).Length() < 1.0f) free = false;
    if (free) {
      make(x, y, true);
      ++placed;
    }
  }

  int fails = 0, reinjected = 0;
  for (int trial = 0; trial < 200; ++trial) {
    b2Body *g = make(0.0f, -11.0f, false); // grano descargado
    g->SetLinearVelocity(b2Vec2(0.3f, -2.0f));
    g->SetAngularVelocity(5.0f);
    world.Step(1e-6f, 1, 1); // actualiza el broadphase
    reinject_grains(&world, gs, rng, true);
    if (!body_data(g)->is_in) continue; // sin lugar: se reintenta luego
    ++reinjected;
    float d_min = 1e9f;
    for (b2Body *b = world.GetBodyList(); b; b = b->GetNext())
      if (b != g)
        d_min = std::min(d_min, (b->GetPosition() - g->GetPosition()).Length());
    b2Vec2 p = g->GetPosition();
    bool ok = d_min >= 1.0f && g->GetLinearVelocity().Length() == 0.0f &&
              g->GetAngularVelocity() == 0.0f && p.y >= y_min && p.y <= y_max &&
              std::abs(p.x) <= x_max;
    if (!ok) {
      ++fails;
      std::printf("FALLA: d_min = %.4f, |v| = %.3f, w = %.3f, (x, y) = (%.2f, "
                  "%.2f)\n",
                  d_min, g->GetLinearVelocity().Length(),
                  g->GetAngularVelocity(), p.x, p.y);
    }
  }
  std::printf("reinyección: %d granos iniciales, %d reinyectados de 200, "
              "%d fallas\n",
              placed, reinjected, fails);
  if (reinjected == 0) ++fails;
  std::printf("%s\n", fails ? "HAY FALLAS" : "Todas las pruebas pasan");
  return fails ? 1 : 0;
}
