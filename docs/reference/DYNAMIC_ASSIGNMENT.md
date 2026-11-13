# Dynamic assignment (Micro) — the between-run path-cost loop (M9, D549)

Written 2026-11-13 as a contract, before code. ROADMAP M9 asks for:
- an iterative between-run path-cost loop over the existing routing decisions;
- costs in a separate file with provenance;
- convergence reported.

En-route rerouting is out. The owner chose it as the next engine milestone and answered §9 in the
same session, each with the recommended option.

Rule 4 holds:
- every output keeps the not-validated marker;
- reproducing a book number checks arithmetic, not behaviour.

**Sources.** Paraphrased and cited, never vendored.
- **Ortúzar and Willumsen (2011)**, *Modelling Transport* 4th ed. (repository
  `Modelling-Transport-4-edition-Markdown`, commit `1b3d41d`).
  - **The δ indicator (Eq. 10.22, book p. 368).** It is the flow-weighted excess cost over each
    pair's minimum, divided by the flow-weighted minimum cost.
  - **MSA (§10.5.4, Eq. 10.23, book p. 371).** The current flows are a linear combination of the
    previous flows and an all-or-nothing auxiliary load, with step φ = 1/n. Smock's rule gives
    each auxiliary load equal weight and converges to Wardrop's equilibrium, though slowly
    (citing Sheffi 1985).
  - **Example 10.6 (book p. 371)** applies MSA to the town-centre/bypass pair of Example 10.4:
    - costs `10 + 0.02 V` (town) and `15 + 0.005 V` (bypass), in minutes;
    - 2000 vehicles.
  - **Example 11.1 (book p. 394)** gives the equilibrium of the same pair: 600 town and 1400
    bypass, both at 22 min (checked in [book-copies-check](../evidence/book-copies-check.md) §5).
  - **The relative gap (Eq. 11.11, book p. 397)** is the link form of the same measure. Ortúzar
    cites Boyce et al. (2004) recommending at most 0.0001 for a final Macro calibration.
- **Sheffi (1985) Ch. 11–12**, MSA's convergence (BOOKS §3.1): background only. No number from it
  is used.

## 1. Scope

- **In:**
  - the loop over a project's routing decisions (M2.4, placed M2.1.1, positioned D119), each with
    its intervals (M2.1.2);
  - cost measurement from a Micro run;
  - the MSA update;
  - δ per cell and overall;
  - the route-costs file and reading it back;
  - the CLI.
- **Out:**
  - en-route rerouting;
  - SUE and logit loading;
  - departure-time choice;
  - decisions with vehicle-type rules (refused, §2);
  - path enumeration beyond the authored decision entries;
  - writing weights back into the project (a later slice, §8 A4);
  - the editor;
  - Meso (M11.5 reuses this loop later).

## 2. What is iterated

**A cell** is one decision × one of its intervals. Outside every interval the decision's
whole-period `relativeFlow` applies, and that is one more cell, `"default"`.

**A cell's choices** are the decision's entries in authored order, whether a `routeId` or a
`destinationLinkId`.

**Its weights** are the entries' flows in that cell, normalised to shares. An all-zero interval
falls back to the default shares, as `decisionFlowAt` already does. The loop then writes explicit
shares for it.

**Refused before the first run,** as `DYNAMIC_ASSIGNMENT_INVALID` at the decision's path:
- a project with no routing decision;
- a decision with `typeRules`;
- a decision with fewer than two entries. Such a decision is skipped, not refused, when others
  exist; it is listed under `skipped`.

## 3. Cost measurement (one run)

**For every vehicle whose route a decision set,** the cost runs from that decision's moment to the
vehicle's arrival, in seconds of simulated time:
- **The moment:**
  - positioned decision: the tick its front passes the station;
  - unpositioned decision: the vehicle's departure onto the network.
- **The cell** is chosen by the same time that chose the weights:
  - positioned decision: passage time;
  - unpositioned decision: scheduled demand time (M2.1.2).
- **The choice** is the entry the vehicle drew.

**A cell's cost per choice** is the mean over its vehicles.
- **Censored vehicles.** A vehicle still on the network at run end counts with its elapsed time,
  a lower bound. The cell's `censored` count is written beside it.
