

#include <stdio.h>
#include <immintrin.h>
#include <math.h>
#include "avx_mathfun.h" // Used for log256_ps impl
#include "volatility.h"
#include "black_scholes.h"

float get_mean(float *data, int n)
{
    __m256 sum = _mm256_setzero_ps();
    int i = 0;
    for (; i < n - 8; i += 8)
    {
        __m256 d_ = _mm256_loadu_ps(data + i);
        sum = _mm256_add_ps(sum, d_);
    }

    float tmp[8];
    _mm256_storeu_ps(tmp, sum);
    float result = 0;
    for (int j = 0; j < 8; j++)
        result += tmp[j];

    while (i < n)
        result += data[i++];

    return (result / n);
}

float std(float *data, float mu, int n)
{
    int i = 0;

    __m256 sq_err_vec = _mm256_setzero_ps();
    for (; i < n - 8; i += 8)
    {
        __m256 d_ = _mm256_loadu_ps(data + i);
        __m256 mu_ = _mm256_set1_ps(mu);

        __m256 e_ = _mm256_sub_ps(d_, mu_);
        sq_err_vec = _mm256_add_ps(sq_err_vec, _mm256_mul_ps(e_, e_));
    }

    float tmp[8];
    _mm256_storeu_ps(tmp, sq_err_vec);
    float error = 0.0f;
    for (int j = 0; j < 8; j++)
        error += tmp[j];

    while (i < n)
    {
        float diff = data[i++] - mu;
        error += (diff * diff);
    }

    return sqrtf(error / n);
}

float get_volatility(float *past_prices, int n)
{
    float *returns = (float *)(malloc(sizeof(float) * (n - 1)));

    int i = 0;
    for (; i < n - 8; i += 8)
    {
        __m256 past = _mm256_loadu_ps(past_prices + i);
        __m256 present = _mm256_loadu_ps(past_prices + i + 1);

        __m256 rcp = _mm256_rcp_ps(past);

        _mm256_storeu_ps(returns + i, // Computes log(( present / past))
                         log256_ps(_mm256_mul_ps(present, rcp)));
    }

    while (i < n - 1)
    {
        returns[i] = (past_prices[i + 1] / past_prices[i]) - 1.0f;
        i++;
    }

    return std(returns, get_mean(returns, n - 1), n - 1);
}

IVResult get_implied_vol(float S_0, float K, float T, float r, float market_price)
{
    IVResult out = {0.0f, 0, 0, "none"};
    if (S_0 <= 0.0f || K <= 0.0f || T <= 0.0f)
        return out;

    float lower = fmaxf(S_0 - K * expf(-r * T), 0.0f);
    float upper = S_0;
    if (market_price < lower || market_price > upper)
        return out;

    const float tol = 1.0e-6f;
    const float sigma_min = 1.0e-4f;
    const float sigma_max = 3.0f;
    const int max_iter = 100;

    float lo = sigma_min;
    float hi = sigma_max;

    float price_lo = bs_call(S_0, K, T, r, lo);
    float price_hi = bs_call(S_0, K, T, r, hi);

    // Expand upper bound if needed
    while (price_hi < market_price && hi < 10.0f)
    {
        hi *= 2.0f;
        price_hi = bs_call(S_0, K, T, r, hi);
    }

    if (!(price_lo <= market_price && market_price <= price_hi))
        return out;

    // Newton-Raphson attempt, but constrained to bracket
    float sigma = 0.5f;
    if (sigma < lo || sigma > hi)
        sigma = 0.5f * (lo + hi);

    for (int i = 0; i < max_iter; i++)
    {
        float price = bs_call(S_0, K, T, r, sigma);
        float diff = price - market_price;

        if (fabsf(diff) < tol)
        {
            out.sigma = sigma;
            out.converged = 1;
            out.iterations = i + 1;
            out.method = "newton";
            return out;
        }

        Greeks g = get_greeks(S_0, K, T, r, sigma, 0);

        // Update bracket
        if (diff > 0.0f)
            hi = sigma;
        else
            lo = sigma;

        // If vega is too small, bail to bisection
        if (fabsf(g.vega) < 1.0e-8f)
            break;

        // Important: use vega scaling consistent with get_greeks()
        // If g.vega is already dPrice/dSigma, do NOT multiply by 100.
        float next_sigma = sigma - diff / g.vega;

        if (next_sigma <= lo || next_sigma >= hi || !isfinite(next_sigma))
            break;

        sigma = next_sigma;
    }

    // Bisection fallback
    for (int i = 0; i < max_iter; i++)
    {
        float mid = 0.5f * (lo + hi);
        float price = bs_call(S_0, K, T, r, mid);
        float diff = price - market_price;

        if (fabsf(diff) < tol || fabsf(hi - lo) < tol)
        {
            out.sigma = mid;
            out.converged = 1;
            out.iterations = i + 1;
            out.method = "bisection";
            return out;
        }

        if (diff > 0.0f)
            hi = mid;
        else
            lo = mid;
    }

    out.sigma = 0.5f * (lo + hi);
    out.converged = 0;
    out.iterations = max_iter;
    out.method = "bisection";
    return out;
}

float calculate_iv_percentile(OptionInfo today, float current_market_price, OptionInfo *history, float *hist_market_prices, int n)
{
    float iv_samples[252];
    int count = 0;

    IVResult current_res = get_implied_vol(today.current_price, today.strike_price, today.expiry_years, today.rf_rate, current_market_price);
    if (!current_res.converged)
        return -1.0f;

    for (int i = 0; i < n; i++)
    {
        IVResult res = get_implied_vol(history[i].current_price, history[i].strike_price, history[i].expiry_years, history[i].rf_rate, hist_market_prices[i]);
        if (res.converged)
        {
            iv_samples[count++] = res.sigma;
        }
    }

    int below = 0;
    for (int i = 0; i < count; i++)
    {
        if (iv_samples[i] < current_res.sigma)
            below++;
    }

    return (float)below / count;
}