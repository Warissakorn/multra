# PROGRESS log archive — 2026-10-19

Old PROGRESS entries, moved whole, newest first. Nothing here is current: NEXT and ROADMAP
hold status, the decision record holds the reasoning.

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
