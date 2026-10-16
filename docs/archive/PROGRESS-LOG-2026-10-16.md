# PROGRESS log archive — 2026-10-16

Old PROGRESS entries, moved whole, newest first. Nothing here is current: NEXT and ROADMAP
hold status, the decision record holds the reasoning.
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
