# M6_S1_RUN — M6's first comparable run, kind S1 (D535)

Contract for the run that compares the engine's queue discharge with S1-a, S1-d and S1-f
([m6-benchmark-options](../evidence/m6-benchmark-options.md) §6). The three rows share one project
and one set of runs. Written before code and before any engine figure is seen; rows MS1–MS7 fail
first. The run validates only what its bands say (rule 4); a miss is reported, never tuned away.

## 1. The project

`data/projects/m6-s1-approach.traffic.json`, built by `tools/m6_s1_network.hpp` through the editor
commands (the four-leg pattern) and written by `multra-m6-s1-fixture`. Never edited by hand.

- **Network.** Left-hand drawing.
  - **Approach Link:** one lane, 3.5 m wide, straight and level, 301 m long.
  - **Signal head:** 1 m before the Link's end, so 300 m from the source to the stop line.
  - **Exit Link:** one lane, 100 m, reached by a straight Connector.
  - No parking, no bus stop, no lane change.
- **Signal.** One fixed-time controller and one group: cycle 90 s, green 0–40 s, amber 3 s, red for the rest.
- **Demand.** One route and one input: `car`, 1,200 veh/h over the whole run. Car only, so veh = pc.
- **Run.** dt 0.1 s, duration 4,500 s, evaluation warm-up 900 s, window end 4,500 s, cool-down 0.
  Random streams `perInput`. Driver behaviour `data/driver-behaviour/default.json` as shipped (no assignment).

## 2. The command

`multra-m6-s1-run <project> <data-dir> <compile_commands.json> <new-output-dir>`

- **Refusals.** The tool refuses an existing output directory (MS7) and a project that fails to compile. Exit status 1 with a message.
- **Runs.** Seeds 42–81, each a full run.
- **Discharge settings.** Window [0, 4,500], warm-up 900 s, type `car`, steady ranks 5–12 and startup ranks 1–4: `--discharge`'s semantics ([DISCHARGE](DISCHARGE.md)).
- **Outputs.** Each is written to the new directory:
  - `runs.jsonl`: one line per seed, with the run totals and `dischargeJson`'s cycles, crossings included.
  - `cycles.csv`: one row per cycle and seed, including unavailable cycles with their reason, and the crossing times of ranks 1–12 (empty where absent).
  - `summary.json`: §3's figures and §4's verdicts.
  - `metadata.json`, written **last** with `complete: true`. It holds the build commit, the compiler command, the tool sources' SHA-256, the input manifest (project, catalogs, behaviour) and every output's SHA-256. A directory without it is an incomplete run.

## 3. The reductions

A cycle is **usable** when `estimateDischarge` gives it a mean headway (empty `unavailableReason`). Then ranks 1–12 are all queued at Go and crossed in the cycle.

- **S1-a and S1-f's `s`.** Per seed:
  1. take `H` = the mean over usable cycles of `meanHeadwaySeconds` (ranks 5–12);
  2. compute `s_seed = 3600 / H`.

  A seed without a usable cycle has no `s_seed`; it is counted and named, never dropped silently. The figure is the mean of `s_seed` over seeds with `n`, `sd` and the 95 % half-width `tQuantile975(n − 1) · sd / √n` (`src/runner/batch.hpp`).
- **S1-d, the headway by rank.** For each usable cycle with crossing times `t[1..12]` and Go time `go`:
  - `h[1] = t[1] − go`;
  - `h[r] = t[r] − t[r−1]`.

  Per seed, take the mean over its usable cycles; then take the mean over seeds, with `n` and the half-width. A rank is **unavailable** when the usable cycles across all seeds number fewer than 30.
