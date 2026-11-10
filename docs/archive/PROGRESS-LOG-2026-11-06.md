# PROGRESS archive — 2026-11-06

Whole entries moved from the live PROGRESS log.

## 2026-11-06 — Procedure sequence P1: engine, schema 73, CLI (D484)

- **Owner:** approved the plan after D483.
- **D484 ([PROCEDURE_SEQUENCE](../reference/PROCEDURE_SEQUENCE.md) §8):**
  - `procedures` at schema 73, and `runProcedure` (nearest earlier active input, an in-memory
    assignment demand, a refusal stops the rest, Cancel);
  - `procedureJson` and `--procedure`;
  - `generatedOdMatrices` moved into the project layer.
- **Rows:** `procedure` (PS1–PS8) and `cli-procedure*` (PS9). Found by the rows: a `{}` initialiser
  had set every step to *done*; fixed. Four mutations each failed a row.
- **Verified:** Linux only (desktop and headless). Windows not checked.

## 2026-11-06 — Procedure sequence contract (D483)

- **Owner:** "ลำดับขั้นตอนการรัน"; answered §6 in the session: a general step list (1b, against
  the recommended fixed chain), in-memory matrices (2a), stored procedures (3a), feedback later
  (4a).
- **D483 ([PROCEDURE_SEQUENCE](../reference/PROCEDURE_SEQUENCE.md)):** steps `tripGeneration`,
  `tripDistribution`, `modeChoice` and `assignment`, each taking the nearest earlier active input;
  a refused step stops the later ones; schema 73; `--procedure`; rows PS1–PS11. Docs only.

## 2026-11-06 — M10.3 D4: destination-constrained purposes (D482)

- **Owner:** "งานถัดไปตามแนะนำ"; the recommended options (all a) stand.
- **D482 ([TRIP_DISTRIBUTION](../reference/TRIP_DISTRIBUTION.md) §16):**
  - `constraint: "destination"` (A_i = 1, p. 183, re-read from the page image), computed as the
    origin path on the transposed problem;
  - T_ii = s · A*_i, and owned sets using it at schema 72;
  - `productionsModelled`, the CSV suffix and the page note.
- **Rows:** `tripdistributiondestination` (TD21–TD25: Example 5.2 transposed, weights, intrazonal,
  refusals, schema 72), `cli-trip-distribution-destination` and `trip-distribution-ui` (TD26).
  TD19's bad value is now `"both"`. Four mutations each failed a row.
- **Verified:** Linux only (desktop and headless). Windows not checked (OWNER_SITTING 42).

## 2026-11-06 — M10.4 N: nested logit, contract and N1 (D480, D481)

- **Owner:** "ทำงานถัดไป" (nested logit, picked at the compaction). Choices answered in the
  session: two levels, φ in (0, 1] refused outside, and a nest editor in the dialog (3b, against
  the recommended Import only).
- **D480 ([NESTED_LOGIT](../reference/NESTED_LOGIT.md)):** N0 read Eq. 7.16–7.23 (PDF 260–261) and
  §7.4.4.3 (PDF 267) from the page images; the contract.
- **D481 (NESTED_LOGIT §7):**
  - `nestedLogitShares` (Eq. 7.23 with β = 1, shifted logsums), `splitModes(…, ModeNests)`;
  - per-purpose `nests` in mode-choice sets, owned nested sets at schema 71;
  - the run and the JSON echo;
  - *Nests…* in the Mode choice sets dialog.
- **Rows:** `nestedlogit` (NL1–NL7: Example 6.3's ten rows, the MNL collapse, availability,
  numerics, the codec, schema 71, the run), `cli-mode-choice-nested`, `nested-logit-ui` (NL8);
  MC1–MC14 unchanged. Four mutations each failed a row.
- **Verified:** Linux only (desktop and headless). An offscreen grab of the dialog was checked for
  layout. Windows not checked (OWNER_SITTING 42).

## 2026-11-06 — M10.4 E2: owned sets, Mode choice page, Write OD matrices (D479)

- **Owner:** "ทำงานถัดไป".
- **D479 ([MODE_CHOICE](../reference/MODE_CHOICE.md) §13):**
  - owned `modeChoiceSets` and `odMatrices[i].generatedBy` at schema 70;
  - the *Mode choice* Results page, with *Mode choice sets…* and *Period factors…*;
  - *Write OD matrices*, one History step replacing only generated matrices, which Run Macro
    assigns.
  - **Fixed:** D476's note key shadowed the Trip distribution page's Origin column. Locale keys
    are now checked for repeats.
- **Rows:** `modechoiceowned` (MC11–MC12) and `mode-choice-ui` (MC13); `workspace-ui` counts 11
  Results tabs. Three mutations each failed a row.
- **Verified:** Linux only (desktop and headless). An offscreen grab of the page was checked for
  layout. Windows not checked (OWNER_SITTING 42).

## 2026-11-06 — M10.4 E1: mode choice engine, period factors, run and CLI (D478)

