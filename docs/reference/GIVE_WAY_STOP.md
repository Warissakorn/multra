# A feasible stop at a give-way line (M3.2.8a.1 slice)

Written 2026-10-18 (D239); implemented the same day (D240,
[evidence](../evidence/give-way-stop.md)). This is a slice of ROADMAP M3.2.8a.1, the remaining moving clamps. It
addresses the give-way line clamps that D232 traced
([evidence](../evidence/give-way-line-clamps.md)). It is a contract before code, as the milestone's
gate requires. No clamp-free claim follows from it, and no gate is closed.

## 1. The problem and the owner's choice

A minor vehicle that has stood at its give-way line moves off into an open gap. A few ticks later a
major vehicle comes within the zone's headway or gap time. The vehicle can still stop at its type's
maximum deceleration, so it is not committed (D69), and the line holds it again.

The prototype's braking softens near a standing target, so the move it wants passes the line by
millimetres to centimetres. Phase 1 then clamps the move to the line and reports a safety clamp.
The deceleration actually needed was at most 7.9 m/s², below the car's 8 m/s².

The shipped Yield `t-junction-priority` shows 6 such clamps at seeds 42–81. A Stop copy shows 8–15.

The owner chose on 2026-10-18: **brake to stop at the line.** A vehicle that can stop within the gap
at its type's maximum brakes exactly hard enough to stop there, and no clamp is reported.

## 2. The rule

In phase 1 of the step, for one vehicle on one tick:
1. **The line gap.** It is the nearest gap at which a priority rule or a conflict zone holds the
   vehicle this tick: a minor's waiting line, or a zone entry held for another holder. Signal heads,
   dead ends, lot gates and courtesy holds are not give-way lines.
2. **It applies only when all of these hold:**
   - the line gap is the binding cap, equal to `allowedDistance`, so no nearer vehicle or other
     hold caps the move;
   - the move from the following model would pass it (`move.distance > allowedDistance`), so today
     the tick ends in a clamp;
   - `speed > 0` and `speed² ≤ 2 · maxDeceleration · gap`, with the vehicle type's maximum.
     `commitmentDeceleration` (D219) is not read here.
3. **Then** the vehicle brakes at `a = −speed² / (2 · gap)` for the tick:
   - the move is that braking's integrated distance. It ends at the line, past it by at most rounding,
     which the line tests already tolerate (`kPast`);
   - the speed is the integrated speed, 0 when it stops within the tick;
   - the mode is braking, and no `SafetyClampEvent` is emitted.
4. **Otherwise nothing changes.** A move that would not pass the line, a stop that needs more than
   the maximum, a nearer vehicle or another hold all keep today's path, clamp included.

Applied on consecutive ticks, the same deceleration ends the vehicle exactly at the line.

**Why only on a clamping tick.** D227 measured an always-on floor of this kind behind any standing
leader. It changed ordinary braking and added a creep clamp. Applied only where today's tick clamps,
every run that has no such clamp stays byte-identical.

## 3. What stays the same

- **Commitment (D69).** The rule never lets a vehicle pass a held line. It only replaces an
  impossible-looking clamp with the feasible stop it was.
- **Other caps:** phase-2 caps (`resolveRequests`), the standstill cap behind a vehicle (D105,
  D201 B5), the D108 source guard, and signal heads, including D227's red-onset case.
- **Following models.** The braking replaces the model's acceleration for that tick only, on both
  models. The W74 regime state stores the model's own decision, as a clamp does today.
- **Events and checks.** No event is suppressed: a stop beyond the maximum is still a clamp. Body
  and swept-overlap checks are unchanged.

## 4. Rows (BA146–BA151)

| Row | Case | Asserts |
|---|---|---|
| BA146 | A vehicle braking to a held line with no clamp today | Its trajectory is unchanged on every tick; the rule did not fire |
| BA147 | A minor at 0.5 m/s, 5 cm short of a Yield zone line whose gap closes | Stops at the line; no clamp; implied deceleration ≤ the type's maximum |
| BA148 | The same at a priority-rule stop line | The same |
| BA149 | The same at a Stop zone | The same |
| BA150 | A stop needing more than the maximum | Still clamped and reported |
| BA151 | A standing vehicle leader nearer than the line | Unchanged; the rule does not fire |

## 5. Gate items (ROADMAP M3.2.8a.1)

The evidence records:
- clamp accounting, before and after, for the shipped Yield T-junction and the lane-change lab at
  seeds 42–81;
- each movement's mean delay, before and after;
- the decelerations used on the replaced ticks;
- a same-build replay of one seed;
- the existing overlap checks passing.

The other shipped projects have no give-way lines and must stay byte-identical: the four-leg, M2.6
and `trafficsim-cli 42`.
