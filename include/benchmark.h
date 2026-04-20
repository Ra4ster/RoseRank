#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "pricing.h"

float benchmark_models(OptionInfo option, float american);

void benchmark_model(OptionInfo option, float american, char *model);

#endif // BENCHMARK_H