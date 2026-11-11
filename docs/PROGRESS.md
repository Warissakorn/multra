# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-11 — Random streams per input in the engine (D506)

- **Owner:** "ทำตามคำแนะนำ" after D505: the recommendations, in their order. B9 and B43 needed no
  code; B44 (streams before the M6 runs) is the first that did. Branch protection and B45 stay the
  owner's; the D500 move and the `EditorWindow` split wait for their own sessions.
- **D506 ([INPUT_STREAMS](reference/INPUT_STREAMS.md) §4, §8 slices 1–2):** `Scenario::randomStreams`,
  `shared` by default. Under `perInput` each input and each lot draws from its own xorshift32 state
  seeded from the run seed and an FNV-1a key of its id; a routing draw is hashed from the seed, the
  vehicle's origin key and ordinal, and the decision's id (its id, not its index, so an added
  decision moves no other). A vehicle carries `streamKey` and `ordinal`; under `shared` they and
  the new state fields stay zero. A `perInput` checkpoint also writes the stream states.
- **Moved to slice 3:** the codec key and IS8's summary/CSV field and `--compare` refusal. No run
  outside the tests can be `perInput` until the project sets it, and an unconditional field would
  change every existing output's bytes.
- **Rows:** IS1–IS7 and IS6b (a lot's draws) in `inputstreams`. Each `perInput` row runs again
  under `shared` and expects the check to fail there, which is its failure-first proof.
- **Split:** `types.hpp` was at 500 lines; the event structs moved whole to `event_types.hpp`
  (no rebuild change measured: `types.hpp` includes it).
- **IS9 (callgrind, Release, `multra-engine-benchmark 12 300`, seed 42):** `shared` 909.8 M →
  914.0 M instructions (+0.46 %, the larger `PendingVehicle`), same 553 trips. `perInput` against
  `shared` per vehicle-tick: 2408 vs 2419 (−0.4 %); the two runs are different traffic (551 and 553
  trips), so the whole-run totals are not compared. The benchmark takes `shared|perInput`.
- **Verified:** Linux, GCC 13.3: headless ctest 208/208 and `check`; `multra-cli 42`, its event
  file and a four-leg 42–45 batch CSV byte-identical to the base commit. Desktop not built.
- **CI:** `azure.archive.ubuntu.com` stopped resolving on the Linux runners; the apt step hung
  ~47 min and failed twice. It now rewrites that mirror to `archive.ubuntu.com`, retries, and
  times out at 15 min (`native.yml`). Windows jobs passed both times.

## 2026-11-11 — The owner's answers to the open questions (D505)

- **Owner:** asked for a recommendation on each open question, then took all of them. Documents
  only; no code, example or baseline changed, and no new work was started.
- **M6 benchmark sources (was B9):** Q4 (d); S1-a, S1-d second; S2-b below saturation, S2-a
  across it, S2-c first; U-a after U-c, U-e booked for R7. Sources only: no gate closes and the
  tolerance is still written before any comparable run ([ROADMAP](ROADMAP.md) M6.0).
- **Random streams (was B44):** `perInput` for new documents at a schema step, traits re-keyed by
  input, paired comparison as a later slice, before the M6 runs
  ([INPUT_STREAMS](reference/INPUT_STREAMS.md) §7).
- **Micro's M6 gate (was B43):** may close before M10 and M11.
- **Approved, not started:** the D500 example on the pocket template; `EditorWindow` split in
  small steps. **Waits:** `src/project/`. **Not wanted:** a start page; tool names at 1366 px.
- **Still the owner's:** B45 on Windows, and requiring `Native C++` on GitHub.
- **Verified:** Linux: headless `check`.

## 2026-11-11 — File → Open recent (D504)

- **Owner:** asked for the rest of the UI review's queue: recent files and a start page.
- **D504:** the last eight projects opened or saved, newest first, in `recent-files.json` in the
  application's local data folder (as `vehicle-colors.json` is), never in a project. Remembered
  on *Open*, on a save (`saveFile`, so an automation save too), on `--scenario` and when opened
  from the menu; not an example (it opens untitled) and not a recovery draft. A file that is gone
  is listed disabled; a damaged list reads as empty. An empty drawing's status line names the
  menu once the list holds a file (`recent_files.*`, free functions; no new window member).
- **Not built: the start page.** A panel over the empty canvas would sit where a drawing starts;
  its look is the owner's choice and is an Ask in NEXT. Both things it would offer are in File.
- **Tests:** new `recent-files-ui` (failed before the menu existed): order, no repeats, the cap,
  a missing file, an example leaves the list alone, a second window reads it, Thai, Clear list,
  a damaged file.
- **Verified:** Linux, GCC 13.3: desktop ctest and headless `check`. The folder on Windows and the
  menu's look there are the owner's (B45). Images (composited from the two menus):
  [en](images/open-recent-d504/open-recent-en.png), [th](images/open-recent-d504/open-recent-th.png).

## 2026-11-11 — The palette names its tools on a 1440 px window (D503)

- **Owner:** chose this from the UI review's remaining items: at 1440 × 900 the palette was 16
  icons with no names.
- **Cause:** the palette went icons-only automatically below 1500 px of window width, so a
  maximised window on a 1366 or 1440 px laptop never showed a tool's name.
- **D503:** the threshold is 1440 px, one constant (`namedPaletteMinWindowWidth`) in place of
  three literals. A person's own toggle still stops the automatic switch.
- **Why not lower (the plan said 1280):** with names, a 1366 px window leaves the canvas 810 px,
  and `ui-audit-layout` (D432's audit) requires 900 there; at 1280 that row and
  `properties-layout-ui` failed. They were left as they are and the threshold raised instead.
  At 1440 the canvas is 884 px with names (1066 with icons). 1366 px windows keep the rail.
- **Tests:** new `palette-width-ui` (failed at the old threshold: "Wrong palette view at 1440 px");
  `workspace-ui` reads the constant instead of the literal 1500.
- **Verified:** Linux, GCC 13.3, desktop ctest 318/318 and headless `check`. Thai captions fit at
  1440 ([en](images/palette-d503/palette-1440-en.png), [th](images/palette-d503/palette-1440-th.png)).
  Windows and high-DPI are the owner's (B45).

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