- **Owner:** "เอาตามแนะนำ" on MODE_CHOICE §10 (1a–8a).
- **D478 ([MODE_CHOICE](../reference/MODE_CHOICE.md) §13):**
  - the MNL split (exp(V − max V), unavailable alternatives excluded);
  - P-A to OD by period factors (root `periodFactors`, schema 69);
  - vehicles by occupancy, summed per mode;
  - marked JSON/CSV, and `--mode-choice`/`--period`.
  - The zone skim moved to `zone_skim`, with distribution unchanged.
  - The four-step CLI runs moved to `tools/four_step_cli`.
- **Rows:** `modechoice` (MC1–MC9) and `cli-mode-choice*` (MC10). Four mutations each failed a row.
- **Verified:** Linux only (desktop and headless); Windows not checked.

## 2026-11-06 — M10.4 contract: mode choice and OD conversion (D477)

- **Owner:** "ทำงานถัดไปตามแนะนำ".
- **M0:** read Ortúzar §5.4.4 and Ch. 6–7 from the page images
  ([evidence](../evidence/ortuzar-ch6-7-mode-choice.md)):
  - Examples 6.2 and 6.3 reproduce, including every row of the λ₁ table;
  - MNL (7.9) and the NL condition φ ≤ 1;
  - no book value for occupancy or period shares.
- **D477 ([MODE_CHOICE](../reference/MODE_CHOICE.md)), docs only:** eight owner choices, slices
  E1–E2, rows MC1–MC14.

## 2026-11-06 — M10.3 D3: origin-constrained purposes (D476)

- **Owner:** "เอาตามแนะนำ" on TRIP_DISTRIBUTION §15.2 (1a–5a).
- **D476 ([TRIP_DISTRIBUTION](../reference/TRIP_DISTRIBUTION.md) §15.4):**
  - a per-purpose `constraint: "origin"` in deterrence sets (schema 68 when owned);
  - one row normalisation of A_j f_ij (Eq. 5.16), with T_ii = s · P_i;
  - JSON, CSV and the page name the constraint and the modelled attractions;
  - doubly outputs keep their bytes.
