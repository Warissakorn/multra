# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-11 — Pick the origin once, then several destinations (D506)

- **Owner:** "ต้องการให้เลือกต้นทางครั้งเดียว และเลือกปลายทางได้หลายอัน" (pick the origin once, then
  several destinations).
- **D506:** after the Route tool saves a traced destination, `EditorCanvas::commitRouteDraft` keeps
  the start object and station, so the next click is another destination from the same origin and
  joins the same routing decision. A click back on the origin's line does nothing; the same
  destination by the same roads is refused (`EDIT_ROUTE_DUPLICATE`, en/th); Esc or Backspace on the
  bare origin ends it. The status hint says the origin stays. `Ctrl`+right/`Enter` is unchanged.
- **Rows:** `station-routing-ui` now draws two destinations from one origin click and checks the
  no-op click, the refused duplicate, Esc and Backspace; `road-crossbar-ui` expects the bare origin
  after a commit. Verified: Linux, desktop ctest 312/312 and `check`; Windows is the owner's.

## 2026-11-09 — A shipped Macro example: four steps on a four-zone town (D492)

- **Owner:** "สร้างโปรเจ็คตัวอย่างการทำ แบบจำลองระดับ Macro" (a sample project for Macro modelling).
- **D492:** `data/projects/macro-four-step.traffic.json`, built by `tools/macro_example_network.hpp`
  (`multra-macro-example <data-dir> <output>`). Four zones (West, North housing; East employment;
  South mixed) around a centre: two-way radial arterials, a two-way ring road and an access pair per
  zone, so the origin's split is route choice. It owns everything the procedure reads: three
  practical-capacity link types, trip rates (HBW, HBO), gravity betas, transit minutes, a car/transit
  logit, AM period factors and `four-step`. Every value is an illustration, cited as such.
- **Found while building:** the installed HCM link types are `ultimate` and BPR is `practical`, so
  Macro refuses them (D304); and the installed car has no PCU (`MODE_PCU_MISSING`), so the project
  owns its demand catalog with car = 1 PCU by definition.
- **Result (not validated):** 11 427 car veh/h; converged in 8 iterations (gap 9.3 × 10⁻⁵).
  West → East is 6 % quicker on the arterials at free flow; the arterial into East reaches V/C 0.83
  and the two routes equalise (632 s vs 636 s on their Links).
- **Rows:** `macroexample` (the file is the builder's output; the procedure's four steps, Wardrop
  equality, the inbound peak) and `cli-macro-example`; `zonescodec` z1 exempts the example from
  "no shipped zones" and `studyperiods` counts seven files. Two mutations (a rate edited in the
  file; demand cut to 30 %) each failed a row. Verified: Linux headless only, ctest 204/204;
  the desktop (Qt) suite not run.

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

- **D485 ([PROCEDURE_SEQUENCE](reference/PROCEDURE_SEQUENCE.md) §9):**
  - *Procedures…* edits the list and its steps, refused in place, as one History step;
  - the twelfth Results tab runs a procedure on a worker with Cancel and shows each step's status
    and summary, with stale marking and a marked CSV.
- **Rows:** `procedure-ui` (PS10); `workspace-ui` counts 12 tabs.
- **Note:** `docs/decisions/RECORD.md` reached 500 lines with D485; splitting it is the owner's call
  (NEXT).
- **Verified:** Linux only (desktop and headless). Windows not checked (OWNER_SITTING 42).

## 2026-11-06 — Procedure sequence P1: engine, schema 73, CLI (D484)

- **Owner:** approved the plan after D483.
- **D484 ([PROCEDURE_SEQUENCE](reference/PROCEDURE_SEQUENCE.md) §8):**
  - `procedures` at schema 73, and `runProcedure` (nearest earlier active input, an in-memory
    assignment demand, a refusal stops the rest, Cancel);
  - `procedureJson` and `--procedure`;
  - `generatedOdMatrices` moved into the project layer.
- **Rows:** `procedure` (PS1–PS8) and `cli-procedure*` (PS9). Found by the rows: a `{}` initialiser
  had set every step to *done*; fixed. Four mutations each failed a row.
- **Verified:** Linux only (desktop and headless). Windows not checked.