- **Empty choice.** A choice with no vehicle in the cell takes its free-flow time from the
  decision moment (the D39 `freeFlowTime` rule over the route's remaining part). The cell's
  `freeFlowFilled` lists it.

**A vehicle that passes several decisions** is costed for each from that decision's own moment.
An upstream cost then includes the downstream choices' outcomes. This is a known limit of a
decision-local loop, written in the output's notes.

## 4. The loop

- **Iteration 1** runs the project's own weights `w¹` (the book's V¹, φ = 1).
- **Iteration n ≥ 2:**
  - For every cell, the auxiliary share `yⁿ` is 1 on the cheapest choice of iteration n−1's costs
    and 0 elsewhere. A tie within 1e-9 s splits evenly among the tied choices.
  - `wⁿ = wⁿ⁻¹ + (1/n)(yⁿ − wⁿ⁻¹)` (Eq. 10.23 with φ = 1/n).
  - Run again.
- **Every run uses the same seed:** common random numbers, so run-to-run noise is not mistaken for
  a change of costs.
- **δ** of a run is Σ_cells Σ_k q_k (c_k − c*) / Σ_cells Σ_k q_k c* (Eq. 10.22):
  - q_k is the count of vehicles on choice k;
  - c* is the cell's minimum cost.
  - Censored vehicles count, and free-flow-filled choices have q = 0.
- **Stop** after the run whose δ ≤ `gap`, so `converged` is true; otherwise stop at
  `maxIterations` with `converged` false.
- **Written weights.** The file holds the weights of the last run, the ones whose costs it
  reports. It does not hold an untried next update.

**Settings and defaults:**
- `maxIterations`: integer in [2, 200], default 20.
- `gap`: finite and ≥ 0, default 0.05.

The default `gap` is far looser than Ortúzar's 0.0001, which is a Macro figure. A simulation's
noise sets a floor that is not yet measured; revisit it with the first fixture's δ trace.

## 5. The route-costs file

```json
{ "format": "MultraRouteCosts", "version": 1, "notValidated": true,
  "provenance": { "project": "<file name only>", "seed": 42, "maxIterations": 20, "gap": 0.05,
                  "tool": "<MULTRA version>" },
  "converged": true, "iterations": [ { "n": 1, "delta": 0.21 }, … ],
  "decisions": [ { "id": "rd-1", "cells": [
      { "interval": 0, "choices": [ { "entry": 0, "weight": 0.6, "cost": 1320.0, "count": 600 }, … ],
        "censored": 0, "freeFlowFilled": [] },
      { "interval": "default", … } ] } ],
  "skipped": [], "notes": [ … ] }
```

**Privacy and provenance:**
- No path beyond the file name is written (privacy).
- The structure (decision ids, entry count, interval bounds) is the staleness key. A
  content hash is not used: the CLI has no hash outside the Qt shell.

**Reading it back:** `--route-costs FILE` with `--project` (single run or `--seeds`) replaces each
listed cell's flows with its weights at compile.
- `ROUTE_COSTS_STALE` at `decisions[d]` when:
  - a listed decision is missing;
  - the entry count differs;
  - the interval bounds differ.
- `ROUTE_COSTS_INVALID` at its path for a shape fault: wrong format or version, a weight that is
  negative or not finite, or a cell whose weights sum to 0.
- A run without the flag is byte-identical to today's.

## 6. CLI

`--project P --dynamic-assignment OUT [--iterations N] [--gap G] [--seed S]`, alone like
`--macro`. It runs the loop, writes OUT, and prints one stderr line per iteration
(`dynamic assignment: iteration n, delta d`).
- Exit code 0 even when not converged. In that case a final stderr line says
  `dynamic assignment: not converged after N iterations (delta d)`, so a batch is not broken.
- Every refusal is the project's usual JSON issue list, with exit 1.

## 7. Hand values and rows

**(a) Ortúzar Example 10.6, the pure loop.** Costs come from the two analytic functions; flow is
the share × 2000; w¹ = all on town. Exact arithmetic (script, 2026-11-13):

| n | town | bypass | c town | c bypass | δ |
|---|---|---|---|---|---|
| 1 | 2000 | 0 | 50 | 15 | 2.333333 |
| 2 | 1000 | 1000 | 30 | 20 | 0.25 |
| 3 | 666.667 | 1333.333 | 23.333 | 21.667 | 0.025641 |
| 4 | 500 | 1500 | 20 | 22.5 | 0.09375 |
| 5 | 800 | 1200 | 26 | 21 | 0.095238 |
| 6 | 666.667 | 1333.333 | 23.333 | 21.667 | 0.025641 |
| 7 | 571.429 | 1428.571 | 21.429 | 22.143 | 0.023810 |
| 8 | 750 | 1250 | 25 | 21.25 | 0.066176 |
| 9 | 666.667 | 1333.333 | 23.333 | 21.667 | 0.025641 |
| 10 | 600 | 1400 | 22 | 22 | 0 |

