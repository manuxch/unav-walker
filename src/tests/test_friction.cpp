/*! \file test_friction.cpp
 * \brief Pruebas de karnopp y pivot_friction (base_friction.hpp).
 *
 * Disco de radio R = 0.5 y masa m = 1 (I = m R^2 / 2), N = m g = 1,
 * mu_s = 0.2, mu_d = 0.15, dt = 0.005.
 */

#include "base_friction.hpp"

#include <cmath>
#include <cstdio>

namespace {

int fails = 0;

void expect(const char *what, double got, double expected, double tol = 1e-6) {
  bool ok = std::fabs(got - expected) < tol;
  if (!ok) ++fails;
  std::printf("%-62s %+.6f (esperado %+.6f) %s\n", what, got, expected,
              ok ? "OK" : "FALLA");
}

void test_karnopp() {
  const double m = 1.0, dt = 0.005, v_tol = 1e-4, mu_s = 0.2, mu_d = 0.15,
               N = 1.0;
  // Banda de adherencia: max(1e-4, mu_d N dt / m) = 7.5e-4
  auto F = [&](b2Vec2 v, b2Vec2 f_ext, b2Vec2 a) {
    return karnopp(v, f_ext, a, m, dt, v_tol, mu_s, mu_d, N);
  };
  expect("karnopp: adherido sin fuerzas -> 0",
         F({0, 0}, {0, 0}, {0, 0}).Length(), 0);
  expect("karnopp: adherido, base acelera 0.1 -> F_y = m a",
         F({0, 0}, {0, 0}, {0, 0.1f}).y, 0.1);
  expect("karnopp: adherido, F_ext = (0.1, 0) -> F_x = -0.1",
         F({0, 0}, {0.1f, 0}, {0, 0}).x, -0.1);
  expect("karnopp: ruptura, base acelera 1 -> F_y = mu_s N",
         F({0, 0}, {0, 0}, {0, 1}).y, 0.2);
  expect("karnopp: v_rel = 5e-4 en la banda -> F_x = -m v / dt",
         F({5e-4f, 0}, {0, 0}, {0, 0}).x, -0.1);
  expect("karnopp: deslizando v_rel = (0, -1) -> F_y = +mu_d N",
         F({0, -1}, {0, 0}, {0, 0}).y, 0.15);
  b2Vec2 f = F({3, 4}, {0, 0}, {0, 0});
  expect("karnopp: deslizando v_rel = (3, 4) -> F_x = -mu_d N 3/5", f.x, -0.09);
  expect("karnopp: deslizando v_rel = (3, 4) -> F_y = -mu_d N 4/5", f.y, -0.12);
}

void test_pivot() {
  const double R = 0.5, I = 0.125, dt = 0.005, v_tol = 1e-5, mu_s = 0.2,
               mu_d = 0.15, N = 1.0;
  const double arm = 2.0 / 3.0 * R;
  auto tau = [&](double w, double tau_ext) {
    return pivot_friction(w, tau_ext, I, R, dt, v_tol, mu_s, mu_d, N);
  };
  expect("pivot: quieto sin torques -> 0", tau(0, 0), 0);
  expect("pivot: quieto, tau_ext = 0.03 -> -tau_ext", tau(0, 0.03), -0.03);
  expect("pivot: quieto, tau_ext = 0.1 -> -(2/3) mu_s N R", tau(0, 0.1),
         -mu_s * N * arm);
  expect("pivot: w = 1e-3 en la banda -> -I w / dt", tau(1e-3, 0),
         -I * 1e-3 / dt);
  expect("pivot: girando w = +2 -> -(2/3) mu_d N R", tau(2, 0),
         -mu_d * N * arm);
  expect("pivot: girando w = -2 -> +(2/3) mu_d N R", tau(-2, 0),
         mu_d * N * arm);
  // Independencia de dt: desde w = 1 el disco frena en t = I / (mu_d N arm)
  const double t_exact = I / (mu_d * N * arm);
  for (double h : {0.005, 0.001, 0.0002}) {
    double w = 1.0, t = 0.0;
    while (w != 0.0 && t < 10.0) {
      w += h * pivot_friction(w, 0, I, R, h, v_tol, mu_s, mu_d, N) / I;
      t += h;
    }
    char msg[80];
    std::snprintf(msg, sizeof msg, "pivot: tiempo de frenado con dt = %g", h);
    expect(msg, t, t_exact, 2 * h);
  }
}

} // namespace

int main() {
  test_karnopp();
  test_pivot();
  std::printf("%s\n", fails ? "HAY FALLAS" : "Todas las pruebas pasan");
  return fails ? 1 : 0;
}
