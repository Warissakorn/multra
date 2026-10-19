# M3.2.8a.1b: anticipation across a join, prototypes measured — 2026-10-19

Branch `claude/red-running-owner-choices`, on `118df3c` (D252). Linux, GCC 13.3.0, Release `-g`
in a scratch directory. Contract and options: [JOIN_ANTICIPATION](../reference/JOIN_ANTICIPATION.md)
(D253). **No engine code is committed.** Every variant was a temporary patch of
`src/core/simulation.cpp`, built and reverted (D232's, D241's and D249's method). Each patch also
carried D249's span-pair overlap counter and a clamp print.

## What was tried

The starting point is D251's defaults, with `clampRules` all on. M2.6 keeps 17 clamps there:
pull-out (J1) 9, late catch-up (J2) 4, merge creep (M) 3, and 1 new
([MERGE_CLAMPS](../reference/MERGE_CLAMPS.md) §1).

**Shared by every variant:**
- A *join* is a segment with two or more predecessors.
- Each tick lists every vehicle whose front is within 50 m (`R`) of a join on its route, at
  distance `d`.
- For vehicle X, a vehicle Y on another predecessor of the same join leads it when
  `dY < dX`, ties to the lower id. Y's rear is then projected onto X's route, so the gap is
  `dX − (dY + L_Y)`, at Y's speed and acceleration.
- The hard cap, receiving space and zones never see Y.

| Variant | Which Y count | How Y reaches X's following |
|---|---|---|
| A1 | Past its own give-way line, or on an approach with no rule toward the join | Replaces the following leader when nearer. A gap ≤ 0 is passed as 0 |
| A2 | Moving and not held by its rule on the previous tick | Same as A1 |
| A3 | A1's rule, plus: its front is on the join's own predecessor segment, it is moving, and the projected gap is positive | Same as A1 |
| **A4** | A3's rule | **A second following law, run against Y. The stricter acceleration of the two is kept**, as the courtesy hold already does |
| A1J, A3J, A4J | The same, plus D249's J: a vehicle that its rule held on the previous tick is not `committed` to that rule | — |

## Results

Seeds 42–81 on all four projects. "Seeds changed" is against D251's defaults (`e926bb6`).

| Variant | Four-leg clamps / delay | T-junction clamps / delay | M2.6 clamps / delay | M2.6 overlaps | Seeds changed (four-leg, T, M2.6) |
|---|---|---|---|---|---|
| D251 | 0 / 46.001 | 0 / 7.069 | 17 / 46.010 | 0 | — |
| A1 | 3317 / 74.9 | 84 / 92.3 | 7557 / 165.7 | 0 | 40, 40, 40 |
| A2 | 3858 / 61.2 | 180 / 8.5 | 9552 / 59.2 | 0 | 40, 40, 40 |
| A1J | 3319 / 74.9 | 84 / 92.3 | 7630 / 165.7 | 0 | 40, 40, 40 |
| A3 | 304 / 46.000 | 56 / 7.302 | 177 / 45.998 | **12** | 40, 40, 40 |
| A3J | 304 / 46.000 | 56 / 7.302 | 195 / 45.995 | 0 | 40, 40, 40 |
| **A4** | **0 / 46.001 (identical)** | 0 / **7.545** (±0.558) | **16** / 46.014 | 0 | 0, 40, 36 |
| A4J | 0 / 46.001 | 0 / 7.545 | 23 / 46.011 | 0 | 0, 40, 37 |

- **Unchanged in every variant:** the lane-change lab (1 clamp, identical) and `trafficsim-cli 42`.
- **A4's other checks:** the TypeScript reference rows pass 5 of 5, and all 949 `trafficsim-tests`
  rows pass under it.

### Why A1–A3 fail

- **Phantom leaders.** A1 and A2 count a vehicle standing at a red head up to 50 m up the other
  approach as "first to the join". Green traffic then stops behind it, and the queues clamp.
  - Example: the four-leg's turns into an exit are joins with no rule (the signals separate them).
  - A2 also counts every minor creeping at its line.
- **Side by side.** A gap ≤ 0, two vehicles alongside, became an emergency stop.
- **A nearer but faster virtual leader.** In A1–A3 it *replaced* the real leader, so a vehicle
  followed Y's speed and accelerated into its slower real leader. That is A3's 177 clamps and 12
  overlaps. A4 keeps the stricter of the two laws and has neither.

### What A4 does

- **M2.6 17 → 16.** The changes are:

  | Class | D251 | A4 |
  |---|---|---|
  | J1 (pull-out) | 9 | 5 |
  | J2 (late catch-up) | 4 | 1 |
  | M (merge creep) | 3 | 3 |
  | new (at other ticks or vehicles) | 1 | 7 |

  Mean delay moves by less than its half-width, and no movement moves by more than 0.03 s.
- **The T-junction's delay shifts from the minor road to the major road:**

  | Movement | D251 (s) | A4 (s) |
  |---|---|---|
  | Major eastbound | 4.23 | 5.00 |
  | Major westbound | 4.13 | 5.18 |
  | Minor → Major westbound | 25.13 | 23.61 |
  | Minor → Major eastbound | 15.03 | 13.29 |
  | Mean | 7.069 (±0.484) | 7.545 (±0.558) |

  Majors now ease off for a minor that has pulled out ahead of them, and they did not before.
- **The four-leg is byte-identical.** Its joins meet only on signal-separated movements. Those
  vehicles are either held at their heads or arrive well apart, so no Y passes A3's filter in time.
- **J (held means not committed)** turns the remaining pull-outs into other clamps (A4J: 23), as in
  D249. A minor that has crept onto its line cannot stop in about 0 m.

**Cost.** Ir was not measured. No variant is recommended, and every prototype carried the
overlap counter, which dominates its cost.

## Not covered here

- A larger or smaller `R`, other tie rules, or anticipation for lane changes.
- The line approach itself: D227's braking floor and how a minor stops at its line. J1 and the 7 new
  clamps point there.

## D255: the switch implemented

The owner chose A4 as `clampRules.joinAnticipation`, off by default (D254). Branch
`claude/red-running-owner-choices`, on `c72d73a` (D253). Linux, GCC 13.3.0, Release `-g`.

- **On reproduces the prototype.** A scratch build with the default flipped to on gives outputs
  byte-identical to D253's A4 build on seeds 42–81 of all four shipped projects (M2.6 16 clamps,
  delay 46.014 s; T-junction 7.545 s; four-leg 46.001 s; lane lab unchanged).
- **Off is today.** The committed default gives outputs byte-identical to D251's
  (`e926bb6`) on the same 160 runs, and `trafficsim-cli 42` keeps its pin.
- **Cost off.** Callgrind Ir at seed 42 against D251's build: four-leg +0.16 %, lane lab +0.11 %,
  M2.6 +0.00 %. The step tests one empty vector per vehicle.
- **Mutations** of `src/core/join_anticipation.cpp`, each run against BA168–BA172:

  | Mutation | Caught by |
  |---|---|
  | No held-upstream test | BA169 |
  | No positive-gap test | BA171 |
  | No moving test | BA169 |
  | No "front on the predecessor" test | BA169 |
  | Replace the following result instead of keeping the stricter | BA170 |
  | Never apply the second law | BA168 |
  | No "nearer the join" test | survived: a positive gap implies it, so the test was removed |
