#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "analysis_interface.h"

typedef struct
{
    FILE *handle;
    int node;
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

    fprintf(plotter->handle, "set term png size 500,500\n");
    fprintf(plotter->handle, "set output 'q3.png'\n");
    fprintf(plotter->handle, "set xrange [-1.5:1.5]\n");
    fprintf(plotter->handle, "set ytics nomirror\n");
    fprintf(plotter->handle, "set yrange [-1.5:1.5]\n");
    fprintf(plotter->handle, "set grid\n");

    fprintf(plotter->handle, "set xlabel 'Ex'\n");
    fprintf(plotter->handle, "set ylabel 'Ey'\n");

    plotter->node = 4000;

    fprintf(plotter->handle, "$P << EOD\n");

    return plotter;
}

void process(const analysis_context_t *ctx)
{
    if (ctx->current_step % 20 != 0)
        return;

    plotter_t *plotter = (plotter_t *)ctx->user_data;
    uint32_t N = ctx->N;
    int pos = plotter->node;

    fprintf(plotter->handle, "%e %e\n", ctx->Ex[pos], ctx->Ey[pos]);
}

void finish(const analysis_context_t *ctx)
{
    plotter_t *plotter = (plotter_t *)ctx->user_data;

    fprintf(plotter->handle, "EOD\n");

    fprintf(plotter->handle, "plot $P u 1:2 w l t ''\n");

    pclose(plotter->handle);
    free(plotter);
}
