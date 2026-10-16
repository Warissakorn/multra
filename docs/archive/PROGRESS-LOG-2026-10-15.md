# PROGRESS log archive — 2026-10-15

Old PROGRESS entries, moved whole, newest first. Nothing here is current: NEXT and ROADMAP
hold status, the decision record holds the reasoning.
## 2026-10-15 — M3.7 slice 3a: lateral motion, admission and crossing (D189)

NEXT item 1, LATERAL §7. The owner chose a test-only target writer and split zones into slice 3b.
- **State.** `Vehicle::lateralTarget` is yT, from the lane centre and in `SimState`. Only tests
  write it until M3.4/M3.7.1, so every product run keeps `lateral` at 0 (BA39).
- **Motion.** `moveLaterally` (`src/core/lateral_motion.*`) runs after the tick's publish, only
  switched on. Moves are capped at `maxLateralSpeed·dt`, kept inside the carriageway's edges and,
  without a covering span, inside the lane. They are admitted in vehicle-id order only without 2D
  overlap with a published or already-moved body (touching admitted).
- **Crossing.** A centre past a covered lane line remaps the route through `recordLaneChange`, now
  shared with the instantaneous change, and re-expresses `lateral` and the target; that tick's
  MovedEvent names the new lane.

Checked on Linux headless: `check`, CLI seed 42 unchanged, mutation checks
([evidence](../evidence/lateral-motion.md)).

## 2026-10-15 — M3.7 slice 2: lateral spans and predicates (D188)

NEXT item 1, LATERAL §4–§6. Only with the switch on; every lane-based run is unchanged.
- **State.** `Vehicle` gains `lateral`/`lateralSpeed` (0 until slice 3). `ScenarioIndex` gains a
  lateral index of each lane section's neighbours on its carriageway.
- **Spans.** Each span carries the body's band. A body covering another lane by more than
  `kLateralOverlap` adds a span there, mapped through the Link's reference line, since lane metres
  differ on a bend. Cross-sections gain reference ends and are required once switched on.
- **Predicates.** The leader search, PERCEPTION's search and `around` count a span on the
  follower's carriageway only within its body plus `dLat(v)`. Priority rules see straddlers.
  Courtesy skips other-lane copies.
- **Zones deferred (owner, D188).** They read route positions, so §6's "no new code path" was wrong
  there; slice 3 adds the span scan. LATERAL §5/§6/§10 are corrected.

Checked on Linux headless: `check` 102/102, CLI seed 42 unchanged, mutation checks; switched on,
the four-leg run costs about 8 % more ([evidence](../evidence/lateral-spans.md)).

## 2026-10-15 — M3.7 slice 1: lateral codec and cross-section compile (D187)

The first lateral code, NEXT item 1. Storage and compile only; no tick reads the data.
- **Keys.** `LateralParameters` is its own key table (`src/core/lateral.*`): all three or none,
  a partial set refused at its first missing key, on both models. Schema 33 is written only when an
  owned behaviour carries one, and a file below 33 carrying one is refused.
- **Compile.** With the switch on, `buildScenario` adds one `SegmentCrossSection` per segment and
  `drivingSide`. Lane centres come from `laneCentreOffset`, now shared with `laneGeometry`.
  Connector paths store start/end widths from `connectorLaneWidths`. `createSimulation` re-sorts
  them with their segments, and validation refuses a malformed table.
- **Contract refinements (D187).** Start/end widths replace one `width`, and `DrivingSide` moved from
  the model into core.
- **No change without keys (BA39)**, and none with them yet. Nine "future schema" fixtures moved
  33 → 34.

Checked on Linux headless: `check` 101/101, CLI seed 42 unchanged, mutation checks
([evidence](../evidence/lateral-codec.md)).

## 2026-10-15 — M3.5 slice 5: reaction-time discharge evidence (D185)

A C++ sweep measures BA38 on the four-leg project: prototype and the cited W74 preset,
reaction time 0/0.5/1 s, dt 0.1, seeds 42–81. All 240 experimental runs and 80 absent-key
controls completed. Zero-delay trajectories match on every tick; all runs pass accounting
and 32 ticks of copied-state replay, including live histories in positive-delay copies.
- Raw cycles/crossings, reasons/nulls, resolved snapshots and exact input/compiler/source/output
  provenance are retained; summaries weight seeds equally and report changing availability.
- Prototype flow barely changes while clamps increase sharply; W74 throughput falls. Negative
  startup estimates remain signed. These responses are recorded, not tuned or validated.
- Five focused tool tests cover variant isolation, identity/replay, deliberate mismatches,
  FIFO comparison and seed weighting; CLI coverage protects existing output directories.

