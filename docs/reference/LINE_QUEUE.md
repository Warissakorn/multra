# Stopping behind a standing vehicle: one stop law with the line

Written 2026-11-25 (D655). The owner looked at F9 on package #149 (LINE_STOP §9). They found the queue's
followers crawling without end, and asked whether stopping at a line and stopping behind a car should be alike.
They chose **A** (this contract), then **B** (a softer, cited stop profile).

This contract follows LINE_STOP's pattern (D651):
- options are measured before code ([evidence](../evidence/line-queue-options.md));
- **the owner chooses** (§6);
- no engine change is committed with it.

Rule 4 applies: nothing here is validated, and the unvalidated marker stays.

## 1. The problem

Under `lineRest`, a vehicle at a line rests exactly there (D652). A vehicle behind a **standing vehicle** is
still left to its car-following model.

**Measured** (four-leg, seeds 42–51, per vehicle at 0.01–1 m/s):

| Model | Median crawl | p90 crawl | Vehicles crawling ≥ 10 s |
|---|---|---|---|
| prototype | 17 s | 53 s | 56 % |
| W74 fixture | 30 s | 106 s | 73 % |
| W74 SUMO preset | 36 s | 169 s | 75 % |
| W99 | 2.6 s | 3.8 s | 1 % (D654) |

- **The prototype's law only approaches rest.** On the four-leg, vehicle 8 is still moving at 0.013 m/s after
  40 s.
- **W74 creeps in its `ABX` band** (W74 §7).
- **This predates D652.** With the switch off, every car crawls, the first one included.

## 2. The rule proposed

1. **One law for a standing obstacle.**
   - Behind a vehicle at rest, the vehicle stops with LINE_STOP §2's law, exactly at the leader's rear less the
     behaviour's standstill gap (`standstillGap`: CC0, `ax` or `standstillDistance`).
   - This is the same law, braking and finish as at a line. Only the target differs: the line itself, or the
     rear less the gap.
2. **The model follows moving vehicles only.** When the leader moves again, the model takes over, as at a
   line when it turns green.
