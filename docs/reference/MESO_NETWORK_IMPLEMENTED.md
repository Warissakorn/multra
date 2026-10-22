# Meso network — as implemented (M11.2–M11.4b)

What each slice of [MESO_NETWORK](MESO_NETWORK.md) built, kept apart from the contract so that file
stays under 500 lines. The section numbers are the contract's, so "MESO_NETWORK §17" still names a
section here. Every Meso result carries the not-validated marker (rule 4).

## 14. M11.2 as implemented (D293)

- **The graph** (`src/meso/graph.hpp`, imports nothing):
  - `MesoLink`, `MesoTurn`, `MesoConflict`, `MesoNode` and `MesoGraph` (links, turns grouped by
    node, entries, exits);
  - `MesoSettings` with `kMesoDefaults{1, 300}`;
  - `mesoStepLimit` and `mesoShortestLink`.
- **The compile** (`compileMesoNetwork`, `src/model/demand/meso_compile.*`):
  - stations as Macro's, plus every valid Node's crossings;
  - pieces whose midpoint lies inside a valid Node are left out;
  - station groups by union-find over outside Connectors and each Node's crossings;
  - one node per group a link touches, ordered by the group's least (Link id, station).
  - `mesoNetwork(d, data)` (`src/project/meso_input.*`) resolves the link types and adds
    `linkTypeIssues`.
- **The file** (`src/project/meso_settings.*`, `link_type_codec.*`): `mesoSettings` and
  `jamDensity` at schema 52, `EDIT_UNSUPPORTED_FIELD` below it. The future-schema rows moved to 53.
- **Rows:** ME1 and ME2 in `tests/meso_network_tests.cpp` (ctest `mesonetwork`).
  - ME1 covers the T-junction and four-leg Nodes (4 and 12 turns), a junction without a Node, two
    lane Connectors merged into one turn, and a refused Node compiled as if it were not drawn.
  - ME2 covers the jam density and the settings, the step limit, and the shipped projects'
    schemas.
- **Mutations caught:**
  - interior pieces kept;
  - the through turn dropped;
  - lane Connectors not merged;
  - the schema gate removed;
  - the critical-density check removed;
  - a refused Node treated as valid.

  Two of them survived a first version of the rows. Lane Connectors not merged needed a fixture
  with two lane Connectors. A refused Node treated as valid needed the Node over the junction
  rather than in empty space.

## 15. M11.3a as implemented (D295)

- **The values** (`src/meso/demand.hpp`, imports nothing):
  - `MesoInterval`, `MesoPath` (its route, links, the turns between them, and veh/h intervals) and
    `MesoClass` (id, PCU, paths);
  - `mesoPathFlow`, `mesoTurnShares` (per turn, the class's share of its in-link's flow at t) and
    `mesoEntryFlows` (per link, veh/h entering at its upstream end).
- **The mapping** (`mesoPath`, `src/model/demand/meso_paths.*`):
  - Each Link of the route is driven over the range its Connectors give, and its Meso links in that
    range are taken in order, so a Node's interior drops out.
  - The turn joining consecutive links comes from the graph, which keeps one per (in, out) pair.
- **The demand** (`mesoDemand`, `src/project/meso_input.*`):
  - It expands the document as the Micro compile does (OD, then decisions).
  - Inputs on one route and class share a path. Classes and paths are ordered by id.
- **Rows:** ME12–ME16 in `tests/meso_demand_tests.cpp` (ctest `mesodemand`).
  - ME12 runs the four-leg with its suggested Node (12 routes, one Node turn each), the same project
    without it, and the T-junction's through turns.
- **Mutations caught:**
  - an inclusive interval end;
  - classes merged by vehicle type;
  - the routeless refusal removed;
  - the Connector join check removed;
  - shares not normalised.

## 16. M11.3b as implemented (D296)

- **The expansion is Micro's.**
  - `mesoDemand` runs `routelessIssues` and `expandRouteless` over the routeless inputs, after
    `resolveCatalogs`, as `compileDocument` does. Lane splits, slice shares and typed routing are
    therefore Micro's, with no second copy.
  - Each part returns to its authored input by id (`<id>/type-…` included), so a composition's
    type parts sum back into its class.
- **Lanes to objects:** `mesoObjectChain` (`src/model/demand/meso_paths.*`) maps lane sections to
  their Link and connector paths to their Connector, then `mesoPath` maps the chain as in M11.3a.
