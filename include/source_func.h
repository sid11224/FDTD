#pragma once

typedef struct
{
    double t;
    double dt;
} source_context_t;

// Implement this function to apply a source
typedef double (*source_func_t)(source_context_t ctx);
