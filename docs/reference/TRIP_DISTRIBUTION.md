# Trip distribution (M10.3)

Written 2026-11-06 (D472) as a contract with nine owner choices (§11), after slice D0 read
Ortúzar and Willumsen Ch. 5 from the page images ([evidence](../evidence/ortuzar-ch5-trip-distribution.md)).
The owner accepted every recommendation (D473: 1a–9a); D1 (D473) and D2 (D474) are implemented (§14). Singly constrained purposes are contracted in §15 (D475), with owner choices open. This is the second four-step stage
([MACRO_NETWORK](MACRO_NETWORK.md) §1, §15; [TRIP_GENERATION](TRIP_GENERATION.md) §9). It turns each
purpose's productions and balanced attractions into a production–attraction (P-A) trip table,
using a gravity model with exponential deterrence balanced by Furness, as D158 chose.

Rule 4 applies. Every distribution result carries the not-validated marker. Reproducing a
textbook example checks arithmetic, not behaviour, and removes no marker ([BOOKS](BOOKS.md) §1).

## 1. Scope

- **In M10.3:**
  - per purpose, T_ij = a_i b_j exp(−β c_ij), balanced to P_i and A*_i by Furness (§3);
  - the stopping rule and its flag (§4);
  - zone-to-zone costs (§5, choices 1–2);
  - deterrence parameters as data (§6, choice 3);
  - outputs in JSON and CSV, the CLI and one editor surface (§8, choices 6–7).
