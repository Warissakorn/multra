# PROGRESS log archive — 2026-10-16

Old PROGRESS entries, moved whole, newest first. Nothing here is current: NEXT and ROADMAP
hold status, the decision record holds the reasoning.
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
output byte-identical to main. Release timings in the [evidence](../evidence/lateral-zones.md): switch
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
[At 3 px](../images/ui-lod-after/crossing-at-3px.png), [just under](../images/ui-lod-after/crossing-under-3px.png).

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
[Grayscale run](../images/ui-run-after/run-grayscale.png).

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
or high-DPI verification. [HUD with a rotated georeference](../images/ui-hud-after/hud-rotated.png),
[Thai 1024 workspace](../images/ui-hud-after/workspace-th.png).

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
[Dialog table](../images/ui-tables-after/tables-dialog.png), [elided-name popup](../images/ui-tables-after/tables-tooltip.png).

## 2026-10-16 — UI slice d1: Properties presentation (D195)

Properties forms now line up (`editor_interaction.cpp/.hpp`, `editor_inspector.cpp`,
`editor_connectors.cpp`, `editor_behaviour.cpp`, `editor_workspace.cpp`, one QSS rule, one token,
five keys per locale).
- **Cause, measured:** `ReflowLabel`'s hint was its natural width and `QFormLayout` sizes the column
  to the widest label, so the English markings label pushed every Link field under its label, and
  ID/Name, appearance and behaviour each had their own column ([before](../images/ui-properties-before/properties-en-1360.png)).
- **Column:** `alignFormLabels` marks the Properties form labels once; they take 120 px
  (`formLabelWidth`), wrap inside it and keep their wrapped height as a minimum. Below two columns a
  form switches to `WrapAllRows`: the first fixed-column build squeezed fields to slivers at 200 px.
- **Apply divider and headings:** a rule with 8 px each side separates drivingSide (immediate) from
  level/display (Apply); eyebrow headings Geometry, Split and opposite, Attachments, Lanes and shape,
  Placement. Connector positions moved up beside the lane counts (row order only).
- **Tests first:** new `properties_layout_ui_tests` (field column per group at 1360/1024 × en/th, no
  clipped label, fields ≥ 80 px beside labels, all stacked at 200 px, only action buttons may scroll
  sideways, divider position, drivingSide immediate / level on Apply / Name on editingFinished,
  headings in both languages, connector row order), also at 1.5× and 2×. It failed on the markings
  row before the change.
- **Found, not fixed (E17):** at 1024×768 with the appearance fold open the non-scrolling top section
  does not fit and Qt compresses it; that was so before (the Apply button overlapped) and wrapped
  labels make it taller, so the suite checks 1024 folded. Text-only action buttons (up to 232 px)
  still need the page's horizontal scroll at 200 px, as before.

Checked on Linux (Ubuntu 24.04, GCC 13.3, Qt 6.4.2 offscreen): desktop `check` passes architecture,
file sizes, docs and **153/153** CTest; workspace canvas unchanged at 807×394 / 471×231. No Windows
or high-DPI verification. [After](../images/ui-properties-after/properties-en-1360.png),
[Thai 1024](../images/ui-properties-after/properties-th-1024.png),
[narrow](../images/ui-properties-after/properties-th-narrow.png).

## 2026-10-16 — UI slice c: grouped Objects navigation (D194)

The owner's B16 choice (layout A) is in (`src/shell/editor_tables.cpp`, `editor_workspace.cpp`,
`editor_window.hpp`, five Problems/Show-in-table jump sites, seven keys per locale).
- **Task groups:** a combo `editorObjectGroup` as the Objects strip's top-left corner widget shows
  Network, Control, Demand, Measurement, Results or Problems pages. `editorObjectsDock`,
  `editorObjectTabs` and all 14 pages keep their names and order; hidden tabs use `setTabVisible`.
