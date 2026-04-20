#include "volatility.h"
#include "benchmark.h"
#include "black_scholes.h"
#include "best_stock.h"

#define VERSION "1.0.0"

void help_msg()
{
    printf("Option Pricer\nGuide:\n");
    printf("--------------------\n");
    printf("-set (S, K, T, r, v, H)\t Uses the following values:\n");
    printf("\t 1. S: Current Price\n\t 2. K: Strike Price\n\t 3. T: Years until Expiration"
           "\n\t 4. r: Risk-Free Rate\n\t 5. v: Volatility\n\t 6. H: History (used to calculator volatility)\n"
           "\t Example Usage: \"-set (43, 40, 0.015, 0.004, 0, ./data.csv)\"\n");
    printf("-contract <call|put>\t Possible contracts (default: call).\n");
    printf("-type <american|european>\t Version of option to use (default: european).\n");
    printf("-model <BlackScholes|Binomial|MonteCarlo>\t"
           "Model to use, currently supporting BlackScholes, Binomial, & MonteCarlo.\n");
    printf("-greeks\t Prints the standard option metrics.\n");
    printf("-help\t Prints the usage.\n");
    printf("-screener <file>\t Parses and outputs information on several stocks at once. Gives recommended structure.\n"
           "Example output:\n"
           "$ ./option_pricer -screener stocks_to_watch.csv\n"
           "\n"
           "Ticker   | Rank   | IVP   | Yield    | LEAP Eff.    | Recommended Structure\n"
           "-------------------------------------------------------------------------------------\n"
           "NVDA     | 0.92   | 0.45  | 0.68     | 9.52         | Neutral / Wait for Entry\n"
           "AAPL     | 0.88   | 0.19  | 0.16     | 28.29        | Deep ITM LEAP (High Efficiency)\n"
           "TSLA     | 0.45   | 0.66  | 1.02     | 6.80         | Neutral / Wait for Entry\n"
           "KO       | 0.55   | 0.13  | 0.06     | 37.32        | Neutral / Wait for Entry\n"
           "GME      | 0.20   | 2.16  | 20.82    | 3.27         | Bear Call Spread    \n"
           "MSFT     | 0.75   | 0.21  | 0.28     | 22.96        | Neutral / Wait for Entry)\n");
    printf("-version\t Prints this version.\n\n");
    printf("--------------------\n");
}

void screener(const char *ticker_list_file)
{
    FILE *file = fopen(ticker_list_file, "r");
    if (!file)
    {
        fprintf(stderr, "Error: Could not open screener file %s\n", ticker_list_file);
        return;
    }

    char line[1024];
    // Skip header line
    fgets(line, sizeof(line), file);

    printf("\n%-8s | %-6s | %-5s | %-8s | %-12s | %-20s\n",
           "Ticker", "Rank", "IVP", "Yield", "LEAP Eff.", "Recommended Structure");
    printf("-------------------------------------------------------------------------------------\n");

    while (fgets(line, sizeof(line), file))
    {
        char ticker[16];
        float rank, price, call_p, put_p, strike, expiry;
        if (sscanf(line, "%[^,],%f,%f,%f,%f,%f,%f",
                   ticker, &rank, &price, &call_p, &put_p, &strike, &expiry) == 7)
        {
            Candidate c;
            c.stock_rank = rank;

            IVResult res = get_implied_vol(price, strike, expiry, 0.05, call_p);

            // Use sigma directly as a placeholder for IVP until you implement historical arrays
            c.iv_percentile = res.converged ? res.sigma : 0.0f;
            c.call_yield = calculate_cc_yield(price, call_p, expiry);

            Greeks g = get_greeks(price, strike, expiry, 0.05, res.sigma, 0);
            c.leap_efficiency = calculate_leap_efficiency(price, call_p, g.delta);

            IVResult iv_put = get_implied_vol(price, strike, expiry, 0.05, put_p);
            float skew = iv_put.sigma - res.sigma;

            const char *structure = determine_best_structure(c, skew);
            printf("%-8s | %-6.2f | %-5.2f | %-8.2f | %-12.2f | %-20s\n",
                   ticker, c.stock_rank, c.iv_percentile, c.call_yield, c.leap_efficiency, structure);
        }
    }

    fclose(file);
}

