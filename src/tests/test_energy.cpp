/*! \file test_energy.cpp
 * \brief Pruebas de EnergyBudget (energy_budget.hpp).
 *
 * 1. Un disco solo sobre la base vibrada, deslizando y girando: sin
 *    contactos, el cambio de energía cinética es exactamente el trabajo de la
 *    fricción (W_cont = 0); la disipación contra la base es positiva, y el
 *    pivoteo disipa toda la energía de rotación inicial.
 * 2. Dos discos que chocan de frente, sin fricción con la base: todo el
 *    cambio de energía es trabajo de los contactos, y con restitución e la
 *    pérdida es (1 - e^2) por la energía cinética relativa.
 *
 * Discos de radio 0.5 y masa 1 (densidad 4 / pi), dt = 0.005.
 */

#include "base_friction.hpp"
#include "body_data.hpp"
#include "energy_budget.hpp"

#include <cmath>
#include <cstdio>
#include <numbers>
#include <vector>

namespace {

int fails = 0;

void expect(const char *what, double got, double expected, double tol) {
  const bool ok = std::fabs(got - expected) < tol;
  if (!ok) ++fails;
  std::printf("%-58s %+.6e (esperado %+.6e) %s\n", what, got, expected,
              ok ? "OK" : "FALLA");
}

void expect_true(const char *what, bool cond) {
  if (!cond) ++fails;
  std::printf("%-58s %s\n", what, cond ? "OK" : "FALLA");
}

b2Body *make_disc(b2World &world, BodyData &bd, b2Vec2 pos, b2Vec2 v, float w,
                  double mu_s, double mu_d, float rest) {
  bd.is_grain = true;
  bd.is_in = true;
  bd.fric_s = mu_s;
  bd.fric_d = mu_d;
  b2BodyDef def;
  def.type = b2_dynamicBody;
  def.position = pos;
  def.linearVelocity = v;
  def.angularVelocity = w;
  def.userData.pointer = reinterpret_cast<uintptr_t>(&bd);
  b2Body *b = world.CreateBody(&def);
  b2CircleShape c;
  c.m_radius = 0.5f;
  b2FixtureDef fd;
  fd.shape = &c;
  fd.density = static_cast<float>(4.0 / std::numbers::pi); // m = 1
  fd.restitution = rest;
  b->CreateFixture(&fd);
  return b;
}

double kinetic(const b2Body *b) {
  const b2Vec2 v = b->GetLinearVelocity();
  const double w = b->GetAngularVelocity();
  return 0.5 * b->GetMass() * (double(v.x) * v.x + double(v.y) * v.y) +
         0.5 * b->GetInertia() * w * w;
}

void test_single_grain() {
  b2World world(b2Vec2(0.0f, 0.0f));
  BodyData bd;
  const float w0 = 4.0f;
  b2Body *g = make_disc(world, bd, {0, 0}, {0.3f, 0.1f}, w0, 0.2, 0.15, 0.5f);
  const double dt = 0.005, f = 0.45, omega = 2 * std::numbers::pi * f;
  const double k0 = kinetic(g);
  EnergyBudget energy(f);
  EnergyWork tot;
  for (int n = 0; n < 4000; ++n) {
    const double t = n * dt;
    // Base armónica con amplitud de velocidad 0.5 (se mueve con -y)
    const double vb = 0.5 * std::cos(omega * t);
    const double ab = -0.5 * omega * std::sin(omega * t);
    apply_base_friction(&world, vb, ab, 1e-4, 1.0, dt);
    energy.before_step(&world, vb);
    world.Step(static_cast<float>(dt), 8, 3);
    tot.add(energy.after_step(&world, t, dt));
    world.ClearForces();
  }
  const double scale = std::fabs(tot.w_base) + std::fabs(tot.w_lin) +
                       std::fabs(tot.w_piv) + std::fabs(tot.dk);
  std::printf("grano solo: Wb = %.4f, Wlin = %.4f, Wpiv = %.4f, dK = %.4f\n",
              tot.w_base, tot.w_lin, tot.w_piv, tot.dk);
  expect("grano solo: W_cont / escala = 0 (sin contactos)",
         tot.w_cont() / scale, 0.0, 1e-12);
  expect("grano solo: suma de dK = K(final) - K(inicial)", tot.dk,
         kinetic(g) - k0, 1e-6 * scale);
  expect_true("grano solo: D_slide = Wb - Wlin > 0",
              tot.w_base - tot.w_lin > 0);
  // El pivoteo frena el giro: disipa toda la energía de rotación inicial
  expect("grano solo: Wpiv = -I w0^2 / 2 (giro detenido)", tot.w_piv,
         -0.5 * g->GetInertia() * w0 * w0, 1e-4);
  expect("grano solo: w final = 0", g->GetAngularVelocity(), 0.0, 1e-6);
}

void test_collision() {
  b2World world(b2Vec2(0.0f, 0.0f));
  std::vector<BodyData> bd(2); // sin redimensionar: punteros estables
  const float e = 0.5f;
  b2Body *a = make_disc(world, bd[0], {0, 0}, {1, 0}, 0, 0.0, 0.0, e);
  b2Body *b = make_disc(world, bd[1], {1.2f, 0}, {-1, 0}, 0, 0.0, 0.0, e);
  const double dt = 0.005;
  const double k0 = kinetic(a) + kinetic(b);
  EnergyBudget energy(0.45);
  EnergyWork tot;
  for (int n = 0; n < 200; ++n) {
    apply_base_friction(&world, 0.0, 0.0, 1e-4, 1.0, dt); // mu = 0: F = 0
    energy.before_step(&world, 0.0);
    world.Step(static_cast<float>(dt), 8, 3);
    tot.add(energy.after_step(&world, n * dt, dt));
    world.ClearForces();
  }
  // Velocidad relativa 2 -> 2 e: K pasa de 1 a e^2
  const double expected = -(1.0 - e * e) * k0;
  std::printf("choque: v_A = %.4f, v_B = %.4f\n", a->GetLinearVelocity().x,
              b->GetLinearVelocity().x);
  expect("choque: Wlin + Wpiv = 0 (sin fricción con la base)",
         tot.w_lin + tot.w_piv, 0.0, 1e-12);
  expect("choque: W_cont = -(1 - e^2) K0", tot.w_cont(), expected, 1e-4);
  expect("choque: W_cont = dK", tot.w_cont(), tot.dk, 1e-12);
}

} // namespace

int main() {
  test_single_grain();
  test_collision();
  std::printf("%s\n", fails ? "HAY FALLAS" : "Todas las pruebas pasan");
  return fails ? 1 : 0;
}