- **S1-f's profile.** For each rank `k`, the mean of `t[k] − go`, reduced the same way. It is shown against `t(k)`, the time at which [AkcBes02] Eq. 7 (PDF p. 4, image read) gives `n_s(t) = k`, where:
  - `n_s(t) = (q_n/3600)[(t − t_r) − (1 − e^{−m_q(t − t_r)})/m_q]`;
  - `q_n` = 2,086 veh/h, `m_q` = 0.369, `t_r` = 0 (Table 1's average, pp. 5–6).

  Solve it by bisection on [0, 200] s to 1e−9 s. The reference times:

  | `k` | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
  |---|---|---|---|---|---|---|---|---|---|---|---|---|
  | `t(k)` (s) | 3.759 | 5.849 | 7.731 | 9.533 | 11.297 | 13.043 | 14.779 | 16.510 | 18.239 | 19.966 | 21.693 | 23.419 |

- **Counts.** `summary.json` reports the cycles by `unavailableReason`, summed over seeds. The usable cycles and the per-seed `n` are reported beside every figure.

## 4. The bands and verdicts (D516, D532, D533)

A figure **passes** when `|mean − reference| ≤ 0.05 · reference`, edge included. Otherwise it **misses**. An unavailable figure is **unavailable**, never a pass.

| Figure | Reference | Verdict |
|---|---|---|
| S1-a `s` | 1,900 veh/h (HCM 7 Exhibit 19-11) | ±5 % |
| S1-a, beside | [Zegeer86] 1,875 veh/h | shown |
| S1-d ranks 2–9 | [LeeChen86] 40 mph: 2.46, 2.28, 2.17, 2.07, 2.00, 1.93, 1.91, 1.93 s | ±5 % each |
| S1-d ranks 1, 10–12 | [LeeChen86] 3.70, 1.78, 1.64, 1.76 s | shown |
| S1-d ranks 1–12 | [Bonneson92] Eq. 20 at `v` 16.8: 3.22 … 1.76 s | shown |
| S1-f `s` | 2,083 veh/h ([AkcBes02] Table 2) | ±5 % |
| S1-f profile | `t(k)` above | shown |

## 5. Acceptance rows

| Row | What must hold | Test |
|---|---|---|
| MS1 | `m6s1::approach()` serialised equals the committed `data/projects/m6-s1-approach.traffic.json`, and the project compiles with one controller, one head, one input and `perInput` | `m6s1` |
| MS2 | Two synthetic seeds: seed A has usable cycles with mean headways 1.8 and 2.0 s (`H` 1.9, `s` 1,894.74); seed B has one with 2.0 s (`s` 1,800). The figure is 1,847.37 with `n` 2, and a seed without usable cycles is listed and not in `n` | `m6s1` |
| MS3 | Synthetic crossings give the hand `h[r]` per rank and `t[k] − go`. With 29 usable cycles in total, every rank is unavailable; with 30, available | `m6s1` |
| MS4 | `t(k)` of Eq. 7 matches the §3 table within 1e−3 s for `k` = 1 … 12 | `m6s1` |
| MS5 | The verdicts: 1,995 against 1,900 passes (edge), 1,995.1 misses, 1,805 passes, 1,804.9 misses, and an absent figure is unavailable | `m6s1` |
| MS6 | Cycles with a reason are counted by reason in the summary and kept in `cycles.csv`'s rows; `usable + Σ reasons` = all cycles | `m6s1` |
| MS7 | The command refuses an existing output directory and prints usage for `--help` | `cli-m6-s1-run-help`, `cli-m6-s1-run-refused` |
| MS8 | An observer fed hand-built ticks records `standDistance`, `moveTime` and `lineSpeed` as §7 defines them, including a vehicle that stops twice before the line | `m6s1` |
| MS9 | §7's reduction on synthetic cycles gives hand values for `L_hj`, `t_x,measured`, `v_S`, `t_r` and the per-rank speeds | `m6s1` |
| MS10 | Synthetic cycles that obey Eq. 9 exactly give `t_x,Eq10 = t_x,measured` within 1e−9 s | `m6s1` |
| MS11 | A usable cycle with a rank 1–12 vehicle lacking a record is counted as `diagnosticUnavailable`, kept in `diagnostic.csv`, and left out of the diagnostic only | `m6s1` |

## 6. References

[AkcBes02], [LeeChen86], [Bonneson92], [Zegeer86] and HCM 7 as cited in
[m6-benchmark-options](../evidence/m6-benchmark-options.md#references).

## 7. The S1 diagnostic (D530, D536)

Used because S1-f missed its band ([m6-s1-first-run](../evidence/m6-s1-first-run.md)). It
**decides nothing and has no band** (D530). It splits the engine's saturation headway as
[AkcBes02] Eq. 9 does (PDF p. 8, image read): `h_S = t_x + 3.6 L_hj / v_S`.

- `t_x`: the queue departure response time;
- `L_hj`: the jam spacing, front to front, so it includes the vehicle length (Fig. 2, p. 4; Fig. 5, p. 9);
- `v_S`: the saturation speed, in km/h.

Eq. 10 (p. 9) gives `t_x = h_S − 3.6 L_hj / v_S`. Each part is shown against Table 2's Through
(isolated) average (p. 11): `t_x` 1.17 s, `L_hj` 6.9 m, `v_S` 45.1 km/h.

**The observer.** It reads every tick's `SimState` for each vehicle that has not yet crossed the line.
- The line is the signal head's route distance. The tool refuses a project whose head is not on its one route's first segment.
- A vehicle is **standing** when its speed is below 0.1 m/s.
- It keeps three values per vehicle:
  - **`standDistance`:** the front's route distance (`Vehicle::distance`) at the last standing tick before the crossing;
  - **`moveTime`:** the end-of-tick time of the first moving tick after that standing tick;
  - **`lineSpeed`:** the speed at the end of the tick in which the front first reaches the line, the same tick DISCHARGE stamps as the crossing.
- A vehicle that stops again before the line replaces both `standDistance` and `moveTime`.
- Records are matched to the discharge crossings by `vehicleId`.

**Per usable cycle (§3), over ranks 5–12, the ranks of `h_S`:**
- `L_hj` = the mean of `standDistance[r−1] − standDistance[r]`;
- `t_x,measured` = the mean of `moveTime[r] − moveTime[r−1]`;
- `v_S` = 3.6 × the mean of `lineSpeed[r]`.

Shown beside them:
- the per-rank `lineSpeed` for ranks 1–12, in km/h;
- `t_r` = `moveTime[1] − go`, the first vehicle's response.

A usable cycle in which any vehicle at ranks 1–12 lacks a record is counted as
`diagnosticUnavailable`. It stays in `diagnostic.csv` and still counts for §3.

**Reduction.** Per seed, take the mean over its cycles, then the mean over seeds with the 95 %
half-width, as §3. `t_x,Eq10` is per seed `H_seed − 3.6 L_hj,seed / v_S,seed`, with `H_seed` the
seed's §3 mean headway; it is then averaged over seeds. `t_x,measured` and `t_x,Eq10` are shown
side by side; how far apart they are tells how well Eq. 9 describes the engine.

**Stated beside the figures.** The paper's `v_S` is the fitted maximum queue discharge speed
(`v_n`, Table 1, p. 5); the engine's is the measured speed at the line over the same ranks as `h_S`,
and still rising there. Whatever the split shows, no model parameter is changed by this
contract. A change is its own decision, asked of the owner.

**Output.**
- `diagnostic.csv` has one row per seed, cycle and rank 1–12: `seed, go, rank, vehicleId, standDistance, moveTime, crossingTime, lineSpeed`, empty where there is no record.
- `summary.json` gains a `diagnostic` object.
- The tool's source hashes add `m6_s1_diagnostic.hpp`.

