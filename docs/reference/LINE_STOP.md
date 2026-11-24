# Stopping at the line, for every line and every model

Written 2026-11-24 (D651). The owner asked for the program to be as realistic and efficient as possible
("ถ้าต้องการให้โปรแกรมสมจริง มีประสิทธิภาพที่สุด").

This contract follows LINE_APPROACH's pattern (D254–D258):
- the options are measured before any code, in the [evidence](../evidence/line-stop-options.md);
- **the owner chooses** (§7);
- no engine change is committed with it.

Rule 4 applies: nothing here is validated, and the unvalidated marker stays.

## 1. The problem

The engine hands every line to the car-following model as a standing leader, `Leader{gap, 0, 0}`, in
`src/core/simulation.cpp`. The lines are signal heads on red or amber, give-way and Stop lines, a conflict zone's
waiting line, dead ends and lot gates. The hard cap, however, is the line itself.

So each model rests its own standstill gap short:
- the prototype by `standstillDistance` plus its speed term;
- W74 by `AX` up to `ABX(0.1)`;
- W99 aims at CC0 short.

STOP_LINES §2 defines the line as where the front stops. The only case that already rests there is the
prototype at a zone's line (D596), which is what the owner saw on the T-junction.

**Measured today (L0, seeds 42–51):**

| | Prototype | W74 fixture | W74 SUMO preset | W99 fixture |
|---|---|---|---|---|
| Rest short of a four-leg red, median | 2.20 m | 2.94 m | 3.36 m | 1.40 m |
| Rest short of a give-way / Stop line, median | 2.95 m | 2.94 m | 3.42 m | 1.40 m |
| Crawl below 1 m/s at the line, median | 10–11 s | 10–11 s | 13–14 s | 1 s |
| Startup lost time (four-leg) | −0.11 s | −0.65 s | −1.22 s | −0.11 s |

Negative startup lost time is the head start a car gets from standing metres short of its line. HCM's default is
2.0 s.

**Other programs separate the stop at a line from following:**
- SUMO uses `stopSpeed`, a safe-stop speed for the line, combined by the minimum with the follow speed.
- Vissim has a separate standstill distance for static obstacles.

## 2. The rule proposed

1. **The front stops at the line**, for every line type and every model.
2. **A line is not a leader.** Following reads vehicles; the line has its own law. The standstill gaps (CC0, `ax`,
   `standstillDistance`) are distances behind vehicles only.
