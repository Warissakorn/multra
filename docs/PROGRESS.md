# PROGRESS — TrafficSim

Newest entry at the top: what changed and why. What to do next is in [`NEXT.md`](NEXT.md);
decisions go once into the [decision record](decisions/RECORD.md). Keep about ten entries here;
move older ones whole to the top of the newest [`archive/`](archive/README.md) PROGRESS log.

---

## 2026-10-16 — Parallel tests, cheaper hit-testing, vcpkg cache (no decision)

An /optimize pass over engine, editor, build/CI and docs (Linux, GCC 13.3, Qt 6.4 offscreen; no
Windows or real-display verification). Three changes, one commit each:

- **`check` and CI run ctest in parallel.** The test groups were already separate ctest tests so
  `-j` could spread them, but `check` and every CI ctest step ran serially. `check` now passes
  the host core count; Windows and packaging steps pass `--parallel 4`. Desktop Debug `check`
  294 → 74 s; 7/7 parallel runs green; Windows desktop ctest 1m20 → 0m40 in CI. The sanitizer step is
  noisy on CI (8m12 serial, 9m39 parallel, 13m18 serial in runs 554–556) but scaled 952 → 254 s
  locally, so it runs in parallel too; `TRAFFICSIM_TEST_JOBS` overrides the job count.
- **`hitObjects` rejects misses by padded bounds** and takes shapes from `linkShape` /
  `connectorShape` instead of an id scan per object (it was quadratic), computing distance for hits
  only. Release editor benchmark, 200 intersections: 8.28 → 1.22 ms, wireframe 14.24 → 4.61 ms.
  Band selection uses the same helpers. A connector whose paths cannot be built now throws only
  when it is hit.
- **windows-desktop caches vcpkg binaries** (`.vcpkg-cache`): proj was rebuilt from source,
  5m11s of every run; with the cache it takes 24 s and the
  whole windows-desktop job about 3 min (run 556).

