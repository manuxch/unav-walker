/*! \file power_output.hpp
 * \brief Salidas de la potencia exacta de los contactos: por grano y por
 * cuadro (.fz), por celdas y fase (potencia_celdas) y entre regiones
 * (potencia_pares).
 *
 * Se alimentan, en cada paso, de ContactPower (contact_power.hpp) y se
 * escriben con ogzstream (comprimidas si comprimir_salidas). Todas las
 * magnitudes están en unidades reducidas (m = d = g = 1).
 *
 * Fase: el ciclo de dos períodos se divide en phase_bins bines; el bin de un
 * paso en el tiempo t es floor(phase_bins * ((f t) mod 2) / 2), de modo que
 * los períodos pares e impares quedan separados (respuesta de período 2).
 *
 * Regiones del análisis por pares (por la posición de cada grano al inicio
 * del paso; R: semiancho del silo, r: del orificio, H: altura):
 *
 *   0 paredes   cuerpos que no son granos (silo y tapa)
 *   1 cuña      0 <= y <= h_c y |x| <= r + y
 *   2 costados  y > 0, |x| > R - w_c, fuera de la cuña
 *   3 lecho bajo   resto del silo con y <= y_l
 *   4 lecho alto   resto del silo con y > y_l
 *   5 chorro    -L_c <= y < 0
 *   6 aguas abajo  y_fuera <= y < -L_c
 *   7 fuera     y < y_fuera
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#pragma once

#include "contact_power.hpp"
#include "global_setup.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

/// Cantidad de regiones del análisis por pares.
constexpr int kNRegions = 8;

/// Región (0..7) de un grano en la posición \p p (ver el comienzo del archivo).
int region_of(b2Vec2 p, const GlobalSetup &gs);

/*! \class FzFrames
 * \brief Acumula por grano, entre dos cuadros, las fuerzas y potencias de
 * ContactPower y escribe `<pre>_<frame>.fz`: promedios sobre los pasos del
 * intervalo y el estado al final.
 *
 * Columnas: gID tipo x y theta vx vy w Cgg_x Cgg_y Cgw_x Cgw_y tau_c Fb_x
 * Fb_y tau_b P_tr_gg P_tr_gw P_rot P_lin P_piv P_par T_in D_gg D_gw. C: fuerza de
 * contacto con granos (gg) y con paredes (gw); tau_c: torque de contacto
 * (respecto del centro); Fb, tau_b: fricción de Karnopp y torque de
 * pivoteo; P_tr, P_rot: potencia traslacional y rotacional de los
 * contactos; P_lin, P_piv: potencia de Karnopp y del pivoteo; P_par:
 * potencia de las paredes (fuente); T_in: potencia que recibe de otros granos;
 * D_gg (la mitad de D_ij de cada
 * contacto) y D_gw: disipación. Posición y velocidades: al final del
 * intervalo (el estado de los .ve del mismo cuadro); theta, el ángulo del
 * disco.
 */
class FzFrames {
public:
  explicit FzFrames(const GlobalSetup &gs) : gs_(gs) {}
  /// Suma un paso (llamar después de ContactPower::after_step).
  void add_step(const ContactPower &power);
  /// Escribe el cuadro \p n_frame con el intervalo acumulado y lo reinicia.
  /// No escribe nada si no se acumuló ningún paso.
  void write(b2World *w, int n_frame, double t, uint32_t n_step);

private:
  static constexpr int kNQ = 17;
  struct Acc {
    double q[kNQ] = {0};
  };
  const GlobalSetup &gs_;
  std::vector<Acc> acc_; // por gid
  long steps_ = 0;
};

/*! \class PowerMaps
 * \brief Potencia por celdas de un diámetro y por fase, y flujos de energía
 * entre regiones por fase, promediados en el tiempo.
 *
 * potencia_celdas_<pre>.dat: por celda (ix, iy) y bin de fase, con la
 * suma sobre los granos de la celda del promedio temporal de cada
 * magnitud (equivale a la densidad por unidad de área, la celda tiene área
 * ancho^2): n (granos), vx, vy, w, Cgg_x, Cgg_y, Cgw_x, Cgw_y, tau_c, Fb_x,
 * Fb_y, P_tr_gg, P_tr_gw, P_rot, P_lin, P_piv, P_par, T_in, D_gg, D_gw.
 *
 * potencia_pares_<pre>.dat: por bin de fase y par de regiones (i <= j),
 * el flujo neto de energía T_{i->j} (potencia que pasa de la región i a la
 * j por contactos grano-grano), la disipación D_ij de los contactos entre
 * esas regiones (para i = 0: grano-pared, con la región del grano en j) y,
 * en la fila (0, j), el trabajo de las paredes sobre j.
 */
class PowerMaps {
public:
  explicit PowerMaps(const GlobalSetup &gs);
  /// Suma un paso (llamar después de ContactPower::after_step); \p t es el
  /// tiempo al inicio del paso.
  void add_step(const ContactPower &power, double t);
  /// Escribe las dos salidas (en frames_<dirID>/).
  void write(const std::string &header) const;

private:
  static constexpr int kNQ = 21;
  const GlobalSetup &gs_;
  int nx_, ny_, nb_;
  double x_max_, y_min_, y_max_, w_;
  std::vector<double> cells_;        // [bin][iy][ix][kNQ]
  std::vector<long> steps_;          // pasos por bin
  std::vector<double> flow_, diss_;  // [bin][i][j]
  std::vector<double> wall_;         // [bin][j]
  double t_first_ = -1.0, t_last_ = 0.0;
  size_t cell_index(int ib, int iy, int ix) const {
    return ((static_cast<size_t>(ib) * ny_ + iy) * nx_ + ix) * kNQ;
  }
};
