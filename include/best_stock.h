#ifndef BESTSTOCK_H
#define BESTSTOCK_H
typedef struct
{
    float stock_rank;      // From your fundamental model (0.0 - 1.0)
    float iv_percentile;   // From get_implied_vol + history
    float call_yield;      // Annualized CC return
    float leap_efficiency; // Leverage ratio
} Candidate;

float calculate_cc_yield(float stock_price, float call_price, float expiry_years);

float calculate_leap_efficiency(float stock_price, float leap_price, float leap_delta);

const char *determine_best_structure(Candidate c, float skew_score);

#endif // BESTSTOCK_H