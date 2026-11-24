# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-24 — Stopping at the line: the contract and its measured options (D651)

- **Owner:** reviewed line stops, M0 and other programs, then: "ถ้าต้องการให้โปรแกรมสมจริง มีประสิทธิภาพที่สุด"; "ทำตามแผนเขียนสัญญา".
- **Measured** ([evidence](evidence/line-stop-options.md); a temporary [patch](evidence/line-stop-options.patch), reverted):
  - five projects × prototype, W74 fixture, W74 SUMO preset, W99 × L0 / L1 / L1F / L3 / L3F, seeds 42–51;
  - L3 rests every model at every line type (0 m), cuts crawls from 10–14 s to about 0.5 s and moves startup lost
    time to +1.4 to +2.1 s, at +1.1 % instructions;
  - L1 leaves W74 1 m short and clamps 225 per run on M2.6;
  - a first run counted rounding residues at the line as clamps, so the finish (D596's, GIVE_WAY_STOP §2) is part
    of every option.
- **Contract** [LINE_STOP](reference/LINE_STOP.md): L3F recommended as `clampRules.lineRest`, scoped like D257 so
  M0 keeps its bytes (an unscoped rule moves bare M0, 1 → 5 clamps); rows BA239–BA245; W99's arrival clamps left to
  a W99 §4 slice. **The owner chooses.**
- **Verified:** `multra-cli 42` byte-identical after the revert; `check` passes; docs only committed.

## 2026-11-24 — W99's standstill creep waits until it fits (D650)

- **Owner:** reviewed creep in SUMO, Vissim and W74, then stopping at lines; "เริ่มเลย" to the creep's hold.
- **Rule (W99 §4):** a stopped driver is held while `g − CC0 ≤ CC7 · dt²`, the most one creep can move (a tick of
  +CC7, then too-close braking). `w99Acceleration` and `follow` take the run's `timeStep`, read by W99 only;
  `deadEndGoverns` passes it on. Every caller is updated.
- **Rows:** BA238 in `w99` (held at `CC0 + CC7·dt²` exactly, +CC7 just above; D649's 10⁻⁹ m residue held; moving
  unaffected by dt) and `w99run` (a queued car at rest is never clamped in 600 ticks; from 3 m back it closes to
  within `CC7·dt²` of CC0). Mutations bite: D649's `g ≤ CC0` and half the bound.
- **Re-run** ([evidence](evidence/w99-discharge.md#after-d650)): W99 clamps 7,870 / 3,056 / 1,512 → 331 / 324 / 330
  per run, each a car's one arrival (the soft closing approach, NEXT's line contract). Headway unchanged.
- **Verified (Linux only):** `multra-cli 42` summary and events byte-identical; the W74 sweep byte-identical;
  `check` passes (374/374); the build without the PCH compiles 691 units, 0 failed.

## 2026-11-24 — W99 BA231: queue-discharge timestep record (D649)

- **Owner:** "ทำต่อ"; the plan approved (NEXT's BA231).
- **Tool:** BA27's sweep loop moved to `tools/discharge_sweep.hpp`; `multra-w99-discharge-sweep` runs prototype and
  `w99` (`W99_ORIGINAL`, fixed cc1, fixture values) on the four-leg project at dt 0.1/0.25/0.5, seeds 42–81. The W74
  sweep's per-cycle CSV is byte-identical before and after the move; the W99 sweep repeats byte for byte and its
  prototype arm equals `multra-cli --discharge` in all 123 cycles of seed 42.
- **Result** ([evidence](evidence/w99-discharge.md)): W99 headway 1.959 / 1.942 / 1.901 s, falling with dt (every seed
  at 0.5), where the prototype's rises (2.052 → 2.127 s). W99 clamps 7,870 / 3,056 / 1,512 per run: almost all are
  standing cars clamped every tick, because §4's creep asks +cc7 at a gap 10⁻⁹–10⁻⁷ m above cc0 that D105's `g ≤ cc0`
  misses. Recorded, not fixed; no engine change; nothing bounded or calibrated.
- **Verified (Linux only, Release `perf` tree, GCC 13.3):** the sweeps above; `check` passes (374/374); the build without the PCH compiles 691 units, 0 failed.

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
  ignoring the share, the refusal kept ([evidence](evidence/w99-cc1-draw.md)).
- **Verified (Linux only, desktop preset, GCC 13.3):** `check` passes, `ctest` 374/374; the build without the PCH
  compiles every unit; `multra-cli 42` summary and events byte-identical.

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

