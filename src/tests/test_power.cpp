/*! \file test_power.cpp
 * \brief Prueba de ContactPower (contact_power.hpp): cierre exacto de la
 * potencia de los contactos.
 *
 * Un silo con 250 granos que se depositan y vibran con la base de Karnopp,
 * con restitución y fricción grano-grano y con paredes fijas y móviles. En
 * cada paso se verifica que
 *
 *  - por grano, el cambio de energía cinética que producen los contactos
 *    (K(v'') - K(v')) coincide con sum_c J_c . u_c;
 *  - en total, W_cont = -(D_gg + D_gp) + W_par;
 *  - la disipación total D_gg + D_gp es positiva;
 *  - con paredes fijas, W_par = 0;
 *  - el trabajo de contacto de ContactPower coincide con el de EnergyBudget
 *    (dK - W_g).
 *
 * Uso: `test_power archivo_de_parámetros` (H = 30, R = 10, r = 2.5).
 */

#include "base_friction.hpp"
#include "body_data.hpp"
#include "contact_power.hpp"
#include "energy_budget.hpp"
#include "global_setup.hpp"
#include "rng.hpp"
#include "silo_builder.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

int fails = 0;

void expect_true(const char *what, bool cond) {
  if (!cond) ++fails;
  std::printf("%-66s %s\n", what, cond ? "OK" : "FALLA");
}

void run(GlobalSetup &gs, bool moviles) {
  gs.paredes_moviles = moviles;
  RNG rng(gs.rnd_seed);
  SiloBodies bodies;
  b2World world(b2Vec2(0, 0));
  build_silo(&world, gs, rng, bodies);
  const double dt = gs.dt, omega = 2 * b2_pi * gs.silo.frec;
  for (int k = 0; k < 20; ++k) world.Step(dt, gs.p_iter, gs.v_iter);
  // Velocidades y giros al azar: sin gravedad, los granos solo chocan si se
  // mueven
  for (b2Body *b = world.GetBodyList(); b; b = b->GetNext()) {
    if (b->GetType() != b2_dynamicBody) continue;
    b->SetLinearVelocity(b2Vec2(static_cast<float>(rng.get_double(-0.6, 0.6)),
                                static_cast<float>(rng.get_double(-0.6, 0.6))));
    b->SetAngularVelocity(static_cast<float>(rng.get_double(-1.0, 1.0)));
    // Un grano que duerme pierde su velocidad sin contactos (Box2D la anula)
    b->SetSleepingAllowed(false);
  }

  ContactPower power;
  EnergyBudget energy(gs.silo.frec);
  const int n_steps = 3000;
  long n_contacts = 0, n_neg_d = 0;
  double worst_grain = 0.0, worst_total = 0.0, worst_budget = 0.0;
  double sum_d = 0.0, sum_par = 0.0;
  for (int k = 0; k < n_steps; ++k) {
    const double t = k * dt;
    const MovBase base = base_excitation(t, gs.silo.gamma, omega, gs);
    if (gs.paredes_moviles)
      move_walls(bodies, -base_excitation(t + dt, gs.silo.gamma, omega, gs).x,
                 dt);
    apply_base_friction(&world, base.v, base.a, gs.silo.zero_tol, gs.g, dt);
    energy.before_step(&world, base.v);
    power.before_step(&world);
    world.Step(static_cast<float>(dt), gs.p_iter, gs.v_iter);
    const EnergyWork ew = energy.after_step(&world, t, dt);
    power.after_step(&world, dt);
    world.ClearForces();

    const PowerStep &p = power.total();
    // Escala: la energía cinética total, porque el solver trabaja en float
    const double scale = ew.k_sum + p.d_gg + p.d_gp + std::fabs(p.w_par);
    worst_grain = std::max(worst_grain, power.max_residual());
    worst_total = std::max(
        worst_total,
        std::fabs(p.w_cont() - (-(p.d_gg + p.d_gp) + p.w_par)) / scale);
    worst_budget =
        std::max(worst_budget, std::fabs(p.w_cont() - ew.w_cont()) / scale);
    sum_d += p.d_gg + p.d_gp;
    sum_par += p.w_par;
    for (const ContactRecord &r : power.records()) {
      ++n_contacts;
      if (r.d < 0) ++n_neg_d;
    }
  }
  std::printf("--- paredes_moviles = %s: %ld contactos, D total = %.4f, "
              "W_par = %.4f\n", moviles ? "T" : "F", n_contacts, sum_d,
              sum_par);
  std::printf("    residuo máximo por grano %.2e, de W_cont = -D + W_par "
              "%.2e, frente a EnergyBudget %.2e; D < 0 en %.1f %% de los "
              "contactos\n", worst_grain, worst_total, worst_budget,
              100.0 * n_neg_d / std::max(1L, n_contacts));
  expect_true("hubo contactos y disipación positiva",
              n_contacts > 1000 && sum_d > 0);
  // Los granos hacen float: el residuo por grano es el redondeo del solver
  // (60 iteraciones por paso)
  expect_true("por grano: K de contactos = sum J . u (residuo < 1e-5)",
              worst_grain < 1e-5);
  expect_true("en total: W_cont = -D + W_par (residuo < 1e-6)",
              worst_total < 1e-6);
  expect_true("W_cont de ContactPower = dK - W_g de EnergyBudget (< 1e-6)",
              worst_budget < 1e-6);
  if (!moviles)
    expect_true("paredes fijas: W_par = 0", std::fabs(sum_par) < 1e-12);
  else
    expect_true("paredes móviles: W_par != 0", std::fabs(sum_par) > 1e-6);
}

} // namespace

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::printf("Uso: %s <archivo_de_parámetros>\n", argv[0]);
    return 2;
  }
  GlobalSetup gs(argv[1]);
  run(gs, false);
  run(gs, true);
  std::printf("%s\n", fails ? "HAY FALLAS" : "Todas las pruebas pasan");
  return fails ? 1 : 0;
}
