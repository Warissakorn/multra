# PROGRESS archive — 2026-11-21

Whole entries moved from the live PROGRESS log.

## 2026-11-24 — CC1 curve slice 1: codec and schema 86 (D646)

- **Owner:** "ทำ CC1 เซสชันแรกเลย"; the plan approved.
- **Codec:** `W99Parameters::cc1Curve` (`W99Cc1Point{value, share}`); a w99 behaviour holds `cc1` or `cc1Curve`,
  never both or neither (`INVALID_BEHAVIOUR_PARAMETER` at `.cc1`); each point must be an object of exactly two
  numbers; the curve is written in `cc1`'s place. Schema 86 (`kLatestSchema` 85 → 86) only when an owned w99
  behaviour has a curve; below it `EDIT_UNSUPPORTED_FIELD`; fixed files stay at 85. The composed schema's W99 row
  pairs `cc1`/`cc1Curve` with D566's `unless`, and `w99Cc1Point` is closed. `w99ParameterIssues` checks the
  points (two or more, values ≥ 0 increasing, shares in [0, 1] not falling, 0 and 1 at the ends).
- **Interim guards:** `validateScenario` refuses any curved behaviour (`UNSUPPORTED_BEHAVIOUR_MODEL_RUN` at
  `.cc1Curve`), one `runDiagnostics` row; the behaviour dialog declines to open one (en, th).
