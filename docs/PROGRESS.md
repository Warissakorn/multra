# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-10 — UI review: Results by level, Open example, crossing rows, the batch band (D497–D499)

- **Owner:** chose four of the UI review's five proposals; the fifth (an example that shows LOS and
  the HCM table) is to be a new shipped project, next, not a change to `four-leg-signalised`.
- **The batch band (a bug):** `refreshBatch` hid the single run's two tables but not the frame
  around them, which kept half the page. The frame goes with them. `batch-run-ui` failed on the
  new row before the fix.
- **D497, Results by level:** a picker at the head of the Results toolbar shows Micro, Meso, Macro
  or Node (HCM); tabs are ordered by level, Macro in the study chain's order. Parking lots and
  Nodes leave the strip while the project has none; a one-tab level shows no strip. Making any tab
  current picks its level, so every existing jump (`setCurrentWidget`) works unchanged
  (`results_levels.*`). Found while testing: Qt makes the next tab shown the current one while the
  current tab is hidden, so the current tab is set first.
- **D498, File → Open example:** `data/examples.json` lists the seven projects with names and
  purposes in both languages; an example opens untitled. An empty drawing's status line points at
  the Link tool and the menu (`example_menu.*`).
- **D499, crossing rows:** Check runnability adds one advisory per place where two roads cross with
  no conflict area, from `automaticConflicts` and `conflictGroups`; selecting it selects a road.
  In the shell only: `runDiagnostics`, the CLI and readiness are unchanged.
- **Tests:** new `example-menu-ui`, `crossing-problem-ui`; rows added to `results-tabs-ui` and
  `batch-run-ui`. `tables-ui` asserted "no findings" on `crossing.json`, which has an unprotected
  crossing: it now asserts that every row is that advisory. `src/shell/CMakeLists.txt` gains
  `multra_ui_test` and five registrations use it (the file was at 500 lines).
- **Seen in the window** (Linux, 1440 × 900): [images](images/ui-review-d497/). The run view's
  protection line gained one short sentence and wraps to two lines at this width.
- **Verified:** Linux only. Desktop build and ctest 314/314; headless `check` passes (ctest 204/204,
  architecture, docs, file sizes). Windows and high DPI are the owner's look (B45).

## 2026-11-10 — Engine review: the tick's phases split; streams contract (D495, D496)

- **Owner:** "ทำต่อ ข้อเสนอของรีวิว engine"; on random streams chose the contract first, code later.
- **D495:** `stepSimulation` loses two of its phases to `tick.hpp`: `insertArrivals`
  (`insertion.cpp`) and `publishMoves` (`publish.cpp`). The lines are the same lines; `simulation.cpp`
  goes 500 → 380. Phase 1's per-vehicle loop stays one body (the hot path; split it with a
  callgrind run).
  - **Evidence:** 30 outputs compared before and after, byte-identical: six shipped projects at
    seeds 42 and 47 with every diagnostic flag, and M0 event streams at seeds 7, 42, 43.
  - **Cost:** `multra-engine-benchmark 12 120` under callgrind, Release: 249.14 M → 247.45 M
    instructions (−0.7 %). One run each; the count is deterministic, wall time was not compared.
- **D496:** [INPUT_STREAMS](reference/INPUT_STREAMS.md), a proposed contract for one random stream
  per input. Today every draw shares `randomState`, so one edited input moves every arrival
  (D459 measured 7 %). Rows IS1–IS9 and four owner choices (OWNER_SITTING B44). No code.
- **Not done, and why:** rows comparing the engine with HCM need a tolerance; M6.0 keeps engine
  numbers out until the owner chooses the benchmark (B9), so none was written.
- **Small:** `progressionFactor` returns 0 at P = 1 (its limit; 0/0 as written, unreachable from
  the product), with rows in NA3; an `if` in `node_suggestions.cpp` moved to its own line.
- **Verified:** Linux only. Desktop build and ctest 312/312; headless `check` passes (ctest 204/204,
  architecture, docs, file sizes). Windows not checked.

## 2026-11-10 — Architecture review: every module's includes are checked (D494)

- **Owner:** "ทำต่อ ข้อเสนอของรีวิวสถาปัตยกรรม" (the architecture review's proposals).
- **D494, the checker:** `multra-check-architecture` gains one rule per layer above the engines:
  `model` (engines only, no Qt or JSON), `commands` and `geo` (never the UI, no Qt) and `editor`
  (never `shell` or `lab`). Negative fixtures in `--self-test`. Two mutations (a shell include in
  `canvas_cursors.cpp`; `<QString>` in `geometry.cpp`) each failed the check.
- **`editor` no longer includes `shell`:** `path.hpp` moved to `src/editor/`; `EditorIcon` and
  `paintEditorGlyph` moved out of `shell/editor_style.*` into `editor/editor_glyph.*`. No behaviour
  changes: the same code in another file.
- **Targets:** `multra_macro`, `multra_meso` and `multra_analysis` are their own libraries, linked
  by `multra_model`; they were sources of `multra_core`. A stray include between engines now also
  fails to link.
- **ARCHITECTURE:** a *Module dependencies* table (what each module may include, Qt, JSON), checked
  against the real include counts.
- **Not done, the owner's choice first:** splitting `src/project/` and `EditorWindow`; both stay
  **Ask** in [NEXT](NEXT.md). `shell` has no rule: it may include every module.
- **Verified:** Linux only. Desktop build and ctest 312/312; headless `check` passes (ctest 204/204,
  architecture, docs, file sizes). Windows not checked; the moved headers are compiled there by CI.

## 2026-11-10 — Purpose review: status documents brought back to the code (D493)

- **Owner:** asked for a five-part review in chat (purpose, architecture, engine, UI, process), then
  "ทำตามข้อเสนอตั้งแต่รีวิวแรก" and chose: the first review's proposals only this session; PROBLEM §2
  unchanged (D38 stands); D380 unchanged, with the question filed for the owner.
- **Status documents:** README, ARCHITECTURE, PROJECT_SCHEMA, DESKTOP_AUTOMATION and
  `src/project/README` named schemas 32, 65 and 58 as the latest; they now name `kLatestSchema`
  instead of a copy of its value (it is 73). README's Implemented table gains Macro, Meso and the
  Node method; ROADMAP no longer calls M10 and M11 "not started"; ARCHITECTURE's Macro and Meso
  rows say what is built. No gate moved.
- **D493:** PROBLEM §6 no longer says the physics is not the hard part; joining the published
  models is named as the second hard part, with the roadmap items that show it.
- **Owner items:** OWNER_SITTING B43 asks whether Micro's M6 gate may close before M10 and M11
  (D380). M5.7, the M6 benchmark choice (B9) and a second user stay the owner's; no session can do them.
- **Queued:** the other four reviews' proposals are in [NEXT](NEXT.md), in review order.
- **Verified:** Linux headless only: `check` passes (ctest 204/204, docs, file sizes). Documentation
  only; the desktop suite was not run.

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
