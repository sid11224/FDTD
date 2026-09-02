#pragma once

#include <stdint.h>
#include <stdlib.h>

#include "config.h"

#define eps0 8.8541878128e-12
#define mu0 1.25663706127e-6

typedef struct
{
    uint32_t N;
    double dz;
    double dt;

    double *eps;
    double *mu;
} mesh_t;

int create_mesh(const config_t *config, mesh_t *mesh);
void delete_mesh(mesh_t *mesh);
