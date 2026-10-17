# Behaviour-key exposure: every level-4 key editable — 2026-10-17

Branch `claude/m3.4-lane-change-duration-slice1` on `a69debe` plus this change (D217). Local host:
Linux, GCC 13.3.0, Ninja, Qt 6.4.2 offscreen, 4 cores. `--preset desktop` (Debug) and
`cmake --build build/desktop --target check`. **Linux only.** Design:
[BEHAVIOUR_PARAMETERS](../reference/BEHAVIOUR_PARAMETERS.md) §5–§6.

## What was hidden

- **Behaviour editor:** perception (5 keys), lateral (3 keys, all or none) and
  `curveLateralAcceleration` were kept on a save but not shown. D213 found a save that dropped one.
- **Behaviour Lab panel:** it showed only the 5 prototype keys, amber and D90.

All of these are level-4 keys (D216): driver behaviour, local by road, and they apply to either
model.

## Rows

**`behaviour-library-ui`**
- The prototype edit of the copied behaviour shows seven rows, all unticked:
  `lookAheadDistanceSet`, `observedVehiclesAheadSet`, `lookBackDistanceSet`,
  `rearTimeToCollisionSet`, `reactionTimeSet`, `lateralSet` (one box for the three lateral keys) and
  `curveLateralAccelerationSet`.
- Ticking look-ahead 150, observed 2, reaction 0.5, lateral 0.5 / 1 / 1 and curve 3 writes exactly
  those values to the copy and nothing to the original.
- The W74 fixture now carries curve 2.5 and reaction 0.4. Its edit shows both rows ticked. A curve
  edit to 2 is saved with the reaction time kept. A second edit that unticks the reaction time
  removes it.
- Failed first with `FAIL: lookAheadDistanceSet`.

**`behaviour-lab-ui`**
- Both panel tabs (`focus.` and `traffic.`) have the same seven rows, unticked. With nothing ticked,
  the panel's settings equal the run's.
- Ticking `focus.curveLateralAcceleration` 3 and `focus.observedVehiclesAhead` 2 reaches the
  running settings.
- *Apply to project* writes both to the project's car behaviour. Before the change, `applyLabFocus`
  copied only the prototype keys, amber and D90.
- Failed first with `focus.lookAheadDistanceSet`.

**`lab` (`d217_curve_perception_and_lateral_keys_run_on_the_circuit`)**

Can a Lab run use each key? The panel shows a key only where it can (§5).
- **Curve:** with the key on, the Urban circuit compiles 2 curved segments: the X2 and X3 left turns,
  radius 4.7 m. The Oval compiles 4: its 4 m joins, radius 61 m, where they meet the arcs. The Links'
  own arcs carry none, because SPEED_CONTROL slice 1 reads Connector geometry only. The curve
  checkbox's tooltip (`labCurveNote`) says so.
- **Each group on its own** (curve; perception with observed 2 and look-ahead 150; lateral 0.5 / 1 / 1):
  a 2-lane Urban run reaches at least one lap in 1200 s with no clamp. Cross-sections are compiled
  only for the lateral run.
- **Mid-run:** ticking lateral, curve and reaction 0.5 at once gets applied, not unchanged. The
  cross-sections appear, and 300 s more run with no clamp. A new reaction-tick count makes the Lab
  restart (BEHAVIOUR_LAB §2's `RETUNE_SHAPE_CHANGED`).
- This row passed as soon as it was written: it measures existing engine behaviour, which is why all
  three groups are offered in the Lab. A first version expected the Oval to have no curved segment.
  The run showed its joins at R 61 m, so the row now states that.

`check` **166/166**.

## Not offered in the Lab

- The D95 keys and `kerbLaneBias`: the Lab clears lane changes (`lab_run.cpp`) until the circuit's
  lane-change wrap rule exists.
- W74: the Lab tunes prototype behaviours only.

## Shipped runs

No engine, codec or data file changed; the change is in `src/shell` and the locale files. The CLI
and the projects do not read either, so shipped outputs cannot move. `trafficsim-cli 42` is byte-identical to
the D210 baseline.