- **One page identity:** the mapping is one table of registered page keys; a page in no group fails
  construction. The group follows the current page, so every existing `setCurrentWidget` jump
  (tools, batch, comparison, tests) reveals its group without code at the call site. Problems jumps,
  Show in table and conflict selection use `revealObjectPage`, which also shows the dock.
- **Transient:** choosing a group opens its leftmost page; nothing is saved; Reset returns
  Network/Links; focus mode keeps the group.
- **Corner width:** the tab widget places the strip from the corner's hint but never resizes the
  corner, so the Thai combo kept the English width and covered the first tab; the combo's width
  is now fixed to its hint on every retranslation.
- **Tests first:** new `object_groups_ui_tests` (six groups and their pages in en/th, a jump to each
  hidden page from another group and a locale switch on it, tool navigation, failed Run with the dock
  hidden, focus, Reset, every group at 1360×860 and 1024×768 in both languages, no overlap with the
  strip); `workspace_ui_tests` checks the batch shows the Results group. Two deliberate breaks
  (no group sync, no dock reveal) and the overlap each fail the suite.

Checked on Linux (Ubuntu 24.04, GCC 13.3, Qt 6.4.2 offscreen): desktop `check` passes architecture,
file sizes, docs and **150/150** CTest. Canvas after 100 Steps unchanged at 807×394 English and
471×231 Thai: the selector adds no dock height. No Windows or high-DPI verification.
[Before](../images/ui-palette-after/workspace-en.png) / [after](../images/ui-groups-after/workspace-en.png),
[Demand group, Thai 1024×768](../images/ui-groups-after/objects-demand-th.png).

## 2026-10-16 — UI slice b: palette groups, icons-only view and Simulation toolbar (D193)

The owner's B17 choice is in (`src/shell/editor_palette.cpp`, `editor_workspace.cpp`,
`editor_run.cpp`, `editor_window.cpp`, one QSS rule in `editor_style.cpp`, two keys per locale).
- **Five palette groups:** Geometry {0,1,5,2}, Control {8,9}, Demand {6,7,12}, Measurement {10,11},
  Tools {3,4}. Mode numbers, chords and the hidden `editorTool` combo are unchanged.
- **Icons-only view:** a checkable `editorPaletteIconsOnly` action on an arrow button above the tree
  (and in View). Its checked state is the only record of the view: `translatePalette()` reads it,
  nothing saves it, and Reset panel layout returns to names. Rows keep name + chord in tooltip and
  accessible text; the level filter shrinks and the background toggle shows the image glyph.
- **Toolbars:** Run/Pause, Step | Seed, speed | Reset on the Simulation bar; Delete selected trails
  Drawing tools behind a separator. The Run button lost its vertical toolbar padding: text beside
  icon made it 27 px and the Simulation bar 35 px, not 32, at every width.
- **Tests first:** `workspace_ui_tests` (five groups; both views × both languages keep number,
  chord, name and accessible text; chord and Down-key traversal; reset; separators; 1024/1179/
  1180/1360 × en/th × idle/seed-focused/running/paused/focus mode: 32 px bars, 24 px fields, no
  overflow, playback in view), `design_system_ui_tests` (shared glyphs and 24 px toggle in both views;
  icons-only active row Mid + left bar, focus-only row frame), `editor_workflow_ui_tests` (F5, F6,
  Space, Esc, +/− contexts; a guard that already passed). Before the change the workspace and design
  suites failed on the missing toggle; the toolbar rows then found the 35 px bar.

Checked on Linux (Ubuntu 24.04, GCC 13.3, Qt 6.4.2 offscreen): desktop `check` passes architecture,
file sizes, docs and **149/149** CTest. Canvas after 100
Steps: 807×394 English, 471×231 Thai (was 807×391 / 471×228). No Windows or high-DPI verification.
[Before](../images/ui-palette-before/workspace-en.png) / [after](../images/ui-palette-after/workspace-en.png),
[icons only](../images/ui-palette-after/workspace-icons.png).

