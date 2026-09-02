#pragma once

#include <stdint.h>

#include "analysis_interface.h"
#include "config.h"

typedef struct
{
    analysis_init_func_t init;
    analysis_process_func_t process;
    analysis_finish_func_t finish;
    void *handle;
    void *user_data;
} analyzer_t;

int load_and_compile_analyzer(analysis_config_t analyzer_config, analyzer_t *analyzer);
void free_analyzer(analyzer_t *analyzer);
