/*! \file rng.hpp
 * \brief Generador de números aleatorios (Mersenne Twister, std::mt19937).
 *
 * Con la misma semilla (rand_seed) la simulación es reproducible.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#pragma once

#include <cstdint>
#include <random>

/*! \class RNG
 * \brief Envoltorio de std::mt19937 con distribuciones uniformes.
 */
class RNG {
public:
  /*! Semilla tomada de std::random_device (no reproducible). */
  RNG();
  /*! Semilla fija (reproducible). */
  explicit RNG(uint32_t seed);
  /*! Entero uniforme en [min, max]. */
  uint32_t get_int(uint32_t min, uint32_t max);
  /*! Real uniforme en [min, max). */
  double get_double(double min, double max);
  /*! true con probabilidad p. */
  bool flip(double p);

private:
  std::mt19937 generator;
};