## 2026-10-16 — UI slice a: icon vocabulary (D192)

The two source-identical glyph pairs from the UI audit are separated, and Fit/Focus no longer
differ by the centre cross alone (`src/shell/editor_style.cpp`, `editorIcon()` only). Connector is a
two-rail road bend with no arrow; Route is a start dot, trace and arrowhead; Reset is the mirrored
return arrow around an origin dot (Rotate unchanged); Focus is a wide panel with a double arrow.
Metrics, pen, roles, raster ratios, cache and every assignment are unchanged. Counter/Section and
Link/Split were already different outlines at 16 px (shape difference 0.90 / 0.73), so they keep
their glyphs; the contact sheet shows Grid/Conflict as another near pair for the owner's review.

- **Tests first** (`tests/design_system_ui_tests.cpp`): all 30 glyphs have exactly 16/32/48 px
  rasters and paint only WindowText (run/step: also the accent); no two 16 px ink masks are equal;
  the five named pairs differ in ≥ 40 % of their combined ink; palette rows and Network objects
  share `editorIcon()`'s cached QIcon. Before the redraw the suite failed on
  "rotate and reset are the same 16 px shape" (connector/route 0, fit/focus 0.23). The floor was
  set before measuring; the first Focus redraw measured 0.39 and the shape was changed, not the floor.
- **Contact sheet:** `trafficsim-design-ui-tests <data> --contact-sheet <png>` renders each glyph at
  actual 16 px, 4× without smoothing and at 32/48 px, in grey. A Rotate with a tilted body inside
  read as "@" at 16 px and was dropped. Pixel difference is not recognition; OWNER_SITTING E14 is.
- Nested Objects-page toolbars stay text-only: an icon there would replace or widen their labels on
  pages slice c rearranges.

Checked on Linux (Ubuntu 24.04, GCC 13.3, Qt 6.4.2 offscreen): desktop `check` passes architecture,
file sizes, docs and **149/149** CTest. Workspace canvas 807×391 (English) and 471×228 (Thai, after
100 Steps) before and after; the audit's 475×248 came from another font setup. No Windows or
high-DPI verification. [Before](../images/ui-icons-before/contact-sheet.png) /
[after](../images/ui-icons-after/contact-sheet.png).

## 2026-10-16 — Parallel tests, cheaper hit-testing, vcpkg cache (no decision)

An /optimize pass over engine, editor, build/CI and docs (Linux, GCC 13.3, Qt 6.4 offscreen; no
Windows or real-display verification). Three changes, one commit each:

- **`check` and CI run ctest in parallel.** The test groups were already separate ctest tests so
  `-j` could spread them, but `check` and every CI ctest step ran serially. `check` now passes
  the host core count; Windows and packaging steps pass `--parallel 4`. Desktop Debug `check`
  294 → 74 s; 7/7 parallel runs green; Windows desktop ctest 1m20 → 0m40 in CI. The sanitizer step is
  noisy on CI (8m12 serial, 9m39 parallel, 13m18 serial in runs 554–556) but scaled 952 → 254 s
  locally, so it runs in parallel too; `TRAFFICSIM_TEST_JOBS` overrides the job count.
- **`hitObjects` rejects misses by padded bounds** and takes shapes from `linkShape` /
  `connectorShape` instead of an id scan per object (it was quadratic), computing distance for hits
  only. Release editor benchmark, 200 intersections: 8.28 → 1.22 ms, wireframe 14.24 → 4.61 ms.
  Band selection uses the same helpers. A connector whose paths cannot be built now throws only
  when it is hit.
- **windows-desktop caches vcpkg binaries** (`.vcpkg-cache`): proj was rebuilt from source,
  5m11s of every run; with the cache it takes 24 s and the
  whole windows-desktop job about 3 min (run 556).

Looked at and left: the engine (no `stepSimulation` child above 9 %, noise ±7 %), the Run view
(already batches Steps per frame), docs (the audit's broken references were false positives).
