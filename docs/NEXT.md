# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the session history; the
[decision record](decisions/RECORD.md) stores the reasoning; **this file is the part a session must read before starting.**

**Write the next session's work here, not into a new `PROGRESS.md` entry.** Two copies of what
to do next is the duplication hard rule 3 forbids, and the copy that rots is always the one in
the log. Rewrite this file; do not append to it. Owner items live only in
[OWNER_SITTING](plans/OWNER_SITTING.md); dated measurements and reverted ideas live in
[ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md).

## State on 2026-10-16

- **M3.7 slice 3b is done (D202,** [evidence](evidence/lateral-zones.md), BA47): with the switch
  on, a straddler's other-lane span blocks, is anticipated by, holds and sweeps that lane's zones.
  The straddler itself is not held at the other lane's zone (stated limit, D202). Only tests set a
  lateral target, so no product run moves sideways; BA39 holds (CLI pin, Release CLI byte-identical).
- The UI presentation slices a–e2 (D192–D200) and the sitting triage (D201) are on main. E13–E21
  wait on the owner, not a session.
- The run casing costs paint time (D199: ≈ +9–16 % in a Debug offscreen run); a Release measurement
  is still owed if the owner reports slowness.
- `map-ui` failed once in a parallel `check` on 2026-10-16 and passed in every rerun (PROGRESS D196
  entry); if it fails again, capture its output before re-running.
- M3.5 is implemented through BA38 (D181–D186).
- Local evidence is Linux (headless for D202; offscreen Qt 6.4.2 for the UI slices); no Windows or
  high-DPI verification is claimed.
- No milestone gate is closed by a session; ROADMAP holds every gate.

## Next session — M3.7 slice 4: display (BA46)

The owner chose slice 3b from the engine queue on 2026-10-16. A session told only to "continue"
takes the queue's next item below; if the owner names another item, that comes first.
1. **Slice 4, display** ([LATERAL](reference/LATERAL.md) §10 editor-pose row, BA46 in
   [DRIVING_BEHAVIOUR](plans/DRIVING_BEHAVIOUR.md)): the run view draws a body at lane centre +
   `lateral` on both driving sides (`src/editor/vehicle_pose.hpp`, `lane_change_pose.cpp`). Write
   BA46 first: a test-set `lateral` ≠ 0 on a left- and a right-hand network, drawn offset by exactly
   that amount, and display never feeding physics. Read `src/editor/README.md` and D97/D199 first.
   Qt is needed: install `qt6-base-dev libproj-dev nlohmann-json3-dev` (BUILDING) and build `desktop`.
2. Then **M3.4 lane-change duration**, the first product writer of `lateralTarget`: its contract
   decides whether the straddler must be held at the other lane's zone (D202's failure condition),
   and re-runs LATERAL §11 with real targets.
3. After that, the review's §5 order: M3.6 turning speeds, M3.4 overtaking with driving side, M3.8
   motorcycles, M3.5 attention, M3.3.3b W99 and M3.9 (D177/D178).

To build here: Ubuntu 24.04 needs `qt6-base-dev libproj-dev nlohmann-json3-dev lld` (BUILDING);
a cold Debug desktop build takes about 7 minutes on 8 cores, about 15 on 4.

## Deferred work — preserved from the preceding NEXT

The owner-directed alternatives below stay queued, not cancelled; take one only at the owner's
direction. ROADMAP retains every gate.

1. **M3.7 after slice 3b:** slice 4 and M3.4 are the next session's items above. The hand mirrors
   (`placeVehicle`, Lab telemetry, `dead_end_waits.cpp`) stay one-dimensional (LATERAL §10).
