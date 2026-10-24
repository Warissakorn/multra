# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-10-24 — S5.2b: gate flows in the editor; S5.2 complete (D328)

- **Asked:** continue; S5.2b was next.
- **Built** ([STUDY_AREA](reference/STUDY_AREA.md) §17):
  - *Gate flows…* on the Nodes tab, a dialog with one origins × destinations grid in veh/h per
    study period and composition or mode;
  - `studyAreaEnds`, the one list of gates and inside zones that both the dialog and the run read;
  - a light fill on the canvas for each Node in the area.
- **Rows:** SA16 (`studyarea`), with the dialog, the fill and the disabled action in `node-ui`.
  Linux only, GCC 13, Qt 6.4.2 offscreen: `ctest --preset desktop` 225/225; file-size, docs and
  architecture checks pass. Not built or run on Windows; the dialog's size on a 1366×768 screen is
  not checked (OWNER_SITTING's D318 item covers it).

## 2026-10-24 — S5.2a: the study area and a Micro run on its kept set (D327)

- **Asked:** continue; S5.2 was next. It is split in two: S5.2a here; S5.2b, the gate-flow table and
  the canvas outline, comes next.
- **Built** ([STUDY_AREA](reference/STUDY_AREA.md) §16):
  - `studyArea` at schema 55;
  - the *Study area* column on the Nodes tab;
  - the kept set: the area Nodes' interiors, their legs and the pieces between them;
  - gates at the open ends, and typed gate flows;
  - a run-time copy that every Micro caller compiles.
  The document's own demand is listed as not run.
- **Found:**
  - F4 rebuilt a split arm from its template and grew it 0.1 m, so the split's Connector could not
    run (`UNSUPPORTED_CONNECTOR_POSITION`). F4 now keeps a leg's far end (READINESS §14 amended);
  - a coarse Node always reads `NODE_NO_TURN`, so the area checks detail first, to name the right
    fix.
- **Rows:** SA1–SA6 (`studyarea`), SA3's editor part in `node-ui`. Linux only, GCC 13, Qt 6.4.2
  offscreen: `ctest --preset desktop` 225/225; file-size, docs and architecture checks pass. Not
  built or run on Windows.

## 2026-10-24 — S5.1: the study area contract, answered (D326)

- **Asked:** continue; the owner picked S5 and answered its six choices in chat.
- **Contract:** [STUDY_AREA](reference/STUDY_AREA.md).
  - The study area is a set of Nodes (owner, against a polygon).
  - Micro keeps their interiors, legs and the pieces between them, with gates at the open ends.
  - Gate flows are typed, or cut from Macro or from Meso: both cuts are in S5 (owner).
  - Macro and Meso results go to side files, and staleness is SHA-256 of the project and source.
  - Generated objects in the area are warnings, and an export gets a marking line only when it
    applies.
- **Found while reading the code:** `MacroAssignment` keeps no path flows, so S5.3 keeps each
  Frank–Wolfe iteration's paths with their step weights, an exact decomposition. Meso keeps no
  per-path count at an inner link, so S5.4 adds FIFO counts at the gate links only. Today's stale
  check is an in-memory revision, lost on reopen.
- **Rows:** SA1–SA15 planned. Docs only, no code changed; the docs and file-size checks pass.

## 2026-10-24 — S4.5c: signals for a generated junction; S4.5 complete (D325)

- **Asked:** continue; S4.5c was next. The owner answered two points: co-green turns inside a stage
  yield by S4.5b's HCM ranks (so a signal needs two major arms), and derive back stays exact,
  reporting a turn carried across an intergreen as a stage of its own.
- **Built** (READINESS §16): a generated controller and one group per turn from the coarse stages,
  a generated head on each turn's Connector paths, and crossings only between turns green together.
- **Found:**
  - signal times must sit on the time step, so intergreens round down to it and the last takes the
    rest (`FILL_NODE_SIGNAL_GRID` for an off-step green);
  - a merge takeover joins turns never green together, and mixing rank priorities with its order
    made priority cycles. Every pair of a generated merge is now ordered by rank, then drawing order.
- **Rows:** RS15, plus signal cases in RS11, RS16 and RS18 (`nodedetail`, 10 rows); the signalised
  four-arm fixture runs at Micro. Linux only, GCC 13, Qt 6.4.2
  offscreen: `ctest --preset desktop` 224/224; file-size, docs and architecture checks pass. Not built or
  run on Windows.

## 2026-10-24 — S4.5b: who gives way at a generated junction (D324)

- **Asked:** continue; S4.5b was next.
- **Source:** HCM 7th ed., Ch. 20 §2 in the owner's copy (PDF p. 1590). The minor far turn is Rank 3
  at a three-leg junction and 4 only at a four-leg one; READINESS §13.4's draft said 4 everywhere.
- **Built** (READINESS §15): ranks by arm class and sweep sign; authored areas where generated turns
  of different arms and ranks cross or merge, higher rank yielding with `default.json`'s numbers;
  Stop on minor arms; `generated` on waiting lines, areas, rules and Stop controls. F4 now asks for
  major arms (`FILL_NODE_PRIORITY_UNDETERMINED`) and two-way stop's shape (`FILL_NODE_MAJOR_ARMS`).
- **Found:** derive back lost `major` once F4 drew priority, because a major far turn yields to the
  opposite through. Its rule now calls an arm major when one of its turns meets a decided conflict
  and never yields (ROADS_IMPLEMENTED §20 amended).
- **Rows:** RS14, RS18 (`nodedetail`); RS18 runs the generated four-arm junction at Micro.
  Linux only, GCC 13, Qt 6.4.2 offscreen: `ctest --preset desktop` 224/224;
  file-size, docs and architecture checks pass. Not built or run on Windows.

## 2026-10-23 — F4 contract answered; S4.5a: a coarse Node's Connectors (D323)

- **Asked:** continue; the owner picked F4 (S4.5) and answered its six choices as recommended.
- **Contract:** [READINESS](reference/READINESS.md) §13. Legs are cut where a lane surface meets
  another arm's (F4-1a), turns share lanes kerb to far (F4-2a), priority by HCM 7 Ch. 20's ranks
  with Stop on minor arms (F4-3a) and `default.json`'s gap values (F4-4a), signals from the stages
  in order (F4-5a), in three slices (F4-6a). Rows RS11–RS18.
- **S4.5a** (§14): `Connector::generated` (schema 54), cleared by the first edit in
  `History::execute`; the pure cut and turn plan (`node_detail.*`); `generateNodeDetail`; the
  `nodeDetail` fill, with a refused Node listed and not tickable. Cuts are measured on each arm's
  template geometry (`templateLinks`, extracted from *Apply template*), and *Apply template* takes
  them before changing its Links: a first try re-anchored the generated Connectors to the uncut end
  and lost them. A re-apply now changes no byte.
- **Reading taken:** only generated arms in S4.5a, since a hand-drawn arm's uncut shape is lost
  after a cut. Until S4.5b nobody gives way at the generated crossings.
- **Rows:** RS11–RS13, RS16, RS17 (`nodedetail`). Linux only, GCC 13, Qt 6.4.2 offscreen: `ctest --preset
  desktop` 224/224; file-size, docs and architecture checks pass. Not built or run on Windows.

## 2026-10-23 — Wider OSM zoom, stable panning and country/province selection (D321, D322)

- **Asked:** zoom out farther, stop the map jumping back after a pan/click, and choose a
  country with an optional province instead of entering coordinates only.
- **Navigation:** map zoom reaches 0.0005 px/m (viewport-adjusted on very large displays),
  with a bounded 2,000 km overview. Redraw previously applied a network-only scene rectangle
  before expanding it for the map, clamping the camera between the two calls. It now applies
  the complete bounds once and reserves the next viewport before pointer-centered zoom.
- **Places:** searchable offline selectors in settings and navigation, with 239 countries/
  territories, 4,588 regions and all 77 Thai provinces. A country-only selection is valid;
  manual coordinates remain available. Only the accepted reference coordinates are saved.
  Natural Earth public-domain points and Wikidata CC0 Thai names are cited in the catalog
  and [third-party notices](../THIRD-PARTY-NOTICES.md). Dialogs are free functions with a
  scrolling body and fixed footer (D318, D319).
- **Recovery:** Map → Retry failed map tiles retries current viewport failures, keeping
  cached tiles, active requests and Retry-After. Hidden/removed maps cannot be retried;
  navigation and retry create no document revision.
- **Coverage:** `map-navigation` reproduces the pan/click jump and old zoom floor, including
  a large viewport and local-server overview rendering. `map-location-catalog` and
  `map-location-ui` cover country-only/province/manual/cancel, language, layout and saved
  geometry; `map-retry` and `map-ui` cover explicit recovery. See
  [MAP_BACKGROUND](reference/MAP_BACKGROUND.md).
- **Verification:** Linux, GCC 14.2.0, Qt 6.8.2 and PROJ 9.6.0: all seven map CTest groups
  pass, including `mapproject`; architecture, file-size and documentation checks pass.
  The initial full suite passes 215/220: `legacy-storage`, `m1-workflow` and `test-storage-ui`
  cannot create Qt test-mode storage on this environment's read-only home; `workspace-ui`
  and `design-system-ui` fail existing font assertions under Qt 6.8.2. The same five failures
  reproduce before the navigation/place changes. No Windows or MAP10 gate is closed.

## 2026-10-23 — The owner's book copies analysed and mapped (docs only)

- **Asked:** analyse the five book repositories and add them as development sources.
- **Added:** [BOOKS](reference/BOOKS.md), how to attach and cite each copy, how far to trust each
  conversion, and which chapters serve which milestone; a read-by-task row in the docs map. The
  books stay in their own repositories (attached with `add_repo`, never vendored).
- **Checked** ([book-copies-check](evidence/book-copies-check.md)), from the source images:
  - HCM 7 confirms the M6.0 sheet's TWSC headways (Exhibits 20-17, 20-18, two-lane column),
    Eq. 20-18, the 1,750 pc/h/ln default and the base conditions; Eq. 19-19 keeps `PF`; the field
    saturation-flow method counts from the fourth queued vehicle;
  - `data/los/hcm.json`'s bounds equal HCM 7 Exhibits 19-8, 20-2 and 21-8;
  - ten Sheffi page images are empty, book p. 114 among them, so B21 still needs the print;
  - Ortúzar Example 11.1 (two routes, 1400 / 600 at 22 min) fits M9's two-route fixture;
  - the Planning Handbook is the 4th edition (2016).
- **Not done:** no number entered `data/` or a test, no benchmark was chosen, no gate moved.

## 2026-10-23 — UI suites isolate their recovery drafts (D320)

- **The problem:** a crashed or killed UI run left its recovery draft behind. The next local
  run of that suite then waited at the startup recovery dialog until ctest's 60 s timeout.
  Eleven suites had no Qt test mode, so they wrote drafts into the user's own data folder.
- **The fix:** all 51 `EditorWindow` suites call `useTestStorage()`. It enables test mode and
  removes drafts whose lock nobody holds, keeping a running instance's draft.
- **Rows:** `test-storage-ui` fails without the cleanup, and fails at the window (not a hang)
  without the file checks.
- **Results:** a planted crash draft no longer blocks `behaviour-library-ui`. Two full runs pass
  219/219; the eleven suites wrote nothing to the user's folder.

## 2026-10-23 — Optimization review: UX, build, docs, engine (D319)

- **Measured first** (Linux; `.optimize/` ledger, not committed):
  - UX: a `QDialog::exec` shim over the 61 UI suites, in English and Thai;
  - engine: Release `multra-cli`, seeds 42–51, and callgrind;
  - build: Debug rebuilds after touching one file;
  - docs: the words a session reads before it starts.
- **Done:**
  - the seam dialog's *Use Node* button lost 8 px to the table's item padding; the column gets
    it back, the only clipped control measured;
  - NEXT cut to open items (1113 → 469 words);
  - D319 records the accepted rules.
- **Found on the way, fixed:** D318's behaviour editor corrupted the heap. Qt 6.4.2's
  `QFormLayout::setRowVisible` on a shown dialog writes out of bounds (valgrind); the forms now
  stay still while their rows change. A crashed local UI run leaves a recovery file that makes the
  next run wait at its dialog.
- **Not done:**
  - caching lateral bands (+10–28 % wall switched on, no shipped keys);
  - a shared `multra_shell`;
  - splitting `types.hpp`: estimated 466 → 420 objects rebuilt, 338 with the JSON
    declarations moved. The owner deferred it to the next engine slice that edits the runtime
    state (NEXT).

## 2026-10-23 — Dialogs keep their buttons on screen; the behaviour editor in tabs (D318)

- **The owner's report:** the behaviour editor was taller than the screen; not every key showed
  and Confirm was off screen. Measured, it needed at least 1378 px.
- **The editor now has three parts:**
  - a header with users, id, name and model;
  - four tabs that each scroll: *Car following*, *Lane changes*, *Perception and drivers*, and
    *Junctions, signals and speed*;
  - a footer with the error and the buttons.

  The other model's rows are hidden. Prototype opens at 579 px at 1366×768.
- **Other dialogs:** the UI suites, measured through a scratch `QDialog::exec` shim, gave vehicle
  type 696 px minimum, input 500, decision 452, and coarse Node values (one row per leg). These now
  share `dialog_fit` (a scrolling body, a fixed footer and a first size capped to the screen).
- **Rows:** `requireFitsScreen` in four UI suites. Three fail with the old dialogs; the coarse and
  decision rows are growth guards.

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
