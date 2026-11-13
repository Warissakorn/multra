# Mode choice and OD conversion (M10.4)

Written 2026-11-06 (D477) as a contract with eight owner choices (§10); the owner answered 1a–8a; E1 (D478) and E2 (D479) are implemented (§13). Slice M0 first read
Ortúzar and Willumsen §5.4.4 and Ch. 6–7 from the page images
([evidence](../evidence/ortuzar-ch6-7-mode-choice.md)). This is the last demand stage of the
four-step chain:
- [TRIP_GENERATION](TRIP_GENERATION.md) produces trip ends;
- [TRIP_DISTRIBUTION](TRIP_DISTRIBUTION.md) produces per-purpose production–attraction (P-A)
  tables;
- this stage splits those tables by mode, converts them to directional OD for each study period,
  and converts persons to vehicles;
- the M10.1 assignment ([MACRO_NETWORK](MACRO_NETWORK.md) §6) then reads the result as
  `odMatrices`.

Rule 4 applies. Every result carries the not-validated marker. Reproducing a textbook example
checks arithmetic, not behaviour, and removes no marker ([BOOKS](BOOKS.md) §1). No utility
constant, coefficient, occupancy or period share ships as a default.

## 1. Scope

- **In M10.4:**
  - per purpose and P-A pair, a multinomial logit (MNL) split over the purpose's alternatives
    (§3, choice 1);
  - each alternative's cost from the skim of its network mode, or from an owned cost matrix
    (§4, choice 2);
  - P-A to directional OD per study period by owner shares (§5, choice 4);
  - persons to vehicles by occupancy (§5, choice 5);
  - outputs, the CLI, an editor page, and writing `odMatrices` (§7, choice 7).