- **Rows:** `tests/w99_cc1_codec_tests.cpp` (group `w99cc1codec`, BA233, 5 tests); labelled schema cases (curve only,
  both, a point's extra key, missing share, text value; the curve on other models); `behaviour-library-ui` checks
  the decline.
- **Verified (Linux only, desktop preset, GCC 13.3):** `check` passes, `ctest` 373/373, `schema-labels` 144,220
  labels and 0 disagreements; the build without the PCH compiles every unit; `multra-cli 42` byte-identical.
  Mutations bite: the "both" refusal, the schema floor, the decline, the Run refusal.

## 2026-11-24 — F7 on Windows; the W99 dialog names an out-of-range key first (D643)

- **Owner (F7, package #145, verbatim in [OWNER_SITTING_WINDOWS](../plans/OWNER_SITTING_WINDOWS.md)):** empty fields,
  help, the empty-key refusal and the fit pass in Thai and English at 100/150 %; with `cc0 = 0`, Confirm showed no
  range message and moved to the next field.
- **Cause:** the dialog read keys until the first empty one and ran the range check only when all were filled, so
  a value just typed was never reported while a later key was empty (W74 had the same order). The range message
  also said "w74".
- **Fix (D643):** `readKeys` reads every key (empty ones as NaN), runs the model's own check once, and names and
  focuses the first key in order that is empty or out of range; the message is model-neutral (en, th) and ends
  with the key. `behaviour-library-ui` checks `cc0 = 0` with later keys empty (error "· cc0", focus on cc0), then
  the next empty key; it fails on the old order.
- **Verified (Linux only):** `check` passes (372/372); the build without the PCH compiles every unit. A new
  package for the owner's recheck.
- **Owner, same day:** asked how other programs treat heavy vehicles' acceleration; after the comparison (Vissim's
  per-type curves, SUMO's per-type `accel`, Aimsun's per-type maxima), "ตกลง" to not halving: D644, W99 §1.
- **Owner, same day:** "CC1 ทำให้เลือกได้ดีหรือไม่ โดยให้ค่าเริ่มต้น เป็นการสุ่ม", then "ok แบบกราฟสะสม เริ่มสัญญาเลย":
  W99 §5a contracts CC1 as a per-driver cumulative curve (D645, rows BA233–BA237); nothing implemented.

## 2026-11-24 — W99 slice 5: composition, the Run refusal lifted (D642)

- **Owner:** "ทำต่อ" after the trait slice; the plan approved.
- **Composition:** `follow` gains `previousAcceleration` (every caller passes the vehicle's published
  acceleration; insertion's first-step check passes 0) and dispatches a `w99` behaviour to `w99Acceleration` with
  `zW`, returning no W74 state. `standstillGap` is `cc0`, `desiredGap` `cc0 + cc1·v`; the prototype's standing-body
  floor skips w99. `retune` compares w99 presence (traits are drawn at generation); the lab's prototype fallback
  skips w99.
- **Run:** D639's refusal is removed from `validateScenario` and `runDiagnostics` (`w99RoadRunIssues` deleted). The
  dialog's W99 help drops its "Run refuses" sentence (en, th).
- **Rows:** `tests/w99_run_tests.cpp` (group `w99run`, 9 tests): BA226 (standstill creep serves a Stop line;
  stops within `SDXc` of a red head), BA230 (hard cap, D105, source D108, queue discharge, an open platoon without a
  clamp, multi-anticipation, reaction time, courtesy, replay), BA232 (prototype → w99 → w74 → w99 with `aPrev`). The
  codec test now runs a used w99 behaviour; the lab test opens a lab beside a w99 car and Apply is refused.
  Mutations bite: no creep, `aPrev` 0, `standstillGap` without w99, the dispatch skipped
  ([evidence](../evidence/w99-composition.md)). `multra-cli 42` summary and events byte-identical.
- **Verified (Linux only, desktop preset, GCC 13.3):** `check` passes, `ctest` 372/372; the build without the PCH
  compiles every unit. No Windows run; package #145 predates this.

## 2026-11-24 — W99 slice 4: trait `zW` (D641)

- **Owner:** "ทำ 1 เลย" (W99 next); the plan approved.
- **Trait:** `w99Trait(seed, id)` is W74 §5's hash at index 15, through a factored `w74Uniform` that `w74Traits`
  now calls (its values unchanged). `PendingVehicle::w99Trait` is set at generation from `traitIdentity` when
  `ScenarioIndex::w99` (any `w99` behaviour); it never touches `randomState`. Checkpoints write `w99Trait` only
  when present. Nothing reads it until composition.
- **Rows:** `tests/w99_traits_tests.cpp` (group `w99traits`, BA229, 6 tests): Python golden values, independence
  from W74's draws 0–14, unchanged draws and events beside a prototype-only and a W74-only run, a copied state,
  `perInput` identity, checkpoints. `w99codec`'s unused-behaviour run strips the trait and asserts it is present.
  Mutations bite: index 14, the trait on every scenario, drawn from the stream, checkpoint key always, `>> 12`.
- **Two compilers:** 1,000 values from g++ 13.3 and clang++ 18.1 identical (`d206e1ac8a8750ac`) and equal to Python
  ([evidence](../evidence/w99-traits.md)). `multra-cli 42` summary and events byte-identical.
- **Verified (Linux only, desktop preset, GCC 13.3):** `check` passes, `ctest` 371/371; the build without the PCH
  compiles every unit. No Windows run.

## 2026-11-23 — W99 slice 3: the W99 dialog (D640)

- **Owner:** "พัฒนาต่อ" after slice 2; the plan (dialog plus the lab guard) approved.
- **Dialog:** the decline is gone. *W99 (Wiedemann 99)* is the third model item: a help note, one empty text field per
  `w99ParameterKeys()` entry (filled only from the behaviour's own values), and the cooperative and amber options shared
  with W74. Confirm names the first empty key (`editorBehaviourKeyMissing`, renamed from the W74-only key), then shows
  `w99ParameterIssues`' first issue; the result is rebuilt from the id, so switching model drops the other's keys. W74
  and W99 share one key-field builder and one reader.
- **Prototype source:** the library's first behaviour of neither model (it was "not W74", which could pick a w99 one).
- **Lab (a gap from D639):** Apply was offered for a car on a `w99` behaviour and wrote prototype fields the writer
  then dropped. Both guards now refuse `w99` as `w74`; a new lab does not open on one, since Run refuses it.
- **Rows:** `behaviour-library-ui` edits a w99 behaviour (out-of-range refused, one key and the name saved, schema 85),
  switches W99 → Prototype (library values, also with the w99 entry first), Prototype → W99 (fields empty, an empty
  one refused), Cancel, and the Thai item and help. `behaviour-lab-ui` checks the refused Apply. Mutations bite: the
  lab guard, the w99 reset on the prototype branch, the starting model index and the prototype source.
- **Verified (Linux only, desktop preset, GCC 13.3):** `check` passes, `ctest` 370/370. No Windows run; the owner's
  look is [F7](../plans/OWNER_SITTING_WINDOWS.md).

## 2026-11-23 — W99 slice 2: codec, schema 85 and the Run refusal (D639)

- **Owner:** "ทำงานถัดไป" after slice 1. Contract §10 grouped codec and dialog; W74 split them (D136, D139), so this
  slice is the codec and the dialog is next (recorded in D639).
- **Codec:** `DriverBehaviour::w99` (last field); `parseBehaviour` reads `model: "w99"` and the ten cc keys through one
  generic read/refuse pair shared with W74; the writer and `addBehaviourLibraryJson` tag and write it; schema 85
  (`w99ModelVersion`, `kLatestSchema` 84 → 85) is selected only when an owned behaviour is `w99`; `rejectW99Before85`.
  A cc key on a prototype or W74 behaviour is refused at every version (it was accepted and ignored only on an open
  pre-21 row).
- **Composed schema:** a `w99Behaviour` row from `behaviourRow(Model::w99)`; from 85 the W74 branch's `else` is a W99
  dispatch; the model enum gains `w99`; cc keys are `false` on the other rows at every version.
- **Run:** `validateScenario` refuses a `w99` behaviour a vehicle type or a compiled road assignment names
  (`UNSUPPORTED_BEHAVIOUR_MODEL_RUN`; authoring drops `UNSUPPORTED_*`, so save and load accept it);
  `w99RoadRunIssues` adds the road case to `runDiagnostics`, once.
- **Dialog:** declines to open a `w99` behaviour with a message (en, th), so nothing rewrites its keys.
- **Rows:** `tests/w99_codec_tests.cpp` (group `w99codec`, BA228, 6 tests); `behaviour_schema_cases.hpp` gains the
  W99 model; `behaviour_schema_tests.cpp` checks the nested dispatch; `behaviour-library-ui` checks the decline and
  fails without it.
- **Verified (Linux only, desktop preset, GCC 13.3):** full build, `ctest` 370/370, `check` passes (`schema-labels`
  141,694 labels, 0 disagreements). No Windows run.

## 2026-11-23 — W99 slice 1: the pure function (D638)

- **Owner:** "ok" to the W99 contract (D637).
- **Code:** `src/core/w99_types.hpp` (`W99Parameters`, CC0–CC9) and `src/core/w99.*`: `w99Thresholds`, `w99Acceleration`
  (W99 §3–§4, the type bounds and D105, as W74's pure function), `w99ParameterKeys()` and `w99ParameterIssues` (§5).
  Nothing calls it yet: no codec, trait draw or composition; `types.hpp` untouched.
- **Rows:** `tests/w99_tests.cpp`, group `w99`. BA224: §8's ten cases and the threshold cases to 10⁻¹², plus the free
  rule, the type bounds and D105. BA225: both sides of all five equalities, with binary-exact values. BA227: a
  18,144-point grid over four parameter sets (every range edge included); closing only beyond `SDXc`, its denominator
  ≤ −0.1, every result finite. Shown to bite: dropping the standstill creep, `≤ SDXc` as `<`, the `vSlow` floor or the
  linear CC8 → CC9 rule each fails one or two tests.
- **Verified (Linux only, desktop preset, GCC 13.3):** full build, `ctest` 369/369 (the new `w99` group), `check` passes.

## 2026-11-23 — The W99 contract, before code (D637, M3.3.3b)

- **Owner:** chose "W99 contract (M3.3.3b)" from the offered picks.
- **Sources read 2026-11-23:** PTV's public Wiedemann 99 parameter page (CC0–CC9 meanings and units, the safety
  distance, the free acceleration's CC8→CC9 rule and its `z` term); `glgh/w99-demo` `js/model.js` (MIT, commit
  `80eb392`), the threshold set and decision tree SUMO's `MSCFModel_W99` credits as its source; SUMO for one rule
  (the slower-speed floor). Nothing copied: the contract is our formulation, as W74's was.
- **[W99](../reference/W99.md):** inputs and W99's sign of `dv`; six thresholds; four regimes with equality sides; ten
  keys `cc0`–`cc9` with ranges that make the closing division exact; one hashed trait `zW` (index 15) and the existing
  `driverFactor`; no new state (`aPrev` is the published acceleration); composition through `follow` and the two
  accessors; persistence and dialog for the codec slice; a five-slice delivery order.
- **Seven deviations from the demo, each with its reason (§4):** the demo's free acceleration is dimensionally wrong
  (35 m/s² at 80 km/h), its `−10` floors are code constants, its `vDes − v` cap mixes units, one branch is dead code;
  a negative slower speed is floored (SUMO); a driver at rest in the following band creeps (as W74 §7), or a Stop line
  is never served.
- **Ten hand-checked cases** (§8), checked by hand and by a scratch transcription (not committed). Rows BA224–BA232
  written failure-first in the delivery plan; all pending.
- **Verified:** docs check passes. No code changed, so no build or test was needed.

## 2026-11-23 — `procedures` in the composed project schema (D636)

- **Owner:** "ok" to D573–D574, which released the briefed slice ([PROCEDURE_SCHEMA_SLICE](../plans/PROCEDURE_SCHEMA_SLICE.md)).
- **Pins first:** `tests/procedure_schema_tests.cpp` (PRS1) passed on the unchanged reader, project 121/121: both floors
  (feedback's after parsing), kind → active → keys, exclusive assignment, ids, feedback bounds, a 12-issue order.
- **Schema:** root `procedures` false below 73; closed procedure rows; four step rows chosen by `kind`; assignment's
  `period`/`macroRun` exclusive via `unless`; feedback false below 75, one row with a constantWeight/msa weight condition.
  Coverage `-procedures`. The emitted schema is unchanged outside `procedures` (per-version root and every old `$def`).
- **One key source:** `procedure_codec.cpp` reads keys with `onlyKnownKeys`; the Macro sets' and procedures' tables
  moved whole to `src/project/macro_set_fields.cpp` (`network_fields.cpp` 472 → 427 lines). Codes, paths, order, writer
  bytes and save floors unchanged; `document.cpp` names 73/75 with the new constants.
- **Labels:** `tests/procedure_schema_cases.hpp` in `schema-labels`: 138,710 labels and 20 projects, zero
  disagreements. By hand (temporary, deleted): writer output at 73/75 and 20 probes with jsonschema 4.26.0; zero
  schema-refused/codec-accepted.
- **Owner:** "ok" to D636 and its review fixes (2026-11-23).
- **Review fixes (owner: "แก้ตามแนะนำ"):** feedback's `weight` and `tolerance` have their own kinds rather than
  deterrence's `intrazonalShare` and trip rates' `rate`; PRS3's probe tally corrected to 5 / 2 / 13; the D636 constants'
  comment moved off D573's. The emitted schema is byte-identical to the first D636 commit.
- **Verified (Linux only, desktop preset, GCC 13.3):** full build, `ctest` 368/368, `check` passes. No-PCH and
  Windows not run.

## 2026-11-23 — Insertion's bucket rebuild after the last arrival dropped (D635)

- **User:** "พัฒนาต่อ"; no owner item was free, so the session took NEXT's untried engine lever: insertion rebuilt every
  span bucket after each inserted arrival (`insertion.cpp`), ENGINEERING_NOTES 2026-11-23.
- **Change:** the buckets are rebuilt only before a later candidate's leader search; after the last arrival they are
  left unbuilt and `Snapshot::built` is cleared, as `stepSimulation` already re-resolves a grown fleet (D584's test).
- **Measured** (callgrind, GCC 13.3, Release `-g1`, seed 42): benchmark `48 900` 9,623.9 → 9,546.0 M Ir (−0.81 %);
  `stepSimulation` on the study −0.05 %, M2.6 −0.09 %. Event hashes (FNV-1a of every `eventJson`, every shipped
  project that runs and the study, seeds 42 and 43) identical. The scratch harness was not committed.
- **Verified (Linux only, desktop preset, GCC 13.3):** full build, `ctest` 368/368, `check` passes. No Windows run.

## 2026-11-23 — The four parallel slices merged (D626 kept)

- **User:** "ตามแนะนำ merge ทั้งหมดเลย", after the review: D626 kept (benchmark −5.9 %, study +0.15 %, M2.6 +0.25 %,
  inside D598's 0.5 % for the same trade, though outside slice A's brief).
- Merged D, C, A, B in that order. Conflicts only in PROGRESS, NEXT's slice lines and the RECORD/index tails
  (rows ordered by ID). Three entries moved whole to [PROGRESS-LOG-2026-11-21](../archive/PROGRESS-LOG-2026-11-21.md).
- **Review before merging (Linux):** B's configure output equals the base's (368 tests and their commands, the
  target list and `compile_commands.json`); C's `results-tabs-ui` re-run with screenshots; D's moved prose
  spot-checked against its contracts. Merged tree: see the checks recorded with the merge commit.

## 2026-11-23 — Slice D: ARCHITECTURE's *Editor boundary* moved into the contracts

- **Brief:** [PARALLEL_SESSIONS](../plans/PARALLEL_SESSIONS.md) D. ARCHITECTURE's 130 lines of per-decision notes now
  sit with the behaviour they describe; ARCHITECTURE keeps 22 lines: `ProjectDocument` owns the document, commands
  commit through `History`, project never imports commands or Qt, the canvas/window split, and one link per contract.
- Merged, not copied (rule 3): sentences a contract already said were dropped, only the code seams it lacked were added.
  NETWORK_EDITOR was at 500 lines, so its *Geometry and end handles* and *Attachment stations* sections moved whole to
  NETWORK_EDITOR_CONNECTORS (stubs left, as for the earlier two); the moved text's stale "square fallback" now reads as
  D80's P1–P4 mouth. No link pointed into the old section (`grep 'ARCHITECTURE.md#'`: none).

| Old paragraph (ARCHITECTURE lines) | Now in |
|---|---|
| Automation server, GUI-thread dispatch, patches through History (152–158) | DESKTOP_AUTOMATION *Request and response* |
| `ProjectDocument`, `History` commit, project never imports commands (160–165, 170) | stays in ARCHITECTURE |
| `History::states`, `restore` (163–165) | EDITOR_WORKFLOW *History* |
| Keyboard nudge command, level-filter callback, gesture cancel (166–169) | EDITOR_WORKFLOW *Keyboard moves* (cancel already there) |
| `rotation.cpp`, `rotateObjects` (172–176) | EDITOR_WORKFLOW *Rotate a selection* |
| `road_crossbar.*`, route hover, `routeDestinations` (178–182) | EDITOR_WORKFLOW *Draw a route* (destinations already there) |
| `editor_interaction`, `ReflowLabel` (183–184) | EDITOR_WORKFLOW *Safe form and document refreshes* |
| `EditorCanvas`/`EditorWindow` (186–190) | stays in ARCHITECTURE; immutable background bytes already in NETWORK_EDITOR |
| Connector commands, `connectorCurve`, `connectorPaths`, path IDs (191–204) | NETWORK_EDITOR_CONNECTORS *Code seams* |
| `AuthoringDefinition`, `compileDocument`, `RunSnapshot` (206–211) | NETWORK_EDITOR *Check and Run* |
| `editor_storage`, UUID recovery, `QLockFile`, schema 1–4 reads (213–217, 239) | NETWORK_EDITOR *Save, recovery and formats* |
| `laneOffset`, `laneBlend`, stations, `laneAttachment`, `runtimeSections`, widths (218–238) | NETWORK_EDITOR_CONNECTORS *Code seams*; copies in EDITOR_WORKFLOW *Selection* |
| Markings, Link geometry commands, advisories (241–247) | AUTHORING_EXTENSIONS (schema 7 rejection already in NETWORK_EDITOR) |
| `directionAlong`, D80 axis and surface (249–255) | CONNECTOR_FOUR_POINT_MOUTH *Axis before edges* |
| D109/D115/D117/D118 conflict groups, overlaps, bands, polygon math, D114 adapters (256–273) | M3_CONTRACT §1 |
| Surface mouths, `selfIntersecting`, waiting bars (275–280) | already in CONNECTOR_FOUR_POINT_MOUTH; links kept in ARCHITECTURE's table |

- No decision numbers used (638–641 left unused). Docs only; headless preset built for the tools;
  `multra-check-docs .` and `multra-check-file-sizes .` pass.

## 2026-11-23 — Results tabs at 1024 in Thai; the clamp note's marker (slice C, D634)

- **Strip:** measured first, on Linux offscreen at 1024 × 768 with every tab of each level forced visible: Thai Micro
  605 px, Macro 572 px, English 673 and 483, of a 1008 px Results page. D497's one-level strip had already fixed the
  scroll NEXT carried from D490's review, so no label changed and no control was added (D634).
- **Clamp note:** now opens "Not yet validated." / "ยังไม่ผ่านการตรวจสอบความถูกต้อง", as every other Results note does
  (rule 4); "Safety clamp" stays untranslated.
- **Test:** `results-tabs-ui` replaces its conditional strip check with one at 1024 × 768 in Thai and English that first
  asserts the size and language took effect, then per level: no scroll button, no tab past the page, no elided label;
  and the clamp note's statement in both locales (a third locale fails it until added). Proved to bite: the old
  locales fail on the note; a 1614 px Thai Micro label fails on the strip.
- **Verified (Linux only):** desktop build, full `ctest --preset desktop` and `check`. Screenshots taken for the check
  only, not committed (no contract cites them). The Windows Thai look is the owner's.

## 2026-11-23 — Slice A: the kept leader order measured and not shipped (D627); long routes bisect (D626)

- **Brief:** [PARALLEL_SESSIONS](../plans/PARALLEL_SESSIONS.md) A. Callgrind, GCC 13.3, Release `-g1`, seed 42; baseline study
  34,553.6 M Ir, M2.6 4,031.1 M, `multra-engine-benchmark 48 900` 10,226.8 M. Event hashes: every event (`eventJson`) of
  every shipped project that runs and the study, seeds 42 and 43, identical after every variant.
- **D627, not shipped:** buckets ordered by (rear, span index), seeded from the fleet's id order and repaired by
  insertion, the scan stopping at the first pass. The best variant cut the study 8.8 % but cost the benchmark 0.6 %
  (0.8 % on top of D626), over the 0.5 % bar; code reverted, five variants' numbers in ENGINEERING_NOTES.
- **D626, shipped:** `closestVehicle` bisects routes of more than 8 parts to the first part not behind the vehicle.
  The walk over passed parts was 5.7 % of the benchmark. Benchmark −5.9 %, study +0.15 %, M2.6 +0.25 % (D598's
  bar). New test in `straddlerleader` compares it with the full scan at 501 positions.
- **Verified (Linux only, desktop preset, GCC 13.3):** full build, `ctest` 368/368, `check` passes. No Windows run.

## 2026-11-23 — `CMakeLists.txt` split by responsibility (D630, parallel slice B)

- **Brief:** [PARALLEL_SESSIONS](../plans/PARALLEL_SESSIONS.md) B. The root file was at 498 of 500 lines.
- **Moved whole, in place and order:** tool executables → `cmake/tools.cmake`; check tools and `check` →
  `cmake/check_target.cmake`; `multra-tests` and its groups → `cmake/model_tests.cmake`; tool, CLI and script
  tests → `cmake/cli_tests.cmake`. Root 498 → 194 lines; the map is in
  [BUILDING](../BUILDING.md#where-the-build-rules-live).
- **No behaviour change**, on `desktop` (Qt 6) and `headless`: sorted `ctest -N`, `--target help`, sorted
  `compile_commands.json`, `cmake_install.cmake` and sorted `ninja -t commands` identical before and after;
  `CTestTestfile.cmake` differs only in each test's backtrace (file and line).
- Checks (Linux only, GCC 13.3): full build, `ctest` and `check` pass on `headless` (251/251) and `desktop` (368/368).

## 2026-11-23 — Four slices briefed for parallel sessions

- **User:** "พัฒนาต่อ โดยเปิด Branch และเขียน prompt เพื่อให้ Agent ทำงาน พิจารณาการทำหลายงานพร้อมกันด้วย".
- Of NEXT's open items, four need no owner choice and touch disjoint files: the leader order kept across
  ticks (A), the `CMakeLists.txt` split (B), the Results tab strip at 1024 in Thai and the clamp note (C),
  and ARCHITECTURE's *Editor boundary* prose into the contracts (D). Briefs:
  [PARALLEL_SESSIONS](../plans/PARALLEL_SESSIONS.md); one branch and one cloud session each.
- To keep four merges trivial: decision numbers 626–641 and BA216–BA223 reserved per slice; only B edits
  `CMakeLists.txt`; the four oldest PROGRESS entries moved whole to
  [PROGRESS-LOG-2026-11-21](../archive/PROGRESS-LOG-2026-11-21.md) to leave room for four new ones.
- Docs only; `multra-check-docs` and `multra-check-file-sizes` pass.

## 2026-11-22 — Windows items 51 and 52 passed; conflict-area display as Vissim (D620)

- **Owner (verbatim):** "4. 51 ok"; on item 52: "5. สมจริง แต่สามารถใช้ตาม Aimsun next ได้อย่างถูกต้องตามกฎหมายใช่หรือไม่".
  Findings 1–3 with two pictures: arrows off their lanes at multi-lane crossings; new junctions already green and red;
  "3. อธิบาย conflict area ที่ยังไม่สามารถปรับได้". Answers: "พิมผิด ต้องการให้แสดงเส้นลูกษรตรงกลางเฉพาะตอนเลือก Conflict area นั้น";
  "เหลืองเหมือนกันหมด".
- **D620:** centre arrows on the lane's own centre and only on the selected area; unauthored areas yellow (automatic merges
  included), authored undecided red on both sides. Explained in chat: the dark brown areas are branching (a diverge from one
  lane), read-only because vehicles keep their order there; the licensing answer (methods from public manuals, reimplemented;
  no code or text copied; not legal advice).
- **Verified (Linux, offscreen):** desktop 367/367 (priority-canvas checks red-red and no arrows off the selected area); the
  new geometry row puts two lanes' centres 3.5 m apart; docs and sizes pass.

## 2026-11-21 — G7 slice 2: the gap reduction in the dialogs, and the waiting-acceptance lab (D619)

- **Owner:** "ทำต่อ".
- **D619:** the area dialog and each marker in the rule dialog have *Gap reduction while waiting (Aimsun)*. The
  [lab](../../data/projects/README.md) `yield-impatience-lab` ships for OWNER_SITTING_WINDOWS item 52. Seed 42: the minor
  delays fall from 60.9 to 24.7 s and 38.7 to 12.8 s, the major ones rise 0.14–0.29 s; no clamp, no overlap.
- **Verified (Linux, offscreen):** desktop 367/367, headless 250/250 (YI1–YI12 pass), docs and sizes pass; `multra-cli 42`
  keeps 45 / 31.75870154816726 / 1. The no-PCH check and Windows wait for the package build.

## 2026-11-21 — G7 slice 1: a waiting driver accepts less, Aimsun's gap reduction and SUMO's impatience (D618)

- **Owner (verbatim):** "G7 ทำแบบ Aimsun"; on the follow-on, "ในสถานะการที่รถทางรองออกแล้วรถทางหลักหยุด แล้วรถทางรองคันถัดไป
  ออกต่อกันกับคนแรก เนื่องจากมีโอกาสจากรถทางหลักหยุด"; "ทบทวนความเต็มใจให้รถทางหลักชะรอของ sumo"; "ทำทั้งสองส่วนตามแนะนำ".
- **Read first** (hosts allowed for this): Vissim 2025's stop-sign and conflict-area help, SUMO's docs and `MSLink.cpp`, Aimsun
  Next's manual. G8 needs nothing (all three hold a regular stop for a step or stop fully); Vissim has no G7 key.
- **D618, [YIELD_IMPATIENCE](../reference/YIELD_IMPATIENCE.md):** schema 84; Aimsun's linear gap reduction over a per-vehicle
  hashed yield time, set per rule or marker; SUMO's impatience at conflict areas only, so a standing major vehicle lets the
  next minor vehicle follow. Off by default; `multra-cli 42` keeps 45 / 31.75870154816726 / 1.
- **Verified (Linux):** `yield_impatience` YI1–YI9 pass; `ctest --preset headless` 250/250; desktop: all but `results-tabs-ui`
  passed in the parallel run, which timed out under load and passes alone (34 s); docs and sizes pass.
- **Next:** slice 2, the place keys in the dialogs and a lab scene for the owner's look.

## 2026-11-21 — PRIORITY_RULES slice 3: display, table rows and the unreached-marker row (D617)

- **Owner:** "ทำส่วนที่ 3 เลย".
- **D617:** each marker's headway shows as a faint green band upstream; a picked rule (line, marker or row) links its
  line to each marker. The Conflict areas tab lists the rules after the areas; the row's dialog edits the name, the
  line's Stop/Yield and classes, and each marker's gapTime, headway, maxSpeed (0 = no limit) and classes in one Undo
  step (`setPriorityRule`); Delete keeps the line (`removePriorityRule`). A marker no route reaches is an advisory
  (`PRIORITY_MARKER_UNREACHED`). Classes are typed as ids for now.
- **Fixed on the way:** the table's group loop ran to the new row count and read past the groups (caught by
  `stop-line-ui` and `priority-ui` as a SEGFAULT before commit).
- **Rows:** `priority_marker` (dialog command, refusals, Delete, advisory), `priority-ui` (rows, canvas pick, dialog,
  Undo, Delete on the lab), `priority-canvas` unchanged.
- **Checks:** `multra-cli 42` keeps 45 / 31.75870154816726 / 1; `check` desktop 366/366, headless 249/249; 679 units
  build without the PCH. Windows: Package binaries run started for the owner's item 51.

## 2026-11-21 — The priority-rule lab for the owner's Windows look (item 51)

- **Owner (verbatim):** "สร้าง Project สำหรับทดสอบเช็ค บน Window", then "สร้างเสร็จแล้ว pack binary เลย".
- **`data/projects/priority-rule-lab.traffic.json`** (builder `tools/priority_rule_network.hpp`, regenerate with
  `multra-priority-rule-fixture`; *Open example → Priority rule lab*): the T-junction's roads and demand, left-hand,
  with two priority rules placed by the Stop line tool's commands and no conflict area. The right turn yields at its
  line to markers on both streams; the left turn has a Stop with no area. Owner steps: OWNER_SITTING_WINDOWS item 51.
- **Rows:** `priority_marker` (file is the builder's output; rules compile, Stop on the left turn, every derived
  merge rule has the turn yielding; a seed-42 run holds right-turners and serves every left-turner's Stop; same events).
- **Reading, no verdict (rule 4):** seeds 42–44, mean delay right turn 29.6 s, left turn (Stop) 44.4 s, major 4.0 s;
  the Yield T-junction with conflict areas gives 17.5 s and 9.5 s.
- **Checks:** `check` desktop 366/366, headless 249/249 (`studyperiods` SP1 counts ten shipped projects).

## 2026-11-21 — PRIORITY_RULES slice 2: the Stop line tool adds conflict markers (D616)

- **Owner:** "ทำงานถัดไป".
- **D616:** the Stop line tool places a line with no area downstream (amends D595's refusal); with a line picked,
  Ctrl+right-click on another lane adds a marker at 3.0 s / 5.0 m, starting the rule; a marker drags along its path
  and Delete removes it, the rule with its last marker; one Undo step each (`priority_marker_commands.cpp`).
  Markers follow their roads (split, delete, copy, study-area cut) and show as green bars (`canvas_markers.cpp`).
- **Rows:** `priority_marker` PR7, PR8 (5 tests); `stop-line-ui` extended (place alone, add, drag, Delete, Undo);
  `stoplines` SL4 updated for the lone line.
- **Checks:** `multra-cli 42` keeps 45 / 31.75870154816726 / 1; `check` desktop 366/366, headless 249/249; 677 units
  build without the PCH. Windows not run.

## 2026-11-21 — PRIORITY_RULES slice 1b: a Stop on a priority rule's line (D615)

- **Owner:** "ทำงานถัดไป" (the slice-1 remainder, D606 Q4 (a)).
- **D615:** a Stop control on a marker rule's waiting line makes every marker's runtime rule a Stop
  (`PriorityRule::stop`); the engine holds the vehicle until its stop service records a full stop at that line,
  then the gap test applies, as a Stop zone does. Such a control may name no area (no schema step). Yield is the
  rule alone.
- **Rows:** `priority_rule` PR14–PR16 and PR10 with a Stop (13 tests). Without the hold PR14 fails.
- **Checks:** `multra-cli 42` keeps 45 / 31.75870154816726 / 1; `check` desktop 365/365, headless 248/248. Windows not run.

## 2026-11-22 — Three optimisation passes: the test loop, the Debug build, the live docs (D621)

- **Owner:** /optimize-program three times (the third with "เอกสาร"); each plan answered "แก้ตามแนะนำ". The owner did not
  revert the W74 split of `types.hpp` (line limit only; NEXT keeps the deferred rebuild split).
- **Pass 1 (after G7):** the schema-label dump streams case by case, output byte-identical (dump 38.3 → 25.3 s, peak memory
  2,217 → 603 MB, `schema-labels` 84.2 → 71.5 s); merge groups out of `right_of_way.cpp` (500 → 447 lines); comments and the
  D592/D610 amendment notes brought in line with D620; one misleading-indentation warning fixed. A pickling change for the
  Python half was measured and dropped (1.1 s of pickling; fork-shared validation was slower, 40.2 → 66.1 s).
- **Pass 2, D621:** GCC/Clang Debug builds compile with `-Og`: the desktop suite 331 s (313–336) → 138–143 s, `check`
  336–356 → 148 s, `multra-cli 42` byte-identical; `-DMULTRA_DEBUG_OG=OFF` restores `-O0`. The engine was not touched:
  Release runs 0.25 µs per vehicle-tick. Two more warnings fixed; `types.hpp` 497 → 471 (`w74_types.hpp`).
- **Pass 3 (docs):** NEXT's four stale statements corrected; finished-slice history moved verbatim to
  [NEXT-state-2026-11-22](NEXT-state-2026-11-22.md) and the `procedures` brief to
  [PROCEDURE_SCHEMA_SLICE](../plans/PROCEDURE_SCHEMA_SLICE.md) (NEXT ~7,100 → ~5,400 tokens); D400–D599 frozen in
  [RECORD-D400-D599](../decisions/RECORD-D400-D599.md) as D486 splits every 200 IDs (RECORD.md ~61,700 → ~5,900 tokens).
- **Verified (Linux, offscreen):** desktop `check` 367/367 after each pass; docs and sizes pass. Nothing visible changed,
  so there is no Windows look.

## 2026-11-22 — M3.2's diverge rows written: A64 fails, bodies overlap inside a split (D622)

- **Owner:** "พัฒนาต่อ"; the one NEXT item needing no owner was M3.2's diverge acceptance-fixture row (review O6/R7).
- **D622:** `tests/diverge_tests.cpp` (`diverge.*`), rows A62–A66 in M3_ACCEPTANCE: one lane splitting by two Connectors.
  A62 (branching only, nothing compiled), A63 (free flow: departure order kept, no clamp, no overlap), A65 (a branch
  blocked at the split holds everyone behind it, first-in first-out) and A66 (copied-state replay) pass. **A64 fails:**
  when a branch queue reaches back into the split, a car to the other branch loses its leader once the leader's rear
  leaves the shared lane and drives through it where the lanes still overlap (8 of 10 seeds, up to 1.69 m, no clamp).
  Bodies are checked as rectangles in the plane; station overlap alone over-reports (lanes 11.76 m, bodies about 8 m).
- **Not done:** the engine is unchanged; the remedy changes runs at every diverge, so it is the owner's choice
  (OWNER_SITTING item 53; recommended (a), sibling-branch leaders). [Evidence](../evidence/diverge-acceptance.md).
- **Verified (Linux, offscreen):** desktop ctest 368/368 (the new `diverge` group included); docs, file sizes pass.

## 2026-11-22 — Red-red branching enforced: A64 passes (D623)

- **Owner:** "อ่าน sumo ก่อน", then "ทำตามแนะนำ" on item 53: option (a), the extent being the branching area the editor
  draws. Read first: Vissim's manual (branching red-red, vehicles see each other) and SUMO's `MSLink.cpp` (same-source
  sibling lanes are foes, car-following until the foe's back passes the divergence point).
- **D623:** `compiledBranchings` (`src/model/network/branching_compile.cpp`) gives `Scenario::branchings` from the
  Connector pairs leaving one lane; `appendVehicleSpans` copies a body whose rear is inside its area onto the sibling
  (`own = false`). Rows A64, A67–A70 written first; all nine `diverge` rows pass, A64 with 0 overlaps on every seed.
- **Measured:** completed trips unchanged except the study (−1.5 %); clamps unchanged; `multra-cli 42` byte-identical;
  three movements beyond their noise, the T-junction's minor → eastbound 9.9 → 14.2 s (owner item 54). The first
  version compiled through `automaticConflicts` (+18.8 % on the benchmark); compiling only same-lane pairs fixed that:
  callgrind study +2.5 %, M2.6 +3.1 %, benchmark +2.0 %.
  [Evidence](../evidence/branching-leaders.md).
- **Verified (Linux, offscreen):** desktop ctest 368/368; docs, sizes, architecture pass; 682 units build without the PCH.

## 2026-11-23 — Windows package; branching stays read-only (D624)

- **Package:** *Package binaries* run #144 on this branch (`088955e`), both jobs green (their ctest passed on Linux and
  Windows); artifacts `multra-windows-x64` and `multra-linux-x86_64`, kept to 2026-12-07.
- **Owner, on the package:** asked whether branching should be adjustable like other conflict areas; after reading
  Vissim's manual ([review](../evidence/vissim-branching-review.md)): "คงไว้แบบนี้ (แนะนำ)". **D624:** branching stays derived
  and read-only; a visibility key waits for a study and a cited default (Vissim prints none).
- **Item 54 passed:** the owner, after watching the T-junction's one-lane minor approach on Windows: "สมจริง".
- Docs only; `multra-check-docs` passes.

## 2026-11-23 — M3.2 closed by the owner (D625)

- **Owner:** item 54 "สมจริง"; then, shown each gate piece with its evidence, "ตกลง" to closing M3.2.
- **D625:** ROADMAP, M3_ACCEPTANCE and NEXT say closed. The one Windows look with no verdict of its own (M3.2.4e merge
  mouth, M3.2.12 stop lines and tool) was added to M3.2.13 as item 4 rather than read into A2. Still open on their own
  gates: M3.2.10, M3.2.13, M3.2.8a.1, M3.4, M6; the not-yet-validated marker stays.
- Docs only; `multra-check-docs` passes.

## 2026-11-24 — CC1 curve slice 2: draw, rule and composition (D647)

- **Owner:** "ทำ CC1 เซสชันถัดไปเลย"; the plan approved.
- **Draw:** `w99Cc1Share = u(16)` per driver at generation when a w99 behaviour has a curve
  (`ScenarioIndex::w99Cc1Curve`, `PendingVehicle::w99Cc1Share`); a checkpoint key only when present.
- **Rule:** SPEED_DISTRIBUTION §3's inverse is now `cumulativeInverse`, shared by `speedAt` (unchanged to the bit)
  and `w99Cc1At`.
- **Composition:** `w99DriverParameters` gives `follow`, `desiredGap` and `stopLineReach` the driver's CC1;
  the four `stopLineReach` callers pass the vehicle's share; retune compares curve presence. D646's Run refusal
  is removed; the dialog's decline stays until BA237.
- **Rows:** `tests/w99_cc1_run_tests.cpp` (group `w99cc1run`, BA234–BA236, 6 tests); `w99cc1codec`'s refusal case
  now runs a curved behaviour. Mutations bite: index 15, the share always drawn, CC1 not replaced, `desiredGap`
  ignoring the share, the refusal kept ([evidence](../evidence/w99-cc1-draw.md)).
- **Verified (Linux only, desktop preset, GCC 13.3):** `check` passes, `ctest` 374/374; the build without the PCH
  compiles every unit; `multra-cli 42` summary and events byte-identical.

## 2026-11-24 — CC1 curve slice 3: the dialog (D648)

- **Owner:** "ทำ CC1 เซสชันถัดไปเลย"; the plan approved.
- **Dialog:** a *CC1* choice (*Fixed* / *Distribution (per driver)*, en and th); switching to W99 starts in
  Distribution with an empty table and `cc1` greyed; a curved behaviour opens with its points, a fixed one in
  Fixed. The table (`cc1` s, share %) has Add/Remove as the desired-speed table. Confirm names an empty table, a
  non-number cell or the validator's point and selects its row; Fixed is unchanged. D646's decline is gone.
- **Rows:** `behaviour-library-ui` checks the default, the refusals ("cc1Curve", "cc1Curve[1].share"), a saved
  three-point curve at schema 86, Distribution → Fixed, a fixed behaviour opening in Fixed and the Thai item.
  Mutations bite: Fixed as the default, the curve not saved, the decline restored.
- **Verified (Linux only, desktop preset, GCC 13.3):** `check` passes, `ctest` 374/374; the build without the PCH
  compiles every unit. F8 lists the owner's Windows look.

## 2026-11-24 — W99 BA231: queue-discharge timestep record (D649)

- **Owner:** "ทำต่อ"; the plan approved (NEXT's BA231).
- **Tool:** BA27's sweep loop moved to `tools/discharge_sweep.hpp`; `multra-w99-discharge-sweep` runs prototype and
  `w99` (`W99_ORIGINAL`, fixed cc1, fixture values) on the four-leg project at dt 0.1/0.25/0.5, seeds 42–81. The W74
  sweep's per-cycle CSV is byte-identical before and after the move; the W99 sweep repeats byte for byte and its
  prototype arm equals `multra-cli --discharge` in all 123 cycles of seed 42.
- **Result** ([evidence](../evidence/w99-discharge.md)): W99 headway 1.959 / 1.942 / 1.901 s, falling with dt (every seed
  at 0.5), where the prototype's rises (2.052 → 2.127 s). W99 clamps 7,870 / 3,056 / 1,512 per run: almost all are
  standing cars clamped every tick, because §4's creep asks +cc7 at a gap 10⁻⁹–10⁻⁷ m above cc0 that D105's `g ≤ cc0`
  misses. Recorded, not fixed; no engine change; nothing bounded or calibrated.
- **Verified (Linux only, Release `perf` tree, GCC 13.3):** the sweeps above; `check` passes (374/374); the build without the PCH compiles 691 units, 0 failed.
