/*! \file rng.cpp
 * \brief Generador de números aleatorios (Mersenne Twister, std::mt19937).
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#include "rng.hpp"

RNG::RNG() : generator(std::random_device{}()) {}
RNG::RNG(uint32_t seed) : generator(seed) {}

uint32_t RNG::get_int(uint32_t min, uint32_t max) {
  std::uniform_int_distribution<uint32_t> distribution(min, max);
  return distribution(generator);
}

double RNG::get_double(double min, double max) {
  std::uniform_real_distribution<double> distribution(min, max);
  return distribution(generator);
}

bool RNG::flip(double p) {
  std::uniform_real_distribution<double> distribution(0.0, 1.0);
  double x = distribution(generator);
  return x < p;
}
