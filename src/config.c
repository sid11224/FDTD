#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "log.h"

static int compare_regions(const void *a, const void *b)
{
    const region_config_t *ra = a;
    const region_config_t *rb = b;

    if (ra->start < rb->start)
        return -1;
    else if (ra->start > rb->start)
        return 1;

    return 0;
}

typedef enum
{
    CONF_NONE,
    CONF_SIM,
    CONF_BOUNDARY,
    CONF_MATERIAL,
    CONF_REGION,
    CONF_SOURCE
} section_t;

static char *trim(char *str)
{
    while (isspace((unsigned char)*str))
        str++;

    char *end = str + strlen(str);

    while (end > str && isspace((unsigned char)*(end - 1)))
        end--;

    *end = '\0';

    return str;
}

int read_config(char *path, config_t *config)
{
    FILE *conf = fopen(path, "r");

    LOG_INFO("Reading config from %s", path);

    if (conf == NULL)
    {
        LOG_ERROR("Failed to open file!");
        return -1;
    }

    *config = (config_t){0};

    section_t section = CONF_NONE;
    char *line = (char *)malloc(256 * sizeof(char));

    const char *material_name;

    while (fgets(line, 256, conf))
    {
        if (line[0] == '[')
        {
            char *end = strchr(line, ']');
            if (end == NULL)
            {
                LOG_WARN("Skipping malformed section %s", line);
                section = CONF_NONE;
                continue;
            }

            *end = '\0';

            if (strcmp(line + 1, "simulation") == 0)
            {
                section = CONF_SIM;
            }
            else if (strcmp(line + 1, "boundary") == 0)
            {
                section = CONF_BOUNDARY;
            }
            else if (strncmp(line + 1, "material.", 9) == 0)
            {
                section = CONF_MATERIAL;
                material_name = line + 10;
                config->num_materials++;
                config->materials = realloc(config->materials, config->num_materials * sizeof(material_config_t));
                config->materials[config->num_materials - 1].name = strdup(material_name);
            }
            else if (strcmp(line + 1, "region") == 0)
            {
                section = CONF_REGION;
                config->num_regions++;
                config->regions = realloc(config->regions, config->num_regions * sizeof(region_config_t));
            }
            else if (strcmp(line + 1, "source") == 0)
            {
                section = CONF_SOURCE;
                config->num_sources++;
                config->sources = realloc(config->sources, config->num_sources * sizeof(source_config_t));
            }
            continue;
        }

        char *eq = strchr(line, '=');
        if (eq == NULL)
        {
            continue;
        }

        *eq = '\0';

        char *key = trim(line);
        char *value = trim(eq + 1);

        switch (section)
        {
        case CONF_SIM: {
            if (strcmp(key, "N") == 0)
            {
                config->sim_config.N = atoi(value);
            }
            else if (strcmp(key, "dz") == 0)
            {
                config->sim_config.dz = atof(value);
            }
            else if (strcmp(key, "dt") == 0)
            {
                config->sim_config.dt = atof(value);
            }
            else if (strcmp(key, "steps") == 0)
            {
                config->sim_config.steps = atoi(value);
            }
            else
            {
                LOG_WARN("Unrecognised key %s", key);
            }
            break;
        }
        case CONF_BOUNDARY: {
            if (strcmp(key, "left") == 0)
            {
                if (strcmp(value, "PEC") == 0)
                    config->boundary.left_boundary = BC_PEC;
            }
            else if (strcmp(key, "right") == 0)
            {
                if (strcmp(value, "PEC") == 0)
                    config->boundary.right_boundary = BC_PEC;
            }
            else
            {
                LOG_WARN("Unrecognised key %s", key);
            }
            break;
        }
        case CONF_MATERIAL: {
            if (strcmp(key, "eps_r") == 0)
            {
                config->materials[config->num_materials - 1].eps_r = atof(value);
            }
            else if (strcmp(key, "mu_r") == 0)
            {
                config->materials[config->num_materials - 1].mu_r = atof(value);
            }
            else
            {
                LOG_WARN("Unrecognised key %s", key);
            }
            break;
        }
        case CONF_REGION: {
            if (strcmp(key, "start") == 0)
            {
                config->regions[config->num_regions - 1].start = atoi(value);
            }
            else if (strcmp(key, "end") == 0)
            {
                config->regions[config->num_regions - 1].end = atoi(value);
            }
            else if (strcmp(key, "material") == 0)
            {
                config->regions[config->num_regions - 1].material = strdup(value);
            }
            else
            {
                LOG_WARN("Unrecognised key %s", key);
            }
            break;
        }
        case CONF_SOURCE: {
            if (strcmp(key, "position") == 0)
            {
                config->sources[config->num_sources - 1].pos = atoi(value);
            }
            else if (strcmp(key, "type") == 0)
            {
                if (strcmp(value, "Ex") == 0)
                {
                    config->sources[config->num_sources - 1].type = SRC_EX;
                }
                else if (strcmp(value, "Ey") == 0)
                {
                    config->sources[config->num_sources - 1].type = SRC_EY;
                }
                else if (strcmp(value, "Hx") == 0)
                {
                    config->sources[config->num_sources - 1].type = SRC_HX;
                }
                else if (strcmp(value, "Hy") == 0)
                {
                    config->sources[config->num_sources - 1].type = SRC_HY;
                }
                else
                {
                    LOG_ERROR("Undefined source type %s", value);
                }
            }
            else if (strcmp(key, "file") == 0)
            {
                config->sources[config->num_sources - 1].c_path = strdup(value);
            }
            else if (strcmp(key, "function") == 0)
            {
                config->sources[config->num_sources - 1].func = strdup(value);
            }
            else
            {
                LOG_WARN("Unrecognised key %s", key);
            }
            break;
        }
        default:
            break;
        }
    }

    qsort(config->regions, config->num_regions, sizeof(region_config_t), compare_regions);

    free(line);
    fclose(conf);
    return 0;
}

