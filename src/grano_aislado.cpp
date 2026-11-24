/*! \file grano_aislado.cpp
 * \brief Movilidad de un disco aislado sobre la base vibrada.
 *
 * Un disco solo (m = 1, g = 1) con una fuerza constante F en la dirección
 * del eje y, sobre la base biarmónica de unav-walkers. Se integra con el
 * mismo esquema que Box2D (v += dt F / m; x += dt v) y el modelo de Karnopp
 * del simulador (karnopp, base_friction.hpp), y se compara con el modelo de
 * Coulomb regularizado que se usa en el análisis de los experimentos,
 *
 *     F_base = -mu_d m g tanh(v_rel / v_reg),   v_reg = 1e-4,
 *
 * integrado con un paso mucho menor. Devuelve la velocidad media de deriva
 * en función de F y la movilidad efectiva
 *
 *     tau_ef = m dv/dF  (F = 0)  =  m [v(+F0) - v(-F0)] / (2 F0),
 *
 * con F0 = 0.05 mu_d m g.
 *
 * Uso: grano_aislado [opciones]
 *   --gamma G --rho r --phi p --freq f   excitación (por defecto, los
 *                                        parámetros del vídeo)
 *   --mu_s a --mu_d b                    fricción con la base
 *   --dt h        paso de Karnopp (por defecto T / 500)
 *   --periodos N  períodos de promedio (por defecto 100; se descartan 20)
 *   --F f1,f2,..  fuerzas (por defecto -0.1,-0.05,0,0.05,0.1)
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#include "base_friction.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numbers>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Params {
  double gamma = 9.9 / 9.81, rho = 0.495, phi = 1.79, freq = 0.45175395;
  double mu_s = 0.28, mu_d = 0.28;
  double dt = 0.0; // 0: T / 500
  int periodos = 100;
  std::vector<double> F = {-0.1, -0.05, 0.0, 0.05, 0.1};
};

constexpr double kMasa = 1.0, kG = 1.0, kTransitorio = 20.0;

/// Velocidad media (laboratorio) con el modelo de Karnopp del simulador.
double deriva_karnopp(const Params &p, double F) {
  const double T = 1.0 / p.freq, w = 2.0 * std::numbers::pi * p.freq;
  const double dt = p.dt > 0 ? p.dt : T / 500.0;
  const long n_per = std::lround(T / dt);
  double v = 0.0, suma = 0.0;
  long n = 0;
  const long total = static_cast<long>((kTransitorio + p.periodos) * n_per);
  for (long k = 0; k < total; ++k) {
    const double t = k * dt;
    const MovBase b = base_excitation(t, p.gamma, w, p.rho, p.phi, kG);
    // La base se mueve en -y (convención del simulador)
    const b2Vec2 v_rel(0.0f, static_cast<float>(v + b.v));
    const b2Vec2 a_base(0.0f, static_cast<float>(-b.a));
    const b2Vec2 f = karnopp(v_rel, b2Vec2(0.0f, static_cast<float>(F)),
                             a_base, kMasa, dt, 1.0e-5, p.mu_s, p.mu_d,
                             kMasa * kG);
    v += dt * (F + f.y) / kMasa;
    if (k >= static_cast<long>(kTransitorio * n_per)) {
      suma += v;
      ++n;
    }
  }
  return suma / n;
}

/// Velocidad media con Coulomb regularizado (paso 25 veces menor).
double deriva_coulomb(const Params &p, double F) {
  const double T = 1.0 / p.freq, w = 2.0 * std::numbers::pi * p.freq;
  const double dt = T / 20000.0, v_reg = 1.0e-4;
  const long n_per = std::lround(T / dt);
  double v = 0.0, suma = 0.0;
  long n = 0;
  const long total = static_cast<long>((kTransitorio + p.periodos) * n_per);
  for (long k = 0; k < total; ++k) {
    const double t = k * dt;
    const MovBase b = base_excitation(t, p.gamma, w, p.rho, p.phi, kG);
    const double v_rel = v + b.v;
    v += dt * (F - p.mu_d * kMasa * kG * std::tanh(v_rel / v_reg)) / kMasa;
    if (k >= static_cast<long>(kTransitorio * n_per)) {
      suma += v;
      ++n;
    }
  }
  return suma / n;
}

} // namespace

int main(int argc, char *argv[]) {
  Params p;
  bool mu_s_dado = false;
  for (int i = 1; i < argc; ++i) {
    auto arg = [&](const char *name) {
      return !std::strcmp(argv[i], name) && i + 1 < argc;
    };
    if (arg("--gamma")) p.gamma = std::atof(argv[++i]);
    else if (arg("--rho")) p.rho = std::atof(argv[++i]);
    else if (arg("--phi")) p.phi = std::atof(argv[++i]);
    else if (arg("--freq")) p.freq = std::atof(argv[++i]);
    else if (arg("--mu_d")) p.mu_d = std::atof(argv[++i]);
    else if (arg("--mu_s")) { p.mu_s = std::atof(argv[++i]); mu_s_dado = true; }
    else if (arg("--dt")) p.dt = std::atof(argv[++i]);
    else if (arg("--periodos")) p.periodos = std::atoi(argv[++i]);
    else if (arg("--F")) {
      p.F.clear();
      std::stringstream ss(argv[++i]);
      for (std::string item; std::getline(ss, item, ',');)
        p.F.push_back(std::atof(item.c_str()));
    } else {
      std::fprintf(stderr, "Uso: %s [--gamma G] [--rho r] [--phi p] [--freq f] "
                   "[--mu_s a] [--mu_d b] [--dt h] [--periodos N] "
                   "[--F f1,f2,...]\n", argv[0]);
      return 1;
    }
  }
  if (!mu_s_dado) p.mu_s = p.mu_d; // sin --mu_s, igual a mu_d
  if (p.mu_s < p.mu_d || p.periodos < 1) {
    std::fprintf(stderr, "Se requiere mu_s >= mu_d y periodos >= 1\n");
    return 1;
  }
  std::printf("# Grano aislado: Gamma = %g rho = %g phi = %g f = %g "
              "mu_s = %g mu_d = %g dt = %g (T/%g)\n", p.gamma, p.rho, p.phi,
              p.freq, p.mu_s, p.mu_d,
              p.dt > 0 ? p.dt : 1 / p.freq / 500.0,
              p.dt > 0 ? 1 / p.freq / p.dt : 500.0);
  std::printf("# F  v_karnopp  v_coulomb  (velocidad media de deriva, "
              "sqrt(g d), + = hacia +y)\n");
  for (double F : p.F)
    std::printf("%+.5f %+.6f %+.6f\n", F, deriva_karnopp(p, F),
                deriva_coulomb(p, F));
  const double F0 = 0.05 * p.mu_d * kMasa * kG;
  const double tau_k = kMasa * (deriva_karnopp(p, F0) - deriva_karnopp(p, -F0)) / (2 * F0);
  const double tau_c = kMasa * (deriva_coulomb(p, F0) - deriva_coulomb(p, -F0)) / (2 * F0);
  std::printf("# tau_ef (F0 = %g): Karnopp %.5f  Coulomb %.5f  (tiempo "
              "reducido sqrt(d/g))\n", F0, tau_k, tau_c);
  return 0;
}
