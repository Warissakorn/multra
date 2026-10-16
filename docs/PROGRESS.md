# PROGRESS — TrafficSim

Newest entry at the top: what changed and why. What to do next is in [`NEXT.md`](NEXT.md);
decisions go once into the [decision record](decisions/RECORD.md). Keep about ten entries here;
move older ones whole to the top of the newest [`archive/`](archive/README.md) PROGRESS log.

---

## 2026-10-16 — M3.4 slice 1: lane-change duration (D206)

NEXT's item. `src/core/lanes.*`, `lateral_motion.cpp`, one line of `simulation.cpp`. With the lateral
switch on, an accepted lane change now sets `lateralTarget`; the body moves at `maxLateralSpeed`,
the route remaps at the crossing and the target clears on arrival. Rows BA48–BA53 failed first
([evidence](evidence/lane-change-duration.md)).

Switched-on product runs exposed two flaws in the start rules as D204 wrote them; both are fixed:
- the span-window rule counted the body's length, so a changer standing near its dead end never
  started (four-leg seed 42 jammed);
- a change under way was invisible to other starts, so two changers converged on one lane 5 cm
  apart. It now holds its target place for the whole change.

Still open, recorded in NEXT: a swap deadlock at shared dead ends that predates this slice
(lane-change-lab seed 44, switch off too); two mid-change clamps in ten lab seeds; and the
switched-on cost (+33 % lab, +17 % four-leg, Release), owed an engine round before any shipped
project carries lateral keys. Switched off: `check` 162/162, CLI pin byte-identical, benchmark
unchanged. Linux only.

## 2026-10-16 — Main's Windows-only UI reds (D205)

NEXT's first item. The windows-desktop job had failed four UI tests since PR #170, on Windows only. Both
causes came from the CI log ([evidence](evidence/windows-ui-red.md)):
- **`canvas-lod-ui`**: with a rotated view, `viewScale()` came out an ulp below exactly 3 px per lane, and
  the threshold comparison had no margin. `laneMarkingsReadable` now allows a relative 1e-9.
- **`properties-layout-ui`** (Thai only): Windows Qt cannot break Thai words, so the 123 px word in
  `editorMedianGap` widened the Link page's column by 3 px and no other page's. The column is now one
  `FormColumn` shared by every form, widened to such a word when it is under a quarter wider.

- **Round 2** (the branch's first Windows run): the threshold was confirmed, and a hidden form kept
  the old column when the width changed back with the appearance fold closed. Hidden forms are now laid
  out at once.

All three now reproduce on Linux. Each row failed before its fix and fails again with the fix reverted.
Checked on Linux offscreen: `check` 161/161, CLI pin byte-identical to main. Windows is confirmed only
by this branch's CI run.

## 2026-10-16 — M3.4 lane-change duration contract (D204, docs only)

The owner asked for recommendations on the contract's three open choices and took all three:
- a change starts only if it can finish before the next conflict area on both lanes (an extension of
  the existing "no change inside an area" rule), so no zone has to hold a straddler;
- the D97 slide stays for switched-off runs, which still change instantaneously;
- no new key: `maxLateralSpeed` sets the duration.
New [LANE_CHANGE_DURATION](reference/LANE_CHANGE_DURATION.md) with rows BA48–BA53; also found that a
change must cross before its span's `fromEnd`, since the crossing needs a covering span (LATERAL §7).
No code changed; `check` was run for the docs.

## 2026-10-16 — M3.7 slice 4: lateral display (D203)

NEXT's next item after slice 3b. `src/editor/canvas_run.cpp`, `src/core/types.hpp`, `lateral_motion.cpp`.
- **Drawing:** a run body sits at lane centre + `lateral`, the front shifted along the left normal of
  its heading. Left of travel is the engine's convention, so left- and right-hand networks need no
  case. `lateral = 0` draws exactly as before.
- **Found while reading:** a crossing made by lateral motion records a `LaneChangeTrace`, and the D97
  slide would have drawn the body about a lane away from where the engine keeps it. The entry is now
  flagged `lateral` (display only, no tick reads it) and such crossings are not slid.
- `types.hpp` stays at 500 lines (the field shares its line). The M3.7.0 contract's four slices are
  all in; the gate stays open (ROADMAP).

Checked on Linux offscreen: `ctest --preset desktop` 161/161, `check` green, CLI pin unchanged and the
Release CLI output byte-identical to main. No Windows or high-DPI run.

## 2026-10-16 — M3.7 slice 3b: zone occupancy by span (D202)

The owner chose the head of the engine queue. `src/core/conflicts.*`, `lateral.*`, `lateral_types.hpp`.
- **What a straddler's other-lane span does to a zone** (switch on only): inside a major area it
  blocks, on the chain's first segment it is anticipated by headway/gapTime (one `blockMajor` helper
  shared with route positions), inside a minor area it holds, and in phase 2 it sweeps a major area.
  `LateralIndex::zoneSides` gives each segment's zone sides once per run.