3. **No clamp at a line.**
   - The line law ends exactly at the line.
   - A move that reaches the line cap finishes as GIVE_WAY_STOP §2's `stopAtLine` does.
   - A rounding residue at the line is rest, not a clamp (D596's finish).
4. **The amber rule is unchanged** (AMBER, D147). Whether a line holds a vehicle is decided as today; only how the
   vehicle stops changes.

## 3. Options measured

| Option | What | Measured result (evidence) |
|---|---|---|
| **L0** | Today | §1 |
| **L1** | D596 extended: every line handed to the model shifted by its standstill gap | Prototype at the line, but M2.6 clamps 225 per run; W74 still 1 m short and still crawls 10–14 s |
| **L1F** | L1 with D258's floor extended to W74 and W99 | As L1; fewer hard-braking ticks |
| **L3** | SUMO-style: a separate line law braking at the type's `comfortableDeceleration` to stop at the line; the lower of it and the model's acceleration | Every model at the line (median and p90 0 m); crawl about 0.5 s; startup lost time about +1.4 to +2.1 s; clamps about as L0 (the prototype on M2.6 0.1 → 0.3 per run), except W99 (§6); +1.1 % instructions |
| **L3F** | L3 with the extended floor | As L3; W74 and W99 hard-braking ticks roughly halved |
| **L2** | Vissim-style: a behaviour key for the distance at lines | Not measured: it needs a cited default (D178), and none is in hand. It could later set L3's target to `line − distance` |

## 4. The session's recommendation

**L3F, as a project switch on by default:**
- It is the one option that meets §2 for every model and every line type.
- It matches how SUMO separates the two.
- It removes the long crawls.
- It moves startup lost time toward HCM's range.
- Its cost is +1.1 % instructions on the four-leg.

## 5. Scope, switch and rows (for the code session, once chosen)

- **The switch.**
  - The key is `clampRules.lineRest`, beside D257's `standingStopFloor`.
  - Absent means on; the file writes it only when `false`, at the next schema version. Older schemas refuse it
    with `EDIT_UNSUPPORTED_FIELD`, and a non-boolean is refused with `INVALID_CLAMP_RULE`.
  - Run settings gains a checkbox for it.
- **M0 is untouched.**
  - As proposed, the rule applied only where `standingStopFloor` applies today: behaviours carrying
    `amberDeceleration`, set by projects and never by bare scenario files. §7 amends this to include every
    W74 and W99 behaviour.
  - So `multra-cli 42`, the four TypeScript reference rows and `crossing.json` keep their bytes, and D369 stays
    closed.
  - The measurements show that an unscoped rule would move bare M0 (1 → 5 clamps).
- **Rows** (BA239–BA245, failing first; each row first asserts that its forcing worked):

| Row | Case | Asserts |
|---|---|---|
| BA239 | Each model at a red head, a give-way line, a Stop line, a zone line and a dead end, arriving at 10 m/s | Rest within 10⁻⁶ m of the line; no clamp; deceleration within the type's comfortable value except where the hold appears too late |
| BA240 | Each model released from rest at a line | No crawl: it is not below 1 m/s for more than 1 s while short of the line before the release |
| BA241 | A vehicle leader nearer than the line | Following as today: the line law is not the binding one |
| BA242 | A hold that appears inside the stopping distance | Capped at `maxDeceleration`; still a counted clamp or a committed pass (D105) |
| BA243 | A bare scenario; the switch off | Today's bytes: the pin, the reference rows, the four-leg and M2.6 with the switch off |
| BA244 | Stop service (`atStopLine`, `stopLineReach`) | Served at the line for every model |
| BA245 | Codec and editor | Round trip, older schema refused, non-boolean refused at its path, the checkbox as one Undo step |

## 6. What stays out

- **W99's arrival clamps.** About 320 per run on the four-leg and about 1,200 on M2.6, under every option. §4's
  creep keeps `max(aPrev, CC7) > 0` until the stop is infeasible. That is a W99 §4 question: creep only while it
  can still stop. It is its own slice.
- **Calibration** of the comfortable deceleration used by the line law.
- **M0, and the amber rule** (D147, D369).
- **L2's distance**, until a cited value exists.

## 7. The owner's choice and the implementation (D652)

The owner chose **L3F** on 2026-11-24 ("เลือก L3F ทำเลย").

- **The key.** `clampRules.lineRest`, the sixth clamp rule. Absent means on; the file writes it only when it is
  `false`, and that file is **schema 87**. A schema 86 or older file carrying it is refused with
  `EDIT_UNSUPPORTED_FIELD` at `definition.clampRules.lineRest`; a non-boolean is refused with
  `INVALID_CLAMP_RULE`. Run settings has a sixth checkbox, `lineRest`, ticked by default; confirming is one Undo
  step with the other run settings.
- **Scope (amended from §5).** `DriverBehaviour::lineRest` is runtime only. `createSimulation` sets it to the
  switch and (`amberDeceleration`, or the `w74` or `w99` model). So:
  - every W74 and W99 behaviour gets the law, as measured;
  - a prototype gets it only with `amberDeceleration`;
  - a prototype without that key is the frozen M0 law, so `multra-cli 42`, the reference rows and
    `crossing.json` keep their bytes.

  §5's "only where `standingStopFloor` applies" would have left a W99 behaviour made without
  `amberDeceleration` out.
- **Lines (amended from §2).** The law covers signal heads, give-way and Stop lines, zone lines and lot gates.
  **A dead end is not a line here.** Its approach is the lane change's, so it stays a leader as before. With
  dead ends as lines, the weaving lab's 150 m weave served 341 vehicles instead of 389 (seed 42): a changer
  kept its speed until the late stop and found fewer gaps. The lane-change lab, measured, had barely moved.
- **Engine.**
  - `src/core/line_stop.hpp`/`.cpp` (`lineStopAcceleration`, `applyLineStop`).
  - In `stepSimulation`, the model follows the vehicle leader (or the perceived one) when a line is the
    nearer obstacle.
  - The finish at the line: `stopAtLine` within the type's maximum, and a rounding residue of up to 10⁻⁶ m is
    rest.
  - **Against the measured L3F** (clamps, delay, headway and startup lost time, all four behaviours, seeds 42–51):
    - The T-junction and the priority lab are reproduced byte for byte.
    - The four-leg, M2.6 and the lane lab, which have dead ends, differ where those dead ends now stay
      leaders ([evidence](../evidence/line-stop-options.md#after-d652)). The largest change is the W74 SUMO
      preset on M2.6: delay 149.6 → 171.5 s against L3F, still below L0's 186.1 s.
- **Rows.** BA239–BA245 in `tests/line_stop_tests.cpp` (`linestop`), and the checkbox in `signal_ui_tests`.
  BA241 adds W99's floor behind a standing car (−1 m/s² where its closing law asks −0.95). Mutations bite:
  - the scope narrowed to `amberDeceleration`;
  - the finish at the line removed;
  - the floor removed.

  Two rows were amended once they ran:
  - **BA242:** a prototype with `amberDeceleration` that cannot stop goes, committed (D147); only W74 and W99
    are held and clamp.
  - **BA244:** a Stop rests a vehicle from below walking pace (M3.2.5), so it rests within 1 cm, not 10⁻⁶ m.
- **Rows kept on the old mechanics** (the switch off, each with a comment):
  - D258's `stopfloor` and the `mergeclamps` rows, which test rules that predate this one;
  - W74's composition rows (BA23–BA28) and W99 CC1's composition row;
  - the M6 U record and sweep (D543, D546);
  - red running's output row BA145. Its counted runs were cars **creeping at the line when red began**, and
    the law removes the creep.
- **Rows moved to the new behaviour:**
  - diverge A68: its standing car is held at its own front, not 2.5 m ahead;
  - visibility VS5: the hold shows over 20 ticks, not 3, because the law brakes later.
