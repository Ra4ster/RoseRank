#define _USE_MATH_DEFINES
#include <math.h>

#define two_pi (2.0 * M_PI)

float box_muller_rand_normal(float mu, float sigma);

float ziggurat_rand_normal();

float rand_normal();

float mc_value(float S, float K, float T, float r, float sigma, int sims, int is_put);