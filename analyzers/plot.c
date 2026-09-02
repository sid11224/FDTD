#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "analysis_interface.h"

typedef struct
{
    FILE *handle;
    float minX;
    float maxX;
} plotter_t;

void *init(void)
{
    plotter_t *plotter = (plotter_t *)malloc(sizeof(plotter_t));
    plotter->handle = popen("gnuplot -persist", "w");

    if (!plotter->handle)
    {
        perror("gnuplot");
        return NULL;
    }

    fprintf(plotter->handle, "set term gif animate delay 10\n");
    fprintf(plotter->handle, "set output 'q1.gif'\n");
    fprintf(plotter->handle, "set xrange [-3:3]\n");
    fprintf(plotter->handle, "set ytics nomirror\n");
    fprintf(plotter->handle, "set yrange [-1.5:1.5]\n");
    fprintf(plotter->handle, "set grid\n");

    plotter->minX = -3.0;
    plotter->maxX = 3.0;
    return plotter;
}

void process(const analysis_context_t *ctx)
{
    if (ctx->current_step % 10 != 0)
        return;

    plotter_t *plotter = (plotter_t *)ctx->user_data;
    uint32_t N = ctx->N;

    fprintf(plotter->handle, "$E << EOD\n");
    for (int i = 0; i < N; i++)
    {
        double x = i * ctx->dz + plotter->minX;
        double y = ctx->Ey[i];

        fprintf(plotter->handle, "%e %e\n", x, y);
    }
    fprintf(plotter->handle, "EOD\n");
    fprintf(plotter->handle, "plot $E u 1:2 axes x1y1 w l smooth unique t 'E'\n");
}

void finish(const analysis_context_t *ctx)
{
    plotter_t *plotter = (plotter_t *)ctx->user_data;
    pclose(plotter->handle);
    free(plotter);
}