- **Not in M10.4:**
  - nested logit (choice 1b's later slice): now [NESTED_LOGIT](NESTED_LOGIT.md) (D480–D481), with
    Example 6.3 as its fixture;
  - estimating or calibrating utility parameters (Ortúzar §6.5.4, Ch. 8; choice 8);
  - logsum feedback into distribution, and the G/MS/D/A structure (§6.5.2);
  - congested-cost feedback (Ch. 11) and the procedure sequence (MACRO_NETWORK §19 c);
  - freight and truck demand (choice 3);
  - transit assignment and bus vehicles;
  - time-of-day choice.

## 2. Today (measured from the code on `1562834`)

- **Distribution output.** `runTripDistribution` gives, per purpose, a P-A trip table
  (`result.trips`) in the rate set's `unit`, which is never interpreted (TRIP_GENERATION §5), and
  the zone order `generation.zoneIds`. It is a run result and is not stored (TRIP_DISTRIBUTION
  choice 6a).
- **Skims.** `freeFlowSkim(graph, origins, destinations, mode)` restricts the links to those whose
  type allows the mode. The distribution run turns it into zone-to-zone minutes, with no diagonal,
  and an owned cost matrix may override cells (D474).
- **Modes and periods.**
  - The modes catalog ships `car` and `mixed` only. D158's mode set (car, motorcycle, bus, truck)
    is not shipped as modes.
  - Study periods are project-owned: `{id, name, start, end}` in clock seconds (STUDY_PERIODS §3).
- **`odMatrices` (ZONES_AND_PARKING §2.3, STUDY_PERIODS §5).**
  - A matrix is `{id, zoneIds, cells, periodId | startTime+endTime, modeId | compositionId}`, and
    its cells are **vehicle trips in the period**.
  - Macro groups a period's matrices by mode into classes, converts them to veh/h, and weights
    them by the mode's derived PCU. M10.4 therefore writes vehicles per mode and never applies
    PCU.

## 3. Model (Ortúzar Eq. 7.9, Eq. 6.8)

For purpose p, pair (i, j) and the alternatives k available for the pair (those with a finite
cost):

```
V_k  = ASC_k + θ_k · c_k(i, j)                 (c in minutes; one ASC fixed at 0, §7.3.1)
P_k  = exp(V_k) / Σ_m∈available exp(V_m)       (β normalised to 1, Eq. 7.9)
T^k_ij = P_k · T_ij                            (persons, P-A, the rate set's unit)
```

- **The coefficients.** θ_k is the alternative's cost coefficient. It may differ per alternative,
  and is normally negative. A positive θ is refused, `MODE_CHOICE_INVALID_SET` at its path,
  because a share that rises with cost is a data error here.
- **Available alternatives.** Only alternatives with a finite cost on the pair take part. A pair
  with trips and no available alternative is `MODE_CHOICE_NO_ALTERNATIVE` at
  `zones[i].<purpose>`, refused before any output, as with distribution choice 5a.
- **Numerics.** The share is computed as exp(V_k − max V), so large costs cannot overflow.
- **The diagonal.** It takes part like any cell, using the cost source's diagonal. The skim gives
  none (∞), so intrazonal trips need a matrix diagonal or an alternative with one (§4).
- **Structure.** This stage runs G/D/MS/A. The distribution cost stays the one chosen in M10.3;
  no logsum is fed back, so Eq. (6.11)'s β ≤ λ is not tested. D548 adds the logsum as an option per
  purpose ([MODE_CHOICE_LOGSUM](MODE_CHOICE_LOGSUM.md)). The book prefers G/MS/D/A for
  non-work purposes (§6.5.2, p. 214), and that stays a later slice.

## 4. Costs (choice 2)

- **Per alternative:**
  - `modeId`: the network mode whose free-flow skim gives c_k;
  - `costMatrix`: optionally, an owned cost matrix (D474's `costMatrices`, minutes) whose
    non-null cells override the skim, and which alone gives a diagonal.
- **An alternative with no `modeId`** (walk, rail) takes its costs from its matrix only. A null
  cell means the alternative is unavailable on that pair.
- **Costs are free-flow minutes,** as in M10.3. Generalised cost (fares, parking, value of time)
  enters only through an owned matrix the study prepares (Ortúzar Eq. 5.2 is outside M10.4).

## 5. P-A to OD, and persons to vehicles (choices 4–5)

- **Directional split** (§5.4.4, p. 191). For purpose p, study period s and alternative k:

  ```
  OD^{k,s}_ij = f_PA(p, s) · T^k_ij + f_AP(p, s) · T^k_ji
  ```

  - f_PA and f_AP are owner shares of the P-A total that travel production→attraction and
    attraction→production in the period. Each is in [0, 1], and f_PA + f_AP ≤ 1.
  - The book's 70 %/15 % is an illustration and is never shipped.
  - A purpose with no factor row for a period contributes nothing to that period; the run reports
    this as an advisory, `MODE_PERIOD_FACTOR_MISSING`.
- **Persons to vehicles** (§11.3, p. 407).
  - An alternative may carry `occupancy` (persons per vehicle, finite and ≥ 1) and a `modeId`. Its
    vehicle trips are OD^{k,s} / occupancy.
  - An alternative without occupancy (bus, walk, rail) is reported in persons only and writes no
    matrix. Bus vehicles are a scheduled service, not demand, and stay outside (§1).
  - Two alternatives with the same `modeId` (car driver, plus car passenger with occupancy) add
    their vehicles in that mode's matrix.
- **Units.**
  - The rate set's unit is still not interpreted. The period shares turn "the rate set's trips"
    into "trips in the period".
  - Only if the rate set counts daily person trips are the cells vehicle trips in the period. The
    output states this conditional beside the marker, as TRIP_GENERATION §5 does.

## 6. Data and storage (choices 2, 4)

- **A mode-choice set** is the analogue of a deterrence set: catalog `data/mode-choice/*.json` (none
  ships), plus project-owned `modeChoiceSets` (E2).

  ```json
  { "id": "...", "name": "...", "source": { "citation": "...", "table": "..." },
    "costUnit": "min",
    "purposes": [ { "id": "HBW", "alternatives": [
      { "id": "car", "modeId": "car", "constant": 0, "timeCoefficient": -0.05, "occupancy": 1.2 },
      { "id": "bus", "modeId": "car", "costMatrix": "bus-times", "constant": -0.8, "timeCoefficient": -0.03 } ] } ] }
  ```

  - **Purposes** match the rate set's ids. A missing purpose is `MODE_CHOICE_PURPOSE_MISSING`.
  - **Alternatives:**
    - ids unique within the purpose;
    - at least one constant equal to 0, else `MODE_CHOICE_INVALID_SET` at the purpose, since only
      differences are identified (§7.3.1);
    - every alternative needs a `modeId` or a `costMatrix`;
    - a `modeId` must name a resolved mode at run time (`UNKNOWN_MODE`);
    - a `costMatrix` must name an owned matrix (`COST_MATRIX_UNKNOWN`), as in D474.
  - The values above illustrate the shape only; they are not values to use.
- **Period factors** are project-owned, because periods are, in a root `periodFactors` at schema 69:
  `[{ "purposeId": "HBW", "periodId": "period-3", "productionToAttraction": 0.4, "attractionToProduction": 0.05 }]`.
  - The pair `purposeId` + `periodId` must be unique, and the period must exist.
  - A purpose id is checked only by a run, as with `costMatrices` zones in D474, so editing a rate
    set stays possible.

## 7. Outputs and the editor (choices 6–7)

- **Run result** (`runModeChoice`). It holds:
  - per purpose and alternative, the person P-A matrix and its shares;
  - per period and alternative, OD persons and, where occupancy is given, vehicles;
  - per period and `modeId`, the summed vehicle matrix;
  - totals, plus the advisories and the input manifest.
- **JSON and CSV:**
  - Both carry the marker and the measure line.
  - The CSV columns are `period,purpose,alternative,originId,destinationId,persons,vehicles,share`,
    with a totals row per period and alternative.
- **CLI:** `--mode-choice SET` together with `--trip-distribution` and `--deterrence`, and
  `--period PERIOD` (repeatable; default every period with a factor).
- **Editor (E2):**
  - A *Mode choice* Results page, with the same shape as the Trip distribution page: a worker
    thread, no History step, and stale marking.
  - A *Mode choice sets…* dialog (the D474 owned-list dialog).
  - A *Period factors* table.
  - **Write OD matrices**, which is one History step. It replaces every matrix carrying
    `"generatedBy": "mode-choice"` and adds one matrix per period and `modeId` (id
    `mc-<period>-<mode>`, `periodId`, `modeId`, vehicle cells over every zone). Matrices the user
    authored are never touched.
  - `generatedBy` is a new optional `odMatrices` field, so E2 raises the schema again (70). A
    matrix without the field keeps its bytes.

## 8. Seams

- **M10.1:** the written matrices are ordinary `odMatrices`. Run Macro needs no change, and PCU is
  applied there (MACRO_NETWORK §6).
- **Micro:** a written matrix is also Micro demand, as any OD matrix is. Its cells being whole
  vehicles is not required (ZONES_AND_PARKING §3 draws trips from the cell).
- **Feedback and the procedure sequence** (MACRO_NETWORK §19 c) can chain generation, distribution,
  mode choice and assignment once this exists. Out of M10.4.
- **Calibration** (choice 8): a fitted set is still just a mode-choice set.

## 9. Benchmark and sources

| Point | Source | Page (book) |
|---|---|---|
| MNL | Ortúzar Eq. (7.9) | 232 |
| One constant fixed at 0 | §7.3.1, Example 7.1 | 232–233 |
| N-way logit fixture | Example 6.2, P_C = 0.33 | 214 |
| Nested logit fixture ([NESTED_LOGIT](NESTED_LOGIT.md) NL1) | Example 6.3, C_B = 49.23 and the λ₁ table | 216 |
| P-A to OD by period shares | §5.4.4 | 190–191 |
| Occupancy | §11.3, η in Eq. (11.18)–(11.19) | 407 |
| Structure and β ≤ λ | §6.5.2, Eq. (6.11) | 213–214 |

## 10. Owner choices (answered 2026-11-06, D478: 1a–8a)

The owner: "เอาตามแนะนำ". The options are kept for the record.

1. **Model.**
   - **(a) An MNL per purpose with linear utilities as data (a constant plus a cost coefficient
     per alternative); nested logit in a later slice (recommended).** It is the book's simplest
     form, and Example 6.3 is ready as the NL fixture.
   - (b) Nested logit now, with nests and φ ≤ 1 (Eq. 7.18) in the set.
2. **Costs.**
   - **(a) Each alternative's free-flow skim by its `modeId`, overridable by an owned cost
     matrix, which is also the only source for an alternative with no network mode
     (recommended).** This is D474's rule, reused.
   - (b) Owned matrices only.
3. **Truck.**
   - **(a) Outside person mode choice: truck demand stays authored `odMatrices` (recommended).**
     Freight is not produced by person-trip rates.
   - (b) An alternative in mode choice.
4. **P-A to OD.**
   - **(a) Owner shares f_PA and f_AP per purpose and period, with no default (recommended).**
     This is the book's "better approach" (p. 191).
   - (b) Half of the P-A total in each direction, with one share per period for the daily
     fraction.
5. **Persons to vehicles.**
   - **(a) Occupancy per alternative as data, with no default. An alternative without one is
     reported in persons and writes no matrix (recommended).**
   - (b) Every alternative must name a mode and an occupancy.
6. **Units.**
   - **(a) Do not interpret the rate set's unit. The output states that its cells are vehicle
     trips in the period only for a daily person-trip rate set (recommended).** This is
     TRIP_GENERATION §5's rule.
   - (b) Require a fixed unit string.
7. **Output.**
   - **(a) E1 gives a run result (JSON/CSV/CLI); E2 adds the page and *Write OD matrices*, which
     replaces only generated matrices in one History step (recommended).**
   - (b) A run result only, with matrices written by hand.
8. **Calibration and feedback.**
   - **(a) Out of M10.4 (recommended).**
   - (b) A Berkson–Theil binary calibration (Example 6.4) now.

## 11. Slices

| Slice | Content | Rows |
|---|---|---|
| M0 | Read §5.4.4 and Ch. 6–7 into an evidence file; this contract. **Done (D477)** | — |
| E1 | Mode-choice codec and catalog; root `periodFactors` at schema 69; the pure `src/macro/mode_choice` (MNL shares, the directional split, occupancy); `runModeChoice`; JSON/CSV; `--mode-choice`/`--period`. **Implemented (D478)** | MC1–MC10 |
| E2 | Owned `modeChoiceSets`; the *Mode choice* page; the sets dialog; the *Period factors* table; *Write OD matrices*. **Implemented (D479)** | MC11–MC14 |

## 12. Acceptance rows (MC1–MC10 implemented D478, MC11–MC14 D479)

Each row first asserts that its forcing worked (AGENTS).

| Row | Case | Asserts |
|---|---|---|
| MC1 | Example 6.2 | Three alternatives at equal cost: P = 1/3 each; drop one and P = 1/2 |
| MC2 | MNL by hand | Two- and three-alternative shares from given V to 1e-12; IIA (the ratio unchanged when a third is added); adding a constant to every V changes nothing |
| MC3 | Availability | An alternative with ∞ cost gets 0; a pair with trips and no alternative is `MODE_CHOICE_NO_ALTERNATIVE` at its path |
| MC4 | Large costs | exp(V − max V): no overflow at 10⁴ minutes |
| MC5 | Directional split | OD = f_PA T + f_AP Tᵀ by hand; f_PA + f_AP ≤ 1 enforced; a missing factor row gives `MODE_PERIOD_FACTOR_MISSING` and no trips |
| MC6 | Occupancy | Vehicles = persons / occupancy; two alternatives on one mode add; an alternative without occupancy writes no vehicles |
| MC7 | Conservation | Σ over alternatives of persons = the P-A total, per purpose |
| MC8 | Codec | Round-trip; each bad shape refused at its path (positive θ, no zero constant, neither `modeId` nor `costMatrix`, occupancy < 1) |
| MC9 | Schema 69 | `periodFactors` round-trips; older schemas refuse it; no factors keeps old bytes |
| MC10 | CLI | Marker line in JSON and CSV; unknown set and period refused; selector conflicts refused |
| MC11 | Owned sets | `modeChoiceSets` round-trip; one Undo step per accept |
| MC12 | Write OD matrices | One History step; replaces only `generatedBy: mode-choice` matrices; the written matrices pass `odMatrices` validation; Run Macro assigns them |
| MC13 | Editor page | A run is no History step; stale after an edit |
| MC14 | Shipped projects | Every Micro, Meso, Macro, generation and distribution output byte-identical |

## 13. As implemented (E1, D478)

- **Files:**
  - `src/macro/mode_choice.*`, which is pure: `logitShares`, `splitModes`, `directionalOd`;
  - `src/project/zone_skim.*`, the zone skim and the cost-matrix overlay, moved out of
    `trip_distribution_run.cpp` unchanged and shared by both runs;
  - `src/project/mode_choice_codec.*`;
  - `src/project/period_factor_codec.*`, with `ProjectDocument::periodFactors` and
    `kLatestSchema` 69;
  - `src/project/mode_choice_run.*`, with stages distribution → set → costs → split → periods;
  - `src/project/mode_choice_output.*`;
  - `tools/four_step_cli.*`, holding the generation and distribution runs moved out of
    `run_simulation.cpp`, plus `--mode-choice`.
- **Sets:** E1 reads catalog sets only (`data/mode-choice/`, none shipped). Owned
  `modeChoiceSets` are E2.
- **Periods:**
  - With no `--period`, the run covers every study period that some factor names, in
    study-period order. If there is none, it refuses `MODE_CHOICE_NO_PERIOD` at `periodFactors`.
  - A purpose without a factor row for a period adds the advisory `MODE_PERIOD_FACTOR_MISSING`
    (at `periodFactors`) and contributes nothing.
  - Deleting a study period that a factor names is refused (`UNKNOWN_PERIOD` at
    `periodFactors[i].periodId`).
- **Costs:**
  - The skim per `modeId` is computed once per run.
  - An alternative with only a `costMatrix` starts from all-∞, and its matrix's non-null cells
    make it available.
  - The distribution run's skim (`--skim-mode`, `--cost-matrix`) is unchanged.
- **Outputs:**
  - **JSON:** per purpose, each alternative's costs (null where unavailable), P-A persons and
    total. Per period, each alternative's OD persons and vehicles (null without occupancy) and each
    mode's vehicles and total.
  - **CSV:** the columns `period,alternative,originId,destinationId,persons,vehicles`, with a
    totals row per period and alternative.
  - Both carry the marker and the unit conditional.
- **Test data:**
  - `tests/data/trip-distribution/mode-choice/hand.json`: car with occupancy 1.25, and walk by
    an owned matrix. Its values are illustrations;
  - `modes/car.json`;
  - `mode-choice.traffic.json`: the generation fixture plus one study period, one factor row and
    the `walk-times` matrix, at schema 69.
- **Rows:**
  - `tests/mode_choice_tests.cpp` (group `modechoice`) covers MC1–MC9, with MC6 and MC7 in one
    test;
  - `cli-mode-choice`, `-alone` and `-unknown` cover MC10;
  - TD1–TD20 pass unchanged after the skim was moved.
- **Mutations:** each of the following failed at least one row: no max shift, the return share
  without the transpose, occupancy multiplied, and unavailable alternatives kept.

### E2 as implemented (D479)

- **Schema 70:**
  - root `modeChoiceSets`, the owned sets, with the citation optional;
  - `odMatrices[i].generatedBy`, whose only value is `"mode-choice"` (`OD_GENERATED_INVALID`
    otherwise).
  - Older versions refuse each at its path. A file without them keeps its bytes.
- **Lookup:** owned sets are searched before the catalog. An id clash is `DUPLICATE_ID` at
  `modeChoiceSets[i].id`.
- **Commands** (`src/commands/mode_choice_commands.*`):
  - `setModeChoiceSets` and `setPeriodFactors`;
  - `generatedOdMatrices(run)`, which gives one matrix per period and mode with id
    `mc-<period>-<mode>`;
  - `writeGeneratedOdMatrices`, which removes only generated matrices and appends the new ones.
    It calls `ownModesWhenNamed`, as the OD dialog does, so a project owning its compositions
    also owns its modes.
  - Writing the same result again makes no History step.
- **Editor:**
  - the *Mode choice* Results tab (`src/shell/mode_choice_panel.*`), the eleventh;
  - *Mode choice sets…*, through the shared owned-list dialog;
  - *Period factors…* (`src/shell/period_factors_dialog.*`), where a share outside [0, 1], or a
    row whose shares sum above 1, is refused in place;
  - *Write OD matrices*, enabled only for a fresh result with periods.
- **Assignment:**
  - On a network that runs one way only, a return share (A→P) gives the return cells no path in
    Macro (`MACRO_NO_PATH`). That is the data's consequence, not M10.4's.
  - MC12 therefore assigns the fixture with A→P = 0.
- **Rows:**
  - `tests/mode_choice_owned_tests.cpp` (group `modechoiceowned`) covers MC11 and MC12, including
    Run Macro on the written matrix;
  - `mode-choice-ui` covers MC13;
  - `workspace-ui` now counts 11 Results tabs;
  - MC14: the shipped projects keep their bytes (TG1, TD14).
- **Mutations:** each of the following failed a row: appending without removing, removing
  authored matrices, and not writing `generatedBy`.
- **Also fixed:** D476's note key had reused `editorDistributionOrigin`. That shadowed the Origin
  column label of the Trip distribution page. The note key is now
  `editorDistributionOriginConstrained`, and `project.catalog_and_translation_keys` refuses a
  repeated locale key.
