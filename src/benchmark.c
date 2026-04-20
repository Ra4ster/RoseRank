#include "benchmark.h"

float benchmark_models(OptionInfo option, float american)
{
    printf("Option: %s\n", option.is_put ? "Put" : "Call");
    printf("Style: %s\n", american ? "American" : "European");
    printf("Inputs:\n");
    printf("S=%f K=%f T=%f r=%f sigma=%f\n\n",
           option.strike_price, option.current_price, option.expiry_years, option.rf_rate, option.volatility);

    PricingResult pr_s[3];
    pr_s[0] = price(option, "BlackScholes", american);
    pr_s[1] = price(option, "Binomial", american);
    pr_s[2] = price(option, "MonteCarlo", american);

    print_results(3, pr_s, american);
    return (pr_s[0].price + pr_s[1].price + pr_s[2].price) / 3.0f;
}

void benchmark_model(OptionInfo option, float american, char *model)
{
    if (strcmp(model, "Binomial") == 0)
    {
        PricingResult results[7];
        results[0] = price(option, "BlackScholes", american);
        results[1] = price_advanced(option, model, american, 10, 0);
        results[2] = price_advanced(option, model, american, 50, 0);
        results[3] = price_advanced(option, model, american, 100, 0);
        results[4] = price_advanced(option, model, american, 500, 0);
        results[5] = price_advanced(option, model, american, 1000, 0);
        results[6] = price_advanced(option, model, american, 5000, 0);
        print_results(7, results, american);
    }
    else if (strcmp(model, "MonteCarlo") == 0)
    {
        PricingResult results[5];
        results[0] = price(option, "BlackScholes", american);

        results[1] = price_advanced(option, model, american, 0, 1e3);
        results[2] = price_advanced(option, model, american, 0, 1e4);
        results[3] = price_advanced(option, model, american, 0, 1e5);
        results[4] = price_advanced(option, model, american, 0, 1e6);
        print_results(5, results, american);
    }
    else
    {
        PricingResult result[1] = {price(option, "BlackScholes", american)};
        print_results(1, result, american);
    }
}