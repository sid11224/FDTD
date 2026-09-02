#pragma once

#include <stdint.h>

#include "config.h"
#include "source_func.h"

typedef struct
{
    uint32_t pos;
    source_type_t type;
    source_func_t func;
    void *handle;
} source_t;

int load_and_compile_source(source_config_t source_config, source_t *src);
void free_source(source_t *src);