- **Not done, on purpose:** the straddler is not held at the other lane's zone. That needs the zone
  mapped into its own route's distances; no product writer sets targets until M3.4, so it waits for
  M3.4's evidence rather than being built blind (D202's failure condition names it).
- `zoneHold` code is unchanged; `summarizeZones` keeps a 4-argument overload that builds spans only
  with the switch on, so the ten lane-based test callers did not change. `simulation.cpp` stays at
  500 lines: the step passes its existing spans on the same lines.
- Rows: BA47 (6 tests, `lateralzone`), each with its forcing asserted and a mutation that fails it.

Checked on Linux headless: `ctest` 105/105, `check` green, CLI pin unchanged and the Release CLI
output byte-identical to main. Release timings in the [evidence](evidence/lateral-zones.md): switch
off inside noise; switched-on four-leg +7 % on a noisy container, below D164's 10 %. No desktop or
Windows run.

## 2026-10-16 — Owner sitting triage (D201, docs only)

The owner asked which sitting items truly needed a decision, then answered “ปิดตามข้อเสนอ”.
- **Closed as proposed:** B1 (keep `TrafficSim` until a release), B2 (D95/D101 off; route (i) only,
  in M3.4), B3, B5 (standstill cap booked in M3.2.8a.1), B6, B8, B12 (asked at multi-level S1), B13
  (`w74` preset stays out of the catalog; noted in M4.2).
- **B3 is a code/contract discrepancy, not a choice:** `automatic_conflicts.cpp` skips pairs whose
  drawing `level` differs, while M3_CONTRACT §1 says level is not separation. Queued as a fix slice
  in NEXT with a failure-first row; B8's 300 s cool-down is queued the same way. No code changed.
- **R4 (b) applied before the sitting:** C5, C6, D1, E1, E3, E4, E7 (listed more than 14 days)
  became ROADMAP M1.29 and M3.2.10; C4 merged into C3; E12 dropped (slices a/b changed the
  workspace it asked to see unchanged); E13–E15 merged into one look. Nothing is closed by this.
  Numbered D201 because main took D194–D200 for the UI slices.
- What remains only the owner's: A1, B4 → A3, A2, B9, the milestone closures and the looks.

Checked on Linux: `check_docs` and `check_file_sizes` (built standalone from `tools/`) pass. No
code changed and no desktop build or CTest run was made for this entry.

## 2026-10-16 — UI slice e2: measured LOD (D200)

The last UI presentation slice (`canvas_style.hpp`, `canvas.cpp`, `canvas_connectors.cpp`).
- **Rule:** below 3 logical px per lane (narrowest lane × `viewScale()`), Link lane markings,
  Connector edges/dividers and mouth edges are not drawn. They are 0.10 m strokes, under a tenth
  of a pixel there. Nothing else is hidden; hits read model geometry, so picking is unchanged.
- **Vehicles:** the 4×2.5 px floor and 14 px windshield/cab rule (D97) were already in; the new
  tests pin both (mutating 14→13 or 2.5→2.0 fails them).
