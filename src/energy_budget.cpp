/*! \file energy_budget.cpp
 * \brief Balance de energía de la base vibrada.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#include "energy_budget.hpp"

#include "body_data.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numbers>
#include <sstream>

namespace {
constexpr double kTwoPi = 2.0 * std::numbers::pi;

/// Energía cinética de traslación.
double kinetic_lin(double m, b2Vec2 v) {
  return 0.5 * m * (double(v.x) * v.x + double(v.y) * v.y);
}

/// Energía cinética de rotación.
double kinetic_rot(double I, double w) { return 0.5 * I * w * w; }

/// Energía cinética de un grano.
double kinetic(const b2Body *b) {
  return kinetic_lin(b->GetMass(), b->GetLinearVelocity()) +
         kinetic_rot(b->GetInertia(), b->GetAngularVelocity());
}
} // namespace

void EnergyWork::add(const EnergyWork &o) {
  w_base += o.w_base;
  w_lin += o.w_lin;
  w_piv += o.w_piv;
  dk += o.dk;
  dk_reiny += o.dk_reiny;
  k_sum += o.k_sum;
  n_steps += o.n_steps;
  n_grains += o.n_grains;
}

EnergyBudget::EnergyBudget(double freq, int phase_bins)
    : freq_(freq), phase_bins_(std::max(1, phase_bins)),
      phase_acc_(phase_bins_) {}

void EnergyBudget::open(const std::string &period_file,
                        const std::string &phase_file,
                        const std::string &header) {
  out_.open(period_file);
  phase_file_ = phase_file;
  header_ = header;
  out_ << header;
  out_ << "# Balance de energía por período de la excitación (b = floor(f t)), "
          "en m g d.\n"
          "# Wb: trabajo de la base; Wlin, Wpiv: trabajo de la fricción de "
          "Karnopp y del pivoteo sobre los granos;\n"
          "# Wg = Wlin + Wpiv; Dslide = Wb - Wlin y Dpiv = -Wpiv: disipación "
          "contra la base; Wcont = dK - Wg: trabajo de los contactos;\n"
          "# dK: cambio de energía cinética dentro de los pasos; dK_reiny: "
          "por descarga y reinyección; K: energía cinética media.\n"
          "# Cierre: K(fin) - K(inicio) = Wg + Wcont + dK_reiny.\n"
          "# periodo t_ini t_fin npasos N Wb Wlin Wpiv Wg Dslide Dpiv Wcont dK "
          "dK_reiny K\n";
}

void EnergyBudget::before_step(b2World *w, double base_vel) {
  base_vel_ = b2Vec2(0.0f, static_cast<float>(-base_vel));
  snap_.clear();
  k_before_ = 0.0;
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    if (b->GetType() != b2_dynamicBody || !body_data(b)->is_grain) continue;
    const BodyData *bd = body_data(b);
    snap_.push_back({b, b->GetLinearVelocity(), b->GetAngularVelocity(),
                     bd->f_base, bd->tau_base});
    k_before_ += kinetic(b);
  }
}

EnergyWork EnergyBudget::after_step(b2World *w, double t, double dt) {
  (void)w; // los cuerpos son los de la foto: Step no crea ni destruye
  EnergyWork s;
  s.n_steps = 1;
  s.n_grains = static_cast<long>(snap_.size());
  double k_after = 0.0;
  for (const Snapshot &r : snap_) {
    const float m = r.body->GetMass();
    const float I = r.body->GetInertia(); // disco: origen local = centro
    // Primera etapa del Step, con la misma aritmética (float) que
    // b2Island::Solve: v' = v0 + (h / m) F, w' = w0 + (h / I) tau. Así
    // W_cont = K(v1) - K(v') es exactamente el cambio de los contactos.
    const float h = static_cast<float>(dt);
    const b2Vec2 v1p = r.v0 + (h * (1.0f / m)) * r.force;
    const float w1p = r.w0 + h * (1.0f / I) * r.torque;
    const double k0_lin = kinetic_lin(m, r.v0), k0_rot = kinetic_rot(I, r.w0);
    s.w_base += dt * (double(r.force.x) * base_vel_.x +
                      double(r.force.y) * base_vel_.y);
    s.w_lin += kinetic_lin(m, v1p) - k0_lin;
    s.w_piv += kinetic_rot(I, w1p) - k0_rot;
    const double k1 = kinetic(r.body);
    s.dk += k1 - (k0_lin + k0_rot);
    k_after += k1;
  }
  if (k_after_ >= 0.0) s.dk_reiny = k_before_ - k_after_;
  k_after_ = k_after;
  s.k_sum = k_after;
  if (!active()) return s;

  // Período: el mismo bloque que usa stress_profile, b = floor(f t)
  const long k = static_cast<long>(std::floor(freq_ * t));
  if (period_ < 0) {
    period_ = k;
    t_period_start_ = t;
  }
  if (k != period_) {
    flush_period();
    period_ = k;
    t_period_start_ = t;
  }
  cur_.add(s);
  t_period_end_ = t + dt;

  const double phase = std::fmod(kTwoPi * freq_ * t, kTwoPi);
  int ib = static_cast<int>(std::floor(phase_bins_ * phase / kTwoPi));
  ib = std::clamp(ib, 0, phase_bins_ - 1);
  phase_acc_[ib].add(s);

  if (t_first_ < 0.0) t_first_ = t;
  t_last_ = t + dt;
  return s;
}

void EnergyBudget::write_row(std::ofstream &o, const std::string &prefix,
                             const EnergyWork &a, double scale) {
  const double wg = a.w_lin + a.w_piv;
  o << prefix << " " << a.n_steps << " " << std::fixed << std::setprecision(4)
    << double(a.n_grains) / a.n_steps << std::scientific
    << std::setprecision(10);
  // 0.0 - x: evita imprimir -0 cuando x = 0
  for (double x : {a.w_base, a.w_lin, a.w_piv, wg, a.w_base - a.w_lin,
                   0.0 - a.w_piv, a.w_cont(), a.dk, a.dk_reiny})
    o << " " << x * scale;
  o << " " << a.k_sum / a.n_steps << "\n";
}

void EnergyBudget::flush_period() {
  if (cur_.n_steps == 0) return;
  std::ostringstream pre;
  pre << period_ << " " << std::setprecision(10) << t_period_start_ << " "
      << t_period_end_;
  write_row(out_, pre.str(), cur_, 1.0);
  out_.flush();
  cur_ = EnergyWork{};
}

void EnergyBudget::close() {
  if (!active()) return;
  flush_period();
  out_.close();

  // Perfil en fase: trabajo por período en cada bin de fase
  const double n_periods = (t_last_ - t_first_) * freq_;
  std::ofstream ph(phase_file_);
  ph << header_;
  ph << "# Balance de energía por bin de fase (2 pi f t mod 2 pi): trabajo "
        "por período, en m g d (sumado sobre los bines da el trabajo por "
        "período).\n"
        "# periodos="
     << n_periods
     << "  columnas como en el archivo por período; K es la media en el "
        "bin.\n"
        "# fase dfase npasos N Wb Wlin Wpiv Wg Dslide Dpiv Wcont dK dK_reiny "
        "K\n";
  for (int i = 0; i < phase_bins_; ++i) {
    const EnergyWork &a = phase_acc_[i];
    if (a.n_steps == 0 || n_periods <= 0.0) continue;
    std::ostringstream pre;
    pre << std::setprecision(6) << kTwoPi * (i + 0.5) / phase_bins_ << " "
        << kTwoPi / phase_bins_;
    write_row(ph, pre.str(), a, 1.0 / n_periods);
  }
}
