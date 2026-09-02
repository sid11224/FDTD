#include <stdint.h>
#include <stdio.h>

#include "log.h"
#include "mesh.h"

int create_mesh(const config_t *config, mesh_t *mesh)
{
    LOG_INFO("Generating mesh...");
    uint32_t N = config->sim_config.N;

    *mesh = (mesh_t){0};

    mesh->N = config->sim_config.N;
    mesh->dz = config->sim_config.dz;
    mesh->dt = config->sim_config.dt;

    mesh->eps = (double *)malloc(N * sizeof(double));
    if (mesh->eps == NULL)
    {
        return -1;
    }

    mesh->mu = (double *)malloc(N * sizeof(double));
    if (mesh->mu == NULL)
    {
        free(mesh->eps);

        mesh->eps = NULL;
        return -1;
    }

    for (uint32_t r = 0; r < config->num_regions; ++r)
    {
        const region_config_t *region = &config->regions[r];
        const material_config_t *material = find_material(config, region->material);

        if (material == NULL)
        {
            LOG_ERROR("Material '%s' not found", region->material);

            free(mesh->eps);
            free(mesh->mu);

            mesh->eps = NULL;
            mesh->mu = NULL;

            return -1;
        }

        for (uint32_t i = region->start; i <= region->end; ++i)
        {
            mesh->eps[i] = material->eps_r * eps0;
            mesh->mu[i] = material->mu_r * mu0;
        }
    }

    return 0;
}

void delete_mesh(mesh_t *mesh)
{
    if (mesh == NULL)
        return;

    if (mesh->eps != NULL)
    {
        free(mesh->eps);
        mesh->eps = NULL;
    }

    if (mesh->mu != NULL)
    {
        free(mesh->mu);
        mesh->mu = NULL;
    }
}
