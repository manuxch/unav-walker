#include "rng.hpp"

RNG::RNG() : generator(std::random_device{}()) {}
RNG::RNG(uint32_t seed) : generator(seed) {}

uint32_t RNG::get_int(uint32_t min, uint32_t max) {
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(generator);
}

double RNG::get_double(double min, double max) {
    std::uniform_real_distribution<double> distribution(min, max);
    return distribution(generator);
}

bool RNG::flip(double p) {
    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    double x = distribution(generator);
    return (x >= 0.0 ? true : false);
}

