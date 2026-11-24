# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-24 — F7 on Windows; the W99 dialog names an out-of-range key first (D643)

- **Owner (F7, package #145, verbatim in [OWNER_SITTING_WINDOWS](plans/OWNER_SITTING_WINDOWS.md)):** empty fields,
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
  ([evidence](evidence/w99-composition.md)). `multra-cli 42` summary and events byte-identical.
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
  ([evidence](evidence/w99-traits.md)). `multra-cli 42` summary and events byte-identical.
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
  look is [F7](plans/OWNER_SITTING_WINDOWS.md).

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
- **[W99](reference/W99.md):** inputs and W99's sign of `dv`; six thresholds; four regimes with equality sides; ten
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

- **Owner:** "ok" to D573–D574, which released the briefed slice ([PROCEDURE_SCHEMA_SLICE](plans/PROCEDURE_SCHEMA_SLICE.md)).
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
  (rows ordered by ID). Three entries moved whole to [PROGRESS-LOG-2026-11-21](archive/PROGRESS-LOG-2026-11-21.md).
- **Review before merging (Linux):** B's configure output equals the base's (368 tests and their commands, the
  target list and `compile_commands.json`); C's `results-tabs-ui` re-run with screenshots; D's moved prose
  spot-checked against its contracts. Merged tree: see the checks recorded with the merge commit.

## 2026-11-23 — Slice D: ARCHITECTURE's *Editor boundary* moved into the contracts

- **Brief:** [PARALLEL_SESSIONS](plans/PARALLEL_SESSIONS.md) D. ARCHITECTURE's 130 lines of per-decision notes now
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
