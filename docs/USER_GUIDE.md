# A first study in MULTRA: the four-leg, start to table

A short walk through the Micro sentence of [PROBLEM](PROBLEM.md) §4 on a shipped example: a
signalised four-leg, counted volumes, a signal timing, ten seeds and a delay table to paste into a
report. It names the controls as the English interface does; the Thai interface has the same ones
(*Language*, top right). Details live in the linked references, not here.

**Read this first.** Every result is marked *Not yet validated*: the model has not passed its
calibration gate (M6, [ROADMAP](ROADMAP.md)). The shipped examples use plausible round numbers,
not counts ([data/projects](../data/projects/README.md)). Use the numbers to learn the workflow,
not to design a junction.

## 1. Open the example

*File → Open example → Four-leg signalised junction.* It opens as an untitled copy, so *Save*
asks for a place of your own and never changes the installed file. Next time, *File → Open
recent* lists what you saved.

What you see is what a study needs, already drawn:

| Part of the sentence | In the example | Where you would author it |
|---|---|---|
| "opens an aerial image" | none loaded | *Background image* (Ctrl+B) and *Calibrate image: two points*, or the *Map* menu ([MAP_BACKGROUND](reference/MAP_BACKGROUND.md)) |
| "draws a four-leg signalized intersection" | Links, Connectors and right-turn pockets | The Link and Connector tools; Ctrl + right-click creates in every tool ([NETWORK_EDITOR](reference/NETWORK_EDITOR.md), [EDITOR_WORKFLOW](reference/EDITOR_WORKFLOW.md)) |
| "enters counted turning volumes" | one volume per movement | *Vehicle inputs* in the Objects tables, and *Routing decisions* where one input is split by turn |
| "sets the signal timing" | one fixed-time program and its heads | *Signal control* and *Signal heads* |

To start from two centrelines instead of single Links, draw Street segments, use Find crossings
and a junction template ([JUNCTION_TEMPLATES](reference/JUNCTION_TEMPLATES.md)); the example
*Signalised junction with LOS and HCM analysis* was built that way.

## 2. Check, then watch one run

1. *Simulation → Check runnability.* Problems lists what would stop a run, and one advisory per
   place where two roads cross with no conflict area.
2. *Run (F5)* plays one seed; *Step (F6)* advances one step; *Reset* returns to time zero. Watch
   that queues form at red and discharge at green. A run that looks wrong is wrong: fix the
   drawing before reading any number.
3. *Simulation → Run settings* holds the duration, the time step and the evaluation period
   (warm-up, end and cool-down).

## 3. Run seeds and read the table

One seed is one random day. In *Results*, level *Micro*, tab *Movements and queues*:

1. Type the seeds, for example `42-51` for ten, and press *Run seeds*.
2. The table gives each movement's mean delay, the ± 95 % half-width over the seeds, vehicles,
   mean travel time and unfinished trips; further tables give sections and queues by approach.
   The note above the table states the seeds and the evaluation period ([BATCH](reference/BATCH.md)).
3. A wide ± means more seeds are needed before two alternatives can be told apart.
   *Compare with…* reads a second project and reports the difference with its interval.

## 4. LOS letters

The delay in the movement table is simulated travel-time delay; it gets no letter. A letter
needs a **travel-time section** with a control type ([LOS](reference/LOS.md)):

- Draw a section from the approach to the exit with the *Travel-time sections* tool and set
  *Control (LOS)* to *Signalised*. After *Run seeds* the section table shows a letter per
  section, and the export adds approach and intersection rows.
- For the HCM 7 control-delay table (capacity, v/c, delay, LOS, queue) use the Node method:
  *Results*, level *Node (HCM)*, *Analyse* ([NODE_ANALYSIS](reference/NODE_ANALYSIS.md)). It is
  analytical, not simulated; the two delays are different quantities and are labelled so.

The four-leg example has no sections and no Node. *File → Open example → Signalised junction
with LOS and HCM analysis* has both: *Analyse* and *Run seeds* there give both tables at once.

## 5. Into the report

*Copy results table* puts the table on the clipboard as text a spreadsheet or a word processor
accepts; *Export results (CSV)…* writes the same figures under the not-validated line. Quote the seeds, the evaluation period and that line with any number.

## Other levels

The Macro four-step example (*Macro four-step model*: Results, level *Macro*, *Procedure*) and
the Meso run are described in [MACRO_NETWORK](reference/MACRO_NETWORK.md),
[PROCEDURE_SEQUENCE](reference/PROCEDURE_SEQUENCE.md) and [MESO_NETWORK](reference/MESO_NETWORK.md).
