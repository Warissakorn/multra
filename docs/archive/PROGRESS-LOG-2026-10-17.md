# PROGRESS log archive — 2026-10-17

Old PROGRESS entries, moved whole, newest first. Nothing here is current: NEXT and ROADMAP
hold status, the decision record holds the reasoning.

## 2026-10-18 — A wrapped Properties text asks only for its longest word (D244)

The Windows diagnostic named the widget ([evidence](../evidence/windows-ui-red.md#round-5-a-wrapped-texts-minimum-held-the-panel-d244--2026-10-18)).
- **Cause:** `editorEffectiveBehaviour` asked for a 257 px minimum on Windows (Qt 6.5.3), against
  69 px on Linux. That held the floating panel at 273 px, so the top forms never stacked.
- **Fix:** `ReflowLabel`'s wrapped minimum is its longest word, never more than Qt's.
- **Tests:** a check that every wrapped top text asks for no more than its word (plus 8 px). It
  passes on Linux before and after; the evidence that it fails is the Windows run. Inflating Qt's
  value fails without the cap and passes with it.

`check` 178/178, Linux only. Unconfirmed on Windows until `windows-desktop` runs.

## 2026-10-18 — Top Properties buttons elide (D243)

D242's diagnostic on Windows named the real cause ([evidence](../evidence/windows-ui-red.md#round-4-a-top-group-button-held-the-panel-wide-d243--2026-10-18)).
- **Cause:** the top forms' host was 273 px with no viewport. The appearance toggle's unwrappable
  text set the floating panel's minimum width (Linux 218 px, Windows about 273).
- **The owner's choice:** buttons shrink and elide with "…".
- **Fix:** `ElidingToolButton` (full-text size, ellipsis minimum, `Preferred` policy, elided
  paint) for the toggle, Apply behaviour type and Apply display.
- **Tests:** a long-toggle row failed first (the dock went to 299 px), and three mutations are
  caught.

`check` 178/178, Linux only. **Windows result:** the same failure (host 273 px), so the toggle was
not the cause there. The narrow check now prints the dock, body and tab widths and the widest top
widgets, so the next Windows run names the culprit.

## 2026-10-18 — Windows narrow Properties forms (D242)

The owner asked to fix CI ([evidence](../evidence/windows-ui-red.md#round-3-a-narrow-form-kept-a-row-beside-its-label-d242--2026-10-18)).
- **Failure:** `windows-desktop` failed `properties-layout-ui` (1×, 1.5×, 2×) with "A narrow form
  kept a field beside its label", on main too.
- **Cause:** a scrolled page is never narrower than its widest action button. On Linux the English
  Link page is 248 px in the 200 px dock, 8 px short of two columns. A slightly wider Windows button
  reaches them, so the form kept rows beside their labels.
- **Fix:** `FormReflow` decides by the visible width (the scroll viewport's when that is less) and
  watches the viewport.
- **Tests:** the new row failed first with Windows' message, and both mutations are caught. A row
  still beside its label in a narrow check now prints its form and widths.

`check` 178/178, Linux only. Unconfirmed on Windows until `windows-desktop` runs.

## 2026-10-18 — M2.6's red-running clamps traced (D241, no code change)

NEXT's item 1 ([evidence](../evidence/red-running.md#d241-the-added-m26-clamps)). A temporary probe
was never committed.
- **Where:** only seed 51 (3 → 5 at 2 s and 3 s) and seed 73 (0 → 1 at 3 s) change.
- **None is a red-head hold.** Every added clamp is bound by a vehicle leader:
  - Seed 51: a runner released at red onset, after braking for amber, meets the green flow at a
    derived merge rule. It is let go 0.55 m behind a leader and hits the standstill cap (D105,
    D201 B5); its follower clamps behind it.
  - Seed 73: a runner held again when the 3 s window ends stops at 6 m/s², and its follower needs
    10.6 m/s².
- **Found:** at 2 s the seed-51 runner crosses after the window as committed under
  `amberDeceleration`, so no event is counted. RED_RUNNING §3 now says so. Three §3 behaviours are
  listed as owner choices.

`check` 178/178, Linux only. CLI 42 byte-identical after the probe was reverted.

## 2026-10-18 — Give-way line stop implemented (D240)

D239's contract ([evidence](../evidence/give-way-stop.md)).
- **Rule:** on a tick that would clamp at a binding give-way line, a stop within the type's maximum
  brakes at `v²/(2·gap)` instead. BA147–BA149 failed first; the guards were proved by mutation.
- **Measured:** the shipped Yield T-junction's 6 clamps at seeds 42–81 go, with delays unchanged.
  Every other shipped output is byte-identical. The 11 replaced ticks braked at 2.5–7.4 m/s². Ir
  +0.11–0.15 %.
- **Correction:** D232's "none at seeds 42–51" was wrong (seed 46 has 2).
- **Sanitizer:** CI ASan caught BA151 reading a reference into a temporary scenario; the row now
  holds it in a local.

`check` 178/178, Linux only.

## 2026-10-18 — Give-way line stop contract (D239, docs only)

The owner asked for the easiest open item: M3.2.8a.1's give-way line clamps (D232)
([GIVE_WAY_STOP](../reference/GIVE_WAY_STOP.md)).
- **The owner's choice:** brake to stop at the line.
- **The contract:** only on a tick that today clamps at a binding give-way line, and only when the
  stop fits the type's maximum, the vehicle brakes at `v²/(2·gap)` and no clamp is reported.
  Everything else stays byte-identical. Rows BA146–BA151.

## 2026-10-18 — B-9 red-light running implemented (D238); tier B complete

D237's contract ([evidence](../evidence/red-running.md)).
- **Rule:** a hashed share of drivers pass a red head in its first seconds while moving; a
  `red-running` event when they pass the line; `redRunning` in reports only with the keys. BA139
  and BA144 failed first against a step that ignored the keys.
- **File and editor:** schema 44, refused below; the pair and ranges validated on both models; two
  behaviour-dialog rows. Every hook proved by mutation.
- **Measured:** shipped runs byte-identical. At a 5 % share runs are rare and rise steeply with the
  window (four-leg 2, 8, 24 in 40 runs for 1–3 s); delay falls within its half-width. Ir −0.44 %
  (layout).

`check` 177/177, Linux only.

## 2026-10-18 — B-9 red-light running contract (D237, docs only)

NEXT's item, the last of tier B ([RED_RUNNING](../reference/RED_RUNNING.md)). Today red holds
every vehicle that can still stop.
- **The owner's choices:** only in the first T seconds of red and only while moving; a fixed share
  of drivers decided once per vehicle by a hash; no new interaction with crossing traffic, but each
  run is counted.
- **The contract:** `redRunningShare` and `redRunningTime` as a pair at schema 44; W74's hash at
  index 15; a `red-running` event and a `redRunning` count written only with the keys; rows
  BA138–BA145.

## 2026-10-18 — B-4 driver-factor spread implemented (D236)

D235's contract ([evidence](../evidence/driver-factor-spread.md)).
- **Draw:** `samplePending` takes the standard deviation from the type's own behaviour, 0.15 without
  the key; the random stream is unchanged. Rows BA132, BA133, BA135 and BA136 failed first against
  the 0.15 stub.
- **File and editor:** schema 43, refused below; `INVALID_BEHAVIOUR_PARAMETER` on both models; a
  behaviour-dialog row. Codec, validation and the row proved by mutation.
- **Measured:** shipped runs byte-identical. Spreads of 0, 0.3 and 0.5 leave mean delay inside its
  half-width on all four projects; clamps rise with a wide spread. Ir ±0.015 %.

`check` 176/176, Linux only.

## 2026-10-18 — B-4 driver-factor spread contract (D235, docs only)

NEXT's item ([DRIVER_FACTOR](../reference/DRIVER_FACTOR.md)). Today every driver's factor is drawn
with a fixed spread of 0.15.
- **The owner's choices:** on the driver behaviour, from the type's own behaviour when the vehicle
  is made; the standard deviation only (the mean would repeat `additiveSafetyDistance`).
- **The contract:** `driverFactorSpread` in [0, 0.5] at schema 43; the same two draws, so the random
  stream is unchanged; drawn once, never redrawn by road; rows BA131–BA137.

## 2026-10-17 — B-7 lane preference by vehicle type implemented (D234)

D233's contract ([evidence](../evidence/lane-preference.md)).
- **Split:** `laneSplit` takes the type's `kerbLanes` in compile, routeless paths and the editor's
  input table. Rows BA125–BA129 failed first against a stub that ignored it.
- **File and editor:** schema 42, refused below; `INVALID_KERB_LANES`; a type-dialog row (failed
  first). Codec and hooks proved by mutation.
- **Measured:** shipped runs byte-identical. On the lane-change lab, heavy vehicles on the kerb lane
  add 0.3–0.4 s on their movements. M2.6 does not move: its entry decisions choose lanes by
  destination, which the contract now names. Ir −0.35 % (layout).

`check` 175/175, Linux only.

## 2026-10-17 — B-7 lane preference by vehicle type contract (D233, docs only)

NEXT's item ([LANE_PREFERENCE](../reference/LANE_PREFERENCE.md)). Today every type gets the same
per-lane entry split, so heavy vehicles spread across all lanes.
- **The owner's choices:** entry lane by type, on the vehicle type, the kerb-most N lanes.
- **The contract:** `kerbLanes` (1–16) at schema 42 renormalises the split over lanes 0 to N−1;
  mandatory changes and D95 unchanged; rows BA124–BA130.

## 2026-10-17 — Give-way line clamps traced (D232, no code change)

NEXT's item ([evidence](../evidence/give-way-line-clamps.md)). D231's Stop-line clamps are not the
Stop rule's: the shipped Yield `t-junction-priority` clamps the same way (6 at seeds 42–81).
- **What fires:** phase 1, on a minor vehicle creeping off its line at walking pace when a major
  vehicle comes inside the zone's headway or gap time. It can still stop at its maximum, so it is
  not committed (D69) and is held again at the line. The braking law overshoots by millimetres to
  centimetres. A few standing cases at dt 0.5 are the standstill cap (D201 B5).
- **Booked:** these are M3.2.8a.1's moving minor clamps. Its gate wants a contract and rows before
  any motion change, and each option moves shipped output, so nothing was changed.

## 2026-10-17 — B-8 Stop dwell time implemented (D231)

D230's contract ([evidence](../evidence/stop-dwell.md)).
- **Engine:** the Stop service entry records whole ticks of rest from the vehicle's behaviour when it
  comes to the line; one without the key. Rows BA118–BA123 failed first against a one-tick stub.
- **File and editor:** schema 41, refused below; validation on both models; a behaviour-dialog row
  (failed first). Codec hooks proved by mutation.
- **Measured:** shipped runs byte-identical. On a T-junction copy with Stop controls, a 2 s dwell
  raised minor delay by 13–18 s. Every clamp there sits at a Stop line, with or without the key
  (traced next). +0.25 % Ir on four-leg, +0.41 % on the Stop copy.

`check` 174/174, Linux only.

## 2026-10-17 — B-8 Stop dwell time contract (D230, docs only)

NEXT's item ([STOP_DWELL](../reference/STOP_DWELL.md)). Today a Stop rests a vehicle for 1–2 ticks,
so its effect depends on the time step.
- **The owner's choices:** a driver-behaviour key, absent = today (byte-identical), fixed seconds.
- **The contract:** `stopDwellTime` (s) at schema 41; the rest becomes whole ticks of at least
  that time, recorded when the vehicle comes to the line; rows BA117–BA123.

## 2026-10-17 — B-5 desired-speed distribution curve implemented (D229)

D228's contract ([evidence](../evidence/speed-distribution.md)).
- **Engine:** `speedAt`/`shareOf` in `speed_distribution.hpp`, the old uniform formulas without a
  curve; used by the draw, `quantileOf` and D191's rescale. Rows BA112–BA115 failed first against a
  stub; BA110/BA111 pin today's draw.
- **File and editor:** schema 40, refused below; `INVALID_SPEED_CURVE`; the type dialog's curve box
  and point table (BA116 failed first); the Lab moves a curve with min/max. Codec hooks proved by
  mutation.
- **Measured:** shipped runs byte-identical; a 50 km/h-centred car curve on four-leg moved mean delay
  46.00 → 45.40 s (inside the noise); +0.06 % Ir.

`check` 173/173, Linux only.

## 2026-10-17 — B-5 desired-speed distribution contract (D228, docs only)

NEXT's item after D227, read against ROADMAP M2.1 ("distributions") and tier B
([SPEED_DISTRIBUTION](../reference/SPEED_DISTRIBUTION.md)).
- **The owner's choices:** a piecewise-linear cumulative curve (Vissim's form), decisions and areas
  still linear on the driver's share, set on the vehicle type.
- **The contract:** optional `desiredSpeed.curve` at schema 40, ends equal to `min`/`max`; the same
  single draw inverted through it, so files without a curve stay byte-identical; rows BA110–BA116.

## 2026-10-17 — D223's red-onset clamp traced (D227, no code change)

NEXT's item ([evidence](../evidence/speed-decisions.md#d227-the-seed-47-red-onset-clamp)).
- **Cause:** AMBER §1's red rule meeting a slow arrival. Vehicle 58 went on amber, was 6.7 m short
  at red, could stop at its maximum and was held. The prototype's braking softens near the line,
  so the last tick overshot by 1.6 cm at 0.33 m/s.
- **Not the decision code:** it is idle on those ticks, and slow car types with no decision clamp
  the same way (seed 68). Shipped speeds give no clamp in seeds 42–81.
- **Candidate:** a `v²/(2·gap)` braking floor removed both red-onset clamps but added a creep clamp
  (seed 56). Not adopted; noted in ENGINEERING_NOTES.
- AMBER §1 gains the note. The CLI pin is byte-identical; no code changed.

`check` 172/172, Linux only.

## 2026-10-17 — Acceleration capped under the curve bound (D226)

NEXT's item ([evidence](../evidence/curve-speed.md#d226-acceleration-capped-while-the-bound-holds)).
D225 found that a speed target below the desired speed clamped only the speed, so the distance ran
`½·a·dt²` a tick ahead. D212's curve bound had the same gap.
- **Row BA109** (the BA57 fixture: each tick adds at most the trapezoid of its speeds) failed first.
- **One line in `simulation.cpp`** now caps acceleration for any bound below the desired speed, curve
  or area. It replaces D225's area-only line.
- **No key, no area:** byte-identical, and the Ir is identical too.
- **Key on:** delay moves by ≤ 0.04 s; t-junction clamps 3 → 1.

`check` 172/172, Linux only. CI on PR #176: the sanitizer failure (BA85's dangling reference) is
fixed. The Windows `properties-layout-ui` failure is also red on `main`.

## 2026-10-17 — B-6 slice 2: reduced-speed areas (D224, D225)

NEXT's item, contract first ([SPEED_DECISIONS §7](../reference/SPEED_DECISIONS.md),
[evidence](../evidence/reduced-speed-areas.md)). PR #176 opened for D206–D223 first.
- **The owner's choices:**
  - the anticipation deceleration is a per-area value, else comfortable;
  - delay inside an area is not counted;
  - an area can be limited to chosen lanes, named by lane id.
- **What changed:**
  - engine: `min(desired, area)` inside an area, anticipated before it, with an acceleration cap
    while the area binds;
  - area pieces in the free flow;
  - codec at schema 39, validation (including overlap), compile per lane section;
  - commands, including a lane-drop guard and the lane-id map on split;
  - an editor tab: select a Link, then *Add reduced-speed area*, with lane boxes.
- **Tests:** every engine row but BA97 and BA102 failed first. The rows also caught three faults in
  the first implementation; one of them, the missing acceleration cap, also affects D212's curve
  bound (listed in NEXT). Every codec and command mutation is caught.
- **Measured** with a 30 km/h area on the four-leg west approach: on one lane it slows only that
  lane's movement, by +8.6 s travel time and +1.9 s delay. No clamps.
- No area: byte-identical, +0.77 % Ir (hooks +0.5 %), recorded.

`check` 172/172, Linux only.

## 2026-10-17 — B-6 slice 1: desired-speed decisions (D222, D223)

NEXT's item, contract first ([SPEED_DECISIONS](../reference/SPEED_DECISIONS.md),
[evidence](../evidence/speed-decisions.md)).
- **The owner's choices:**
  - Vissim's model: per-type ranges at a sign, with each driver keeping their place in the range;
  - delay measured against the speed in force;
  - decisions before reduced-speed areas (areas written as a contract only).
- **What changed:**
  - engine, codec at schema 38, validation, compile per lane, and commands;
  - an editor tab: select a Link, then *Add speed decision*.
- **Tests:** the engine and UI rows failed first; the later rows were proved by mutation.
- **Measured** with a 30 km/h decision on the four-leg west approach: travel time +26 s but delay
  only +4–6 s. One red-onset clamp in ten seeds is recorded, not yet traced.
- No decision: byte-identical, hooks +0.10 % Ir.

`check` 170/170, Linux only.

## 2026-10-17 — B-3: accepted deceleration for mandatory lane changes (D220, D221)

NEXT's item, contract first ([MERGE_DECELERATION](../reference/MERGE_DECELERATION.md),
[evidence](../evidence/merge-deceleration.md)).
- **The owner's choices:** two keys, as Vissim has (`mandatoryDecelerationOwn`,
  `mandatoryDecelerationTrailingVehicle`), and the trailing one is read from the changer's behaviour.
