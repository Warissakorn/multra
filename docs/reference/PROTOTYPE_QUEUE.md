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
| P7 | P1 plus a bound on the first tick's speed | 5 / 21 clamps; the crawl partly returns |
| P14 | P1, with a cap hit behind a slower leader finished as braking | 216 clamps |
| P16 | P1 plus a start rule at the leader's speed | Delay doubles (47.5 → 97.9 s) |
| **P17** | §2 | Clean at dt 0.1, 0.25 and 0.5 |
| P18 | P17 plus P7's bound | As P17, with higher delay at dt 0.5 |

**P17 against P0:**
- **Crawl:** four-leg median / p90 17.0 / 53.0 → 4.7 / 6.0 s, and vehicles crawling ≥ 10 s 56 % → 0.8 %. M2.6 is
  11.3 / 50.2 → 4.6 / 6.3 s.
- **Clamps:** 0.0 → 0.0 (four-leg), 0.3 → 0.2 (M2.6), and 0–0.5 per run on every project and timestep.
- **Delay:** four-leg +0.19 s, M2.6 +0.41 s, the T-junction +0.45 s (5.94 → 6.39), the priority lab +0.14 s, and the
  weaving lab −0.45 s.
- **Headway:** within 0.06 s at dt 0.1. At dt 0.5, +0.19 s (four-leg) and +0.21 s (M2.6).
- **Costs:**
  - jerk ticks 23 → 516 per run on the four-leg, 144 → 1,951 on M2.6;
  - moving hard braking +33 % (four-leg) and +29 % (M2.6). These are followers arriving behind a leader that the
    stop law brakes at a constant 2 m/s², a later stop than the prototype's law makes. That is LINE_STOP's
    profile, so it is B's question.

## 4. The session's recommendation

**P17, if the owner wants the prototype's queues to rest. Otherwise P0, with W74 or W99 recommended for
realism.**
- P17 is the only option measured that removes the crawl with no clamps at every timestep, and it adds no value.
- **Against it:**
  - The jerk ticks rise 13–22×. They are not traced yet. Before code, trace them, as D656 traced its hard braking.
    If they are the hand-back from the law to the model, rows must bound them.
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
| BA261 | The jerk trace's finding | Bounded as the trace finds |

## 6. The owner's choice

Open. Options: **P17** (the session's recommendation if the prototype is to rest), **P0** (leave it), or another.