- **Tests first:** new `canvas_lod_ui_tests` (crossing fixture at 3 px per lane and just under:
  markings present/absent; every other tag count equal; selected Link keeps outline, arrows and
  grips; hits equal; Ctrl+A equal either side; rotated and flipped views decide alike; vehicle
  boundary rows). It failed before the change with “Lane markings drawn below 3 px per lane”.
  `wireframe_ui_tests` now views the M2.6 template at 4 px/m: its fit zoom is under 3 px per lane.
- **Measured** (Debug offscreen, three runs each): `trafficsim-editor-benchmark` at 0.08 px/m,
  whole-scene redraw 6.6–8.1 → 3.0–4.2 ms, wireframe (untouched) 3.9–5.5 → 4.1–5.4 ms. The run-view
  benchmark sits at 3.1 px per lane, above the threshold: no claim. Both tools now print their
  px/m.

Checked on Linux (Ubuntu 24.04, GCC 13.3, Qt 6.4.2 offscreen): desktop `check` passes architecture,
file sizes, docs and **159/159** CTest. No Windows or high-DPI verification.
[At 3 px](images/ui-lod-after/crossing-at-3px.png), [just under](images/ui-lod-after/crossing-under-3px.png).

## 2026-10-16 — UI slice e1b: run casings and signal shapes (D199)

Run frames read without colour (`src/editor/run_vehicle_item.cpp/.hpp`, `canvas_run.cpp`,
`road_crossbar.cpp/.hpp`, `canvas_map.cpp`, `canvas.hpp`).
- **Casings:** each run vehicle paints its fill, a 1 px cosmetic edge opposite the fill's tone
  (`connectorBoundaryColor`) and any followed outline. Painting, not a pen, keeps `shape()` and
  so every run-vehicle click exactly as before.
- **Signal shapes:** red is the full bar, amber thins the middle third, green opens it. Hit shape
  and colours are unchanged; edit-mode dashed bars still mean draft.
- **Found from the evidence image:** `QGraphicsView` scrolls the viewport's child widgets with the
  scene, so the HUD (and the map credit and status before it) slid with the content until the 80 ms
  map timer; `scrollContentsBy` now re-places them at once.
- **Tests first:** new `run_presentation_ui_tests` (a vehicle the colour of its background still
  shows an edge ≥ 0.3 luminance, both dark and light; casing leaves `shape()` equal to the bare
  path; the three cues have distinct ink masks with red the superset and one hit shape; after 300
  Steps every run vehicle is cased and a click reaches `runVehicleClicked`). Removing the casing or
  the cue fails it. `canvas_hud_ui_tests` gained a scroll row that failed before the fix.
- **Cost:** `trafficsim-run-view-benchmark` on the M2.6 template, 600 Steps, Debug offscreen, three
  runs each: paint 1477–1692 → 1671–2015 ms (≈ +16 %), action 523–571 → 546–622 ms (unchanged code,
  so ≈ +7 % is noise); ≈ +9 % paint against action. Recorded in D199; not hidden.

Checked on Linux (Ubuntu 24.04, GCC 13.3, Qt 6.4.2 offscreen): desktop `check` passes architecture,
file sizes, docs and **158/158** CTest. No Windows or high-DPI verification.
[Grayscale run](images/ui-run-after/run-grayscale.png).

## 2026-10-16 — UI slice e1: canvas HUD (D197)

The canvas has a HUD (`src/editor/canvas_hud.cpp/.hpp`, `canvas_map.cpp`, `canvas.cpp/.hpp`,
`map_layer.cpp/.hpp`, `editor_map.cpp`, two keys per locale).
- **Scale and zoom:** a bar of the largest 1/2/5×10ⁿ m that fits 160 px at `viewScale()` (64–160 px:
  the steps are up to 2.5× apart, so 80–160 cannot always hold) and the zoom in px/m.
