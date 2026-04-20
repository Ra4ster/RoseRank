#include "black_scholes.h"
#include "pricing.h"

static inline float d1_(float S_0, float K, float r, float sigma, float T)
{
    return (log(S_0 / K) + (r + (sigma * sigma / 2.0f)) * T) / (sigma * sqrt(T));
}

static inline float d2_(float d1, float sigma, float T)
{
    return d1 - sigma * sqrt(T);
}

float bs_call(float S_0, float K, float T, float r, float sigma)
{
    float val = 0.0f;

    float d1 = d1_(S_0, K, r, sigma, T);
    float d2 = d2_(d1, sigma, T);

    // C = S_0 N(d_1) - K e^{-rT} N(d_2)
    val = S_0 * normalCDF(d1) - K * exp(-r * T) * normalCDF(d2);

    return val;
}

float bs_put(float S_0, float K, float T, float r, float sigma)
{
    float val = 0.0f;

    float d1 = d1_(S_0, K, r, sigma, T);
    float d2 = d2_(d1, sigma, T);

    // P = K e^{-rT} N(-d_2) - S_0 N(-d_1)
    val = K * exp(-r * T) * normalCDF(-d2) - S_0 * normalCDF(-d1);

    return val;
}

Greeks get_greeks(float S_0, float K, float T, float r, float sigma, int is_put)
{
    Greeks g;
    float d1 = d1_(S_0, K, r, sigma, T);
    float d2 = d2_(d1, sigma, T);

    float npdf = normalPDF(d1);
    float ncdf1 = normalCDF(d1);
    float ncdf2 = normalCDF(d2);
    float stf = sqrtf(T);
    float e_rt = expf(-r * T);

    g.gamma = npdf / (S_0 * sigma * stf);
    g.vega = S_0 * stf * npdf * 0.01f; // 1% vol change
    if (is_put)
    {
        g.delta = ncdf1 - 1.0f;
        g.rho = -K * T * e_rt * normalCDF(-d2) * 0.01f;

        float term1 = -(S_0 * npdf * sigma) / (2.0f * stf);
        float term2 = r * K * e_rt * normalCDF(-d2);
        g.theta = (term1 + term2) / 365.0f;
    }
    else
    {
        g.delta = ncdf1;
        g.rho = K * T * e_rt * ncdf2 * 0.01f;

        float term1 = -(S_0 * npdf * sigma) / (2.0f * stf);
        float term2 = r * K * e_rt * ncdf2;
        g.theta = (term1 - term2) / 365.0f;
    }
    return g;
}