2. **Multi-level S0: the Node contract** (D175; ask the owner B12's Thai sources when S1 starts, D201). Polygon, mapping of Micro detail to
   legs/turns/conflicts/g/C, coarse values and lifecycle, merged with M5.1's evaluation nodes
   ([workflow](plans/MULTI_LEVEL_WORKFLOW.md) §3, §10). Then S1 StudyPeriod + PCU. M9 keeps its
   scope but no longer leads the order.
3. **M3.4 parts that need neither M3.5 nor M3.7** (R7): `laneChangeDistance`, cooperative
   lane change. Discretionary changes (D95/D101, off) reopen only by route (i) after M3.5's look-past (D201).
4. **Smaller engine or editor slices** — each has its context in ENGINEERING_NOTES §Session work:
   - vehicle turning beyond phase 3 (swept-body clearance rows first)
   - M3.2.8a.1 moving merge anticipation, with the standstill cap (D201)
   - drawing `level` is not separation (D201, M3_CONTRACT §1): `automatic_conflicts.cpp` skips
     pairs on different levels; failure-first row (two crossing Links on different levels get an
     area), then remove the skip and re-check every shipped project's areas
   - `four-leg-signalised` cool-down 300 s (D201, D146, [evidence](evidence/m5.9-cooldown.md)):
     its batch warnings clear; update its tests' expected numbers by that recorded decision
   - M3.2.4e merge admission contract
   - a per-lane share gesture (D32/D71)
   - keyboard-only route and input gestures (M1.25/M1.26)
   - M3.2 closure rows (a diverge fixture row, M3_ACCEPTANCE §4)
   - the third `validateDocument` walk set (D163)
5. **M5.7 rehearsal — prepare only:** ready a copy of the M2.6 template for the owner's real counts
   (sitting A3/B4) and a checklist; a session cannot close it.
6. **Behaviour Lab next slices, only on the owner's say:**
   - W74 parameters in the panel: needs `retuneSimulation` to allow a prototype → W74 change, so
     traits must exist
   - lane-change fields: need a lane-change wrap rule on circuits first (BEHAVIOUR_LAB §1.1)

   Give-way intersections in the lab wait for a circuit wrap rule for conflict zones and priority
   rules.

## Waiting on the owner (details in OWNER_SITTING)

D201 (2026-10-16) closed B1–B3, B5, B6, B8, B12 and B13 as proposed and carved the oldest looks into
ROADMAP M1.29 and M3.2.10. What only the owner can answer, in order:

- **A1 M0 plausibility** first: a *no* reorders the engine work (D13).
- **B4** the real study's counts, timing sheet and image, then **A3** the M5.7 rehearsal.
- **A2** the M3.2.7d T-junction exercise, which M3.2 closes on.
- **B9** the M6 benchmark choice, before any comparable run.
- **Milestone closures:** M4.2 amber (D147), M5 (M5.8 look C7 included) and M8 (a
  development-access study end to end, plus C8/C9).
- **Looks (C–F) still on the sheet:** C1–C3, C7–C12 (C12 is a timing, not a verdict), D2–D4,
  E2, E5, E6, E8–E11, the merged UI look E13–15, E16–E21, F1–F3; plus the carved M1.29 and
  M3.2.10 looks.
- Motorcycles are core scope (D177); they need M3.7 first (review §5).
- Items still open and listed more than 14 days become ROADMAP carve-outs (R4; D201 applied it once
  without waiting for the sitting).

## Known limits to keep in mind

- **Driving behaviour** ([review](audits/DRIVING_BEHAVIOUR_REVIEW.md) §2): one-dimensional
  vehicles, no lateral position, instantaneous lane changes, no turning-speed reduction.
  Perception limits, multi-anticipation and reaction time are optional; with no keys the old
  single-leader, unbounded-sight, zero-delay path remains. A second rear follower is not modelled.
- **M8:**
  - exit weights are one set per run (the row is summed over matrices)
  - lot rows' and compositions' time intervals are not used for departures or initial types
  - the lot report has one window
  - `--compare` has no lot block
- **Delay:** completed-trip delay includes source waiting and acceleration; it is not HCM control
  delay. The entry-acceleration bias needs travel-time sections, not a correction factor.
- **Before/after comparisons of one movement need ≥ 40 seeds (42–81, D88).**
