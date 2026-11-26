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
- It moves startup lost time toward HCM's range. **Withdrawn by D653 (§8):** that was a measurement artefact.
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
  can still stop. It is its own slice. **Done as D654** (W99 §4): about 39 per run remain on the four-leg and
  about 150 on M2.6, mostly behind slowly moving leaders.
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

## 8. Correction: the measurement at the line (D653)

The discharge, stop-line and priority-line observers counted a front exactly at a line as having passed it. Under
§2 the front stops at the line, so D652's queue heads were counted as crossing during red and dropped from their
cycle.

- **The rule.** `pastLine` (`src/eval/line_passage.hpp`): a front exactly at a line has not passed it. A line at a
  route's start is passed on insertion. An arrival is past every line on its route.
- **Rows.** BA246–BA248 in `tests/line_passage_tests.cpp` (`linepassage`). The old rule fails all three. The
  independent counter in PL1–PL9 (`priority_lines_tests`) takes the same rule.
- **What it changes.**
  - **Startup lost time.** §3's "+1.4 to +2.1 s" and §7's measured startup lost times were artefacts. Corrected,
    the switch lowers startup lost time by 0.3–1.0 s
    ([evidence](../evidence/line-stop-options.md#after-d653)).
  - **Unchanged:** the engine, clamps, delay, the rest at the line and the crawls.
  - **D652's choice stands** on those.
- **M0:** `multra-cli 42` is byte-identical.

## 9. The owner's look on Windows, and a queue behind a standing car (open)

**The owner, 2026-11-25, on package #149 (F9):** "ติ๊ก lineRest และรถคันหน้าจอดตรงเส้นแต่การหยุดไม่สมจริง; รถที่ต่อท้าย
ไม่จอดนิ่ง เหมือนกับว่าเกิดการคืบไม่มีที่สิ้นสุด; การหยุดที่เส้น กับการหยุดต่อท้ายคันหน้าควรเหมือนกันหรือไม่".

**Measured on Linux** (four-leg, seed 42, `lineRest` on):
- **The first car** brakes at a constant −2.0 m/s² from about 6.5 m/s and ends with a jump to 0. That is the law's
  constant comfortable braking (§2, as SUMO's `stopSpeed`).
- **The followers** spend 40–60 s (prototype) or 25–30 s (W74) at 0.01–1 m/s behind a standing car and never
  reach 0. W99 spends about 3 s (D654).
- **The crawl predates D652.** With the switch off every car crawls, the first one included (40–74 s).
- **The prototype's following law only approaches rest.**

**The owner's choice** (2026-11-25): A, then B.
- **A.** One stop law for lines and standing vehicles: the target is the rear less the standstill gap, for every
  model under `lineRest`, and M0 is untouched.
- **B.** A softer stop profile, from a cited source.

**A first attempt, not committed** ([patch](../evidence/line-stop-queue-attempt.patch)):
- **What it did.** Behind a standing vehicle, the model was replaced by the law, and a SUMO-style safe-speed
  bound was added behind moving leaders.
- **What worked.** BA252 (a queue of four comes to rest without a crawl, every model) passed.
- **What broke.** Nine groups failed, the lab circuit reaching 611 clamps, for two reasons:
  - **The switch on `speed == 0`.** A follower flips between the law (leader at 0) and the model (leader at
    10⁻⁴ m/s). The prototype's model then brakes at up to −8 m/s² near a nearly stopped car, so the follower
    alternates −8 and +2.5 each tick.
  - **Inching.** A car resting at its target inches forward by sub-millimetre rooms, tick on, tick off, so a
    queue never quite settles.

**Written as [LINE_QUEUE](LINE_QUEUE.md) (D655); Q2 implemented for W74 and W99 (D656).** A needed its contract measured first, as D651 did:
- the leader's state as a continuous input (no `speed == 0` switch);
- a start rule for a car at rest at its target;
- the safe-speed bound's place after the reaction delay;
- then rows and the sweeps.

## 10. A set-back at holding lines (D668, sources only)

The owner asked whether `lineRest` should stop a suitable distance short of the line. The sources are in
[line-setback-sources](../evidence/line-setback-sources.md):
- Vissim stops 0.5 m before a signal head by default;
- SUMO's `jmStoplineGap` is 1 m;
- neither cites field data.

An NGSIM look on an unverified stop-bar stand-in puts first cars about 1 m short. The recommendation is §3's L2 as a
key with default 0, so no result changes. Nothing is implemented.

The contract and its measured options are [LINE_SETBACK](LINE_SETBACK.md) (D669).