- **North only when the map supplies it:** `MapLayer::worldToGeographic` (new) gives the view centre's
  latitude; one step north mapped back and through the view transform gives the arrow, so
  convergence, `rotationDegrees` and Lab heading-up are included. No map projection, no arrow.
- **Never in the way:** mouse-transparent, bottom-right, above the map credit when they would meet,
  stacked when narrow. Found while testing: `EditorCanvas::event` placed overlays before the viewport
  resized, so after a resize the HUD (and credit) sat at the old size until the 80 ms map timer; they
  are placed after `QGraphicsView::event` now.
- **Tests first:** new `canvas_hud_ui_tests` (1/2/5 steps and 64–160 px at four zooms under rotation and
  mirroring; no arrow without a georeference; 0° on the central meridian, ±30° with the rotation, the
  ≈0.56° convergence at 101.5°E, +40° in a rotated view; `childAt` skips the HUD; clear of credit and
  status at four sizes), also at 1.5× and 2×. Swapping in grid north fails it. `workspace_ui_tests`
  checks the HUD inside the viewport at 1360 and 1024 in both languages. `design_system_ui_tests`'
  empty-grid pixel check now skips the HUD's own rectangle and asserts it is there.
- **Carved out:** run vehicle casings and an R/A/G shape cue are slice e1b (two systems, AGENTS.md).
- **Owner E19, same day (D198):** north shows whenever there is a georeference; `MapLayer` builds the
  projection without waiting for a tile provider. The HUD suite checks an unknown provider and a
  hidden map (it failed on both before the change).

Checked on Linux (Ubuntu 24.04, GCC 13.3, Qt 6.4.2 offscreen): desktop `check` passes architecture,
file sizes, docs and **157/157** CTest; workspace canvas unchanged at 807×394 / 471×231. No Windows
or high-DPI verification. [HUD with a rotated georeference](images/ui-hud-after/hud-rotated.png),
[Thai 1024 workspace](images/ui-hud-after/workspace-th.png).

## 2026-10-16 — UI slice d2: table and numeric presentation (D196)

Every table now reads alike (`editor_style.cpp/.hpp`, `editor_interaction.cpp`, `editor_workspace.cpp`,
`od_lot_dialogs.cpp` and the figure cells of Results, batch, comparison, Links, routes, inputs,
conflicts and the Lab axis).
- **Recipe at polish time:** `applyTableRecipe` gives every `QTableView` 24 px rows, alternating rows,
  no grid and control-height minimum columns when it is polished; before, only tables alive in
  `buildWorkspace` had it, so dialog tables kept Qt defaults. `editorCustomRows` keeps the OD/lot
  grids' spin-box rows.
- **Elided cells:** a cell without its own tooltip shows its full text in the bounded popup when the
  text is wider than the cell; a cell that fits shows nothing.
- **Figures:** cell `QString::number(x,'f',d)` calls became `formatValue(x,d)`. The default locale is
  C, so no byte changes: the CLI report and `--csv` for `four-leg-signalised` (seeds 42–43) are
  identical before and after, apart from the build-commit stamp.
- **Tests first:** new `table_presentation_ui_tests` (a table built after startup and the demand
  catalog dialog's tables carry the recipe; an elided long link name opens the popup, a fitting cell
  does not; Link, route and movement figures identical in English and Thai and right-aligned). It
  failed on the late table before the change; removing the elision check fails it too.
- **Noticed:** `map-ui` failed once in the first parallel `check` ("libpng error: Not a PNG file"
  among its output, no failure line captured); it passed alone, in a second full parallel run and
  5/5 times under concurrent load. The bounded popup is taller than two lines need (D151 sizing).

Checked on Linux (Ubuntu 24.04, GCC 13.3, Qt 6.4.2 offscreen): desktop `check` second run passes
architecture, file sizes, docs and **154/154** CTest. No Windows or high-DPI verification.
[Dialog table](images/ui-tables-after/tables-dialog.png), [elided-name popup](images/ui-tables-after/tables-tooltip.png).

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
