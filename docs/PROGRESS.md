# PROGRESS — MULTRA

Newest entry at the top: what changed and why. The rules are in [AGENTS](../AGENTS.md#working-rules)
(next work in [`NEXT.md`](NEXT.md); about ten entries, older ones whole to [`archive/`](archive/README.md)).

---

## 2026-11-25 — Queues behind a standing car: the contract and its measured options (D655)

- **Owner:** F9 on package #149: followers crawl without end. They asked whether a stop at a line and behind a
  car should be alike, chose A then B, then "เริ่มเขียนสัญญา A เลย".
- **Measured** ([evidence](evidence/line-queue-options.md); a temporary [patch](evidence/line-queue-options.patch),
  reverted). Six projects × four behaviours × options Q0–Q5, seeds 42–51:
  - **Today:** median crawl is 17 s (prototype) and 30–36 s (W74).
  - **Q2, for W74 and W99** (the line's stop law behind a vehicle at rest, plus a hold at the target):
    - median crawl about 3 s;
    - W99 clamps −80 to −99 % (the weaving lab 2,987 → 31 per run);
    - lower delay;
    - costs: more jerk ticks, and on M2.6 more W74 hard-braking ticks, not yet traced.
  - **The prototype has no clean option.** Q2 clamps 303–1,100 per run, and D546's lever still 49–208.
- **Contract** [LINE_QUEUE](reference/LINE_QUEUE.md):
  - Q2 recommended for W74 and W99; the prototype keeps today's law, and its crawl becomes its own question.
  - Rows BA252–BA256.
  - **The owner chooses.**
- **Verified:** `multra-cli 42` byte-identical after the revert; `check` passes; docs only committed.

## 2026-11-24 — W99 stops at CC0 behind a standing car (D654)

- **Owner:** "ทำขั้นต่อไป" (NEXT's W99 §4 slice), plan approved.
- **Probe (seed 42, four-leg):** the clamps were arrivals behind standing cars, at 0.03–0.14 m/s with millimetres
  left:
  - *closing* aims 0.1 m past CC0;
  - *following* creeps at +CC7;
  - D652's floor skipped them.
- **Rule (W99 §4):**
  - Behind a standing obstacle, a moving driver asks no more than the exact stop at CC0 at the type's
    comfortable deceleration (`lineStopAcceleration`).
  - The exact `−v²/(2·room)` applies when the stop falls inside the tick.
  - It aims 1 nm short of CC0 against rounding.
  - It is not gated by `lineRest`.
- **Rows:**
  - BA249 in `w99run`: 33 arrivals over dt × room × speed, no clamp, rest within `CC7·dt²` of CC0.
  - BA250 and BA251 in `w99`: not binding far away or behind a moving leader; hand-checked numbers.
  - BA238's dt row now uses a moving leader.
  - Five mutations each fail a row.
- **Measured** ([evidence](evidence/w99-discharge.md#after-d654)):
  - BA231's W99 clamps: 345 / 222 / 145 → 42 / 25 / 32 per run.
  - Four-leg 337.5 → 39.2, M2.6 1,258.9 → 150.2.
  - Headway is unchanged.
  - About 22 of the 48 left at seed 42 are behind leaders still moving when they stop. The leader's braking is
    unknown, so this stays open.
- **Verified (Linux only):**
  - `multra-cli 42` is byte-identical, and so are the prototype and W74 sweeps.
  - Each sweep, run twice, gives identical output.
  - `check` passes.
- **Package #149** (`f5653f8`): for the owner's [F9](plans/OWNER_SITTING_WINDOWS.md) (lineRest, queues at the
  line, a W99 queue), with F7 and F8.

## 2026-11-24 — A front at the line has not passed it; BA27/BA231 and the SUMO look re-run (D653)

- **Owner:** "ทำต่อไป ตามแนะนำ" (re-run the sweeps under D652, then the SUMO comparison).
- **Found:**
  - With D652, the sweeps' available cycles fell (prototype 1,316 → 998).
  - The discharge, stop-line and priority-line observers counted a front exactly at a line as passed. So a
    queue head resting there under LINE_STOP was counted as crossing during red and dropped from its cycle.
  - D652's "startup lost time +1.6 to +2.0 s, toward HCM" was this artefact. It is withdrawn in RECORD,
    LINE_STOP §8 and the evidence.
- **Fix:** `pastLine` (`src/eval/line_passage.hpp`) is used by all three observers:
  - a front at a line has not passed it;
  - a line at a route's start is passed on insertion;
  - an arrival is past every line.
- **Rows:** BA246–BA248 (`linepassage`), all three failing under the old rule. PL1–PL9's independent counter
  takes the same rule.
- **Re-measured:**
  - [W99](evidence/w99-discharge.md#after-d652-and-d653) and [W74](evidence/w74-discharge.md#after-d652-and-d653-2026-11-24)
    sweeps.
  - With the switch on, the prototype's and W99's headway are flat across dt. W99 no longer falls.
  - Startup lost time is −0.6 s (prototype) and −1.7 s (W74), lower than with the switch off.
  - [Four-leg and M2.6](evidence/line-stop-options.md#after-d653): clamps and delay are unchanged to the digit.
- **SUMO:** the [M0 scene under project rules](evidence/m0-plausibility.md#under-project-rules-d652-d653):
  - no clamps, no red passes;
  - start-up lost time 0.74 s, between SUMO Krauss 0.69 s and IDM 0.44 s;
  - the amber rule without the line rule passed on red 6 times in 10 runs.
- **Verified (Linux only):**
  - `multra-cli 42` is byte-identical.
  - The switch-off sweeps with the old rule reproduce D650's per-cycle CSVs byte for byte.
  - Each sweep, run twice, gives identical output.
  - `check` passes.

## 2026-11-24 — Stopping at the line: L3F implemented (D652)

- **Owner:** "เลือก L3F ทำเลย".
- **Engine:**
  - `src/core/line_stop.*`: with `DriverBehaviour::lineRest` (the switch, and `amberDeceleration` or the w74/w99
    model), the model follows vehicles and the nearest holding line has its own law (comfortable braking to the
    line), plus D258's floor for W74/W99.
  - At the line: `stopAtLine`, or rest, in place of a clamp.
  - Dead ends stay leaders. As lines they cut the weaving lab's served count from 389 to 341.
- **File and editor:** `clampRules.lineRest` (absent on, false at schema 87), refused below 87; Run settings'
  sixth checkbox.
- **Rows:** BA239–BA245 (`linestop`) and the checkbox in `signal_ui_tests`. Older-mechanics rows run with the
  switch off:
  - `stopfloor` and `mergeclamps`;
  - the W74/W99 composition rows and the M6 U fixture;
  - red running's output row BA145. Its runs were cars creeping at the line when red began.

  Diverge A68 and visibility VS5 move to the new behaviour.
- **Measured** ([evidence](evidence/line-stop-options.md#after-d652)): the T-junction and the priority lab equal
  D651's L3F byte for byte; on the four-leg the prototype's startup lost time is +1.98 s (was −0.11) with 0
  clamps. (That startup figure is withdrawn: a measurement artefact, D653.)
- **Verified (Linux only):** `multra-cli 42` byte-identical; `check` passes (375/375); the build without the PCH compiles 692 units, 0 failed. Package #147 (`c193433`) recorded for F7/F8.

## 2026-11-24 — Stopping at the line: the contract and its measured options (D651)

- **Owner:** reviewed line stops, M0 and other programs, then: "ถ้าต้องการให้โปรแกรมสมจริง มีประสิทธิภาพที่สุด"; "ทำตามแผนเขียนสัญญา".
- **Measured** ([evidence](evidence/line-stop-options.md); a temporary [patch](evidence/line-stop-options.patch), reverted):
  - five projects × prototype, W74 fixture, W74 SUMO preset, W99 × L0 / L1 / L1F / L3 / L3F, seeds 42–51;
  - L3 rests every model at every line type (0 m), cuts crawls from 10–14 s to about 0.5 s and moves startup lost
    time to +1.4 to +2.1 s (withdrawn by D653: a measurement artefact), at +1.1 % instructions;
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
