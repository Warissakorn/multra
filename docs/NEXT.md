# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the session history; the
[decision record](decisions/RECORD.md) stores the reasoning; **this file is the part a session must read before starting.**

**Write the next session's work here, not into a new `PROGRESS.md` entry.** Two copies of what
to do next is the duplication hard rule 3 forbids, and the copy that rots is always the one in
the log. Rewrite this file; do not append to it. Owner items live only in
[OWNER_SITTING](plans/OWNER_SITTING.md); dated measurements and reverted ideas live in
[ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md).

## State on 2026-10-17

- **PR #176** carries D207–D228 to `main` (#175 merged D206; `main` merged back in). Its Windows
  `properties-layout-ui` failure is also red on `main` (D205's open item); the sanitizer failure was
  BA85's dangling reference, fixed.
- **B-5 contract written** (D228, [SPEED_DISTRIBUTION](reference/SPEED_DISTRIBUTION.md)): a piecewise
  cumulative desired-speed curve on the vehicle type, schema 40, rows BA110–BA116; implementation next.
- **D227 (no code change):** D223's seed-47 red-onset clamp is AMBER §1's red rule with a slow
  arrival; the prototype's braking softens near the line ([evidence](evidence/speed-decisions.md#d227-the-seed-47-red-onset-clamp)).
  A braking floor was measured and not adopted (ENGINEERING_NOTES).
- **D226:** acceleration is capped while a curve or area bound holds (BA109); shipped runs
  byte-identical.
- **B-6 slice 2 is in** (D224, D225, [SPEED_DECISIONS §7](reference/SPEED_DECISIONS.md), [evidence](evidence/reduced-speed-areas.md)):
  reduced-speed areas (schema 39) over a Link stretch, by lane id, anticipated at a per-area
  deceleration; delay not counted inside; editor tab with lane boxes. No area byte-identical; +0.77 %
  Ir recorded (the hooks resisted three rewrites).
- **B-6 slice 1 is in** (D222, D223, [SPEED_DECISIONS](reference/SPEED_DECISIONS.md), [evidence](evidence/speed-decisions.md)):
  desired-speed decisions (schema 38), quantile kept, delay against the speed in force; editor tab via the
  selected Link. Reduced-speed areas are a contract only (§7).
- **B-3 is in** (D220, D221, [MERGE_DECELERATION](reference/MERGE_DECELERATION.md), [evidence](evidence/merge-deceleration.md)):
  `mandatoryDecelerationOwn` / `mandatoryDecelerationTrailingVehicle` (schema 37) in the mandatory
  safety tests; editor rows. Shipped projects barely move; no value ships.
- **B-2 is in** (D218, D219, [COMMITMENT](reference/COMMITMENT.md), [evidence](evidence/commitment-deceleration.md)):
  `commitmentDeceleration` (schema 36) at give-way lines, the lower of it and the type maximum; editor
  row. A low value raises clamps (D69's risk), so no value ships; clamps are not yet split by road.
- **Behaviour-key exposure is in** (D217, [evidence](evidence/behaviour-key-exposure.md)): perception,
  lateral (one box) and curve are off-until-ticked rows in the behaviour editor and the Lab panel;
  Lab *Apply* writes them back. The D95 keys and kerb bias stay out of the Lab (circuit clears lane
  changes).
- **M3.4 slice 1 is in** (D206, [evidence](evidence/lane-change-duration.md), BA48–BA53): with the lateral
  switch on, a lane change is a lateral move that remaps at the crossing. Two start rules of D204 were
  corrected from switched-on runs (window without the body length; a change under way holds its target
  place). No shipped project carries lateral keys, so every shipped run is unchanged (CLI pin
  byte-identical); BA39 holds.
- **Keep-to-kerb slice 1 is in** (D215, [evidence](evidence/kerb-bias.md)): `kerbLaneBias` (schema 35)
  with the D95 pair. A53 still fails with any bias, so D95 stays off in shipped data.
- **Settings scope designed** (D216, BEHAVIOUR_PARAMETERS §6): project, catalog default, vehicle type,
  driver behaviour (local by road), network object, per vehicle.
- **D213 (docs only):** [BEHAVIOUR_PARAMETERS](reference/BEHAVIOUR_PARAMETERS.md) proposes which
  behaviour values are settable now, should become optional keys, or stay fixed; decided on the
  owner's delegation with tier B ordered (D214). The [OVERTAKING](reference/OVERTAKING.md) keep-to-kerb contract is written
  (BA64–BA71, not implemented). The behaviour editor's W74 save now keeps the curve key.
- **M3.6 slice 1 is in** (D212, [evidence](evidence/curve-speed.md), BA56–BA63): with
  `curveLateralAcceleration` on a behaviour (schema 34), vehicles slow for a Connector's curve to
  `sqrt(a·R)`, anticipated at comfortable deceleration. No value ships (D178), so every shipped run is
  byte-identical. D212 also recovered D210's switched-off cost (`closestVehicle` inline again).
- **A straddler follows the lanes it covers** (D210, [evidence](evidence/straddler-leaders.md), BA55):
  `closestVehicle` (now `src/core/leaders.cpp`) and `perceivedLeaders` also scan the lanes a body
  covers, through each lane's only successor. Switched off byte-identical; switched on +6.0 % / +4.8 %
  Ir against `5847cb9`, inside LATERAL §11.
- **Switched-on cost is inside the budget** (D209): +3.5 % lab, +3.0 % four-leg Ir against `5847cb9`;
  outputs byte-identical.
- **D208: mid-change, a changer waits at the line** until the start's safety tests hold on the lane it
  would cover (owner's choice); switched-on clamps 2 → 0.
- **D207: a swapping pair of stubs resolves by the rear one yielding** (owner's choice). Shipped
  four-leg and M2.6 multi-seed outputs moved by ≤ 0.2 s mean delay; the CLI pin is byte-identical.
- M3.7.0's four slices are in (D202, D203); M3.7's gate stays open (ROADMAP).
- D205 (main's Windows-only UI reds) is on main; it is confirmed only by a green Windows run.
- The run casing costs paint time (D199: ≈ +9–16 % in a Debug offscreen run); a Release measurement
  is still owed if the owner reports slowness.
- `map-ui` failed once in a parallel `check` on 2026-10-16 and passed in every rerun (archived PROGRESS D196
  entry); if it fails again, capture its output before re-running.
- M3.5 is implemented through BA38 (D181–D186).
- Local evidence is Linux (offscreen Qt 6.4.2); no Windows or high-DPI verification is claimed.
- No milestone gate is closed by a session; ROADMAP holds every gate.

## Next session — B-5 implementation, then tier B

A session told only to "continue" takes item 1:
1. **Implement [SPEED_DISTRIBUTION](reference/SPEED_DISTRIBUTION.md)** (rows BA110–BA116), in its §6 order:
   engine rows first against a stub, then codec at schema 40, then the type dialog.
2. Then tier B in order: B-8 (stop-sign service time), B-7, B-4, B-9.
3. The red-onset clamp stays recorded (D227). Reopen it only if the owner wants slow-approach
   clamps gone; the candidate is in ENGINEERING_NOTES.
