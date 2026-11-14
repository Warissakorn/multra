# M6 — calibration options after the first runs, and the owner's choice (D544), 2026-11-13

Every M6 kind has a first comparable run and its diagnostics:
- S1: [first run](m6-s1-first-run.md) and [diagnostic](m6-s1-diagnostic.md);
- S2: [S2-c](m6-s2c-first-run.md), [S2-b/S2-a](m6-s2ab-first-run.md), [arrivals](m6-s2-arrivals.md) and [effective green](m6-s2-green.md);
- U: [first run](m6-u-first-run.md) and [follow-up split](m6-u-split.md).

D540 held one calibration decision until then. This sheet records the options the session
estimated and the owner's choice. **The effects below are arithmetic on the measured parts, not
runs.**

## 1. What the code says (read 2026-11-13)

- **Desired gap.** The prototype's desired gap is `standstill + (additive + multiplicative·f)·√v`,
  that is 2 + (2 + 3f)·√v from `default.json` (`src/core/following.cpp`).
  - At 0.1 m/s it is about 3.6 m, not the 2 m standstill.
  - The √v term is steep near 0, so a queue approaches a standing target asymptotically and creeps.
  - This gives S1's jam spacing of 7.95 m (paper 6.9 m).
  - It also gives U's Stop stand position, 3.2 m short of the line: `stopLineReach` is
    `desiredGap(0.1) + 0.1` (`src/core/conflicts.cpp`).
- **Approach to a standing target.** It follows `(gap − desired)/T² − closing/T` with `T` =
  `followingTime` = 1.5 s, a natural period of about 2πT ≈ 9 s. That is the order of U's 7–8 s
  move-up over the first 6 m.
- **The D258 floor.** It acts only in braking mode behind a leader at exactly 0 m/s, and a creeping
  queue is not at 0 m/s.

## 2. Options and estimated effects

| | Option | S1 (S1-a pass 1,944; S1-f miss −6.7 %) | S2-c (3/3 pass); S2-b/a (1/6, 0/6) | U (5/5 miss, `t_f` 13.7 s) | Cost |
|---|---|---|---|---|---|
| A | No calibration on M6 | unchanged | unchanged | unchanged | Keeps M6 a validation |
| B | `standstillDistance` 2 → 0.95 m (jam spacing 6.9 m) | about 2,035 veh/h: S1-a misses (+7 %), S1-f passes (−2 %) | capacity +4–5 %: S2-b/a misses grow; S2-c likely in band | `t_f` −1 to −2 s: miss | Data only; every shipped project moves, and M0 reopens (D369) |
| C | `followingTime` 1.5 → 1.0 s | above 2,083: both likely miss | large capacity rise: S2-b/a worse | `t_f` about 9–10 s: miss | Changes free flow and every stream |
| D | Engine: a follower behind a standing or near-standing target stops at its standstill gap (the D258 floor extended to following mode) | jam spacing 6.5 m, about 2,070 veh/h: S1-a misses (+9 %), S1-f passes | S2-b/a misses grow | `settle` removed: `t_f` about 10 s, miss | Code, with its own contract and gate rows |
| E | Engine, Stop only: the target is the line itself, and "at the line" is slower than about 0.5 m/s within 0.5 m | none | none | `t_f` about 9 s: miss (about −60 %) | Code; M3_CONTRACT §5 is amended |

- **No single lever closes U.** HCM's 3.3 s against the engine's 13.7 s is structural:
  - a queued follower restarts only after its leader leaves, with a 1.5 s response;
  - each vehicle must stand again at the line.
- **S1-a and S1-f pull apart.** The two references (1,900 and 2,083 veh/h) differ by 9.6 %. With
  ±5 % bands, only 1,979–1,995 veh/h passes both. Every lever that shortens the spacing trades
  S1-a for S1-f.

## 3. The owner's choice (2026-11-13)

**"ไม่ calibrate + sweep"**, the session's recommendation.
- **No calibration on M6.** Calibrating to a benchmark and grading on the same benchmark is
  circular. M6's misses stand as the known deviations of the uncalibrated engine.
- **D540's hold is closed** on that answer.
- **When calibration happens.** It waits for independent data: U-e's inD trajectories (R7) and
  Thai data (Q4 (d)).
- **The sweep.** B–E are measured in a pre-registered sensitivity sweep. A sweep figure decides
  nothing and is never adopted.
  - Phase 1 runs B and C ([M6_SWEEP](../reference/M6_SWEEP.md), D545).
  - Phase 2, D and E, needs engine switches, each with its own contract.