3. **A vehicle at rest at its target waits.** Within 10⁻⁶ m of the target (D596's tolerance), behind a
   standing vehicle, it does not accelerate. Without this, the residue of an exact stop makes it inch forward
   tick on, tick off.
4. **No clamp.** The law ends at the target, and the hard cap there finishes it as at a line (`stopAtLine`).

## 3. Options measured

| Option | What | Result |
|---|---|---|
| **Q0** | Today | §1 |
| **Q1** | §2.1–2 with a safe-speed bound behind moving leaders | Crawl gone for every model; the prototype clamps 435 / run on the four-leg |
| **Q2** | §2 (rules 1–4) | Crawl gone for every model. Clean for W74 and W99. The prototype clamps 303 / 1,100 per run (four-leg / M2.6) and jerks |
| **Q3** | Keep the model; the law finishes below 0.1 m/s | Crawl stays; clamps up for every model |
| **Q4** | Q2 for W74 and W99; the prototype gets D546's `standingTarget` lever | W74 and W99 as Q2. The prototype clamps 49 / 208 per run |
| **Q5** | Q2 with "standing" below 0.1 m/s | Clamps up for every model |

**Q2 for W74 and W99:**
- **Crawl:** median 30–36 s → 3 s, and vehicles crawling ≥ 10 s 73–75 % → 10 %. The W99 p90 is 3.8 → 2.5 s.
- **Clamps:**
  - W74 is equal: 8.9 → 9.2 on the four-leg.
  - W99 falls by 80–99 %: 39 → 8 on the four-leg, 150 → 33 on M2.6, and 2,987 → 31 on the weaving lab.
- **Delay:** equal or lower. M2.6's W74 is 131.9 → 122.6 s, and its SUMO preset 171.5 → 130.2 s.
- **Headway:** within 0.03 s.
- **Costs:**
  - About 3–4× more jerk ticks, from the hand-back to the model when the leader moves.
  - On M2.6, 5–14× more W74 hard-braking ticks (below −3 m/s²). They are not clamps, and their cause is
    not yet traced.

**The prototype has no clean option.**
- The law removes its crawl, but the hand-back to its following law at standstill range brakes up to the
  maximum, because its desired gap grows with √speed. So it clamps.
- Its crawl is the M0 law's own approach to rest, and that law is frozen for M0.

## 4. The session's recommendation

**Q2 for W74 and W99. The prototype keeps today's law, and its crawl becomes its own question.**
- For W74 and W99, Q2 meets §2, removes the crawl and most of W99's clamps, and lowers delay.
- For the prototype, every option measured trades the crawl for clamps.
- **Scope.** The rule applies where `lineRest` applies and the behaviour is W74 or W99, so the switch and the
  file are unchanged. The prototype, with or without `amberDeceleration`, keeps its bytes, and M0 is
  untouched.
- **Before code, trace Q2's W74 hard braking on M2.6.** If it is the hand-back, rows must bound it.
- **The prototype's crawl.** Its options are a separate contract, because they change the prototype's approach
  law:
  - leave it, and recommend W74 or W99 for realism;
  - give it an approach that ends at rest.

## 5. Rows (for the code session)

| Row | Case | Asserts |
|---|---|---|
| BA252 | A queue of four behind a red, W74 and W99 | Every follower at exactly 0 m/s, within 10⁻⁶ m of the standstill gap; no crawl over 1 s; no clamp. Forcing: the switch off crawls |
| BA253 | A moving leader | Ticks equal with the rule off (the law reads standing vehicles only) |
| BA254 | A vehicle at rest at its target behind a standing vehicle | Does not move for 600 ticks; moves the tick after the leader does |
| BA255 | The prototype, with and without `amberDeceleration` | Unchanged ticks (scope) |
| BA256 | The four-leg and M2.6, W74 | Hard-braking ticks bounded as the trace finds; clamps not above Q0 (became a measurement, §6) |

## 6. The owner's choice and the implementation (D656)

The owner chose **Q2** on 2026-11-25 ("เลือก Q2 ตามแนะนำ ทำเลย").

- **The trace first (§4).** M2.6's extra W74 "hard-braking" ticks were vehicles **at rest** publishing W74's −8 m/s²
  request. It moves nothing.
  - Counting only ticks where the vehicle moved faster than 0.1 m/s the tick before, Q2 brakes hard *less* than
    Q0 on M2.6: the W74 fixture 801 → 728 per run, the SUMO preset 746 → 592.
  - On the four-leg: the fixture 269 → 220, the SUMO preset 155 → 187.
  - BA256 is therefore this measurement, not a row.
- **Engine** (`src/core/simulation.cpp`, `stepSimulation`).
  - `queueStop` is `lineRest` and (`w74` or `w99`).
  - A vehicle leader at exactly 0 m/s adds its rear less `standstillGap` to the nearest line.
  - The model then sees no leader. Rule 3's hold comes after the reaction delay, as the hard cap does.
- **Against the measurement.** On six projects, all four behaviours and seeds 42–51, the code reproduces Q2 to the
  digit: crawl, clamps, delay, headway and startup lost time.
  - Prototype rows are unchanged.
  - `multra-cli 42` is byte-identical.
- **BA231/BA27 sweeps** ([W99](../evidence/w99-discharge.md#after-d656), [W74](../evidence/w74-discharge.md#after-d656)):
  - W99 clamps 42 / 25 / 32 → 8.0 / 8.2 / 8.0 per run, and available headways 1,342 → 1,416 at dt 0.1. Headway is
    unchanged.
  - W74 headway 3.229 → 3.256 s at dt 0.1.
- **Rows.** BA252–BA255 in `tests/line_stop_tests.cpp` (`linestop`).
  - BA252's scene is spaced about 2 s apart. At 15 m spacing a W74 follower clamps once behind a leader still
    *braking* to its stop. Moving leaders are outside Q2 (§3), and such a leader is followed by the model as before.
  - BA241's W99 row is amended: behind a car at rest the law governs, not D258's floor.
  - Each of these mutations fails a row:
    - the model still following the standing car;
    - no hold;
    - the prototype in scope;
    - the rule off.
- **Left:**
  - the prototype's crawl (§4), now its own contract: [PROTOTYPE_QUEUE](PROTOTYPE_QUEUE.md) (D657), implemented as P17 (D662);
  - a leader braking to a stop (W74 can still meet the cap in dense scenes);
  - B, the stop's profile.
