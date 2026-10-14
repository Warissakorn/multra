# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the session history; the
[decision record](decisions/RECORD.md) stores the reasoning; **this file is the part a session must read before starting.**

**Write the next session's work here, not into a new `PROGRESS.md` entry.** Two copies of what
to do next is the duplication hard rule 3 forbids, and the copy that rots is always the one in
the log. Rewrite this file; do not append to it.

## Code-review fixes (2026-10-14, D167)

The owner approved [the review correction plan](plans/REVIEW_FIXES-2026-10-14.md).
The working branch is `codex/review-fixes-2026-10-14`; corrections are split by system.
Before merging, check native CI on the final PR commit (Linux headless/desktop/release and
Windows desktop/core). Owner appearance checks: lane change followed by a routing decision;
Lab length and Apply/Undo; Cancel/Restart while a long seed is running. Automated checks do
not close any milestone or engineering-validation gate. After this review return to the queue below.

## Behaviour Lab review (2026-10-14, D165)

The owner's request — a window to tune driving behaviour on a vehicle that laps a test network —
is implemented as M3.3.4 ([contract](reference/BEHAVIOUR_LAB.md), rows BL1–BL12) on the branch
`claude/behaviour-lab`. Verified on Linux only (GCC 13.3, Qt 6.4.2 offscreen). **Before merging:**
open a PR so `native.yml` runs `behaviour-lab-ui` and the core `circuit`/`lab` groups on Windows.
**Owner's look:** Simulation → *Behaviour Lab…*; run at 1×–5×; change `additiveSafetyDistance`
and watch the gap chart move without a reset; change lanes or `straightLength` (restarts); click
another vehicle, then *Follow tuned vehicle*; *Apply to project*, then Undo; switch to Thai.
D166 (same branch): the default Urban circuit — judge the layout, the X2/X3 left turns and the
heading-up camera (toggle *Heading up*); change `green` (live) and *layout* → Oval (restarts).
Give-way intersections in the lab wait for a circuit wrap rule for conflict zones/priority rules.
Next lab slices, one per session and only on the owner's say: W74 parameters in the panel (needs
`retuneSimulation` to allow a prototype → W74 change, so traits must exist), then lane-change fields
(needs a lane-change wrap rule on circuits first, BEHAVIOUR_LAB §1.1). After the review, return to
the queue below.

## GUI interaction review (2026-10-13, D151)

The owner's Routes/reflow/tooltip/wheel fixes are implemented; contract in
[EDITOR_WORKFLOW](reference/EDITOR_WORKFLOW.md). Check native CI on the GUI PR before
merging. Owner appearance review: create a Route to the middle of a curved road,
move the pointer within that road, save/reopen and Undo; narrow/widen Properties in
Thai and English; hover long descriptions near each window edge; wheel over focused
and unfocused settings in panels and dialogs. Settings must stay unchanged while
panels scroll and canvas zoom works. The destination marker is presentation only;
vehicles still travel the compiled Route. Existing milestone/validation gates stay open.
After these checks return to the unchanged owner queue below.

## OSM background review (2026-10-12, D150)

