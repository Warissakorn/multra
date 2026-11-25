# The stop laws under a reaction delay

Written 2026-11-25 (D659). D658 found that neither LINE_STOP's line law (D652) nor LINE_QUEUE's queue law (D656)
was measured with PERCEPTION §5's `reactionTime`. The owner chose to contract it for every model first
(PROTOTYPE_QUEUE §6, choice 1: "ตามแนะนำ").

This contract follows LINE_QUEUE's pattern:
- options are measured before code ([evidence](../evidence/reaction-stop-options.md));
- **the owner chooses** (§6);
- no engine change is committed with it.

Rule 4 applies: nothing here is validated, and the unvalidated marker stays.

## 1. The problem

With `reactionTime` τ, a vehicle applies the decision it made `d = floor(τ/dt + 0.5)` ticks earlier (PERCEPTION
§5). The line law and the queue law are part of that decision, so they are delayed with it.

**Measured** (seeds 42–51, `reactionTime` 0.5 s, `lineRest` on, against off):
- **W74 behind a standing car clamps.** On the four-leg, W74 clamps 755 per run, against 23 with the switch off.
  On M2.6 it clamps 3,277, against 91. Of the four-leg's clamps, 673 are behind a vehicle at rest: the applied
  decision dates from when that car still moved, and the law rests the follower at exactly `s0`, with no margin.
- **The prototype passes red lines.** Its delayed line law reaches the line too fast to stop. M4.2's `committed`
  rule (AMBER) then lets it go on red:
  - 128 passes per run on the four-leg (1.8 with the switch off);
  - 235 at 1 s;
  - 453 on M2.6.

  This is not a `RedRunningEvent`. Its delay falls by a third (48 → 32 s on the four-leg), which would read as a
  better junction.
- **W99 clamps under a delay with or without the line rules** (555 / 2,410 per run, four-leg / M2.6, switch off).

**Scope.** No shipped project sets `reactionTime`. Runs without it are not changed by anything here, and
`multra-cli 42` must stay byte-identical.

## 2. The rule proposed (R7)

For a vehicle whose behaviour has a reaction delay (`reactionTicks > 0`), under `lineRest`:

1. **The line law is not delayed.** After the delayed decision is taken from the FIFO, LINE_STOP's `applyLineStop`
   (the law and, for W74 and W99, D258's floor) is applied again on the current state, toward the nearest holding
   line. The lower acceleration wins.
   - A line's position is known in advance, so aiming at it is the driver's judgement. PERCEPTION §5 already keeps
     its hypothetical evaluations undelayed, and the hard cap always reads the current state.
2. **Behind a standing vehicle, the model follows, delayed.** LINE_QUEUE's queue law (D656) does not apply to a
   delayed vehicle, so its model follows a car at rest as with the switch off. Its own gap, wider than `s0`,
   absorbs the delay.
3. **Without a reaction delay nothing changes.**

## 3. Options measured

| Option | For a delayed vehicle | Result |
|---|---|---|
| R0 | Today | §1 |
| R1 | No queue law | W74 66 / 79 clamps on the four-leg; red passes stay |
| R2 | No `lineRest` at all | As the switch off: red passes 1.8–28, crawl at lines |
| R3 | The line law reapplied after the delay | Red passes 0; W74 206 / 239, W99 214 |
| R4 | The law aims short by `v·d·dt` | Barely better than R0 |
| R5 | R1 + R3 | W74 5.2 / 6.6; W99 444 |
| R6 | All of `applyLineStop` after the delay, lines and standing cars | Red passes 0; W74 148 / 174, W99 171. Clamps move behind leaders edging off below 0.5 m/s |
| **R7** | §2: R6 for lines, R1 for standing cars | Red passes 0; W74 5–8 (four-leg), 33–48 (M2.6) |

**R7 against the switch off and R0** (W74 fixture / SUMO preset, clamps per run):

| Case | Off | R0 | R7 |
|---|---|---|---|
| Four-leg, 0.5 s | 23 / 32 | 755 / 910 | 5 / 8 |
| M2.6, 0.5 s | 91 / 68 | 3,277 / 3,626 | 33 / 48 |
| Four-leg, 1 s (fixture) | 301 | 879 | 35 |

- **The prototype under R7:** 0 red passes everywhere, and delay back to the switch-off value (47–49 s on the
  four-leg). Clamps are about the switch-off count: 13 / 12 at 0.5 s, 311 / 277 at 1 s.
- **W99 under R7:** 325 / 1,417 (four-leg / M2.6), below the switch off (555 / 2,410) but above R6 (171 / 741).
- **Cost:** delayed W74 vehicles crawl behind standing cars again (four-leg medians 30–37 s), as with the switch off.
  Delay on M2.6 is 164 / 201 s, against R0's 150 / 176 and the switch off's 173 / 208.

## 4. The session's recommendation

**R7.**
- It removes the prototype's passes on red and W74's clamps under a delay, below even the switch off.
- It adds no value, and runs without a delay keep their bytes.
- **Against it:**
  - A delayed vehicle behind a standing car crawls again, as before D656. Q2's rest at `s0` cannot absorb a
    reaction delay, and every option that keeps it (R0, R3, R4, R6) clamps behind leaders that edge off and stop
    again.
  - W99 keeps hundreds of clamps under a delay. That is its model's own question (W99 §6 assumes an immediate
    action), measured here but not answered.
- **What comes after:**
  - PROTOTYPE_QUEUE's P17 would then apply to undelayed prototypes only, so the `lab` d217 clamps are out of its
    scope. Its three lost scene forcings and BA197's 4 % stay to be settled.
  - W99 under a reaction delay, if the owner asks.
  - B, the softer profile.

## 5. Rows (for the code session, if R7)

| Row | Case | Asserts |
|---|---|---|
| BA262 | A prototype car with `reactionTime` 0.5 s approaching a line that turns red | It stops at or before the line, with no pass on red and no clamp. Forcing: R0 passes |
| BA263 | A W74 queue behind a red, every car with `reactionTime` 0.5 s | No clamp. Forcing: R0 clamps |
| BA264 | A delayed W74 car behind a standing car | Its ticks equal the switch-off ticks while the leader stands (rule 2) |
| BA265 | No reaction delay, every model | Ticks unchanged; `multra-cli 42` byte-identical |
| BA266 | The four-leg and M2.6 at 0.5 s, W74 and the prototype | Clamps and red passes as measured (a measurement, not a row, if the scene cost is high) |

## 6. The owner's choice

Open. Options: **R7** (the session's recommendation), R6 (no crawl, more clamps behind leaders edging off), or
another.
