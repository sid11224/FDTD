#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "analysis_interface.h"
#include "analyzer.h"
#include "config.h"
#include "log.h"
#include "mesh.h"
#include "solver.h"
#include "source.h"
#include "source_func.h"

int create_fields(const mesh_t *mesh, field_t *field)
{
    LOG_INFO("Initializing fields...");

    field->N = mesh->N;
    field->Ex = (double *)calloc(field->N + 1, sizeof(double));
    if (field->Ex == NULL)
    {
        return -1;
    }
    field->Ey = (double *)calloc(field->N + 1, sizeof(double));
    if (field->Ey == NULL)
    {
        free(field->Ex);
        field->Ex = NULL;
        return -1;
    }
    field->Hx = (double *)calloc(field->N, sizeof(double));
    if (field->Hx == NULL)
    {
        free(field->Ex);
        free(field->Ey);
        field->Ex = NULL;
        field->Ey = NULL;
        return -1;
    }
    field->Hy = (double *)calloc(field->N, sizeof(double));
    if (field->Hy == NULL)
    {
        free(field->Ex);
        free(field->Ey);
        free(field->Hx);
        field->Ex = NULL;
        field->Ey = NULL;
        field->Hx = NULL;
        return -1;
    }

    return 0;
}

void delete_fields(field_t *field)
{
    if (field == NULL)
        return;

    if (field->Ex != NULL)
    {
        free(field->Ex);
        field->Ex = NULL;
    }
    if (field->Ey != NULL)
    {
        free(field->Ey);
        field->Ey = NULL;
    }
    if (field->Hx != NULL)
    {
        free(field->Hx);
        field->Hx = NULL;
    }
    if (field->Hy != NULL)
    {
        free(field->Hy);
        field->Hy = NULL;
    }

    field->N = 0;
}

/* -- Maxwell's Equations --
 *	V.D = p         => e(dEx/dx + dEy/dy + dEz/dz) = p
 *	V.B = 0         => e(dEx/dx + dEy/dy + dEz/dz) = p
 *	VxE = -dB/dt    => (dEz/dy - dEy/dz) = -dBx/dt, (dEx/dz - dEz/dx) = -dBy/dt, (dEy/dx - dEx/dy) = -dBz/dt
 *	VxH = J + dD/dt => (dHz/dy - dHy/dz) = Jx + dDx/dt, (dHx/dz - dHz/dx) = Jy + dDy/dt, (dHy/dx - dHx/dy) = Jz + dDz/dt
 */

// For 1D along z
/* -- Maxwell's Equations --
 *	V.D = p         => dDz/dz = p
 *	V.B = 0         => dBz/dz = 0
 *	VxE = -dB/dt    => dEy/dz = dBx/dt,        dEx/dz = -dBy/dt,     dBz/dt = 0
 *	VxH = J + dD/dt => dHy/dz = -Jx + -dDx/dt, dHx/dz = Jy + dDy/dt, Jz + dDz/dt = 0
 */

static void solve_step(field_t *field, const mesh_t *mesh, const boundary_config_t *boundary)
{
    for (uint32_t i = 0; i < mesh->N; ++i)
    {
        field->Hx[i] += ((field->Ey[i + 1] - field->Ey[i]) / mesh->dz) * (mesh->dt / mesh->mu[i]);
        field->Hy[i] -= ((field->Ex[i + 1] - field->Ex[i]) / mesh->dz) * (mesh->dt / mesh->mu[i]);
    }

    for (uint32_t i = 1; i < mesh->N; ++i)
    {
        double eps = 0.5 * (mesh->eps[i] + mesh->eps[i - 1]);
        field->Ex[i] -= ((field->Hy[i] - field->Hy[i - 1]) / mesh->dz) * (mesh->dt / eps);
        field->Ey[i] += ((field->Hx[i] - field->Hx[i - 1]) / mesh->dz) * (mesh->dt / eps);
    }

    if (boundary->left_boundary == BC_PEC)
    {
        field->Ex[0] = 0.0f;
        field->Ey[0] = 0.0f;
    }

    if (boundary->right_boundary == BC_PEC)
    {
        field->Ex[mesh->N] = 0.0f;
        field->Ey[mesh->N] = 0.0f;
    }
}

void apply_source(field_t *field, const source_t *src, const source_context_t *src_ctx)
{
    double value = src->func(src_ctx);
    if (fabs(value) < 2e-3)
        return;

    switch (src->type)
    {
    case SRC_EX:
        field->Ex[src->pos] = value;
        break;
    case SRC_EY:
        field->Ey[src->pos] = value;
        break;
    case SRC_HX:
        field->Hx[src->pos] = value;
        break;
    case SRC_HY:
        field->Hy[src->pos] = value;
        break;
    dedefault:
        break;
    }
}

void run_solver(field_t *field, const mesh_t *mesh, const config_t *config)
{
    uint32_t steps = config->sim_config.steps;
    boundary_config_t boundary = config->boundary;

    uint32_t num_srcs = config->num_sources;
    source_t *srcs = (source_t *)malloc(num_srcs * sizeof(source_t));

    LOG_INFO("Compiling sources...");

    for (uint32_t i = 0; i < num_srcs; ++i)
    {
        load_and_compile_source(config->sources[i], &srcs[i]);
    }

    uint32_t num_analyzers = config->num_analyzers;
    analyzer_t *analyzers = (analyzer_t *)malloc(num_analyzers * sizeof(analyzer_t));

    LOG_INFO("Compiling analyzers...");

    for (uint32_t i = 0; i < num_analyzers; ++i)
    {
        load_and_compile_analyzer(config->analyzers[i], &analyzers[i]);
        analyzers[i].user_data = analyzers[i].init();
    }

    LOG_INFO("Running simulation for %u timesteps", steps);

    double dt = mesh->dt;

    source_context_t src_ctx;
    src_ctx.dt = dt;

    analysis_context_t analysis_ctx;
    analysis_ctx.dt = dt;
    analysis_ctx.N = mesh->N;
    analysis_ctx.dz = mesh->dz;
    analysis_ctx.Ex = field->Ex;
    analysis_ctx.Ey = field->Ey;
    analysis_ctx.Hx = field->Hx;
    analysis_ctx.Hy = field->Hy;

    for (uint32_t i = 0; i < steps; ++i)
    {
        double t = i * dt;

        src_ctx.t = t;
        analysis_ctx.t = t;
        analysis_ctx.current_step = i;

        for (int j = 0; j < num_srcs; ++j)
        {
            apply_source(field, &srcs[j], &src_ctx);
        }

        for (uint32_t j = 0; j < num_analyzers; ++j)
        {
            analysis_ctx.user_data = analyzers[j].user_data;
            analyzers[j].process(&analysis_ctx);
        }

        solve_step(field, mesh, &boundary);
    }

    for (uint32_t j = 0; j < num_analyzers; ++j)
    {
        analysis_ctx.user_data = analyzers[j].user_data;
        analyzers[j].finish(&analysis_ctx);
        free_analyzer(&analyzers[j]);
    }

    for (int i = 0; i < config->num_sources; ++i)
        free_source(&srcs[i]);

    free(srcs);
    free(analyzers);
}
