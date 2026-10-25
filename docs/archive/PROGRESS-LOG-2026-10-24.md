# Archived progress — 2026-10-23–24

Entries moved whole from the live log; dates and evidence describe their own session.

## 2026-10-24 — S5.5b: generated objects in the area, export marking; S5 complete (D332)

- **Asked:** continue; S5.5b was next. The contract left no owner question open.
- **Built** ([STUDY_AREA_IMPLEMENTED](reference/STUDY_AREA_IMPLEMENTED.md) §21):
  - the Problems tab warns once per study-area Node holding program-drawn objects, with the
    count by kind, and the row selects the Node. The run still starts;
  - every export (single, batch, comparison, Macro, Meso; editor and CLI) carries a second header
    line when generated objects or stale inputs are behind it, plus a line naming where a study
    area's flows came from. JSON gains `marking` / `studyArea` the same way. Unmarked exports keep
    their bytes;
  - the CLI reads the side files too, and a comparison's alternative uses its own.
- **Rows:** SA14, SA15 (`studyarea`), the Problems row in `node-ui`. Linux only, GCC 13, Qt 6.4.2
  offscreen: `ctest --preset desktop` 225/225; file-size, docs and architecture checks pass. Not
  built or run on Windows. No test runs the CLI on a study-area project.

## 2026-10-24 — S5.5a: Macro and Meso side files, stale by SHA-256 (D331)

- **Asked:** continue; S5.5 was next. It was split like S5.2: S5.5a is the side files and
  staleness; S5.5b is the marking line and the generated-object warnings. The owner chose: after
  a reopen, the cut source only; a network mismatch runs at compile, marked stale; paths and
  watches only when the area cuts that level.
- **Built** ([STUDY_AREA_IMPLEMENTED](../reference/STUDY_AREA_IMPLEMENTED.md) §20):
  - Run Macro and Run Meso of a saved project write `<project>.macro.json` / `.meso.json` with the
    project bytes' SHA-256;
  - the study area's cut reads a usable side file in place of the run at compile, and says
    `MICRO_AREA_SOURCE_STALE` when it is for other bytes or unusable;
  - a bad side file is refused at its JSON path (`MICRO_AREA_SOURCE_INVALID`);
  - the Macro and Meso results are stale by hash, so this survives a reopen.
- **Rows:** SA12, SA13 (`studyarea`), and the reopen checks in `macro-ui` / `meso-ui`, which now
  open their fixture from a temporary copy. Linux only, GCC 13, Qt 6.4.2 offscreen:
  `ctest --preset desktop` 225/225; file-size, docs and architecture checks pass. Not built or
  run on Windows.

## 2026-10-24 — S5.4: the Meso cut for a study area (D330)

- **Asked:** continue; S5.4 was next. The owner chose that Meso, like Macro, runs at compile until
  S5.5, and that each Meso output slice becomes one Micro interval.
- **Source:** Yperman 2007 §3.2 ("FIFO conditions") and Eq. (5.2), read in the owner's copy. Route
  counts on a link follow its count by FIFO.
- **Found:** `loadMeso` already keeps those route counts, so the cut only exposes them for the
  kept Links (`watch`). No new model.
- **Built** ([STUDY_AREA_IMPLEMENTED](../reference/STUDY_AREA_IMPLEMENTED.md) §19):
  - `mesoGateFlows`: gate flows per slice, with their own run-time window;
  - *Cut from Meso* in the gate-flow dialog.

  A class naming one vehicle type cannot become a gate flow yet (`MICRO_AREA_CLASS_UNSUPPORTED`).
- **Rows:** SA10, SA11 (`studyarea`), the Source in `node-ui`. Linux only, GCC 13, Qt 6.4.2
  offscreen: `ctest --preset desktop` 225/225; file-size, docs and architecture checks pass. Not
  built or run on Windows.

## 2026-10-24 — S5.3: the Macro cut for a study area (D329)

- **Asked:** continue; S5.3 was next. The owner chose that, until S5.5's side files, Micro runs
  Macro at compile for each study period inside the run.
- **Built** ([STUDY_AREA_IMPLEMENTED](../reference/STUDY_AREA_IMPLEMENTED.md) §18):
  - Frank–Wolfe keeps its paths when asked: each step's all-or-nothing paths with the step weights,
    which sum exactly to the link flows;
  - `macroGateFlows` cuts each path's kept stretches into gate (or inside-zone) trips by period;
  - the gate-flow dialog's *Source*, Typed or Cut from Macro.
- **Measured:** keeping paths costs +72 % wall on a 12×12 grid with 1,892 OD cells (Debug), with
  27,089 paths. Ordinary runs do not keep them. The M2.6 template has no zones, so it cannot run
  Macro.
- **Rows:** SA7–SA9 (`studyarea`), the Source in `node-ui`. `MACRO_NOT_CONVERGED`'s pass-through is
  not exercised: the corridor's single path has gap 0. Linux only, GCC 13, Qt 6.4.2 offscreen:
  `ctest --preset desktop` 225/225; file-size, docs and architecture checks pass. Not built or run
  on Windows.

## 2026-10-24 — S5.2b: gate flows in the editor; S5.2 complete (D328)

- **Asked:** continue; S5.2b was next.
- **Built** ([STUDY_AREA_IMPLEMENTED](../reference/STUDY_AREA_IMPLEMENTED.md) §17):
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
- **Built** ([STUDY_AREA_IMPLEMENTED](../reference/STUDY_AREA_IMPLEMENTED.md) §16):
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
- **Contract:** [STUDY_AREA](../reference/STUDY_AREA.md).
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
- **Contract:** [READINESS](../reference/READINESS.md) §13. Legs are cut where a lane surface meets
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
  and [third-party notices](../../THIRD-PARTY-NOTICES.md). Dialogs are free functions with a
  scrolling body and fixed footer (D318, D319).
- **Recovery:** Map → Retry failed map tiles retries current viewport failures, keeping
  cached tiles, active requests and Retry-After. Hidden/removed maps cannot be retried;
  navigation and retry create no document revision.
- **Coverage:** `map-navigation` reproduces the pan/click jump and old zoom floor, including
  a large viewport and local-server overview rendering. `map-location-catalog` and
  `map-location-ui` cover country-only/province/manual/cancel, language, layout and saved
  geometry; `map-retry` and `map-ui` cover explicit recovery. See
  [MAP_BACKGROUND](../reference/MAP_BACKGROUND.md).
- **Verification:** Linux, GCC 14.2.0, Qt 6.8.2 and PROJ 9.6.0: all seven map CTest groups
  pass, including `mapproject`; architecture, file-size and documentation checks pass.
  The initial full suite passes 215/220: `legacy-storage`, `m1-workflow` and `test-storage-ui`
  cannot create Qt test-mode storage on this environment's read-only home; `workspace-ui`
  and `design-system-ui` fail existing font assertions under Qt 6.8.2. The same five failures
  reproduce before the navigation/place changes. No Windows or MAP10 gate is closed.
