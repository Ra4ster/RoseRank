#ifndef BINOM_H
#define BINOM_H

#include <math.h>
#include <immintrin.h>

float *binom_tree(float S_0, float K, float T, float sigma, int n,
                  int is_put);

float binom_value(float S_0, float K, float T, float r, float sigma, int n,
                  int is_put, int is_american);

#endif // BINOM_H