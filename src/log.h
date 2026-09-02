#pragma once

#include <stdio.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL (3)
#endif

#define DEBUG (3)
#define INFO (2)
#define WARNING (1)
#define ERROR (0)

#define INFOCOLOR "\x1b[37m"
#define WARNCOLOR "\x1b[33m"
#define ERRORCOLOR "\x1b[31m"
#define RESETCOLOR "\x1b[39m"

#define LOG_DEBUG(...)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        fprintf(stdout, INFOCOLOR "DEBUG: ");                                                                          \
        fprintf(stdout, __VA_ARGS__);                                                                                  \
        fprintf(stdout, RESETCOLOR "\n");                                                                              \
    } while (0)
#define LOG_INFO(...)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        fprintf(stdout, INFOCOLOR "INFO: ");                                                                           \
        fprintf(stdout, __VA_ARGS__);                                                                                  \
        fprintf(stdout, RESETCOLOR "\n");                                                                              \
    } while (0)
#define LOG_WARN(...)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        fprintf(stdout, WARNCOLOR "WARNING: ");                                                                        \
        fprintf(stdout, __VA_ARGS__);                                                                                  \
        fprintf(stdout, RESETCOLOR "\n");                                                                              \
    } while (0)
#define LOG_ERROR(...)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        fprintf(stdout, ERRORCOLOR "ERROR: ");                                                                         \
        fprintf(stdout, __VA_ARGS__);                                                                                  \
        fprintf(stdout, RESETCOLOR "\n");                                                                              \
    } while (0)

#if LOG_LEVEL < DEBUG
#undef LOG_DEBUG
#define LOG_DEBUG(...) ((void)0)
#endif

#if LOG_LEVEL < INFO
#undef LOG_INFO
#define LOG_INFO(...) ((void)0)
#endif

#if LOG_LEVEL < WARNING
#undef LOG_WARN
#define LOG_WARN(...) (void(0))
#endif
