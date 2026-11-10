# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-10 — Process review: stated facts checked, ROADMAP tables, a user guide (D502)

- **Owner:** asked for the rest of the queue: the process review's three proposals.
- **`multra-check-docs` checks facts:** a decision id in any maintained document must be a row of a
  decision record (`D18` names the family D18a–D18d); no document cites a schema above
  `kLatestSchema`, read from `src/project/document.hpp`; `data/projects/README.md` lists exactly
  the shipped projects. Negative fixtures for each. On its first run it flagged nothing wrong in
  the documents, and then this session's own `D502` until the row existed.
- **ROADMAP:** M10, M11, M13 and M14 are one table row per slice (contract, implemented, open)
  with scope and done-condition above; their paragraphs are whole in
  [the archive](archive/ROADMAP-implemented-2026-11-10.md). To stay at 500 lines the M1 and M2
  implemented-slice tables and the S1–S5 stage lines went there too, M0.1 became a paragraph of
  M0, and M8's follow-up line joined its paragraph. No scope, gate or status changed.
- **[USER_GUIDE](USER_GUIDE.md):** the Micro sentence on the four-leg example in five steps, with
  where LOS letters and the HCM table come from. Every quoted control was checked against
  `data/locales/en.json`. English only, no pictures.
- **Not done:** milestone status statements are not checked against code (no mechanical source
  for "implemented"); branch protection is the owner's on GitHub.
- **Verified:** Linux, GCC 13.3: headless `check`; desktop `documentation` rows.

## 2026-11-10 — A routing decision reaches a pocket lane (D501)

- **Owner:** chose this defect, found by D500, as the next work.
- **Cause:** `routeLaneFamily` took the first lane path leaving a lane at a named Connector and
  stopped. Fill's pocket taper is 1 → 2: first path to the ordinary lane, second into the pocket.
  A route still reached the pocket by D459's joined chain and a lane change; a decision's family
  is built without joining, so every chain to the far turn was a stub, the family was empty and
  `entryDecision` skipped the destination with no issue.
- **D501:** without joining, a chain forks over every path leaving its lane, and each first-Link
  lane keeps its first full chain, else its first stub. With joining (routes, compile, Node
  membership) nothing changes. An entry decision's destination that no lane serves now adds the
  existing advisory `ROUTING_DECISION_UNREACHABLE`.
- **Tests:** `pocketdecision`, 4 rows on the P3 pocket template, both driving sides; three failed
  before the fix (a destination at 1/2 instead of 1/3, a turn with no vehicle, no advisory), the
  fourth pins that a route's family is as before.
- **Unchanged results:** CLI output of the eight shipped projects and `crossing.json` is
  byte-identical before and after (build metadata lines aside); no fixture or baseline moved.
- **Not done:** the D500 example stays on the template without pockets; moving it is an Ask in NEXT.
  A decision's destination that needs a lane change on a later Link with no fan-out is still not
  served; it is now named instead of dropped.
- **Verified:** Linux, GCC 13.3: desktop ctest and headless `check`.

## 2026-11-10 — A shipped example that shows LOS and the HCM table (D500)

- **Owner:** chose a new example over changing `four-leg-signalised` (UI review, D497–D499 round).
- **D500:** `signalised-node-analysis.traffic.json`, built by `tools/node_example_network.hpp`
  (`multra-node-example`) the way the editor builds a junction today: two Street segments, Find
  crossings, the installed template `signal-4-p3-shared` with NEMA timing, and Fill. It carries
  one routeless input and one placed routing decision per approach, one signalised travel-time
  section per movement (12), turning counts on the Node equal to the inputs, and an evaluation
  window of 300–1200 s that is also the analysis period (0.25 h).
- **What it shows:** *Analyse* gives the HCM 7 Ch. 19 table (8 lane groups, every one below
  capacity); *Run seeds* gives section, approach and intersection letters. Both stay session
  results and keep the not-validated marker; every number is an illustration, not a count.
- **Found, not fixed (queued in NEXT):** with `signal-4-p3-major-far-pockets`, a placed routing
  decision's far-turn destination gets no path, although one exists lane by lane (the taper
  Connector's second lane path enters the pocket, and the turn leaves it) and `placedDecisions`
  lists both object chains. The destination is dropped with no issue and the other destinations
  take its share: 0 of an expected 30 far-turn vehicles in the window, while an explicit route
  over the same objects carried 29. Cause not yet found. The example therefore uses the template
  without pockets, and a test row pins that none of its destinations is dropped.
- **Changed from the approved plan:** the counts are typed on the Node, not *Fill from model*.
  A filled analysis stores SHA-256 of the project and of every installed catalog file read, so
  the committed example would fail its builder test on any catalog edit.
- **Tests:** `nodeexample` (3 rows, the first two failed before the file existed),
  `cli-node-example`, a row in `example-menu-ui` (open, Analyse, 8 + 4 rows, the Node level
  shown); `studyperiods` counts eight projects.
- **Verified:** Linux, GCC 13.3: desktop ctest and headless `check`. Windows is the owner's (B45). Images:
  [HCM table](images/node-example-d500/hcm-table.png), [LOS after five seeds](images/node-example-d500/los-sections.png).

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
