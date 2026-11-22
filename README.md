# Vpliv periode ure na energetsko učinkovitost multiplekserjev 2 : 1 v QCADesigner-E

Datoteke za ponovitev vseh simulacij in analiz seminarske naloge.

## Vsebina

| Mapa / datoteka | Opis |
|---|---|
| `designs/MUX_USE.qca` | Multiplexer1 s popravljeno cono izhodne celice OUT (3 → 2) |
| `designs/MUX_SEN.qca` | Multiplexer2 s popravljenimi conami: zgornja vrata AND 2 → 0, OUT 0 → 1 |
| `src/run_cve.c` | Gonilnik brez grafičnega vmesnika za pogon *Coherence Vector (w/ Energy)* QCADesigner-E 2.2 s tabelo vektorjev |
| `src/run_one.sh`, `src/jobs.py` | Zagon ene simulacije in seznami vseh zagonov |
| `img/` | Grafi (PDF, PNG in JPG) |

## Nastavitve simulacij

Pogon *Coherence Vector (w/ Energy)*, QCADesigner-E 2.2.

| Parameter | Vrednost |
|---|---|
| Temperatura | 1 K |
| Relaksacijski čas | 1e-15 s |
| Clock High / Low | 9,8e-22 J / 3,8e-23 J, Clock Shift 0 |
| Radius of Effect | 80 nm |
| Relativna permitivnost | 12,9 |
| Razmik plasti | 11,5 nm |
| Celica / pika / razmik | 18 × 18 nm / 5 nm / 20 nm |
| Oblika ure | GAUSS, Clock Slope = T/40 (σ = Clock Slope/3) |
| Periode T | 10, 20, 40 ps |
| Časovni korak | 1e-17 s (preverjanje 5e-18 s) |
| Trajanje | 13,5 T (12 popolnih ciklov; zavrženi cikli 1–4, merjeni 5–12) |
| Metoda | Euler, Randomize Simulation Order vklopljen |
| Vhodi | tabela vektorjev z eno vrstico (konstantni vhodi; Zeroing of Inputs v tem načinu ni aktiven) |

Merjeno jedro (enako pravilo za obe izvedbi): vse celice razen vhodnih celic. Nobena izvedba nima celic z
*Ignore Energy Dissipation*, vhodne celice pa so pri obeh neposredno ob jedru.
Energija = `E_bath_total` (disipacija v okolico) na cikel, kot ga izpiše QCADesigner-E.

## Ponovitev

```sh
# 1) prevedi pogon (Linux, GLib/GObject razvojne knjižnice)
git clone https://github.com/FSillT/qcadesigner-e
cd qcadesigner-e/QCADesignerE/src
SRC="fileio.c fileio_helpers.c custom_widgets.c simulation.c simulation_data.c bistable_simulation.c \
 coherence_vector.c coherence_vector_energy.c design.c objects/QCADCell.c objects/QCADCompoundDO.c \
 objects/QCADDesignObject.c objects/QCADDOContainer.c objects/QCADLabel.c objects/QCADLayer.c objects/QCADRuler.c \
 objects/QCADStretchyObject.c objects/QCADSubstrate.c exp_array.c objects/object_helpers.c hsl2rgb.c \
 generic_utils.c graph_dialog_widget_data.c vector_table.c"
gcc -O3 -w -DSTDIO_FILEIO -DPACKAGE_DATA_DIR=\"/tmp\" -DPACKAGE_LOCALE_DIR=\"/tmp\" -DVERSION=\"2.2\" \
  -DPACKAGE=\"QCADesigner\" -I. -Iobjects $(pkg-config --cflags glib-2.0 gobject-2.0) $SRC \
  <pot>/src/run_cve.c -o <pot>/src/run_cve $(pkg-config --libs glib-2.0 gobject-2.0) -lm

# 2) zaženi vse simulacije (2 vzporedno)
cd <pot>
{ python3 src/jobs.py delay; python3 src/jobs.py energy; python3 src/jobs.py half; python3 src/jobs.py delay17; } \
  | xargs -P 2 -L 1 src/run_one.sh

# 3) analiza
python3 src/layout_metrics.py
python3 src/delay_analysis.py
python3 src/report.py
```