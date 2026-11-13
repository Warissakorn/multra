# Dynamic assignment — as implemented (M9, D549–D552)

The "As implemented" sections of [DYNAMIC_ASSIGNMENT](DYNAMIC_ASSIGNMENT.md), moved here to keep the
contract under 500 lines. Each keeps its section number, so "§14" means the same section in either
file. The contract still governs.

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

## 14. As implemented (A3, 2026-11-13)

- **Code:**
  - `src/project/dynamic_assignment_loop.*`: `runDynamicAssignment`. It runs `iterateMsa` with
    each iteration's weights written into a copy of the document and measured by
    `measureDecisionCosts`, logs one line per iteration plus the "not converged" line, and returns
    the last run's `RouteCosts`.
  - `tools/two_route_network.hpp`: `fixture::twoRoute()`, the §13 drawing at 19 : 1 (§13.1).
  - `multra-cli --dynamic-assignment OUT [--iterations N] [--gap G]`, alone with `--project`,
    `--seed` and `--data-dir`. It refuses an existing OUT before running and exits 0 when
    unconverged.
  - Refusals print as the CLI's usual `MULTRA: CODE: path` lines, with exit 1. A JSON issue list
    is not written; §6 is amended to that.
- **Observed at seed 42.** Simulated, not validated (rule 4).

| n | δ | short (s, vehicles) | long (s, vehicles) |
|---|---|---|---|
| 1 (19 : 1) | 0.8103 | 170.42 (216) | 92.04 (11) |
| 2 (0.475 : 0.525) | 0.0108 | 70.19 (111) | 68.68 (116) |

  - The loop stops converged at n = 2.
  - **Before §13.1,** the 1 : 1 start gave 73.31 / 69.42 s with δ = 0.0269 at n = 1.
  - **Two MSA iterations are not a measure of MSA's speed here.** The one step from 19 : 1 halves
    to 0.475 short, which happens to lie inside the gap.
- **Rows:** `tests/dynamic_assignment_loop_tests.cpp` (group `dynassignloop`) covers DA7 and DA8.
- **By hand, the CLI on the fixture's JSON** (generated in a scratch directory, not committed):
  - two runs were `cmp`-identical;
  - `--iterations 2 --gap 0` printed the "not converged after 2 iterations" line and exited 0;
  - an existing OUT was refused;
  - `--route-costs` with the written file ran (227 completed).
- **The M9 gate.** ROADMAP's "Done when" asks for:
  - an analytic two-route fixture converging within the stated tolerance (A1, Example 10.6 → 11.1,
    exact; A3, the Micro pair within δ 0.05);
  - same inputs giving the same bytes (DA8);
  - non-convergence flagged (DA1, DA8).

  Every part is now asserted by a row. Closing the gate is the owner's call (rule 8). Placed and
  positioned decisions (A2b) stay open.

## 16. As implemented (A2b, 2026-11-13)

- **Code:**
  - `expandRouteless(..., RouteFamilies*)` fills `RunSnapshot::routeFamilies` in
    `compileDocument`.
  - `decisionFamilyName` mirrors `familyName`.
  - `planDynamicAssignment(def, level)`:
    - drops the A2 refusal of placed decisions;
    - refuses entries sharing a final object;
    - for Meso, refuses positioned decisions.
  - `measureDecisionCosts` tags a vehicle per decision. Placed decisions are tagged at departure,
    by the departure route's families; positioned decisions at their `RoutingEvent`, by the chosen
    route's families. A vehicle may carry several tags.
  - The positioned free-flow fill subtracts the gate distance (`RouteDecision::at`).
  - `applyRouteCosts` no longer treats a placed decision as stale.
- **Rows:** `tests/dynamic_assignment_placed_tests.cpp` (group `dynassignplaced`) covers DA10–DA13.
  DA9's placed-refusal line now asserts DA12's Meso refusal, as D551 changes that rule.
- **One test fix, no value changed.** The first DA10 and DA11 runs mapped a vehicle to its entry
  by its *departure* route in the independent recomputation. On a stub, which a turn's other lane
  is, that route ends before the exit, so 8 and 5 of the left-turners were dropped. The
  recomputation now maps by the route the vehicle *arrives* on, or is on at the end. Then every
  cell's count and mean cost equal the measurement to 1e-9.
- **Consequence of §15's family rule.** A placed decision without a position cannot choose
  between routes to the same destination, because they share a family. Use a positioned decision
  (Micro) or an unplaced one (Micro and Meso). MESO_ASSIGNMENT §10.1 records the fixture that
  showed it.

## 18. As implemented (A4, 2026-11-13)

- **Code:**
  - `src/shell/dynamic_assignment_panel.*`, modelled on the Procedure page. It is wired in
    `editor_results.cpp` (construct, translate, refresh, CSV) and `editor_run.cpp` (Export and Copy
    state), with its key in `results_levels.cpp`'s Micro list.
  - `iterateMsa` and `runDynamicAssignment` take `keepGoing`. The result carries `cancelled`; a
    cancel before the first iteration returns an empty, cancelled result.
  - Apply calls `applyRouteCosts` inside one History step, `editorDynamicApplyStep`.
  - en and th locale keys, including the four `DYNAMIC_ASSIGNMENT_*` and `ROUTE_COSTS_*` codes.
- **Rows:** `dynamic-assignment-ui` (`tests/dynamic_assignment_ui_tests.cpp`) covers DA14–DA17,
  and all passed on the first run.
- **The page, as observed.** An offscreen capture at 1440 × 900, English, after A3's fixture ran
  and viewed only by the session, showed:
  - the settings row;
  - the note "Converged after 2 iterations (δ 0.0108)";
  - two δ rows (0.8103, 0.0108);
  - the cells 0.4750 / 0.5250 at 70.19 / 68.68 s.
- **Owner check:** OWNER_SITTING_WINDOWS C17.
- **The first full check failed two inventory pins, both changed openly:**
  - `workspace-ui` counts 13 Results tabs, not 12;
  - `results-tabs-ui` moves Meso to index 6, Macro to 10, Macro's first tab to 7 and Node (HCM)
    to 12.
  - The page also joins D497's tabs without data: it is left out of the Micro strip in a project
    with no routing decision. So the visible Micro count in `results-tabs-ui` is unchanged.
