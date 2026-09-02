#include <stdio.h>

#include "config.h"
#include "log.h"
#include "mesh.h"
#include "solver.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        LOG_INFO("Usage: %s <config file path>", argv[0]);
        return 0;
    }

    config_t config;
    if (read_config(argv[1], &config) != 0)
    {
        return -1;
    }
    validate_config(&config);

    mesh_t mesh;
    if (create_mesh(&config, &mesh) != 0)
    {
        delete_config(&config);
        return -1;
    }

    field_t field;
    if (create_fields(&mesh, &field) != 0)
    {
        delete_config(&config);
        delete_mesh(&mesh);
        return -1;
    }

    run_solver(&field, &mesh, &config);

    delete_fields(&field);
    delete_mesh(&mesh);
    delete_config(&config);
}
