#ifndef PRICING_H
#define PRICING_H

#include <string.h>
#include <stdio.h>
#include "black_scholes.h"
#include "binom.h"
#include "testing.h"

typedef struct PricingResult
{
    const char *model;
    float price;
    double runtime;
    int steps;
    int simulations;
} PricingResult;

// current_price
// strike_price
// expiry_years
// rf_rate
// volatility
// is_put
typedef struct OptionInfo
{
    float current_price;
    float strike_price;
    float expiry_years;
    float rf_rate;
    float volatility;

    int is_put;
} OptionInfo;

PricingResult price(OptionInfo option, char *model, int is_american);
PricingResult price_advanced(OptionInfo option, char *model, int is_american, int n_steps, int n_sims);

static inline void print_results(int count, PricingResult results[], int american)
{
    printf("\nModel Results\n");
    printf("-----------------------------------------------------------------\n");
    printf("%-20s %-12s %-12s %-10s\n", "Model", "Applicable", "Price", "Time");
    printf("-----------------------------------------------------------------\n");

    for (int i = 0; i < count; i++)
    {
        PricingResult pr = results[i];

        char label[32];
        char applicable[12];
        char price_str[16];

        if (strcmp(pr.model, "Binomial") == 0)
        {
            snprintf(label, sizeof(label), "%s(%d)", pr.model, pr.steps);
        }
        else if (strcmp(pr.model, "MonteCarlo") == 0)
        {
            snprintf(label, sizeof(label), "%s(%.0e)", pr.model, (double)pr.simulations);
        }
        else
        {
            snprintf(label, sizeof(label), "%s", pr.model);
        }

        if (american)
        {
            if (strcmp(pr.model, "Binomial") == 0)
            {
                snprintf(applicable, sizeof(applicable), "Yes");
                snprintf(price_str, sizeof(price_str), "%.2f", pr.price);
            }
            else if (strcmp(pr.model, "MonteCarlo") == 0)
            {
                snprintf(applicable, sizeof(applicable), "No*");
                snprintf(price_str, sizeof(price_str), "N/A");
            }
            else
            {
                snprintf(applicable, sizeof(applicable), "No");
                snprintf(price_str, sizeof(price_str), "N/A");
            }
        }
        else
        {
            snprintf(applicable, sizeof(applicable), "Yes");
            snprintf(price_str, sizeof(price_str), "%.2f", pr.price);
        }

        // ----- Unified print -----
        printf("%-20s %-12s %-12s %-10.4f\n",
               label,
               applicable,
               price_str,
               pr.runtime);
    }

    printf("-----------------------------------------------------------------\n");

    if (american)
    {
        printf("* basic implementation prices European exercise only\n");
    }
}

#endif // PRICING_H