#include "testing.h"
#include "monte_carlo.h"

float ziggurat_rand_normal()
{
    return NAN; // TODO: NOT IMPLEMENTED
}

float rand_normal()
{
    return box_muller_rand_normal(0.0f, 1.0f);
}

float box_muller_rand_normal(float mu, float sigma)
{
    static int has_spare = 0;
    static float spare; // Cache z1

    if (has_spare)
    {
        has_spare = 0;
        return mu + sigma * spare;
    }

    float u1, u2;

    do
    {
        u1 = rand_percent();
    } while (u1 <= 1e-7f);

    u2 = rand_percent();

    float mag = sqrtf(-2.0f * logf(u1));
    float z0 = mag * cosf(two_pi * u2);
    float z1 = mag * sinf(two_pi * u2);

    spare = z1;
    has_spare = 1;

    return mu + sigma * z0;
}

float mc_value(float S, float K, float T, float r, float sigma, int sims, int is_put)
{
    float sum = 0.0f;
    float drift = (r - 0.5f * sigma * sigma) * T;

    if (is_put)
        for (int i = 0; i < sims; i++)
        {
            float z = rand_normal();
            float ST = S * expf(drift +
                                sigma * sqrtf(T) * z);

            float payoff = fmaxf(K - ST, 0.0f);
            sum += payoff;
        }
    else
        for (int i = 0; i < sims; i++)
        {
            float z = rand_normal();
            float ST = S * expf(drift +
                                sigma * sqrtf(T) * z);

            float payoff = fmaxf(ST - K, 0.0f);
            sum += payoff;
        }

    return expf(-r * T) * (sum / sims);
}