- **What changed:**
  - mandatory changes only; the room condition still prevents clamps;
  - schema 37 and two editor rows.
- **Tests:** four rows failed first, and two mutations are caught. The third mutation showed the
  contract's `min(key, maxDeceleration)` could never matter, because following already clamps there,
  so it was removed.
- **Measured** in the shipped projects: the keys barely move anything (delays by ≤ 0.16 s, no
  clamps, the lab's one 2.5 s dead-end wait gone), because their demand rarely meets a refused
  change. Keys absent: byte-identical, +0.002 % Ir.

`check` 168/168, Linux only.

## 2026-10-17 — B-2: commitment deceleration at give-way lines (D218, D219)

NEXT's item, contract first ([COMMITMENT](../reference/COMMITMENT.md),
[evidence](../evidence/commitment-deceleration.md)).
- **The owner's choices:**
  - the key applies at give-way lines only, and the red head keeps the type's maximum (red-light
    running is B-9);
  - the lower of the key and the type's `maxDeceleration` wins.
- **What changed:**
  - `commitmentDeceleration` (schema 36, either model) in the give-way commitment test;
  - an editor row; no Lab row, because the circuits have no give-way line.
- **Tests:** five rows failed first, and three mutations are caught. The first scene clamped even
  without the key, so a sweep chose an 8 m scene.
- **Measured** with the key on every behaviour:
  - T-junction delay falls from 6.45 to 5.65 s at 2 m/s², while clamps rise from 0.2 to 0.8 per seed;
  - M2.6 clamps rise from 1.1 to 8.3, as D69 warned.

  No value ships. Key absent: byte-identical, +0.04 % Ir.

`check` 167/167, Linux only.

