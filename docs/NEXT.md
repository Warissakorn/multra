# NEXT — what a session does first

The single live to-do for this project. [`PROGRESS.md`](PROGRESS.md) is the session history; the
[decision record](decisions/RECORD.md) stores the reasoning; **this file is the part a session must read before starting.**

**Write the next session's work here, not into a new `PROGRESS.md` entry.** Two copies of what
to do next is the duplication hard rule 3 forbids, and the copy that rots is always the one in
the log. Rewrite this file; do not append to it. Owner items live only in
[OWNER_SITTING](plans/OWNER_SITTING.md); dated measurements and reverted ideas live in
[ENGINEERING_NOTES](reference/ENGINEERING_NOTES.md).

## State on 2026-10-19

- **Red running follows the owner's answers to D241** (D246, D247, [RED_RUNNING](reference/RED_RUNNING.md) §3,
  [evidence](evidence/red-running.md#d247-the-owners-answers-to-d241-2026-10-18)):
  - a runner is let through anew only when it cannot stop gently (`amberDeceleration`, else the type's
    `comfortableDeceleration`);
  - once let through, the head does not hold it again during that red while it moves (`SimState::redReleases`);
  - its crossing is counted even after the window.

  Shipped outputs are byte-identical. With a 5 % share, M2.6's clamps return to the no-key 41.
- **The merge clamp switches are in** (D250, D251, [MERGE_CLAMPS](reference/MERGE_CLAMPS.md) §3,
  [evidence](evidence/merge-clamps.md#d251-the-package-implemented)).
  - **What they are:** `clampRules{leaderTravel, sideEntryAtLine, redLineStop}` on the project. All
    three are on by default, written at schema 45 only when one is off, and set by three Run settings
    checkboxes.
  - **Effect:** M2.6 has 41 → 17 clamps, with delay, trips and overlaps unchanged. With all three
    off, every output is byte-identical to before. The pin and the TS reference rows are unchanged.
  - **Rows:** BA158–BA167. D105's row keeps its clamp with `sideEntryAtLine` off.
  - **Still open:** the 17 left (pull-out 9, late catch-up 4, merge creep 3, 1 new), the lane lab's
    1 clamp (traced, D252) and source clearance (D108). The gate stays open.
- **`joinAnticipation` is in, off by default** (D254, D255, [JOIN_ANTICIPATION](reference/JOIN_ANTICIPATION.md) §5):
  the owner chose D253's A4 anyway. Absent means off; written only when true at schema 46; a fourth
  Run settings checkbox; rows BA168–BA172. On, it reproduces D253's A4 numbers; off, every shipped
  output is byte-identical to D251.
- **D256 (docs only):** stopping cleanly at a line, measured with eleven reverted prototypes.
  D227's floor, scoped to `amberDeceleration`, cuts M2.6 17 → 5 and keeps the four-leg, lane lab,
  pin and TS rows; it is recommended as a fifth switch and awaits the owner.
- **D252:** the lane lab's one clamp is a follower under-braking behind a leader stopping hard at
  a dead end (D241's follower class), not a merge.
- **D253:** anticipation across a join, measured with eight reverted prototypes. The workable variant
  (A4) nets M2.6 17 → 16 and moves T-junction delay from minor to major (7.069 → 7.545 s). It is
  not recommended ([JOIN_ANTICIPATION](reference/JOIN_ANTICIPATION.md)).
- **D249** traced all 41 clamps and measured seven options with reverted prototypes. Its red-head
  prototype braked with the wrong distance; D251's numbers replace it.
- **D248:** the owner kept the window as an upper bound and the count as it is; the contract now
  says so.
- **Finding (D247):** on the shipped projects the window no longer changes anything. The 1, 2 and 3 s
  copies give identical outputs. Every run starts at, or 0.3 s after, red onset. The reading (not probed
  further): a vehicle meeting red later has been stopping gently since amber, so only one that went on
  amber is still unable to stop gently. 6 of M2.6's 15 counted
  runs would have crossed as committed anyway.
- **PR #177 is merged** (D207–D245). Every job on its final head was green, `windows-desktop` included,
  which confirms D245 (libthai) and D205 on Windows. D242–D244 stay as robustness fixes.
- **D241 (no code change)** traced M2.6's clamps added by red running; D246/D247 removed them.
- **Give-way line stop is in** (D239, D240, [GIVE_WAY_STOP](reference/GIVE_WAY_STOP.md), [evidence](evidence/give-way-stop.md)):
  on a tick that would clamp at a binding give-way line, a stop within the type's maximum brakes to
  the line instead. The shipped Yield T-junction's 6 clamps at seeds 42–81 go, with delays
  unchanged; every other shipped output is byte-identical.
- **Tier B is complete.** **B-9 is in** (D237, D238, [RED_RUNNING](reference/RED_RUNNING.md), [evidence](evidence/red-running.md)):
  `redRunningShare` and `redRunningTime` on the driver behaviour (schema 44); a hashed share of
  drivers pass red only in its first seconds while moving; a counted `red-running` event; two
  dialog rows. Shipped runs byte-identical. No crossing traffic reacts (no conflict areas at the
  shipped signals).
- **B-4 is in** (D235, D236, [DRIVER_FACTOR](reference/DRIVER_FACTOR.md), [evidence](evidence/driver-factor-spread.md)):
  `driverFactorSpread` on the driver behaviour (schema 43) sets the factor's standard deviation,
  read from the type's own behaviour when a vehicle is made; absent = 0.15; the random stream is
  unchanged; behaviour-dialog row. Shipped runs byte-identical. Mean delay does not move with the
  spread; a wide spread adds clamps.
- **B-7 is in** (D233, D234, [LANE_PREFERENCE](reference/LANE_PREFERENCE.md), [evidence](evidence/lane-preference.md)):
  `kerbLanes` on the vehicle type (schema 42) splits that type's entry only over the kerb-most N
  lanes; mandatory changes unchanged; type-dialog row. Not where an entry routing decision chooses
  lanes by destination (so M2.6 does not move). Shipped runs byte-identical.
- **D232 (no code change):** D231's Stop-line clamps are give-way line clamps, not the Stop rule's
  ([evidence](evidence/give-way-line-clamps.md)). A minor vehicle creeping off its line meets a gap
  that closes while it can still stop; the braking law overshoots. The shipped Yield T-junction shows
  6 at seeds 42–81. Booked in M3.2.8a.1 (contract and rows first; each option moves shipped output).
- **B-8 is in** (D230, D231, [STOP_DWELL](reference/STOP_DWELL.md), [evidence](evidence/stop-dwell.md)):
  `stopDwellTime` on the driver behaviour (schema 41) rests a vehicle at a Stop line for whole ticks
  of at least that time; absent = today's one tick. Shipped runs byte-identical; +0.25–0.41 % Ir.
- **B-5 is in** (D228, D229, [SPEED_DISTRIBUTION](reference/SPEED_DISTRIBUTION.md), [evidence](evidence/speed-distribution.md)):
  an optional piecewise cumulative desired-speed curve on the vehicle type (schema 40), the same draw,
  the share as the quantile decisions and areas map linearly; type-dialog curve box; Lab fit. Shipped
  runs byte-identical, +0.06 % Ir.
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
- The run casing costs paint time (D199: ≈ +9–16 % in a Debug offscreen run); a Release measurement
  is still owed if the owner reports slowness.
- `map-ui` failed once in a parallel `check` on 2026-10-16 and passed in every rerun (archived PROGRESS D196
  entry); if it fails again, capture its output before re-running.
- M3.5 is implemented through BA38 (D181–D186).
- Local evidence is Linux (offscreen Qt 6.4.2); no Windows or high-DPI verification is claimed.
- No milestone gate is closed by a session; ROADMAP holds every gate.

## Next session — ask the owner (D256's choice)

The line-approach contract is written with measured prototypes ([LINE_APPROACH](reference/LINE_APPROACH.md)).
Ask the owner before any code:
1. **F1a** (the session's recommendation): D227's floor for behaviours with `amberDeceleration`, as
   a fifth `clampRules` switch, `standingStopFloor`. It cuts M2.6 17 → 5 clamps with the pin and
   TS rows kept. Ask whether it is on or off by default. Then write rows BA173–BA178 failure-first,
   amend BA164's forcing to switch it off, and implement.
2. **F2**, a floor behind a braking leader, which would remove the lane lab's clamp. It is not
   recommended: it reshapes ordinary braking everywhere.
3. **Neither:** the remaining clamps stay traced and open.

After it: **M3.3.3b, the W99 contract,** before any code (ROADMAP M3.3 "Next", D177). M2.8 has only
owner checks left. Without an answer, the session asks again rather than choosing a rule change.