Looked at and left: the engine (no `stepSimulation` child above 9 %, noise ±7 %), the Run view
(already batches Steps per frame), docs (the audit's broken references were false positives).

## 2026-10-15 — Six audit correctness fixes from main (D191)

The owner authorized fixes from main after the repository review; this branch starts at
`0b45fee`. The six reproduced issues are corrected:

- Compile exact runtime-to-authored route provenance outside core; `/` in authored IDs no longer
  sends completed trips to another movement, including lane expansion and seed aggregation.
  Follow-up (2026-10-16): the three remaining consumers that rebuilt the relation from the id's
  spelling now read the same map — `authoredRouteLength` (the editor's route Length, which gave
  `r` the length of an authored `r/lane-2` listed before it), parking-lot exits and the lab's
  loop lanes. Outputs are otherwise unchanged (CLI seed 42 and all four projects' 42–43 batches).
- Live desired-speed reductions use bounded braking with consistent speed, acceleration and
  distance in both prototype and W74. Ordinary runs retain their frozen replay. Review follow-up:
  the prototype's free term `(desired − speed)/followingTime` had no floor, so a 15→5 m/s retune
  braked at −6.7 m/s² (25→5: −13.3, past `maxDeceleration`); it is now floored at
  `−comfortableDeceleration`, the rate W74 already used. It is never negative without a retune or
  placement, so baselines are unchanged; both models' tests now assert the comfortable bound.
- Reject retune demand edits atomically instead of retaining an incompatible arrival schedule.
- Check finite, integral, in-range signal group numbers before casting to `int`, for controllers
  and heads; reject empty chains on either side of a conflict zone.
- Mirror runtime catalog deletions and renames, preserving the separately managed PROJ database.
  Add a guarded CMake sync test and a fail-fast headless sanitizer preset/Linux CI matrix entry.

Verified locally with GCC 14.2: headless target `check`, **104/104** CTest suites passed; the six
new regression cases passed. Four behavior regressions were observed failing before their
fixes; the sync test also rejects the old copy-only implementation. Instrumented core smoke
passed ASan/UBSan; instrumented parser checks rejected 14 invalid values and read four `int`
boundaries under UBSan/float-cast-overflow. CLI seed-42 JSON is byte-identical to the
review baseline; seed 42–43 serial/parallel batch JSON is byte-identical. Full sanitizer CI,
desktop and Windows verification are pending; no modelling-validation gate is closed.

## 2026-10-15 — Quiet canvas, hover/selected states and type roles (D190)

Owner request (before slice a): no tooltip on the canvas background, a simpler and more modern
hover/selection look, and a review of text sizes and weights. The owner chose the grey fill plus
blue underline for the active tool and to remove the conflict-key, panel and run-note tooltips.

- Tooltips removed from the empty canvas, Objects panel background, conflict areas and run
  summary; F1 now shows keyboard help, gesture help and the run-status note (`editorHelp`,
  which overlapped the gesture help, is gone). Accessible descriptions keep the texts.
- One state language in `editorStyleSheet()`: hover fill, Mid when pressed/checked/selected with
  an accent underline or left bar, accent frame only for keyboard focus; rounded menu rows.
- Type: dock titles 13/600, hints 11/400, Lab group boxes 14/600. Menu bar and dock title
  heights unchanged or 2 px shorter in Thai.
- Found while capturing: child widgets drew Qt's default greys/blue, not the tokens (the window
  palette did not reach them under the stylesheet on Qt 6.4). `applyEditorStyle` now also sets
  the application palette. Before/after images: `docs/images/ui-states-{before,after}/`.
- Tests: empty canvas shows no tooltip; conflict areas have none but keep their key; a checked
  toolbar button stays 24 px with an accent underline and no frame.

Linux Qt 6.4 offscreen: desktop build, CTest **148/148**, target `check` passed. Windows and
100/150/200 % visual review is OWNER_SITTING E13.

## 2026-10-15 — UI phase 1 and stable-tab slice 0

Owner requested the UI audit/spec/slice plan, then accepted the recommendation: “ทำตามแนะนำ”.
B16=A (group navigation within the existing Objects dock), B17=five tool groups; B18 logic/state
features stay separate and deferred. [Audit](audits/UI_PRESENTATION_AUDIT.md),
[proposal](plans/UI_PRESENTATION_SPEC.md), [slices](plans/UI_PRESENTATION_SLICES.md).

- Counted 14 Objects pages and four Results pages from construction/translation code; found
  identical Connector/Route and Rotate/Reset glyphs. Designs include both window sizes,
  typography, monochrome shapes, HUD/LOD and Properties/table recipes, with no theme work.
- Slice 0 registers each actual page widget against its existing translation key. Translations,
  Show in table, Problems/error jumps, active-conflict checks, Signal-head toolbar visibility
  and batch Results selection no longer depend on presentation positions. Updated page tests
  resolve the actual containing widget; all tool mode/chord values and page order stay intact.
- Failure-first: the new workspace fixture proves its tab move succeeded, then the old code
  fails with “Reordered object page translated by position”. It checks both languages, all 18
  page labels, tool navigation, toolbar visibility, failed Run/batch and batch page selection.
  A separate window keeps this stress fixture from changing the original layout baseline.
- No new D-row: no visible behaviour/token change in slice 0. The approved layout/glyph changes
  belong to later sessions. OWNER_SITTING E12 retains Windows visual review; NEXT is slice a.

Linux Qt offscreen: desktop build, CTest **148/148**, and target `check` (including **148/148**)
pass. Before/after English, Thai and focus PNGs are byte-identical; audit §8 records the
existing Thai post-Step 248 px viewport limitation. The prepared
cloud toolchain uses verified Debian packages outside the checkout, the bundled font through
local fontconfig (system same-family fonts otherwise override its tabular digits), and writable
Qt test storage. No core/model/commands/schema, translations, tokens or numerical output changed.

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
([evidence](evidence/lateral-motion.md)).

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
the four-leg run costs about 8 % more ([evidence](evidence/lateral-spans.md)).

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
([evidence](evidence/lateral-codec.md)).

## 2026-10-15 — Reaction FIFO entering a delayed road (D186)

Review of D184 found that a vehicle crossing from a zero-delay road onto a road with `d > 0`
had an empty FIFO that was filled with `{0, free}`, so a driver braking at its maximum coasted
for `d` ticks (reproduced at τ = 1 s: ten ticks of `a = 0`). The empty FIFO now applies the
tick's own decision and is filled with copies of it, as §5 pads a lengthened FIFO; insertion
still fills zeros. A BA36 regression covers both models; BA38 is unaffected (no road
assignments in its fixture). Linux headless `check` passed.

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
[Evidence and reproducible commands](evidence/perception-discharge.md). No owner gate is closed.

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
See [the evidence](evidence/perception-delay.md). No owner gate is closed.

## Older entries

Older entries are in [`archive/`](archive/README.md), newest first. Decisions live in the
[decision record](decisions/RECORD.md); the M0 backlog and question register are
[archived](archive/PROGRESS-M0-backlog-and-questions.md).
