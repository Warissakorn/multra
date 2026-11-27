# RULE_LOOKBACK — how far upstream a priority rule looks (contract)

Written 2026-11-26 (D674). It is a contract with measured options; **nothing is implemented**. The owner asked for
D672's E2: "เขียน contract E2 และวัดทางเลือก". The measurements are in
[rule-lookback-options](../evidence/rule-lookback-options.md).

## 1. Today

- [PRIORITY_RULES](PRIORITY_RULES.md) §1: a rule holds the minor vehicle while "a major vehicle on the marker's
  segment" is over the marker, within `headway` short of it, or reaches it in less than `gapTime`.
- So a major car upstream of the marker's segment is invisible to the rule. On a short straight connector (2.73 m
  on `signalised-node-analysis`), an approaching car is seen only when it is about to reach the point. D672 traced a
  clamp to this.
- D673 (E1) settled the boundary: a front exactly at the segment's start has not entered it.

## 2. What Vissim does (manual, read 2026-11-26)

- **"Creating priority rules"** (2023 page): the minimum gap time is "the time that the first upstream vehicle will
  require in order to reach the green bar of the conflicting marker with its present speed". The worked example has
  a vehicle 49 m upstream. The marker "also detects vehicles on all connectors, which lead upstream of the green bar
  onto the link". No distance limit is stated.
- **"Attributes of priority rules"** (2024 page):
  - *Look beyond red signals*: when selected, "vehicles traveling upstream of a red signal are also observed by the
    conflict marker"; when not, "the time gaps and min. clearances are only checked up to the Red signal
    controller". The page gives no default.
  - The defaults are gap time 3.0 s and minimum clearance 5.0 m.

## 3. The rule proposed

1. **Upstream along the major vehicle's route.** A major vehicle whose route reaches the marker's segment, with its
   front still upstream of that segment, is also tested.
   - Its reach is the route distance from its front to the marker.
   - The tests are today's: headway (with `maxSpeed`), gap time, and the class filter.
   - A standing vehicle beyond the headway does not block, as today.
   - There is no new distance limit: the gap-time test bounds who can block, as in Vissim.
2. **A red signal ends the look** (option G1R). A major vehicle with a signal head that is not green between its
   front and the marker is not tested. This is Vissim with *Look beyond red signals* off.
3. **No new key and no value.** Spans on the marker's segment are scanned as today (with E1).

## 4. Options measured (seeds 42–51, `lineRest` on, reverted patch)

| Option | What | Measured |
|---|---|---|
| **G0** | Today (with E1, D673) | Baseline |
| **G1** | §3 rule 1, looking past red signals | Node analysis worsens: delay +7.5 s (prototype), +11.3 s (W74), +13.7 s (W99); W74 clamps 15.7 → 17.7, W99 12.1 → 14.4. M2.6 as G1R. Others unchanged |
| **G1R** | §3 rules 1–2 | **Node analysis, four-leg, the T-junction and the three labs are identical to G0.** M2.6: prototype clamps 0.2 → 0, W74 29.5 → 28.5, W99 hard braking 646 → 603 per run; delay +0.06 s (+0.1 %) |

- **The remaining W74 and W99 clamps on node analysis (15.7, 12.1 per run) do not come from E2.** G1R does not move
  them. They are the line and queue clamps those models carry elsewhere (LINE_QUEUE, REACTION_STOP).
- **Cost not measured.** The prototype scans every vehicle for each rule a minor vehicle meets, so it is O(n) per
  test. Wall times from parallel runs were too noisy to read. An implementation would index major vehicles by the
  segment before the marker, and needs callgrind before and after, as D495 asks for the hot path.

## 5. The session's recommendation

**G1R**, built with an index and measured with callgrind first.
- It removes a blind spot that D672 proved can release a minor vehicle into an approaching car.
- It follows Vissim's definition ("the first upstream vehicle", stopping at a red signal).
- It changes nothing where no such approach exists, and its measured gain today is small: M2.6 only.
- If the cost is above the noise floor for that gain, keep G0 and record the blind spot as a known limit.

G1 is not recommended: it holds turners behind major cars that are stopped at their red.

## 6. Rows (for the code session, once chosen)

| Row | What | Pass |
|---|---|---|
| PR18 | A major car upstream of a 3 m marker segment, 1.5 s away | Holds the minor; today it goes (the forcing) |
| PR19 | The same car with a red head between it and the marker | Does not hold (G1R) |
| PR20 | A standing major car upstream, beyond the headway | Does not hold |
| PR21 | Every shipped project except M2.6 | Byte-identical events to G0 |

## 7. G1R built and measured; kept out (D675)

The owner chose the recommendation ("ตามแนะนำ"): G1R with an index, measured with callgrind first, and G0 if the cost
is above the gain.
- **Built** ([patch](../archive/patches/rule-lookback-g1r.patch), not committed to the engine): `src/core/rule_lookback.cpp`.
  - Per rule, the index holds the routes through its conflict segment and where each reaches it.
  - Per tick, the vehicles grouped by route are built lazily, as one flat array.
  - A per-rule cache per tick holds the answer for rules without a gap reduction.
  - The output is identical to the measuring patch's on both projects.
- **Callgrind** (Release, GCC 13.3, one run, seed 42, `multra-cli --project`):

  | Project | Today (Ir) | G1R (Ir) | Δ |
  |---|---|---|---|
  | M2.6 | 4,353,654,959 | 4,497,851,028 | +3.31 % |
  | node analysis | 2,774,360,638 | 2,824,386,143 | +1.80 % |

  - A first version that built per-route vectors each tick cost +10.1 % and +5.1 %.
  - The cache did not lower the cost: each rule is asked by about one vehicle per tick.
  - What remains is the scan itself (about 2 %) and the grouping (about 1 %).
- **Against the gain** (§4: only M2.6 changes; prototype clamps 0.2 → 0, W74 29.5 → 28.5, W99 hard braking
  646 → 603), the cost is above it.
- **So G0 stays.** The blind spot is a **known limit**: a priority rule sees only major vehicles on its conflict
  segment. On a short straight connector, an approaching major car is seen only once it is on it.
- **To reopen:** a study whose short connectors release minor vehicles into approaching cars, or a cheaper index.
  An example of a cheaper index is to scan only vehicles on the segment just before the conflict segment, which
  needs its own measurement. The patch applies to `05a28fd`.

