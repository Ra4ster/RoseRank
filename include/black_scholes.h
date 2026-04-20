#ifndef BLACK_SCHOLES_H
#define BLACK_SCHOLES_H
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define _USE_MATH_DEFINES
#include <math.h>
#include <locale.h>
#include "pricing.h"

// Fallback if MinGW is being stubborn with constants
#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440
#endif

#define INVSQRT2PI 0.3989422804f

// Source - https://stackoverflow.com/a/18786808
// Posted by JFS, modified by community. See post 'Timeline' for change history
// Retrieved 2026-04-15, License - CC BY-SA 4.0
static inline double normalCDF(double value)
{
    return 0.5f * erfcf(-value * M_SQRT1_2);
}

static inline float normalPDF(float x)
{
    return INVSQRT2PI * expf(-0.5f * x * x);
}

typedef struct Greeks
{
    float delta;
    float gamma;
    float vega;
    float theta;
    float rho;
} Greeks;

// Calculates the Black-Scholes value of a call option with:
// - Stock price of S_0
// - Strike price of k
// - Expiration time of T
// - Risk-free rate of r
// - Volatility of sigma
//
// For more information please see [here](https://en.wikipedia.org/wiki/Black%E2%80%93Scholes_equation).
float bs_call(float S_0, float K, float T, float r, float sigma);

// Calculates the Black-Scholes value of a put option with:
// - Stock price of S_0
// - Strike price of k
// - Expiration time of T
// - Risk-free rate of r
// - Volatility of sigma
//
// For more information please see [here](https://en.wikipedia.org/wiki/Black%E2%80%93Scholes_equation).
float bs_put(float S_0, float K, float T, float r, float sigma);

// Calculates the greeks of an option.
//
// For more information, please see [here](https://en.wikipedia.org/wiki/Greeks_(finance)).
Greeks get_greeks(float S_0, float K, float T, float r, float sigma, int is_put);

static inline void print_greeks(Greeks g)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001); // Set terminal to UTF-8
#endif

    printf("\n------------------------------------------\n");
    // Standard UTF-8 strings for Delta, Vega, Theta, Gamma
    printf("   \xce\x94         v         \xce\x98         \xce\x93\n");
    printf("  Delta     Vega      Theta     Gamma\n");
    printf("------------------------------------------\n");
    printf(" %f  %f  %f  %f\n",
           g.delta, g.vega, g.theta, g.gamma);
    printf("------------------------------------------\n");
    printf("(using \xcf\x81 = %f)\n", g.rho);
}

#endif // BLACK_SCHOLES_H