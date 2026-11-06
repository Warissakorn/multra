# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-06 — M10.3 contract after reading Ortúzar Ch. 5 (D472)

- **Owner:** "สำหรับงานถัดไปคือ M10.3", then "ทำงานถัดไป".
- **D0 ([evidence](evidence/ortuzar-ch5-trip-distribution.md)):** the page images of Tables
  5.6–5.10 and the band table were read. A scratch replay reproduces Table 5.7 and Table 5.10
  (cells, a_i, b_j, bands) with three row-first passes. Table 5.10's row-2 sum is misprinted. The
  book gives no Furness tolerance.
- **D472 ([TRIP_DISTRIBUTION](reference/TRIP_DISTRIBUTION.md)):** gravity with exponential deterrence
  over M10.2's ends, row-first Furness, a free-flow skim, deterrence sets as data (none shipped),
  rows TD1–TD14, slices D0–D2, and nine owner choices. Docs only.

## 2026-11-06 — M10.2 G2: owned rate sets, zone attributes, Trip generation page (D471)

- **Owner:** "เริ่มเลย" on G2; chose attributes in the Edit Zone dialog and a rate-set list with
  Import/Export JSON (over table columns and a form editor).
- **D471 ([TRIP_GENERATION](reference/TRIP_GENERATION.md) §6.2, §6.3, §14):** `tripRateSets` at
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
- **D470 ([TRIP_GENERATION](reference/TRIP_GENERATION.md) §14):** `src/macro/trip_generation` (pure),
  the rate-set codec and catalog, `runTripGeneration` by stage, JSON/CSV and `--trip-generation`.
  Example 4.1 reproduces 2125 and 3000 trips/day; Example 4.4's zonal form gives 356.9.
- **Environment:** installed `nlohmann-json3-dev`; no Qt here, so the headless preset only.
- **Rows:** TG1–TG10, TG4b, TG12 and three CLI tests. All passed on first run, so four engine
  mutations were run; each failed at least one row.
- **Verified:** `ctest --preset headless` 180/180 (176 before + 4 new), `check` passes on Linux.
  Desktop and Windows not built.

## 2026-11-06 — M10.2 owner answers and G0, the book read (D469)

- **Owner:** "เอาตามแนะนำ 1a–8a อนุญาตแนบหนังสือ เริ่ม G0".
- **G0 ([evidence](evidence/ortuzar-ch4-trip-generation.md)):** Ortúzar Ch. 4 attached and read
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
- **D468 ([TRIP_GENERATION](reference/TRIP_GENERATION.md)):** trip-rate sets as data, one method
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

## 2026-11-05 — Review fixes to D464–D466 (D467)

- **Owner:** asked for a fix for each review finding, then "ข้อ 1 ไม่แก้ … ที่เหลือเอาตามแนะนำ".
- **Fixed:** the shadow kept its size on high-DPI screens (it was half size at ratio 2); it no
  longer copies the body path per paint; one palette parser; the head frame reads `ahead`.
- **New checks:** painted casings over a road; every drawn pole off the road, in the shipped
  signalised projects and at a Connector mouth; type palette refusals.
- **Kept:** dark casings for a user's dark colours (the shadow shows them); a head box past its
  stop line still takes clicks there. The run-overlay policy waits for a third overlay.
- **Verified:** desktop build, `ctest --preset desktop` 277/277 and `check` pass on Linux; Windows not
  checked (the high-DPI fix is tested on an offscreen ratio-2 image, not a real 200 % screen).

## 2026-11-05 — Signal heads past the stop line; dark vehicle casings (D466)

- **Owner:** asked whether to move the pole past the stop line, use dark casings only and hang
  the boxes on the arm, then "เอาตามแนะนำ เริ่ม D466 เลย".
- **Heads:** each box lies past its stop line (0.35 m gap, 0.9 m box) and hangs by its back edge
  from one straight arm; one pole per stop line, and no arm crosses a box. Hits move with the box.
- **Casings:** run vehicles take the road edge's dark ink; wireframe keeps D199's contrast rule.
- **Rows:** road-crossbar-ui and run-presentation-ui, each failed first.
- **Verified:** desktop build, `ctest --preset desktop` 277/277 and `check` pass on Linux; Windows not
  checked. Images: `docs/images/canvas-heads-d466/` (four-leg mid-run, 6 and 14 px/m, colour and grey).

## 2026-11-05 — Canvas style slice C: run vehicles (D465)

- **Owner:** reviewed slice C, then "เอาตามแนะนำ เริ่ม Slice C เลย".
- **Palettes:** car, pickup and van ship realistic body colours (white, silvers, black, red,
  blue); bus, trucks and motorcycle keep their type colour. A user's palette still wins; a type on
  its shipped palette is not saved, and Reset returns to it. Every shipped vehicle colour keeps
  contrast ≥ 1.5 against every road grey.
- **Shadow:** a 2 px down-right translucent copy of the body, from 14 px, no blur.
- **Run overlays:** Node outlines and unpinned rate labels no longer sit on the vehicles; a
  selected road's rate shows as its box.
- **Rows:** vehicle-colors-ui, vehicle-appearance-ui and run-presentation-ui, each failed first.
- **Also:** `ui-audit-layout` now expects no unpinned rate labels during its one run step.
- **Verified:** desktop build, `ctest --preset desktop` 277/277 and `check` pass on Linux; Windows not
  checked. Images: `docs/images/canvas-run-d465/` (four-leg mid-run, 6 and 14 px/m, colour and grey).

## 2026-11-05 — Exported images keep rates and heads; the LOD test measures again (D464)

- **Owner:** asked to review the decisions D460 replaced, then "เริ่มเลย" on the three fixes.
- **PNG export:** every input rate shows as a grey-framed box, and every signal head keeps its
  box (4 px at least, no pole when grown). D462's "no marker" relied on hover, which a picture
  lacks. The live canvas is unchanged.
- **canvas-lod-ui:** after D460 the crossing fixture drew no lane decoration at 3 px, so its
  threshold checks compared 0 with 0. It now measures a selected Connector's mouth edges and
  requires them.
- **Records:** D423, D432, D434 and D435 now say what D460 changed in them.
- **Rows:** network-image-ui, road-crossbar-ui and canvas-lod-ui each failed before the fix.
- **Verified:** desktop build, `ctest --preset desktop` 277/277 and `check` pass on Linux; Windows not
  checked. Images: `docs/images/export-d464/` (whole-network PNG before and after).

## 2026-11-05 — UI fixes, round 2 (D463)

- **Owner:** "เริ่ม UI รอบสองเลย".
- **Properties width:** the Link page needed 393 px in a 280 px panel because of the Link type
  combobox. The box now shrinks below its longest item, and the widest button label is shorter
  ("Midpoint on longest segment"); an eliding button made the form layout loop. No sideways
  scroll at any density.
- **Clipped summary:** at 800 px it ends on a whole row instead of through a field.
- **Labels:** "Lane widths (m)", "Boundary markings", "New lane width (m)" (Thai likewise); the
  format is the placeholder and tooltip.
- **Scope banner:** one line, "Not yet validated · prototype model — hover for its limits"; the
  full limits stay in its tooltip.
- **Results:** a movement or queue cell with no figure reads "—"; a Node-table cell that does not
  apply (a turn's queue) stays blank.
- **Verified:** desktop build, `ctest --preset desktop` 277/277 and `check` pass on Linux after fixing what the first full run caught (a layout loop from eliding buttons, a Node-table blank); Windows not checked. Images: `docs/images/ui-round2-d463/`.
