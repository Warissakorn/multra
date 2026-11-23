# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

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

- **Brief:** [PARALLEL_SESSIONS](plans/PARALLEL_SESSIONS.md) A. Callgrind, GCC 13.3, Release `-g1`, seed 42; baseline study
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

- **Brief:** [PARALLEL_SESSIONS](plans/PARALLEL_SESSIONS.md) B. The root file was at 498 of 500 lines.
- **Moved whole, in place and order:** tool executables → `cmake/tools.cmake`; check tools and `check` →
  `cmake/check_target.cmake`; `multra-tests` and its groups → `cmake/model_tests.cmake`; tool, CLI and script
  tests → `cmake/cli_tests.cmake`. Root 498 → 194 lines; the map is in
  [BUILDING](BUILDING.md#where-the-build-rules-live).
- **No behaviour change**, on `desktop` (Qt 6) and `headless`: sorted `ctest -N`, `--target help`, sorted
  `compile_commands.json`, `cmake_install.cmake` and sorted `ninja -t commands` identical before and after;
  `CTestTestfile.cmake` differs only in each test's backtrace (file and line).
- Checks (Linux only, GCC 13.3): full build, `ctest` and `check` pass on `headless` (251/251) and `desktop` (368/368).


## 2026-11-23 — Four slices briefed for parallel sessions

- **User:** "พัฒนาต่อ โดยเปิด Branch และเขียน prompt เพื่อให้ Agent ทำงาน พิจารณาการทำหลายงานพร้อมกันด้วย".
- Of NEXT's open items, four need no owner choice and touch disjoint files: the leader order kept across
  ticks (A), the `CMakeLists.txt` split (B), the Results tab strip at 1024 in Thai and the clamp note (C),
  and ARCHITECTURE's *Editor boundary* prose into the contracts (D). Briefs:
  [PARALLEL_SESSIONS](plans/PARALLEL_SESSIONS.md); one branch and one cloud session each.
- To keep four merges trivial: decision numbers 626–641 and BA216–BA223 reserved per slice; only B edits
  `CMakeLists.txt`; the four oldest PROGRESS entries moved whole to
  [PROGRESS-LOG-2026-11-21](archive/PROGRESS-LOG-2026-11-21.md) to leave room for four new ones.
- Docs only; `multra-check-docs` and `multra-check-file-sizes` pass.

## 2026-11-23 — M3.2 closed by the owner (D625)

- **Owner:** item 54 "สมจริง"; then, shown each gate piece with its evidence, "ตกลง" to closing M3.2.
- **D625:** ROADMAP, M3_ACCEPTANCE and NEXT say closed. The one Windows look with no verdict of its own (M3.2.4e merge
  mouth, M3.2.12 stop lines and tool) was added to M3.2.13 as item 4 rather than read into A2. Still open on their own
  gates: M3.2.10, M3.2.13, M3.2.8a.1, M3.4, M6; the not-yet-validated marker stays.
- Docs only; `multra-check-docs` passes.