- **Stubs:** a route in `routeDeadEnds` hands its flow to the full routes its `laneChanges` lead to,
  stub after stub. The flow is split equally in id order.
- **Rows:** ME17–ME19 in `tests/meso_demand_tests.cpp` (ctest `mesodemand`). ME19 loads the M2.6
  template's whole demand (its four placed decisions), conserved at four times.
- **Mutations caught:**
  - the stub hand-off dropped;
  - a connector path not mapped to its Connector;
  - type parts not summed back;
  - the walk's issues not reported.

## 17. M11.3's node model as implemented (D297)

- **`mesoNodeFlows`** (`src/meso/node_model.*`, imports nothing) is the paper's §5.1.2 algorithm,
  step for step:
  - the sets `U_j` and `J`;
  - the reduced supplies `R̃_j`;
  - `a_j` by Eq. 30 and the most restrictive `ĵ`;
  - 4(a), demand-constrained in-links, before 4(b), the capacity-proportional share.

  An in-link without capacity sends nothing.
- **The advisory:** `MesoCompile::advisories` carries `MESO_PRIORITY_IGNORED` for a valid Node with
  conflicts.
- **Rows:** ME20–ME23 in `tests/meso_node_model_tests.cpp` (ctest `mesonode`).
  - ME20 reproduces the paper's Table 8.
  - ME22 checks the requirements on 300 generated nodes, over 50 of them congested.
- **Mutations caught:**
  - step 4(a) skipped;
  - a demand-proportional rule in place of oriented capacities;
  - the advisory removed.
- **Mutations that changed nothing:**
  - a per-turn `min` in 4(b) is a no-op there, since every in-link in 4(b) has `S_i > a C_i`;
  - the tie order among equally restrictive out-links, since the solution is unique.

## 18. M11.3c as implemented (D298)

- **The link model** (`src/meso/link_model.*`, imports nothing):
  - `MesoCurve`: counts at each step, interpolated linearly, with their inverse;
  - `mesoSending` and `mesoReceiving` (Eqs. 4.31 and 4.35), `mesoStepCapacity` and `mesoStorage`;
  - `mesoWaveSpeed` and `mesoLinkTimeLimit` sit in `graph.*`, which `mesoStepLimit` uses.
- **The engine** (`loadMeso`, `src/meso/loading.*`): a commodity per class and path.
  - Per link: entrance and exit counts, and the sending and receiving volume per step, which is §9's
    boundary interface.
  - Per turn: the cumulative transfer.
  - Per commodity: demand, entered and arrived vehicles.
  - A path with no PCU, a repeated link or turns that do not join is refused, not loaded.
- **The node model** accepts an infinite receiving volume, which is the destination column.
- **The advisory:** `MESO_SIGNAL_IGNORED` for a valid signalised Node, retired by M11.3d (D299, §19).
- **Rows:** ME3–ME6, ME8, ME9 and ME24–ME26 in `tests/meso_loading_tests.cpp` (ctest
  `mesoloading`).
  - ME6 reproduces Example 4.5 exactly.
  - ME8 reproduces §7.1: `k_B` 166.67 veh/km; r2m clears at 44 min against the thesis's 46; the r1n
    queue lasts 26.25–82.75 min against 28–82.
  - ME5's shock crosses each piece in 275 s, exactly the theory's figure.
- **Mutations caught:**
  - the receiving lag dropped;
  - the sending lag dropped;
  - Eq. 5.2 replaced by the entrance composition at t (ME25);
  - the step limit back to L/v only (ME24);
  - a finite destination (ME8);
  - the infinite-receiving guard removed (the node model then reads out of range).

## 19. M11.3d as implemented (D299)

- **Turns** carry `signal` (the program id) and `green` (g/C).
  - `compileMesoNetwork` maps each valid Node's heads to the turns whose vehicles pass them.
  - `mesoGreenShares` (`src/model/demand/meso_compile.*`) gives g/C per program; `mesoNetwork` passes
    it in.
- **The node model** takes `MesoNodeStep::internal` supplies (Eqs. 33–35) as an effective capacity
  per in-link, with its demand capped by invariance. With no supplies it is unchanged (ME20–ME22
  pass).