Linux headless `check` **100/100 passed**, including five focused evidence-tool tests,
  CLI help/output protection and all existing perception/safety/reference regressions.
[Evidence and reproducible commands](../evidence/perception-discharge.md). No owner gate is closed.

## 2026-10-15 — Reaction FIFO entering a delayed road (D186)

Review of D184 found that a vehicle crossing from a zero-delay road onto a road with `d > 0`
had an empty FIFO that was filled with `{0, free}`, so a driver braking at its maximum coasted
for `d` ticks (reproduced at τ = 1 s: ten ticks of `a = 0`). The empty FIFO now applies the
tick's own decision and is filled with copies of it, as §5 pads a lengthened FIFO; insertion
still fills zeros. A BA36 regression covers both models; BA38 is unaffected (no road
assignments in its fixture). Linux headless `check` passed.

## 2026-10-15 — M3.5 slice 4: reaction-time action delay (D184)

Own motion now acts on a per-vehicle FIFO of earlier decisions, with d rounded from
`reactionTime / dt`. The shared decision value carries acceleration, mode and W74 state;
physical caps and current type bounds/D105 still apply to the action actually used.
- Source insertion and Lab placement initialise zero/free/no-state startup history.
- Road-boundary delay changes trim or pad the oldest end; zero delay clears history.
- Retunes changing a quantised count are refused atomically, including unused behaviours.
- Hypothetical source, courtesy and lane-change judgements remain undelayed. Delayed W74
  state is published with its applied action, and copied histories replay exactly.
- BA30's unread-key codec assertion is retired into a tick-by-tick zero-delay identity check.

Checked on Linux headless: full Release build and `check` **97/97**, including all 16
new delay cases. The CLI seed-42 pin remains **45 completed / 31.75870154816726 s / 1 clamp**.
Qt desktop and Windows were not verified here.
Interleaved unassigned-scenario benchmark median: +7.08 %, inside the 10 % budget.
See [the evidence](../evidence/perception-delay.md). No owner gate is closed.

## 2026-10-15 — M3.5 slice 3: look-back, rear TTC and perceived lane-change tests (D183)

Lane changes now read the changer's perception keys (`src/core/lanes.cpp`).
- **Mirror check.** `around` drops a target-lane follower further back than `lookBackDistance`.
  An unseen follower no longer blocks the change. It then reacts through its own model, and the
  hard cap keeps it behind (BA33 runs one to the end). Alongside always refuses.
- **Rear time-to-collision.** A seen follower closing on the changer must give `gap / (vF − v) ≥
  rearTimeToCollision`; equal accepts (BA34).
- **Forward test and gain.** These use the perceived leaders on the target lane and §3's lowest
  acceleration. The room check runs against the nearest seen leader, and nothing is tested when
  none is seen. The discretionary gain reads perceived leaders on both lanes.
- **Shared code.** `notePerceived` and `anticipate` are shared with own motion, so the tie rule and
  the multi-anticipation loop exist once.
- **Checks.** Mutation check: four of the six new tests fail against slice 2's `lanes.cpp`. The
  four-leg seed-42 run with neutral key values is equal tick by tick, with lane changes in it.
  Benchmark inside noise.

Checked on Linux: `check` 141/141, CLI seed 42 unchanged
([evidence](../evidence/perception-lane.md)).

## 2026-10-15 — M3.5 slice 2: look-ahead and multi-anticipation (D182)

Own motion now reads two of the perception keys.
- **What a driver perceives.** `perceivedLeaders` returns up to `observedVehiclesAhead` distinct
  vehicles within `lookAheadDistance`, nearest first.
- **How it brakes.** Own motion takes the lowest `follow` over them, using effective gaps less the
  room the vehicles in between need at rest. A standing car two vehicles ahead slows the follower
  before the car in between reacts.
- **What stays physical.** The unbounded leader still sets the hard cap, admission and D108, so an
  unseen queue can only produce a counted clamp, never an overlap.
- **Tie rule changed.** PERCEPTION §2 said ties go by vehicle id; it now says scan order, the rule
  `closestVehicle` already uses. Only then is one observed vehicle with no limit exactly today's
  leader, which the four-leg seed-42 comparison confirms.
- **Benchmark.** Interleaved runs show about +1 % with the keys absent.

Checked on Linux: `check` 140/140, CLI seed 42 unchanged
([evidence](../evidence/perception-motion.md)).

## 2026-10-15 — M3.5 slice 1: perception codec (D181)

