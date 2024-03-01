#ifndef _SIMULACION_H
#define _SIMULACION_H

#include "globalsetup.hpp"
#include "rng.hpp"
#include "cromosoma.hpp"
#include <box2d/box2d.h>
#include <iostream>
#include <cmath>

std::string int2str(int num);
void saveFrame(b2World *w, int frm_id, const GlobalSetup *gs);
double fase(float* x, float* t, int n_datos, float freq);
double eval_fitness(double *obst_params, GlobalSetup *gs, RNG *rng, bool save_frames);

#endif
