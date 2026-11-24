/*! \file test_walls.cpp
 * \brief Prueba de las paredes móviles (move_walls, silo_builder.hpp).
 *
 * 1. Con paredes_moviles, el silo y la tapa siguen la posición de la base,
 *    y_base = -MovBase::x, con error < 1e-5 durante tres períodos.
 * 2. Un disco apoyado sobre la tapa, que sube a la velocidad de la base, es
 *    empujado: durante los primeros pasos tiene la velocidad de la pared.
 * 3. Sin paredes_moviles los cuerpos son estáticos y no se mueven.
 *
 * Uso: `test_walls archivo_de_parámetros` (H = 30, R = 10, r = 2.5).
 */

#include "base_friction.hpp"
#include "body_data.hpp"
#include "global_setup.hpp"
#include "rng.hpp"
#include "silo_builder.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>

namespace {

int fails = 0;

void expect_true(const char *what, bool cond) {
  if (!cond) ++fails;
  std::printf("%-62s %s\n", what, cond ? "OK" : "FALLA");
}

} // namespace

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::printf("Uso: %s <archivo_de_parámetros>\n", argv[0]);
    return 2;
  }
  GlobalSetup gs(argv[1]);
  gs.granos[0].n_granos = 0; // solo las paredes
  const double dt = gs.dt;
  const double omega = 2 * b2_pi * gs.silo.frec;

  for (bool moviles : {false, true}) {
    gs.paredes_moviles = moviles;
    RNG rng(1);
    SiloBodies bodies;
    b2World world(b2Vec2(0, 0));
    build_silo(&world, gs, rng, bodies);

    // Un disco apoyado sobre la tapa (|x| <= r, y = 0)
    BodyData disc;
    disc.is_grain = true;
    disc.is_in = true;
    b2BodyDef def;
    def.type = b2_dynamicBody;
    def.position.Set(0.0f, 0.5f);
    def.userData.pointer = reinterpret_cast<uintptr_t>(&disc);
    b2Body *grain = world.CreateBody(&def);
    b2CircleShape c;
    c.m_radius = 0.5f;
    b2FixtureDef fd;
    fd.shape = &c;
    fd.density = 1.0f;
    fd.restitution = 0.0f;
    grain->CreateFixture(&fd);

    double max_err = 0.0, v_wall_10 = 0.0, v_grain_10 = 0.0;
    const int n_steps = static_cast<int>(3.0 / gs.silo.frec / dt);
    for (int k = 0; k < n_steps; ++k) {
      const double t = k * dt;
      move_walls(bodies, -base_excitation(t + dt, gs.silo.gamma, omega, gs).x,
                 dt);
      if (k == 10) v_wall_10 = bodies.lid->GetLinearVelocity().y;
      world.Step(static_cast<float>(dt), gs.p_iter, gs.v_iter);
      if (k == 10) v_grain_10 = grain->GetLinearVelocity().y;
      const double y_base =
          -base_excitation(t + dt, gs.silo.gamma, omega, gs).x;
      max_err = std::max(
          {max_err, std::fabs(bodies.silo->GetPosition().y - y_base),
           std::fabs(bodies.lid->GetPosition().y - y_base)});
    }
    std::printf("--- paredes_moviles = %s\n", moviles ? "T" : "F");
    if (moviles) {
      expect_true("las paredes siguen a la base (error < 1e-5)", max_err < 1e-5);
      expect_true("el disco sobre la tapa recibe la velocidad de la pared",
                  std::fabs(v_grain_10 - v_wall_10) < 0.02 && v_wall_10 > 0.05);
      expect_true("el silo es cinemático",
                  bodies.silo->GetType() == b2_kinematicBody);
    } else {
      expect_true("las paredes quedan quietas", max_err > 0.0 &&
                      bodies.silo->GetPosition().y == 0.0f);
      expect_true("el silo es estático",
                  bodies.silo->GetType() == b2_staticBody);
    }
  }
  std::printf("%s\n", fails ? "FALLAS" : "Todas las pruebas pasan");
  return fails ? 1 : 0;
}
