# PROGRESS log archive — 2026-10-19

Old PROGRESS entries, moved whole, newest first. Nothing here is current: NEXT and ROADMAP
hold status, the decision record holds the reasoning.

## 2026-10-19 — How Visum and Aimsun organise a Macro run (D276, docs only)

The owner asked whether a Macro run has a working procedure, then for a study of other programs
([evidence](../evidence/macro-run-procedures.md)).
- **Visum:** an ordered procedure sequence (generation → distribution → mode choice → skim →
  assignment) in the version file, logged. Its equilibrium assignment balances routes in an inner
  loop and searches for new routes in an outer loop, to a gap or a cap.
- **Aimsun Next:** a Static Assignment Scenario (demand, outputs) and an Experiment (method,
  iteration cap, relative gap), with convergence data retrieved from the output database.
- **TrafficSim:** `runMacro`'s six stages are now named in [MACRO_NETWORK](../reference/MACRO_NETWORK.md)
  §19, with a choice for S2.4 that the owner makes before any code.
- **Caveat:** the help pages could not be opened from the session; the facts come from search
  summaries and are marked so.

## 2026-10-19 — S2.3: Macro run settings, outputs and CLI (D275)

A Macro run now writes its results ([MACRO_NETWORK](../reference/MACRO_NETWORK.md) §18). The editor
surface is split off as S2.4, since it is a second system (a worker job, a Results tab, stale
marking, a link-type editor).
- **Settings (schema 51):** `definition.macroSettings{targetGap, maxIterations}`, written only
  when set. Absent means 10⁻⁴ (D272) and 1000 iterations; bad values are `INVALID_MACRO_SETTING`.
- **Run:** `runMacro(doc, data, period)` names a period as its study-period id or
  `matrix:<id>`, refuses the graph's and demand's issues, and adds `MACRO_NOT_CONVERGED` at the
  cap. The input manifest records every file read.
- **Outputs:** `macroJson` and `macroCsv`, one row per arc with the link type, veh/h per class,
  pcu/h, V/C, free and congested time; both carry the not-validated marker.
- **CLI:** `--macro PERIOD` with `--project` (and `--csv`, `--data-dir` only).
- **Rows:** MA12, MA13 (ctest `macrooutput`, `cli-macro*`). Writing MA12 showed that S2.2's MA11
  twin had no route choice (each entry Link had its own zone connector); S2.3's fixture fans one
  origin out to two routes, and MA11's comment now says so.
- **Mutations:** nine caught. A missing class column first survived (the row checked only its
  prefix); MA13 now compares a whole CSV row.
- **Unchanged:** Micro outputs byte-identical (only `buildCommit` differs), the pin and the TS
  rows. Linux only.

## 2026-10-19 — S2.2: the Macro assignment (D274)

The Macro graph can now be assigned ([MACRO_NETWORK](../reference/MACRO_NETWORK.md) §17). There are
no outputs or editor yet.
- **Engine** (`src/macro/assign.*`, pure):
  - Dijkstra, where on equal distance the lower link index wins;
  - all-or-nothing loading;
  - multi-class Frank–Wolfe on the shared load Σ pcu·v, with an exact bisection line search;
  - §7's relative gap. Stopping at the iteration cap is flagged, and the flows are still
    returned.
- **Rows:** MA7–MA11, MA16, MA17 (ctest `macroassign`). They cover:
  - closed-form and bisected equilibria;
  - an unused route;
  - the gap recomputed from the flows;
  - identical results with Links permuted;
  - a mode ban;
  - mixed PCU.
- **Mutations:** seven caught. An MSA step first survived, because it still converged. Row MA8 now
  pins that two routes reach equilibrium in one exact step.
- **Unchanged:** Micro outputs, the pin and the TS rows. Linux only.

## 2026-10-19 — S2.1: link types and the Macro graph (D273)

The first Macro code ([MACRO_NETWORK](../reference/MACRO_NETWORK.md) §16). There is no assignment
yet.
- **File (schema 50):** a Link's `linkType` and project-owned `definition.linkTypes`
  `{capacityPerLane, freeSpeed, vdfId, modes, source}`. No type ships; `data/vdf/bpr.json` does.
- **Graph:** `compileMacroNetwork` splits Links at Connector stations into capacitated pieces.
  Connectors become fixed-time arcs at the from-Link's speed, and each zone connector gets its
  own centroid node. The new `src/macro/` is a pure module.
- **Demand:** `macroDemand` gives veh/h per class (mode, or composition) and period by connector
  share. `MODE_PCU_MISSING` and `MACRO_NO_PATH` are refused.
- **Rows:** MA1–MA6 (ctest `macronetwork`); nine mutations caught. Micro outputs, the pin and the
  TS rows are unchanged. Linux only.

## 2026-10-19 — S1.2: modes and PCU (D269, D270)

S1 is complete ([STUDY_PERIODS](../reference/STUDY_PERIODS.md) §6, §14).
- **The call (D269):** the owner asked for the best design from the project's own rules. Modes join
  the Demand catalog under its presence rule: `definition.modes` absent is `data/modes/` (`car`,
  `mixed` ship). A project owning its compositions owns its modes once a matrix names one, so it
  still runs with no data directory, and a catalog copy keeps the schema of a project using none.
- **File (schema 49):** a matrix's `modeId` in place of `compositionId` (`OD_TYPES_AMBIGUOUS`),
  owned `definition.modes`, and a vehicle type's `pcu: {value, source}` (`INVALID_PCU`). Each is
  written only when used. No PCU value ships (D267).
