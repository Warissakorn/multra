# PROGRESS log archive — 2026-10-19

Old PROGRESS entries, moved whole, newest first. Nothing here is current: NEXT and ROADMAP
hold status, the decision record holds the reasoning.

## 2026-10-20 — Docs and AI context: NEXT cut to its open state (D280)

The second item of the optimization pass. A session's mandatory reads were ≈8–11k tokens, and
NEXT was 88 % shipped history that PROGRESS and RECORD already hold.
- **NEXT:** 13,877 → about 2.9 KB. The 2026-10-19 state list moved whole to
  [archive/NEXT-state-2026-10-20.md](NEXT-state-2026-10-20.md); the new state lists only
  what is open, and adds S2 and S0.6, which it lacked.
- **AGENTS:** RECORD is appended to, never read whole (≈70k tokens); the file-size rule now says
  what the check enforces (fails above 500); `src/macro/` is in the layout. NEXT and PROGRESS point
  to AGENTS instead of restating its rules.
- **Stale lines fixed:** ARCHITECTURE's Macro row, ROADMAP's S0 line (S0.6; S0.4 moved to S3),
  read-by-task rows for Macro and study periods, a `src/macro/` README, and the decision index
  (a Macro topic row, D275–D279 summaries, five answered choices no longer "awaiting").

## 2026-10-20 — Node evaluation cost: sections by route, tracks in place (D279)

The owner ran an optimization pass and approved the recommended order; this is its first item.
- **Measured** (Linux, GCC 13.3 RelWithDebInfo, callgrind, seed 42): four-leg with a Node 1,303.6 →
  1,128.8 M Ir (−13.4 %), T-junction with a Node 256.2 → 233.2 M (−9.0 %). A Node now adds +20 %
  and +14 % (was +39 % and +26 %, D264). `SectionAccumulator::observe` 325.8 → 150.9 M.
- **How:** `observe` visits only the sections with a line on the vehicle's route, and advances
  known tracks in place instead of moving every Track into a new vector each tick.
- **Unchanged:** every output byte-identical (four shipped projects and two Node fixtures, run and
  seeds 42–44), the pin and the TS rows.
- **Tests:** two section rows, each killing a mutant that survived the suite (a section with only
  its end line on the new route; a track kept after its vehicle left).

## 2026-10-20 — S0.6: Node rows in Results (D278); MA15 paused

MA15 needs Sheffi (1985) from web.mit.edu, which the proxy still refused after the owner allowed
it, so the owner chose other work ([NODES](../reference/NODES.md) §15).
- **Results → Nodes:**
  - **a run:** the Node, leg and turn rows as the CSV block gives them (vehicles, delay,
    unfinished, queue on legs, travel time on turns, LOS by the Node's control type);
  - **a batch:** n, mean, CI and LOS on the mean;
  - **a comparison:** no rows yet, and it says so.

  The marker comes first.
- **Rows:** N19–N21 (`node-results-ui`). The workspace test and the UI audit list the new tab.
- **Mutations:** seven caught. The batch LOS on mean + CI first survived; N20 now checks every
  row's letter.
- **Unchanged:** every output (editor-only), the pin and the TS rows. Linux offscreen only.

## 2026-10-19 — S2.4: Run Macro in stages, Macro links, link types (D277)

The owner chose §19 (a) and wants more capability later
([MACRO_NETWORK](../reference/MACRO_NETWORK.md) §20).
- **Engine:** a convergence record per iteration (gap and step), and an observer that can cancel
  between iterations.
- **Run Macro…:**
  - asks for the period and runs on its own worker;
  - shows the six stages with their status and the live gap;
  - Cancel stops between iterations;
  - it is not an Undo step.
- **Results → Macro links:** a row per arc with the marker. An edit keeps the rows and marks them
  stale until Undo returns to the result's revision. Export and Copy write the Macro CSV.
- **Run settings:** the target gap and the iteration cap; the defaults write nothing.
- **Network → Link types:** Add, Edit and Delete (a type in use cannot go). The Link page gets a
  Macro type combo.
- **Rows:** MA14 (`macro-ui`), MA18 and the stage and command rows. The workspace and object-group
  tests list the new page.
- **Mutations:** nine caught.
- **Found:** a multi-line note made the Objects dock too tall for the wide-canvas row. It is now one
  paragraph.
- **Unchanged:** Micro outputs byte-identical (only `buildCommit` differs), the pin and the TS rows.
  Linux offscreen only.

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

## 2026-10-19 — The S2 (M10.1) contract: Macro network and assignment (D271, D272, docs only)

The owner chose S2. [MACRO_NETWORK](../reference/MACRO_NETWORK.md) fixes how the drawn network
compiles into a Macro graph and how one study period is assigned. Nothing is implemented.
- **Graph:** Links split at Connector stations become capacitated links. Connectors and zone
  connectors become uncapacitated arcs. Nodes are not read yet.
- **Values:** `Link::linkType` and link types `{capacityPerLane, freeSpeed, vdfId, source}` at
  schema 50, none shipped; BPR 0.15/4 as data (D158).
- **Demand:** pcu/h by connector share for one period; `MODE_PCU_MISSING` is refused.
- **Engine:** a pure `src/macro/` Frank–Wolfe with a named gap formula; non-convergence is
  flagged, never hidden.
- **Rows:** MA1–MA15, planned.
- **Source checks** changed three recommendations:
  - the BPR pair assumes practical capacity, so a type's source must say which capacity it gives;
  - Boyce et al. (2004) argue for a 0.01 % gap;
  - the Sioux Falls data are for academic research only, so the benchmark stays local until the
    licence is confirmed.
- **The owner answered the same day (D272):**
  - one class per mode, with mode bans on link types;
  - a textbook benchmark instead of Sioux Falls;
  - a 0.01 % gap;
  - S0.4 moves to S3;
  - the other choices as recommended.

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
