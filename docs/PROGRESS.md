# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

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

## 2026-11-05 — Owner choices after D461 (D462)

- **Owner:** "ช่วยแนะนำต้องตัดสินใจ", then took every recommendation.
- **Hidden rates:** no extra marker. The blue input bar stays, and hovering or selecting the origin
  road shows the rate as its box.
- **Objects dock:** titled by its current group. A one-page group (Results, Problems) shows no tab
  strip; its picker moves to its own row above the page, since Qt left it floating over the page
  with the strip hidden. It returns to the strip's corner for larger groups.
- **Movement names:** not translated; they are the project's Link names.
- **Verified:** desktop build, `ctest --preset desktop` 277/277 and `check` pass on Linux; Windows not checked.

## 2026-11-05 — Canvas display fixes, round 1 (D461)

- **Owner:** "เริ่มเลย" on round 1 of the display-error list; the median spikes in the junction
  stay (owner choice, Q5).
- **Fit:** reserves label room only for labels that show at Fit. On-lane rates hide there (D460),
  so the four-leg roads now fill 94 % of the view height instead of 68 %.
- **Signal heads zoomed out:** under 4 px a head is its stop line alone, with no box, arm or
  pole; the pole has no pixel floor. A run keeps the state colour and cue on the line.
- **Road edge:** shows while the whole road is 2 px wide, no longer only from 3 px per lane.
- **Heads on a Connector:** the pole stands beside the attached Link's edge at the mouth; a head
  away from any mouth has none.
- **Verified:** desktop build, `ctest --preset desktop` 277/277 and `check` pass on Linux; Windows not checked. The queue-counter test now zooms to its own targets (it scaled Fit by 3). Images: `docs/images/canvas-fixes-d461/`.

## 2026-11-05 — Canvas review fixes (D460)

- **Owner review** of the canvas: four items, then three more rounds (single-colour square heads
  on a pole, thin dark road edges, bare Node outlines and tags, greyer roads, no resting lane guides).
- **Node:** a thin outline with no fill, drawn above the roads. The polygon is no longer filled as
  junction asphalt. Highlighted Nodes stay 2 px accent and study-area Nodes green.
- **Road colour:** neutral greys instead of blue-greys (`data/display-types/`).
- **Node name:** shown only while the Node is selected, as a tight framed tag centred on it, no halo.
- **Connector lane guides:** the faint resting lane-centre lines (D423 Q7) are gone; they tangled
  over every junction. A Connector's lanes still show on hover, selection or the Connect tool.
- **Signal head:** a 0.90 m square box of one colour with sharp corners, 0.35 m upstream of the
  stop line: dark while editing, the state's colour in a run, with no lamps. The run stop line
  keeps D199's shape cue. A mast arm meets the box's kerb-side edge from a pole just outside the
  road on the driving side. It zooms with the road.
- **Road edge:** a thin dark line replaces the thick light kerb, on Links and Connectors. A Link's
  plain solid outer-edge marking is no longer painted white over it; dashed and double edges still
  are.
- **Clear button:** measured 24 px tall at y 2 inside the combo's 22 px edit, so the X sat 3 px low.
  The cause was the app-wide `QToolButton` field-height rule, which also styles a line edit's clear
  button. That button is now exempt.
- **Input rates:**
  - A rate lies in the middle of one of its served lanes, along the road and upright.
  - The rates on one Link fill its lanes side by side, then line up along each lane.
  - While the text is taller than the lane it hides, so there is nothing at Fit on the shipped
    projects. A rate whose origin road is selected or hovered falls back to its box.
  - Boxes are now as tall as the text's ink.
  - This replaces D432's "all visible after Fit" and D426's columns for unpinned rates.
- **Verified:** desktop build, `ctest --preset desktop` 277/277 and `check` pass on Linux; Windows not checked. Images: `docs/images/canvas-review-d460/`.


## 2026-11-04 — Two-entry weaves run (D459)

- **Owner:** "ทำให้วาด weave แบบสองทางเข้าได้", with the test as a fourth weaving-lab scene.
- **Cause:** a lane family kept a stub only beside a full chain on its first Link, so a weave drawn
  with two entries refused both crossing movements (BA179).
- **D459 (M3_8_CONTRACT §2):** a family may start a **joined chain** on a later Link's lane that no
  chain enters, kept only beside a stub that needs it.
  - It takes no input share.
  - Its route carries `leadIn`, which the free-flow time counts.
  - A change onto it also sees the segments feeding its first segment. Without this, a ramp car at
    the gore was clamped.
  - Downstream routing decisions (D93) never build one.
- **Weaving lab, scene 4:** the 300 m weave with a 2-lane mainline and a 1-lane ramp, each with a
  300 m approach, at the same volumes. Scenes 1–3 are unchanged.
- **Rows:** BA179 revised; BA206–BA208.
- **Results** ([evidence](evidence/weaving-two-entries.md)):
  - products and the lateral lab are byte-identical;
  - scene 4 serves 100 % at every factor, off and on, with no lock and no wait at a dead end;
    delay at ×1.0 is 20.0 s off and 26.8 s on;
  - the edge scenes in the full run move only by the shifted random draw;
  - one clamp at ×1.0 on, seed 48, in the 150 m weave (open).
- **Verified:** `ctest --preset headless` passes 176/176.
