/*! \file power_output.cpp
 * \brief Salidas de la potencia exacta de los contactos (power_output.hpp).
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#include "power_output.hpp"

#include "body_data.hpp"
#include "gzstream.hpp"
#include "output.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>

int region_of(b2Vec2 p, const GlobalSetup &gs) {
  const PotenciaOpciones &o = gs.pot;
  if (p.y < o.fuera_y) return 7;
  if (p.y < -o.chorro_L) return 6;
  if (p.y < 0.0f) return 5;
  if (p.y <= o.cuna_h && std::fabs(p.x) <= gs.silo.r + p.y) return 1;
  if (std::fabs(p.x) > gs.silo.R - o.costado_w) return 2;
  return p.y <= o.lecho_y ? 3 : 4;
}

namespace {

/// true si el punto está en el ROI (o si no se usa ROI).
bool in_roi(b2Vec2 p, const GlobalSetup &gs) {
  if (!gs.save_roi_only) return true;
  return std::fabs(p.x) <= gs.x_roi && p.y >= gs.y_min_roi &&
         p.y <= gs.y_max_roi;
}

/// Magnitudes por grano y por paso que se promedian (en este orden):
/// Cgg(2) Cgw(2) tau Fb(2) tau_b P_tr_gg P_tr_gw P_rot P_lin P_piv P_par T_in
/// D_gg D_gw.
void step_quantities(const GrainPower &g, double inv_dt, double *q) {
  q[0] = g.c_gg[0];
  q[1] = g.c_gg[1];
  q[2] = g.c_gw[0];
  q[3] = g.c_gw[1];
  q[4] = g.tau;
  q[5] = g.f_base.x;
  q[6] = g.f_base.y;
  q[7] = g.tau_base;
  q[8] = g.p_tr_gg;
  q[9] = g.p_tr_gw;
  q[10] = g.p_rot;
  q[11] = g.w_lin * inv_dt;
  q[12] = g.w_piv * inv_dt;
  q[13] = g.p_wall;
  q[14] = g.t_in;
  q[15] = g.d_gg;
  q[16] = g.d_gw;
}

} // namespace

// ---------------------------------------------------------------------------
// FzFrames
// ---------------------------------------------------------------------------

void FzFrames::add_step(const ContactPower &power) {
  const double inv_dt = 1.0 / power.dt();
  for (const GrainPower &g : power.grains()) {
    const size_t gid = static_cast<size_t>(body_data(g.body)->gid);
    if (gid >= acc_.size()) acc_.resize(gid + 1);
    double q[kNQ] = {0};
    step_quantities(g, inv_dt, q);
    for (int k = 0; k < kNQ; ++k)
      acc_[gid].q[k] += q[k];
  }
  ++steps_;
}

void FzFrames::write(b2World *w, int n_frame, double t, uint32_t n_step) {
  if (steps_ == 0) return;
  ogzstream fout(
      "frames_" + gs_.dir_id + "/" + gs_.pre_frame_file + "_" +
          frame_id_string(n_frame) + ".fz",
      gs_.comprimir_salidas);
  fout << "# tSim: " << std::setprecision(10) << t << "\n";
  fout << provenance_header(gs_, t, n_step, n_frame);
  fout << "# Promedios sobre los " << steps_
       << " pasos anteriores al instante del cuadro; posición, ángulo y "
          "velocidades: estado del cuadro (como el .ve).\n"
          "# C: fuerza de contacto con granos (gg) y con paredes (gw); tau_c: "
          "torque de contacto; Fb, tau_b: fricción de Karnopp y pivoteo.\n"
          "# P_tr, P_rot: potencia traslacional y rotacional de los "
          "contactos; P_lin, P_piv: potencia de Karnopp y pivoteo; P_par: de "
          "las paredes.\n"
          "# T_in: potencia que el grano recibe de otros granos, sum T_{j->i}; "
          "D_gg: disipación gg (D_ij / 2 por grano); D_gw: grano-pared.\n"
          "# Por grano: P_tr_gg + P_tr_gw + P_rot = T_in - D_gg - D_gw + "
          "P_par. Unidades reducidas.\n";
  fout << "# gID type x y theta vx vy w Cgg_x Cgg_y Cgw_x Cgw_y tau_c Fb_x "
          "Fb_y tau_b P_tr_gg P_tr_gw P_rot P_lin P_piv P_par T_in D_gg D_gw\n";
  std::vector<const b2Body *> grains;
  for (const b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    const BodyData *bd = body_data(b);
    if (bd->is_grain && in_roi(b->GetPosition(), gs_)) grains.push_back(b);
  }
  std::sort(grains.begin(), grains.end(), [](const b2Body *a, const b2Body *b) {
    return body_data(a)->gid < body_data(b)->gid;
  });
  fout << std::scientific << std::setprecision(gs_.comprimir_salidas ? 8 : 6);
  for (const b2Body *b : grains) {
    const BodyData *bd = body_data(b);
    const b2Vec2 p = b->GetPosition(), v = b->GetLinearVelocity();
    fout << bd->gid << " " << bd->tipo << " " << p.x << " " << p.y << " "
         << b->GetAngle() << " " << v.x << " " << v.y << " "
         << b->GetAngularVelocity();
    const size_t gid = static_cast<size_t>(bd->gid);
    for (int k = 0; k < kNQ; ++k)
      fout << " " << (gid < acc_.size() ? acc_[gid].q[k] / steps_ : 0.0);
    fout << "\n";
  }
  acc_.assign(acc_.size(), Acc{});
  steps_ = 0;
}

// ---------------------------------------------------------------------------
// PowerMaps
// ---------------------------------------------------------------------------

PowerMaps::PowerMaps(const GlobalSetup &gs)
    : gs_(gs), nb_(gs.pot.fase_bines), x_max_(gs.pot.x_max),
      y_min_(gs.pot.y_min), y_max_(gs.pot.y_max), w_(gs.pot.celda) {
  nx_ = static_cast<int>(std::ceil(2.0 * x_max_ / w_));
  ny_ = static_cast<int>(std::ceil((y_max_ - y_min_) / w_));
  cells_.assign(static_cast<size_t>(nb_) * ny_ * nx_ * kNQ, 0.0);
  steps_.assign(nb_, 0);
  flow_.assign(static_cast<size_t>(nb_) * kNRegions * kNRegions, 0.0);
  diss_.assign(flow_.size(), 0.0);
  wall_.assign(static_cast<size_t>(nb_) * kNRegions, 0.0);
}

void PowerMaps::add_step(const ContactPower &power, double t) {
  const double inv_dt = 1.0 / power.dt();
  const double ciclo = std::fmod(gs_.silo.frec * t, 2.0) / 2.0; // [0, 1)
  const int ib = std::clamp(static_cast<int>(nb_ * ciclo), 0, nb_ - 1);
  if (t_first_ < 0.0) t_first_ = t;
  t_last_ = t + power.dt();
  ++steps_[ib];

  std::vector<int> region(power.grains().size());
  for (size_t i = 0; i < region.size(); ++i) {
    const GrainPower &g = power.grains()[i];
    region[i] = region_of(g.pos, gs_);
    const int ix = static_cast<int>(std::floor((g.pos.x + x_max_) / w_));
    const int iy = static_cast<int>(std::floor((g.pos.y - y_min_) / w_));
    if (ix < 0 || ix >= nx_ || iy < 0 || iy >= ny_) continue;
    double *c = &cells_[cell_index(ib, iy, ix)];
    c[0] += 1.0;
    c[1] += g.vm.x;
    c[2] += g.vm.y;
    c[3] += g.wm;
    double q[17];
    step_quantities(g, inv_dt, q);
    for (int k = 0; k < 17; ++k)
      c[4 + k] += q[k];
  }
  // Entre regiones: T (flujo neto, antisimétrico) y D (simétrica, i <= j)
  const size_t base = static_cast<size_t>(ib) * kNRegions * kNRegions;
  for (const ContactRecord &r : power.records()) {
    if (r.a_grain && r.b_grain) {
      const int ra = region[body_data(r.a)->power_idx];
      const int rb = region[body_data(r.b)->power_idx];
      flow_[base + ra * kNRegions + rb] += r.t_ab;
      flow_[base + rb * kNRegions + ra] -= r.t_ab;
      diss_[base + std::min(ra, rb) * kNRegions + std::max(ra, rb)] += r.d;
    } else {
      const BodyData *gd = body_data(r.b_grain ? r.b : r.a);
      const int rg = region[gd->power_idx];
      diss_[base + rg] += r.d; // fila 0: pared
      wall_[static_cast<size_t>(ib) * kNRegions + rg] += r.w_wall;
    }
  }
}

void PowerMaps::write(const std::string &header) const {
  const std::string dir = "frames_" + gs_.dir_id + "/";
  const int prec = gs_.comprimir_salidas ? 8 : 6;
  {
    ogzstream fout(dir + "potencia_celdas_" + gs_.pre_frame_file + ".dat",
                   gs_.comprimir_salidas);
    fout << header;
    fout << "# Potencia por celdas y fase (ciclo de dos períodos en " << nb_
         << " bines). Celdas de lado " << w_ << ": ix = floor((x + " << x_max_
         << ") / " << w_ << "), iy = floor((y - " << y_min_ << ") / " << w_
         << "); xc, yc: centro.\n"
            "# Cada valor es la suma sobre los granos de la celda del "
            "promedio temporal en el bin (por celda de área "
         << w_ * w_
         << "). n: granos; vx, vy, w: velocidad media del paso; "
            "C: fuerza de contacto gg y gw; tau_c: torque; Fb, tau_b: "
            "Karnopp y pivoteo;\n"
            "# P_tr_gg, P_tr_gw: potencia traslacional de los contactos; "
            "P_rot: rotacional; P_lin, P_piv: base; P_par: paredes; T_in: "
            "recibida de otros granos; D_gg, D_gw: disipación. Reducidas. "
            "fase2: (ib + 0.5) / nb * 2, en períodos.\n";
    fout << "# ib fase2 ix iy xc yc n vx vy w Cgg_x Cgg_y Cgw_x Cgw_y tau_c "
            "Fb_x Fb_y tau_b P_tr_gg P_tr_gw P_rot P_lin P_piv P_par T_in "
            "D_gg D_gw\n";
    fout << std::scientific << std::setprecision(prec);
    for (int ib = 0; ib < nb_; ++ib) {
      if (steps_[ib] == 0) continue;
      for (int iy = 0; iy < ny_; ++iy)
        for (int ix = 0; ix < nx_; ++ix) {
          const double *c = &cells_[cell_index(ib, iy, ix)];
          if (c[0] == 0.0) continue;
          fout << ib << " " << (ib + 0.5) / nb_ * 2.0 << " " << ix << " " << iy
               << " " << -x_max_ + (ix + 0.5) * w_ << " "
               << y_min_ + (iy + 0.5) * w_;
          for (int k = 0; k < kNQ; ++k)
            fout << " " << c[k] / steps_[ib];
          fout << "\n";
        }
    }
  }
  ogzstream fout(dir + "potencia_pares_" + gs_.pre_frame_file + ".dat",
                 gs_.comprimir_salidas);
  fout << header;
  fout << "# Flujos de energía entre regiones, por bin de fase (potencia "
          "media). Regiones: 0 paredes, 1 cuña, 2 costados, 3 lecho bajo, 4 "
          "lecho alto, 5 chorro, 6 aguas abajo, 7 fuera.\n"
          "# T: potencia neta de la región i a la j por contactos "
          "grano-grano (antisimétrica: T_ji = -T_ij); D: disipación de los "
          "contactos entre i y j (i = 0: grano-pared, con el grano en j);\n"
          "# Wpar: trabajo de las paredes sobre los granos de j (solo i = 0). "
          "Se listan los pares con alguna magnitud no nula.\n";
  fout << "# ib fase2 i j T D Wpar\n";
  fout << std::scientific << std::setprecision(prec);
  for (int ib = 0; ib < nb_; ++ib) {
    if (steps_[ib] == 0) continue;
    const size_t base = static_cast<size_t>(ib) * kNRegions * kNRegions;
    for (int i = 0; i < kNRegions; ++i)
      for (int j = i; j < kNRegions; ++j) {
        const double t_ij = flow_[base + i * kNRegions + j];
        const double d_ij = diss_[base + i * kNRegions + j];
        const double w_j =
            i == 0 ? wall_[static_cast<size_t>(ib) * kNRegions + j] : 0.0;
        if (t_ij == 0.0 && d_ij == 0.0 && w_j == 0.0) continue;
        fout << ib << " " << (ib + 0.5) / nb_ * 2.0 << " " << i << " " << j
             << " " << t_ij / steps_[ib] << " " << d_ij / steps_[ib] << " "
             << w_j / steps_[ib] << "\n";
      }
  }
}
