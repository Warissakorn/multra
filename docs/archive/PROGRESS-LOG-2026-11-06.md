# PROGRESS archive — 2026-11-06

Whole entries moved from the live PROGRESS log.

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
