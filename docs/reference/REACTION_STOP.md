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

## 6. The owner's question, and R9 (D660)

The owner asked whether R7's crawl is a car inching after its leader because of the reaction time, and whether it
stops. They then asked for a rule that lets a delayed vehicle rest before choosing
([evidence](../evidence/reaction-stop-options.md#r8-and-r9-resting-a-delayed-vehicle-without-the-clamps-d660)).
- **The crawl under R7 is W74's own:** creep, stop, creep again.
  - Three quarters of it follows a leader also creeping below 1 m/s, and the head of each chain creeps toward a car
    at rest.
  - The amount is the same with no delay and the switch off.
  - A vehicle stops a median of 6 times (p90 22), against 4 under D656.
- **R6's clamps were mostly a residue at rest.** The follower was exactly at `s0` while its leader crept by
  micrometres. At 1 s, the rest of them is a start from rest that the delay carries on into a leader stopping again.

**R9, the rule now proposed** (in place of §2's R7). For a vehicle with a reaction delay under `lineRest`:
1. **The line law and the queue law are not delayed.** As R6: `applyLineStop` is reapplied after the delay, toward
   the nearest holding line or a vehicle at rest less the standstill gap. The lower acceleration wins.
2. **A hold at the target.** At rest within 10⁻⁶ m of the standstill gap behind any vehicle, the applied acceleration
   is at most 0. This is D105 with D596's tolerance, the delayed counterpart of LINE_QUEUE's rule 3.
3. **A start the delay commits to must fit.** A decision to start from rest with `a > 0` is stored as 0 while the gap
   beyond the standstill gap is below `a·τ²/2 + (a·τ)²/(2·maxDeceleration)`, the distance needed to stop again at the
   type's maximum after holding the start for the delay τ.
4. **Without a reaction delay nothing changes.** No value is added.

**R9 against the switch off** (`reactionTime` 0.5 s, clamps per run; the switch off in brackets):

| Model | Four-leg | M2.6 | Four-leg, 1 s |
|---|---|---|---|
| W74 fixture | 9 (23) | 44 (91) | 82 (301) |
| W74 SUMO preset | 39 (32) | 72 (68) | — |
| W99 | 8 (555) | 29 (2,410) | 32 (894) |
| Prototype | 15 (12) | 49 (51) | 233 (277) |

- **Delayed queues rest.** The crawl median is 2–3 s. A W74 or W99 vehicle stops a median of once (p90 at most 5),
  and almost no slow episode is left.
- **0 passes on red.**
- **Costs:**
  - the SUMO preset at or slightly above the switch off;
  - more jerk ticks than R7 (W74 four-leg 1,130 against 378);
  - at 1 s the prototype's delay is +12 s and its headway +0.26 s against the switch off, from the start rule.
- **Rows** (if R9): BA262–BA266 as §5. BA264 changes to "a delayed W74 car behind a standing car rests within 10⁻⁶
  m of `s0` and does not start until its gap fits rule 3". BA267 covers the start rule: held while the gap is short
  of the formula, and started on the first tick it fits.

**The session's recommendation is now R9:** it answers the crawl and clamps less than the switch off for every
model except the SUMO preset. R7 remains the choice with the fewest W74 clamps, but with the crawl.

## 7. The owner's choice and the implementation (D661)

The owner chose **R9** on 2026-11-25 ("เลือก R9 ทำเลย").

- **Engine** (`src/core/line_stop.cpp`, called from `stepSimulation`'s reaction-delay block). For a behaviour with
  `lineRest` and a delay `d > 0`:
  - `holdDelayedStart` runs before the decision enters the FIFO (rule 3, with τ = `d·dt`);
  - `applyDelayedLineStop` runs after `delayedAction`: the hold at the standstill gap (rule 2), then `applyLineStop`
    on the current state toward `line`, the nearest holding line or a vehicle at rest less the standstill gap
    (rule 1).
- **Against the measurement.** The committed code reproduces R9's rows of the evidence to the digit (four-leg and
  M2.6 at 0.5 s, the four-leg at 1 s, four behaviours). Runs without a delay keep D656's figures, and
  `multra-cli 42` is byte-identical.
- **Rows** (`tests/line_stop_tests.cpp`, `linestop`). Each scene first asserts its forcing: some tick applies well
  below the decision the FIFO handed over.

  | Row | Case |
  |---|---|
  | BA262 | A delayed prototype car at 14 m/s toward a red line rests exactly at it, with no clamp |
  | BA263 | A delayed W74 and W99 queue of four rests without a clamp. Each follower is at or within the start rule's room of its standstill gap: a delayed stop can end a few centimetres short, and a start that short does not fit the delay |
  | BA264 | At rest at its standstill gap behind a leader edging by 10 μm/s, a delayed W74 car neither moves nor clamps; the hold itself at 5·10⁻⁷ and 2·10⁻⁶ m |
  | BA265 | `reactionTime` 0 gives the ticks of an unset one, for every model |
  | BA266 | The measurement above (not a row) |
  | BA267 | The start rule at the threshold. In a run, a delayed prototype 0.1 m beyond its standstill gap waits until its gap fits its start, and starts sooner with the switch off |

  Each of these mutations fails a row:
  - no start rule;
  - the laws left delayed;
  - no hold;
  - no reapply;
  - the start rule without `lineRest`;
  - the start rule without its braking term.
- **Left:**
  - PROTOTYPE_QUEUE's P17 for undelayed prototypes, with its three scene forcings and BA197;
  - B, the softer profile;
  - the SUMO preset's clamps under a delay, at about the switch off's.
