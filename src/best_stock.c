#include <math.h>
#include "best_stock.h"

float calculate_cc_yield(float stock_price, float call_price, float expiry_years)
{
    if (stock_price <= 0 || expiry_years <= 0)
        return 0.0f;

    float raw_return = call_price / stock_price;

    float annualized = powf(1.0f + raw_return, 1.0f / expiry_years) - 1.0f;
    return annualized;
}

inline float calculate_leap_efficiency(float stock_price, float leap_price, float leap_delta)
{
    return leap_price <= 0
               ? 0.0f
               : (stock_price * leap_delta) / leap_price;
}

const char *determine_best_structure(Candidate c, float skew_score)
{
    if (c.stock_rank > 0.8 && c.iv_percentile < 0.3)
    {
        if (c.leap_efficiency > 2.5)
            return "Deep ITM LEAP (High Efficiency)";
        else
            return "Buy Shares + Long Calls";
    }
    if (c.stock_rank > 0.6 && c.iv_percentile > 0.7)
    {
        if (skew_score > 0.15)
            return "Sell Bull Put Spread (High Put Skew)";
        return "Covered Call";
    }
    if (c.iv_percentile > 0.85)
    {
        if (c.stock_rank < 0.4)
            return "Bear Call Spread";
        return "Iron Condor (Volatility Crush Play)";
    }
    return "Neutral / Wait for Entry";
}