- **Run:** a moded matrix runs byte-identical to its composition twin. `UNKNOWN_MODE` and
  `MODE_COMPOSITION_UNKNOWN` are Run refusals. Modes are read only when a matrix names one.
- **Editor:**
  - a Modes tab with the derived PCU ("—" while unknown); owned modes are editable;
  - the OD dialog's Types choice;
  - a PCU row in the type dialog.

  Each commit is one Undo step.
- **Rows:** SP9–SP14 (ctest `modepcu`, `modepcu-ui`); nine mutations caught. Shipped outputs, the
  pin and the TS rows are unchanged. Linux only.

## 2026-10-19 — S1.1: study periods (D268)

OD matrices can now be placed on the study clock ([STUDY_PERIODS](../reference/STUDY_PERIODS.md) §13).
- **File (schema 49):** `studyPeriods[]` in clock seconds, `definition.clockStart`, and a matrix's
  `periodId` in place of its own times. Each is written only when used, so schema-31 and shipped
  files keep their bytes.
- **Run:** `odMatrixWindow` places a period at `start − clockStart`. A placed matrix runs
  byte-identical to its run-second twin, and a window outside the run is `OD_PERIOD_INVALID`.
- **Editor:**
  - a Study periods tab (HH:MM:SS; an end before the start is the next day);
  - the Run settings clock field;
  - the OD dialog's period choice.

  Each commit is one Undo step.
- **Rows:** SP1–SP8 (ctest `studyperiods`, `studyperiod-ui`); six mutations caught. Shipped outputs,
  the pin and the TS rows are unchanged. Linux only.

## 2026-10-19 — The S1 contract: study periods, modes and PCU (D266, D267, docs only)

The owner chose S1 next. [STUDY_PERIODS](../reference/STUDY_PERIODS.md) fills what the workflow left
open:
- **StudyPeriod:** clock periods in a top-level array at schema 49.
- **Placing a period:** the run's `clockStart` places each matrix's period on run time.
- **Matrices:** each takes exactly one of its own times or a `periodId`, and exactly one of a
  composition or a mode. Schema-31 files keep their bytes because nothing is migrated.
- **Modes and PCU:** a `data/modes/` catalog, and an optional cited `pcu` per vehicle type with a
  derived mode PCU. No value ships without a source.
- **Rows:** SP1–SP14, planned. No code changed.
- **The owner accepted every recommendation (D267):** seconds since midnight, `clockStart` on the
  run, no profile or flow basis stored yet, and the PCU field without values.

## 2026-10-19 — Nodes S0.5: Suggest nodes (D265)

The Nodes tab can now draw one `generated` Node per junction no Node covers ([NODES](../reference/NODES.md) §6, §14).
- **The junction rule (D265):** D259 named `conflictGroups`, which are pairs of road objects. A
  junction is now a cluster of conflicts (authored and automatic) joined through their Connectors,
  fed from at least two Links. Its polygon is the padded convex hull of its Connectors and conflict
  points. Overlapping hulls merge; one over an existing Node is dropped.
- **Shipped projects:** one Node each on the four-leg and M2.6 (12 turns; the approach lane adds
  left out) and on the T-junction (4 turns); none on the lane lab. Each membership has no issue.
- **Rows:** N18 (ctest `nodesuggest`) and the `node-ui` action check; six mutations caught.
- **Unchanged:** every run output, the pin and the TS rows. Linux only.

## 2026-10-19 — Nodes S0.3: Micro node evaluation (D264)

Each Node now reports per-turn, per-leg and per-Node delay ([NODES](../reference/NODES.md) §4, §13).
- **How:** one travel-time section per turn (in-leg Link start → out-leg crossing) and one queue
  counter per in-leg at its crossing, measured by the existing accumulators after the authored ones;
  `nodeRows` weights leg and Node delay by vehicles.
- **Output:** JSON `nodes[]`, a CSV block after LOS with its marker line, batch estimates
  `{n, mean, sd, halfWidth95}`, letters with the Node's control type. Nothing without a Node: the
  shipped projects' single and batch outputs are byte-identical.
- **Finding:** vehicles enter from rest, so an in-leg starting at a source includes the start-up
  loss (about 3 s of the T-junction's 3.2 s major-road delay). Choice 1a kept; N14 uses a split
  in-leg.
- **Cost:** no change without a Node; with one, Ir +40 % (four-leg) and +26 % (T-junction).
- **Rows:** N12–N17 (ctest `nodeevaluation`); five mutations caught. Pin and TS rows unchanged.
  Linux only.

## 2026-10-19 — Nodes S0.2: membership (D262, D263)

What a Node holds is now derived from the network inside its polygon ([NODES](../reference/NODES.md) §3, §12).
- **Contract fixed first (D262):** §3 refused any connected Link crossing the boundary twice, which
  refused the shipped T-junction, whose major roads run through as single Links. Legs are now
  boundary crossings: an entry is an in-leg, an exit an out-leg, and a Link's own run through is a
  turn. `NODE_LINK_REENTERS` replaces `NODE_LINK_CROSSES_TWICE`.
- **Model:** `nodeMembership` (`src/model/network/node_membership.*`) gives inside objects, legs,
  turns (checked with `routeLaneChains`), the turn-conflict table and control. Refusals are advisory
  diagnostics at `nodes[i]`, so Run is never blocked.
- **Editor:** the Nodes tab shows control, legs, turns and issues.
- **Rows:** N6–N11 (ctest `nodemembership`); the four-leg gives 4 + 4 legs, 12 turns, signalised;
  the T-junction 3 + 2 legs, 4 turns, unsignalised and 3 conflict rows.
- **Unchanged:** the engine reads no Node; the pin and the TS rows are unchanged. Linux only.
