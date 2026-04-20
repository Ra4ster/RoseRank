#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#define _POSIX_C_SOURCE 199309L

static inline struct timespec now()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t;
}

static inline double stopwatch(struct timespec t1, struct timespec t2)
{
    return (t2.tv_sec - t1.tv_sec) +
           (t2.tv_nsec - t1.tv_nsec) / 1e9;
}

static inline double test_timing(void *fn, void *args)
{
    struct timespec start = now();

    float (*func)(void *) = (float (*)(void *))fn;
    float thing = func(args);
    printf("Output: %f.\n", thing);

    struct timespec end = now();

    return stopwatch(start, end);
}

float rand_percent(void);

static inline void rand_fill(float *x, int n)
{
    srand(time(NULL));

    for (int i = 0; i < n; i++)
    {
        x[i] = 10.0f + ((float)rand() / RAND_MAX) * 1000.0f;
    }
}

static inline void rand_walk(float *x, int n, float c)
{
    srand(time(NULL));
    x[0] = 48.0f;
    for (int i = 1; i < n; i++)
    {
        // Change price by a small random percentage
        float change = c + (((float)rand() / RAND_MAX) * 0.002f - 0.001f);
        x[i] = x[i - 1] * change;
    }
}