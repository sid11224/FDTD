#include <stdlib.h>

#include "analysis_interface.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum
{
    WAIT_INCIDENT,
    TRACK_INCIDENT,
    WAIT_REFLECTED,
    TRACK_REFLECTED,
    PROBE_DONE
} reflection_probe_state_t;

typedef enum
{
    WAIT_TRANSMIT,
    TRACK_TRANSMIT,
    DONE
} transmission_probe_state_t;

typedef struct
{
    int node;
    reflection_probe_state_t state;
    double max_inc;
    double max_ref;
    double t_peak_inc;
    double t_peak_ref;
} reflection_probe_t;

typedef struct
{
    int node;
    transmission_probe_state_t state;
    double max_trans;
    double t_peak_trans;
} transmission_probe_t;

typedef struct
{
    int node;
    double min;
    double max;
    double energy;
} energy_probe_t;

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

    energy_probe_t e_p1;
    energy_probe_t e_p2;

    reflection_probe_t r_p;
    transmission_probe_t t_p;

    FILE *plotter1;
    FILE *plotter2;
    float minX;
    float maxX;
} q2_t;

void *init(void)
{
    q2_t *q2 = (q2_t *)malloc(sizeof(q2_t));

    q2->plotter1 = popen("gnuplot -persist", "w");
    q2->plotter2 = popen("gnuplot -persist", "w");

    if (!q2->plotter1)
    {
        perror("gnuplot");
        return NULL;
    }
    if (!q2->plotter2)
    {
        perror("gnuplot");
        return NULL;
    }

    fprintf(q2->plotter1, "set key top center\n");

    fprintf(q2->plotter1, "set term gif animate delay 3 loop 0 size 800,500\n");
    fprintf(q2->plotter1, "set output 'q2.gif'\n");
    fprintf(q2->plotter1, "set xrange [-3:3]\n");
    fprintf(q2->plotter1, "set ytics nomirror\n");
    fprintf(q2->plotter1, "set y2tics\n");
    fprintf(q2->plotter1, "set yrange [-1.5:1.5]\n");
    fprintf(q2->plotter1, "set y2range [-0.004:0.004]\n");

    fprintf(q2->plotter1, "set ylabel 'Electric field'\n");
    fprintf(q2->plotter1, "set y2label 'Magnetic field'\n");

    fprintf(q2->plotter1, "set grid\n");

    fprintf(q2->plotter1, "set obj 1 rect center 2.5,0 size 1,20 fc rgb 0xbbbbbb fs solid 0.4 behind\n");

    fprintf(q2->plotter2, "set key top center\n");

    fprintf(q2->plotter2, "set term gif animate delay 3 loop 0 size 800,500\n");
    fprintf(q2->plotter2, "set output 'q2-p.gif'\n");
    fprintf(q2->plotter2, "set xrange [-3:3]\n");
    fprintf(q2->plotter2, "set ytics nomirror\n");
    fprintf(q2->plotter2, "set yrange [-0.004:0.004]\n");
    fprintf(q2->plotter2, "set grid\n");

    fprintf(q2->plotter2, "set obj 1 rect center 2.5,0 size 1,20 fc rgb 0xbbbbbb fs solid 0.4 behind\n");

    q2->minX = -3.0;
    q2->maxX = 3.0;

    q2->threshold = 1e-2;

    q2->v_p1.node = 5100;
    q2->v_p2.node = 5300;
    q2->v_p1.state = q2->v_p2.state = WAITING;
    q2->v_p1.max_val = q2->v_p2.max_val = 0.0;
    q2->v_p1.t_peak = q2->v_p2.t_peak = 0.0;

    q2->i_p.node = 5100;
    q2->i_p.impedance = 0.0;
    q2->i_p.N = 0.0;

    q2->e_p1.node = 4990;
    q2->e_p2.node = 5010;
    q2->e_p1.min = q2->e_p2.min = 0.0;
    q2->e_p1.max = q2->e_p2.max = 0.0;
    q2->e_p1.energy = q2->e_p2.energy = 0.0;

    q2->r_p.node = 3500;
    q2->r_p.state = WAIT_INCIDENT;
    q2->r_p.max_inc = 0.0;
    q2->r_p.max_ref = 0.0;
    q2->r_p.t_peak_inc = 0.0;
    q2->r_p.t_peak_ref = 0.0;

    q2->t_p.node = 5100;
    q2->t_p.state = WAIT_TRANSMIT;
    q2->t_p.max_trans = 0.0;
    q2->t_p.t_peak_trans = 0.0;

    return q2;
}