- **The engine** adds one supply per in-link and program, `N = (g/C) C_i S_p / S_i`.
- **Retired:** `MESO_SIGNAL_IGNORED`. Added: `MESO_SIGNAL_AMBIGUOUS`.
- **Rows:** ME7 and ME26 in `mesoloading`, ME27 in `mesonode`.
- **Mutations caught:**
  - supplies ignored;
  - the demand cap removed;
  - `b = 1` instead of `S_p / S_i`;
  - the wrapped green computed wrongly;
  - an ambiguous turn constrained anyway.

  A first version reduced claims per movement (Eq. 35 read literally). It broke `q_j ≤ R_j` on ME27's
  generated cases when an in-link's turns sat in different phases, so the effective capacity replaced
  it.

## 20. M11.4 as implemented (D300)

- **The run** (`runMeso`, `src/project/meso_run.*`):
  - its stages are graph, demand and loading; the caller owns inputs and outputs;
  - `loadMeso` takes a progress callback, and false cancels;
  - `MESO_PATH_NOT_LOADED` is advised for a refused path.
- **The outputs** (`mesoJson`, `mesoCsv`, `mesoLinkSlices`, `mesoTravelTime`,
  `src/project/meso_output.*`) are as §7.
- **The CLI** is `--meso` in `tools/run_simulation.cpp`, with the ctests `cli-meso`, `cli-meso-seeds`
  and `cli-meso-refused`.
  - Fixture: `tests/meso-two-route.traffic.json` is the Macro two-route project with a test
    `jamDensity`, at schema 52.
- **The editor** (`src/shell/editor_meso.*`):
  - Run Meso and Cancel Meso on the Results toolbar;
  - the Meso links tab, with the marker note, the stages, the step reached and a stale notice by
    revision;
  - one table row per link: entered, left, mean travel time, max queue, spill-back slices;
  - Export and Copy give the Meso CSV;
  - the result is discarded on New, Open and Recovery.
  - `workspace-ui`'s tab inventory now counts the seventh Results tab.
- **Rows:**
  - ME10 in `tests/meso_output_tests.cpp` (ctest `mesooutput`);
  - ME11 in `tests/meso_ui_tests.cpp` (ctest `meso-ui`, offscreen Qt).
  - The `MESO_PATH_NOT_LOADED` clause is checked through `MesoLoading::refused`: no shipped fixture
    has a refused path.
- **Measured on the fixture** (1200 trips in an hour over 600 pcu/h/lane links): 1164 of 1200
  vehicles entered and 1132 arrived, with VHT 29.6 pcu-h and delay 18.0 pcu-h. The links are
  congested, which is expected.
- **Mutations caught:**
  - the travel time from the wrong curve;
  - the spill-back flag a step late;
  - a cancel ignored;
  - the CSV marker removed;
  - the stale check removed (ME11).

## 21. M11.4b as implemented (D301)

- **The engine** (`src/meso/loading.*`):
  - per link and commodity slot, an admitted curve (what the node model passed) and an entrance
    curve, the admitted one shifted by the free time τ of the turn into the link: `up(k·Δt) =
    admitted(k·Δt − τ)`, linear between steps;
  - a path's first link has τ = 0, and a link whose slots all have τ = 0 keeps the old arithmetic
    exactly;
  - the receiving volume reads `MesoLoading::admitted`; sending, the Eq. 5.2 composition and the
    outputs read `up`.
- **The outputs** (`src/project/meso_output.*`):
  - turn rows carry `freeTime`;
  - VHT and the free time both add `Σ τ·turned(T − τ)`;
  - `inNodesAtEnd` is `Σ turned(T) − turned(T − τ)`.
- **Rows:**
  - ME28 in `tests/meso_loading_tests.cpp` (ctest `mesoloading`);
  - ME10's conservation and totals in `tests/meso_output_tests.cpp`.
- **Measured on the fixture** (the same project as §20; its simple-node turns are timed by their
  Connectors, 3.3–23.2 s):
  - 1082 of 1200 vehicles entered and 1044 arrived (1164 and 1132 before);
  - VHT 37.9 pcu-h and delay 21.2 pcu-h (29.6 and 18.0 before);
  - 4.9 PCU inside nodes at the end.

  The delay rises because vehicles inside a node take up the out-link's storage.
- **Mutations caught:**
  - the receiving volume read from `up`;
  - τ ignored;
  - τ with the wrong sign;
  - node time left out of VHT (ME10).
