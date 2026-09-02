#pragma once

#include <stdint.h>

typedef struct
{
    uint32_t N;
    double dz;
    double dt;
    uint32_t steps;
} sim_config_t;

typedef struct
{
    char *name;
    double eps_r;
    double mu_r;
} material_config_t;

typedef struct
{
    uint32_t start;
    uint32_t end;
    char *material;
} region_config_t;

typedef enum
{
    BC_PEC
} boundary_type_t;

typedef struct
{
    boundary_type_t left_boundary;
    boundary_type_t right_boundary;
} boundary_config_t;

typedef enum
{
    SRC_EX,
    SRC_EY,
    SRC_HX,
    SRC_HY
} source_type_t;

typedef struct
{
    uint32_t pos;
    source_type_t type;
    char *func;
    char *c_path;
} source_config_t;

typedef struct
{
    sim_config_t sim_config;
    boundary_config_t boundary;

    uint32_t num_materials;
    material_config_t *materials;

    uint32_t num_regions;
    region_config_t *regions;

    uint32_t num_sources;
    source_config_t *sources;
} config_t;

int read_config(char *path, config_t *config);
void delete_config(config_t *config);

int validate_config(const config_t *config);

const material_config_t *find_material(const config_t *config, char *name);
