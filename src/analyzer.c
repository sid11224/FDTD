#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "analysis_interface.h"
#include "analyzer.h"
#include "config.h"
#include "log.h"

static int needs_recompile(const char *c_path, const char *so_path)
{
    struct stat c_stat, so_stat;

    if (stat(so_path, &so_stat) != 0)
        return 1;

    if (stat(c_path, &c_stat) == 0)
    {
        return c_stat.st_mtim.tv_sec > so_stat.st_mtim.tv_sec;
    }

    return 0;
}

int load_and_compile_analyzer(analysis_config_t analyzer_config, analyzer_t *analyzer)
{
    char so_path[256];
    char compile_cmd[512];

    *analyzer = (analyzer_t){0};

    strncpy(so_path, analyzer_config.c_path, sizeof(so_path));
    char *dot = strchr(so_path, '.');
    if (dot)
        strcpy(dot, ".so");

    if (needs_recompile(analyzer_config.c_path, so_path))
    {
        snprintf(compile_cmd, sizeof(compile_cmd), "cc -shared -fPIC -O2 -I. -I./include -lm \"%s\" -o \"%s\"",
                 analyzer_config.c_path, so_path);
        LOG_INFO("Compiling analyzer: %s", analyzer_config.c_path);
        int ret = system(compile_cmd);
        if (ret != 0)
        {
            LOG_ERROR("Compilation failed for %s", analyzer_config.c_path);
            return -1;
        }
    }
    else
    {
        LOG_INFO("Skipping compilation because %s already exists", so_path);
    }

    LOG_INFO("Loading analyzer %s", so_path);
    void *handle = dlopen(so_path, RTLD_LAZY);
    if (!handle)
    {
        LOG_ERROR("dlopen failed: %s", dlerror());
        return -1;
    }

    dlerror();
    analysis_init_func_t init = (analysis_init_func_t)dlsym(handle, analyzer_config.init_func);
    char *err = dlerror();
    if (err)
    {
        LOG_ERROR("dlsym failed: %s", err);
        dlclose(handle);
        return -1;
    }
    analysis_process_func_t process = (analysis_process_func_t)dlsym(handle, analyzer_config.process_func);
    err = dlerror();
    if (err)
    {
        LOG_ERROR("dlsym failed: %s", err);
        dlclose(handle);
        return -1;
    }
    analysis_finish_func_t finish = (analysis_finish_func_t)dlsym(handle, analyzer_config.finish_func);
    err = dlerror();
    if (err)
    {
        LOG_ERROR("dlsym failed: %s", err);
        dlclose(handle);
        return -1;
    }

    analyzer->init = init;
    analyzer->process = process;
    analyzer->finish = finish;
    analyzer->handle = handle;

    return 0;
}

void free_analyzer(analyzer_t *analyzer)
{
    dlclose(analyzer->handle);
    analyzer->init = NULL;
    analyzer->process = NULL;
    analyzer->finish = NULL;
}