void handle_msgs(int argc, char *argv[])
{
    float current = 0.0, strike = 0.0, expiry_years = 0.0;
    float rf_rate = 0.0, sigma = 0.0;
    char history[256] = {0};

    int have_set = 0;
    int model_selected = 0;
    int want_greeks = 0;
    int american = 0;
    char model[64] = "BlackScholes";
    int is_put = 0; // Put flag

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-help") == 0)
        {
            help_msg();
            return;
        }
        else if (strcmp(argv[i], "-version") == 0)
        {
            printf("Option Price %s by https://Ra4ster.github.io", VERSION);
            return;
        }
        else if (strcmp(argv[i], "-screener") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: -screener requires a filename\n");
                return;
            }
            screener(argv[++i]);
            return;
        }
        else if (strcmp(argv[i], "-greeks") == 0)
        {
            want_greeks = 1;
        }
        else if (strcmp(argv[i], "-contract") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: -contract requires <call|put>\n");
                return;
            }
            i++;
            int found_put = 0;
            if ((found_put = strcmp(argv[i], "put") == 0) || strcmp(argv[i], "call") == 0)
            { // Handle invalid contracts
                is_put = found_put;
            }
            else
            {
                fprintf(stderr, "Error: invalid contract '%s' (expected call or put)\n", argv[i]);
                return;
            }
        }
        else if (strcmp(argv[i], "-type") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: -type requires <american|european>\n");
                return;
            }
            i++;
            if (strcmp(argv[i], "american") == 0)
            {
                american = 1;
            }
            else if (strcmp(argv[i], "european") == 0)
            {
                american = 0; // Must use elif to handle error
            }
            else
            {
                fprintf(stderr, "Error: invalid type '%s' (expected american or european)\n", argv[i]);
                return;
            }
        }
        else if (strcmp(argv[i], "-model") == 0)
        {
            model_selected = 1;

            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: -model requires <BlackScholes|Binomial|MonteCarlo>\n");
                return;
            }
            i++;
            if (strcmp(argv[i], "BlackScholes") == 0 ||
                strcmp(argv[i], "Binomial") == 0 ||
                strcmp(argv[i], "MonteCarlo") == 0)
            {
                strncpy(model, argv[i], sizeof(model) - 1);
                model[sizeof(model) - 1] = '\0';
            }
            else
            {
                fprintf(stderr, "Error: invalid model '%s'\n", argv[i]);
                return;
            }
        }
        else if (strcmp(argv[i], "-set") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: -set requires (S, K, T, r, v, H)\n");
                return;
            }

            char buf[512] = {0};
            size_t used = 0;
            int found_close = 0;

            i++;
            for (; i < argc; i++)
            {
                int n = snprintf(buf + used, sizeof(buf) - used, "%s%s",
                                 used ? " " : "", argv[i]);
                if (n < 0 || (size_t)n >= sizeof(buf) - used)
                {
                    fprintf(stderr, "Error: -set argument too long\n");
                    return;
                }

                used += (size_t)n;
                if (strchr(argv[i], ')') != NULL)
                {
                    found_close = 1;
                    break;
                }
            }
            if (!found_close)
            {
                fprintf(stderr, "Error: malformed -set, missing ')'\n");
                return;
            }
            int matched = sscanf(buf,
                                 " ( %f , %f , %f , %f , %f , %255[^)] ) ",
                                 &current, &strike, &expiry_years, &rf_rate, &sigma, history);

            if (matched != 6)
            {
                fprintf(stderr, "Error: could not parse -set values.\n");
                fprintf(stderr, "Expected format: -set (S, K, T, r, v, H)\n");
                return;
            }
            have_set = 1;
        }
        else
        {
            fprintf(stderr, "Error: unknown argument '%s'\n", argv[i]);
            help_msg();
            return;
        }
    }

    if (!have_set)
    {
        fprintf(stderr, "Error: missing required -set option\n");
        help_msg();
        return;
    }
    if (sigma == 0.0 && history[0] != '\0')
    {
        fprintf(stderr, "Note: sigma is 0 and history file '%s' was provided but no volatility-calculation\n"
                        "   function was provided to compute sigma from history. Using sigma = 0.\n",
                history);
    }

    OptionInfo option = {current, strike, expiry_years, rf_rate, sigma, is_put};

    if (want_greeks)
    {
        Greeks g = get_greeks(current, strike, expiry_years, rf_rate, sigma, is_put);
        print_greeks(g);
    }

    {
        PricingResult results[3] = {0};
        int num_results = 0;

        if (model_selected)
        {
            // User picked one specific model
            if (strcmp(model, "BlackScholes") == 0)
            {
                results[0] = price(option, "BlackScholes", american);
            }
            else
            {
                results[0] = price_advanced(option, model, american, 100, 10000);
            }
            num_results = 1;
        }
        else
        {
            // No model specified: Run the "Grand Slam" comparison
            results[0] = price(option, "BlackScholes", american);
            results[1] = price_advanced(option, "Binomial", american, 100, 0);
            results[2] = price_advanced(option, "MonteCarlo", american, 0, 10000);
            num_results = 3;
        }

        print_results(num_results, results, american);
    }
}

int main(int argc, char *argv[])
{
    handle_msgs(argc, argv);
    return 0;
}