- Rows 1–10 equal the book's printed table. The book rounds row 7 to 572 / 1428, and the δ at
  n = 5 equals its 0.095 for Example 10.5's same flows.
- Iteration 10 is Example 11.1's equilibrium: 600 / 1400 at 22 min.

**(b) A Micro two-route fixture** (`tests/data/dynamic-assignment/`, built in A3).
- **Network:** one input and one placed decision with two routes:
  - the short route passes a fixed-time signal whose green limits its capacity;
  - the long route is unsignalised.
- **Demand** exceeds the short route's capacity, so the equilibrium is interior.
- **Fixed before any run:**
  - δ ≤ 0.05 within 20 iterations at seed 42;
  - the two routes' final costs differ by at most 5 % of the cheaper.
- Any amendment after a run is recorded here, never silently tuned.

| Row | Asserts |
|---|---|
| DA1 | The pure loop on (a) reproduces every row's flows to 1e-9 and stops at n = 10 with δ = 0 (`converged`); with `maxIterations` 9 it stops unconverged at δ 0.025641 |
| DA2 | δ by hand on a three-choice cell, censored vehicles counted, free-flow-filled choices weightless |
| DA3 | The update: an exact tie splits evenly; the weights at n = 2, 3, 4 are 1/2, 1/3, 1/4 steps; the written weights are the last run's |
| DA4 | Cost measurement: a single vehicle on a hand-timed route (constant speed, no interaction) gives the decision-to-arrival time for positioned and unpositioned decisions, and the cell by the right clock |
| DA5 | An empty choice is filled with its free-flow time and listed; δ ignores it |
| DA6 | A vehicle still running at the end is counted with its elapsed time and in `censored` |
| DA7 | (b) converges as fixed above |
| DA8 | Two loops on (b) give identical bytes; `--iterations 2` writes `converged: false` and the stderr line, exit 0 |
| DA9 | File round trip; each STALE and INVALID case at its path; refusals of §2; a project run without `--route-costs` keeps its bytes |

Each row first asserts that its forcing worked.

## 8. Slices

| Slice | Content | Rows |
|---|---|---|
| A0 | This contract | — |
| A1 | The pure loop in `src/project/dynamic_assignment.*` behind a cost-provider callback. **Implemented (§10)** | DA1–DA3 |
| A2 | Measurement from a Micro run, the file codec, `--route-costs`; unplaced decisions only (§11). **Implemented (§12)** | DA4–DA6, DA9 |
| A2b | Placed and positioned decisions (route families), before or after A3 as the owner picks | — |
| A3 | The loop over real runs, `--dynamic-assignment`, fixture (b) | DA7–DA8 |
| A4 | Later, the owner's choice: the editor, and weights written back into the project (a schema bump) | — |

`src/core` stays unchanged unless a decision's passage time is not observable from outside. If it
is not, the slice adds the smallest event field and measures the hot path before and after.

## 9. Owner choices (answered 2026-11-13, D549; each the recommended option)

1. **Update rule.**
   - **(a) MSA with all-or-nothing auxiliary shares (chosen).**
   - (b) Logit SUE with MSA.
   - (c) Swap by cost difference.
2. **Cost.**
   - **(a) Experienced time per choice, from the decision to arrival, by interval (chosen).** An
     empty choice is free-flow-filled and flagged.
   - (b) D39 whole-route movement delay.
3. **Results.**
   - **(a) A separate costs file read back by a flag, no schema change (chosen).**
   - (b) Weights written into the project.
4. **Fixture.**
   - **(a) Two layers (chosen):** the pure loop on the book's analytic costs, plus a real Micro
     two-route network.
   - (b) Micro only.

## 10. As implemented (A1, 2026-11-13)

- **Code:** `src/project/dynamic_assignment.*`:
  - `allOrNothing`, `msaStep` and `msaDelta`;
  - `iterateMsa(w¹, provider, settings)`, which refuses `maxIterations` outside [2, 200] and a
    negative or non-finite `gap`.
