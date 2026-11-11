# PROGRESS archive — 2026-11-06

Whole entries moved from the live PROGRESS log.

## 2026-11-08 — A gap under the toolbar row (D491)

- **Owner:** the Run (F5) toolbar should stand a little apart from the panels, like the panels do.
- **D491:** the toolbar row above the panels ends in the dock separator's 6 px midlight band, as
  a bottom contents margin set by `layoutToolbars`; stacked, only the lower row has it.
- **Tried:** a QSS bottom border/padding alone (Qt's toolbar layout ignores both) and a minimum
  height (the controls centred into the band).
- **Tests:** `compact-ui` checks the band's row, size and that no panel starts above it;
  workspace-ui's 32 px applies to the contents. Desktop ctest 310/310, `check` pass (Linux).
- **CI fix (windows-desktop):** `ui-audit-layout` (1×, 1.5×, 2×) failed in Thai at 1366: queue
  approach names under 140 px, after D490's row header took ~21 px. Side-by-side Results now split
  4:3 (was 3:2); on Linux Thai 176 → 215 px, English 235 → 274, movement viewport 778 → 739.

## 2026-11-08 — Compactness pass after D490 (D490a)

- **Owner:** asked for a compactness review in chat, then "แก้ตามแนะนำ" (all of A–D).
- **Fixed:** whole English column headers; the Objects group picker no longer covers the first tab
  after a language change; the Tables layout reaches its target height (Qt's `resizeDocks` fell
  short) and aims at 10.5 rows within the existing preview floor; Results notes fold to one line;
  the stacked queue table is only as tall as its rows; no grey header band below the last row.
- **Measured (Linux offscreen, Compact):** Objects rows 7.7 → 9.4 at 1360×860, 6.9 → 8.4 at
  1024×768, 8.6 → 10.5 at 1920×1080; Thai 1024 Results movements 9 → 12 of 12.
- **Reverted:** a 300/220 px preview floor, which broke workspace-ui and object-groups-ui's
  owner-set 350/250 px preview.
- **Tests:** new `compact-ui`; desktop ctest 310/310 and `check` pass. Windows look is the owner's.

## 2026-11-07 — Tables read like Excel (D490)

- **Owner:** "ต้องการให้ตารางในโปรแกรมเป็นเหมือนตาราง excel"; chose grid look, Copy to Excel and
  cell-by-cell selection (header sorting not chosen).
- **D490:** `table_grid.*` joins the table recipe applied to every table view on polish: grid lines,
  no alternating rows, numbered row header with a select-all corner, framed current cell, cell
  selection where several selections are allowed, Ctrl+C as Excel tab-separated text (headers on
  a block, quoting, named rows, cell editors' text), Ctrl+A kept by a focused table. One-object
  lists keep row selection, because `selectRow()` does nothing for single selection in cell mode.
- **Readers fixed:** the object tables' canvas sync read column-0 cells only, and Merge Nodes,
  Street-segment selection, Problems and the Macro run dialog read `selectedRows()`; all use
  `selectedTableRows` now, so a cell block anywhere in a row selects it.
- **Canvas mirror:** a cell block selects its rows' objects, and the canvas then mirrored whole
  rows back, erasing the block; the mirror now leaves a table whose selected rows already match.
- **Tests:** new `table-grid-ui` (in `editor_ux_tests.cmake`; CMakeLists is at 500 lines);
  `table-presentation-ui`'s recipe check updated. Desktop ctest 309/309 and `check` pass. Linux only:
  the Windows look is the owner's check (NEXT).

## 2026-11-07 — CI: the Linux ccache holds a whole Debug build (D489)

- **Measured (ccache 4.9.1, desktop preset, CI's settings):** a clean build fills 1.1 GB. Capped at
  CI's 500M, a rebuild of the same tree hit 57/736 (7.7 %) and took 14m05; with the whole cache it
  hit 736/736 in 15 s. CI's Windows cache was saved at 487 MB, at its cap too.
- **D489:** `CCACHE_MAXSIZE` for the Linux matrix 500M → 1500M. Windows stays at 500M: its MSVC
  Release cache could not be measured here.
- **Not yet verified on CI:** the next run's `ccache -s` shows whether hits rise; headless,
  sanitizer and release sizes were not measured.

## 2026-11-07 — Linux desktop preset links the multra libraries shared (D488)

- **Cause:** every UI test executable linked `libmultra_shell.a` and carried its own copy of its
  debug info: a Debug tree passed 30 GB (73 executables, 23.6 GB; 92 % DWARF) and ran out of disk
  here, and one shell `.cpp` edit relinked 118 executables.
- **D488:** `MULTRA_SHARED_LIBS` (default OFF) makes every `multra_*` library shared, with
  `CMAKE_LINK_DEPENDS_NO_SHARED`; only the `desktop` preset turns it on, on Linux. headless,
  sanitizer, release, packaging and Windows stay static. `multra_shell` now links
  `multra_vehicle_pose` PUBLIC, since `canvas.hpp` holds `RearAxlePath` (static linking hid that).
- **Measured (Linux, 4 cores):** tree >30 GB → 7.3 GB, `bin/` 23.6 → 1.2 GB; touching
  `src/shell/node_analysis_panel.cpp` 2m00 → 8.3 s (median of 3). Clean build ≈14 min, as before.
- **Verified:** desktop ctest 308/308, the three checks; the release preset still configures static.

## 2026-11-07 — Run view: no dock relayout per Step (D487)

- **Owner:** ran /optimize-program on everything and chose the recommended plan.
- **Cause (callgrind):** 41.6 % of a Run view's instructions were `ReflowLabel::sizeHint`/
  `minimumSizeHint` from a whole-dock relayout every Step. gdb on the LayoutRequests found two
  triggers: `refreshRun` set the Run action's text and a new icon every Step, and the D463
  Properties summary called `updateGeometry()` on every LayoutRequest of its content.
- **D487:** both now act only on a change: the Run action when Run/Pause (or the translated text)
  differs, and the summary when its `sizeHint()` moved.
- **Measured (Linux, Release, offscreen):** `multra-run-view-benchmark` M2.6 6000 2000, back to back,
  median of 3: 32.9 → 19.2 s (−42 %). Either fix alone: summary −14 %, Run action within noise.
- **Verified:** desktop ctest 308/308 and the three checks, Linux only. Windows look of the
  Properties summary and the Run/Pause button not checked.
- **Guard (same day):** `run-presentation-ui` counts LayoutRequests on the window and its docks over
  200 Steps of `crossing.json` and fails above 20; measured 0 with D487 and 800 with it reverted.

## 2026-11-06 — Docs: the decision record split every 200 IDs (D486)

- **Owner:** ran /optimize-program on docs and AI context, and chose the split by 200 IDs, with the
  decision index split by the same ranges.
- **D486:** `RECORD.md` (500 lines, ~134k tokens) became `RECORD-D001-D199.md` and
  `RECORD-D200-D399.md` (frozen) and `RECORD.md` (D400 onward, 97 lines). The rows moved unchanged.
  527 anchored links in 24 files now name the file that holds the row. AGENTS says how to cut the
  next range.
- **Verified:** `multra-check-docs` and `multra-check-file-sizes`; no code changed.

## 2026-11-06 — Procedure sequence P2: Procedures dialog and Procedure page (D485)

- **D485 ([PROCEDURE_SEQUENCE](../reference/PROCEDURE_SEQUENCE.md) §9):**
  - *Procedures…* edits the list and its steps, refused in place, as one History step;
  - the twelfth Results tab runs a procedure on a worker with Cancel and shows each step's status
    and summary, with stale marking and a marked CSV.
- **Rows:** `procedure-ui` (PS10); `workspace-ui` counts 12 tabs.
- **Note:** `docs/decisions/RECORD.md` reached 500 lines with D485; splitting it is the owner's call
  (NEXT).
- **Verified:** Linux only (desktop and headless). Windows not checked (OWNER_SITTING 42).

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
