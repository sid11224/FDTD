#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"
#include "log.h"
#include "source.h"
#include "source_func.h"

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

int load_and_compile_source(source_config_t source_config, source_t *src)
{
    char so_path[256];
    char compile_cmd[512];

    *src = (source_t){0};

    strncpy(so_path, source_config.c_path, sizeof(so_path));
    char *dot = strchr(so_path, '.');
    if (dot)
        strcpy(dot, ".so");

    if (needs_recompile(source_config.c_path, so_path))
    {
        snprintf(compile_cmd, sizeof(compile_cmd), "cc -shared -fPIC -O2 -I. -I./include -lm \"%s\" -o \"%s\"",
                 source_config.c_path, so_path);
        LOG_INFO("Compiling source: %s", source_config.c_path);
        int ret = system(compile_cmd);
        if (ret != 0)
        {
            LOG_ERROR("Compilation failed for %s", source_config.c_path);
            return -1;
        }
    }
    else
    {
        LOG_INFO("Skipping compilation because %s already exists", so_path);
    }

    LOG_INFO("Loading source %s", so_path);
    void *handle = dlopen(so_path, RTLD_LAZY);
    if (!handle)
    {
        LOG_ERROR("dlopen failed: %s", dlerror());
        return -1;
    }

    dlerror();
    source_func_t func = (source_func_t)dlsym(handle, source_config.func);
    char *err = dlerror();
    if (err)
    {
        LOG_ERROR("dlsym failed: %s", err);
        dlclose(handle);
        return -1;
    }

    src->pos = source_config.pos;
    src->type = source_config.type;
    src->handle = handle;
    src->func = func;

    return 0;
}

void free_source(source_t *src)
{
    dlclose(src->handle);
    src->func = NULL;
    src->pos = 0;
}