- **Rows:** `tripdistributionorigin` (TD15–TD19; TD15 reproduces Example 5.2's Table 5.5),
  `cli-trip-distribution-origin` and `trip-distribution-ui` (TD20).
- **Verified:** Linux only (desktop and headless); Windows not checked.

## 2026-11-06 — M10.3 D3 contract: singly constrained purposes (D475)

- **Owner:** asked for an explanation of singly constrained models, then "ทำงานต่อไปตามแนะนำ".
- **D475 ([TRIP_DISTRIBUTION](../reference/TRIP_DISTRIBUTION.md) §15), docs only:**
  - Eq. (5.16) (p. 183) read from the page image;
  - Example 5.2 (Tables 5.4–5.5) replayed as the fixture, with a misprinted column sum (701.2 for
    710.2) ([evidence](../evidence/ortuzar-ch5-trip-distribution.md) §6);
  - raw and balanced attractions shown to give the same trips;
  - five owner choices; rows TD15–TD20.
- **Not done:** no code. Slice D3 waits for the owner's answers.

## 2026-11-06 — M10.3 D2: owned deterrence sets and cost matrices, Trip distribution page (D474)

- **Owner:** "เริ่มเลย" on D2.
- **D474 ([TRIP_DISTRIBUTION](../reference/TRIP_DISTRIBUTION.md) §14):**
  - `deterrenceSets` and `costMatrices` at schema 67, with commands;
  - a run-chosen matrix overrides skim cells and is the only source of a diagonal;
  - G2's trip-rate dialog made a shared owned-list dialog;
  - the self-owned *Trip distribution* page;
  - `--cost-matrix`.
- **Rows:**
  - `tripdistributionowned` (TD12 and the override rows) and `trip-distribution-ui` (TD12 save,
    TD13, the dialogs).
  - `workspace-ui` updated deliberately for the tenth Results tab.
- **Verified:** Linux only (desktop and headless; counts in the session report). An offscreen grab
  of the page and the dialog was checked for layout. Windows not checked (OWNER_SITTING 42).

## 2026-11-06 — M10.3 D1: gravity distribution engine, skim, run, outputs and CLI (D473)

- **Owner:** "ตามแนะนำ" on TRIP_DISTRIBUTION §11 (1a–9a).
- **D473 ([TRIP_DISTRIBUTION](../reference/TRIP_DISTRIBUTION.md) §14):**
  - `src/macro/trip_distribution` (row-first Furness, exponential gravity seeded by T/Σf,
    intrazonal share, the stopping measure);
  - `freeFlowSkim`, with zone costs as the connector-share-weighted mean;
  - deterrence sets as data;
  - `runTripDistribution`, JSON/CSV and `--trip-distribution`.
- **Rows:**
  - TD1–TD10 (group `tripdistribution`) and four CLI tests (TD11).
  - Examples 5.3 and 5.4 reproduce the printed tables.
  - Four engine mutations each failed a row.
- **Found while building:** the Macro graph's centroids are per zone connector, not per zone, so
  the skim aggregates connector pairs by share (§5).

## 2026-11-06 — M10.3 contract after reading Ortúzar Ch. 5 (D472)

- **Owner:** "สำหรับงานถัดไปคือ M10.3", then "ทำงานถัดไป".
- **D0 ([evidence](../evidence/ortuzar-ch5-trip-distribution.md)):** the page images of Tables
  5.6–5.10 and the band table were read. A scratch replay reproduces Table 5.7 and Table 5.10
  (cells, a_i, b_j, bands) with three row-first passes. Table 5.10's row-2 sum is misprinted. The
  book gives no Furness tolerance.
- **D472 ([TRIP_DISTRIBUTION](../reference/TRIP_DISTRIBUTION.md)):** gravity with exponential deterrence
  over M10.2's ends, row-first Furness, a free-flow skim, deterrence sets as data (none shipped),
  rows TD1–TD14, slices D0–D2, and nine owner choices. Docs only.

## 2026-11-06 — M10.2 G2: owned rate sets, zone attributes, Trip generation page (D471)

- **Owner:** "เริ่มเลย" on G2; chose attributes in the Edit Zone dialog and a rate-set list with
  Import/Export JSON (over table columns and a form editor).
- **D471 ([TRIP_GENERATION](../reference/TRIP_GENERATION.md) §6.2, §6.3, §14):** `tripRateSets` at
  schema 66, looked up before the catalog (a same-id clash is refused); `setTripRateSets`; the
  Edit Zone attribute table; *Trip-rate sets…*; a self-owned *Trip generation* Results page.
- **Rows:** `triprateowned` (TG11 codec, command, lookup) and `trip-generation-ui` (TG11 save,
  TG13, TG14, the dialog). `utdf` and `workspace-ui` updated for schema 66 and the ninth Results tab.
- **Environment:** installed Qt 6.4.2, PROJ and xkbcommon; a Debug build with debug info filled
  the disk (23 GB), so both presets were built with `-g0`.
- **Verified:** `ctest --preset desktop` 283/283 and `check` pass; `ctest --preset headless`
  181/181. Linux offscreen only; Windows not checked (OWNER_SITTING 42).

## 2026-11-06 — M10.2 G1: trip generation engine, run, outputs and CLI (D470)

- **Owner:** "เริ่มเลย" on G1.
- **D470 ([TRIP_GENERATION](../reference/TRIP_GENERATION.md) §14):** `src/macro/trip_generation` (pure),
  the rate-set codec and catalog, `runTripGeneration` by stage, JSON/CSV and `--trip-generation`.
  Example 4.1 reproduces 2125 and 3000 trips/day; Example 4.4's zonal form gives 356.9.
- **Environment:** installed `nlohmann-json3-dev`; no Qt here, so the headless preset only.
- **Rows:** TG1–TG10, TG4b, TG12 and three CLI tests. All passed on first run, so four engine
  mutations were run; each failed at least one row.
- **Verified:** `ctest --preset headless` 180/180 (176 before + 4 new), `check` passes on Linux.
  Desktop and Windows not built.

## 2026-11-06 — M10.2 owner answers and G0, the book read (D469)

- **Owner:** "เอาตามแนะนำ 1a–8a อนุญาตแนบหนังสือ เริ่ม G0".
- **G0 ([evidence](../evidence/ortuzar-ch4-trip-generation.md)):** Ortúzar Ch. 4 attached and read
  from the page images. §3's two forms match Eq. (4.16) and §4.2.5's aggregation; §4.2.6's
  Eq. (4.14) is the same rule as choice 3a.
- **Correction:** Example 4.5 (p. 161), booked as the fixture, prints only sample counts. The
  fixture is Example 4.1 (2125 and 3000 trips/day), with Example 4.4's zonal form as TG4b;
  BOOKS §3.3 corrected.
- **Contract:** answers recorded in §11; household-based models enter on zone totals with no
  second regression form (§3).
- **Verified:** `check_docs` and `check_file_sizes` pass (compiled with g++; the CMake preset still
  lacks nlohmann/json here). Docs only.

## 2026-11-06 — M10.2 trip generation contract (D468)

- **Owner:** asked what could come next, then "เริ่มเขียนสัญญา M10.2 trip generation".
- **D468 ([TRIP_GENERATION](../reference/TRIP_GENERATION.md)):** trip-rate sets as data, one method
  per purpose side (cross-classification or zonal regression), attractions balanced to productions,
  the set's unit carried with no period or mode conversion, outputs, CLI and editor surface, seams
  to M10.3/M10.4, slices G0–G2 and rows TG1–TG14. Docs only; no code.
- **Measured:** `Zone::attributes` is read and written by the codec only; no editor surface or
  command edits it, so choice 7a includes attribute editing.
- **Not read:** attaching the owner's Ortúzar copy was refused by the session's permission check,
  so Example 4.5 (p. 161) is booked as slice G0, and §3–§4 rest on the standard formulation until
  that reading cites Ch. 4.
- **Open:** eight owner choices, OWNER_SITTING B42.
- **Verified:** `check_docs` and `check_file_sizes` pass, compiled directly with g++; the headless
  preset did not configure here (nlohmann/json not installed), so no CMake build or ctest ran. Docs only.
