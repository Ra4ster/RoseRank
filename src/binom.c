#include "binom.h"

float *binom_tree(float S_0, float K, float T, float sigma, int n,
                  int is_put)
{
    float dt = T / n;
    float up = expf(sigma * sqrt(dt));
    float down = 1.0f / up;
    float step_up = up / down;
    float S_curr = S_0 * powf(down, n);
    float *values = (float *)malloc(sizeof(float) * (n + 1));

    if (is_put)
        for (int j = 0; j <= n; j++)
        {
            values[j] = fmaxf(0.0, K - S_curr);
            S_curr *= step_up;
        }
    else
        for (int j = 0; j <= n; j++)
        {
            values[j] = fmaxf(0.0, S_curr - K);
            S_curr *= step_up;
        }

    return values;
}

float binom_value(float S_0, float K, float T, float r, float sigma, int n,
                  int is_put, int is_american)
{
    float dt = T / n;
    float up = expf(sigma * sqrtf(dt));
    float down = 1.0f / up;
    float step_up = up / down;

    float p = (expf(r * dt) - down) / (up - down);
    float p_inv = 1.0f - p;
    float e_rdt = expf(-r * dt);

    float *values = binom_tree(S_0, K, T, sigma, n, is_put);
    if (!values)
        return NAN;

    if (is_american)
    { // SIMD american is complex... TODO
        for (int i = n - 1; i >= 0; i--)
        {
            float S_ij = S_0 * powf(down, i);

            for (int j = 0; j <= i; j++)
            {
                float hold = e_rdt * (p * values[j + 1] + p_inv * values[j]);

                float exercise = is_put ? fmaxf(K - S_ij, 0.0f)
                                        : fmaxf(S_ij - K, 0.0f);

                values[j] = fmaxf(hold, exercise);
                S_ij *= step_up;
            }
        }
    }
    else
    {
#ifdef __AVX__
        __m256 v_p = _mm256_set1_ps(p);
        __m256 v_p_inv = _mm256_set1_ps(p_inv);
        __m256 v_erdt = _mm256_set1_ps(e_rdt);
#endif

        for (int i = n - 1; i >= 0; i--)
        {
            int j = 0;

#ifdef __AVX__
            for (; j <= i - 7; j += 8)
            {
                __m256 v_j = _mm256_loadu_ps(&values[j]);
                __m256 v_j_1 = _mm256_loadu_ps(&values[j + 1]);

#ifdef __FMA__
                __m256 v_v = _mm256_mul_ps(
                    v_erdt,
                    _mm256_fmadd_ps(v_p, v_j_1,
                                    _mm256_mul_ps(v_p_inv, v_j)));
#else
                __m256 v_v = _mm256_mul_ps(
                    v_erdt,
                    _mm256_add_ps(_mm256_mul_ps(v_p, v_j_1),
                                  _mm256_mul_ps(v_p_inv, v_j)));
#endif
                _mm256_storeu_ps(&values[j], v_v);
            }
#endif

            for (; j <= i; j++)
            {
                values[j] = e_rdt * (p * values[j + 1] + p_inv * values[j]);
            }
        }
    }

    float result = values[0];
    free(values);
    return result;
}