# PROGRESS — TrafficSim

Newest entry at the top: what changed and why. What to do next is in [`NEXT.md`](NEXT.md);
decisions go once into the [decision record](decisions/RECORD.md). Keep about ten entries here;
move older ones whole to the top of the newest [`archive/`](archive/README.md) PROGRESS log.

---

## 2026-10-19 — S2.1: link types and the Macro graph (D273)

The first Macro code ([MACRO_NETWORK](reference/MACRO_NETWORK.md) §16). There is no assignment
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

The owner chose S2. [MACRO_NETWORK](reference/MACRO_NETWORK.md) fixes how the drawn network
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

S1 is complete ([STUDY_PERIODS](reference/STUDY_PERIODS.md) §6, §14).
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

OD matrices can now be placed on the study clock ([STUDY_PERIODS](reference/STUDY_PERIODS.md) §13).
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

The owner chose S1 next. [STUDY_PERIODS](reference/STUDY_PERIODS.md) fills what the workflow left
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

The Nodes tab can now draw one `generated` Node per junction no Node covers ([NODES](reference/NODES.md) §6, §14).
- **The junction rule (D265):** D259 named `conflictGroups`, which are pairs of road objects. A
  junction is now a cluster of conflicts (authored and automatic) joined through their Connectors,
  fed from at least two Links. Its polygon is the padded convex hull of its Connectors and conflict
  points. Overlapping hulls merge; one over an existing Node is dropped.
- **Shipped projects:** one Node each on the four-leg and M2.6 (12 turns; the approach lane adds
  left out) and on the T-junction (4 turns); none on the lane lab. Each membership has no issue.
- **Rows:** N18 (ctest `nodesuggest`) and the `node-ui` action check; six mutations caught.
- **Unchanged:** every run output, the pin and the TS rows. Linux only.

## 2026-10-19 — Nodes S0.3: Micro node evaluation (D264)

Each Node now reports per-turn, per-leg and per-Node delay ([NODES](reference/NODES.md) §4, §13).
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

What a Node holds is now derived from the network inside its polygon ([NODES](reference/NODES.md) §3, §12).
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

## 2026-10-19 — Nodes S0.1: object, file, tool and tab (D260, D261)

The owner accepted every Node recommendation (D260) ([NODES](reference/NODES.md) §11).
- **Model:** `Network::nodes` and `nodeIssues` (ids, simple polygon, no overlap). The polygon tests
  are shared with zones in `polygon.*`.
- **File:** `network.nodes` is written only when a Node exists, at schema 48, with
  `EDIT_NODE_INVALID` at its path. `generated` is written only when true.
- **Commands:** `putNode`, `renameNode`, `deleteNode`. An edit clears `generated`.
- **Editor:** the Node tool (`N`) shares the zone draft, and the Nodes tab in the Measurement group
  can rename and delete. Each action is one Undo step.
- **Rows:** N1–N5 (ctest `nodes`, `node-ui`). The workspace and object-group UI tests list the new
  tool and page.
- **Unchanged:** the engine does not read Nodes, so every run output, the pin and the TS rows are
  unchanged. Linux only.

## 2026-10-19 — The Node contract (S0, D259, docs only)

The owner chose to start the multi-level work (D145, D175) at S0, before W99 ([NODES](reference/NODES.md)).
- **The object:** a simple polygon at `network.nodes`, at schema 48, merged with M5.1's evaluation
  nodes.
- **Derived from the Micro detail inside, never stored:** legs, turns (the `routeLaneChains`
  walk), conflicts and control.
- **Micro evaluation:** one automatic travel-time section per turn, with leg and Node rows weighted
  by vehicles, a queue per leg, and LOS letters labelled as LOS §4.
- **Coarse values** and refine/derive-back are specified but booked with S2.
- **Rows:** N1–N18 planned.
- **Five owner choices,** each with a recommendation. Also fixed: MULTI_LEVEL_MODELLING's dangling
  "VISSIM_PARITY §6" (now §2b).

No code. Linux only.

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