void update_impedance_probe(const analysis_context_t *ctx, q2_t *q2)
{
    impedance_probe_t *iprobe = &q2->i_p;
    int pos = iprobe->node;

    if (ctx->Ey[pos] < q2->threshold)
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

void update_energy_probes(const analysis_context_t *ctx, q2_t *q2)
{
    int pos1 = q2->e_p1.node;
    int pos2 = q2->e_p2.node;

    double E1 = ctx->Ey[pos1];
    double E2 = ctx->Ey[pos2];
    double H1 = (ctx->Hx[pos1] + ctx->Hx[pos1 - 1]) * 0.5;
    double H2 = (ctx->Hx[pos2] + ctx->Hx[pos2 - 1]) * 0.5;

    double S1 = -E1 * H1;
    double S2 = -E2 * H2;

    if (S1 > q2->e_p1.max)
        q2->e_p1.max = S1;
    if (S1 < q2->e_p1.min)
        q2->e_p1.min = S1;

    if (S2 > q2->e_p2.max)
        q2->e_p2.max = S2;
    if (S2 < q2->e_p2.min)
        q2->e_p2.min = S2;

    q2->e_p1.energy += S1 * ctx->dt;
    q2->e_p2.energy += S2 * ctx->dt;
}

void update_reflection_probe(reflection_probe_t *p, double field_val, double current_t, double threshold)
{
    double abs_val = fabs(field_val);

    switch (p->state)
    {
    case WAIT_INCIDENT:
        if (abs_val > threshold)
        {
            p->state = TRACK_INCIDENT;
            p->max_inc = abs_val;
            p->t_peak_inc = current_t;
        }
        break;

    case TRACK_INCIDENT:
        if (abs_val > p->max_inc)
        {
            p->max_inc = abs_val;
            p->t_peak_inc = current_t;
        }
        if (current_t > 1e-8)
        {
            p->state = WAIT_REFLECTED;
        }
        break;

    case WAIT_REFLECTED:
        if (abs_val > threshold)
        {
            p->state = TRACK_REFLECTED;
            p->max_ref = abs_val;
            p->t_peak_ref = current_t;
        }
        break;

    case TRACK_REFLECTED:
        if (abs_val > p->max_ref)
        {
            p->max_ref = abs_val;
            p->t_peak_ref = current_t;
        }
        if (current_t > 1.65e-8)
        {
            p->state = PROBE_DONE;
        }
        break;

    case PROBE_DONE:
        break;
    }
}

void update_transmission_probe(transmission_probe_t *p, double field_val, double current_t, double threshold)
{
    double abs_val = fabs(field_val);

    if (current_t < 1e-8)
    {
        return;
    }

    switch (p->state)
    {
    case WAIT_TRANSMIT:
        if (abs_val > threshold)
        {
            p->state = TRACK_TRANSMIT;
            p->max_trans = abs_val;
            p->t_peak_trans = current_t;
        }
        break;

    case TRACK_TRANSMIT:
        if (abs_val > p->max_trans)
        {
            p->max_trans = abs_val;
            p->t_peak_trans = current_t;
        }
        if (current_t > 1.65e-8)
        {
            return;
        }
        break;

    case DONE:
        break;
    }
}

void process(const analysis_context_t *ctx)
{
    q2_t *q2 = (q2_t *)ctx->user_data;

    update_impedance_probe(ctx, q2);

    update_velocity_probe(&q2->v_p1, ctx->Ey[q2->v_p1.node], ctx->t, q2->threshold);
    update_velocity_probe(&q2->v_p2, ctx->Ey[q2->v_p2.node], ctx->t, q2->threshold);

    update_energy_probes(ctx, q2);

    update_reflection_probe(&q2->r_p, ctx->Ey[q2->r_p.node], ctx->t, q2->threshold);
    update_transmission_probe(&q2->t_p, ctx->Ey[q2->t_p.node], ctx->t, q2->threshold);

    if (ctx->current_step % 300 != 0)
        return;

    uint32_t N = ctx->N;
    FILE *plotter1 = q2->plotter1;
    FILE *plotter2 = q2->plotter2;

    fprintf(plotter1, "$E << EOD\n");
    for (int i = 0; i < N + 1; ++i)
    {
        double x = i * ctx->dz + q2->minX;
        double y = ctx->Ey[i];

        fprintf(plotter1, "%e %e\n", x, y);
    }
    fprintf(plotter1, "EOD\n");

    fprintf(plotter1, "$H << EOD\n");
    for (int i = 0; i < N; ++i)
    {
        double x = (i + 0.5) * ctx->dz + q2->minX;
        double y = ctx->Hx[i];

        fprintf(plotter1, "%e %e\n", x, y);
    }
    fprintf(plotter1, "EOD\n");

    fprintf(plotter2, "$S << EOD\n");
    for (int i = 1; i < N; ++i)
    {
        double x = i * ctx->dz + q2->minX;
        double H = 0.5 * (ctx->Hx[i] + ctx->Hx[i - 1]);
        double E = ctx->Ey[i];

        double S = -E * H;

        fprintf(plotter2, "%e %e\n", x, S);
    }
    fprintf(plotter2, "EOD\n");

    fprintf(plotter1, "plot $E u 1:2 axes x1y1 w l smooth unique t 'Ey', "
                      "$H u 1:2 dt 2 axes x1y2 w l smooth unique t 'Hx'\n");

    fprintf(plotter2, "plot $S u 1:2 axes x1y1 w l smooth unique t 'S'\n");
}

void finish(const analysis_context_t *ctx)
{
    q2_t *q2 = (q2_t *)ctx->user_data;

    const impedance_probe_t *iprobe = &q2->i_p;
    const velocity_probe_t *vprobe_1 = &q2->v_p1;
    const velocity_probe_t *vprobe_2 = &q2->v_p2;

    const energy_probe_t *eprobe_1 = &q2->e_p1;
    const energy_probe_t *eprobe_2 = &q2->e_p2;

    const reflection_probe_t *rprobe = &q2->r_p;
    const transmission_probe_t *tprobe = &q2->t_p;

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

    printf("\n=== Measured Coefficients ===\n");
    printf("Probe 1 (Node %d):\n", q2->r_p.node);
    printf("  Max Incident Field  |E_inc| : %.6e V/m (at t = %.6e s)\n", q2->r_p.max_inc, q2->r_p.t_peak_inc);
    printf("  Max Reflected Field |E_ref| : %.6e V/m (at t = %.6e s)\n", q2->r_p.max_ref, q2->r_p.t_peak_ref);

    printf("Probe 2 (Node %d):\n", q2->t_p.node);
    printf("  Max Transmitted Field |E_trans|: %.6e V/m (at t = %.6e s)\n", q2->t_p.max_trans, q2->t_p.t_peak_trans);

    double R = q2->r_p.max_ref / q2->r_p.max_inc;
    double T = q2->t_p.max_trans / q2->r_p.max_inc;

    printf("\nCalculated Coefficients:\n");
    printf("  Reflection Coefficient  : %.4f\n", R);
    printf("  Transmission Coefficient : %.4f\n", T);
    printf("=============================\n");

    printf("\n=== Poynting Vector ===\n");
    printf("Probe 1 (Node %d)\n", eprobe_1->node);
    printf("  Net Energy flow : %.6e J/m^2\n", eprobe_1->energy);
    printf("Probe 2 (Node %d)\n", eprobe_2->node);
    printf("  Net Energy flow : %.6e J/m^2\n", eprobe_2->energy);

    printf("\nEnergy difference : %.6e J/m^2\n", eprobe_2->energy - eprobe_1->energy);
    printf("=======================\n");

    printf("\nSaving simulation animation to 'q2.gif', 'q2-p.gif'\n");

    fprintf(q2->plotter1, "unset output\n");
    fprintf(q2->plotter2, "unset output\n");
    pclose(q2->plotter1);
    pclose(q2->plotter2);
    free(q2);
}
