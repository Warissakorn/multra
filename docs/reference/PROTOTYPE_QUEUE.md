# The prototype behind a standing car: ending the crawl

Written 2026-11-25 (D657). LINE_QUEUE (D655–D656) stopped W74 and W99 queues at rest behind a standing car. It
left the prototype's crawl as its own question, because every option it measured traded the crawl for clamps.
The owner asked for this contract ("เรื่องการคืบของ prototype (เขียนสัญญาก่อน)").

This contract follows LINE_QUEUE's pattern:
- options are measured before code ([evidence](../evidence/prototype-queue-options.md));
- **the owner chooses** (§6);
- no engine change is committed with it.

Rule 4 applies: nothing here is validated, and the unvalidated marker stays.

## 1. The problem

Under `lineRest`, the prototype (a behaviour carrying `amberDeceleration`) behind a standing car is left to its
own law.

**It crawls without end** (four-leg, seeds 42–51): a median of 17 s per vehicle at 0.01–1 m/s, a p90 of 53 s,
and 56 % of vehicles crawling for 10 s or more.
- The law's desired gap is `s0 + k·√v`, with `k = additive + multiplicative · driverFactor` (about 5 by default).
- Near rest it settles where `gap − s0 ≈ k·√v`, so the gap closes as `k²/t` and never reaches `s0`. A car is still
  about 0.6 m short after 40 s.

**LINE_QUEUE's law alone clamps** (P1 = Q2: 303 per run on the four-leg, 1,100 on M2.6).
- The trace finds every clamp at a restart from an exact rest at `s0`, behind a leader edging off at mm/s.
- Once v > 0, `k·√v` exceeds the gap at once, because √v's slope is unbounded at 0. So the follower brakes, and it
  cannot shed its speed within the micrometres the cap leaves.
- Today's crawling follower never meets this state, since it is still about half a metre back.

**Constraint.** The prototype's law is M0's, and M0 is closed (D369). Anything here applies under `lineRest` only,
which bare M0 does not carry, and `multra-cli 42` must stay byte-identical.

## 2. The rule proposed (P17)

1. **The stop law behind a standing car.** As LINE_QUEUE §2 rules 1–4, with the prototype in scope: behind a
   vehicle at exactly 0 m/s, the vehicle stops with LINE_STOP §2's law at the leader's rear less
   `standstillDistance`. The law sees no leader, and at the target (within 10⁻⁶ m) the vehicle waits.
2. **A start rule.** A prototype vehicle at rest behind a vehicle, moving or not, does not accelerate while its
   gap is below its own desired gap at the behaviour's `speedThreshold`:
   `s0 + k·√speedThreshold`, 2 + 5·√0.2 ≈ 4.2 m by default.
   - It starts on the tick the gap allows it, and from then on its law governs as today.
   - The rule reads no timestep and adds no value. It gives `speedThreshold` a second meaning (§4).
3. **Scope.** `lineRest` and a prototype behaviour. W74 and W99 keep D656, and bare M0 and a prototype without
   `amberDeceleration` keep their bytes.

## 3. Options measured

| Option | What | Result |
|---|---|---|
| **P0** | Today | §1 |
| P1 | LINE_QUEUE Q2 for the prototype | Crawl gone; 303 / 1,100 clamps per run (four-leg / M2.6) |
| Q4 | P1 with D546's `standingTarget` | 49 / 208 clamps |
| R5 | P1 plus D258's floor behind a slower vehicle | 0.1 clamps, but thousands of hard brakes on the lane and weaving labs |
| P2 | P1 plus a start rule at the first tick's speed `a·dt` | Clean at dt 0.1. At dt 0.5, headway 2.115 → 2.832 s and delay +11 s |
| P6 | P2, with the law only for the model's final phase | 0 clamps; more severe braking (66 ticks below −6 m/s² vs 13) |
| P7L | P6's late law plus a bound on the first tick's speed | 5 / 21 clamps; the crawl partly returns |
| P14L | P6's late law, with a cap hit behind a slower leader finished as braking | 216 clamps |
| P16L | P6's late law plus a start rule at the leader's speed | Delay doubles (47.5 → 97.9 s) |
| P17L | P6's late law plus §2's start rule | Clean at dt 0.1, 0.25 and 0.5 |
| P18L | P17L plus P7L's bound | As P17L, with higher delay at dt 0.5 |
| **P17** | §2 (measured in D658) | Clean at dt 0.1, 0.25 and 0.5 |

