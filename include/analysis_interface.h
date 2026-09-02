#pragma once

#include <stdint.h>

typedef struct
{
    double t;
    double dt;
    uint32_t current_step;
    uint32_t N;
    double dz;
    const double *Ex;
    const double *Ey;
    const double *Hx;
    const double *Hy;
    void *user_data;
} analysis_context_t;

// Implement these functions to do any analysis
typedef void *(*analysis_init_func_t)(void);
typedef void (*analysis_process_func_t)(const analysis_context_t *ctx);
typedef void (*analysis_finish_func_t)(const analysis_context_t *ctx);