The first code in D178's order. `DriverBehaviour` gains `PerceptionParameters`: five optional
keys from [PERCEPTION](../reference/PERCEPTION.md) §6, appended last.
- **One key table.** `src/core/perception.hpp` has an integer flag and the §6 ranges. Parser,
  serializer, the library's known keys and `validateScenario` all read it, on both models.
- **Schema 32** is written only when an owned behaviour carries a key; a file below 32 that carries
  one is refused.
- **Dialog.** The Driving behaviours dialog's `w74` path rebuilt the behaviour from scratch, so it
  now copies the keys and no longer drops them.
- **No motion change.** No motion reads the keys yet: BA30 compares a four-leg seed-42 run with and
  without them and finds them equal.
- **Fixtures.** Nine fixtures that used 32 as the refused "future" schema now use 33.

Checked on Linux: `check` 139/139, CLI seed 42 unchanged
([evidence](../evidence/perception-codec.md)).

## 2026-10-15 — M3.7.0 two-dimensional occupancy contract (D180; docs only)

[LATERAL](../reference/LATERAL.md) is the second contract in D178's order.
- **Frame and data.** Each carriageway gets a lateral frame (positive left). Compiled
  cross-sections take their lane centres from the one function `laneGeometry` uses, and
  `drivingSide` moves into the Scenario.
- **Vehicle state.** A vehicle's `lateral` is relative to its current lane centre, so routing
  stays keyed to one lane chain.
- **Spans.** The key choice is to emit a span on **every lane section a body overlaps**. A changer
  or a motorcycle on a lane line then appears on both lanes, and the existing leader search,
  `around`, zones and admission handle it without a second occupancy engine.
- **Lateral tests.** Leader and alongside tests add a lateral overlap check with a speed-dependent
  minimum lateral distance.
- **Moves.** Lateral moves are admitted in vehicle-id order, as phase 2 does.
- **Switch.** One switch (any of three keys) keeps lane-based runs byte-identical.

Rows BA39–BA46 are pending. NEXT now starts code with the perception codec slice. Checked:
`check` on Linux.

## 2026-10-15 — M3.5.0 perception contract (D179; docs only)

