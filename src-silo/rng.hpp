#ifndef _RNG_H
#define _RNG_H

#include <random>

class RNG {
public:
    RNG();
    explicit RNG(uint32_t seed);
    uint32_t get_int(uint32_t min, uint32_t max);
    double get_double(double min, double max);
    bool flip(double p);

private:
    std::mt19937 generator;
};

#endif

