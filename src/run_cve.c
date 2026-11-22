/* Headless driver for QCADesigner-E (v2.2) "Coherence Vector (w/ Energy)" with a vector table.
 *
 * usage: run_cve file.qca T_ps n_periods time_step zero(0/1) VEC1 [VEC2 ...]
 *   VECk: comma-separated NAME=0|1 for every input label, e.g. "S=0,I1=1,I0=0"
 *   All other parameters follow Table I(a) of Sill Torres et al. (2018); Clock Slope = T/40, GAUSS clock.
 * Writes trace.csv (time, all output traces by label, all input traces, clock 0..3) in the cwd.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <glib-object.h>
#include "global_consts.h"
#include "fileio.h"
#include "design.h"
#include "simulation.h"
#include "vector_table.h"
#include "coherence_vector_energy.h"
#include "objects/QCADCell.h"
#include "objects/QCADLayer.h"
#include "objects/QCADLabel.h"
#include "objects/QCADSubstrate.h"
#include "objects/QCADStretchyObject.h"
#include "objects/QCADDesignObject.h"

extern coherence_energy_OP coherence_energy_options;

static int lookup (const char *vec, const char *name)
  {
  /* find ",name=" or "^name=" in vec */
  size_t n = strlen (name);
  const char *p = vec;
  while ((p = strstr (p, name)) != NULL)
    {
    if ((p == vec || p[-1] == ',') && p[n] == '=') return p[n + 1] == '1';
    p += n;
    }
  fprintf (stderr, "input %s missing in vector %s\n", name, vec);
  exit (4);
  }

int main (int argc, char **argv)
  {
  DESIGN *design = NULL;
  VectorTable *pvt;
  simulation_data *sd;
  int i, v, nin, nvec, nout;
  double T, dur;

  setvbuf (stdout, NULL, _IONBF, 0);
  if (argc < 7) { fprintf (stderr, "usage: run_cve file.qca T_ps n_periods time_step zero VEC...\n"); return 1; }
  g_type_init ();
  qcad_design_object_get_type (); qcad_stretchy_object_get_type (); qcad_layer_get_type ();
  qcad_cell_get_type (); qcad_label_get_type (); qcad_substrate_get_type ();
  if (!open_project_file (argv[1], &design) || !design) { fprintf (stderr, "open failed\n"); return 2; }

  T = atof (argv[2]) * 1e-12;
  dur = atof (argv[3]) * T;
  coherence_energy_options.T = 1.0;
  coherence_energy_options.relaxation = 1e-15;
  coherence_energy_options.time_step = atof (argv[4]);
  coherence_energy_options.duration = dur;
  coherence_energy_options.clock_high = 9.8e-22;
  coherence_energy_options.clock_low = 3.8e-23;
  coherence_energy_options.clock_shift = 0;
  coherence_energy_options.clock_period = T;
  coherence_energy_options.input_period = T;
  coherence_energy_options.t_slope_ramp = T / 40.0;
  coherence_energy_options.radius_of_effect = 80;
  coherence_energy_options.epsilonR = 12.9;
  coherence_energy_options.layer_separation = 11.5;
  coherence_energy_options.algorithm = EULER_METHOD;
  coherence_energy_options.clock_type = GAUSS;
  coherence_energy_options.randomize_cells = TRUE;
  coherence_energy_options.animate_simulation = FALSE;
  coherence_energy_options.zero_mode_act = atoi (argv[5]);
  coherence_energy_options.display_cell_diss = FALSE;
  coherence_energy_options.diss_trace_cood_x = -1;
  coherence_energy_options.diss_trace_cood_y = -1;

  pvt = VectorTable_new ();
  VectorTable_fill (pvt, design);
  nin = pvt->inputs->icUsed;
  nvec = argc - 6;
  printf ("inputs:");
  for (i = 0; i < nin; i++) printf (" %s", qcad_cell_get_label (exp_array_index_1d (pvt->inputs, VT_INPUT, i).input));
  printf ("\n");
  for (v = 0; v < nvec; v++)
    {
    VectorTable_add_vector (pvt, v);
    for (i = 0; i < nin; i++)
      exp_array_index_2d (pvt->vectors, gboolean, v, i) =
        lookup (argv[6 + v], qcad_cell_get_label (exp_array_index_1d (pvt->inputs, VT_INPUT, i).input));
    }

  sd = run_simulation (COHERENCE_VECTOR_ENERGY, VECTOR_TABLE, design, pvt);
  if (!sd) { fprintf (stderr, "sim failed\n"); return 3; }

  {
  int ns = sd->number_samples, k;
  int nInTr = design->bus_layout->inputs->icUsed;
  double dt = dur / ns;
  FILE *f = fopen ("trace.csv", "w");
  nout = design->bus_layout->outputs->icUsed;
  fprintf (f, "t_ps");
  for (i = 0; i < nout; i++)
    fprintf (f, ",OUT_%s", qcad_cell_get_label (exp_array_index_1d (design->bus_layout->outputs, BUS_LAYOUT_CELL, i).cell));
  for (i = 0; i < nInTr; i++)
    fprintf (f, ",IN_%s", qcad_cell_get_label (exp_array_index_1d (design->bus_layout->inputs, BUS_LAYOUT_CELL, i).cell));
  fprintf (f, ",clk0,clk1,clk2,clk3\n");
  for (k = 0; k < ns; k++)
    {
    fprintf (f, "%.6g", k * dt * 1e12);
    for (i = 0; i < nout; i++) fprintf (f, ",%.5g", sd->trace[nInTr + i].data[k]);
    for (i = 0; i < nInTr; i++) fprintf (f, ",%.5g", sd->trace[i].data[k]);
    for (i = 0; i < 4; i++) fprintf (f, ",%.5g", sd->clock_data[i].data[k]);
    fprintf (f, "\n");
    }
  fclose (f);
  printf ("outputs:");
  for (i = 0; i < nout; i++)
    printf (" %s(clk%d)", qcad_cell_get_label (exp_array_index_1d (design->bus_layout->outputs, BUS_LAYOUT_CELL, i).cell),
            exp_array_index_1d (design->bus_layout->outputs, BUS_LAYOUT_CELL, i).cell->cell_options.clock);
  printf ("\nrecorded samples: %d (dt=%g ps)\n", ns, dt * 1e12);
  }
  return 0;
  }
