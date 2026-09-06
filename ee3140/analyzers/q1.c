#include <stdlib.h>

#include "analysis_interface.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    int node;
    double impedance;
    int N;
} impedance_probe_t;

typedef enum
{
    WAITING,
    TRACKING,
    LOCKED
} velocity_probe_state_t;

typedef struct
{
    int node;
    velocity_probe_state_t state;
    double max_val;
    double t_peak;
} velocity_probe_t;

typedef struct
{
    velocity_probe_t v_p1;
    velocity_probe_t v_p2;
    impedance_probe_t i_p;
    double threshold;

    FILE *plotter;
    float minX;
    float maxX;
} q1_t;

void *init(void)
{
    q1_t *q1 = (q1_t *)malloc(sizeof(q1_t));

    q1->plotter = popen("gnuplot -persist", "w");

    if (!q1->plotter)
    {
        perror("gnuplot");
        return NULL;
    }

    fprintf(q1->plotter, "set term gif animate delay 3 loop 0 size 800,500\n");
    fprintf(q1->plotter, "set output 'q1.gif'\n");
    fprintf(q1->plotter, "set key top center\n");

    fprintf(q1->plotter, "set xrange [-3:3]\n");
    fprintf(q1->plotter, "set ytics nomirror\n");
    fprintf(q1->plotter, "set y2tics\n");
    fprintf(q1->plotter, "set yrange [-1.5:1.5]\n");
    fprintf(q1->plotter, "set y2range [-0.004:0.004]\n");

    fprintf(q1->plotter, "set ylabel 'Electric field'\n");
    fprintf(q1->plotter, "set y2label 'Magnetic field'\n");

    fprintf(q1->plotter, "set grid\n");

    q1->minX = -3.0;
    q1->maxX = 3.0;

    q1->threshold = 1e-2;
    q1->v_p1.node = 4000;
    q1->v_p1.state = WAITING;
    q1->v_p1.max_val = 0.0;
    q1->v_p1.t_peak = 0.0;

    q1->v_p2.node = 5300;
    q1->v_p2.state = WAITING;
    q1->v_p2.max_val = 0.0;
    q1->v_p2.t_peak = 0.0;

    q1->i_p.node = 4000;
    q1->i_p.impedance = 0.0;
    q1->i_p.N = 0.0;

    return q1;
}

void update_impedance_probe(const analysis_context_t *ctx, q1_t *q1)
{
    impedance_probe_t *iprobe = &q1->i_p;
    int pos = iprobe->node;

    if (ctx->Ey[pos] < q1->threshold)
        return;

    double E = ctx->Ey[pos];
    double H = (ctx->Hx[pos] + ctx->Hx[pos - 1]) * 0.5;

    (iprobe->N)++;
    iprobe->impedance = ((iprobe->N - 1) * iprobe->impedance + fabs(E / H)) / iprobe->N;
}

void update_velocity_probe(velocity_probe_t *vp, double field_val, double t, double threshold)
{
    if (vp->state == LOCKED)
        return;

    double val = fabs(field_val);

    if (vp->state == WAITING)
    {
        if (val > threshold)
        {
            vp->state = TRACKING;
            vp->max_val = val;
            vp->t_peak = t;
        }
    }
    else if (vp->state == TRACKING)
    {
        if (val > vp->max_val)
        {
            vp->max_val = val;
            vp->t_peak = t;
        }
        else if (val < 0.5 * vp->max_val)
        {
            vp->state = LOCKED;
        }
    }
}

void update_velocity_probes(const analysis_context_t *ctx, q1_t *q1)
{
    velocity_probe_t *vprobe_1 = &q1->v_p1;
    velocity_probe_t *vprobe_2 = &q1->v_p2;

    update_velocity_probe(vprobe_1, ctx->Ey[vprobe_1->node], ctx->t, q1->threshold);
    update_velocity_probe(vprobe_2, ctx->Ey[vprobe_2->node], ctx->t, q1->threshold);

    if (ctx->current_step % 300 != 0)
        return;

    uint32_t N = ctx->N;
    FILE *plotter = q1->plotter;

    fprintf(plotter, "$E << EOD\n");
    for (int i = 0; i < N + 1; ++i)
    {
        double x = i * ctx->dz + q1->minX;
        double y = ctx->Ey[i];

        fprintf(plotter, "%e %e\n", x, y);
    }
    fprintf(plotter, "EOD\n");

    fprintf(plotter, "$H << EOD\n");
    for (int i = 0; i < N; ++i)
    {
        double x = (i + 0.5) * ctx->dz + q1->minX;
        double y = ctx->Hx[i];

        fprintf(plotter, "%e %e\n", x, y);
    }
    fprintf(plotter, "EOD\n");

    fprintf(plotter, "plot $E u 1:2 axes x1y1 w l smooth unique t 'Ey', "
                     "$H u 1:2 dt 2 axes x1y2 w l smooth unique t 'Hx'\n");
}

void process(const analysis_context_t *ctx)
{
    q1_t *q1 = (q1_t *)ctx->user_data;

    update_impedance_probe(ctx, q1);
    update_velocity_probes(ctx, q1);
}

void finish(const analysis_context_t *ctx)
{
    q1_t *q1 = (q1_t *)ctx->user_data;

    const impedance_probe_t *iprobe = &q1->i_p;
    const velocity_probe_t *vprobe_1 = &q1->v_p1;
    const velocity_probe_t *vprobe_2 = &q1->v_p2;

    double delta_z = (vprobe_2->node - vprobe_1->node) * ctx->dz;
    double delta_t = vprobe_2->t_peak - vprobe_1->t_peak;
    double v = delta_z / delta_t;

    printf("\n=== Velocity Results ===\n");
    printf("Probe 1 (Node %d): Locked Peak at t = %.6e s\n", vprobe_1->node, vprobe_1->t_peak);
    printf("Probe 2 (Node %d): Locked Peak at t = %.6e s\n", vprobe_2->node, vprobe_2->t_peak);
    printf("  Measured Velocity : %.6e m/s\n", v);
    printf("=========================\n");

    printf("\n=== Impedance Results ===\n");
    printf("Probe 1 (Node %d)\n", iprobe->node);
    printf("  Measured Impedance : %.6e Ohm\n", iprobe->impedance);
    printf("=========================\n");

    printf("\nSaving simulation animation to 'q1.gif'\n");

    fprintf(q1->plotter, "unset output\n");
    pclose(q1->plotter);
    free(q1);
}
