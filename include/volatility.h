#ifndef VOLATILITY_H
#define VOLATILITY_H

#include "pricing.h"

// Gets the mean of data with count n.
float get_mean(float *data, int n);

// Gets the standard deviation of data with mean mu and count n.
float std(float *data, float mu, int n);

// Calculates the volatility of prices with count n.
float get_volatility(float *past_prices, int n);

typedef struct IVResult
{
    float sigma;
    int converged;
    int iterations;
    const char *method;
} IVResult;

IVResult get_implied_vol(float S_0, float K, float T, float r, float market_price);

// Naive implementation of IV Percentile. Uses AVX2 `get_implied_vol`.
float calculate_iv_percentile(OptionInfo today, float current_market_price, OptionInfo *history,
                              float *hist_market_prices, int n);

#endif // VOLATILITY_H