- **Provider contract.** A cell or choice count that differs from the weights is a logic error.
  The provider receives exactly the weights that each iteration reports (DA3).
- **Rows:** `tests/dynamic_assignment_tests.cpp` (group `dynassign`) covers DA1–DA3.
  - DA1 reproduces the §7 table to 1e-9 and stops at n = 10 with δ = 0. Stopped at 9, it is
    unconverged at 1/39.
  - No value was changed after the first run.

## 11. Amendments before A2 code (2026-11-13)

Written after reading the compile path and before any A2 code or figure. Every decision below is
recorded openly.

1. **A2 supports unplaced (M2.4) decisions only.**
   - **Why.** Compile splits an unplaced decision at the source into inputs
     `<input>/route-<routeId>`, and the authored route of each runtime route is known
     (`RunSnapshot::routeOrigins`).
   - **What is refused.** A decision with a `linkId` (placed, positioned or not) is refused as
     `DYNAMIC_ASSIGNMENT_UNSUPPORTED` at `routingDecisions[i]` until a slice **A2b**. Such a
     decision is met on the network walk, and its entries are recoverable only through route
     families. Its `RoutingEvent` (positioned) gives the passage time, but not the entry.
2. **The moment and clock of an unplaced decision.**
   - The moment is departure, so the cost is the arrival's `travelTime` (time − entered time).
   - The cell is chosen by `scheduledTime`, which `DepartedEvent` and the vehicle carry.
3. **Membership.** A vehicle belongs to decision D when its core input's id equals the id of an
   authored input naming D, or begins with that id followed by `/`. Its entry is the one whose
   `routeId` is the authored origin of its departure route. Lane changes stay inside one authored
   route (D95), so the origin does not change on the way.
4. **Free-flow fill (§3) for an unplaced decision:**
   - the entry's authored route length (`authoredRouteLength`) ÷ the mean desired speed of D's
     departed vehicles in the run;
   - if D had none, the mean over all departed vehicles;
   - a run with no departed vehicle at all is refused (`DYNAMIC_ASSIGNMENT_INVALID` at
     `routingDecisions`).
5. **Cells.** A decision's cells are its intervals in order, then `default`. A cell's shares are
   its entries' flows normalised. An all-zero interval takes the default shares. An all-zero
   default is `DYNAMIC_ASSIGNMENT_INVALID` at `routingDecisions[i]`.
6. **Writing weights.**
   - The default cell sets each entry's `relativeFlow`; interval k sets `intervalFlows[k]`.
   - A share of 0 gives the entry no vehicle, so it is free-flow-filled.
   - The route-costs file lists cells by `interval` index or `"default"`. Cells it omits keep the
     project's flows.
7. **DA4 reworded.** No single vehicle is hand-timed. The four-leg fixture's random arrivals give
   no exact single vehicle. DA4 instead asserts that each cell's cost equals the mean `travelTime`
   of the cell's arrived vehicles, recomputed in the test from the raw event stream, with the cell
   taken by `scheduledTime`; both intervals and both entries are populated as forcing.

## 12. As implemented (A2, 2026-11-13)

- **Code:**
  - `src/project/dynamic_assignment_run.*`:
    - `planDynamicAssignment` (the §2 and §11 refusals);
    - `decisionWeights` and `applyDecisionWeights`;
    - `measureDecisionCosts`, which steps one compiled run at a seed and reads `DepartedEvent`,
      `ArrivedEvent` and the final state.
  - `src/project/route_costs.*`: `routeCostsFor`, `routeCostsJson`, `parseRouteCosts` and
    `applyRouteCosts`.
  - The file's interval cells carry `start` and `end`, the interval bounds §5 names as the
    staleness key.
  - `tools/dynamic_assignment_cli.*` and `--route-costs` in `multra-cli`, for a single run or
    `--seeds`. Any other mode, and `--compare`, refuses the flag.
- **Rows:** `tests/dynamic_assignment_run_tests.cpp` (group `dynassignrun`) covers DA4–DA6 and
  DA9 on the four-leg fixture's West approach, with two intervals.
- **One test fix, no value changed.** DA6's first form set the run duration (650 s) below the
  input's end (900 s), which the project refuses (`INVALID_INTERVAL`). The input now ends at
  650 s too.
- `src/core` is unchanged.
