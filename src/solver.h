#pragma once

#include "config.h"
#include "mesh.h"
#include <stdint.h>

typedef struct
{
    uint32_t N;

    double *Ex;
    double *Hy;

    double *Ey;
    double *Hx;
} field_t;

int create_fields(const mesh_t *mesh, field_t *field);
void delete_fields(field_t *field);

void run_solver(field_t *field, const mesh_t *mesh, const config_t *config);