The current owner request adds M1.28 ([contract](reference/MAP_BACKGROUND.md),
[plan](plans/OSM_BACKGROUND.md)). The first commit's five native CI jobs passed,
including Windows desktop/core. The detailed [review](audits/OSM_REVIEW.md) corrected
cache, viewport, anchor-range and packaged-database defects. Check CI on the latest
review commit in [PR #138](https://github.com/Warissakorn/trafficsim/pull/138) before merging.
MAP10 remains the owner's check: set a reference on a real study,
trace a known road, pan/zoom, save/reopen and judge alignment/responsiveness.
No automated check closes survey accuracy, M1.23 interchange or existing engine gates.
After that review, return to the queue below; M8 is code complete (M8.1–M8.6b, D153–D157, D160–D162; B11 seed mixing D159); its gate is the owner's.

## Queue — owner's answers to the 2026-10-09 review (D143)

The owner accepted every recommendation, R1–R8 ([review](audits/ROADMAP_REVIEW-2026-10-09.md) §8,
[D143](decisions/RECORD.md#d143)). One slice per session, contract and rows first for engine work:

1. **M5.7 rehearsal — prepare only.** It is the owner's ([sitting](plans/OWNER_SITTING.md) A3).
   A session may ready a copy of the M2.6 template for the owner's real counts (sitting B4) and a
   checklist; it cannot close it.
2. **M5.9 evaluation cool-down — implemented 2026-10-09 (D146).** Schema 27
   `evaluation.cooldown`; new projects 900 s; old files byte-identical
   ([evidence](evidence/m5.9-cooldown.md)). Native CI green on PR #135, Linux and Windows. Left:
   whether the shipped `four-leg-signalised` example gets a cool-down is the owner's
   ([sitting](plans/OWNER_SITTING.md) B8).
3. **M4.2 amber stop-or-go — implemented 2026-10-09 (D147).** Catalog `amberDeceleration` 3.0,
   continuous check, commitment at red; M0 keeps amber as red, so the frozen fixtures did not move.
   Four-leg clamps 220 → 1 over 40 seeds, every movement's delay down 1.4–7.5 s
   ([evidence](evidence/m4.2-amber.md)). Native CI green on PR #135, Linux and Windows. Left: the
   owner closes the milestone.
4. **M6.0 benchmark option sheet — delivered 2026-10-10** (R2 b, docs only):
   [m6-benchmark-options](evidence/m6-benchmark-options.md), options for kinds S1, S2 and U with
   the command for each comparable figure, no engine numbers and no tolerance. The owner answers
   by letter ([sitting](plans/OWNER_SITTING.md) B9); the tolerance follows that answer and
   precedes any comparable run. No session runs a benchmark scenario before then.
5. **M5.8 scenario comparison (R1) — implemented 2026-10-10.** M5.8a, the CLI
   `--project BASE --compare ALT --seeds LIST` (D148, rows CMP1–CMP7), and M5.8b, the editor's
   Results → *Compare with…* (D149, rows EC1–EC7): the open project as base against a chosen file
   over the Seeds field, Copy and Export byte for byte the CLI's `--compare --csv`. Contract in
   [BATCH](reference/BATCH.md) §7. Verified on Linux (GCC 13.3, Qt 6.4.2, offscreen); Windows waits for
   `native.yml`. Left: the owner's look ([sitting](plans/OWNER_SITTING.md) C7), and the
   M5 gate, which stays open.

**The D143 queue has no session slice left** (items 1, 2's example cool-down, 3's closure and 4's
answer are the owner's). Next, as the owner chooses: the W74 cited preset and W99 contract
(below), or M3.4 (R7, §3 below). M8's code is done (below); without a choice, the next session takes the
**W74 cited preset and W99 contract**.
Motorcycles wait for M5.7 (R3; the question is in [PROBLEM](PROBLEM.md) §8).

**Multi-level modelling (D145): M8 code complete.** M8.1 contract (D153), M8.2 zone-layer
codec/commands (D154), M8.3 OD → routes/inputs (D155), M8.4a engine lot runtime (D156), M8.4b
authored ParkingLot and compiler (D157), M8.5 lot outputs (D160), M8.6a Zone tool and Zones tab
(D161), M8.6b OD matrices tab, lot dialog and Results lot table (D162);
[ZONES_AND_PARKING](reference/ZONES_AND_PARKING.md).
- **The gate is the owner's:** a development-access study run end to end, and sitting C8/C9.
- **Before merging** the branch `claude/m8.1-zones-contract` (no PR yet): open a PR, so that
  `native.yml` runs the Windows UI suites. Linux desktop passed 133/133 locally (Qt 6.4.2,
  offscreen).
- **M9** (dynamic assignment) is the next multi-level milestone. Its contract needs the owner's
  B7 answers (D158) only for M10/M11.
Known limits recorded: exit weights are one set per run (row summed over matrices); lot rows'
time intervals and compositions' intervals are not used for departures/initial types; the lot
report has one window (no sub-interval series) and `--compare` has no lot block.
**Owner's answers 2026-10-14:** B7 and B10 as recommended (D158; M8.5's gate wait is reported
apart from movement delay, which keeps it; the M10/M11 contracts can now be written, citing the
convergence defaults). B11 fixed (D159): seeds are mixed with splitmix64 before the first draw;
`trafficsim-cli 42` is now 45 completed / 31.75870154816726 s / 1 clamp; the TS fixtures replay
under `SeedStart::raw`, unmodified ([evidence](evidence/b11-seed-mixing.md)). **Check first:**
the Qt test `scenario-run-ui` pins the new CLI numbers and was not run here (no Qt); confirm it
in `native.yml` (Windows); on Linux the whole desktop suite passed on the D159 numbers (2026-10-14).

**Owner items** — gates, chat decisions and every desktop look — are in one sheet,
[OWNER_SITTING](plans/OWNER_SITTING.md) (R4). After the sitting, items still open and older than
14 days become ROADMAP carve-outs. **Native CI** of M5.5 (D134), M5.6 (D141) and D142 is checked on their PRs; M5.9 (D146) and
M4.2 (D147) were green on PR #135; M5.8b (D149) has no native CI
run yet.

## Open engineering notes from 2026-10-05/06 (non-owner parts)

What was built is history: [ROADMAP](ROADMAP.md) M2.8/M3.2.4/M3.3, PROGRESS and the
[decision record](decisions/RECORD.md). No owner, merge or validation gate below is closed.

- **Driving behaviour** (D120, [plan](plans/DRIVING_BEHAVIOUR.md), [contract](reference/DRIVING_BEHAVIOUR.md)).
  Do not treat inverse selected-type headway as selected-type throughput (D122). W74 runs since
  D138 and is **not validated**; the [clamp trace](evidence/w74-clamps.md) found no W74 defect.
  Next: a cited, uncalibrated `w74` preset in `data/driver-behaviour/` (the fixture's 15 uncited
  values are not a source; rerun `trafficsim-w74-clamp-trace` with it), then the M3.3.3b W99
  contract before code. Keep prototype runs byte-identical (BA18).
- **Merge admission** (M3.2.4e): its own admission contract and failure-first clearance tests
  before runtime reservations widen to the physical mouth.
- **Demand** ([DEMAND_IMPROVEMENT](plans/DEMAND_IMPROVEMENT.md)): further distributions,
  exact-count or dynamic routing need their own contract. M2.1, M2.7 and M2.8 stay open
  sub-milestones of the closed M2 (R5).

## Session work without the owner

Pick one per session, as the user asks. Rows and contract come first for engine work.

- **Vehicle turning after phase 3:** rear-axle display, optional axle data and continuous
  lane-change guidance are implemented ([contract](reference/VEHICLE_POSE.md)). Next engine work
  needs separate acceptance rows for swept-body conflict clearance and between-lanes
  occupancy. Audit where `occupiedSpans`/conflict reservations use scalar length;
  reproduce a rigid heavy vehicle whose displayed swept body exceeds that envelope
  before changing admission.
  Steering feasibility and articulated trailer joints are separate slices. Keep the
  traffic front-bumper distance contract distinct from displayed axle coordinates;
  owner appearance review does not validate swept paths or traffic behaviour.

- **M3.2.8a.1 — moving merge anticipation.** D108 completes the source first-step
  slice (M3.2.8a.1a): queue entry waits until ordinary following/integration fits the
  current leader clearance. On D107 geometry, 120 stress runs remove 3 source clamps;
  21 moving minor clamps remain. Read [source evidence](evidence/source-first-step.md),
  the original [diagnosis](evidence/m3.2.8a-clamps.md) and ROADMAP first. Trace the current
  moving cases, then write anticipation/failure-first rows before changing motion.
  Preserve the buffer, D50 setback, maximum deceleration and genuine emergency reporting.
  Post-entry lane changes can introduce a new leader; the source guard does not cover that.
  Do not claim clamp-free or close M0/M3 owner observations.
- **Per-lane shares (D32; one weight per entry-Link lane since D71).** No canvas gesture sets one.
  The input table row shows set weights since 2026-10-03 (`laneSplit`).
- **Keyboard-only equivalents** of the route and vehicle-input gestures (M1.25/M1.26).
- **M3.4 — lane-change behaviour refinements** (the M3.2.8c remainder, carved out by R7):
  - Discretionary changes (D95, rows A47–A55; hold D101, rows A56–A58; both off): visibility at
    areas and a between-lanes state are unwritten. A change
    is instantaneous in the engine; the Run view uses a continuous rear-rolling spatial guide (phase 3).
  - `laneChangeDistance`, only on a network where changes are measured late (D87; D89 found
    left-turners change at the first tick allowed).
  - Vissim's cooperative lane change (a vehicle moving out of the way) is not modelled.
  - Free walk with no decision stays lane-fixed, and positioned recognition now follows M2.1.3
    ([contract](reference/POSITIONED_ROUTING.md)).
- **Conflict areas, later, not booked:** a branching (diverge) kind, red-red status, front/rear
  gap and visibility.
- **Split targetStanding (measurement only).** Is the target lane standing at its own red, or in
  a queue spilling back? Add it to `--wait-causes` only when a behaviour row needs it.
- **Engine cost after D90.** Linux/GCC 15.2 callgrind (2026-10-01): lane-change work ≈3%; the
  9.1% state copy is gone (D91).
  - Before any new baseline replaces 4.35G, run a **same-compiler** callgrind at `b472e05`,
    `f9964f1` and HEAD. The 4.01G was GCC 15.2 against D70's 13.3, so not comparable.
  - 2026-10-08, MSVC Release on P-cores, M2.6 CLI run (wall median 634 → 568 ms): `observe`'s
    per-line walk and queue matching (155 → 97 ms) and publish's vehicle copy (98 → 88 ms) are
    done. Split now: compile ≈116, step ≈316, observe ≈97 ms.
  - **Compile** (D163, 2026-10-14, Linux GCC 13.3 RelWithDebInfo, study template):
    `compileDocument` 45.9 → 30.2 M Ir, 6.1–6.7 → 4.0–4.1 ms; `routelessIssues` and Run's
    `expandRouteless` share one `RoutelessWalks` (walks 76 → 52 per compile) and each compile or
    validation builds `runtimeSections` once (7 → 2). **Owner check:** the MSVC Release
    compile figure (≈116 ms before) on the M2.6 CLI run. Still open: `validateDocument`'s own
    expansion is the third walk set (≈24 walks, sharing it with Run means proving its
    `withRoutingDecisions` input equals Run's), then phase 1 (≈110 ms). The
    `MovedEvent::segmentId` string copy would need an event-interface decision.
  - 2026-10-13, Linux GCC 13.3 callgrind, one M2.6 run (3.88 G Ir): step 80 %, observe 18 %,
    compile ≈2 % (D140 shrank it). **Do not retry** reusing the source-candidate refs/spans as
    the main snapshot without a new measurement: only 5,242 of 36,000 ticks qualify (no arrival
    inserted, no route decision), worth ≈1–1.5 %. Batches gain far more from D152 (seeds in parallel).
  - 2026-10-14 (D164), same build, M2.6 run 3,833.9 → 3,364.2 M Ir (−12.3 %; wall ≈0.41 s, noisy):
    signal events reuse `headColors`, queue lines walk only crossing routes (`observe` 713 → 404 M),
    `generateArrivals` checks due time first. Owner check: MSVC Release wall on P-cores. Left,
    measured: `closestVehicle` 8 %, `occupiedSpans` 9 %, the `MovedEvent::segmentId` copy 2 %
    (event-interface decision), `Vehicle` moves 2 %.
- **Linux replay of D91–D94:** CI (`native.yml`) is the evidence.
- **M3.2 closure rows:** a diverge acceptance-fixture row (R7; the D98 lab is a development bed,
  not evidence), then `plans/M3_ACCEPTANCE.md` §4, the rows a session can fill ([2026-10-03 review §7, S5](archive/ROADMAP-review-2026-10-03.md#7-proposed-sequence--owner-free-sessions-after-o1)).
- The entry-acceleration bias in movement delay needs travel-time sections (M5), not a correction
  factor.

## Working notes

- **Seeds.** Use at least 40 seeds (42–81) for any before/after comparison of one movement (D88).
  A right-turn movement's per-seed change has an SD of about 7 s.
- **The `b472e05` "before" recipe:**
  1. Make a scratch worktree and copy in the diagnostic's two files.
  2. Add its source to `trafficsim_eval`.
  3. Patch `runProject`, uncommitted, to observe it behind an environment variable.
  4. `cmp` the default output first.
- **Lane-change lab (D98):** `data/projects/lane-change-lab.traffic.json`, built from
  `tools/lane_change_network.hpp`, tested by `lanelab`. Any lane-change work iterates there first.
  - Scenes: Overtaking, Three lanes, Lane drop, Diverge.
  - Lane drop is the D96 case. It has no discretionary span, and the downstream second lane stays
    empty, so D95 does not close D96's run-side gap.
- **Build on this machine:** MSVC 14.51, Ninja and Qt 6.8.3 on D: (vcvars64, then
  `cmake --preset desktop`) gives Windows evidence.
  - **WSL is not installed here** (`wsl.exe -l -v`, 2026-10-03). Linux evidence comes from CI
    (`native.yml`) or another Linux/WSL machine.
  - On a 3.7 GB WSL machine, `ar` on `/mnt/c` failed with "Cannot allocate memory" at full
    parallelism; use `-j 4` there. On another, a one-file edit waited 150 s (134 s linking the 101 MB Debug
    `trafficsim-tests` on `/mnt/c`), and an unbounded `-j` hung a 3 GB WSL. The remedy: a build
    tree on ext4 via an untracked `CMakeUserPresets.json`, `CMAKE_JOB_POOLS=link=1` with
    `CMAKE_JOB_POOL_LINK=link`, `-j2`, and `--target` only what the change needs.
- **Timing on this machine:** the i5-12450H is hybrid. The same Debug run is 26–29 s CPU on
  P-cores (`ProcessorAffinity` 0xFF) and 89–104 s on E-cores (0xF00), 2026-10-04. Pin to one
  core type before comparing. Wall time under ctest swung 24–90 s on unchanged code.
- **Tests:** add every new model test group to `TRAFFICSIM_TEST_GROUPS`, or `all-model-tests`
  fails (D70). The longest test is `m26study_merge` (≈6 s, Linux). `scenario-run-ui` paints
  every 100th Step (2.8 s CPU on a P-core).
- **What M3 shows:** M3.1 supplied merge arbitration only, a deterministic gap-time/headway
  threshold, not a calibrated critical-gap model. Derived stop lines sit 1 m short of the join
  (D50). M3's done-condition (minor-road delay responds to gap time) rests on development
  evidence only; no gate result is inferred.

## Do not retry without a new measurement

- **Removing the M3.2.8b courtesy (D71):** without cooperation M2.6 was 61.6 s with a 404 s wait.
- **`comfortableDeceleration` for commitment (D69):** it clamped the major road.
- **The Run view "slowing down" over a run (2026-10-02):** this is the vehicle count warming up,
  not a leak. Per manual Step, cost rises 1.9 → ~3.8 ms over 12,000 steps of M2.6, and after
  Reset it restarts low and climbs the same way.
  - Scene items stay ≈270; the BSP index and the pending queue are ruled out. ≈67% is Qt widget
    painting (callgrind, Linux).
  - Windows (D99): painting was 93% of a Step. The grid cache and the hidden-Results skip halved
    12,000 Steps (107 → 57 s).
  - Left per Step: canvas ≈1.4 ms, window flush ≈1.1 ms, run label relayout ≈0.8 ms. Measure
    with `trafficsim-run-view-benchmark`, not by hand.
- **Setting the Run/Pause icon only when it changes:** layout stayed 2,439 → 2,438 ms per 3,000
  Steps, because the word-wrapped `editorRunInfo` label relayouts the window every Step anyway.
  Reverted. The lever is that label's layout, a UI change for the owner.
- **One `refreshRun()` per Step instead of two:** 2,169 → 2,342 ms per 1,000 Steps, inside the
  spread. Reverted. Qt paints once per event-loop pass.
- **`redraw()` copying only the primary Link:** −1.2% of `redraw()`, inside the clock's spread.
  What is left in a frame is M1.23's culling and LOD.
- **Publishing inline when a scenario has no zone (M3.2.3a):** 52.05M against 51.29M instructions
  for the three-phase loop. Reverted.
- **Not booked:** `compileDocument` (paid once per Run) and the `push_back` work left in
  `occupiedSpans`. The per-tick fleet sort is already a merge.
- **Measure the engine with callgrind, not the clock,** when resolving a few percent: wall time
  swung ±7%.
  - Baseline (2026-09-26, Release, after D70): the M2.6 one-hour run is 4.35G instructions,
    median 0.45 s wall.
  - Split: `stepSimulation` 66%, `observe` 26%, `compileDocument` 4%.
