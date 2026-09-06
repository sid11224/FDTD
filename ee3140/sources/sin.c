#include <math.h>

#include "source_func.h"

double pulse(const source_context_t *ctx)
{
    const double freq = 1e9;
    const double t0 = 5e-9;
    const double T = 2e-9;

    double amp = exp(-pow((ctx->t - t0) / T, 2));
    double value = amp * sin(2 * M_PI * freq * ctx->t);

    return value;
}

double smooth1(const source_context_t *ctx)
{
    const double freq = 1e9;
    const double T = 1e-9;

    double amp = 1 - exp(-ctx->t / T);
    double value = amp * sin(2 * M_PI * freq * ctx->t);

    return value;
}

double smooth2(const source_context_t *ctx)
{
    const double freq = 1e9;
    const double T = 1e-9;

    const double phi = -M_PI / 8;

    double amp = 1 - exp(-ctx->t / T);
    double value = amp * sin(2 * M_PI * freq * ctx->t + phi);

    return value;
}
