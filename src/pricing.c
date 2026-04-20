#include "monte_carlo.h"
#include "pricing.h"

PricingResult price(OptionInfo option, char *model, int is_american)
{
    return price_advanced(option, model, is_american, 1000, 1000000);
}

PricingResult price_advanced(OptionInfo option, char *model, int is_american, int n_steps, int n_sims)
{
    model = model ? model : "(unknown)";
    PricingResult result = {0};
    result.model = model;
    result.runtime = INFINITY;

    struct timespec t1, t2;
    if (strcmp("BlackScholes", model) == 0)
    {
        if (is_american)
        {
            return result;
        }
        float price = 0.0f;
        if (option.is_put)
        {
            t1 = now();
            price = bs_put(
                option.current_price, option.strike_price,
                option.expiry_years, option.rf_rate, option.volatility);
            t2 = now();
        }
        else
        {
            t1 = now();
            price = bs_call(
                option.current_price, option.strike_price,
                option.expiry_years, option.rf_rate, option.volatility);
            t2 = now();
        }

        result.price = price;
        result.runtime = stopwatch(t1, t2);
        result.simulations = 0;
        result.steps = 1;
    }
    else if (strcmp("Binomial", model) == 0)
    {
        float price = 0.0f;
        t1 = now();
        price = binom_value(
            option.current_price, option.strike_price,
            option.expiry_years, option.rf_rate, option.volatility, n_steps, option.is_put, is_american);
        t2 = now();

        result.price = price;
        result.runtime = stopwatch(t1, t2);
        result.simulations = 0;
        result.steps = n_steps;
    }
    else if (strcmp("MonteCarlo", model) == 0)
    {
        if (is_american)
        { // TODO: American MC
            return result;
        }
        float price = 0.0f;

        t1 = now();
        price = mc_value(option.current_price, option.strike_price,
                         option.expiry_years, option.rf_rate, option.volatility, n_sims, option.is_put);
        t2 = now();

        result.price = price;
        result.runtime = stopwatch(t1, t2);
        result.simulations = n_sims;
        result.steps = 0;
    }

    return result;
}