void delete_config(config_t *config)
{
    if (config == NULL)
        return;

    if (config->materials != NULL)
    {
        for (uint32_t i = 0; i < config->num_materials; ++i)
        {
            free(config->materials[i].name);
        }
        free(config->materials);

        config->num_materials = 0;
        config->materials = NULL;
    }

    if (config->regions != NULL)
    {
        for (uint32_t i = 0; i < config->num_regions; ++i)
        {
            free(config->regions[i].material);
        }
        free(config->regions);

        config->num_regions = 0;
        config->regions = NULL;
    }

    if (config->sources != NULL)
    {
        for (uint32_t i = 0; i < config->num_sources; ++i)
        {
            free(config->sources[i].func);
            free(config->sources[i].c_path);
        }
        free(config->sources);

        config->num_sources = 0;
        config->sources = NULL;
    }
}

static int validate_sim(const config_t *config)
{
    sim_config_t sim_config = config->sim_config;

    if (sim_config.N < 3)
    {
        LOG_ERROR("N must be atleast 3!");
        return -1;
    }

    if (sim_config.dz < 0.0 || isnan(sim_config.dz) || isinf(sim_config.dz))
    {
        LOG_ERROR("dz must be a finite positive quantity!");
        return -1;
    }

    if (sim_config.dt < 0.0 || isnan(sim_config.dt) || isinf(sim_config.dt))
    {
        LOG_ERROR("dt must be a finite positive quantity!");
        return -1;
    }

    if (sim_config.steps < 1)
    {
        LOG_ERROR("steps must be atleast 1!");
        return -1;
    }

    const double c = 299792458.0f;

    if (c * sim_config.dt > 0.9 * sim_config.dz)
    {
        LOG_ERROR("c*dt must be less that 0.9dz!");
        return -1;
    }

    return 0;
}

static int validate_materials(const config_t *config)
{
    if (config->num_materials < 1)
    {
        LOG_ERROR("There must be atleast 1 material!");
        return -1;
    }

    return 0;
}

static int validate_regions(const config_t *config)
{
    if (config->num_regions < 1)
    {
        LOG_ERROR("There must be atleast 1 region!");
        return -1;
    }

    for (uint32_t i = 0; i < config->num_regions; ++i)
    {
        if (config->regions[i].start >= config->regions[i].end)
        {
            LOG_ERROR("Region start is after end! start: %u end: %u", config->regions[i].start, config->regions[i].end);
            return -1;
        }

        if (config->regions[i].end > config->sim_config.N)
        {
            LOG_ERROR("Region end is outside simulation domain! end: %u", config->regions[i].end);
            return -1;
        }

        if (find_material(config, config->regions[i].material) == NULL)
        {
            LOG_ERROR("Material %s isnt defined!", config->regions[i].material);
            return -1;
        }
    }

    if (config->regions[0].start != 0)
    {
        LOG_ERROR("Cell 0 isnt covered!");
        return -1;
    }

    for (uint32_t i = 1; i < config->num_regions; ++i)
    {
        if (config->regions[i].start != config->regions[i - 1].end)
        {
            LOG_ERROR("Cell %u isnt covered!", config->regions[i - 1].end);
            return -1;
        }
    }

    if (config->regions[config->num_regions - 1].end != config->sim_config.N)
    {
        LOG_ERROR("Cell %u isnt covered!", config->sim_config.N - 1);
        return -1;
    }

    return 0;
}

int validate_sources(const config_t *config)
{
    if (config->num_sources < 1)
        return 0;

    for (int i = 0; i < config->num_sources; ++i)
    {
        if (config->sources[i].pos == 0)
        {
            LOG_ERROR("Cannot have source at 0");
            return -1;
        }
        else if (config->sources[i].pos >= config->sim_config.N)
        {
            LOG_ERROR("Cannot have source at or beyond %u", config->sim_config.N);
            return -1;
        }

        const char *c_file = config->sources[i].c_path;
        if (!c_file || access(c_file, F_OK) != 0)
        {
            LOG_ERROR("Source file does not exist or is unreadable: '%s'", c_file ? c_file : "NULL");
            return -1;
        }

        if (!config->sources[i].func || strlen(config->sources[i].func) == 0)
        {
            LOG_ERROR("Source at position %d has no function specified", config->sources[i].pos);
            return -1;
        }
    }

    return 0;
}

int validate_config(const config_t *config)
{
    LOG_INFO("Validating config...");
    if (validate_sim(config) != 0)
        return -1;

    if (validate_materials(config) != 0)
        return -1;

    if (validate_regions(config) != 0)
        return -1;

    if (validate_sources(config) != 0)
        return -1;

    return 0;
}

const material_config_t *find_material(const config_t *config, char *name)
{
    for (int i = 0; i < config->num_materials; ++i)
    {
        if (strcmp(config->materials[i].name, name) == 0)
            return &config->materials[i];
    }

    return NULL;
}