[PERCEPTION](../reference/PERCEPTION.md) is the first contract in D178's order.
- **Two leaders.** The **physical leader** (today's unbounded search) keeps the hard cap, zone
  admission and phase-2 caps. **Perceived leaders** (within `lookAheadDistance`, up to
  `observedVehiclesAhead`, effective gaps less the room of the vehicles in between) feed only
  the acceleration, so an unseen leader becomes a counted clamp, never an overlap.
- **Mirror check.** A target-lane follower counts only within `lookBackDistance`, plus an optional
  rear time-to-collision test. Something alongside always refuses the change.
- **Reaction time** is an action delay: a per-vehicle FIFO of `round(τ/dt)` computed
  accelerations. It replaces the review's ring of past snapshots, because it costs one entry per
  vehicle per tick and needs no remapping of past positions. Hypothetical tests stay undelayed.
- **Data.** Five optional keys; absent = today. One new schema, only when used.

Rows BA30–BA38 are pending; nothing is implemented. Checked: `check` on Linux.

## 2026-10-15 — Owner's answers to B14 and B15 (D178; docs only)

"B14 รับ, B15 ใช้ค่ามาตรฐานไปก่อน และจะทำการวิจัยในอนาคต". The review's order is the queue, keep-left
is the default bias on left-hand-traffic networks (a behaviour key), and turn-on-red/U-turns stay
in M3.9. Motorcycle values start from cited published standards, flagged uncertain, until the
owner's research. Recorded in RECORD, ROADMAP M3.4/M3.8/M3.9, NEXT and the review header; B14/B15
moved out of OWNER_SITTING.

## 2026-10-15 — Driving-behaviour completeness review (D177; docs only)

The owner asked for a thorough review of driving behaviour (lateral behaviour, mirror checks,
overtaking, everything that makes runs realistic) and chose "review plus roadmap" with
motorcycles as core scope. [The review](../audits/DRIVING_BEHAVIOUR_REVIEW.md) reads the whole of
`src/core/` and the behaviour contracts. It finds a one-dimensional engine, one observed vehicle
each way at unbounded distance, no reaction delay, instantaneous lane changes with a display-only
slide, overtaking only as D95 (off), no turning speeds and no lateral position. Its gap matrix
covers twelve areas (A–L). Lateral, lane-change duration and motorcycle work all need one
2D-occupancy contract, and perception and reaction need one observation contract, so both
come before W99. ROADMAP gains M3.5–M3.9 and an extended M3.4 (hard rule 8); PROBLEM §2 gains the
motorcycle row and §8.1 is answered; NEXT follows the review's order; B14/B15 hold the owner's
remaining calls. Checked: `check` (docs, sizes, architecture, CTest) on Linux.

## 2026-10-15 — A cited, uncalibrated `w74` preset (NEXT item 1, D176)

`data/driver-behaviour/presets/w74-sumo-defaults.json` maps every one of the 18 keys from SUMO's
`MSCFModel_Wiedemann` defaults (pinned at `79e42b50`), the file W74.md already follows. It sits
in a subfolder because the catalog loader reads only top-level `*.json`, and an unused `w74`
behaviour would give every vehicle traits (BA18); `tools/t_junction_sweep.hpp` now lists the
same files the loader reads, so its archived metadata still matches. A codec test pins that the
preset is valid and not loaded. **Evidence** ([w74-preset](../evidence/w74-preset.md)): with the
preset, four-leg discharge ≈3.44 s (fixture ≈3.11 s, prototype ≈2.05 s) and 3.2–4.7 clamps
per run (fixture 8.2–14.2), 72–98 % at amber, which `w74` behaviours still hold as red. No
defect, nothing calibrated. The fixture no longer reproduces BA27's hashes (D159, M4.2), so
both were rerun on one build. Shipping it, and an amber value, are OWNER_SITTING B13.
Checked: desktop build and full CTest (Linux, GCC 13.3), `check`, CLI seed 42 unchanged.

## 2026-10-15 — Documentation cleanup for AI reading (no behaviour change)

The owner asked for docs that are organised, trimmed and efficient for an AI to read, and for
an analysis of whether the archive and the evidence are still needed. **Archive:** the 124
PROGRESS files were reachable only from the archive index. They hold history; the reasoning is
already in RECORD. They are merged verbatim, newest first, into 17 dated `PROGRESS-LOG-*` files
(134 → 31 archive files, index 206 → 45 lines), and each keeps its former file name as an anchor.
**Evidence:** every file is cited, either directly or through its note, and tests read the
`m3.2.7-*` data, so nothing is deleted. A new `evidence/README.md` says to read the note, not the
raw data. **Live docs, moved whole rather than summarised:** PROGRESS keeps its recent entries
(498 → 91 lines); ROADMAP keeps open milestones, gates and status rows (50 → 27 KB); RECORD
collapses the wholly superseded D3, D4, D9, D10 and D14; VISSIM_PARITY drops its 2026-09-14
§1, §3 and §6; the answered 2026-10-09 roadmap review moves to `archive/`. Every ID, gate and
anchor is kept. Checked: `trafficsim-check-docs` and `trafficsim-check-file-sizes` built
standalone (Linux, GCC 13). The full CTest was not run because nlohmann/json is missing in this
container; no code, test, data or spec file changed.

## 2026-10-15 — Documentation consistency pass (no behaviour change)

The owner asked for docs an AI can read efficiently. The session path (CLAUDE → AGENTS → NEXT)
went from 470 to 262 lines: AGENTS' three reading sections became one Session path. NEXT keeps
only live items; its notes moved verbatim (checked line by line) to
[ENGINEERING_NOTES](../reference/ENGINEERING_NOTES.md), with machine notes in BUILDING. Appearance
checks of merged reviews moved to OWNER_SITTING C10–C12 and E10–E11. `native.yml` passed on `main`
`c2bacd1`, retiring NEXT's CI notes. Status lines contradicting the code were corrected: W74
runs (D138), LOS and batches exist (M5), M8 is implemented, schema 31. History, archive, evidence
and specs are untouched.

## 2026-10-15 — Multi-level workflow design accepted (D175)

Docs only. A design review in chat (Thai) walked how the three levels, the editor and the
network work together, compared with Aimsun, and asked for starting at any level. Two scrutiny
passes corrected the drafts: the D145 plan reused an M5.1 Node that does not exist, bound OdMatrix
periods to Micro's run duration, and stored subarea cuts that go stale; per-field status flags
duplicated what field presence already says; mid-link Micro destinations need a core contract
(D153), so Micro keeps driveways as Links. Web checks before recording: GEH/DMRB wording
(secondary sources, flow test added), HCM 1,900 pc/h/ln with wide local variation, UTM 47N/48N
split at 102°E with legacy Indian 1975 datums, and motorcycle PCU varying within the green. The
owner accepted every recommendation; [MULTI_LEVEL_WORKFLOW](../plans/MULTI_LEVEL_WORKFLOW.md) records
the design, answers and order (S0 Node first). No code, schema or test changed. The D147 entry
moved whole to `archive/`.