- **Not in M10.3:**
  - calibrating β (Hyman's method, Ortúzar §5.5; choice 8);
  - mode choice, person-to-vehicle conversion and the study-period factor (M10.4;
    TRIP_GENERATION §5);
  - converting the P-A table to directional OD (Ortúzar §5.4.4, p. 190–191; M10.4);
  - the procedure sequence (MACRO_NETWORK §19 c);
  - feedback of congested costs (Ortúzar Ch. 11);
  - external and gate zones (Ortúzar §5.8.2; TRIP_GENERATION §9);
  - growth-factor updating of an observed matrix (§5.2) and tri-proportional fitting (§5.6).

## 2. Today (measured from the code on `73894c3`)

- **`runTripGeneration`** (`src/project/trip_generation_run.hpp`) gives per purpose
  `productions`, `attractions` (balanced, ΣA* = ΣP) and `totals`, over `zoneIds` in id order,
  plus the set's `unit`. Nothing is stored.
- **The Macro graph** (`src/macro/graph.hpp`) maps each zone id to an origin and a destination
  node (`MacroGraph::origins`, `destinations`). Each arc has a `freeTime` and the modes it allows.
  `compileMacroNetwork` (`src/model/demand/macro_compile.hpp`) builds it from the network, zone
  connectors, link types and VDFs.
- **`shortestTree`** (`src/macro/assign.hpp`) is Dijkstra from one node over the arcs a mode may
  use, with a fixed tie rule (MA7). Unreached nodes keep an infinite distance.
- **No skim, matrix or deterrence type exists.** `odMatrices` are Micro and Macro demand in
  trips per period ([MACRO_NETWORK](MACRO_NETWORK.md) §6).

## 3. The model (Ortúzar Eq. 5.7, Example 5.4)

- **Deterrence.** For purpose p, f_ij = exp(−β_p c_ij), with β_p > 0 in 1/cost unit. An
  unreachable pair (c_ij = ∞) has f_ij = 0.
- **Seed.** t_ij = f_ij · T / Σ f, where T = Σ_i P_i. This is Example 5.4's expansion. It
  changes no converged cell. It makes a fixed-pass replay match the book, and it fixes the scale
  of the factors.
- **Furness.**
  - One **pass** corrects every row to P_i, then every column to A*_j. Rows go first, which
    Example 5.3 shows (§10).
  - A row or column whose target is 0 is set to zeros, and its factor becomes 0 (as built,
    D473), so T_ij = a_i b_j f_ij holds everywhere.
  - a_i is the product of row i's corrections. b_j is the product of column j's corrections ×
    T / Σ f. Then T_ij = a_i b_j f_ij, as the book prints them.
- **Condition.** Furness needs Σ P = Σ A* (Eq. 5.8, p. 181). TRIP_GENERATION §4 guarantees it. An
  input that breaks it by more than 10⁻⁹ relative is `DISTRIBUTION_UNBALANCED`, at the purpose.
  It can only come from a hand-built input, never from M10.2.
- **Uniqueness.** a_i and b_j are unique only up to a constant (p. 190). The outputs give the
  book's normalisation above and say so.

## 4. Stopping (choice 4)

- **Measure:** the largest relative trip-end error after a pass:
  max(maxᵢ |Σⱼ T_ij − P_i| / P_i, maxⱼ |Σᵢ T_ij − A*_j| / A*_j), over the nonzero targets.
  Columns are exact after a pass, so this is in effect the row error.
- **Target:** the target is choice 4. It is a run setting with a cap of 1000 passes. The book
  gives no number: "within 3 to 5 %" in a few iterations, and tighter "may be important" (p. 180).
  So the value is the owner's, recorded as such (D158 asked for a citation; there is none).
- **Not converged:** reaching the cap gives `DISTRIBUTION_NOT_CONVERGED`. The result is still
  written, flagged with the error reached, never hidden, as `MACRO_NOT_CONVERGED`
  (MACRO_NETWORK §7).
- **Infeasible:** a zone with P_i > 0 whose every f_ij·A*_j is 0 (nothing reachable that attracts)
  cannot be balanced. It is `DISTRIBUTION_ROW_IMPOSSIBLE` at `zones[i].<purpose>`, or
  `DISTRIBUTION_COLUMN_IMPOSSIBLE`. This check runs before any pass (choice 5).
- **Fixed passes:** a test-only option runs exactly n passes, for the book replays (§13). It is
  not a document field.

## 5. Costs (choices 1–2)

- **The skim (choice 1a):**
  - c_ij is the shortest free-flow time in minutes on the Macro graph, from zone i's origin node
    to zone j's destination node. It uses `shortestTree` at `freeTime` over the arcs of the run's
    named mode (empty: every arc).
  - The graph comes from `compileMacroNetwork` as Run Macro builds it, and its refusals are the
    run's.
  - **Zone to zone (as built, D473):** the graph's centroids are per zone connector. A zone's cost
    is the share-weighted mean of the connector-pair times, using the split Macro demand uses
    (ZONES_AND_PARKING §3). A positive-share pair with no path, or a zone with no connector of that
    direction, makes the pair unreachable (∞).
  - No congested skim: feedback is out of scope (§1).
- **Matrix override (choice 1a):** a project cost matrix, if present, replaces skim cells it names.
  It is `{zone ids, rows, unit: "min"}`. Its shape faults are `INVALID_COST_MATRIX` at
  `costMatrices[k]…`. It enters at the schema step of choice 3a.
- **Intrazonal (choice 2a, Ortúzar §5.8.3, pp. 201–202):**
  - The book prefers taking intrazonal trips out of the gravity model, as a fixed proportion of
    the trip ends.
  - Each purpose names a share s_p in [0, 1). Then T_ii = s_p · min(P_i, A*_i) is set aside, and
    its value is subtracted from both ends before Furness.
  - The diagonal is excluded from the gravity model (f_ii = 0).
  - With no share, s_p = 0: nothing is set aside, and the diagonal stays out (as built, D473).
  - A diagonal is used only from an override matrix (D2). The engine's `useDiagonal` flag is how
    TD2 runs Example 5.4.
- **Units:** costs are minutes and β is per minute. A matrix in another unit is refused, not
  converted.

## 6. Data and storage (choice 3)

- **A deterrence set** is the analogue of a trip-rate set (TRIP_GENERATION §6.1):
  `{id, name, citation, table, function: "exponential", costUnit: "min", purposes: [{id, beta, intrazonalShare?}]}`.
  - Purposes match the trip-rate set's purpose ids. A rate-set purpose missing from the
    deterrence set is `DISTRIBUTION_PURPOSE_MISSING` at `<setPath>.purposes`.
  - β must be finite and > 0, and a share must be in [0, 1). Faults are
    `INVALID_DETERRENCE_SET` at the JSON path.
- **Catalog** `data/deterrence/*.json`, read as `data/trip-rates/` is. **None ships (as
  TRIP_GENERATION 8a):** β is local and dated, and Example 5.4's 0.10 is an illustration.
- **Owned sets** `deterrenceSets` (and `costMatrices`, §5) at **schema 67**, written only when
  present. Below 67 they are `EDIT_UNSUPPORTED_FIELD`. They follow the owned-before-catalog
  lookup and `DUPLICATE_ID` clash rule of TRIP_GENERATION §6.3.
- **Test data** in `tests/data/trip-distribution/`: Example 5.4's set (β 0.10), its cost matrix,
  and a set for the hand network.

## 7. Engine (`src/macro/`) and project layer

- **`src/macro/trip_distribution.{hpp,cpp}`** is pure, imports nothing, and has the allowed
  headers only. Its signature is `distributeTrips(costs, productions, attractions, beta, share, settings)`,
  returning `{T, a, b, intrazonal, passes, error, converged}` with index-located issues.
  Fixed index order, no clock, no seed.
- **`src/macro/skim`** (or a function beside `shortestTree`): `freeFlowSkim(graph, zoneIds, mode)`
  gives an n×n matrix in minutes, with ∞ where a pair is unreachable.
- **`src/project/trip_distribution_run.{hpp,cpp}`:**
  `runTripDistribution(doc, data, rateSetId, deterrenceSetId, mode, InputManifest*, stage)`.
  It calls `runTripGeneration`, then runs the stages costs → deterrence → balancing → outputs.
  The first refusal names its stage, as TRIP_GENERATION §7.

## 8. Outputs (choice 6)

- **Per purpose:**
  - the P-A matrix over `zoneIds`;
  - a_i, b_j (book normalisation, §3) and the intrazonal trips;
  - passes, the error reached, and `converged`;
  - the mean cost (Σ T c / Σ T);
  - trip-length bands of a run-chosen width (default 4 min, as Example 5.4's table);
  - the deterrence and rate set ids and the unit, carried through.
- **JSON** at full precision. **CSV:** one row per purpose and OD pair, plus a totals row per
  purpose, two decimals, with TRIP_GENERATION §8's marker and number format. The same input
  gives the same bytes.
- **Not stored (choice 6a).** M10.4 makes the stored, per-mode OD matrices.
- **CLI:** `--trip-distribution <rateSetId> --deterrence <setId> [--skim-mode <mode>]`, with
  `--project`, `--csv` and `--data-dir` only. It is exclusive of the other run selectors, as
  `--trip-generation`.
- **Editor (choice 7a):** a *Trip distribution* Results page built as `TripGenerationPanel`:
  - a choice of the two sets and the mode, and a Run on a worker with stages;
  - a table (purpose, origin, destination, trips, cost) and a summary line per purpose (passes,
    error, mean cost);
  - stale marking, Copy and Export; no History step;
  - a *Deterrence sets…* list dialog with Import/Export JSON, as D471's.

## 9. Seams

- **M10.4** reads each purpose's P-A matrix and unit. It owns the conversion to OD and to
  vehicles, and the period factor.
- **M10.1:** distribution never writes `odMatrices` (choice 6a). The skim uses the same compiled
  graph and Dijkstra as assignment, so a network change shows in both.
- **Feedback** (Ortúzar Ch. 11) and the procedure sequence wait on M10.4.
- **Calibration** (choice 8): a fitted β is still just a deterrence set.

## 10. Benchmark and sources

Read in D0 (2026-11-06; [evidence](../evidence/ortuzar-ch5-trip-distribution.md)), each number from
its page image:

- **Example 5.3** (book p. 181), Tables 5.6 → 5.7. Three row-first passes reproduce every printed
  cell to two decimals, and columns-first does not. Row TD1.
- **Example 5.4** (book pp. 189–190), Tables 5.8 → 5.10 with β = 0.10. Three row-first passes
  reproduce the cells, a_i, b_j and the band table. The book's row-2 sum (455.56) is a misprint
  for 455.47. Converged row 1 is `157.04 100.36 66.14 76.46`. Row TD2.
- **No convergence number in the book** (p. 180; Ch. 11 searched): choice 4 is the owner's.
- **Practice** (§5.8.3–§5.8.4, pp. 201–202): intrazonal trips taken out (choice 2), and non-work
  purposes usually singly constrained (choice 9).

## 11. Owner choices (answered 2026-11-06, D473: 1a–9a)

The owner: "ตามแนะนำ". The options are kept for the record.

1. **Cost source.**
   - **(a) The free-flow skim, with an optional project matrix overriding its cells
     (recommended).** The network is already in the project, and a study with surveyed times can
     still use them.
   - (b) An imported matrix only.
   - (c) The skim only.
2. **Intrazonal trips.**
   - **(a) A per-purpose share taken out of the gravity model (recommended),** §5, as the book
     prefers (§5.8.3).
   - (b) Diagonal costs in the gravity model: a per-zone intrazonal cost, required, with no
     default.
3. **Where β lives.**
   - **(a) Deterrence sets: a catalog with none shipped, plus project-owned sets and cost
     matrices at schema 67 (recommended),** §6. β changes with the cost definition, not with the
     rates.
   - (b) A `beta` inside each trip-rate set purpose.
4. **Furness target** (no book number, §4).
   - **(a) 0.1 % (D158), with a cap of 1000 passes (recommended).** Example 5.3 needs 5 passes for
     it.
   - (b) 0.01 %, to match the Macro gap (D272).
5. **A zone that cannot be balanced.**
   - **(a) Refuse, at the zone and purpose, before any pass (recommended).** Oscillating to the cap
     hides the cause.
   - (b) Run anyway and flag `DISTRIBUTION_NOT_CONVERGED`.
6. **Output.**
   - **(a) A run result, not stored (recommended),** as generation is. M10.4 makes the stored
     matrices.
   - (b) Write `odMatrices` now, in persons.
7. **Editor.**
   - **(a) A Trip distribution Results page and a Deterrence sets dialog (recommended),** in D2.
   - (b) The CLI only for M10.3.
8. **Calibration.**
   - **(a) Not in M10.3 (recommended).**
   - (b) Hyman's method on an observed trip-length distribution.
9. **Constraint per purpose** (D158 says doubly constrained; the book says that is for work trips,
   p. 202).
   - **(a) Doubly constrained for every purpose now, singly constrained a later slice
     (recommended).** This keeps D158, and the singly constrained case is a one-line variant once
     the engine exists.
   - (b) A per-purpose `constraint: double | production` in the deterrence set now.

## 12. Slices

| Slice | Content | Rows |
|---|---|---|
| D0 | Read Ortúzar Ch. 5 into an evidence file; this contract. **Done (D472)** | — |
| D1 | Deterrence codec and catalog, the free-flow skim, `src/macro/trip_distribution`, the run, JSON/CSV and `--trip-distribution`. **Implemented (D473)** | TD1–TD11 |
| D2 | Owned deterrence sets and cost matrices at schema 67, the *Deterrence sets…* dialog, the *Trip distribution* page. **Implemented (D474)** | TD12–TD14 |
| D3 | Singly constrained purposes (§15). **Contract written (D475); owner choices open** | TD15–TD20 |

## 13. Acceptance rows (implemented: TD1–TD11 D473, TD12–TD14 D474)

Each row first asserts that its forcing worked (AGENTS).

| Row | Case | Asserts |
|---|---|---|
| TD1 | Example 5.3, three fixed passes, prior as the seed | Every Table 5.7 cell to ±0.005; columns exact; columns-first differs |
| TD2 | Example 5.4, β 0.10, override matrix with diagonal, three fixed passes | Table 5.10's cells, a_i, b_j (to ±0.005) and band table (to ±0.05); converged run row 1 = 157.04, 100.36, 66.14, 76.46 |
| TD3 | Converged run | Every trip end within the target; `converged`; the passes counted |
| TD4 | Cap | Cap 1 on Example 5.4 gives `DISTRIBUTION_NOT_CONVERGED` with the error reached and a written result |
| TD5 | Infeasible | A zone whose every destination is unreachable is `DISTRIBUTION_ROW_IMPOSSIBLE` at its path; no result |
| TD6 | Unbalanced input | Σ P ≠ Σ A by 10⁻⁶ is `DISTRIBUTION_UNBALANCED` |
| TD7 | Skim on a hand network | Free-flow minutes by hand, including via-connector times; ∞ for an unreachable pair; a mode-restricted link avoided |
| TD8 | Intrazonal share | T_ii = s · min(P_i, A*_i); the remaining ends balance; f_ii unused |
| TD9 | Deterrence codec | Round-trip; each bad shape refused at its path; a missing purpose is `DISTRIBUTION_PURPOSE_MISSING` |
| TD10 | Determinism | Same bytes twice; zones and links permuted in the file give the same bytes |
| TD11 | CLI | Marker line in JSON and CSV; unknown set ids refused; selector conflicts refused |
| TD12 | Owned sets | Schema-67 round-trip; no owned sets keeps old bytes and schema; older schemas refuse the fields; accept is one Undo step |
| TD13 | Editor page | A run is no History step; rows per purpose and pair; a network or attribute edit marks it stale |
| TD14 | Shipped projects | Every Micro, Meso, Macro and generation output byte-identical |

## 14. As implemented (D1, D473)

- **Files:**
  - `src/macro/trip_distribution.*` (pure: `furness`, `tripEndError`, `distributeTrips`,
    `meanCost`, `tripLengthBands`);
  - `freeFlowSkim` in `src/macro/assign.*`;
  - `src/project/deterrence_codec.*` (`parseDeterrenceSet`, `deterrenceSetJson`,
    `resolveDeterrenceSets`);
  - `src/project/trip_distribution_run.*` (`runTripDistribution`, stages generation →
    deterrence → costs → balancing);
  - `src/project/trip_distribution_output.*`;
  - `--trip-distribution`, `--deterrence` and `--skim-mode` in `tools/run_simulation.cpp`.
- **Settings:** the defaults of choice 4a (0.1 %, 1000 passes). Neither the CLI nor the document
  sets them yet. Bands are 4 min wide.
- **CSV:**
  - a comment line per purpose with β, passes, error and `DISTRIBUTION_NOT_CONVERGED` when it
    applies;
  - columns `purpose,originId,destinationId,trips,cost`;
  - an unreachable pair's cost is blank;
  - the `totals` row's cost cell is the mean cost.
- **JSON:** an unreachable cost is `null`.
- **CLI:** the CLI warns `DISTRIBUTION_NOT_CONVERGED` on stderr.
- **Catalog:** `data/deterrence/` does not exist (choice 3a), which reads as an empty catalog. The
  rows use `tests/data/trip-distribution/`, which holds Example 4.1 and a one-way hand rate set
  (site produces, town attracts; the fixture's network runs site → town only), the `hand`
  deterrence set (β 0.1, marked as Example 5.4's illustration) and a copy of `data/vdf/bpr.json`.
- **Rows:**
  - `tests/trip_distribution_tests.cpp` (group `tripdistribution`, TD1–TD10);
  - the CLI tests `cli-trip-distribution`, `-conflict`, `-alone`, `-unknown` (TD11).
  - TD1 and TD2 reproduce Tables 5.7 and 5.10 (cells, a_i, b_j, bands) to the printed
    decimals.
- **Mutations:** columns first, b without T/Σf, the diagonal kept, and the row error ignored. Each
  failed at least one row.

### D2 as implemented (D474)

- **Files:**
  - `ProjectDocument::deterrenceSets` and `costMatrices`, with `kLatestSchema` 67;
  - `parseOwnedDeterrenceSets` / `addOwnedDeterrenceSetsJson` / `ownedDeterrenceSetIssues`
    (`deterrence_codec.*`);
  - `src/project/cost_matrix_codec.*`;
  - `setDeterrenceSets` and `setCostMatrices` (`src/commands/trip_rate_commands.*`);
  - `src/shell/owned_list_dialog.*`, the trip-rate dialog's body made generic. `trip_rate_dialog.*`
    now wraps it for trip-rate sets, deterrence sets and cost matrices, and G2's object names are
    kept;
  - the self-owned `TripDistributionPanel` (`src/shell/trip_distribution_panel.*`), installed in
    `editor_results.cpp` with no `EditorWindow` member;
  - `--cost-matrix` on the CLI.
- **Costs:**
  - The skim gives no diagonal. A run-chosen matrix's non-null cells replace skim cells; the
    matrix is the only source of a diagonal.
  - A purpose with an intrazonal share drops the diagonal anyway (choice 2a). Its mean cost and
    bands then leave the intrazonal trips out.
  - A matrix naming a zone the project lacks is refused at run time
    (`COST_MATRIX_ZONE_UNKNOWN`), so deleting a zone stays possible.
- **Lookup:** owned deterrence sets are searched before the catalog. An id clash is
  `DUPLICATE_ID` at `deterrenceSets[i].id`.
- **Rows:**
  - `tests/trip_distribution_owned_tests.cpp` (group `tripdistributionowned`) covers TD12: the
    codec, the commands, the lookup and the override rows. With a full matrix, Example 4.1
    distributes on the fixture where D1 refused it.
  - `tests/trip_distribution_ui_tests.cpp` (`trip-distribution-ui`) covers TD12 save and TD13
    (the page, the stale marking and both dialogs).
  - TD14: the shipped projects keep their bytes. Neither list is written when empty, and TG1
    still passes.

## 15. Singly constrained purposes (D3 contract, D475)

Written 2026-11-06, docs only, as choice 9a's later slice. Evidence:
[§6 of the Ch. 5 check](../evidence/ortuzar-ch5-trip-distribution.md#6-singly-constrained-d3-contract-d475).
Ortúzar §5.8.4 (p. 202) models work trips doubly constrained and most other purposes singly
constrained, because their attractions are poorly known and proxies are used.

### 15.1 Model (Eq. 5.16, p. 183)

- **Per purpose,** `constraint` is either absent or `"double"`, which is today's behaviour, or
  `"origin"`.
- **Origin-constrained:**
  - T_ij = P_i · A_j f_ij / Σ_k A_k f_ik, which is one row normalisation of the deterrence
    matrix;
  - the purpose's attractions are only weights, and column sums float;
  - by the scaling invariance (evidence §6), raw and balanced attractions give the same trips, so
    the run passes the balanced ones it already has.
- **Result fields:** `passes` = 1; `converged` = true; `error` is the row error (0 up to
  rounding); a_i = P_i / Σ_k A_k f_ik; b_j = A_j.
- **Refusals:**
  - `DISTRIBUTION_UNBALANCED` does not apply;
  - a row with P_i > 0 and Σ_k A_k f_ik = 0, including when every weight is 0, is
    `DISTRIBUTION_ROW_IMPOSSIBLE` at that zone and purpose, as in D1.
- **Costs and diagonal** follow §5 and D474 unchanged.

### 15.2 Owner choices (open)

1. **Which constraint.**
   - **(a) `origin` only; destination-constrained booked for later (recommended).** A P-A table
     is fixed on its productions, and §5.8.4's purposes are all production-led.
   - (b) Both `origin` and `destination`.
2. **Where it lives.**
   - **(a) A per-purpose field in the deterrence set, beside β (recommended).** The same rates
     can then be run either way.
   - (b) In the trip-rate set's purpose.
3. **Schema.**
   - **(a) An owned deterrence set that uses `constraint` saves at schema 68; below 68 the field
     is `EDIT_UNSUPPORTED_FIELD` at its path; catalog files carry no schema (recommended).** This
     is the repository's pattern (D471, D474).
   - (b) No bump: the field is accepted at 67.
4. **Intrazonal share when singly constrained.**
   - **(a) T_ii = s · P_i (recommended).** Only the origin end is fixed, and §5.8.3 takes
     intrazonal trips as a proportion of the trip ends.
   - (b) Keep s · min(P_i, A*_i), as for doubly constrained.
5. **Outputs.**
   - **(a) Each purpose reports `constraint`, and per zone its modelled attractions (column sums)
     beside the input weights (recommended).** The CSV comment line and the page note name the
     constraint. The difference between the two attraction columns is what the model predicts.
   - (b) `constraint` only.

### 15.3 Slice D3 and rows (after the answers)

- **Engine:** `DistributionInput` gains a `constraint` (default doubly). The origin path seeds f,
  sets the intrazonal trips aside and makes one row pass. `furness` does not change.
- **Rows:**

| Row | Case | Asserts |
|---|---|---|
| TD15 | Example 5.2 through the origin path, Table 5.4's prior as the weights × deterrence | Every Table 5.5 cell to ±0.05; rows equal O_i exactly; columns 257.8, 464.6, 529.5, 710.1 (the print's 701.2 is a misprint) |
| TD16 | Every f equal | Each origin's shares equal the weights' shares exactly |
| TD17 | Raw against balanced attractions | The same trips |
| TD18 | `"double"` or no field | Every distribution output byte-identical; shipped projects unchanged |
| TD19 | Refusals and schema | `DISTRIBUTION_ROW_IMPOSSIBLE` at its path; a bad `constraint` refused at its path; schema 68 round-trips, older schemas refuse the field |
| TD20 | CLI and page | The constraint and the modelled attractions are shown, with the marker |