**Corrected in D658.** D657's table named the late-law variants P7, P14, P16, P17 and P18 (the probe's P6 branch ran
for all of them), and its "P17 against P0" figures were P17L's. The figures below are P17's as §2 writes it
([evidence](../evidence/prototype-queue-options.md#correction-d658)).

**P17 against P0:**
- **Crawl:** four-leg median / p90 17.0 / 53.0 → 1.3 / 1.4 s, and vehicles crawling ≥ 10 s 56 % → 0.3 %. M2.6 is
  11.3 / 50.2 → 1.3 / 1.6 s.
- **Clamps:** 0.0 → 0.0 (four-leg), 0.3 → 0.3 (M2.6), and at most 0.8 per run on every project and timestep.
- **Delay:** four-leg +0.21 s, M2.6 +0.47 s, the T-junction +0.44 s (5.94 → 6.38), the priority lab +0.11 s, and the
  weaving lab −0.45 s.
- **Headway:** within 0.06 s at dt 0.1. At dt 0.5, +0.19 s (four-leg) and +0.21 s (M2.6).
- **Costs:**
  - jerk ticks 23 → 296 per run on the four-leg, 144 → 1,170 on M2.6. They are the stop law's bang-bang profile
    (accelerate to the comfortable-braking curve, then brake on it), not the hand-back (§6);
  - moving hard braking 417 → 1,027 per run on the four-leg, 1,841 → 3,814 on M2.6, mostly −3 to −4 m/s². Below
    −6 m/s² it is 13 → 20 on the four-leg. These are followers arriving behind a leader that the stop law brakes at
    a constant 2 m/s², later than the prototype's law. That is LINE_STOP's profile, so it is B's question.

## 4. The session's recommendation

**P17, if the owner wants the prototype's queues to rest. Otherwise P0, with W74 or W99 recommended for
realism.**
- P17 is the only option measured that removes the crawl with no clamps at every timestep, and it adds no value.
- **Against it:**
  - The jerk ticks rise 8–13× (D657 said 13–22×, from P17L's figures). D658 traced them to the stop law's profile,
    not the hand-back (§6).
  - `speedThreshold` gains a second meaning. Today it is the closing speed that starts the approaching mode. Under
    P17 it is also the speed whose gap a vehicle at rest waits for. Its value 0.2 m/s is the behaviour file's and
    is uncited, like every prototype value.
  - At dt 0.5, headway rises 0.2 s.
- **For P0:** the prototype is a reduced Wiedemann-inspired law, NOT W74/W99 (SIMULATION). Its crawl is the law's
  own, and W74 and W99 now rest (D656).
- **B comes after either choice,** since the stop law's late constant braking is a cost for every model.

## 5. Rows (for the code session, if P17)

| Row | Case | Asserts |
|---|---|---|
| BA257 | A prototype queue of four behind a red | Every follower at exactly 0 m/s within 10⁻⁶ m of `standstillDistance`; no crawl over 1 s; no clamp. Forcing: P0 crawls |
| BA258 | At rest behind a leader that moves off | No acceleration while the gap is below `s0 + k·√speedThreshold`; it accelerates the tick the gap reaches it |
| BA259 | The four-leg at dt 0.1, 0.25 and 0.5 | No clamp in the queue scene; headway within the measured band |
| BA260 | Scope: a prototype without `amberDeceleration`, W74 and W99 | Ticks unchanged; `multra-cli 42` byte-identical |
| BA261 | The jerk trace's finding | Not needed: the trace found the stop law's profile, not the hand-back (§6) |

In D658's attempt BA259 ran the queue scene at dt 0.25 and 0.5, with a crawl bound of 2 s at dt 0.5 (the law's
last braking ticks below 1 m/s alone count 2 s there). BA260 also asserted that M0's law starts before the gap.

## 6. The owner's choice and the first attempt (D658, open)

The owner chose **P17** on 2026-11-25 ("P17"). That choice was made on D657's figures, which were P17L's (§3).

- **The jerk trace (§4).** The trace is the stop law's profile behind a car at exactly 0 m/s:
  - free acceleration followed by the law's braking (1,485 of 2,965 ticks);
  - the model's approach followed by the law's acceleration (1,320).

  It is not the hand-back, so BA261 is not needed. The profile is B's question.
- **The attempt** ([patch](../archive/patches/prototype-queue-attempt.patch), not committed).
  - The engine: `queueStop` became `lineRest`, and the start rule came after rule 3's hold.
  - Rows BA257–BA260 were written, and BA241's prototype row amended. Each of four mutations failed a row.
  - `multra-cli 42` was byte-identical.
- **The full `check` failed five groups** ([evidence](../evidence/prototype-queue-options.md#p17-as-written-and-the-attempt-d658)):
  - **`lab` d217: 4 clamps on the circuit with `reactionTime` 0.5 s.** Behind a car that has just stopped, the
    law's decision is acted on 0.5 s late, and the stop at exactly `s0` leaves no margin for it.
  - **Three forcings lost:**
    - `tjunction_signal`'s congested-major headway ticks (91 → 0);
    - BA248's follow-up (1 → 0);
    - BA107's 0.2 s slowed-lane effect on one seed.

    The law alone (P1) already moves each of them.
  - **`weavinglab` BA197:** served 372 → 357, from the start rule.
- **The reaction delay is a gap in both contracts, and it reaches D656.** With every behaviour's `reactionTime`
  at 0.5 s on the four-leg, W74 clamps 755 per run with `lineRest` on, against 23 with it off. No shipped project
  sets `reactionTime`.
- **Choices before a second attempt:**
  1. **The reaction delay first, for every model** (recommended). A contract amendment on how the stop law behind
     a vehicle at rest meets a reaction delay, measured for W74, W99 and the prototype. It may fix D656's 755 clamps
     too. Options to measure include:
     - a vehicle with a reaction delay keeps its model behind a standing car;
     - the law reads the standing leader without the delay.
  2. **P17 without reaction delays.** Scope P17 to behaviours without `reactionTime`, amend the three forcings'
     scenes, and accept BA197's 4 % fewer served vehicles.
  3. **Leave the prototype (P0)** and do 1 for W74 and W99 only.

**The owner chose 1** ("ตามแนะนำ"): [REACTION_STOP](REACTION_STOP.md) (D659) contracts the reaction delay for every
model. P17 waits for its choice.

## 7. The implementation (D662)

The owner asked for P17 to go on after D661 ("ทำ P17 ต่อเลย").

- **Engine** (`src/core/simulation.cpp`), as the D658 attempt:
  - `queueStop` is `lineRest` for every model;
  - the start rule (§2 rule 2) follows LINE_QUEUE's hold.

  On D661, the attempt's `lab` d217 clamps are gone: they were under a reaction delay, which is R9's scope.
- **Against the measurement.** Runs without a delay reproduce P17 to the digit. W74, W99 and `multra-cli 42` are
  unchanged.
  - With a delay, P17 and R9 together clamp below the switch off: 0.5 / 3.1 per run at 0.5 s (four-leg / M2.6),
    against 12.1 / 51.2.
  - At 1 s the prototype's delay is +28 s against the switch off, because the two start rules add up
    ([evidence](../evidence/prototype-queue-options.md#after-d662-p17-committed-on-d661)).
- **Rows.** BA257–BA260 in `linestop`, with BA241's prototype row amended, as the attempt (§6). Each of four
  mutations fails a row.
- **The three lost forcings, settled:**
  - **`tjunction_signal`, the congested-major forcing.** The variant's head and phases are pinned by the archived
    M3.2.7c evidence, which was measured before `lineRest` existed. Under P17 its queue rests 6 m per car and ends
    about 2 m short of the crossing entry. The row now runs the archived variant with `lineRest` off. Under
    `lineRest` this variant no longer exercises the headway arm.
  - **BA248.** Under P17 the minor follower brakes late. It stood queued at 9.00 s, after the leader left at
    8.20 s, so no follow-up existed. A fourth major car now holds the leader longer. The row passes with and
    without P17.
  - **BA107.** The slowed lane's effect is 0.2 s on one seed. The row now compares the mean over seeds 42–46 (+0.14 s
    with P17, +0.14 s without).
- **BA197:** served 372 → 357, from the start rule. Its floor is revised to 350, with the reason in the row. Its
  clamps stay 0.
- **Left:** B, the stop's profile, which is also the source of P17's hard braking and jerk (§3).

